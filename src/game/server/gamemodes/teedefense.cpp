/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/generated/protocol.h>
#include <game/mapitems.h>
#include <game/server/bot.h>
#include <game/server/gamecontext.h>
#include <game/server/entities/CKs.h>
#include <game/server/entities/tower-main.h>
#include <game/server/GameCore/Account/account.h>
#include <engine/shared/protocol.h>
#include <engine/shared/config.h>
#include "teedefense.h"

CGameControllerTeeDefense::CGameControllerTeeDefense(class CGameContext *pGameServer)
	: IGameController(pGameServer)
{
	m_IsTeamplay = true;
	m_Wave = 0;
	mem_zero(m_Zombie, sizeof(m_Zombie));
}

bool CGameControllerTeeDefense::OnEntity(int Index, vec2 Pos)
{
	IGameController::OnEntity(Index, Pos);

	int Type = -1;
	switch (Index)
	{
	case ENTITY_LOG:
		Type = ITEM_LOG;
		break;
	case ENTITY_COAL:
		Type = ITEM_COAL;
		break;
	case ENTITY_COPPER:
		Type = ITEM_COPPER;
		break;
	case ENTITY_IRON:
		Type = ITEM_IRON;
		break;
	case ENTITY_GOLD:
		Type = ITEM_GOLD;
		break;
	case ENTITY_DIAMOND:
		Type = ITEM_DIAMOND;
		break;
	case ENTITY_ENERGY:
		Type = ITEM_ENEGRY;
		break;
	case ENTITY_MAIN_TOWER:
		new CTowerMain(&GameServer()->m_World, Pos);
		break;

	default:
		break;
	}

	if (Type != -1)
	{
		new CKs(&GameServer()->m_World, Type, Pos);
		return true;
	}
	return false;
}

void CGameControllerTeeDefense::InitBots()
{
	// Init bots
	for (int i = 0; i < MAX_BOTS; i++)
		GameServer()->AddBot();

	for (auto &pPlayer : GameServer()->m_apPlayers)
	{
		if (!pPlayer || !pPlayer->m_pBot)
			continue;

		GameServer()->AsleepBot(pPlayer->GetCID());
		for (int i = 0; i < ETarget::NUM_TARGETS; i++)
			pPlayer->m_pBot->m_aTargetAllow[i] = false;

		pPlayer->m_pBot->m_aTargetAllow[ETarget::TARGET_PLAYER] = true;
	}
}

void CGameControllerTeeDefense::StartRound()
{
	m_RoundStartTick = Server()->Tick();
	m_GameOverTick = -1;
	// Zomb2
	for (int i = MAX_PLAYERS; i < MAX_CLIENTS; i++) // bugfix
		OnZombieKill(i);
	m_Wave++;
	StartWave(m_Wave);
}

void CGameControllerTeeDefense::EndRound()
{
}

void CGameControllerTeeDefense::OnCharacterSpawn(CCharacter *pChr)
{
	IGameController::OnCharacterSpawn(pChr);
	if (pChr->GetPlayer()->IsBot())
		pChr->IncreaseHealth(m_Wave);
	else
	{
		// default health
		pChr->IncreaseHealth(g_Config.m_SvPlayerMaxHealth);
		pChr->GiveWeapon(WEAPON_GUN, 10);
	}
}

int CGameControllerTeeDefense::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon)
{
	if (!pKiller)
		return 0;

	if (pVictim->GetPlayer()->GetTeam() == TEAM_BOT)
	{
		if (pKiller && pKiller->GetTeam() == TEAM_HUMAN)
		{
			int Reward = ITEM_LOG;
				int Rando = rand() % 100 + 1;
			if (Rando <= 50)
				Reward = ITEM_LOG;
			else if (Rando >= 51 && Rando <= 75)
				Reward = ITEM_COPPER;
			else if (Rando <= 99)
				Reward = ITEM_GOLD;

			pKiller->m_AccData.m_aItems[Reward].m_Num++;
			GameServer()->Broadcast(pKiller->GetCID(), "You picked up a {}", GameServer()->ItemHelper()->GetItemName(Reward));

			pKiller->m_AccData.m_aItems[ITEM_ZOMBIEHEART].m_Num++;
			pKiller->m_Score++;
			GameServer()->TW()->Account()->SaveAccountData(pKiller->GetCID(), CGameContext::TABLE_ITEM, pKiller->m_AccData);
			GameServer()->ClearVotes(pKiller->GetCID());
		}
		DoZombMessage(m_ZombLeft--);
		GameServer()->AsleepBot(pVictim->GetPlayer()->GetCID());
	}

	// do scoreing
	if (Weapon == WEAPON_GAME)
		return 0;
	if (pKiller && pKiller == pVictim->GetPlayer())
		return 0; // suicide
	else
	{
		if (IsTeamplay() && pVictim->GetPlayer()->GetTeam() == pKiller->GetTeam())
		{
			if (g_Config.m_SvTeamdamage)
				pKiller->m_Score--; // teamkill
		}
		else
			pKiller->m_Score++; // normal kill
	}
	return 0;
}

