/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/shared/config.h>
#include <game/mapitems.h>

#include <game/generated/protocol.h>

#include "entities/area-flag.h"
#include "entities/vehicle/aircraft.h"
#include "entities/vehicle/car.h"
#include "entities/vehicle/helicopter.h"
#include "entities/vehicle/jet.h"
#include "entities/vehicle/tank.h"
#include "entities/vehicle/vehicle_util.h"
#include "entities/workbench.h"
#include "battle.h"
#include "door.h"
#include "gamecontroller.h"
#include "gamecontext.h"
#include "player.h"

#include <engine/storage.h>
#include <engine/shared/linereader.h>

CGameControllerWorkbenches::CGameControllerWorkbenches(class CGameContext *pGameServer)
{
	m_pGameServer = pGameServer;
	m_pServer = m_pGameServer->Server();
	m_pGameType = "Workbenches"; // Set the gametype to Alchemy by default

	//
	DoWarmup(g_Config.m_SvWarmup);
	m_UnpauseTimer = 0;
	m_GameOverTick = -1;
	m_SuddenDeath = 0;
	m_RoundStartTick = Server()->Tick();
	m_RoundCount = 0;
	m_GameFlags = GAMEFLAG_TEAMS;
	m_aMapWish[0] = 0;

	m_UnbalancedTick = -1;
	m_ForceBalanced = false;

	m_aNumSpawnPoints[0] = 0;
	m_aNumSpawnPoints[1] = 0;
	m_aNumSpawnPoints[2] = 0;

	mem_zero(m_apWorkbenches, sizeof(m_apWorkbenches));
	m_NumFlag = 0;

	for (int Team = 0; Team < 2; Team++)
	{
		for (int i = 0; i < NUM_RESOURCE; i++)
			m_aTeamResources[Team][i] = 0;

		for (int i = 0; i < NUM_BUILDING; i++)
			m_aTeamBuildings[Team][i] = 0;

		m_aTeamMoney[Team] = 0;
	}

	for (unsigned int i = 0; i < sizeof(m_Switches); i++)
		m_Switches[i] = false;

	/*GameServer()->Collision()->GenerateWaypoints();

	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), "%d waypoints generated, %d connections created", GameServer()->Collision()->WaypointCount(), GameServer()->Collision()->ConnectionCount());
	GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "cstt", aBuf);*/

	LoadMapConfig();
}

CGameControllerWorkbenches::~CGameControllerWorkbenches() {}

float CGameControllerWorkbenches::EvaluateSpawnPos(CSpawnEval *pEval, vec2 Pos)
{
	float Score = 0.0f;
	CCharacter *pC = static_cast<CCharacter *>(GameServer()->m_World.FindFirst(CGameWorld::ENTTYPE_CHARACTER));
	for (; pC; pC = (CCharacter *)pC->TypeNext())
	{
		// team mates are not as dangerous as enemies
		float Scoremod = 1.0f;
		if (pEval->m_FriendlyTeam != -1 && pC->GetPlayer()->GetTeam() == pEval->m_FriendlyTeam)
			Scoremod = 0.5f;

		float d = distance(Pos, pC->GetPos());
		Score += Scoremod * (d == 0 ? 1000000000.0f : 1.0f / d);
	}

	return Score;
}

void CGameControllerWorkbenches::EvaluateSpawnType(CSpawnEval *pEval, int Type)
{
	// get spawn point
	for (int i = 0; i < m_aNumSpawnPoints[Type]; i++)
	{
		// check if the position is occupado
		CCharacter *aEnts[MAX_CLIENTS];
		int Num = GameServer()->m_World.FindEntities(m_aaSpawnPoints[Type][i], 64, (CEntity **)aEnts, MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);
		vec2 Positions[5] = {vec2(0.0f, 0.0f), vec2(-32.0f, 0.0f), vec2(0.0f, -32.0f), vec2(32.0f, 0.0f), vec2(0.0f, 32.0f)}; // start, left, up, right, down
		int Result = -1;
		for (int Index = 0; Index < 5 && Result == -1; ++Index)
		{
			Result = Index;
			for (int c = 0; c < Num; ++c)
				if (GameServer()->Collision()->CheckPoint(m_aaSpawnPoints[Type][i] + Positions[Index]) ||
					distance(aEnts[c]->GetPos(), m_aaSpawnPoints[Type][i] + Positions[Index]) <= aEnts[c]->GetProximityRadius())
				{
					Result = -1;
					break;
				}
		}
		if (Result == -1)
			continue; // try next spawn point

		vec2 P = m_aaSpawnPoints[Type][i] + Positions[Result];
		float S = EvaluateSpawnPos(pEval, P);
		if (!pEval->m_Got || pEval->m_Score > S)
		{
			pEval->m_Got = true;
			pEval->m_Score = S;
			pEval->m_Pos = P;
		}
	}
}

