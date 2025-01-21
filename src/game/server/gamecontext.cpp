/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <new>
#include <base/math.h>
#include <algorithm>
#include <sstream>
#include <vector>
#include <string>
#include <engine/shared/config.h>
#include <engine/map.h>
#include <engine/console.h>
#include <game/version.h>
#include <game/collision.h>
#include <game/gamecore.h>
#include <game/mapitems.h>
#include <string.h>
#include <thread>

#include <teeother/tl/nlohmann_json.h>
#include <teeother/components/localization.h>

#include "GameCore/Account/account.h"
#include "Item/item.h"
#include "chatai.h"
#include "gamemodes/main.h"
#include "gamemodes/teedefense.h"
#include "bot.h"

#include "entities/growingexplosion.h"

#include "gamecontext.h"

enum
{
	RESET,
	NO_RESET
};

void CGameContext::Construct(int Resetting)
{
	m_pItemHelper = new CItemHelper(this);
	m_Resetting = 0;
	m_pServer = 0;

	for (int i = 0; i < MAX_CLIENTS; i++)
		m_apPlayers[i] = 0;

	m_pController = 0;
	m_VoteCloseTime = 0;
	m_pVoteOptionFirst = 0;
	m_pVoteOptionLast = 0;
	m_NumVoteOptions = 0;
	m_LockTeams = 0;
	m_ConsoleOutputHandle_ChatPrint = -1;
	m_ConsoleOutput_Target = -1;

	if (Resetting == NO_RESET)
		m_pVoteOptionHeap = new CHeap();

	m_pBotEngine = new CBotEngine(this);
	m_pDB = new CDB();
}

CGameContext::CGameContext(int Resetting)
{
	Construct(Resetting);
}

CGameContext::CGameContext()
{
	Construct(NO_RESET);
}

CGameContext::~CGameContext()
{
	for(int i = 0; i < m_LaserDots.size(); i++)
		Server()->SnapFreeID(m_LaserDots[i].m_SnapID);
	for(int i = 0; i < m_HammerDots.size(); i++)
		Server()->SnapFreeID(m_HammerDots[i].m_SnapID);

	for (int i = 0; i < MAX_CLIENTS; i++)
		delete m_apPlayers[i];
	if (!m_Resetting)
		delete m_pVoteOptionHeap;

	delete m_pBotEngine;
	delete m_pController;
	delete m_pTWorldController;
	delete m_pItemHelper;
	delete m_pDB;
	delete m_pChatAI;
}

void CGameContext::OnSetAuthed(int ClientID, int Level)
{
	if (m_apPlayers[ClientID])
		m_apPlayers[ClientID]->m_Authed = Level;
}

class CCharacter *CGameContext::GetPlayerChar(int ClientID)
{
	if (ClientID < 0 || ClientID >= MAX_CLIENTS || !m_apPlayers[ClientID])
		return 0;
	return m_apPlayers[ClientID]->GetCharacter();
}

void CGameContext::CreateDamageInd(vec2 Pos, float Angle, int Amount, CClientMask Mask)
{
	float a = 3 * 3.14159f / 2 + Angle;
	// float a = get_angle(dir);
	float s = a - pi / 3;
	float e = a + pi / 3;
	for (int i = 0; i < Amount; i++)
	{
		float f = mix(s, e, float(i + 1) / float(Amount + 2));
		CNetEvent_DamageInd *pEvent = m_Events.Create<CNetEvent_DamageInd>(Mask);
		if (pEvent)
		{
			pEvent->m_X = (int)Pos.x;
			pEvent->m_Y = (int)Pos.y;
			pEvent->m_Angle = (int)(f * 256.0f);
		}
	}
}

void CGameContext::CreateHammerHit(vec2 Pos, CClientMask Mask)
{
	// create the event
	CNetEvent_HammerHit *pEvent = m_Events.Create<CNetEvent_HammerHit>(Mask);
	if (pEvent)
	{
		pEvent->m_X = (int)Pos.x;
		pEvent->m_Y = (int)Pos.y;
	}
}

void CGameContext::CreateExplosion(vec2 Pos, int Owner, int Weapon, bool NoDamage, CClientMask Mask)
{
	// create the event
	CNetEvent_Explosion *pEvent = m_Events.Create<CNetEvent_Explosion>(Mask);
	if (pEvent)
	{
		pEvent->m_X = (int)Pos.x;
		pEvent->m_Y = (int)Pos.y;
	}

	CreateExtraEffect(Pos, 0, Mask);

	if (!NoDamage)
	{
		// deal damage
		CCharacter *apEnts[MAX_CLIENTS];
		float Radius = 135.0f;
		float InnerRadius = 48.0f;
		int Num = m_World.FindEntities(Pos, Radius, (CEntity **)apEnts, MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);
		for (int i = 0; i < Num; i++)
		{
			vec2 Diff = apEnts[i]->GetPos() - Pos;
			vec2 ForceDir(0, 1);
			float l = length(Diff);
			if (l)
				ForceDir = normalize(Diff);
			l = 1 - clamp((l - InnerRadius) / (Radius - InnerRadius), 0.0f, 1.0f);
			float Dmg = 6 * l;
			if ((int)Dmg)
				apEnts[i]->TakeDamage(ForceDir * Dmg * 2, (int)Dmg, Owner, Weapon);
		}

		if (GetPlayer(Owner))
		{
			int Electron = ItemHelper()->GetCard(GetPlayer(Owner)->GetExtraHolding(ITYPE_SWORD), ITEM_CARD_ELECTRON);
			if (Electron)
				new CGrowingExplosion(&m_World, Pos, vec2(0, 0), Owner, 5.f * Electron, GROWINGEXPLOSIONEFFECT_ELECTRIC);
		}
	}
}

void CGameContext::CreatePlayerSpawn(vec2 Pos, CClientMask Mask)
{
	// create the event
	CNetEvent_Spawn *ev = m_Events.Create<CNetEvent_Spawn>(Mask);
	;
	if (ev)
	{
		ev->m_X = (int)Pos.x;
		ev->m_Y = (int)Pos.y;
	}
}

void CGameContext::CreateDeath(vec2 Pos, int ClientID, CClientMask Mask)
{
	// create the event
	CNetEvent_Death *pEvent = m_Events.Create<CNetEvent_Death>(Mask);
	if (pEvent)
	{
		pEvent->m_X = (int)Pos.x;
		pEvent->m_Y = (int)Pos.y;
		pEvent->m_ClientID = ClientID;
	}
}

void CGameContext::CreateSound(vec2 Pos, int Sound, CClientMask Mask)
{
	if (Sound < 0)
		return;

	// create a sound
	CNetEvent_SoundWorld *pEvent = m_Events.Create<CNetEvent_SoundWorld>(Mask);
	if (pEvent)
	{
		pEvent->m_X = (int)Pos.x;
		pEvent->m_Y = (int)Pos.y;
		pEvent->m_SoundID = Sound;
	}
}

void CGameContext::CreateSoundGlobal(int Sound, int Target)
{
	if (Sound < 0)
		return;

	CNetMsg_Sv_SoundGlobal Msg;
	Msg.m_SoundID = Sound;
	if (Target == -2)
		Server()->SendPackMsg(&Msg, MSGFLAG_NOSEND, -1, m_WorldID);
	else
	{
		int Flag = MSGFLAG_VITAL;
		if (Target != -1)
			Flag |= MSGFLAG_NORECORD;
		Server()->SendPackMsg(&Msg, Flag, Target, m_WorldID);
	}
}

void CGameContext::CreateExtraEffect(vec2 Pos, int Effect, CClientMask Mask)
{
	if (Effect)
	{
		CNetEvent_Finish *pEvent = m_Events.Create<CNetEvent_Finish>(Mask);
		if (pEvent)
		{
			pEvent->m_X = (int)Pos.x;
			pEvent->m_Y = (int)Pos.y;
		}
	}
	else
	{
		CNetEvent_Birthday *pEvent = m_Events.Create<CNetEvent_Birthday>(Mask);
		if (pEvent)
		{
			pEvent->m_X = (int)Pos.x;
			pEvent->m_Y = (int)Pos.y;
		}
	}
}

void CGameContext::CreateMapSound(vec2 Pos, int MapSoundID, CClientMask Mask)
{
	CNetEvent_MapSoundWorld *pEvent = m_Events.Create<CNetEvent_MapSoundWorld>(Mask);
	if (pEvent)
	{
		pEvent->m_X = (int)Pos.x;
		pEvent->m_Y = (int)Pos.y;
		pEvent->m_SoundID = MapSoundID;
	}
}

void CGameContext::CreateMapSoundGlobal(int MapSoundID, int Target)
{
	if (MapSoundID < 0)
		return;

	CNetMsg_Sv_MapSoundGlobal Msg;
	Msg.m_SoundID = MapSoundID;
	if (Target == -2)
		Server()->SendPackMsg(&Msg, MSGFLAG_NOSEND, -1, m_WorldID);
	else
	{
		int Flag = MSGFLAG_VITAL;
		if (Target != -1)
			Flag |= MSGFLAG_NORECORD;
		Server()->SendPackMsg(&Msg, Flag, Target, m_WorldID);
	}
}

void CGameContext::CreateLaserDotEvent(vec2 Pos0, vec2 Pos1, int LifeSpan)
{
	CGameContext::LaserDotState State;
	State.m_Pos0 = Pos0;
	State.m_Pos1 = Pos1;
	State.m_LifeSpan = LifeSpan;
	State.m_SnapID = Server()->SnapNewID();
	
	m_LaserDots.add(State);
}

void CGameContext::CreateHammerDotEvent(vec2 Pos, int LifeSpan)
{
	CGameContext::HammerDotState State;
	State.m_Pos = Pos;
	State.m_LifeSpan = LifeSpan;
	State.m_SnapID = Server()->SnapNewID();
	
	m_HammerDots.add(State);
}

void CGameContext::CreateLoveEvent(vec2 Pos)
{
	CGameContext::LoveDotState State;
	State.m_Pos = Pos;
	State.m_LifeSpan = Server()->TickSpeed();
	State.m_SnapID = Server()->SnapNewID();
	
	m_LoveDots.add(State);
}

void CGameContext::SendChat(int ChatterClientID, int Team, const char *pText)
{
	char aBuf[256];
	if (ChatterClientID >= 0 && ChatterClientID < MAX_CLIENTS)
		str_format(aBuf, sizeof(aBuf), COLOR_YELLOW_BRIGHT "%d:%d:%s: %s", ChatterClientID, Team, Server()->ClientName(ChatterClientID), pText);
	else
		str_format(aBuf, sizeof(aBuf), COLOR_YELLOW_BRIGHT "*** %s", pText);
	Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, Team != CHAT_ALL ? "teamchat" : "chat", aBuf);

	if (Team == CHAT_ALL)
	{
		CNetMsg_Sv_Chat Msg;
		Msg.m_Team = 0;
		Msg.m_ClientID = ChatterClientID;
		Msg.m_pMessage = pText;
		Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, -1, -1);
	}
	else
	{
		CNetMsg_Sv_Chat Msg;
		Msg.m_Team = 1;
		Msg.m_ClientID = ChatterClientID;
		Msg.m_pMessage = pText;

		// pack one for the recording only
		Server()->SendPackMsg(&Msg, MSGFLAG_VITAL | MSGFLAG_NOSEND, -1, -1);

		// send to the clients
		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if (m_apPlayers[i] && m_apPlayers[i]->GetTeam() == Team)
				Server()->SendPackMsg(&Msg, MSGFLAG_VITAL | MSGFLAG_NORECORD, i, -1);
		}
	}
}

void CGameContext::SendEmoticon(int ClientID, int Emoticon)
{
	CNetMsg_Sv_Emoticon Msg;
	Msg.m_ClientID = ClientID;
	Msg.m_Emoticon = Emoticon;
	Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, -1, m_WorldID);
}

void CGameContext::SendWeaponPickup(int ClientID, int Weapon)
{
	CNetMsg_Sv_WeaponPickup Msg;
	Msg.m_Weapon = Weapon;
	Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, ClientID, -1);
}

//
void CGameContext::StartVote(const char *pDesc, const char *pCommand, const char *pReason)
{
	// check if a vote is already running
	if (m_VoteCloseTime)
		return;

	// reset votes
	m_VoteEnforce = VOTE_ENFORCE_UNKNOWN;
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (m_apPlayers[i])
		{
			m_apPlayers[i]->m_Vote = 0;
			m_apPlayers[i]->m_VotePos = 0;
		}
	}

	// start vote
	m_VoteCloseTime = time_get() + time_freq() * 25;
	str_copy(m_aVoteDescription, pDesc, sizeof(m_aVoteDescription));
	str_copy(m_aVoteCommand, pCommand, sizeof(m_aVoteCommand));
	str_copy(m_aVoteReason, pReason, sizeof(m_aVoteReason));
	SendVoteSet(-1);
	m_VoteUpdate = true;
}

