#include "command_processor.h"
#include "entities/buildings.h"
#include "entities/area-flag.h"
#include "battle.h"
#include "entities/vehicle/aircraft.h"
#include "entities/vehicle/car.h"
#include "entities/vehicle/helicopter.h"
#include "entities/vehicle/jet.h"
#include "entities/vehicle/tank.h"

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
	AddCommand("aircraft", "?r", CFGFLAG_CHAT, ConAircraft, pServer, "[give] Spawn aircraft or add one to team stock");
	AddCommand("helicopter", "?r", CFGFLAG_CHAT, ConHelicopter, pServer, "[give] Spawn helicopter or add one to team stock");
	AddCommand("jet", "?r", CFGFLAG_CHAT, ConJet, pServer, "[give] Spawn jet or add one to team stock");
	AddCommand("tank", "?r", CFGFLAG_CHAT, ConTank, pServer, "[give] Spawn tank or add one to team stock");
	AddCommand("car", "", CFGFLAG_CHAT, ConCar, pServer, "Spawn a car for testing");
	AddCommand("makebuilding", "i", CFGFLAG_VOTE, VotMakeBuilding, pServer, "[building] Make building in workbench");
	AddCommand("selectbuilding", "i", CFGFLAG_VOTE, VotSelectBuilding, pServer, "[building] Select building to construct");
	AddCommand("goto", "i", CFGFLAG_VOTE, VotGoto, pServer, "[page] - Go to a vote page");
	AddCommand("battleclass", "i", CFGFLAG_VOTE, VotBattleClass, pServer, "[class] Select battle class");
	AddCommand("class", "i", CFGFLAG_CHAT, ConBattleClass, pServer, "[0-3] Select battle class");
	AddCommand("opclassmenu", "", CFGFLAG_VOTE, VotOpenClassMenu, pServer, "Open battle class menu");
	AddCommand("battleteleport", "i", CFGFLAG_VOTE, VotBattleTeleport, pServer, "[index] Teleport to captured checkpoint");
	AddCommand("e", "", CFGFLAG_CHAT, ConBattleE, pServer, "Dismount vehicle or throw picked item");
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

typedef void (*FSpawnVehicle)(CGameWorld *pWorld, vec2 Pos, int Team);

static void SpawnAircraft(CGameWorld *pWorld, vec2 Pos, int Team) { new CAircraft(pWorld, Pos, Team); }
static void SpawnHelicopter(CGameWorld *pWorld, vec2 Pos, int Team) { new CHelicopter(pWorld, Pos, Team); }
static void SpawnJet(CGameWorld *pWorld, vec2 Pos, int Team) { new CJet(pWorld, Pos, Team); }
static void SpawnTank(CGameWorld *pWorld, vec2 Pos, int Team) { new CTank(pWorld, Pos, Team); }