void CGameControllerTeeDefense::Tick()
{
	int Players = 0;
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		if (!GameServer()->GetPlayer(i))
			continue;

		if (GameServer()->GetPlayer(i)->GetTeam() != TEAM_HUMAN)
			continue;

		if (!GameServer()->GetPlayerChar(i))
			continue;

		Players++;
	}

	if (Players >= 1 && !m_Wave)
		StartRound();

	// do warmup
	if (!GameServer()->m_World.m_Paused && m_Warmup)
	{
		m_Warmup--;
		if (!m_Warmup)
		{
			StartRound();
			GameServer()->m_World.m_Paused = false;
		}
	}

	CheckZombie();
	if (m_GameOverTick != -1)
	{
		// game over.. wait for restart
		if (Server()->Tick() > m_GameOverTick + Server()->TickSpeed() * 10)
		{
			CycleMap();
			PostReset();

			// Zomb2: Do this ONLY when the Game ended, that must be BEFORE the round restarts
			for (int i = MAX_PLAYERS; i < MAX_CLIENTS; i++)
				OnZombieKill(i);
			DoWarmup(g_Config.m_SvWarmup);
			m_GameOverTick = -1;
			m_RoundStartTick = Server()->Tick();
			GameServer()->m_World.m_Paused = false;

			// StartRound(); Not needed anymore, do warmup instead (with warmup config)
			m_RoundCount++;
		}
	}

	// game is Paused
	if (GameServer()->m_World.m_Paused)
		++m_RoundStartTick;
}

void CGameControllerTeeDefense::DoWincheck()
{
}

bool CGameControllerTeeDefense::CanSpawn(int Team, vec2 *pPos)
{
	CSpawnEval Eval;

	// spectators can't spawn
	if (Team == TEAM_SPECTATORS)
		return false;

	if (Team == TEAM_HUMAN)
		EvaluateSpawnType(&Eval, 1);
	else
	{
		EvaluateSpawnType(&Eval, 0);
		EvaluateSpawnType(&Eval, 2);
	}
	*pPos = Eval.m_Pos;
	return Eval.m_Got;
}

void CGameControllerTeeDefense::StartWave(int Wave)
{
	if (!Wave) // Well, just in case ^^ shouldn't be needed
		return;
	// Zaby, Zaby has no alround wave
	else if (Wave == 1)
		m_Zombie[0] = 10;
	else if (Wave == 2)
		m_Zombie[0] = 25;
	else
		SetWaveAlg(Wave % 3, Wave / 3);

	// Message Shit
	m_ZombLeft = 0;
	for (int i = 0; i < (int)(sizeof(m_Zombie) / sizeof(m_Zombie[0])); i++)
		m_ZombLeft += m_Zombie[i];

	DoZombMessage(0);
}

void CGameControllerTeeDefense::CheckZombie()
{
	if (m_Warmup || !m_Wave || EndWave())
		return;
	for (int i = MAX_PLAYERS; i < MAX_CLIENTS; i++)
	{
		if (GameServer()->m_apPlayers[i] && !GameServer()->m_apPlayers[i]->m_CanSnap && !GameServer()->m_apPlayers[i]->m_WantSpawn) // Check if the CID is ok
		{
			int Random = RandZomb();
			if (Random == -1)
				break;

			if (GameServer()->AwakenBot(i))
				GameServer()->m_apPlayers[i]->InitZombie(Random + 1);

			m_Zombie[Random]--;
		}
	}
}