void CGameContext::EndVote()
{
	m_VoteCloseTime = 0;
	SendVoteSet(-1);
}

void CGameContext::SendVoteSet(int ClientID)
{
	CNetMsg_Sv_VoteSet Msg;
	if (m_VoteCloseTime)
	{
		Msg.m_Timeout = (m_VoteCloseTime - time_get()) / time_freq();
		Msg.m_pDescription = m_aVoteDescription;
		Msg.m_pReason = m_aVoteReason;
	}
	else
	{
		Msg.m_Timeout = 0;
		Msg.m_pDescription = "";
		Msg.m_pReason = "";
	}
	Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, ClientID, m_WorldID);
}

void CGameContext::SendVoteStatus(int ClientID, int Total, int Yes, int No)
{
	CNetMsg_Sv_VoteStatus Msg = {0};
	Msg.m_Total = Total;
	Msg.m_Yes = Yes;
	Msg.m_No = No;
	Msg.m_Pass = Total - (Yes + No);

	Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, ClientID, m_WorldID);
}

void CGameContext::AbortVoteKickOnDisconnect(int ClientID)
{
	if (m_VoteCloseTime && ((!str_comp_num(m_aVoteCommand, "kick ", 5) && str_toint(&m_aVoteCommand[5]) == ClientID) ||
							(!str_comp_num(m_aVoteCommand, "set_team ", 9) && str_toint(&m_aVoteCommand[9]) == ClientID)))
		m_VoteCloseTime = -1;
}

void CGameContext::SendTuningParams(int ClientID)
{
	CMsgPacker Msg(NETMSGTYPE_SV_TUNEPARAMS);
	int *pParams = (int *)&m_Tuning;
	for (unsigned i = 0; i < sizeof(m_Tuning) / sizeof(int); i++)
		Msg.AddInt(pParams[i]);
	Server()->SendMsg(&Msg, MSGFLAG_VITAL, ClientID, -1);
}

void CGameContext::SwapTeams()
{
	if (!m_pController->IsTeamplay())
		return;

	Chat(-1, "Teams were swapped");

	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (m_apPlayers[i] && m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS)
			m_apPlayers[i]->SetTeam(m_apPlayers[i]->GetTeam() ^ 1, false);
	}

	(void)m_pController->CheckTeamBalance();
}

void CGameContext::OnTick()
{
	// Test basic move for bots
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (!m_apPlayers[i] || !m_apPlayers[i]->m_IsBot)
			continue;
		CNetObj_PlayerInput Input = m_apPlayers[i]->m_pBot->GetLastInputData();
		m_apPlayers[i]->OnPredictedInput(&Input);
	}

	// copy tuning
	m_World.m_Core.m_Tuning = m_Tuning;
	m_World.Tick();

	// if(world.paused) // make sure that the game object always updates
	m_pController->Tick();

	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (!m_apPlayers[i] || m_apPlayers[i]->GetPlayerWorldID() != m_WorldID)
			continue;

		m_apPlayers[i]->Tick();
		m_apPlayers[i]->PostTick();
	}

	int DotIter;
	
	DotIter = 0;
	while(DotIter < m_LaserDots.size())
	{
		m_LaserDots[DotIter].m_LifeSpan--;
		if(m_LaserDots[DotIter].m_LifeSpan <= 0)
		{
			Server()->SnapFreeID(m_LaserDots[DotIter].m_SnapID);
			m_LaserDots.remove_index(DotIter);
		}
		else
			DotIter++;
	}
	
	DotIter = 0;
	while(DotIter < m_HammerDots.size())
	{
		m_HammerDots[DotIter].m_LifeSpan--;
		if(m_HammerDots[DotIter].m_LifeSpan <= 0)
		{
			Server()->SnapFreeID(m_HammerDots[DotIter].m_SnapID);
			m_HammerDots.remove_index(DotIter);
		}
		else
			DotIter++;
	}
	
	DotIter = 0;
	while(DotIter < m_LoveDots.size())
	{
		m_LoveDots[DotIter].m_LifeSpan--;
		m_LoveDots[DotIter].m_Pos.y -= 5.0f;
		if(m_LoveDots[DotIter].m_LifeSpan <= 0)
		{
			Server()->SnapFreeID(m_LoveDots[DotIter].m_SnapID);
			m_LoveDots.remove_index(DotIter);
		}
		else
			DotIter++;
	}

	// update voting
	if (m_VoteCloseTime)
	{
		// abort the kick-vote on player-leave
		if (m_VoteCloseTime == -1)
		{
			Chat(-1, "Vote aborted");
			EndVote();
		}
		else
		{
			int Total = 0, Yes = 0, No = 0;
			if (m_VoteUpdate)
			{
				// count votes
				char aaBuf[MAX_CLIENTS][NETADDR_MAXSTRSIZE] = {{0}};
				for (int i = 0; i < MAX_CLIENTS; i++)
					if (m_apPlayers[i])
						Server()->GetClientAddr(i, aaBuf[i], NETADDR_MAXSTRSIZE);
				bool aVoteChecked[MAX_CLIENTS] = {0};
				for (int i = 0; i < MAX_CLIENTS; i++)
				{
					if (!m_apPlayers[i] || m_apPlayers[i]->GetTeam() == TEAM_SPECTATORS || aVoteChecked[i] || m_apPlayers[i]->m_IsBot) // don't count in votes by spectators
						continue;

					int ActVote = m_apPlayers[i]->m_Vote;
					int ActVotePos = m_apPlayers[i]->m_VotePos;

					// check for more players with the same ip (only use the vote of the one who voted first)
					for (int j = i + 1; j < MAX_CLIENTS; ++j)
					{
						if (!m_apPlayers[j] || aVoteChecked[j] || str_comp(aaBuf[j], aaBuf[i]))
							continue;

						aVoteChecked[j] = true;
						if (m_apPlayers[j]->m_Vote && (!ActVote || ActVotePos > m_apPlayers[j]->m_VotePos))
						{
							ActVote = m_apPlayers[j]->m_Vote;
							ActVotePos = m_apPlayers[j]->m_VotePos;
						}
					}

					Total++;
					if (ActVote > 0)
						Yes++;
					else if (ActVote < 0)
						No++;
				}

				if (Yes >= Total / 2 + 1)
					m_VoteEnforce = VOTE_ENFORCE_YES;
				else if (No >= (Total + 1) / 2)
					m_VoteEnforce = VOTE_ENFORCE_NO;
			}

			if (m_VoteEnforce == VOTE_ENFORCE_YES)
			{
				Server()->SetRconCID(IServer::RCON_CID_VOTE);
				Console()->ExecuteLine(m_aVoteCommand, -1);
				Server()->SetRconCID(IServer::RCON_CID_SERV);
				EndVote();
				Chat(-1, "Vote passed");

				if (m_apPlayers[m_VoteCreator])
					m_apPlayers[m_VoteCreator]->m_LastVoteCall = 0;
			}
			else if (m_VoteEnforce == VOTE_ENFORCE_NO || time_get() > m_VoteCloseTime)
			{
				EndVote();
				Chat(-1, "Vote failed");
			}
			else if (m_VoteUpdate)
			{
				m_VoteUpdate = false;
				SendVoteStatus(-1, Total, Yes, No);
			}
		}
	}

	// Test basic move for bots
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (!m_apPlayers[i] || !m_apPlayers[i]->m_IsBot)
			continue;
		CNetObj_PlayerInput Input = m_apPlayers[i]->m_pBot->GetInputData();
		m_apPlayers[i]->OnDirectInput(&Input);
	}

#ifdef CONF_DEBUG
	if (g_Config.m_DbgDummies)
	{
		for (int i = 0; i < g_Config.m_DbgDummies; i++)
		{
			CNetObj_PlayerInput Input = {0};
			Input.m_Direction = (i & 1) ? -1 : 1;
			m_apPlayers[MAX_CLIENTS - i - 1]->OnPredictedInput(&Input);
		}
	}
#endif

	if (Server()->Tick() % (50 * 60 * 5) == 0) // Every 5 mins;
		Chat(-1, "Server official QQ group: 1007351135");
}

// Server hooks
void CGameContext::OnClientDirectInput(int ClientID, void *pInput)
{
	if (!m_World.m_Paused)
		m_apPlayers[ClientID]->OnDirectInput((CNetObj_PlayerInput *)pInput);
}

void CGameContext::OnClientPredictedInput(int ClientID, void *pInput)
{
	if (!m_World.m_Paused)
		m_apPlayers[ClientID]->OnPredictedInput((CNetObj_PlayerInput *)pInput);
}

void CGameContext::OnClientEnter(int ClientID)
{
	CPlayer *pPlayer = m_apPlayers[ClientID];
	if (!pPlayer)
		return;

	m_pController->OnPlayerConnect(pPlayer);

	m_apPlayers[ClientID]->Respawn();

	m_VoteUpdate = true;
}

void CGameContext::KillCharacter(int ClientID)
{
	if (m_apPlayers[ClientID])
	{
		if (m_apPlayers[ClientID]->GetCharacter())
			m_apPlayers[ClientID]->KillCharacter(-1);
		// m_apPlayers->SetSpawning(false);
	}
}

void CGameContext::OnClientConnected(int ClientID)
{
	// Check which team the player should be on
	const int StartTeam = g_Config.m_SvTournamentMode ? TEAM_SPECTATORS : m_pController->GetAutoTeam(ClientID);

	if (!m_apPlayers[ClientID])
	{
		const int AllocMemoryCell = ClientID + m_WorldID * MAX_CLIENTS;
		m_apPlayers[ClientID] = new (AllocMemoryCell) CPlayer(this, ClientID, StartTeam);
	}

	// send active vote
	if (m_VoteCloseTime)
		SendVoteSet(ClientID);

	// send motd
	CNetMsg_Sv_Motd Msg;
	Msg.m_pMessage = g_Config.m_SvMotd;
	Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, ClientID, -1);
}

void CGameContext::OnClientDrop(int ClientID, const char *pReason)
{
	if (!m_apPlayers[ClientID])
		return;

	AbortVoteKickOnDisconnect(ClientID);

	// update clients on drop
	m_pController->OnPlayerDisconnect(m_apPlayers[ClientID]);
	m_apPlayers[ClientID]->OnDisconnect();

	(void)m_pController->CheckTeamBalance();
	m_VoteUpdate = true;

	// update spectator modes
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (m_apPlayers[i] && m_apPlayers[i]->m_SpectatorID == ClientID)
			m_apPlayers[i]->m_SpectatorID = SPEC_FREEVIEW;
	}

	// check if all players left and game is paused => unpause
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (m_apPlayers[i] && m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS)
			return;
	}
	if (m_World.m_Paused)
	{
		m_pController->TogglePause();
	}

	delete m_apPlayers[ClientID];
	m_apPlayers[ClientID] = nullptr;
}

