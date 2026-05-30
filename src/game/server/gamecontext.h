/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_GAMECONTEXT_H
#define GAME_SERVER_GAMECONTEXT_H

#include <engine/server.h>
#include <engine/console.h>
#include <engine/shared/memheap.h>

#include <teeother/components/localization.h>

#include <game/layers.h>
#include <game/voting.h>

#include <vector>

#include "eventhandler.h"
#include "gamecontroller.h"
#include "gameworld.h"
#include "player.h"

class CChatAI;

#include "votepage.h"
#include "buildmenu.h"
#include "classmenu.h"

/*
	Tick
		Game Context (CGameContext::OnTick)
			Game World (CGameWorld::Tick)
				Reset world if requested (CGameWorld::Reset)
				All entities in the world (CEntity::Tick)
				All entities in the world (CEntity::TickDeferd)
				Remove entities marked for deletion (CGameWorld::RemoveEntity)
			Game Controller (CGameControllerWorkbenches::Tick)
			All players (CPlayer::Tick)


	Snap
		Game Context (CGameContext::OnSnap)
			Game World (CGameWorld::Snap)
				All entities in the world (CEntity::Snap)
			Game Controller (CGameControllerWorkbenches::Snap)
			Events handler (CEventHandler::Snap)
			All players (CPlayer::Snap)

*/
class CGameContext : public IGameServer
{
	class IConsole *m_pConsole;
	class IStorage *m_pStorage;
	class CLayers* m_pLayers;
	class CCommandProcessor* m_pCommandProcessor;

	IServer *m_pServer;
	CCollision m_Collision;
	CNetObjHandler m_NetObjHandler;
	CTuningParams m_Tuning;

	static bool ConTuneParam(IConsole::IResult *pResult, void *pUserData);
	static bool ConTuneReset(IConsole::IResult *pResult, void *pUserData);
	static bool ConTuneDump(IConsole::IResult *pResult, void *pUserData);
	static bool ConPause(IConsole::IResult *pResult, void *pUserData);
	static bool ConRestart(IConsole::IResult *pResult, void *pUserData);
	static bool ConBroadcast(IConsole::IResult *pResult, void *pUserData);
	static bool ConSay(IConsole::IResult *pResult, void *pUserData);
	static bool ConSetTeam(IConsole::IResult *pResult, void *pUserData);
	static bool ConSetTeamAll(IConsole::IResult *pResult, void *pUserData);
	static bool ConSwapTeams(IConsole::IResult *pResult, void *pUserData);
	static bool ConShuffleTeams(IConsole::IResult *pResult, void *pUserData);
	static bool ConLockTeams(IConsole::IResult *pResult, void *pUserData);
	static bool ConAddVote(IConsole::IResult *pResult, void *pUserData);
	static bool ConRemoveVote(IConsole::IResult *pResult, void *pUserData);
	static bool ConForceVote(IConsole::IResult *pResult, void *pUserData);
	static bool ConClearVotes(IConsole::IResult *pResult, void *pUserData);
	static bool ConVote(IConsole::IResult *pResult, void *pUserData);

	static void ConchainSpecialMotdupdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);

	static void ChatConsolePrintCallback(const char *pLine, void *pUser);
	static bool ConLanguage(IConsole::IResult *pResult, void *pUserData);
	static bool ConAbout(IConsole::IResult *pResult, void *pUserData);
	static bool ConChatAI(IConsole::IResult *pResult, void *pUserData);

	CGameContext(int Resetting);
	void Construct(int Resetting);

	bool m_Resetting;

	int m_ConsoleOutputHandle_ChatPrint;
	int m_ConsoleOutput_Target;

	int m_WorldID;
	int m_RespawnWorldID;