int CGameControllerTeeDefense::RandZomb()
{
	int size = (int)(sizeof(m_Zombie) / sizeof(m_Zombie[0]));
	int Rand = rand() % size;
	int WTF = g_Config.m_SvMaxZombieSpawn; // dont make it to high, can cause bad cpu
	while (!m_Zombie[Rand])
	{
		Rand = rand() % size;
		WTF--;
		if (!WTF) // Anti 100% CPU :D (Very crappy, but it's a fix :P)
			return -1;
	}
	return Rand;
}

bool CGameControllerTeeDefense::EndWave()
{
	int PlayerCount = 0;
	for (int k = 0; k < MAX_PLAYERS; k++)
	{
		if (GameServer()->m_apPlayers[k]) // Make sure a player is there
		{
			PlayerCount++;
			break;
		}
	}

	if (!PlayerCount) // No Players - reset round
	{
		for (int i = MAX_PLAYERS; i < MAX_CLIENTS; i++)
			OnZombieKill(i);
		// HandleTop();
		m_Wave = 0;
		return true;
	}
	for (int j = 0; j < (int)(sizeof(m_Zombie) / sizeof(m_Zombie[0])); j++)
	{
		if (m_Zombie[j])
			return false;
	}
	for (int i = MAX_PLAYERS; i < MAX_CLIENTS; i++)
	{
		if (GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->m_CanSnap)
			return false;
	}
	DoWarmup(g_Config.m_SvZombWarmup + 5 * m_Wave);
	return true;
}

void CGameControllerTeeDefense::DoZombMessage(int Which)
{
	if (!Which)
	{
		GameServer()->Broadcast(-1, "Wave {} started with {} Zombies!", m_Wave, m_ZombLeft);
		return;
	}
	Which -= 1;
	if (Which > 1 && (Which <= 5 || !(Which % 10)))
		GameServer()->Chat(-1, "Wave {}: {} zombies are left", m_Wave, Which);
	else if (Which == 1)
		GameServer()->Chat(-1, "Wave {}: 1 zombie is left", m_Wave);
}

void CGameControllerTeeDefense::SetWaveAlg(int modulus, int wavedrittel)
{
	if (wavedrittel > 11) // endless Waves, but exponentiell Zombie code
	{
		for (int i = 0; i < (int)(sizeof(m_Zombie) / sizeof(m_Zombie[0])); i++)
			m_Zombie[i] = m_Wave + 10; // 3 mal wavedrittel + modulus 2
		return;
	}

	if (!modulus) // 10ner Wave
	{
		m_Zombie[GetZombieReihenfolge(wavedrittel)] = 10;
	}
	else if (modulus == 1) // 40er wave
	{
		m_Zombie[GetZombieReihenfolge(wavedrittel)] = 20;
	}
	else if (modulus == 2)
	{
		for (int i = 0; i <= wavedrittel; i++)
		{
			m_Zombie[GetZombieReihenfolge(i)] = m_Wave;
		}
	}
}

int CGameControllerTeeDefense::GetZombieReihenfolge(int wavedrittel) // Was hei�t Riehenfolge auf englisch ...
{
	if (!wavedrittel)
		return 0;
	if (wavedrittel < NUM_ZOMB)
		return wavedrittel + 1;
	else
		return 1;
}

void CGameControllerTeeDefense::OnZombieKill(int ClientID)
{
	CPlayer *pPlayer = GameServer()->m_apPlayers[ClientID];
	if (!pPlayer)
		return;

	// update spectator modes
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (pPlayer && pPlayer->m_SpectatorID == ClientID)
			pPlayer->m_SpectatorID = SPEC_FREEVIEW;
	}
}

void CGameControllerTeeDefense::OnPlayerConnect(class CPlayer *pPlayer)
{
	const int ClientID = pPlayer->GetCID();
	if (Server()->ClientIngame(ClientID) && pPlayer->GetPlayerWorldID() == GameServer()->GetWorldID())
	{
		char aBuf[128];
		str_format(aBuf, sizeof(aBuf), "TeeDefense Player='%d:%s'", ClientID, Server()->ClientName(ClientID));
		GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);
		GameServer()->Chat(-1, "{} entered and joined Tee Defense", Server()->ClientName(ClientID));
		GameServer()->Chat(ClientID, "Server official QQ group: 1007351135");
		GameServer()->Chat(ClientID, "Server Hoster/Developer E-Mail: ilovejel@163.com");
	}
}