void CGameContext::OnMessage(int MsgID, CUnpacker *pUnpacker, int ClientID)
{
	void *pRawMsg = m_NetObjHandler.SecureUnpackMsg(MsgID, pUnpacker);
	CPlayer *pPlayer = m_apPlayers[ClientID];

	if (!pRawMsg)
	{
		if (g_Config.m_Debug)
		{
			char aBuf[256];
			str_format(aBuf, sizeof(aBuf), "dropped weird message '%s' (%d), failed on '%s'", m_NetObjHandler.GetMsgName(MsgID), MsgID, m_NetObjHandler.FailedMsgOn());
			Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "server", aBuf);
		}
		return;
	}

	if (Server()->ClientIngame(ClientID))
	{
		if (MsgID == NETMSGTYPE_CL_SAY)
		{
			if (g_Config.m_SvSpamprotection && pPlayer->m_LastChat && pPlayer->m_LastChat + Server()->TickSpeed() > Server()->Tick())
				return;

			CNetMsg_Cl_Say *pMsg = (CNetMsg_Cl_Say *)pRawMsg;
			int Team = pMsg->m_Team ? pPlayer->GetTeam() : CGameContext::CHAT_ALL;

			// trim right and set maximum length to 128 utf8-characters
			int Length = 0;
			const char *p = pMsg->m_pMessage;
			const char *pEnd = 0;
			while (*p)
			{
				const char *pStrOld = p;
				int Code = str_utf8_decode(&p);

				// check if unicode is not empty
				if (Code > 0x20 && Code != 0xA0 && Code != 0x034F && (Code < 0x2000 || Code > 0x200F) && (Code < 0x2028 || Code > 0x202F) &&
					(Code < 0x205F || Code > 0x2064) && (Code < 0x206A || Code > 0x206F) && (Code < 0xFE00 || Code > 0xFE0F) &&
					Code != 0xFEFF && (Code < 0xFFF9 || Code > 0xFFFC))
				{
					pEnd = 0;
				}
				else if (pEnd == 0)
					pEnd = pStrOld;

				if (++Length >= 127)
				{
					*(const_cast<char *>(p)) = 0;
					break;
				}
			}
			if (pEnd != 0)
				*(const_cast<char *>(pEnd)) = 0;

			// drop empty and autocreated spam messages (more than 16 characters per second)
			if (Length == 0 || (pMsg->m_pMessage[0] != '/' && g_Config.m_SvSpamprotection && pPlayer->m_LastChat && pPlayer->m_LastChat + Server()->TickSpeed() * ((15 + Length) / 16) > Server()->Tick()))
				return;

			pPlayer->m_LastChat = Server()->Tick();

			if (pMsg->m_pMessage[0] == '/' || pMsg->m_pMessage[0] == '\\')
			{
				switch (m_apPlayers[ClientID]->m_Authed)
				{
				case IServer::AUTHED_ADMIN:
					Console()->SetAccessLevel(IConsole::ACCESS_LEVEL_ADMIN);
					break;
				case IServer::AUTHED_MOD:
					Console()->SetAccessLevel(IConsole::ACCESS_LEVEL_MOD);
					break;
				default:
					Console()->SetAccessLevel(IConsole::ACCESS_LEVEL_USER);
				}

				m_ConsoleOutput_Target = ClientID;

				Console()->ExecuteLineFlag(pMsg->m_pMessage + 1, ClientID, CFGFLAG_CHAT, m_apPlayers[ClientID]->GetLanguage());

				m_ConsoleOutput_Target = -1;

				Console()->SetAccessLevel(IConsole::ACCESS_LEVEL_ADMIN);
			}
			else
				SendChat(ClientID, Team, pMsg->m_pMessage);
		}
		else if (MsgID == NETMSGTYPE_CL_CALLVOTE)
		{
			char aChatmsg[512] = {0};
			char aDesc[VOTE_DESC_LENGTH] = {0};
			char aCmd[VOTE_CMD_LENGTH] = {0};
			CNetMsg_Cl_CallVote *pMsg = (CNetMsg_Cl_CallVote *)pRawMsg;
			const char *pReason = pMsg->m_pReason[0] ? pMsg->m_pReason : "No reason given";

			if (str_comp_nocase(pMsg->m_pType, "option") == 0)
			{
				for (int i = 0; i < m_aPlayerVotes[ClientID].m_aVoteOptions.size(); ++i)
				{
					if (str_comp_nocase(pMsg->m_pValue, m_aPlayerVotes[ClientID].m_aVoteOptions[i].m_aDescription) == 0)
					{
						str_format(aDesc, sizeof(aDesc), "%s", m_aPlayerVotes[ClientID].m_aVoteOptions[i].m_aDescription);
						str_format(aCmd, sizeof(aCmd), "%s", m_aPlayerVotes[ClientID].m_aVoteOptions[i].m_aCommand);
					}
				}
			}

			std::string Command(aCmd);
			if (str_comp(aCmd, "ccv_null") == 0)
			{
				return;
			}
			else if (Command.find("ccv_") == 0)
			{
				switch (m_apPlayers[ClientID]->m_Authed)
				{
				case IServer::AUTHED_ADMIN:
					Console()->SetAccessLevel(IConsole::ACCESS_LEVEL_ADMIN);
					break;
				case IServer::AUTHED_MOD:
					Console()->SetAccessLevel(IConsole::ACCESS_LEVEL_MOD);
					break;
				default:
					Console()->SetAccessLevel(IConsole::ACCESS_LEVEL_USER);
				}

				Console()->ExecuteLineFlag(aCmd + 4, ClientID, CFGFLAG_VOTE);
				Console()->SetAccessLevel(IConsole::ACCESS_LEVEL_ADMIN);

				return;
			}

			if (g_Config.m_SvSpamprotection && pPlayer->m_LastVoteTry && pPlayer->m_LastVoteTry + Server()->TickSpeed() * 3 > Server()->Tick())
				return;

			int64 Now = Server()->Tick();
			pPlayer->m_LastVoteTry = Now;
			if (pPlayer->GetTeam() == TEAM_SPECTATORS)
			{
				Chat(ClientID, "Spectators aren't allowed to start a vote.");
				return;
			}

			if (m_VoteCloseTime)
			{
				Chat(ClientID, "Wait for current vote to end before calling a new one.");
				return;
			}

			int Timeleft = pPlayer->m_LastVoteCall + Server()->TickSpeed() * 60 - Now;
			if (pPlayer->m_LastVoteCall && Timeleft > 0)
			{
				int Sec = (Timeleft / Server()->TickSpeed()) + 1;
				Chat(ClientID, "You must wait {} seconds before making another vote", Sec);
				return;
			}

			if (str_comp_nocase(pMsg->m_pType, "kick") == 0)
			{
				if (!g_Config.m_SvVoteKick)
				{
					Chat(ClientID, "Server does not allow voting to kick players");
					return;
				}

				if (g_Config.m_SvVoteKickMin)
				{
					int PlayerNum = 0;
					for (int i = 0; i < MAX_CLIENTS; ++i)
						if (m_apPlayers[i] && m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS)
							++PlayerNum;

					if (PlayerNum < g_Config.m_SvVoteKickMin)
					{
						Chat(ClientID, "Kick voting requires {} players on the server", g_Config.m_SvVoteKickMin);
						return;
					}
				}

				int KickID = str_toint(pMsg->m_pValue);
				if (KickID < 0 || KickID >= MAX_CLIENTS || !m_apPlayers[KickID])
				{
					Chat(ClientID, "Invalid client id to kick");
					return;
				}
				if (KickID == ClientID)
				{
					Chat(ClientID, "You can't kick yourself");
					return;
				}
				if (Server()->IsAuthed(KickID))
				{
					Chat(ClientID, "You can't kick admins");
					Chat(KickID, "'{}' called for vote to kick you", Server()->ClientName(ClientID));
					return;
				}

				str_format(aChatmsg, sizeof(aChatmsg), "'%s' called for vote to kick '%s' (%s)", Server()->ClientName(ClientID), Server()->ClientName(KickID), pReason);
				str_format(aDesc, sizeof(aDesc), "Kick '%s'", Server()->ClientName(KickID));
				if (!g_Config.m_SvVoteKickBantime)
					str_format(aCmd, sizeof(aCmd), "kick %d Kicked by vote", KickID);
				else
				{
					char aAddrStr[NETADDR_MAXSTRSIZE] = {0};
					Server()->GetClientAddr(KickID, aAddrStr, sizeof(aAddrStr));
					str_format(aCmd, sizeof(aCmd), "ban %s %d Banned by vote", aAddrStr, g_Config.m_SvVoteKickBantime);
				}
			}
			else if (str_comp_nocase(pMsg->m_pType, "spectate") == 0)
			{
				if (!g_Config.m_SvVoteSpectate)
				{
					Chat(ClientID, "Server does not allow voting to move players to spectators");
					return;
				}

				int SpectateID = str_toint(pMsg->m_pValue);
				if (SpectateID < 0 || SpectateID >= MAX_CLIENTS || !m_apPlayers[SpectateID] || m_apPlayers[SpectateID]->GetTeam() == TEAM_SPECTATORS)
				{
					Chat(ClientID, "Invalid client id to move");
					return;
				}
				if (SpectateID == ClientID)
				{
					Chat(ClientID, "You can't move yourself");
					return;
				}

				str_format(aChatmsg, sizeof(aChatmsg), "'%s' called for vote to move '%s' to spectators (%s)", Server()->ClientName(ClientID), Server()->ClientName(SpectateID), pReason);
				str_format(aDesc, sizeof(aDesc), "move '%s' to spectators", Server()->ClientName(SpectateID));
				str_format(aCmd, sizeof(aCmd), "set_team %d -1 %d", SpectateID, g_Config.m_SvVoteSpectateRejoindelay);
			}

			if (aCmd[0])
			{
				Chat(-1, aChatmsg);
				StartVote(aDesc, aCmd, pReason);
				pPlayer->m_Vote = 1;
				pPlayer->m_VotePos = m_VotePos = 1;
				m_VoteCreator = ClientID;
				pPlayer->m_LastVoteCall = Now;
			}
		}
		else if (MsgID == NETMSGTYPE_CL_VOTE)
		{
			if (!m_VoteCloseTime)
				return;

			if (pPlayer->m_Vote == 0)
			{
				CNetMsg_Cl_Vote *pMsg = (CNetMsg_Cl_Vote *)pRawMsg;
				if (!pMsg->m_Vote)
					return;

				pPlayer->m_Vote = pMsg->m_Vote;
				pPlayer->m_VotePos = ++m_VotePos;
				m_VoteUpdate = true;
			}
		}
		else if (MsgID == NETMSGTYPE_CL_SETTEAM && !m_World.m_Paused)
		{
			CNetMsg_Cl_SetTeam *pMsg = (CNetMsg_Cl_SetTeam *)pRawMsg;

			if (pPlayer->GetTeam() == pMsg->m_Team || (g_Config.m_SvSpamprotection && pPlayer->m_LastSetTeam && pPlayer->m_LastSetTeam + Server()->TickSpeed() * 3 > Server()->Tick()))
				return;

			if (pMsg->m_Team != TEAM_SPECTATORS && m_LockTeams)
			{
				pPlayer->m_LastSetTeam = Server()->Tick();
				Broadcast(ClientID, "Teams are locked");
				return;
			}

			if (pPlayer->m_TeamChangeTick > Server()->Tick())
			{
				pPlayer->m_LastSetTeam = Server()->Tick();
				int TimeLeft = (pPlayer->m_TeamChangeTick - Server()->Tick()) / Server()->TickSpeed();
				Broadcast(ClientID, "Time to wait before changing team: {}:{}", TimeLeft / 60, TimeLeft % 60);
				return;
			}

			// Switch team on given client and kill/respawn him
			if (m_pController->CanJoinTeam(pMsg->m_Team, ClientID))
			{
				if (m_pController->CanChangeTeam(pPlayer, pMsg->m_Team))
				{
					pPlayer->m_LastSetTeam = Server()->Tick();
					if (pPlayer->GetTeam() == TEAM_SPECTATORS || pMsg->m_Team == TEAM_SPECTATORS)
						m_VoteUpdate = true;
					pPlayer->SetTeam(pMsg->m_Team);
					(void)m_pController->CheckTeamBalance();
					pPlayer->m_TeamChangeTick = Server()->Tick();
				}
				else
					Broadcast(ClientID, "Teams must be balanced, please join other team");
			}
			else
			{
				int AllowCount = Server()->MaxClients() - g_Config.m_SvSpectatorSlots;
				Broadcast(ClientID, "Only {} active players are allowed", AllowCount);
			}
		}
		else if (MsgID == NETMSGTYPE_CL_SETSPECTATORMODE && !m_World.m_Paused)
		{
			CNetMsg_Cl_SetSpectatorMode *pMsg = (CNetMsg_Cl_SetSpectatorMode *)pRawMsg;

			if (pPlayer->GetTeam() != TEAM_SPECTATORS || pPlayer->m_SpectatorID == pMsg->m_SpectatorID || ClientID == pMsg->m_SpectatorID ||
				(g_Config.m_SvSpamprotection && pPlayer->m_LastSetSpectatorMode && pPlayer->m_LastSetSpectatorMode + Server()->TickSpeed() * 3 > Server()->Tick()))
				return;

			pPlayer->m_LastSetSpectatorMode = Server()->Tick();
			if (pMsg->m_SpectatorID != SPEC_FREEVIEW && (!m_apPlayers[pMsg->m_SpectatorID] || m_apPlayers[pMsg->m_SpectatorID]->GetTeam() == TEAM_SPECTATORS))
				Chat(ClientID, "Invalid spectator id used");
			else
				pPlayer->m_SpectatorID = pMsg->m_SpectatorID;
		}
		else if (MsgID == NETMSGTYPE_CL_CHANGEINFO)
		{
			if (g_Config.m_SvSpamprotection && pPlayer->m_LastChangeInfo && pPlayer->m_LastChangeInfo + Server()->TickSpeed() * 5 > Server()->Tick())
				return;

			CNetMsg_Cl_ChangeInfo *pMsg = (CNetMsg_Cl_ChangeInfo *)pRawMsg;
			pPlayer->m_LastChangeInfo = Server()->Tick();

			// set infos
			char aOldName[MAX_NAME_LENGTH];
			str_copy(aOldName, Server()->ClientName(ClientID), sizeof(aOldName));
			Server()->SetClientName(ClientID, pMsg->m_pName);

			if (str_comp(aOldName, Server()->ClientName(ClientID)) != 0)
				Chat(-1, "'{}' changed name to '{}'", aOldName, Server()->ClientName(ClientID));

			Server()->SetClientClan(ClientID, pMsg->m_pClan);
			Server()->SetClientCountry(ClientID, pMsg->m_Country);
			str_copy(pPlayer->m_TeeInfos.m_SkinName, pMsg->m_pSkin, sizeof(pPlayer->m_TeeInfos.m_SkinName));
			pPlayer->m_TeeInfos.m_UseCustomColor = pMsg->m_UseCustomColor;
			pPlayer->m_TeeInfos.m_ColorBody = pMsg->m_ColorBody;
			pPlayer->m_TeeInfos.m_ColorFeet = pMsg->m_ColorFeet;
			m_pController->OnPlayerInfoChange(pPlayer);
		}
		else if (MsgID == NETMSGTYPE_CL_EMOTICON && !m_World.m_Paused)
		{
			CNetMsg_Cl_Emoticon *pMsg = (CNetMsg_Cl_Emoticon *)pRawMsg;

			if (g_Config.m_SvSpamprotection && pPlayer->m_LastEmote && pPlayer->m_LastEmote + Server()->TickSpeed() * 3 > Server()->Tick())
				return;

			pPlayer->m_LastEmote = Server()->Tick();

			SendEmoticon(ClientID, pMsg->m_Emoticon);
		}
		else if (MsgID == NETMSGTYPE_CL_KILL && !m_World.m_Paused)
		{
			if (pPlayer->m_LastKill && pPlayer->m_LastKill + Server()->TickSpeed() * 3 > Server()->Tick())
				return;

			pPlayer->m_LastKill = Server()->Tick();
			pPlayer->KillCharacter(WEAPON_SELF);
		}
	}
	else
	{
		if (MsgID == NETMSGTYPE_CL_STARTINFO)
		{
			if (pPlayer->m_IsReady)
				return;

			CNetMsg_Cl_StartInfo *pMsg = (CNetMsg_Cl_StartInfo *)pRawMsg;
			pPlayer->m_LastChangeInfo = Server()->Tick();

			// set start infos
			Server()->SetClientName(ClientID, pMsg->m_pName);
			Server()->SetClientClan(ClientID, pMsg->m_pClan);
			str_copy(pPlayer->m_TeeInfos.m_SkinName, pMsg->m_pSkin, sizeof(pPlayer->m_TeeInfos.m_SkinName));
			pPlayer->m_TeeInfos.m_UseCustomColor = pMsg->m_UseCustomColor;
			pPlayer->m_TeeInfos.m_ColorBody = pMsg->m_ColorBody;
			pPlayer->m_TeeInfos.m_ColorFeet = pMsg->m_ColorFeet;
			m_pController->OnPlayerInfoChange(pPlayer);

			// send vote options
			CNetMsg_Sv_VoteClearOptions ClearMsg;
			Server()->SendPackMsg(&ClearMsg, MSGFLAG_VITAL, ClientID, -1);

			CNetMsg_Sv_VoteOptionListAdd OptionMsg;
			int NumOptions = 0;
			OptionMsg.m_pDescription0 = "";
			OptionMsg.m_pDescription1 = "";
			OptionMsg.m_pDescription2 = "";
			OptionMsg.m_pDescription3 = "";
			OptionMsg.m_pDescription4 = "";
			OptionMsg.m_pDescription5 = "";
			OptionMsg.m_pDescription6 = "";
			OptionMsg.m_pDescription7 = "";
			OptionMsg.m_pDescription8 = "";
			OptionMsg.m_pDescription9 = "";
			OptionMsg.m_pDescription10 = "";
			OptionMsg.m_pDescription11 = "";
			OptionMsg.m_pDescription12 = "";
			OptionMsg.m_pDescription13 = "";
			OptionMsg.m_pDescription14 = "";
			CVoteOptionServer *pCurrent = m_pVoteOptionFirst;
			while (pCurrent)
			{
				switch (NumOptions++)
				{
				case 0:
					OptionMsg.m_pDescription0 = pCurrent->m_aDescription;
					break;
				case 1:
					OptionMsg.m_pDescription1 = pCurrent->m_aDescription;
					break;
				case 2:
					OptionMsg.m_pDescription2 = pCurrent->m_aDescription;
					break;
				case 3:
					OptionMsg.m_pDescription3 = pCurrent->m_aDescription;
					break;
				case 4:
					OptionMsg.m_pDescription4 = pCurrent->m_aDescription;
					break;
				case 5:
					OptionMsg.m_pDescription5 = pCurrent->m_aDescription;
					break;
				case 6:
					OptionMsg.m_pDescription6 = pCurrent->m_aDescription;
					break;
				case 7:
					OptionMsg.m_pDescription7 = pCurrent->m_aDescription;
					break;
				case 8:
					OptionMsg.m_pDescription8 = pCurrent->m_aDescription;
					break;
				case 9:
					OptionMsg.m_pDescription9 = pCurrent->m_aDescription;
					break;
				case 10:
					OptionMsg.m_pDescription10 = pCurrent->m_aDescription;
					break;
				case 11:
					OptionMsg.m_pDescription11 = pCurrent->m_aDescription;
					break;
				case 12:
					OptionMsg.m_pDescription12 = pCurrent->m_aDescription;
					break;
				case 13:
					OptionMsg.m_pDescription13 = pCurrent->m_aDescription;
					break;
				case 14:
				{
					OptionMsg.m_pDescription14 = pCurrent->m_aDescription;
					OptionMsg.m_NumOptions = NumOptions;
					Server()->SendPackMsg(&OptionMsg, MSGFLAG_VITAL, ClientID, -1);
					OptionMsg = CNetMsg_Sv_VoteOptionListAdd();
					NumOptions = 0;
					OptionMsg.m_pDescription1 = "";
					OptionMsg.m_pDescription2 = "";
					OptionMsg.m_pDescription3 = "";
					OptionMsg.m_pDescription4 = "";
					OptionMsg.m_pDescription5 = "";
					OptionMsg.m_pDescription6 = "";
					OptionMsg.m_pDescription7 = "";
					OptionMsg.m_pDescription8 = "";
					OptionMsg.m_pDescription9 = "";
					OptionMsg.m_pDescription10 = "";
					OptionMsg.m_pDescription11 = "";
					OptionMsg.m_pDescription12 = "";
					OptionMsg.m_pDescription13 = "";
					OptionMsg.m_pDescription14 = "";
				}
				}
				pCurrent = pCurrent->m_pNext;
			}
			if (NumOptions > 0)
			{
				OptionMsg.m_NumOptions = NumOptions;
				Server()->SendPackMsg(&OptionMsg, MSGFLAG_VITAL, ClientID, -1);
			}

			// send tuning parameters to client
			SendTuningParams(ClientID);

			// client is ready to enter
			pPlayer->m_IsReady = true;
			CNetMsg_Sv_ReadyToEnter m;
			Server()->SendPackMsg(&m, MSGFLAG_VITAL | MSGFLAG_FLUSH, ClientID, -1);
		}
	}
}

