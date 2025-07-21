#include "command_processor.h"
#include "entities/vehicle/car.h"

#include <engine/server.h>
#include <engine/shared/config.h>

#include <game/server/gamecontext.h>
#include <game/server/player.h>

#include <teeother/tl/nlohmann_json.h>

CCommandProcessor::CCommandProcessor(CGameContext *pGS)
{
	m_pGS = pGS;
	m_CommandManager.Init(m_pGS->Console(), this, nullptr, nullptr);

	IServer *pServer = m_pGS->Server();
	AddCommand("build", "ii", CFGFLAG_CHAT, ConBuild, pServer, "[team][type] For testing");
	AddCommand("makebuilding", "i", CFGFLAG_VOTE, VotMakeBuilding, pServer, "[building] Make building in workbench");
	AddCommand("selectbuilding", "i", CFGFLAG_VOTE, VotSelectBuilding, pServer, "[building] Select building to construct");
	AddCommand("goto", "i", CFGFLAG_VOTE, VotGoto, pServer, "[page] - Go to a vote page");
}

CCommandProcessor::~CCommandProcessor()
{
	m_CommandManager.ClearCommands();
}

static CGameContext *GetCommandResultGameServer(int ClientID, void *pUser)
{
	IServer *pServer = (IServer *)pUser;
	return (CGameContext *)pServer->GameServer(pServer->GetClientWorldID(ClientID));
}

// Debug
bool CCommandProcessor::ConBuild(IConsole::IResult *pResult, void *pUserData)
{
	/*
	const int ClientID = pResult->GetClientID();
	CGameContext *pGS = GetCommandResultGameServer(ClientID, pUserData);
	if (!pGS->GetPlayer(ClientID))
		return true;

	new CBuilding(&pGS->m_World, pResult->GetInteger(0), pGS->GetPlayer(ClientID)->GetCharacter()->GetPos(), pResult->GetInteger(1));
	// new CCar(&pGS->m_World, pGS->GetPlayer(ClientID)->GetCharacter()->GetPos());
	*/
	return true;
}

bool CCommandProcessor::VotMakeBuilding(IConsole::IResult *pResult, void *pUserData)
{
	const int ClientID = pResult->GetClientID();
	CGameContext *pGS = GetCommandResultGameServer(ClientID, pUserData);
	if (!pGS->GetPlayer(ClientID))
		return true;

	if (pGS->GetPlayerVote(ClientID)->m_Page == PAGE_MAKE)
	{
		bool Yes = true;
		for (int i = 0; i < NUM_RESOURCE; i++)
		{
			if (pGS->m_pController->m_aTeamResources[pGS->GetPlayer(ClientID)->GetTeam()][i] < pGS->m_pBuildingsInfo->m_aBuildingsInfo[pGS->GetPlayerVote(ClientID)->m_Select].m_Formula[i])
			{
				Yes = false;
				pGS->Chat(ClientID, "Your team doesn't have enough resources!");
				break;
			}
		}
		if (Yes)
		{
			for (int i = 0; i < NUM_RESOURCE; i++)
				pGS->m_pController->m_aTeamResources[pGS->GetPlayer(ClientID)->GetTeam()][i] -= pGS->m_pBuildingsInfo->m_aBuildingsInfo[pGS->GetPlayerVote(ClientID)->m_Select].m_Formula[i];
			pGS->m_pController->m_aTeamBuildings[pGS->GetPlayer(ClientID)->GetTeam()][pGS->GetPlayerVote(ClientID)->m_Select]++;
			pGS->ChatTeam(pGS->GetPlayer(ClientID)->GetTeam(), "'{}' made a {} for the team!", pGS->Server()->ClientName(ClientID), pGS->m_pBuildingsInfo->m_aBuildingsInfo[pGS->GetPlayerVote(ClientID)->m_Select].m_aName);
		}
	}
	else
	{
		pGS->GetPlayerVote(ClientID)->m_Page = PAGE_MAKE;
		pGS->GetPlayerVote(ClientID)->m_Select = pResult->GetInteger(0);
		pGS->ClearVotesTeam(pGS->GetPlayer(ClientID)->GetTeam());
	}
	return true;
}

bool CCommandProcessor::VotSelectBuilding(IConsole::IResult *pResult, void *pUserData)
{
	const int ClientID = pResult->GetClientID();
	CGameContext *pGS = GetCommandResultGameServer(ClientID, pUserData);
	if (!pGS->GetPlayer(ClientID))
		return true;

	pGS->GetPlayer(ClientID)->m_SelectBuilding = pResult->GetInteger(0);
	pGS->Chat(ClientID, "You selected {}! Hammer to build it!", pGS->m_pBuildingsInfo->m_aBuildingsInfo[pResult->GetInteger(0)].m_aName);
	// new CCar(&pGS->m_World, pGS->GetPlayer(ClientID)->GetCharacter()->GetPos());
	return true;
}

bool CCommandProcessor::VotGoto(IConsole::IResult *pResult, void *pUserData)
{
	const int ClientID = pResult->GetClientID();
	CGameContext *pGS = GetCommandResultGameServer(ClientID, pUserData);
	pGS->GetPlayerVote(ClientID)->m_Page = pResult->GetInteger(0);
	pGS->ClearVotes(ClientID);
	return true;
}

/************************************************************************/
/*  Command system                                                      */
/************************************************************************/

void CCommandProcessor::Process(const char *pMessage, CPlayer *pPlayer, int Flags)
{
	const int ClientID = pPlayer->GetCID();

	int Char = 0;
	char aCommand[256] = {0};
	for (int i = 1; i < str_length(pMessage); i++)
	{
		if (pMessage[i] != ' ')
		{
			aCommand[Char] = pMessage[i];
			Char++;
			continue;
		}
		break;
	}

	const IConsole::CCommandInfo *pCommand = GS()->Console()->GetCommandInfo(aCommand, Flags, false);
	if (pCommand)
	{
		GS()->Console()->ExecuteLineFlag(pMessage + 1, ClientID, Flags, GS()->GetClientLanguage(ClientID));
		return;
	}

	GS()->Chat(ClientID, "Command {} not found!", pMessage);
}

// Function to add a new command to the command processor
void CCommandProcessor::AddCommand(const char *pName, const char *pParams, int Flags, IConsole::FCommandCallback pfnFunc, void *pUser, const char *pHelp)
{
	// Register the command in the console
	GS()->Console()->Register(pName, pParams, Flags, pfnFunc, pUser, pHelp);

	// Add the command to the command manager
	m_CommandManager.AddCommand(pName, pHelp, pParams, pfnFunc, pUser);
}