bool CGameControllerWorkbenches::CanSpawn(int Team, vec2 *pOutPos)
{
	CSpawnEval Eval;

	// spectators can't spawn
	if (Team == TEAM_SPECTATORS)
		return false;

	if (IsTeamplay())
	{
		Eval.m_FriendlyTeam = Team;

		// first try own team spawn, then normal spawn and then enemy
		EvaluateSpawnType(&Eval, 1 + (Team & 1));
		if (!Eval.m_Got)
		{
			EvaluateSpawnType(&Eval, 0);
			if (!Eval.m_Got)
				EvaluateSpawnType(&Eval, 1 + ((Team + 1) & 1));
		}
	}
	else
	{
		EvaluateSpawnType(&Eval, 0);
		EvaluateSpawnType(&Eval, 1);
		EvaluateSpawnType(&Eval, 2);
	}

	*pOutPos = Eval.m_Pos;
	return Eval.m_Got;
}

bool CGameControllerWorkbenches::OnEntity(int Index, vec2 Pos)
{
	switch (Index)
	{
	case ENTITY_SPAWN:
		m_aaSpawnPoints[TEAM_RED][m_aNumSpawnPoints[TEAM_RED]++] = Pos;
		break;
	case ENTITY_SPAWN_RED:
		m_aaSpawnPoints[TEAM_RED + 1][m_aNumSpawnPoints[TEAM_RED + 1]++] = Pos;
		break;
	case ENTITY_SPAWN_BLUE:
		m_aaSpawnPoints[TEAM_BLUE + 1][m_aNumSpawnPoints[TEAM_BLUE + 1]++] = Pos;
		break;
	default:
		if (BattleIsEnabled())
			return BattleOnMapEntity(Index, Pos, GameServer(), this);
		break;
	}

	return false;
}

void CGameControllerWorkbenches::EndRound()
{
	if (m_Warmup) // game can't end when we are running warmup
		return;

	GameServer()->m_World.m_Paused = true;
	m_GameOverTick = Server()->Tick();
	m_SuddenDeath = 0;
}

void CGameControllerWorkbenches::ResetGame()
{
	GameServer()->m_World.m_ResetRequested = true;
	for (int i = 0; i < m_lDoors.size(); i++)
		m_lDoors[i].Reset();
}

const char *CGameControllerWorkbenches::GetTeamName(int Team)
{
	if (IsTeamplay())
	{
		if (Team == TEAM_RED)
			return "red team";
		else if (Team == TEAM_BLUE)
			return "blue team";
	}
	else
	{
		if (Team == 0)
			return "game";
	}

	return "spectators";
}

static bool IsSeparator(char c) { return c == ';' || c == ' ' || c == ',' || c == '\t'; }

void CGameControllerWorkbenches::StartRound()
{
	ResetGame();

	m_RoundStartTick = Server()->Tick();
	m_SuddenDeath = 0;
	m_GameOverTick = -1;
	GameServer()->m_World.m_Paused = false;
	m_ForceBalanced = false;

	for (int Team = 0; Team < 2; Team++)
	{
		for (int i = 0; i < NUM_RESOURCE; i++)
			m_aTeamResources[Team][i] = 0;

		for (int i = 0; i < NUM_BUILDING; i++)
			m_aTeamBuildings[Team][i] = 0;

		m_aTeamMoney[Team] = 0;
		if (m_apWorkbenches[Team])
			m_apWorkbenches[Team]->Init();
	}

	Server()->DemoRecorder_HandleAutoStart();
	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "start round type='%s' teamplay='%d'", m_pGameType, m_GameFlags & GAMEFLAG_TEAMS);
	GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);
}

void CGameControllerWorkbenches::CycleMap()
{
	if (m_aMapWish[0] != 0)
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "rotating map to %s", m_aMapWish);
		GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);
		str_copy(g_Config.m_SvMap, m_aMapWish, sizeof(g_Config.m_SvMap));
		m_aMapWish[0] = 0;
		m_RoundCount = 0;
		return;
	}
	if (!str_length(g_Config.m_SvMaprotation))
		return;

	if (m_RoundCount < g_Config.m_SvRoundsPerMap - 1)
	{
		if (g_Config.m_SvRoundSwap)
			GameServer()->SwapTeams();
		return;
	}

	// handle maprotation
	const char *pMapRotation = g_Config.m_SvMaprotation;
	const char *pCurrentMap = " ";

	int CurrentMapLen = str_length(pCurrentMap);
	const char *pNextMap = pMapRotation;
	while (*pNextMap)
	{
		int WordLen = 0;
		while (pNextMap[WordLen] && !IsSeparator(pNextMap[WordLen]))
			WordLen++;

		if (WordLen == CurrentMapLen && str_comp_num(pNextMap, pCurrentMap, CurrentMapLen) == 0)
		{
			// map found
			pNextMap += CurrentMapLen;
			while (*pNextMap && IsSeparator(*pNextMap))
				pNextMap++;

			break;
		}

		pNextMap++;
	}

	// restart rotation
	if (pNextMap[0] == 0)
		pNextMap = pMapRotation;

	// cut out the next map
	char aBuf[512] = {0};
	for (int i = 0; i < 511; i++)
	{
		aBuf[i] = pNextMap[i];
		if (IsSeparator(pNextMap[i]) || pNextMap[i] == 0)
		{
			aBuf[i] = 0;
			break;
		}
	}

	// skip spaces
	int i = 0;
	while (IsSeparator(aBuf[i]))
		i++;

	m_RoundCount = 0;

	char aBufMsg[256];
	str_format(aBufMsg, sizeof(aBufMsg), "rotating map to %s", &aBuf[i]);
	GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);
	str_copy(g_Config.m_SvMap, &aBuf[i], sizeof(g_Config.m_SvMap));
}