bool CGameContext::ConTuneParam(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	const char *pParamName = pResult->GetString(0);
	float NewValue = pResult->GetFloat(1);

	if (pSelf->Tuning()->Set(pParamName, NewValue))
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "%s changed to %.2f", pParamName, NewValue);
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "tuning", aBuf);
		pSelf->SendTuningParams(-1);
	}
	else
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "tuning", "No such tuning parameter");

	return true;
}

bool CGameContext::ConTuneReset(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	CTuningParams TuningParams;
	*pSelf->Tuning() = TuningParams;
	pSelf->SendTuningParams(-1);
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "tuning", "Tuning reset");
	return true;
}

bool CGameContext::ConTuneDump(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	char aBuf[256];
	for (int i = 0; i < pSelf->Tuning()->Num(); i++)
	{
		float v;
		pSelf->Tuning()->Get(i, &v);
		str_format(aBuf, sizeof(aBuf), "%s %.2f", pSelf->Tuning()->m_apNames[i], v);
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "tuning", aBuf);
	}
	return true;
}

bool CGameContext::ConPause(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->m_pController->TogglePause();
	return true;
}

bool CGameContext::ConRestart(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if (pResult->NumArguments())
		pSelf->m_pController->DoWarmup(pResult->GetInteger(0));
	else
		pSelf->m_pController->StartRound();
	return true;
}

bool CGameContext::ConBroadcast(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->Broadcast(-1, pResult->GetString(0));
	return true;
}

bool CGameContext::ConSay(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->Chat(-1, pResult->GetString(0));
	return true;
}

bool CGameContext::ConSetTeam(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int ClientID = clamp(pResult->GetInteger(0), 0, (int)MAX_CLIENTS - 1);
	int Team = clamp(pResult->GetInteger(1), -1, 1);
	int Delay = pResult->NumArguments() > 2 ? pResult->GetInteger(2) : 0;
	if (!pSelf->m_apPlayers[ClientID])
		return true;

	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "moved client %d to team %d", ClientID, Team);
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);

	pSelf->m_apPlayers[ClientID]->m_TeamChangeTick = pSelf->Server()->Tick() + pSelf->Server()->TickSpeed() * Delay * 60;
	pSelf->m_apPlayers[ClientID]->SetTeam(Team);
	(void)pSelf->m_pController->CheckTeamBalance();
	return true;
}

bool CGameContext::ConSetTeamAll(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Team = clamp(pResult->GetInteger(0), -1, 1);

	pSelf->Chat(-1, "All players were moved to the {}", pSelf->m_pController->GetTeamName(Team));

	for (int i = 0; i < MAX_CLIENTS; ++i)
		if (pSelf->m_apPlayers[i])
			pSelf->m_apPlayers[i]->SetTeam(Team, false);

	(void)pSelf->m_pController->CheckTeamBalance();
	return true;
}

bool CGameContext::ConSwapTeams(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->SwapTeams();
	return true;
}

bool CGameContext::ConShuffleTeams(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if (!pSelf->m_pController->IsTeamplay())
		return true;

	int CounterRed = 0;
	int CounterBlue = 0;
	int PlayerTeam = 0;
	for (int i = 0; i < MAX_CLIENTS; ++i)
		if (pSelf->m_apPlayers[i] && pSelf->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS)
			++PlayerTeam;
	PlayerTeam = (PlayerTeam + 1) / 2;

	pSelf->Chat(-1, "Teams were shuffled");

	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (pSelf->m_apPlayers[i] && pSelf->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS)
		{
			if (CounterRed == PlayerTeam)
				pSelf->m_apPlayers[i]->SetTeam(TEAM_BOT, false);
			else if (CounterBlue == PlayerTeam)
				pSelf->m_apPlayers[i]->SetTeam(TEAM_HUMAN, false);
			else
			{
				if (rand() % 2)
				{
					pSelf->m_apPlayers[i]->SetTeam(TEAM_BOT, false);
					++CounterBlue;
				}
				else
				{
					pSelf->m_apPlayers[i]->SetTeam(TEAM_HUMAN, false);
					++CounterRed;
				}
			}
		}
	}

	(void)pSelf->m_pController->CheckTeamBalance();
	return true;
}

bool CGameContext::ConLockTeams(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->m_LockTeams ^= 1;
	if (pSelf->m_LockTeams)
		pSelf->Chat(-1, "Teams were locked");
	else
		pSelf->Chat(-1, "Teams were unlocked");
	return true;
}

bool CGameContext::ConAddVote(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	const char *pDescription = pResult->GetString(0);
	const char *pCommand = pResult->GetString(1);

	if (pSelf->m_NumVoteOptions == MAX_VOTE_OPTIONS)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", "maximum number of vote options reached");
		return true;
	}

	// check for valid option
	if (!pSelf->Console()->LineIsValid(pCommand) || str_length(pCommand) >= VOTE_CMD_LENGTH)
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "skipped invalid command '%s'", pCommand);
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
		return true;
	}
	while (*pDescription && *pDescription == ' ')
		pDescription++;
	if (str_length(pDescription) >= VOTE_DESC_LENGTH || *pDescription == 0)
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "skipped invalid option '%s'", pDescription);
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
		return true;
	}

	// check for duplicate entry
	CVoteOptionServer *pOption = pSelf->m_pVoteOptionFirst;
	while (pOption)
	{
		if (str_comp_nocase(pDescription, pOption->m_aDescription) == 0)
		{
			char aBuf[256];
			str_format(aBuf, sizeof(aBuf), "option '%s' already exists", pDescription);
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
			return true;
		}
		pOption = pOption->m_pNext;
	}

	// add the option
	++pSelf->m_NumVoteOptions;
	int Len = str_length(pCommand);

	pOption = (CVoteOptionServer *)pSelf->m_pVoteOptionHeap->Allocate(sizeof(CVoteOptionServer) + Len);
	pOption->m_pNext = 0;
	pOption->m_pPrev = pSelf->m_pVoteOptionLast;
	if (pOption->m_pPrev)
		pOption->m_pPrev->m_pNext = pOption;
	pSelf->m_pVoteOptionLast = pOption;
	if (!pSelf->m_pVoteOptionFirst)
		pSelf->m_pVoteOptionFirst = pOption;

	str_copy(pOption->m_aDescription, pDescription, sizeof(pOption->m_aDescription));
	mem_copy(pOption->m_aCommand, pCommand, Len + 1);
	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "added option '%s' '%s'", pOption->m_aDescription, pOption->m_aCommand);
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);

	// inform clients about added option
	CNetMsg_Sv_VoteOptionAdd OptionMsg;
	OptionMsg.m_pDescription = pOption->m_aDescription;
	pSelf->Server()->SendPackMsg(&OptionMsg, MSGFLAG_VITAL, -1, -1);
	return true;
}

