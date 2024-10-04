/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_SERVER_H
#define ENGINE_SERVER_H
#include "kernel.h"
#include "message.h"
#include <string>
#include <vector>

#include <engine/map.h>
#include <game/generated/protocol.h>
#include <engine/shared/protocol.h>

class IServer : public IInterface
{
	MACRO_INTERFACE("server", 0)
protected:
	int m_CurrentGameTick;
	int m_TickSpeed;

public:
	class CLocalization *m_pLocalization;

	enum
	{
		AUTHED_NO = 0,
		AUTHED_MOD,
		AUTHED_ADMIN,
	};

public:
	virtual class IGameServer* GameServer(int WorldID = 0) = 0;
	virtual class IGameServer* GameServerPlayer(int ClientID) = 0;
	/*
		Structure: CClientInfo
	*/
	struct CClientInfo
	{
		const char *m_pName;
		int m_Latency;
		int m_Authed;
		bool m_CustClt;
		bool m_GotDDNetVersion;
		int m_DDNetVersion;
		const char *m_pDDNetVersionStr;
		const CUuid *m_pConnectionId;
	};

	inline class CLocalization *Localization() { return m_pLocalization; }

	int Tick() const { return m_CurrentGameTick; }
	int TickSpeed() const { return m_TickSpeed; }

	virtual int MaxClients() const = 0;
	virtual const char *ClientName(int ClientID) = 0;
	virtual const char *ClientClan(int ClientID) = 0;
	virtual int ClientCountry(int ClientID) = 0;
	virtual bool ClientIngame(int ClientID) = 0;
	virtual bool GetClientInfo(int ClientId, CClientInfo *pInfo) const = 0;
	virtual void SetClientDDNetVersion(int ClientId, int DDNetVersion) = 0;
	virtual void GetClientAddr(int ClientID, char *pAddrStr, int Size) = 0;

	virtual std::string GetClientIP(int ClientID) const = 0;

	/**
	 * Returns the version of the client with the given client ID.
	 *
	 * @param ClientId the client Id, which must be between 0 and
	 * MAX_CLIENTS - 1, or equal to SERVER_DEMO_CLIENT for server demos.
	 *
	 * @return The version of the client with the given client ID.
	 * For server demos this is always the latest client version.
	 * On errors, VERSION_NONE is returned.
	 */
	virtual int GetClientVersion(int ClientId) const = 0;
	virtual int SendMsg(CMsgPacker *pMsg, int Flags, int ClientID, int WorldID) = 0;

	template <class T>
	int SendPackMsg(T *pMsg, int Flags, int ClientID, int WorldID)
	{
		CMsgPacker Packer(pMsg->MsgID());
		if (pMsg->Pack(&Packer))
			return -1;
		return SendMsg(&Packer, Flags, ClientID, WorldID);
	}

	virtual void SetClientName(int ClientID, char const *pName) = 0;
	virtual void SetClientClan(int ClientID, char const *pClan) = 0;
	virtual void SetClientCountry(int ClientID, int Country) = 0;
	virtual void SetClientScore(int ClientID, int Score) = 0;

	virtual int SnapNewID() = 0;
	virtual void SnapFreeID(int ID) = 0;
	virtual void *SnapNewItem(int Type, int ID, int Size) = 0;
	template<typename T>
	T *SnapNewItem(int Id)
	{
		const int Type = T::ms_MsgId;
		return static_cast<T *>(SnapNewItem(Type, Id, sizeof(T)));
	}

	virtual void SnapSetStaticsize(int ItemType, int Size) = 0;

	enum
	{
		RCON_CID_SERV = -1,
		RCON_CID_VOTE = -2,
	};
	virtual void SetRconCID(int ClientID) = 0;
	virtual bool IsAuthed(int ClientID) = 0;
	virtual void Kick(int ClientID, const char *pReason) = 0;
	virtual void RedirectClient(int ClientId, int Port, bool Verbose = false) = 0;

	virtual void DemoRecorder_HandleAutoStart() = 0;
	virtual bool DemoRecorder_IsRecording() = 0;

	virtual const char *GetClientLanguage(int ClientID) = 0;
	virtual void SetClientLanguage(int ClientID, const char *pLanguage) = 0;

	virtual void ExpireServerInfo() = 0;

	virtual void AddBot(int WorldID) = 0;

public:
	virtual bool IsClientChangingWorld(int ClientID) = 0;
	virtual void ChangeWorld(int ClientID, int NewWorldID) = 0;
	virtual int GetClientWorldID(int ClientID) = 0;
	virtual const char* GetWorldName(int WorldID) = 0;
	virtual int GetWorldsSize() const = 0;
	
};

class IGameServer : public IInterface
{
	MACRO_INTERFACE("gameserver", 0)
protected:
public:
	virtual void OnInit(int WorldID) = 0;
	virtual void OnConsoleInit() = 0;
	virtual void OnShutdown() = 0;

	virtual void OnTick() = 0;
	virtual void OnPreSnap() = 0;
	virtual void OnSnap(int ClientID) = 0;
	virtual void OnPostSnap() = 0;

	virtual void OnClientPrepareChangeWorld(int ClientID) = 0;
	virtual void ClearClientData(int ClientID) = 0;
	virtual bool PlayerExists(int ClientID) const = 0;

	virtual void OnMessage(int MsgID, CUnpacker *pUnpacker, int ClientID) = 0;

	virtual void OnClientConnected(int ClientID, bool AI) = 0;
	virtual void KillCharacter(int ClientID) = 0;
	virtual void OnClientEnter(int ClientID) = 0;
	virtual void OnClientDrop(int ClientID, const char *pReason) = 0;
	virtual void OnClientDirectInput(int ClientID, void *pInput) = 0;
	virtual void OnClientPredictedInput(int ClientID, void *pInput) = 0;

	virtual bool IsClientReady(int ClientID) = 0;
	virtual bool IsClientPlayer(int ClientID) = 0;

	virtual const char *GameType() = 0;
	virtual const char *Version() = 0;
	virtual const char *NetVersion() = 0;

	virtual void OnSetAuthed(int ClientID, int Level) = 0;

	virtual bool AIInputUpdateNeeded(int ClientID) = 0;
	virtual void AIUpdateInput(int ClientID, int *Data) = 0; // MAX_INPUT_SIZE
};

extern IGameServer *CreateGameServer();
#endif