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

#include "botengine.h"

class CChatAI;

/*
	Tick
		Game Context (CGameContext::OnTick)
			Game World (CGameWorld::Tick)
				Reset world if requested (CGameWorld::Reset)
				All entities in the world (CEntity::Tick)
				All entities in the world (CEntity::TickDeferd)
				Remove entities marked for deletion (CGameWorld::RemoveEntity)
			Game Controller (IGameController::Tick)
			All players (CPlayer::Tick)


	Snap
		Game Context (CGameContext::OnSnap)
			Game World (CGameWorld::Snap)
				All entities in the world (CEntity::Snap)
			Game Controller (IGameController::Snap)
			Events handler (CEventHandler::Snap)
			All players (CPlayer::Snap)

*/
class CGameContext : public IGameServer
{
	class IConsole *m_pConsole;
	class CLayers* m_pLayers;
	class CBotEngine *m_pBotEngine;

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

	static void InitBotEngineThread(CBotEngine *BotEngine, class CTile *pTiles, int Width, int Height);

	bool m_Resetting;

	int m_ConsoleOutputHandle_ChatPrint;
	int m_ConsoleOutput_Target;

	int m_WorldID;
	int m_RespawnWorldID;

public:
	IServer *Server() const { return m_pServer; }
	class IConsole *Console() { return m_pConsole; }
	CCollision *Collision() { return &m_Collision; }
	CTuningParams *Tuning() { return &m_Tuning; }
	CGameContext();
	~CGameContext();
	class CBotEngine *BotEngine() { return m_pBotEngine; }

	void Clear();

	CEventHandler m_Events;
	CPlayer *m_apPlayers[MAX_CLIENTS];

	IGameController *m_pController;
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

	void OnClientConnected(int ClientID) override;
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

	bool IsPlayerEqualWorld(int ClientID, int WorldID = -1) const;
	bool IsPlayersNearby(vec2 Pos, float Distance) const;
	int GetRespawnWorld() const { return m_RespawnWorldID; }

	bool PlayerExists(int ClientID) const override { return m_apPlayers[ClientID]; }
	void OnClientPrepareChangeWorld(int ClientID) override;
	void ClearClientData(int ClientID) override;

	int GetWorldID() const { return m_WorldID; }
	bool IsPlayerInWorld(int ClientID, int WorldID = -1) const;
	bool ArePlayersNearby(vec2 Pos, float Distance) const;

	int GetBotWorldID(int ClientID) override;

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
};

inline int CmaskAll() { return -1; }
inline int CmaskOne(int ClientID) { return 1 << ClientID; }
inline int CmaskAllExceptOne(int ClientID) { return 0x7fffffff ^ CmaskOne(ClientID); }
inline bool CmaskIsSet(int Mask, int ClientID) { return (Mask & CmaskOne(ClientID)) != 0; }
#endif