bool CGameContext::ConRemoveVote(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	const char *pDescription = pResult->GetString(0);

	// check for valid option
	CVoteOptionServer *pOption = pSelf->m_pVoteOptionFirst;
	while (pOption)
	{
		if (str_comp_nocase(pDescription, pOption->m_aDescription) == 0)
			break;
		pOption = pOption->m_pNext;
	}
	if (!pOption)
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "option '%s' does not exist", pDescription);
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
		return true;
	}

	// inform clients about removed option
	CNetMsg_Sv_VoteOptionRemove OptionMsg;
	OptionMsg.m_pDescription = pOption->m_aDescription;
	pSelf->Server()->SendPackMsg(&OptionMsg, MSGFLAG_VITAL, -1, -1);

	// TODO: improve this
	// remove the option
	--pSelf->m_NumVoteOptions;
	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "removed option '%s' '%s'", pOption->m_aDescription, pOption->m_aCommand);
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);

	CHeap *pVoteOptionHeap = new CHeap();
	CVoteOptionServer *pVoteOptionFirst = 0;
	CVoteOptionServer *pVoteOptionLast = 0;
	int NumVoteOptions = pSelf->m_NumVoteOptions;
	for (CVoteOptionServer *pSrc = pSelf->m_pVoteOptionFirst; pSrc; pSrc = pSrc->m_pNext)
	{
		if (pSrc == pOption)
			continue;

		// copy option
		int Len = str_length(pSrc->m_aCommand);
		CVoteOptionServer *pDst = (CVoteOptionServer *)pVoteOptionHeap->Allocate(sizeof(CVoteOptionServer) + Len);
		pDst->m_pNext = 0;
		pDst->m_pPrev = pVoteOptionLast;
		if (pDst->m_pPrev)
			pDst->m_pPrev->m_pNext = pDst;
		pVoteOptionLast = pDst;
		if (!pVoteOptionFirst)
			pVoteOptionFirst = pDst;

		str_copy(pDst->m_aDescription, pSrc->m_aDescription, sizeof(pDst->m_aDescription));
		mem_copy(pDst->m_aCommand, pSrc->m_aCommand, Len + 1);
	}

	// clean up
	delete pSelf->m_pVoteOptionHeap;
	pSelf->m_pVoteOptionHeap = pVoteOptionHeap;
	pSelf->m_pVoteOptionFirst = pVoteOptionFirst;
	pSelf->m_pVoteOptionLast = pVoteOptionLast;
	pSelf->m_NumVoteOptions = NumVoteOptions;
	return true;
}

bool CGameContext::ConForceVote(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	const char *pType = pResult->GetString(0);
	const char *pValue = pResult->GetString(1);
	const char *pReason = pResult->NumArguments() > 2 && pResult->GetString(2)[0] ? pResult->GetString(2) : "No reason given";
	char aBuf[128] = {0};

	if (str_comp_nocase(pType, "kick") == 0)
	{
		int KickID = str_toint(pValue);
		if (KickID < 0 || KickID >= MAX_CLIENTS || !pSelf->m_apPlayers[KickID])
		{
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", "Invalid client id to kick");
			return true;
		}

		if (!g_Config.m_SvVoteKickBantime)
		{
			str_format(aBuf, sizeof(aBuf), "kick %d %s", KickID, pReason);
			pSelf->Console()->ExecuteLine(aBuf, -1);
		}
		else
		{
			char aAddrStr[NETADDR_MAXSTRSIZE] = {0};
			pSelf->Server()->GetClientAddr(KickID, aAddrStr, sizeof(aAddrStr));
			str_format(aBuf, sizeof(aBuf), "ban %s %d %s", aAddrStr, g_Config.m_SvVoteKickBantime, pReason);
			pSelf->Console()->ExecuteLine(aBuf, -1);
		}
	}
	else if (str_comp_nocase(pType, "spectate") == 0)
	{
		int SpectateID = str_toint(pValue);
		if (SpectateID < 0 || SpectateID >= MAX_CLIENTS || !pSelf->m_apPlayers[SpectateID] || pSelf->m_apPlayers[SpectateID]->GetTeam() == TEAM_SPECTATORS)
		{
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", "Invalid client id to move");
			return true;
		}

		pSelf->Chat(-1, "admin moved '{}' to spectator ({})", pSelf->Server()->ClientName(SpectateID), pReason);
		str_format(aBuf, sizeof(aBuf), "set_team %d -1 %d", SpectateID, g_Config.m_SvVoteSpectateRejoindelay);
		pSelf->Console()->ExecuteLine(aBuf, -1);
	}
	return true;
}

bool CGameContext::ConClearVotes(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", "cleared votes");
	CNetMsg_Sv_VoteClearOptions VoteClearOptionsMsg;
	pSelf->Server()->SendPackMsg(&VoteClearOptionsMsg, MSGFLAG_VITAL, -1, -1);
	pSelf->m_pVoteOptionHeap->Reset();
	pSelf->m_pVoteOptionFirst = 0;
	pSelf->m_pVoteOptionLast = 0;
	pSelf->m_NumVoteOptions = 0;
	return true;
}

bool CGameContext::ConVote(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

	// check if there is a vote running
	if (!pSelf->m_VoteCloseTime)
		return true;

	if (str_comp_nocase(pResult->GetString(0), "yes") == 0)
		pSelf->m_VoteEnforce = CGameContext::VOTE_ENFORCE_YES;
	else if (str_comp_nocase(pResult->GetString(0), "no") == 0)
		pSelf->m_VoteEnforce = CGameContext::VOTE_ENFORCE_NO;

	pSelf->Chat(-1, "admin forced vote {}", pResult->GetString(0));

	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "forcing vote %s", pResult->GetString(0));
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);

	return true;
}

void CGameContext::ConchainSpecialMotdupdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	pfnCallback(pResult, pCallbackUserData);
	if (pResult->NumArguments())
	{
		CNetMsg_Sv_Motd Msg;
		Msg.m_pMessage = g_Config.m_SvMotd;
		CGameContext *pSelf = (CGameContext *)pUserData;
		for (int i = 0; i < MAX_CLIENTS; ++i)
			if (pSelf->m_apPlayers[i])
				pSelf->Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, i, -1);
	}
}

bool CGameContext::ConAbout(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pThis = (CGameContext *)pUserData;

	pThis->Chat(pResult->GetClientID(), "{} {} by {}", MOD_NAME, MOD_VERSION, MOD_AUTHORS);

	if (MOD_CREDITS[0])
		pThis->Chat(pResult->GetClientID(), "Credits: {}", MOD_CREDITS);
	if (MOD_THANKS[0])
		pThis->Chat(pResult->GetClientID(), "Thanks to: {}", MOD_THANKS);
	if (MOD_SOURCES[0])
		pThis->Chat(pResult->GetClientID(), "Sources: {}", MOD_SOURCES);

	return true;
}

bool CGameContext::ConLanguage(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

	int ClientID = pResult->GetClientID();

	const char *pLanguageCode = (pResult->NumArguments() > 0) ? pResult->GetString(0) : 0x0;
	char aFinalLanguageCode[8];
	aFinalLanguageCode[0] = 0;

	if (pLanguageCode)
	{
		if (str_comp_nocase(pLanguageCode, "ua") == 0)
			str_copy(aFinalLanguageCode, "uk", sizeof(aFinalLanguageCode));
		else
		{
			for (int i = 0; i < pSelf->Server()->Localization()->m_pLanguages.size(); i++)
			{
				if (str_comp_nocase(pLanguageCode, pSelf->Server()->Localization()->m_pLanguages[i]->GetFilename()) == 0)
					str_copy(aFinalLanguageCode, pLanguageCode, sizeof(aFinalLanguageCode));
			}
		}
	}

	if (aFinalLanguageCode[0])
	{
		pSelf->SetClientLanguage(ClientID, aFinalLanguageCode);
		pSelf->Chat(ClientID, "Language successfully switched to English");
	}
	else
	{
		pSelf->Chat(ClientID, "Unknown language");

		dynamic_string BufferList;
		int BufferIter = 0;
		for (int i = 0; i < pSelf->Server()->Localization()->m_pLanguages.size(); i++)
		{
			if (i > 0)
				BufferIter = BufferList.append_at(BufferIter, ", ");
			BufferIter = BufferList.append_at(BufferIter, pSelf->Server()->Localization()->m_pLanguages[i]->GetFilename());
		}

		pSelf->Chat(ClientID, "Available languages: {}", BufferList.buffer());
	}

	return true;
}

void CGameContext::SetClientLanguage(int ClientID, const char *pLanguage)
{
	Server()->SetClientLanguage(ClientID, pLanguage);
	if (m_apPlayers[ClientID])
	{
		m_apPlayers[ClientID]->SetLanguage(pLanguage);
	}
}

void CGameContext::ChatConsolePrintCallback(const char *pLine, void *pUser)
{
	CGameContext *pSelf = (CGameContext *)pUser;
	int ClientID = pSelf->m_ConsoleOutput_Target;

	if (ClientID < 0 || ClientID >= MAX_CLIENTS)
		return;

	const char *pLineOrig = pLine;

	static volatile int ReentryGuard = 0;

	if (ReentryGuard)
		return;
	ReentryGuard++;

	if (*pLine == '[')
		do
			pLine++;
		while ((pLine - 2 < pLineOrig || *(pLine - 2) != ':') && *pLine != 0); // remove the category (e.g. [Console]: No Such Command)

	pSelf->Chat(ClientID, pLine);

	ReentryGuard--;
}

bool CGameContext::ConChatAI(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pThis = (CGameContext *)pUserData;

	int CID = pResult->GetClientID();

	if (!pThis->m_apPlayers[CID])
		return false;

	if (!g_Config.m_SvChatAI)
	{
		pThis->Chat(CID, "This server has disabled chat AI.");
		return false;
	}

	if (pResult->NumArguments() < 1)
		return false;

	pThis->Chat(CID, "Chat AI(module: {}) has received your message and is currently processing it.", g_Config.m_SvChatAIModule);

	pThis->m_pChatAI->Send(pThis, CID, pThis->Server()->ClientName(CID), pResult->GetString(0));

	return true;
}

bool CGameContext::ConRegister(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

	if (pSelf->GetPlayer(pResult->GetClientID())->LoggedIn())
	{
		pSelf->Chat(pResult->GetClientID(), "You're already logged in.");
		return false;
	}

	if (pResult->NumArguments() != 2)
	{
		pSelf->Chat(pResult->GetClientID(), "Usage: /register <username> <password>");
		return false;
	}

	char Username[512];
	char Password[512];
	str_copy(Username, pResult->GetString(0), sizeof(Username));
	str_copy(Password, pResult->GetString(1), sizeof(Password));

	pSelf->TW()->Account()->Register(pResult->GetClientID(), Username, Password);

	return true;
}

bool CGameContext::ConLogin(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if (pSelf->GetPlayer(pResult->GetClientID())->LoggedIn())
	{
		pSelf->Chat(pResult->GetClientID(), "You're already logged in.");
		return false;
	}

	if (pResult->NumArguments() != 2)
	{
		pSelf->Chat(pResult->GetClientID(), "usage: /login <username> <password>");
		return false;
	}

	char Username[512];
	char Password[512];
	str_copy(Username, pResult->GetString(0), sizeof(Username));
	str_copy(Password, pResult->GetString(1), sizeof(Password));

	pSelf->TW()->Account()->Login(pResult->GetClientID(), Username, Password);
	pSelf->ClearVotes(pResult->GetClientID());

	return true;
}

bool CGameContext::VotGiveItem(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if (!pSelf->GetPlayer(pResult->GetInteger(0)))
		return false;

	pSelf->GetPlayer(pResult->GetInteger(0))->m_AccData.m_aItems[pResult->GetInteger(1)].m_Num += pResult->GetInteger(2);
	pSelf->TW()->Account()->SaveAccountData(pResult->GetInteger(0), CGameContext::TABLE_ITEM, pSelf->GetPlayer(pResult->GetInteger(0))->m_AccData);
	pSelf->ClearVotes(pResult->GetInteger(0));
	return true;
}

bool CGameContext::VotSelectItem(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->GetPlayerVote(pResult->GetClientID())->m_Select[pResult->GetInteger(0)] = pResult->GetInteger(1);
	pSelf->ClearVotes(pResult->GetClientID());
	return true;
}

bool CGameContext::VotGoto(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->m_aPlayerVotes[pResult->GetClientID()].m_Page = pResult->GetInteger(0);
	pSelf->m_aPlayerVotes[pResult->GetClientID()].m_Confirm = false;
	pSelf->ClearVotes(pResult->GetClientID());
	return true;
}