public:
	IServer *Server() const { return m_pServer; }
	class IConsole *Console() { return m_pConsole; }
	class IStorage *Storage() { return m_pStorage; }
	CCommandProcessor* CommandProcessor() const { return m_pCommandProcessor; }
	CCollision *Collision() { return &m_Collision; }
	CTuningParams *Tuning() { return &m_Tuning; }
	CGameContext();
	~CGameContext();

	void Clear();

	CEventHandler m_Events;
	CPlayer *m_apPlayers[MAX_CLIENTS];

	CGameControllerWorkbenches *m_pController;
	CGameWorld m_World;

	CChatAI *m_pChatAI;

	// helper functions
	class CCharacter *GetPlayerChar(int ClientID);
	CPlayer *GetPlayer(int ClientID);

	const char *GetClientLanguage(int ClientID);

	int m_LockTeams;

	// voting
	void StartVote(const char *pDesc, const char *pCommand, const char *pReason);
	void EndVote();
	void SendVoteSet(int ClientID);
	void SendVoteStatus(int ClientID, int Total, int Yes, int No);
	void AbortVoteKickOnDisconnect(int ClientID);

	int m_VoteCreator;
	int64 m_VoteCloseTime;
	bool m_VoteUpdate;
	int m_VotePos;
	char m_aVoteDescription[VOTE_DESC_LENGTH];
	char m_aVoteCommand[VOTE_CMD_LENGTH];
	char m_aVoteReason[VOTE_REASON_LENGTH];
	int m_NumVoteOptions;
	int m_VoteEnforce;
	enum
	{
		VOTE_ENFORCE_UNKNOWN = 0,
		VOTE_ENFORCE_NO,
		VOTE_ENFORCE_YES,
	};
	CHeap *m_pVoteOptionHeap;
	CVoteOptionServer *m_pVoteOptionFirst;
	CVoteOptionServer *m_pVoteOptionLast;

	// helper functions
	void CreateDamageInd(vec2 Pos, float AngleMod, int Amount, CClientMask Mask = CClientMask().set());
	void CreateExplosion(vec2 Pos, int Owner, int Weapon, bool NoDamage, CClientMask Mask = CClientMask().set());
	void CreateHammerHit(vec2 Pos, CClientMask Mask = CClientMask().set());
	void CreatePlayerSpawn(vec2 Pos, CClientMask Mask = CClientMask().set());
	void CreateDeath(vec2 Pos, int Who, CClientMask Mask = CClientMask().set());
	void CreateSound(vec2 Pos, int Sound, CClientMask Mask = CClientMask().set());
	void CreateExtraEffect(vec2 Pos, int Effect, CClientMask Mask = CClientMask().set());
	void CreateMapSound(vec2 Pos, int MapSoundID, CClientMask Mask = CClientMask().set());
	void CreateMapSoundGlobal(int MapSoundID, int Target = -1);

	enum
	{
		CHAT_ALL = -2,
		CHAT_SPEC = -1,
		CHAT_RED = 0,
		CHAT_BLUE = 1
	};

	// network
	void SendChat(int ClientID, int Team, const char *pText);
	void SendEmoticon(int ClientID, int Emoticon);
	void SendWeaponPickup(int ClientID, int Weapon);
	void SetClientLanguage(int ClientID, const char *pLanguage);

	//
	void SendTuningParams(int ClientID);

	//
	void SwapTeams();

	// engine events
	void OnInit(int WorldID) override;
	void OnConsoleInit() override;
	void OnShutdown() override;

	void OnTick() override;
	void OnPreSnap() override;
	void OnSnap(int ClientID) override;
	void OnPostSnap() override;

	void OnMessage(int MsgID, CUnpacker *pUnpacker, int ClientID) override;

	void OnClientConnected(int ClientID, bool AI) override;
	void OnClientEnter(int ClientID) override;
	void KillCharacter(int ClientID) override;
	void OnClientDrop(int ClientID, const char *pReason) override;
	void OnClientDirectInput(int ClientID, void *pInput) override;
	void OnClientPredictedInput(int ClientID, void *pInput) override;

	bool IsClientReady(int ClientID) override;
	bool IsClientPlayer(int ClientID) override;

	void OnSetAuthed(int ClientID, int Level) override;

	const char *GameType() override;
	const char *Version() override;
	const char *NetVersion() override;

	int GetClientVersion(int ClientId) const;

	void AddBot();
	void KickBots();
	void KickBot(int ClientID);
	
	bool IsBot(int ClientID);

	void UpdateAI();

	bool AIInputUpdateNeeded(int ClientID) override;
	void AIUpdateInput(int ClientID, int *Data) override;

	bool IsPlayerEqualWorld(int ClientID, int WorldID = -1) const;
	bool IsPlayersNearby(vec2 Pos, float Distance) const;
	int GetRespawnWorld() const { return m_RespawnWorldID; }

	bool PlayerExists(int ClientID) const override { return m_apPlayers[ClientID]; }
	void OnClientPrepareChangeWorld(int ClientID) override;
	void ClearClientData(int ClientID) override;

	int GetWorldID() const { return m_WorldID; }
	bool IsPlayerInWorld(int ClientID, int WorldID = -1) const;
	bool ArePlayersNearby(vec2 Pos, float Distance) const;

