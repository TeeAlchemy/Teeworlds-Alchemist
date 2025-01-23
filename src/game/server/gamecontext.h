/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_GAMECONTEXT_H
#define GAME_SERVER_GAMECONTEXT_H

#include <engine/server.h>
#include <engine/console.h>
#include <engine/shared/memheap.h>
#include <engine/storage.h>

#include <teeother/components/localization.h>

#include <game/layers.h>
#include <game/voting.h>

#include <vector>

#include "Item/item.h"
#include "eventhandler.h"
#include "gamecontroller.h"
#include "gameworld.h"
#include "player.h"
#include "botengine.h"

#include "GameCore/TWorldController.h"

class CChatAI;

enum EVotePages
{
	PAGE_MENU = 0,
	PAGE_INVENTORY,
	PAGE_CHECK_ITEM,
	PAGE_CRAFT,
	PAGE_CRAFT_SELECTED,
	PAGE_EQUIPMENT,
	PAGE_TURRET,
};

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
	class CBotEngine *m_pBotEngine;
	class TWorldController *m_pTWorldController;

	IServer *m_pServer;
	IStorage *m_pStorage;
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
	static bool ConRegister(IConsole::IResult *pResult, void *pUserData);
	static bool ConLogin(IConsole::IResult *pResult, void *pUserData);

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
	IStorage *Storage() const { return m_pStorage; }
	class IConsole *Console() { return m_pConsole; }
	CCollision *Collision() { return &m_Collision; }
	CTuningParams *Tuning() { return &m_Tuning; }
	CGameContext();
	~CGameContext();
	class CBotEngine *BotEngine() { return m_pBotEngine; }

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
	void CreateExplosion(vec2 Pos, int Owner, int Weapon, bool NoDamage, bool Fusion, CClientMask Mask = CClientMask().set());
	void CreateHammerHit(vec2 Pos, CClientMask Mask = CClientMask().set());
	void CreatePlayerSpawn(vec2 Pos, CClientMask Mask = CClientMask().set());
	void CreateDeath(vec2 Pos, int Who, CClientMask Mask = CClientMask().set());
	void CreateSound(vec2 Pos, int Sound, CClientMask Mask = CClientMask().set());
	void CreateSoundGlobal(int Sound, int Target = -1);
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
	int CountBots();

	/* SQL */
	TWorldController *TW() const { return m_pTWorldController; };

	enum
	{
		TABLE_ACCOUNT = 0,
		TABLE_ITEM,
	};

	struct LaserDotState
	{
		vec2 m_Pos0;
		vec2 m_Pos1;
		int m_LifeSpan;
		int m_SnapID;
	};
	array<LaserDotState> m_LaserDots;
	
	struct HammerDotState
	{
		vec2 m_Pos;
		int m_LifeSpan;
		int m_SnapID;
	};
	array<HammerDotState> m_HammerDots;
	
	struct LoveDotState
	{
		vec2 m_Pos;
		int m_LifeSpan;
		int m_SnapID;
	};
	array<LoveDotState> m_LoveDots;

	void CreateLaserDotEvent(vec2 Pos0, vec2 Pos1, int LifeSpan);
	void CreateHammerDotEvent(vec2 Pos, int LifeSpan);
	void CreateLoveEvent(vec2 Pos);

public:
	CItemHelper *m_pItemHelper;
	CItemHelper *ItemHelper() { return m_pItemHelper; }
	CItem *Items(int i) { return ItemHelper()->Items(i); };

	bool AwakenBot(int ClientID);
	bool AsleepBot(int ClientID);

	void CountItemNum(int ClientID);

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

	// Vote
public:
	struct SPlayerVote
	{
		enum EVoteSelect
		{
			ITEMLIST = 0,
			ITEM,
			EQUIPMENT,
			SEPARATE,
			NUM_SELECT,
		};

		struct SVoteOptions
		{
			char m_aDescription[VOTE_DESC_LENGTH] = {0};
			char m_aCommand[VOTE_CMD_LENGTH] = {0};
		};
		array<SVoteOptions> m_aVoteOptions;
		int m_LastPage;
		int m_Page;
		int m_Select[NUM_SELECT];
		
		bool m_Confirm;

		char m_aExtraText[VOTE_DESC_LENGTH];
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
	void AddVote(const char *pDesc, const char *pCmd, int ClientID = -1);

	// TODO: 把AddVote_变成一个namespace！！
	// Pack
	void AddVote_ListInventory(int ItemType, const char *pCmd, bool Equip = false);
	void AddVote_ListCraft(int ItemType);
	void AddVote_ListFormula(int ItemID);
	void AddVote_Craft(int ItemID);
	bool AddVote_ListExtraSeparate(int ItemID, std::string Type);
	bool AddVote_ListExtraPlace(int ItemID, std::string Type);

	// Return true if there are any card
	bool UpdateItemCapacity(int ClientID, int ItemID);

	// Helper functions
	template <typename... Ts>
	void AddVote_Goto(int Page, const char *pDesc, Ts &&...args)
	{
		if (!PlayerExists(m_VoteClientID))
			return;

		char aPageFormat[64];
		str_format(aPageFormat, sizeof(aPageFormat), "ccv_goto %d", Page);
		AddVote_VL(aPageFormat, pDesc, std::forward<Ts>(args)...);
	}
	void AddVote_Back();
	void AddVote_Space(int Num = 1);
	template <typename... Ts>
	void AddVote_Text(const char *pText, Ts &&...args) { AddVote_VL("ccv_null", pText, std::forward<Ts>(args)...); }
	void SetVoteLastPage(int Page) { m_aPlayerVotes[m_VoteClientID].m_LastPage = Page; }
	void SetVoteClientID(int CID) { m_VoteClientID = CID; }
	template <typename... Ts>
	void SetVoteExtraText(int CID, const char *pExtraText, Ts &&...args) { str_copy(m_aPlayerVotes[CID].m_aExtraText, Server()->Localization()->Format(GetClientLanguage(CID), pExtraText, std::forward<Ts>(args)...).c_str(), VOTE_DESC_LENGTH); }

	// Vote Engine
	void InitVotes(int ClientID);
	void ClearVotes(int ClientID);

	static bool VotGiveItem(IConsole::IResult *pResult, void *pUserData);
	static bool VotSelectItem(IConsole::IResult *pResult, void *pUserData);
	static bool VotGoto(IConsole::IResult *pResult, void *pUserData);
	static bool VotCheckItem(IConsole::IResult *pResult, void *pUserData);
	static bool VotCraft(IConsole::IResult *pResult, void *pUserData);
	static bool VotMake(IConsole::IResult *pResult, void *pUserData);
	static bool VotPlace(IConsole::IResult *pResult, void *pUserData);
	static bool VotEquip(IConsole::IResult *pResult, void *pUserData);
	static bool VotSeparate(IConsole::IResult *pResult, void *pUserData);
	static bool VotSetupTurret(IConsole::IResult *pResult, void *pUserData);

private:
	int m_VoteClientID;
};

inline int CmaskAll() { return -1; }
inline int CmaskOne(int ClientID) { return 1 << ClientID; }
inline int CmaskAllExceptOne(int ClientID) { return 0x7fffffff ^ CmaskOne(ClientID); }
inline bool CmaskIsSet(int Mask, int ClientID) { return (Mask & CmaskOne(ClientID)) != 0; }
#endif