bool CGameContext::VotCraft(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->m_aPlayerVotes[pResult->GetClientID()].m_Page = PAGE_CRAFT_SELECTED;
	pSelf->m_aPlayerVotes[pResult->GetClientID()].m_Select[SPlayerVote::ITEM] = pResult->GetInteger(0);
	pSelf->ClearVotes(pResult->GetClientID());
	return true;
}

bool CGameContext::VotMake(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

	int ClientID = pResult->GetClientID();
	CPlayer *pPlayer = pSelf->GetPlayer(ClientID);
	if (!pPlayer)
		return false;

	int Item = pSelf->m_aPlayerVotes[ClientID].m_Select[SPlayerVote::ITEM];

	if (pSelf->ItemHelper()->GetMax(Item) && pPlayer->m_AccData.m_aItems[Item].m_Num >= pSelf->ItemHelper()->GetMax(Item))
	{
		pSelf->SetVoteExtraText(ClientID, "You have reached the limit");
		if (pPlayer->GetCharacter())
			pSelf->CreateSoundGlobal(SOUND_WEAPON_NOAMMO, ClientID);
		return true;
	}
	// Check formula
	bool IsOK = true;
	for (int Checked = 0; Checked < 2; Checked++)
	{
		for (int i = 0; i < NUM_ITEM; i++)
		{
			if (Checked && IsOK)
			{
				if (pSelf->Items(Item)->m_Formula[i])
					pPlayer->m_AccData.m_aItems[i].m_Num -= pSelf->Items(Item)->m_Formula[i];
			}
			else
			{
				if (pSelf->Items(Item)->m_Formula[i] > pPlayer->m_AccData.m_aItems[i].m_Num)
				{
					IsOK = false;
					pSelf->SetVoteExtraText(ClientID, "You don't have enough materials to make it.");
					break;
				}
			}
		}
	}

	if (IsOK)
	{
		pPlayer->m_AccData.m_aItems[Item].m_Num++;
		pSelf->SetVoteExtraText(ClientID, "You have successfully make a {}!", pSelf->Items(Item)->m_aItemName);
		pSelf->TW()->Account()->SaveAccountData(ClientID, TABLE_ITEM, pPlayer->m_AccData);
	}

	pSelf->ClearVotes(pResult->GetClientID());
	return true;
}

bool CGameContext::VotPlaceCard(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int ClientID = pResult->GetClientID();
	CPlayer *pPlayer = pSelf->GetPlayer(ClientID);
	if (!pPlayer)
		return false;

	/*if (!pSelf->m_aPlayerVotes[ClientID].m_Confirm)
	{
		pSelf->SetVoteExtraText(ClientID, "Are you sure?(This will not be reversible)");
		pSelf->m_aPlayerVotes[ClientID].m_Confirm = true;
		pSelf->ClearVotes(pResult->GetClientID());
		return true;
	}*/

	int Select = pSelf->m_aPlayerVotes[pResult->GetClientID()].m_Select[SPlayerVote::EVoteSelect::ITEM];
	int Card = pResult->GetInteger(0);
	pSelf->SetVoteExtraText(ClientID, "You placed {} on {}!", pSelf->ItemHelper()->GetItemName(Card), pSelf->ItemHelper()->GetItemName(Select));

	int Capacity = pSelf->ItemHelper()->GetCapacity(Card);
	int ExistCard = -1;

	nlohmann::json Json = nlohmann::json::parse(pPlayer->m_AccData.m_aItems[Select].m_aExtra);
	if (!Json["Extra"].contains("Cards") || Json["Extra"]["Cards"].empty())
	{
		if (Capacity <= pSelf->ItemHelper()->GetCapacity(Select))
		{
			Json["Extra"]["Cards"].push_back({{"id", Card}, {"num", 1}});
			pPlayer->m_AccData.m_aItems[Select].m_aExtra = Json.dump();
			pPlayer->m_AccData.m_aItems[Select].m_Capacity = Capacity;
			//pSelf->m_aPlayerVotes[ClientID].m_Confirm = false;

			pPlayer->m_AccData.m_aItems[Card].m_Num--;

			pSelf->TW()->Account()->SaveAccountData(ClientID, TABLE_ITEM, pPlayer->m_AccData);
			pSelf->ClearVotes(pResult->GetClientID());
			return true;
		}
		else
			pSelf->SetVoteExtraText(ClientID, "Not enough capacity!");
		return false;
	}
	else
	{
		int CurrentIndex = -1;
		for (const auto &j : Json["Extra"]["Cards"])
		{
			CurrentIndex++;
			Capacity += pSelf->ItemHelper()->GetCapacity(int(j["id"])) * int(j["num"]);
			if (j["id"] == Card)
				ExistCard = CurrentIndex;
		}
	}

	if (Capacity <= pSelf->ItemHelper()->GetCapacity(Select))
	{
		if (ExistCard != -1)
		{
			if(Json["Extra"]["Cards"][ExistCard]["num"] >= pSelf->ItemHelper()->GetMaxPlace(ExistCard))
			{
				pSelf->SetVoteExtraText(ClientID, "You have reached the limit");
				pSelf->ClearVotes(pResult->GetClientID());
				return true;
			}
			Json["Extra"]["Cards"][ExistCard]["num"] = int(Json["Extra"]["Cards"][ExistCard]["num"]) + 1;
		}
		else
			Json["Extra"]["Cards"].push_back({{"id", Card}, {"num", 1}});
		pPlayer->m_AccData.m_aItems[Select].m_aExtra = Json.dump();
		pPlayer->m_AccData.m_aItems[Select].m_Capacity = Capacity;
		pPlayer->m_AccData.m_aItems[Card].m_Num--;
	}
	else
		pSelf->SetVoteExtraText(ClientID, "Not enough capacity!");

	//pSelf->m_aPlayerVotes[ClientID].m_Confirm = false;

	pSelf->TW()->Account()->SaveAccountData(ClientID, TABLE_ITEM, pPlayer->m_AccData);
	pSelf->ClearVotes(pResult->GetClientID());
	return true;
}

bool CGameContext::VotCheckItem(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->m_aPlayerVotes[pResult->GetClientID()].m_Page = PAGE_CHECK_ITEM;
	pSelf->m_aPlayerVotes[pResult->GetClientID()].m_Select[SPlayerVote::ITEM] = pResult->GetInteger(0);
	pSelf->ClearVotes(pResult->GetClientID());
	return true;
}

bool CGameContext::VotEquip(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int CID = pResult->GetClientID();
	pSelf->GetPlayer(CID)->m_AccData.m_Holding[pSelf->ItemHelper()->GetType(pResult->GetInteger(0))] = pResult->GetInteger(0);
	pSelf->CreateSoundGlobal(SOUND_PICKUP_NINJA, CID);
	pSelf->SetVoteExtraText(CID, "You have successfully equipped the {}", pSelf->ItemHelper()->GetItemName(pResult->GetInteger(0)));
	pSelf->ClearVotes(CID);
	pSelf->TW()->Account()->SaveAccountData(CID, TABLE_ACCOUNT, pSelf->GetPlayer(CID)->m_AccData);
	return true;
}

bool CGameContext::VotSeparateCard(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int CID = pResult->GetClientID();
	int Card = pResult->GetInteger(0);
	int Select = pSelf->m_aPlayerVotes[CID].m_Select[SPlayerVote::EVoteSelect::ITEM];

	nlohmann::json Json = nlohmann::json::parse(pSelf->GetPlayer(CID)->GetExtra(Select));
	if (Json["Extra"].contains("Cards") && !Json["Extra"]["Cards"].empty())
	{
		int ToBeRemove = 0;
		for (const auto &j : Json["Extra"]["Cards"])
		{
			if (Card == int(j["id"]))
			{
				pSelf->SetVoteExtraText(CID, "You separate {} from {}!", pSelf->ItemHelper()->GetItemName(int(j["id"])), pSelf->ItemHelper()->GetItemName(Select));
				if(int(j["num"]) > 1)
					Json["Extra"]["Cards"][ToBeRemove]["num"] = int(Json["Extra"]["Cards"][ToBeRemove]["num"]) - 1;
				else
					Json["Extra"]["Cards"].erase(ToBeRemove);
				pSelf->GetPlayer(CID)->SetExtra(Select, Json.dump());
				pSelf->GetPlayer(CID)->m_AccData.m_aItems[Card].m_Num++;
				break;
			}
			ToBeRemove++;
		}
	}
	else
	{
		pSelf->Server()->Kick(CID, "服务器出现错误！请联系开发者QQ:1562151175！感谢！");
		return true;
	}
	
	pSelf->CreateSoundGlobal(SOUND_CTF_RETURN, CID);
	pSelf->ClearVotes(CID);
	pSelf->TW()->Account()->SaveAccountData(CID, TABLE_ACCOUNT, pSelf->GetPlayer(CID)->m_AccData);
	return true;
}

void CGameContext::OnConsoleInit()
{
	m_pServer = Kernel()->RequestInterface<IServer>();
	m_pConsole = Kernel()->RequestInterface<IConsole>();
	m_pStorage = Kernel()->RequestInterface<IStorage>();

	m_ConsoleOutputHandle_ChatPrint = Console()->RegisterPrintCallback(0, ChatConsolePrintCallback, this);

	Console()->Register("tune", "si", CFGFLAG_SERVER, ConTuneParam, this, "Tune variable to value");
	Console()->Register("tune_reset", "", CFGFLAG_SERVER, ConTuneReset, this, "Reset tuning");
	Console()->Register("tune_dump", "", CFGFLAG_SERVER, ConTuneDump, this, "Dump tuning");

	Console()->Register("pause", "", CFGFLAG_SERVER, ConPause, this, "Pause/unpause game");
	Console()->Register("restart", "?i", CFGFLAG_SERVER | CFGFLAG_STORE, ConRestart, this, "Restart in x seconds (0 = abort)");
	Console()->Register("broadcast", "r", CFGFLAG_SERVER, ConBroadcast, this, "Broadcast message");
	Console()->Register("say", "r", CFGFLAG_SERVER, ConSay, this, "Say in chat");
	Console()->Register("set_team", "ii?i", CFGFLAG_SERVER, ConSetTeam, this, "Set team of player to team");
	Console()->Register("set_team_all", "i", CFGFLAG_SERVER, ConSetTeamAll, this, "Set team of all players to team");
	Console()->Register("swap_teams", "", CFGFLAG_SERVER, ConSwapTeams, this, "Swap the current teams");
	Console()->Register("shuffle_teams", "", CFGFLAG_SERVER, ConShuffleTeams, this, "Shuffle the current teams");
	Console()->Register("lock_teams", "", CFGFLAG_SERVER, ConLockTeams, this, "Lock/unlock teams");

	Console()->Register("add_vote", "sr", CFGFLAG_SERVER, ConAddVote, this, "Add a voting option");
	Console()->Register("remove_vote", "s", CFGFLAG_SERVER, ConRemoveVote, this, "remove a voting option");
	Console()->Register("force_vote", "ss?r", CFGFLAG_SERVER, ConForceVote, this, "Force a voting option");
	Console()->Register("clear_votes", "", CFGFLAG_SERVER, ConClearVotes, this, "Clears the voting options");
	Console()->Register("vote", "r", CFGFLAG_SERVER, ConVote, this, "Force a vote to yes/no");

	Console()->Register("about", "", CFGFLAG_CHAT, ConAbout, this, "Show information about the mod");
	Console()->Register("language", "?s", CFGFLAG_CHAT, ConLanguage, this, "[language code] - Select your language");
	Console()->Register("askai", "s", CFGFLAG_CHAT, ConChatAI, this, "[ask] - Ask Chat AI");

	Console()->Register("register", "ss", CFGFLAG_CHAT, ConRegister, this, "[username] [password] - Register account");
	Console()->Register("login", "ss", CFGFLAG_CHAT, ConLogin, this, "[username] [password] - Login your account");

	Console()->Register("giveitem", "iii", CFGFLAG_CHAT, VotGiveItem, this, "[clientid] [itemid] [numitem] - Give item");

	Console()->Register("selectitem", "ii", CFGFLAG_VOTE, VotSelectItem, this, "[][] - Select");
	Console()->Register("goto", "i", CFGFLAG_VOTE, VotGoto, this, "[page] - Go to a vote page");
	Console()->Register("craft", "i", CFGFLAG_VOTE, VotCraft, this, "[item] - Craft something");
	Console()->Register("make", "", CFGFLAG_VOTE, VotMake, this, "make - Confirm to make something");
	Console()->Register("checkitem", "i", CFGFLAG_VOTE, VotCheckItem, this, "[item] - Confirm to make something");
	Console()->Register("placecard", "i", CFGFLAG_VOTE, VotPlaceCard, this, "[card] - Place card");
	Console()->Register("equip", "i", CFGFLAG_VOTE, VotEquip, this, "[item] - Equip");
	Console()->Register("separatecard", "i", CFGFLAG_VOTE, VotSeparateCard, this, "[card] - Separate Card");

	Console()->Chain("sv_motd", ConchainSpecialMotdupdate, this);

	m_pChatAI = new CChatAI(Kernel()->RequestInterface<IEngine>());
}