void CGameControllerWorkbenches::PostReset()
{
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (GameServer()->m_apPlayers[i])
		{
			GameServer()->m_apPlayers[i]->Respawn();
			GameServer()->m_apPlayers[i]->m_Score = 0;
			GameServer()->m_apPlayers[i]->m_ScoreStartTick = Server()->Tick();
			GameServer()->m_apPlayers[i]->m_RespawnTick = Server()->Tick() + Server()->TickSpeed() / 2;
		}
	}
}

void CGameControllerWorkbenches::OnPlayerInfoChange(class CPlayer *pP)
{
	const int aTeamColors[2] = {65387, 10223467};
	if (IsTeamplay())
	{
		pP->m_TeeInfos.m_UseCustomColor = 1;
		if (pP->GetTeam() >= TEAM_RED && pP->GetTeam() <= TEAM_BLUE)
		{
			pP->m_TeeInfos.m_ColorBody = aTeamColors[pP->GetTeam()];
			pP->m_TeeInfos.m_ColorFeet = aTeamColors[pP->GetTeam()];
		}
		else
		{
			pP->m_TeeInfos.m_ColorBody = 12895054;
			pP->m_TeeInfos.m_ColorFeet = 12895054;
		}
	}
}

int CGameControllerWorkbenches::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon)
{
	// do scoreing
	if (!pKiller || Weapon == WEAPON_GAME)
		return 0;

	if (pKiller == pVictim->GetPlayer())
		pVictim->GetPlayer()->m_Score--; // suicide
	else
	{
		if (IsTeamplay() && pVictim->GetPlayer()->GetTeam() == pKiller->GetTeam())
			pKiller->m_Score--; // teamkill
		else
			pKiller->m_Score++; // normal kill
	}
	if (Weapon == WEAPON_SELF)
		pVictim->GetPlayer()->m_RespawnTick = Server()->Tick() + Server()->TickSpeed() * 3.0f;
	return 0;
}

void CGameControllerWorkbenches::OnCharacterSpawn(class CCharacter *pChr, bool RequestAI)
{
	// default health
	pChr->IncreaseHealth(10);

	if (BattleIsEnabled())
	{
		if (pChr->GetPlayer()->m_BattleClass >= 0)
			BattleApplyLoadout(pChr, pChr->GetPlayer()->m_BattleClass);
		if (pChr->GetPlayer()->m_pAI)
			pChr->GetPlayer()->m_pAI->Reset();
		return;
	}

	// give default weapons
	pChr->GiveWeapon(WEAPON_HAMMER, -1);
	pChr->GiveWeapon(WEAPON_GUN, 10);

	if (pChr->GetPlayer()->m_pAI)
		pChr->GetPlayer()->m_pAI->Reset();
}

void CGameControllerWorkbenches::DoWarmup(int Seconds)
{
	if (Seconds < 0)
		m_Warmup = 0;
	else
		m_Warmup = Seconds * Server()->TickSpeed();
}

void CGameControllerWorkbenches::TogglePause()
{
	if (IsGameOver())
		return;

	if (GameServer()->m_World.m_Paused)
	{
		// unpause
		if (g_Config.m_SvUnpauseTimer > 0)
			m_UnpauseTimer = g_Config.m_SvUnpauseTimer * Server()->TickSpeed();
		else
		{
			GameServer()->m_World.m_Paused = false;
			m_UnpauseTimer = 0;
		}
	}
	else
	{
		// pause
		GameServer()->m_World.m_Paused = true;
		m_UnpauseTimer = 0;
	}
}

bool CGameControllerWorkbenches::IsFriendlyFire(int ClientID1, int ClientID2)
{
	if (ClientID1 == ClientID2)
		return false;

	if (ClientID1 < 0 || ClientID2 < 0)
		return false;

	if (IsTeamplay())
	{
		if (!GameServer()->m_apPlayers[ClientID1] || !GameServer()->m_apPlayers[ClientID2])
			return false;

		if (GameServer()->m_apPlayers[ClientID1]->GetTeam() == GameServer()->m_apPlayers[ClientID2]->GetTeam())
			return true;
	}

	return false;
}