public:
	template <class Tm, typename... Ts>
	void SendNetworkMessage(Tm Msg, int WorldID, int ClientID, const char *pText, Ts &&...args)
	{
		const int Start = (ClientID < 0 ? 0 : ClientID);
		const int End = (ClientID < 0 ? MAX_CLIENTS : ClientID + 1);

		for (int i = Start; i < End; i++)
		{
			if (IsPlayerInWorld(i, WorldID))
			{
				std::string endText = Server()->Localization()->Format(GetClientLanguage(i), pText, std::forward<Ts>(args)...);
				Msg.m_pMessage = endText.c_str();
				Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, i, WorldID);
			}
		}
	}
	
	template <typename... Ts>
	void Chat(int ClientID, const char *pText, Ts &&...args)
	{
		CNetMsg_Sv_Chat Msg;
		Msg.m_ClientID = -1;
		Msg.m_Team = -1;
		SendNetworkMessage<CNetMsg_Sv_Chat>(Msg, -1, ClientID, pText, std::forward<Ts>(args)...);
	}

	template <typename... Ts>
	void ChatTeam(int Team, const char *pText, Ts &&...args)
	{
		CNetMsg_Sv_Chat Msg;
		Msg.m_ClientID = -1;
		Msg.m_Team = -1;
		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if(GetPlayer(i) && GetPlayer(i)->GetTeam() == Team)
				SendNetworkMessage<CNetMsg_Sv_Chat>(Msg, -1, i, pText, std::forward<Ts>(args)...);
		}
	}

	template <typename... Ts>
	void Motd(int ClientID, const char *pText, Ts &&...args)
	{
		CNetMsg_Sv_Motd Msg;
		SendNetworkMessage<CNetMsg_Sv_Motd>(Msg, -1, ClientID, pText, std::forward<Ts>(args)...);
	}

	template <typename... Ts>
	void Broadcast(int ClientID, const char *pText, Ts &&...args)
	{
		CNetMsg_Sv_Broadcast Msg;
		SendNetworkMessage<CNetMsg_Sv_Broadcast>(Msg, -1, ClientID, pText, std::forward<Ts>(args)...);
	}

// Vote
public:
	struct SPlayerVote
	{
		struct SVoteOptions
		{
			char m_aDescription[VOTE_DESC_LENGTH] = {0};
			char m_aCommand[VOTE_CMD_LENGTH] = {0};
		};
		array<SVoteOptions> m_aVoteOptions;

		int m_LastPage;
		int m_Page;

		int m_Select;
	};

	SPlayerVote m_aPlayerVotes[MAX_CLIENTS];

	SPlayerVote *GetPlayerVote(int ClientID) { return &m_aPlayerVotes[ClientID]; }

	template <typename... Ts>
	void AddVote_VL(const char *pCmd, const char *pText, Ts &&...args)
	{
		int ClientID = m_VoteClientID;
		const int Start = (ClientID < 0 ? 0 : ClientID);
		const int End = (ClientID < 0 ? MAX_CLIENTS : ClientID + 1);

		for (int i = Start; i < End; i++)
		{
			std::string endText = Server()->Localization()->Format(GetClientLanguage(i), pText, std::forward<Ts>(args)...);
			AddVote(endText.c_str(), pCmd, i);
		}
	}

	template <typename... Ts>
	void AddVote_Goto(int Page, const char *pDesc, Ts &&...args)
	{
		if (!PlayerExists(m_VoteClientID))
			return;

		char aPageFormat[64];
		str_format(aPageFormat, sizeof(aPageFormat), "ccv_goto %d", Page);
		AddVote_VL(aPageFormat, pDesc, std::forward<Ts>(args)...);
	}
	void AddVote(const char *pDesc, const char *pCmd, int ClientID = -1);
	void AddVote_Back();
	void AddVote_Space(int Num = 1);
	template <typename... Ts>
	void AddVote_Text(const char *pText, Ts &&...args) { AddVote_VL("ccv_null", pText, std::forward<Ts>(args)...); }
	void SetVoteLastPage(int Page) { GetPlayerVote(m_VoteClientID)->m_LastPage = Page; }
	void SetVoteClientID(int CID) { m_VoteClientID = CID; }

	void InitVotes(int ClientID);
	void ClearVotes(int ClientID);
	void ClearVotesTeam(int Team);
	void ChangeVotePage(int ClientID, int Page);

	CBuildMenu m_BuildMenu;
	void OpenBuildMenu(int ClientID);
	void ToggleBuildMenu(int ClientID);
	void CloseBuildMenu(int ClientID);
	void RefreshBuildMenu(int ClientID);
	bool HandleBuildMenuInput(int ClientID, const CNetObj_PlayerInput *pInput, const CNetObj_PlayerInput *pPrevInput);
	void BuildMenuGoBack(int ClientID);
	void RefreshBuildMenuTeam(int Team);

	CClassMenu m_ClassMenu;
	void OpenClassMenu(int ClientID);
	void CloseClassMenu(int ClientID);
	void RefreshClassMenu(int ClientID);
	void RefreshClassVotes(int ClientID);
	bool HandleClassMenuInput(int ClientID, const CNetObj_PlayerInput *pInput, const CNetObj_PlayerInput *pPrevInput);

	int m_VoteClientID;

public:
	CBuildingInfo *m_pBuildingsInfo;
	CBuilding *m_pBuildings;
	
};

inline int CmaskAll() { return -1; }
inline int CmaskOne(int ClientID) { return 1 << ClientID; }
inline int CmaskAllExceptOne(int ClientID) { return 0x7fffffff ^ CmaskOne(ClientID); }
inline bool CmaskIsSet(int Mask, int ClientID) { return (Mask & CmaskOne(ClientID)) != 0; }
#endif