void CGameContext::OnInit(int WorldID)
{
	m_pServer = Kernel()->RequestInterface<IServer>();
	m_pConsole = Kernel()->RequestInterface<IConsole>();
	m_pStorage = Kernel()->RequestInterface<IStorage>();
	m_World.SetGameServer(this);
	m_Events.SetGameServer(this);
	m_WorldID = WorldID;
	m_RespawnWorldID = WorldID;

	for (int i = 0; i < NUM_NETOBJTYPES; i++)
		Server()->SnapSetStaticsize(i, m_NetObjHandler.GetObjSize(i));

	m_Collision.Init(Kernel(), WorldID);

	m_pTWorldController = new TWorldController(this);

	// select gametype
	m_pController = new CGameControllerTeeDefense(this); // force TeeDefense

	// create all entities from the game layer
	// initialize cores
	CMapItemLayerTilemap *pTileMap = m_Collision.GetLayers()->GameLayer();
	CTile *pTiles = (CTile *)Kernel()->RequestInterface<IMap>(WorldID)->GetData(pTileMap->m_Data);
	for (int y = 0; y < pTileMap->m_Height; y++)
	{
		for (int x = 0; x < pTileMap->m_Width; x++)
		{
			const int Index = pTiles[y * pTileMap->m_Width + x].m_Index;
			if (Index >= ENTITY_OFFSET)
			{
				const vec2 Pos(x * 32.0f + 16.0f, y * 32.0f + 16.0f);
				m_pController->OnEntity(Index - ENTITY_OFFSET, Pos);
			}
		}
	}

	m_pBotEngine->Init(pTiles, pTileMap->m_Width, pTileMap->m_Height);
	m_pController->InitBots();

	ItemHelper()->LoadIndex();
}

void CGameContext::OnShutdown()
{
	delete this;
}

void CGameContext::OnSnap(int ClientID)
{
	// check valid player
	CPlayer *pPlayer = m_apPlayers[ClientID];
	if (pPlayer && pPlayer->GetPlayerWorldID() != GetWorldID())
		return;

	// add tuning to demo
	CTuningParams StandardTuning;
	if (ClientID == -1 && Server()->DemoRecorder_IsRecording() && mem_comp(&StandardTuning, &m_Tuning, sizeof(CTuningParams)) != 0)
	{
		CMsgPacker Msg(NETMSGTYPE_SV_TUNEPARAMS);
		int *pParams = (int *)&m_Tuning;
		for (unsigned i = 0; i < sizeof(m_Tuning) / sizeof(int); i++)
			Msg.AddInt(pParams[i]);
		Server()->SendMsg(&Msg, MSGFLAG_RECORD | MSGFLAG_NOSEND, ClientID, GetWorldID());
	}

	m_pController->Snap(ClientID);
	for (const auto &pIterPlayer : m_apPlayers)
	{
		if (pIterPlayer)
			pIterPlayer->Snap(ClientID);
	}
	m_World.Snap(ClientID);
	m_Events.Snap(ClientID);

	//Snap laser dots
	for(int i=0; i < m_LaserDots.size(); i++)
	{
		if(ClientID >= 0)
		{
			vec2 CheckPos = (m_LaserDots[i].m_Pos0 + m_LaserDots[i].m_Pos1)*0.5f;
			float dx = m_apPlayers[ClientID]->m_ViewPos.x-CheckPos.x;
			float dy = m_apPlayers[ClientID]->m_ViewPos.y-CheckPos.y;
			if(absolute(dx) > 1000.0f || absolute(dy) > 800.0f)
				continue;
			if(distance(m_apPlayers[ClientID]->m_ViewPos, CheckPos) > 1100.0f)
				continue;
		}
		
		CNetObj_Laser *pObj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_LaserDots[i].m_SnapID, sizeof(CNetObj_Laser)));
		if(pObj)
		{
			pObj->m_X = (int)m_LaserDots[i].m_Pos1.x;
			pObj->m_Y = (int)m_LaserDots[i].m_Pos1.y;
			pObj->m_FromX = (int)m_LaserDots[i].m_Pos0.x;
			pObj->m_FromY = (int)m_LaserDots[i].m_Pos0.y;
			pObj->m_StartTick = Server()->Tick();
		}
	}
	for(int i=0; i < m_HammerDots.size(); i++)
	{
		if(ClientID >= 0)
		{
			vec2 CheckPos = m_HammerDots[i].m_Pos;
			float dx = m_apPlayers[ClientID]->m_ViewPos.x-CheckPos.x;
			float dy = m_apPlayers[ClientID]->m_ViewPos.y-CheckPos.y;
			if(absolute(dx) > 1000.0f || absolute(dy) > 800.0f)
				continue;
			if(distance(m_apPlayers[ClientID]->m_ViewPos, CheckPos) > 1100.0f)
				continue;
		}
		
		CNetObj_Projectile *pObj = static_cast<CNetObj_Projectile *>(Server()->SnapNewItem(NETOBJTYPE_PROJECTILE, m_HammerDots[i].m_SnapID, sizeof(CNetObj_Projectile)));
		if(pObj)
		{
			pObj->m_X = (int)m_HammerDots[i].m_Pos.x;
			pObj->m_Y = (int)m_HammerDots[i].m_Pos.y;
			pObj->m_VelX = 0;
			pObj->m_VelY = 0;
			pObj->m_StartTick = Server()->Tick();
			pObj->m_Type = WEAPON_HAMMER;
		}
	}
	for(int i=0; i < m_LoveDots.size(); i++)
	{
		if(ClientID >= 0)
		{
			vec2 CheckPos = m_LoveDots[i].m_Pos;
			float dx = m_apPlayers[ClientID]->m_ViewPos.x-CheckPos.x;
			float dy = m_apPlayers[ClientID]->m_ViewPos.y-CheckPos.y;
			if(absolute(dx) > 1000.0f || absolute(dy) > 800.0f)
				continue;
			if(distance(m_apPlayers[ClientID]->m_ViewPos, CheckPos) > 1100.0f)
				continue;
		}
		
		CNetObj_Pickup *pObj = static_cast<CNetObj_Pickup *>(Server()->SnapNewItem(NETOBJTYPE_PICKUP, m_LoveDots[i].m_SnapID, sizeof(CNetObj_Pickup)));
		if(pObj)
		{
			pObj->m_X = (int)m_LoveDots[i].m_Pos.x;
			pObj->m_Y = (int)m_LoveDots[i].m_Pos.y;
			pObj->m_Type = POWERUP_HEALTH;
			pObj->m_Subtype = 0;
		}
	}
}
void CGameContext::OnPreSnap() {}
void CGameContext::OnPostSnap()
{
	m_Events.Clear();
}

bool CGameContext::IsClientReady(int ClientID)
{
	return m_apPlayers[ClientID] && m_apPlayers[ClientID]->m_IsReady ? true : false;
}

bool CGameContext::IsClientPlayer(int ClientID)
{
	return m_apPlayers[ClientID] && m_apPlayers[ClientID]->GetTeam() == TEAM_SPECTATORS ? false : true;
}

int CGameContext::GetClientVersion(int ClientId) const
{
	return Server()->GetClientVersion(ClientId);
}

CPlayer *CGameContext::GetPlayer(int ClientID)
{
	if (m_apPlayers[ClientID])
		return m_apPlayers[ClientID];
	return nullptr;
}

bool CGameContext::IsPlayerEqualWorld(int ClientID, int WorldID) const
{
	if (ClientID < 0 || ClientID >= MAX_CLIENTS || !m_apPlayers[ClientID])
		return false;

	if (WorldID <= -1)
		return m_apPlayers[ClientID]->GetPlayerWorldID() == m_WorldID;
	return m_apPlayers[ClientID]->GetPlayerWorldID() == WorldID;
}

bool CGameContext::IsPlayersNearby(vec2 Pos, float Distance) const
{
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		if (m_apPlayers[i] && IsPlayerEqualWorld(i) && distance(Pos, m_apPlayers[i]->m_ViewPos) <= Distance)
			return true;
	}
	return false;
}

// change the world
void CGameContext::OnClientPrepareChangeWorld(int ClientID)
{
	if (m_apPlayers[ClientID])
	{
		m_apPlayers[ClientID]->KillCharacter(WEAPON_WORLD);
		delete m_apPlayers[ClientID];
		m_apPlayers[ClientID] = nullptr;
	}
	const int AllocMemoryCell = ClientID + m_WorldID * MAX_CLIENTS;
	m_apPlayers[ClientID] = new (AllocMemoryCell) CPlayer(this, ClientID, TEAM_HUMAN);
}

// clearing all data at the exit of the client necessarily call once enough
void CGameContext::ClearClientData(int ClientID)
{
	return; //
}

const char *CGameContext::GetClientLanguage(int ClientID)
{
	return Server()->GetClientLanguage(ClientID);
}

void CGameContext::AddBot()
{
	int BotClientID = MAX_PLAYERS;
	while (m_apPlayers[BotClientID])
	{
		BotClientID++;
		if (BotClientID >= MAX_CLIENTS)
			return;
	}

	Server()->InitClientBot(BotClientID);
	const int AllocMemoryCell = BotClientID + m_WorldID * MAX_CLIENTS;
	m_apPlayers[BotClientID] = new (AllocMemoryCell) CPlayer(this, BotClientID, TEAM_BOT);
	m_apPlayers[BotClientID]->m_BotWorldID = GetWorldID();
	m_apPlayers[BotClientID]->m_IsBot = true;
	m_apPlayers[BotClientID]->m_pBot = new CBot(m_pBotEngine, m_apPlayers[BotClientID]);

	return;
}

bool CGameContext::IsPlayerInWorld(int ClientID, int WorldID) const
{
	if (ClientID < 0 || ClientID >= MAX_CLIENTS || !m_apPlayers[ClientID])
		return false;

	int PlayerWorldID = m_apPlayers[ClientID]->GetPlayerWorldID();
	return PlayerWorldID == (WorldID == -1 ? m_WorldID : WorldID);
}

bool CGameContext::ArePlayersNearby(vec2 Pos, float Distance) const
{
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		CPlayer *pPlayer = m_apPlayers[i];
		if (pPlayer && IsPlayerInWorld(i) && distance(Pos, pPlayer->m_ViewPos) <= Distance)
			return true;
	}

	return false;
}

int CGameContext::GetBotWorldID(int ClientID)
{
	if (GetPlayer(ClientID))
		return GetPlayer(ClientID)->m_BotWorldID;
	return MAIN_WORLD_ID;
}

const char *CGameContext::GameType() { return m_pController && m_pController->GameType() ? m_pController->GameType() : ""; }
const char *CGameContext::Version() { return GAME_VERSION; }
const char *CGameContext::NetVersion() { return GAME_NETVERSION; }

IGameServer *CreateGameServer() { return new CGameContext; }

int CGameContext::CountBots()
{
	int Count = 0;
	for (const auto &Player : m_apPlayers)
	{
		if (!Player)
			continue;

		Count += Player->IsBot();
	}

	return Count;
}

// MMOTee
void CGameContext::AddVote(const char *Desc, const char *Cmd, int ClientID)
{
	while (*Desc && *Desc == ' ')
		Desc++;

	if (ClientID == -2)
		return;

	SPlayerVote::SVoteOptions Vote;
	str_copy(Vote.m_aDescription, Desc, sizeof(Vote.m_aDescription));
	str_copy(Vote.m_aCommand, Cmd, sizeof(Vote.m_aCommand));
	m_aPlayerVotes[ClientID].m_aVoteOptions.add(Vote);

	// inform clients about added option
	CNetMsg_Sv_VoteOptionAdd OptionMsg;
	OptionMsg.m_pDescription = Vote.m_aDescription;
	Server()->SendPackMsg(&OptionMsg, MSGFLAG_VITAL, ClientID, -1);
}

void CGameContext::AddVote_ListInventory(int ItemType, const char *pCmd, bool Equip)
{
	CPlayer *pP = GetPlayer(m_VoteClientID);
	if (!pP)
		return;

	bool Got = false;
	for (int i = 0; i < NUM_ITEM; i++)
	{
		if (ItemHelper()->GetType(i) == ItemType && pP->m_AccData.m_aItems[i].m_Num)
		{
			char aCmd[32];
			str_format(aCmd, sizeof(aCmd), "%s %d", pCmd, i);
			if (Equip && pP->m_AccData.m_Holding[ItemType] == i)
				AddVote_VL(aCmd, "➳ {} x{} ✓", ItemHelper()->GetItemName(i), pP->m_AccData.m_aItems[i].m_Num);
			else
				AddVote_VL(aCmd, "➳ {} x{}", ItemHelper()->GetItemName(i), pP->m_AccData.m_aItems[i].m_Num);
			Got = true;
		}
	}
	if (!Got)
		AddVote_Text("( ´・∧・`)Empty");
}