bool CGameControllerWorkbenches::IsForceBalanced()
{
	if (m_ForceBalanced)
	{
		m_ForceBalanced = false;
		return true;
	}
	else
		return false;
}

bool CGameControllerWorkbenches::CanBeMovedOnBalance(int ClientID)
{
	return true;
}

void CGameControllerWorkbenches::Tick()
{
	// do warmup
	if (!GameServer()->m_World.m_Paused && m_Warmup)
	{
		m_Warmup--;
		if (!m_Warmup)
			StartRound();
	}

	if (m_GameOverTick != -1)
	{
		// game over.. wait for restart
		if (Server()->Tick() > m_GameOverTick + Server()->TickSpeed() * 10)
		{
			CycleMap();
			StartRound();
			m_RoundCount++;
		}
	}
	else if (GameServer()->m_World.m_Paused && m_UnpauseTimer)
	{
		--m_UnpauseTimer;
		if (!m_UnpauseTimer)
			GameServer()->m_World.m_Paused = false;
	}

	// game is Paused
	if (GameServer()->m_World.m_Paused)
		++m_RoundStartTick;

	// do team-balancing
	if (IsTeamplay() && m_UnbalancedTick != -1 && Server()->Tick() > m_UnbalancedTick + g_Config.m_SvTeambalanceTime * Server()->TickSpeed() * 60)
	{
		GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", "Balancing teams");

		int aT[2] = {0, 0};
		float aTScore[2] = {0, 0};
		float aPScore[MAX_CLIENTS] = {0.0f};
		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if (GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS)
			{
				aT[GameServer()->m_apPlayers[i]->GetTeam()]++;
				aPScore[i] = GameServer()->m_apPlayers[i]->m_Score * Server()->TickSpeed() * 60.0f /
							 (Server()->Tick() - GameServer()->m_apPlayers[i]->m_ScoreStartTick);
				aTScore[GameServer()->m_apPlayers[i]->GetTeam()] += aPScore[i];
			}
		}

		// are teams unbalanced?
		if (absolute(aT[0] - aT[1]) >= 2)
		{
			int M = (aT[0] > aT[1]) ? 0 : 1;
			int NumBalance = absolute(aT[0] - aT[1]) / 2;

			do
			{
				CPlayer *pP = 0;
				float PD = aTScore[M];
				for (int i = 0; i < MAX_CLIENTS; i++)
				{
					if (!GameServer()->m_apPlayers[i] || !CanBeMovedOnBalance(i))
						continue;
					// remember the player who would cause lowest score-difference
					if (GameServer()->m_apPlayers[i]->GetTeam() == M && (!pP || absolute((aTScore[M ^ 1] + aPScore[i]) - (aTScore[M] - aPScore[i])) < PD))
					{
						pP = GameServer()->m_apPlayers[i];
						PD = absolute((aTScore[M ^ 1] + aPScore[i]) - (aTScore[M] - aPScore[i]));
					}
				}

				// move the player to the other team
				int Temp = pP->m_LastActionTick;
				pP->SetTeam(M ^ 1);
				pP->m_LastActionTick = Temp;

				pP->Respawn();
				pP->m_ForceBalanced = true;
			} while (--NumBalance);

			m_ForceBalanced = true;
		}
		m_UnbalancedTick = -1;
	}
	DoWincheck();
}

bool CGameControllerWorkbenches::IsTeamplay() const
{
	return m_GameFlags & GAMEFLAG_TEAMS;
}

void CGameControllerWorkbenches::Snap(int SnappingClient)
{
	CNetObj_GameInfo *pGameInfoObj = Server()->SnapNewItem<CNetObj_GameInfo>(0);
	if (!pGameInfoObj)
		return;

	pGameInfoObj->m_GameFlags = m_GameFlags;
	pGameInfoObj->m_GameStateFlags = 0;
	if (m_GameOverTick != -1)
		pGameInfoObj->m_GameStateFlags |= GAMESTATEFLAG_GAMEOVER;
	if (m_SuddenDeath)
		pGameInfoObj->m_GameStateFlags |= GAMESTATEFLAG_SUDDENDEATH;
	if (GameServer()->m_World.m_Paused)
		pGameInfoObj->m_GameStateFlags |= GAMESTATEFLAG_PAUSED;
	pGameInfoObj->m_RoundStartTick = m_RoundStartTick;
	pGameInfoObj->m_WarmupTimer = GameServer()->m_World.m_Paused ? m_UnpauseTimer : m_Warmup;

	pGameInfoObj->m_ScoreLimit = g_Config.m_SvScorelimit;
	pGameInfoObj->m_TimeLimit = g_Config.m_SvTimelimit;

	pGameInfoObj->m_RoundNum = (str_length(g_Config.m_SvMaprotation) && g_Config.m_SvRoundsPerMap) ? g_Config.m_SvRoundsPerMap : 0;
	pGameInfoObj->m_RoundCurrent = m_RoundCount + 1;

	CNetObj_GameData *pGameDataObj = (CNetObj_GameData *)Server()->SnapNewItem(NETOBJTYPE_GAMEDATA, 0, sizeof(CNetObj_GameData));
	if(!pGameDataObj)
		return;

	pGameDataObj->m_TeamscoreRed = GetWorkbenchHealth(TEAM_RED);
	pGameDataObj->m_TeamscoreBlue = GetWorkbenchHealth(TEAM_BLUE);

	pGameDataObj->m_FlagCarrierRed = -1;
	pGameDataObj->m_FlagCarrierBlue = -1;
}

