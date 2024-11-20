/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/generated/protocol.h>
#include <game/server/bot.h>
#include <game/server/gamecontext.h>
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
}

int CGameControllerTeeDefense::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon)
{
	if (!pKiller)
		return 0;

	if (pVictim->GetPlayer()->GetTeam() == TEAM_BOT)
	{
		if (pKiller && pKiller->GetTeam() == TEAM_HUMAN)
			m_aTeamscore[TEAM_BOT]++;
		DoZombMessage(m_ZombLeft--);
		GameServer()->AsleepBot(pVictim->GetPlayer()->GetCID());
	}
	else if (pKiller->GetTeam() == TEAM_HUMAN && pVictim->GetPlayer() && pVictim->GetPlayer()->GetTeam() == TEAM_BOT)
		DoLifeMessage(m_aTeamscore[TEAM_HUMAN]--);

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
		if (GameServer()->m_apPlayers[Players])
		{
			if (GameServer()->m_apPlayers[Players]->GetTeam() == TEAM_HUMAN)
				Players++;
		}
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

	if (IsTeamplay())
	{
		Eval.m_FriendlyTeam = Team;

		// first try own team spawn, then normal spawn and then enemy
		EvaluateSpawnType(&Eval, 1 + (Team & 1));
		if (!Eval.m_Got)
		{
			EvaluateSpawnType(&Eval, 0);
			/* BITCH PLEASE DONT USE ENEMY SPAWN!
			if(!Eval.m_Got)
				EvaluateSpawnType(&Eval, 1+((Team+1)&1));*/
		}
	}
	else
	{
		EvaluateSpawnType(&Eval, 0);
		EvaluateSpawnType(&Eval, 1);
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
		m_Zombie[0] = 40;
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
				GameServer()->m_apPlayers[i]->m_Zomb = Random + 1;

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

void CGameControllerTeeDefense::DoLifeMessage(int Life)
{
	Life -= 1;

	if (Life > 1 && (Life <= 5 || !(Life % 10)))
	{
		if (Life <= 10)
			GameServer()->Broadcast(-1, "Only {} lifes left!", Life);
		else
			GameServer()->Broadcast(-1, "{} lifes left!", Life);
	}
	else if (Life == 1)
		GameServer()->Broadcast(-1, "!!!Only 1 life left!!!");
}

void CGameControllerTeeDefense::SetWaveAlg(int modulus, int wavedrittel)
{
	if (wavedrittel > 11) // endless Waves, but exponentiell Zombie code
	{
		for (int i = 0; i < (int)(sizeof(m_Zombie) / sizeof(m_Zombie[0])); i++)
			m_Zombie[i] = m_Wave - 35; // 3 mal wavedrittel + modulus 2
		return;
	}

	if (!modulus) // 10ner Wave
	{
		m_Zombie[GetZombieReihenfolge(wavedrittel)] = 10;
	}
	else if (modulus == 1) // 40er wave
	{
		m_Zombie[GetZombieReihenfolge(wavedrittel)] = 40;
	}
	else if (modulus == 2)
	{
		for (int i = 0; i <= wavedrittel; i++)
		{
			m_Zombie[GetZombieReihenfolge(i)] = 10;
		}
	}
}

int CGameControllerTeeDefense::GetZombieReihenfolge(int wavedrittel) // Was hei�t Riehenfolge auf englisch ...
{
	// sehr unsch�n, man m�sste die Zombies neu sortieren was ein haufen arbeit ist
	if (!wavedrittel)
		return 0;
	else if (wavedrittel == 1)
		return 2;
	else if (wavedrittel == 2)
		return 3;
	else if (wavedrittel == 3)
		return 4;
	else if (wavedrittel == 4)
		return 6;
	else if (wavedrittel == 5)
		return 5;
	else if (wavedrittel == 6)
		return 7;
	else if (wavedrittel == 7)
		return 8;
	else if (wavedrittel == 8)
		return 9;
	else if (wavedrittel == 9)
		return 10;
	else if (wavedrittel == 10)
		return 11;
	else if (wavedrittel == 11)
		return 1;
	else // shouldnt be needed
		return 0;
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