void CGameContext::AddVote_ListCraft(int ItemType)
{
	CPlayer *pP = GetPlayer(m_VoteClientID);
	if (!pP)
		return;

	bool Got = false;
	for (int i = 0; i < NUM_ITEM; i++)
	{
		if (ItemHelper()->GetType(i) == ItemType && Items(i)->m_HasFormula)
		{
			AddVote_Craft(i);
			Got = true;
		}
	}
	if (!Got)
		AddVote_Text("( ´・∧・`)Empty");
}

void CGameContext::AddVote_Craft(int ItemID)
{
	CPlayer *pP = GetPlayer(m_VoteClientID);
	if (!pP)
		return;

	char aCmd[64];
	str_format(aCmd, sizeof(aCmd), "ccv_craft %d", ItemID);
	AddVote_VL(aCmd, "➳ {} - {}", ItemHelper()->GetItemName(ItemID), Items(ItemID)->m_aItemDesc);
}

void CGameContext::AddVote_ListFormula(int Item)
{
	CPlayer *pP = GetPlayer(m_VoteClientID);
	if (!pP)
		return;

	bool Got = false;
	for (int i = 0; i < NUM_ITEM; i++)
	{
		if (Items(Item)->m_Formula[i])
		{
			AddVote_Text("# {} {}/{}", Items(i)->m_aItemName, pP->m_AccData.m_aItems[i].m_Num, Items(Item)->m_Formula[i]);
			Got = true;
		}
	}
	if (!Got)
		AddVote_Text("( ´・∧・`)Empty");
}

void CGameContext::AddVote_Back()
{
	if (!PlayerExists(m_VoteClientID))
		return;

	AddVote_Goto(m_aPlayerVotes[m_VoteClientID].m_LastPage, "⏎ Back");
}

void CGameContext::AddVote_Space(int Num)
{
	if (!PlayerExists(m_VoteClientID))
		return;

	for (int i = 0; i < Num; i++)
		AddVote_VL("ccv_null", " ");
}

void CGameContext::InitVotes(int ClientID)
{
	CPlayer *pP = GetPlayer(ClientID);
	if (!pP)
		return;

	SetVoteClientID(ClientID);

	CPlayer::SAccData Data = pP->m_AccData;
	SPlayerVote PlayerVote = m_aPlayerVotes[ClientID];
	int Page = PlayerVote.m_Page;
	std::string ItemLists[NUM_ITYPE] = {"Pickaxe", "Axe", "Sword", "Turret", "Material", "Card"};

	AddVote_Text("# Global-Notice: {}", PlayerVote.m_aExtraText);
	AddVote_Text("===");
	switch (Page)
	{
	case PAGE_MENU:
	{
		SetVoteLastPage(Page);
		AddVote_Text("☪ Player Menu");
		AddVote_Text("User ID: {}", Data.m_UserID);
		AddVote_Text("Party: #Under development#");
		AddVote_Space();
		AddVote_Goto(PAGE_INVENTORY, "☞ Inventory ✪");
		AddVote_Goto(PAGE_CRAFT, "☞ Craft ☺");
		AddVote_Goto(PAGE_EQUIPMENT, "☞ Equipment ☭");
	}
	break;

	case PAGE_INVENTORY:
	{
		TW()->Account()->SyncAccountData(ClientID, TABLE_ITEM);
		SetVoteLastPage(PAGE_MENU);
		AddVote_Text("☪ Inventory");
		AddVote_Space();
		CountItemNum(ClientID);
		for (int i = 0; i < NUM_ITYPE; i++)
		{
			if (PlayerVote.m_Select[SPlayerVote::ITEMLIST] != i)
			{
				char aCmd[64];
				str_format(aCmd, sizeof(aCmd), "ccv_selectitem %d %d", SPlayerVote::ITEMLIST, i);
				AddVote_VL(aCmd, "▹ {} ({})", ItemLists[i], pP->m_AccData.m_ItemCount[i]);
			}
			else
				AddVote_Text("▾ {} ({}) ", ItemLists[i], pP->m_AccData.m_ItemCount[i]);
		}
		AddVote_Space();
		AddVote_Back();
		AddVote_Text("---------------------");
		AddVote_ListInventory(PlayerVote.m_Select[SPlayerVote::EVoteSelect::ITEMLIST], "ccv_checkitem");
	}
	break;

	case PAGE_CHECK_ITEM:
	{
		int SelectItem = PlayerVote.m_Select[SPlayerVote::ITEM];
		char aCmd[64];
		bool HaveCards = false;
		int Capacity = 0;
		nlohmann::json Json = nlohmann::json::parse(pP->m_AccData.m_aItems[SelectItem].m_aExtra);
		if (Json["Extra"].contains("Cards") && !Json["Extra"]["Cards"].empty())
		{
			HaveCards = true;
			for (const auto &j : Json["Extra"]["Cards"])
				Capacity += ItemHelper()->GetCapacity(int(j["id"])) * int(j["num"]);
			pP->m_AccData.m_aItems[SelectItem].m_Capacity = Capacity;
		}

		SetVoteLastPage(PAGE_INVENTORY);
		AddVote_Text("☪ Item Info");
		AddVote_Space();
		AddVote_Text("Item: {}", ItemHelper()->GetItemName(SelectItem));
		AddVote_Text("Description: {}", Items(SelectItem)->m_aItemDesc);
		if (ItemHelper()->GetType(SelectItem) == ITYPE_CARD)
			AddVote_Text("Need Capacity: {}", ItemHelper()->GetCapacity(SelectItem));
		else if (ItemHelper()->GetType(SelectItem) != ITYPE_MATERIAL)
			AddVote_Text("Capacity: {}/{}", Capacity, ItemHelper()->GetCapacity(SelectItem));
		AddVote_Text("You have: {}", Data.m_aItems[SelectItem].m_Num);
		AddVote_Space();

		str_format(aCmd, sizeof(aCmd), "ccv_equip %d", SelectItem);
		AddVote_VL(aCmd, "☝ Equip");
		AddVote_Space();
		bool Once = false;
		bool IsEmpty = true;
		AddVote_Text("# Cards #");
		if (HaveCards)
		{
			AddVote_Text("- Current -");
			for (const auto &j : Json["Extra"]["Cards"])
			{
				str_format(aCmd, sizeof(aCmd), "ccv_separatecard %d", int(j["id"]));
				AddVote_VL(aCmd, "★Separate one {}x{}({}*{})", ItemHelper()->GetItemName(int(j["id"])), int(j["num"]), ItemHelper()->GetCapacity(int(j["id"])), int(j["num"]));
			}
			IsEmpty = false;
		}
		AddVote_Space();
		Once = false;
		for (int i = 0; i < NUM_ITEM; i++)
		{
			if (Data.m_aItems[i].m_Num <= 0)
				continue;

			if (ItemHelper()->GetType(i) == ITYPE_CARD)
			{
				CItem_Card *pCard = (CItem_Card *)Items(i);
				if (!(pCard->m_Placeable[ItemHelper()->GetType(SelectItem)]))
					continue;

				if (!Once)
					AddVote_Text("- Placement -");

				Once = true;
				IsEmpty = false;

				str_format(aCmd, sizeof(aCmd), "ccv_placecard %d", i);
				AddVote_VL(aCmd, "☝ Place {}(x{},{})", ItemHelper()->GetItemName(i), pP->GetItemNum(i), ItemHelper()->GetCapacity(i));
				continue;
			}
		}
		if (IsEmpty)
			AddVote_Text("( ´・∧・`)Empty");
		AddVote_Space();
		AddVote_Back();
	}
	break;

	case PAGE_CRAFT:
	{
		TW()->Account()->SyncAccountData(ClientID, TABLE_ITEM);
		SetVoteLastPage(PAGE_MENU);
		AddVote_Text("☪ Craft");
		AddVote_Space();
		for (int i = 0; i < NUM_ITYPE; i++)
		{
			if (PlayerVote.m_Select[SPlayerVote::ITEMLIST] != i)
			{
				char aCmd[64];
				str_format(aCmd, sizeof(aCmd), "ccv_selectitem %d %d", SPlayerVote::ITEMLIST, i);
				AddVote_VL(aCmd, "▹ {}", ItemLists[i]);
			}
			else
				AddVote_Text("▾ {}", ItemLists[i]);
		}
		AddVote_Space();
		AddVote_Back();
		AddVote_Text("---------------------");
		AddVote_ListCraft(PlayerVote.m_Select[SPlayerVote::ITEMLIST]);
	}
	break;

	case PAGE_CRAFT_SELECTED:
	{
		SetVoteLastPage(PAGE_CRAFT);
		AddVote_Text("☪ Craft");
		AddVote_Space();
		AddVote_Text("Item: {}", Items(PlayerVote.m_Select[SPlayerVote::ITEM])->m_aItemName);
		AddVote_Text("Description: {}", Items(PlayerVote.m_Select[SPlayerVote::ITEM])->m_aItemDesc);
		AddVote_Text("You have: {}", Data.m_aItems[PlayerVote.m_Select[SPlayerVote::ITEM]].m_Num);
		AddVote_Text("㊮ Formula:");
		AddVote_Text("---");
		AddVote_ListFormula(PlayerVote.m_Select[SPlayerVote::ITEM]);
		AddVote_Text("===");
		AddVote_VL("ccv_make", "- Craft!");
		AddVote_Space(2);
		AddVote_Back();
	}
	break;

	case PAGE_EQUIPMENT:
	{
		SetVoteLastPage(PAGE_MENU);
		TW()->Account()->SyncAccountData(ClientID, TABLE_ITEM);
		SetVoteLastPage(PAGE_MENU);
		AddVote_Text("☪ Equipment");
		AddVote_Text("Sword: {}", ItemHelper()->GetItemName(pP->m_AccData.m_Holding[ITYPE_SWORD]));
		AddVote_Text("Axe: {}", ItemHelper()->GetItemName(pP->m_AccData.m_Holding[ITYPE_AXE]));
		AddVote_Text("Pickaxe: {}", ItemHelper()->GetItemName(pP->m_AccData.m_Holding[ITYPE_PICKAXE]));
		AddVote_Space();
		CountItemNum(ClientID);
		for (int i = 0; i < NUM_ITYPE; i++)
		{
			if (i != ITYPE_PICKAXE && i != ITYPE_AXE && i != ITYPE_SWORD)
				continue;

			if (PlayerVote.m_Select[SPlayerVote::EQUIPMENT] != i)
			{
				char aCmd[64];
				str_format(aCmd, sizeof(aCmd), "ccv_selectitem %d %d", SPlayerVote::EQUIPMENT, i);
				AddVote_VL(aCmd, "▹ {} ({})", ItemLists[i], pP->m_AccData.m_ItemCount[i]);
			}
			else
				AddVote_Text("▾ {} ({}) ", ItemLists[i], pP->m_AccData.m_ItemCount[i]);
		}
		AddVote_Space();
		AddVote_Back();
		AddVote_Text("---------------------");
		AddVote_ListInventory(PlayerVote.m_Select[SPlayerVote::EVoteSelect::EQUIPMENT], "ccv_equip", true);
	}
	break;
	default:
		break;
	}

	SetVoteClientID(-1);
}

void CGameContext::ClearVotes(int ClientID)
{
	m_aPlayerVotes[ClientID].m_aVoteOptions.clear();

	// send vote options
	CNetMsg_Sv_VoteClearOptions ClearMsg;
	Server()->SendPackMsg(&ClearMsg, MSGFLAG_VITAL, ClientID, -1);

	InitVotes(ClientID);
}

bool CGameContext::AwakenBot(int ClientID)
{
	if (ClientID >= MAX_CLIENTS || ClientID < MAX_PLAYERS || !m_apPlayers[ClientID])
		return false;

	m_apPlayers[ClientID]->m_CanSnap = true;
	m_apPlayers[ClientID]->m_WantSpawn = true;
	m_apPlayers[ClientID]->Respawn();
	return true;
}

bool CGameContext::AsleepBot(int ClientID)
{
	if (ClientID >= MAX_CLIENTS || ClientID < MAX_PLAYERS || !m_apPlayers[ClientID])
		return false;

	m_apPlayers[ClientID]->m_CanSnap = false;
	m_apPlayers[ClientID]->m_WantSpawn = false;
	return true;
}

void CGameContext::CountItemNum(int ClientID)
{
	if (!PlayerExists(ClientID))
		return;

	int ItemCount[NUM_ITYPE] = {0, 0, 0, 0, 0, 0};

	for (int i = 0; i < NUM_ITEM; i++)
	{
		if (m_apPlayers[ClientID]->m_AccData.m_aItems[i].m_Num > 0)
			ItemCount[ItemHelper()->GetType(i)]++;
	}

	std::copy(std::begin(ItemCount), std::end(ItemCount), m_apPlayers[ClientID]->m_AccData.m_ItemCount);
}