int CGameControllerWorkbenches::GetAutoTeam(int NotThisID)
{
	// this will force the auto balancer to work overtime aswell
	if (g_Config.m_DbgStress)
		return 0;

	int aNumplayers[2] = {0, 0};
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (GameServer()->m_apPlayers[i] && i != NotThisID)
		{
			if (GameServer()->m_apPlayers[i]->GetTeam() >= TEAM_RED && GameServer()->m_apPlayers[i]->GetTeam() <= TEAM_BLUE)
				aNumplayers[GameServer()->m_apPlayers[i]->GetTeam()]++;
		}
	}

	int Team = 0;
	if (IsTeamplay())
		Team = aNumplayers[TEAM_RED] > aNumplayers[TEAM_BLUE] ? TEAM_BLUE : TEAM_RED;

	if (CanJoinTeam(Team, NotThisID))
		return Team;
	return -1;
}

bool CGameControllerWorkbenches::CanJoinTeam(int Team, int NotThisID)
{
	if (Team == TEAM_SPECTATORS || (GameServer()->m_apPlayers[NotThisID] && GameServer()->m_apPlayers[NotThisID]->GetTeam() != TEAM_SPECTATORS))
		return true;

	int aNumplayers[2] = {0, 0};
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (GameServer()->m_apPlayers[i] && i != NotThisID)
		{
			if (GameServer()->m_apPlayers[i]->GetTeam() >= TEAM_RED && GameServer()->m_apPlayers[i]->GetTeam() <= TEAM_BLUE)
				aNumplayers[GameServer()->m_apPlayers[i]->GetTeam()]++;
		}
	}

	return (aNumplayers[0] + aNumplayers[1]) < Server()->MaxClients() - g_Config.m_SvSpectatorSlots;
}

bool CGameControllerWorkbenches::CheckTeamBalance()
{
	if (!IsTeamplay() || !g_Config.m_SvTeambalanceTime)
		return true;

	int aT[2] = {0, 0};
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		CPlayer *pP = GameServer()->m_apPlayers[i];
		if (pP && pP->GetTeam() != TEAM_SPECTATORS)
			aT[pP->GetTeam()]++;
	}

	char aBuf[256];
	if (absolute(aT[0] - aT[1]) >= 2)
	{
		str_format(aBuf, sizeof(aBuf), "Teams are NOT balanced (red=%d blue=%d)", aT[0], aT[1]);
		GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);
		if (GameServer()->m_pController->m_UnbalancedTick == -1)
			GameServer()->m_pController->m_UnbalancedTick = Server()->Tick();
		return false;
	}
	else
	{
		str_format(aBuf, sizeof(aBuf), "Teams are balanced (red=%d blue=%d)", aT[0], aT[1]);
		GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);
		GameServer()->m_pController->m_UnbalancedTick = -1;
		return true;
	}
}

bool CGameControllerWorkbenches::CanChangeTeam(CPlayer *pPlayer, int JoinTeam)
{
	int aT[2] = {0, 0};

	if (!IsTeamplay() || JoinTeam == TEAM_SPECTATORS || !g_Config.m_SvTeambalanceTime)
		return true;

	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		CPlayer *pP = GameServer()->m_apPlayers[i];
		if (pP && pP->GetTeam() != TEAM_SPECTATORS)
			aT[pP->GetTeam()]++;
	}

	// simulate what would happen if changed team
	aT[JoinTeam]++;
	if (pPlayer->GetTeam() != TEAM_SPECTATORS)
		aT[JoinTeam ^ 1]--;

	// there is a player-difference of at least 2
	if (absolute(aT[0] - aT[1]) >= 2)
	{
		// player wants to join team with less players
		if ((aT[0] < aT[1] && JoinTeam == TEAM_RED) || (aT[0] > aT[1] && JoinTeam == TEAM_BLUE))
			return true;
		else
			return false;
	}
	else
		return true;
}

