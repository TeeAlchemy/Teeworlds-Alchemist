#ifndef ENGINE_SHARED_ECON_H
#define ENGINE_SHARED_ECON_H

#include "network.h"

#include <engine/console.h>
#include <base/color.h>

class CConfig;
class CNetBan;
class ColorRGBA;

class CEcon
{
	enum
	{
		MAX_AUTH_TRIES = 3,
	};

	class CClient
	{
	public:
		enum
		{
			STATE_EMPTY = 0,
			STATE_CONNECTED,
			STATE_AUTHED,
		};

		int m_State;
		int64_t m_TimeConnected;
		int m_AuthTries;
	};
	CClient m_aClients[NET_MAX_CONSOLE_CLIENTS];

	CConfig *m_pConfig;
	IConsole *m_pConsole;
	CNetConsole m_NetConsole;

	bool m_Ready;
	int m_PrintCBIndex;
	int m_UserClientID;

	static void SendLineCB(const char *pLine, void *pUserData);
	static void ConchainEconOutputLevelUpdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	static void ConLogout(IConsole::IResult *pResult, void *pUserData);

	static int NewClientCallback(int ClientID, void *pUser);
	static int DelClientCallback(int ClientID, const char *pReason, void *pUser);

public:
	CEcon();
	IConsole *Console() { return m_pConsole; }

	void Init(IConsole *pConsole, CNetBan *pNetBan);
	void Update();
	void Send(int ClientID, const char *pLine);
	void Shutdown();
};

#endif
