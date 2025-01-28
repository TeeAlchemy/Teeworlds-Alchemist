#ifndef GAME_SERVER_CORE_COMMAND_PROCESSOR_H
#define GAME_SERVER_CORE_COMMAND_PROCESSOR_H

#include <game/server/commands.h>

class CCommandProcessor
{
	class CGameContext* m_pGS;
	class CGameContext* GS() const { return m_pGS; }
	CCommandManager m_CommandManager;

	static bool ConChatLogin(IConsole::IResult* pResult, void* pUserData);
	static bool ConChatRegister(IConsole::IResult* pResult, void* pUserData);
	static bool ConGiveItem(IConsole::IResult *pResult, void *pUserData);
	
	static bool VotSelectItem(IConsole::IResult *pResult, void *pUserData);
	static bool VotGoto(IConsole::IResult *pResult, void *pUserData);
	static bool VotCheckItem(IConsole::IResult *pResult, void *pUserData);
	static bool VotCraft(IConsole::IResult *pResult, void *pUserData);
	static bool VotMake(IConsole::IResult *pResult, void *pUserData);
	static bool VotPlace(IConsole::IResult *pResult, void *pUserData);
	static bool VotEquip(IConsole::IResult *pResult, void *pUserData);
	static bool VotSeparate(IConsole::IResult *pResult, void *pUserData);
	static bool VotSetupTurret(IConsole::IResult *pResult, void *pUserData);
	static bool VotTravel(IConsole::IResult *pResult, void *pUserData);

public:
	CCommandProcessor(CGameContext* pGS);
	~CCommandProcessor();

	void Process(const char* pMessage, class CPlayer *pPlayer, int Flags);

private:
	void AddCommand(const char* pName, const char* pParams, int Flags, IConsole::FCommandCallback pfnFunc, void* pUser, const char* pHelp);
};

#endif