void CGameControllerWorkbenches::DoWincheck()
{
	if (m_GameOverTick == -1 && !m_Warmup && !GameServer()->m_World.m_ResetRequested)
	{
		if (IsTeamplay())
		{
			if ((g_Config.m_SvTimelimit > 0 && (Server()->Tick() - m_RoundStartTick) >= g_Config.m_SvTimelimit * Server()->TickSpeed() * 60) ||
				GetWorkbenchHealth(TEAM_RED) <= 0 || GetWorkbenchHealth(TEAM_BLUE) <= 0)
			{
				if (GetWorkbenchHealth(TEAM_RED) != GetWorkbenchHealth(TEAM_BLUE))
					EndRound();
				else
					m_SuddenDeath = 1;
			}
		}
		else
		{
			// gather some stats
			int Topscore = 0;
			int TopscoreCount = 0;
			for (int i = 0; i < MAX_CLIENTS; i++)
			{
				if (GameServer()->m_apPlayers[i])
				{
					if (GameServer()->m_apPlayers[i]->m_Score > Topscore)
					{
						Topscore = GameServer()->m_apPlayers[i]->m_Score;
						TopscoreCount = 1;
					}
					else if (GameServer()->m_apPlayers[i]->m_Score == Topscore)
						TopscoreCount++;
				}
			}

			// check score win condition
			if ((g_Config.m_SvScorelimit > 0 && Topscore >= g_Config.m_SvScorelimit) ||
				(g_Config.m_SvTimelimit > 0 && (Server()->Tick() - m_RoundStartTick) >= g_Config.m_SvTimelimit * Server()->TickSpeed() * 60))
			{
				if (TopscoreCount == 1)
					EndRound();
				else
					m_SuddenDeath = 1;
			}
		}
	}
}

int CGameControllerWorkbenches::ClampTeam(int Team)
{
	if (Team < 0)
		return TEAM_SPECTATORS;
	if (IsTeamplay())
		return Team & 1;
	return 0;
}

void CGameControllerWorkbenches::OnPlayerConnect(CPlayer *pPlayer)
{
	const int ClientID = pPlayer->GetCID();
	if (Server()->ClientIngame(ClientID) && pPlayer->GetPlayerWorldID() == GameServer()->GetWorldID())
	{
		char aBuf[128];
		str_format(aBuf, sizeof(aBuf), "team_join player='%d:%s' team=%d", ClientID, Server()->ClientName(ClientID), pPlayer->GetTeam());
		GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);
	}
}

void CGameControllerWorkbenches::OnPlayerDisconnect(CPlayer *pPlayer)
{
	const int ClientID = pPlayer->GetCID();
	if (Server()->ClientIngame(ClientID) && pPlayer->GetPlayerWorldID() == GameServer()->GetWorldID())
	{
		char aBuf[128];
		str_format(aBuf, sizeof(aBuf), "leave player='%d:%s'", ClientID, Server()->ClientName(ClientID));
		GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);
	}

	pPlayer->OnDisconnect();
}

void CGameControllerWorkbenches::OnPlayerInfoChange(CPlayer *pPlayer, int WorldID) {}

void CGameControllerWorkbenches::OnReset() {}

void CGameControllerWorkbenches::LoadMapConfig()
{
	char aFilename[512];
	str_format(aFilename, sizeof(aFilename), "maps/%s.cfg", g_Config.m_SvMap);

	// read file data into buffer
	IOHANDLE File = GameServer()->Storage()->OpenFile(aFilename, IOFLAG_READ, IStorage::TYPE_ALL);
	if (File)
	{
		CLineReader LineReader;
		LineReader.Init(File);
		const char *pLine;
		while ((pLine = LineReader.Get()))
		{
			if (!str_comp_num(pLine, "flag", 4))
			{
				int Level = 0, MaxProgress = 200;
				vec2 Pos0, Pos1;
				if (sscanf(pLine, "flag t%d lv%d lx%f ly%f ux%f uy%f", &MaxProgress, &Level, &Pos0.x, &Pos0.y, &Pos1.x, &Pos1.y))
				{
					new CAreaFlag(&GameServer()->m_World, vec2(Pos0.x * 32 + 32, Pos0.y * 32 + 32), vec2(Pos1.x * 32 + 32, Pos1.y * 32 + 32), MaxProgress, Level, m_NumFlag);
					m_NumFlag++;
				}
			}
			if (!str_comp_num(pLine, "workbench", 9))
			{
				int Team;
				vec2 V1, V2;
				if (sscanf(pLine, "workbench t%d v1x%f v1y%f v2x%f v2y%f", &Team, &V1.x, &V1.y, &V2.x, &V2.y))
					m_apWorkbenches[Team] = new CWorkbench(&GameServer()->m_World, Team, V1 * 32.f, V2 * 32.f);
			}
			if (!str_comp_num(pLine, "cmd", 3))
			{
				char aBuf[256];
				sscanf(pLine, "cmd \"%s\"", aBuf);
				GameServer()->Console()->ExecuteLine(aBuf, -1);
			}
		}
		io_close(File);
	}

	for (int Team = 0; Team < 2; Team++)
	{
		if (!m_apWorkbenches[Team])
		{
			char aBuf[128];
			str_format(aBuf, sizeof(aBuf), "missing workbench for team %d in maps/%s.cfg", Team, g_Config.m_SvMap);
			GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);
		}
	}
}