static bool DebugVehicleCommand(IConsole::IResult *pResult, void *pUserData, int BuildingType, FSpawnVehicle pfnSpawn, const char *pSpawnKey, const char *pGiveKey)
{
	const int ClientID = pResult->GetClientID();
	CGameContext *pGS = GetCommandResultGameServer(ClientID, pUserData);
	CPlayer *pPlayer = pGS->GetPlayer(ClientID);
	CCharacter *pChr = pPlayer ? pPlayer->GetCharacter() : nullptr;
	if (!pPlayer || !pChr)
		return true;

	const int Team = pPlayer->GetTeam();
	if (Team != TEAM_RED && Team != TEAM_BLUE)
	{
		pGS->Chat(ClientID, "You must be on a team to use this command.");
		return true;
	}

	if (pResult->NumArguments() >= 1 && !str_comp(pResult->GetString(0), "give"))
	{
		pGS->m_pController->m_aTeamBuildings[Team][BuildingType]++;
		pPlayer->m_SelectBuilding = BuildingType;
		pGS->Chat(ClientID, pGiveKey);
		return true;
	}

	pfnSpawn(&pGS->m_World, pChr->GetPos(), Team);
	pGS->Chat(ClientID, pSpawnKey);
	return true;
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

bool CCommandProcessor::ConAircraft(IConsole::IResult *pResult, void *pUserData)
{
	return DebugVehicleCommand(pResult, pUserData, BUILDING_AIRCRAFT, SpawnAircraft, "Spawned an aircraft for testing.", "Added 1 aircraft to team stock. Hammer the ground inside a node to place it.");
}

bool CCommandProcessor::ConHelicopter(IConsole::IResult *pResult, void *pUserData)
{
	return DebugVehicleCommand(pResult, pUserData, BUILDING_HELICOPTER, SpawnHelicopter, "Spawned a helicopter for testing.", "Added 1 helicopter to team stock. Hammer the ground inside a node to place it.");
}

bool CCommandProcessor::ConJet(IConsole::IResult *pResult, void *pUserData)
{
	return DebugVehicleCommand(pResult, pUserData, BUILDING_JET, SpawnJet, "Spawned a jet for testing.", "Added 1 jet to team stock. Hammer the ground inside a node to place it.");
}

bool CCommandProcessor::ConTank(IConsole::IResult *pResult, void *pUserData)
{
	return DebugVehicleCommand(pResult, pUserData, BUILDING_TANK, SpawnTank, "Spawned a tank for testing.", "Added 1 tank to team stock. Hammer the ground inside a node to place it.");
}

bool CCommandProcessor::ConCar(IConsole::IResult *pResult, void *pUserData)
{
	const int ClientID = pResult->GetClientID();
	CGameContext *pGS = GetCommandResultGameServer(ClientID, pUserData);
	CPlayer *pPlayer = pGS->GetPlayer(ClientID);
	CCharacter *pChr = pPlayer ? pPlayer->GetCharacter() : nullptr;
	if (!pPlayer || !pChr)
		return true;

	const int Team = pPlayer->GetTeam();
	if (Team != TEAM_RED && Team != TEAM_BLUE)
	{
		pGS->Chat(ClientID, "You must be on a team to use this command.");
		return true;
	}

	(void)pResult;
	new CCar(&pGS->m_World, pChr->GetPos(), Team);
	pGS->Chat(ClientID, "Spawned a car for testing.");
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
		if (pGS->m_pController->MakeBuilding(pGS->GetPlayerVote(ClientID)->m_Select, pGS->GetPlayer(ClientID)->GetTeam(), ClientID))
			pGS->RefreshBuildMenuTeam(pGS->GetPlayer(ClientID)->GetTeam());
	}
	else
	{
		pGS->GetPlayerVote(ClientID)->m_Page = PAGE_MAKE;
		pGS->GetPlayerVote(ClientID)->m_Select = pResult->GetInteger(0);
		pGS->GetPlayer(ClientID)->m_BuildMenuSelection = 0;
		pGS->RefreshBuildMenu(ClientID);
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
	pGS->RefreshBuildMenu(ClientID);
	return true;
}

bool CCommandProcessor::VotGoto(IConsole::IResult *pResult, void *pUserData)
{
	const int ClientID = pResult->GetClientID();
	CGameContext *pGS = GetCommandResultGameServer(ClientID, pUserData);
	pGS->ChangeVotePage(ClientID, pResult->GetInteger(0));
	return true;
}

bool CCommandProcessor::VotBattleClass(IConsole::IResult *pResult, void *pUserData)
{
	const int ClientID = pResult->GetClientID();
	CGameContext *pGS = GetCommandResultGameServer(ClientID, pUserData);
	CPlayer *pPlayer = pGS->GetPlayer(ClientID);
	if (!pPlayer)
		return true;

	const int Class = pResult->GetInteger(0);
	if (Class < 0 || Class >= NUM_BATTLE_CLASS)
	{
		pGS->Chat(ClientID, "Invalid battle class.");
		return true;
	}

	pPlayer->m_BattleClass = Class;
	pPlayer->m_PendingClassMenu = false;
	pGS->CloseClassMenu(ClientID);
	pGS->ClearVotes(ClientID);
	pGS->Chat(ClientID, "Selected {}.", BattleClassName(Class));

	if (CCharacter *pChr = pPlayer->GetCharacter())
		BattleApplyLoadout(pChr, Class);
	else
		pPlayer->TryRespawn();

	return true;
}

bool CCommandProcessor::ConBattleClass(IConsole::IResult *pResult, void *pUserData)
{
	return VotBattleClass(pResult, pUserData);
}

bool CCommandProcessor::VotOpenClassMenu(IConsole::IResult *pResult, void *pUserData)
{
	(void)pResult;
	const int ClientID = pResult->GetClientID();
	CGameContext *pGS = GetCommandResultGameServer(ClientID, pUserData);
	pGS->CloseBuildMenu(ClientID);
	pGS->OpenClassMenu(ClientID);
	return true;
}

bool CCommandProcessor::VotBattleTeleport(IConsole::IResult *pResult, void *pUserData)
{
	const int ClientID = pResult->GetClientID();
	CGameContext *pGS = GetCommandResultGameServer(ClientID, pUserData);
	CPlayer *pPlayer = pGS->GetPlayer(ClientID);
	CCharacter *pChr = pPlayer ? pPlayer->GetCharacter() : nullptr;
	if (!pPlayer || !pChr)
		return true;

	const int Team = pPlayer->GetTeam();
	const int TargetIndex = pResult->GetInteger(0);
	int Current = 0;
	vec2 Dest = vec2(0, 0);
	bool Found = false;

	for (CAreaFlag *pFlag = (CAreaFlag *)pGS->m_World.FindFirst(CGameWorld::ENTTYPE_AREA_FLAG); pFlag; pFlag = (CAreaFlag *)pFlag->TypeNext())
	{
		if (pFlag->GetTeam() != Team)
			continue;
		if (Current == TargetIndex)
		{
			Dest = pFlag->GetPos();
			Found = true;
			break;
		}
		Current++;
	}

	if (!Found)
	{
		pGS->Chat(ClientID, "Invalid checkpoint.");
		return true;
	}

	pChr->GetCore()->m_Pos = Dest;
	pChr->GetCore()->m_Vel = vec2(0, 0);
	pGS->Chat(ClientID, "Teleported to checkpoint {}.", TargetIndex + 1);
	pGS->CloseBuildMenu(ClientID);
	return true;
}

bool CCommandProcessor::ConBattleE(IConsole::IResult *pResult, void *pUserData)
{
	(void)pResult;
	const int ClientID = pResult->GetClientID();
	CGameContext *pGS = GetCommandResultGameServer(ClientID, pUserData);
	CPlayer *pPlayer = pGS->GetPlayer(ClientID);
	if (!pPlayer)
		return true;

	BattleHandleECommand(pGS, pPlayer);
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