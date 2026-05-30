#ifndef GAME_SERVER_CORE_COMMAND_PROCESSOR_H
#define GAME_SERVER_CORE_COMMAND_PROCESSOR_H

#include <game/server/commands.h>

class CCommandProcessor
{
	class CGameContext* m_pGS;
	class CGameContext* GS() const { return m_pGS; }
	CCommandManager m_CommandManager;

	static bool ConBuild(IConsole::IResult *pResult, void *pUserData);
	static bool ConAircraft(IConsole::IResult *pResult, void *pUserData);
	static bool ConHelicopter(IConsole::IResult *pResult, void *pUserData);
	static bool ConJet(IConsole::IResult *pResult, void *pUserData);
	static bool ConTank(IConsole::IResult *pResult, void *pUserData);
	static bool ConCar(IConsole::IResult *pResult, void *pUserData);

	static bool VotMakeBuilding(IConsole::IResult *pResult, void *pUserData);
	static bool VotSelectBuilding(IConsole::IResult *pResult, void *pUserData);
	static bool VotGoto(IConsole::IResult *pResult, void *pUserData);
	static bool VotBattleClass(IConsole::IResult *pResult, void *pUserData);
	static bool ConBattleClass(IConsole::IResult *pResult, void *pUserData);
	static bool VotOpenClassMenu(IConsole::IResult *pResult, void *pUserData);
	static bool VotBattleTeleport(IConsole::IResult *pResult, void *pUserData);
	static bool ConBattleE(IConsole::IResult *pResult, void *pUserData);
	

public:
	CCommandProcessor(CGameContext* pGS);
	~CCommandProcessor();

	void Process(const char* pMessage, class CPlayer *pPlayer, int Flags);

private:
	void AddCommand(const char* pName, const char* pParams, int Flags, IConsole::FCommandCallback pfnFunc, void* pUser, const char* pHelp);
};

#endif