int CGameControllerWorkbenches::GetWorkbenchHealth(int Team)
{
	if (m_apWorkbenches[Team])
		return m_apWorkbenches[Team]->GetHealth();
	return 0;
}

bool CGameControllerWorkbenches::MakeBuilding(int Building, int Team, int ClientID)
{
	if (Building < 0 || Building >= NUM_BUILDING || Building == BUILDING_WORKBENCH)
		return false;

	if (Team < TEAM_RED || Team > TEAM_BLUE)
		return false;

	for (int i = 0; i < NUM_RESOURCE; i++)
	{
		if (m_aTeamResources[Team][i] < GameServer()->m_pBuildingsInfo->m_aBuildingsInfo[Building].m_Formula[i])
		{
			if (ClientID >= 0)
				GameServer()->Chat(ClientID, "Your team doesn't have enough resources!");
			return false;
		}
	}

	for (int i = 0; i < NUM_RESOURCE; i++)
		m_aTeamResources[Team][i] -= GameServer()->m_pBuildingsInfo->m_aBuildingsInfo[Building].m_Formula[i];

	m_aTeamBuildings[Team][Building]++;

	if (ClientID >= 0 && GameServer()->m_apPlayers[ClientID])
	{
		GameServer()->ChatTeam(Team, "'{}' made a {} for the team!", Server()->ClientName(ClientID),
			GameServer()->m_pBuildingsInfo->m_aBuildingsInfo[Building].m_aName);
	}

	return true;
}

void CGameControllerWorkbenches::InitDoors()
{
	if (!GameServer()->Collision()->Switches())
	{
		dbg_msg("Doors", "No Switch layer found");
		return;
	}

	const int Height = GameServer()->Collision()->Layers()->GameLayer()->m_Height;
	const int Width = GameServer()->Collision()->Layers()->GameLayer()->m_Width;

	m_lDoors.clear();

	int TileCount = 0;
	for (int i = 0; i < Width; i++)
	{
		for (int j = 0; j < Height; j++)
		{
			if (GameServer()->Collision()->IsDoor(i * 32 + 16, j * 32 + 16))
				TileCount++;
		}
	}

	array<CDoor::CDoorNode> lPos;
	lPos.set_size(TileCount);

	int Index = 0;
	for (int i = 0; i < Width; i++)
	{
		for (int j = 0; j < Height; j++)
		{
			const int x = i * 32 + 16;
			const int y = j * 32 + 16;
			if (GameServer()->Collision()->IsDoor(x, y))
			{
				lPos[Index].m_Pos = vec2(x, y);
				lPos[Index].m_Type = GameServer()->Collision()->IsDoor(x, y);
				lPos[Index].m_Team = GameServer()->Collision()->GetSwitchTeam(x, y);
				Index++;
			}
		}
	}

	array<CDoor::CDoorNode> lDoor;
	for (int i = 0; i < lPos.size(); i++)
	{
		if (lPos[i].m_Type == TILE_DOOR_START)
		{
			lDoor.add(lPos[i]);
			lPos.remove_index(i);

			const int SwitchNum = GameServer()->Collision()->GetSwitchNum(lDoor[0].m_Pos);
			if (!SwitchNum)
			{
				dbg_msg("Doors", "Found invalid Switch, please fix the map");
				return;
			}

			bool FoundSwitch = false;
			array<CDoor::CDoorNode> lSwitch;
			for (i = 0; i < lPos.size(); i++)
			{
				if (lPos[i].m_Type == TILE_DOOR_SWITCH && GameServer()->Collision()->GetSwitchNum(lPos[i].m_Pos) == SwitchNum)
				{
					lSwitch.add(lPos[i]);
					FoundSwitch = true;
				}
			}
			if (!FoundSwitch)
			{
				dbg_msg("Doors", "No doorswitch found for Switch %i", SwitchNum);
				return;
			}

			bool FoundEnd = false;
			for (i = 0; i < lPos.size(); i++)
			{
				if (lPos[i].m_Type == TILE_DOOR_END && GameServer()->Collision()->GetSwitchNum(lPos[i].m_Pos) == SwitchNum &&
					!GameServer()->Collision()->IntersectLine(lDoor[0].m_Pos, lPos[i].m_Pos, 0x0, 0x0) &&
					!GameServer()->Collision()->DoorBlock(lDoor[0].m_Pos, lPos[i].m_Pos))
				{
					lDoor.add(lPos[i]);
					FoundEnd = true;
				}
			}
			if (!FoundEnd)
			{
				dbg_msg("Doors", "No endpoint found for Switch %i", SwitchNum);
				return;
			}

			int Team = lDoor[0].m_Team;
			for (i = 1; i < lDoor.size(); i++)
			{
				if (Team != lDoor[i].m_Team)
				{
					dbg_msg("Doors", "Switch %i has doors with different teams", SwitchNum);
					return;
				}
			}
			for (i = 1; i < lSwitch.size(); i++)
			{
				if (Team != lSwitch[i].m_Team)
				{
					dbg_msg("Doors", "Switch %i has doorswitches with different teams", SwitchNum);
					return;
				}
			}

			CDoor Door(GameServer(), Team, SwitchNum, lDoor, lSwitch);
			m_lDoors.add(Door);

			lSwitch.clear();
			lDoor.clear();
			i = -1;
		}
	}

	m_lDoors.optimize();

	for (int d = 0; d < m_lDoors.size(); d++)
		m_lDoors[d].Init();
}

void CGameControllerWorkbenches::SwitchDoor(CDoor *pDoor, CPlayer *pPlayer, vec2 Pos, bool Silent)
{
	if ((pDoor->m_SwitchTick + (Server()->TickSpeed() * (float)g_Config.m_SvDoorSwitchTime / 10)) < Server()->Tick())
	{
		if (pPlayer->GetTeam() == pDoor->m_Team || pDoor->m_Team == -1)
		{
			if (pDoor->Switch(Pos, Silent))
				pDoor->m_SwitchTick = Server()->Tick();
		}
		else if (!Silent)
		{
			GameServer()->Chat(pPlayer->GetCID(), "This is your enemies door, you can't operate it!");
			GameServer()->CreateSound(Pos, SOUND_HOOK_NOATTACH);
			pDoor->m_SwitchTick = Server()->Tick();
		}
	}
}

void CGameControllerWorkbenches::SwitchDoorAt(vec2 Pos, CPlayer *pPlayer, bool Silent)
{
	if (!GameServer()->Collision()->Switches() || !pPlayer)
		return;

	const int SwitchNum = GameServer()->Collision()->GetSwitchNum(Pos);
	if (!SwitchNum)
		return;

	for (int i = 0; i < m_lDoors.size(); i++)
	{
		if (m_lDoors[i].m_SwitchNum == SwitchNum)
		{
			SwitchDoor(&m_lDoors[i], pPlayer, Pos, Silent);
			return;
		}
	}
}

bool CGameControllerWorkbenches::BuildBuilding(vec2 Pos, int Type, int Team, int Owner)
{
	if (!GameServer()->GetPlayer(Owner))
		return false;

	if (Team == TEAM_SPECTATORS)
		return false;

	if (m_aTeamBuildings[GameServer()->GetPlayer(Owner)->GetTeam()][Type] <= 0)
	{
		GameServer()->Chat(Owner, "There is no {} in the team!", GameServer()->m_pBuildingsInfo->m_aBuildingsInfo[Type].m_aName);
		return false;
	}

	int Space = 0;
	vec2 TempPos = Pos;
	while (!GameServer()->Collision()->CheckPoint(TempPos) && Space < 32)
	{
		TempPos.y -= 32;
		Space++;
	}

	if (GameServer()->m_pBuildingsInfo->m_aBuildingsInfo[Type].m_Height > Space * 32)
	{
		GameServer()->Chat(Owner, "There's not enough space to build here");
		return false;
	}

	for (CBuilding *pBuilding = (CBuilding *)GameServer()->m_World.FindFirst(CGameWorld::ENTTYPE_BUILDINGS); pBuilding; pBuilding = (CBuilding *)pBuilding->TypeNext())
	{
		if (distance(pBuilding->GetPos(), Pos) < 56)
		{
			GameServer()->Chat(Owner, "This spot is blocked");
			return false;
		}

		if (Team != pBuilding->GetTeam() && distance(pBuilding->GetPos(), Pos) < 400.0f)
		{
			GameServer()->Chat(Owner, "This spot is too close to the enemy!");
			return false;
		}
	}

	if (VehicleSpotBlocked(&GameServer()->m_World, Pos))
	{
		GameServer()->Chat(Owner, "This spot is blocked");
		return false;
	}

	switch (Type)
	{
	case BUILDING_AIRCRAFT:
		new CAircraft(&GameServer()->m_World, Pos, Team);
		break;
	case BUILDING_HELICOPTER:
		new CHelicopter(&GameServer()->m_World, Pos, Team);
		break;
	case BUILDING_JET:
		new CJet(&GameServer()->m_World, Pos, Team);
		break;
	case BUILDING_TANK:
		new CTank(&GameServer()->m_World, Pos, Team);
		break;
	case BUILDING_CAR:
		new CCar(&GameServer()->m_World, Pos, Team);
		break;
	default:
		new CBuilding(&GameServer()->m_World, Team, Pos, Type);
		break;
	}
	m_aTeamBuildings[GameServer()->GetPlayer(Owner)->GetTeam()][Type]--;
	if (GameServer()->GetPlayer(Owner) && GameServer()->GetPlayer(Owner)->m_BuildMenuOpen)
		GameServer()->RefreshBuildMenu(Owner);

	return true;
}