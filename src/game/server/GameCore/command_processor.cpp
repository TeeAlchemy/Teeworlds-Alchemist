#include "command_processor.h"
#include "TWorldController.h"
#include "Account/account.h"

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
	AddCommand("login", "ss", CFGFLAG_CHAT, ConChatLogin, pServer, "[username][password] Login to your account");
	AddCommand("register", "ss", CFGFLAG_CHAT, ConChatRegister, pServer, "[username][password] register an account");
	AddCommand("giveitem", "iii", CFGFLAG_CHAT, ConGiveItem, pServer, "[clientid] [itemid] [numitem] - Give item");

	AddCommand("selectitem", "ii", CFGFLAG_VOTE, VotSelectItem, pServer, "[][] - Select");
	AddCommand("goto", "i", CFGFLAG_VOTE, VotGoto, pServer, "[page] - Go to a vote page");
	AddCommand("craft", "i", CFGFLAG_VOTE, VotCraft, pServer, "[item] - Craft something");
	AddCommand("make", "", CFGFLAG_VOTE, VotMake, pServer, "make - Confirm to make something");
	AddCommand("checkitem", "i", CFGFLAG_VOTE, VotCheckItem, pServer, "[item] - Confirm to make something");
	AddCommand("place", "isi", CFGFLAG_VOTE, VotPlace, pServer, "[item][type][card] - Place card");
	AddCommand("equip", "i", CFGFLAG_VOTE, VotEquip, pServer, "[item] - Equip");
	AddCommand("separate", "isi", CFGFLAG_VOTE, VotSeparate, pServer, "[item][type][card] - Separate Card");
	AddCommand("setupturret", "", CFGFLAG_VOTE, VotSetupTurret, pServer, "do it - Set up a turret");
	AddCommand("travel", "i", CFGFLAG_VOTE, VotTravel, pServer, "[world id] - Transfer to other worlds");
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

bool CCommandProcessor::ConChatLogin(IConsole::IResult *pResult, void *pUser)
{
	const int ClientID = pResult->GetClientID();
	CGameContext *pGS = GetCommandResultGameServer(ClientID, pUser);

	CPlayer *pPlayer = pGS->GetPlayer(ClientID);
	if (pPlayer)
	{
		if (pPlayer->LoggedIn())
		{
			pGS->Chat(ClientID, "You're already signed in!");
			return true;
		}

		char aUsername[16];
		char aPassword[16];
		str_copy(aUsername, pResult->GetString(0), sizeof(aUsername));
		str_copy(aPassword, pResult->GetString(1), sizeof(aPassword));

		pGS->TW()->Account()->Login(ClientID, aUsername, aPassword);
	}
	return true;
}

bool CCommandProcessor::ConChatRegister(IConsole::IResult *pResult, void *pUser)
{
	const int ClientID = pResult->GetClientID();
	CGameContext *pGS = GetCommandResultGameServer(ClientID, pUser);

	CPlayer *pPlayer = pGS->GetPlayer(ClientID);
	if (!pPlayer)
		return true;

	if (pPlayer->LoggedIn())
	{
		pGS->Chat(ClientID, "Sign out first before you create a new account.");
		return true;
	}

	char aUsername[16];
	char aPassword[16];
	str_copy(aUsername, pResult->GetString(0), sizeof(aUsername));
	str_copy(aPassword, pResult->GetString(1), sizeof(aPassword));

	if (str_length(aUsername) > 15 || str_length(aUsername) < 2 || str_length(aPassword) > 15 || str_length(aPassword) < 2)
	{
		pGS->Chat(pResult->GetClientID(), "Username / Password must be 2-15 characters");
		return true;
	}

	pGS->TW()->Account()->Register(ClientID, aUsername, aPassword);
	return true;
}

bool CCommandProcessor::ConGiveItem(IConsole::IResult *pResult, void *pUserData)
{
	const int ClientID = pResult->GetClientID();
	CGameContext *pGS = GetCommandResultGameServer(ClientID, pUserData);
	if (!pGS->GetPlayer(pResult->GetInteger(0)) || !pGS->GetPlayer(ClientID) || !pGS->GetPlayer(ClientID)->m_Authed)
		return true;

	pGS->GetPlayer(pResult->GetInteger(0))->m_AccData.m_aItems[pResult->GetInteger(1)].m_Num += pResult->GetInteger(2);
	pGS->TW()->Account()->SaveAccountData(pResult->GetInteger(0), TABLE_ITEM, pGS->GetPlayer(pResult->GetInteger(0))->m_AccData);
	pGS->ClearVotes(pResult->GetInteger(0));
	return true;
}

bool CCommandProcessor::VotSelectItem(IConsole::IResult *pResult, void *pUserData)
{
	const int ClientID = pResult->GetClientID();
	CGameContext *pGS = GetCommandResultGameServer(ClientID, pUserData);
	pGS->GetPlayerVote(ClientID)->m_Select[pResult->GetInteger(0)] = pResult->GetInteger(1);
	pGS->ClearVotes(ClientID);
	return true;
}

bool CCommandProcessor::VotGoto(IConsole::IResult *pResult, void *pUserData)
{
	const int ClientID = pResult->GetClientID();
	CGameContext *pGS = GetCommandResultGameServer(ClientID, pUserData);
	pGS->GetPlayerVote(ClientID)->m_Page = pResult->GetInteger(0);
	pGS->GetPlayerVote(ClientID)->m_Confirm = false;
	pGS->ClearVotes(ClientID);
	return true;
}

bool CCommandProcessor::VotCraft(IConsole::IResult *pResult, void *pUserData)
{
	const int ClientID = pResult->GetClientID();
	CGameContext *pGS = GetCommandResultGameServer(ClientID, pUserData);
	pGS->GetPlayerVote(ClientID)->m_Page = PAGE_CRAFT_SELECTED;
	pGS->GetPlayerVote(ClientID)->m_Select[CGameContext::SPlayerVote::ITEM] = pResult->GetInteger(0);
	pGS->ClearVotes(ClientID);
	return true;
}

bool CCommandProcessor::VotMake(IConsole::IResult *pResult, void *pUserData)
{
	const int ClientID = pResult->GetClientID();
	CGameContext *pGS = GetCommandResultGameServer(ClientID, pUserData);

	CPlayer *pPlayer = pGS->GetPlayer(ClientID);
	if (!pPlayer)
		return false;

	int Item = pGS->GetPlayerVote(ClientID)->m_Select[CGameContext::SPlayerVote::ITEM];

	if (pGS->ItemHelper()->GetMax(Item) && pPlayer->m_AccData.m_aItems[Item].m_Num >= pGS->ItemHelper()->GetMax(Item))
	{
		pGS->SetVoteExtraText(ClientID, "You have reached the limit");
		if (pPlayer->GetCharacter())
			pGS->CreateSoundGlobal(SOUND_WEAPON_NOAMMO, ClientID);
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
				if (pGS->Items(Item)->m_Formula[i])
					pPlayer->m_AccData.m_aItems[i].m_Num -= pGS->Items(Item)->m_Formula[i];
			}
			else
			{
				if (pGS->Items(Item)->m_Formula[i] > 0 && nlohmann::json::accept(pPlayer->m_AccData.m_aItems[i].m_aExtra))
				{
					nlohmann::json Json = nlohmann::json::parse(pPlayer->m_AccData.m_aItems[i].m_aExtra);
					if (!Json["Extra"]["Cards"].empty() || !Json["Extra"]["Parts"].empty())
					{
						IsOK = false;
						pGS->SetVoteExtraText(ClientID, "This item({}) contains cards/parts!", pGS->ItemHelper()->GetItemName(i));
						break;
					}
				}
				if (pGS->Items(Item)->m_Formula[i] > pPlayer->m_AccData.m_aItems[i].m_Num)
				{
					IsOK = false;
					pGS->SetVoteExtraText(ClientID, "You don't have enough materials to make it.");
					break;
				}
			}
		}
	}

	if (IsOK)
	{
		pPlayer->m_AccData.m_aItems[Item].m_Num++;
		pGS->SetVoteExtraText(ClientID, "You have successfully make a {}!", pGS->Items(Item)->m_aItemName);
		pGS->TW()->Account()->SaveAccountData(ClientID, TABLE_ITEM, pPlayer->m_AccData);
		pGS->CreateSoundGlobal(SOUND_CTF_CAPTURE, ClientID);
	}
	else
		pGS->CreateSoundGlobal(SOUND_TEE_CRY, ClientID);

	pGS->ClearVotes(pResult->GetClientID());
	return true;
}

bool CCommandProcessor::VotPlace(IConsole::IResult *pResult, void *pUserData)
{
	const int ClientID = pResult->GetClientID();
	CGameContext *pGS = GetCommandResultGameServer(ClientID, pUserData);

	CPlayer *pPlayer = pGS->GetPlayer(ClientID);
	if (!pPlayer)
		return false;

	/*if (!pGS->GetPlayerVote[ClientID].m_Confirm)
	{
		pGS->SetVoteExtraText(ClientID, "Are you sure?(This will not be reversible)");
		pGS->GetPlayerVote[ClientID].m_Confirm = true;
		pGS->ClearVotes(pResult->GetClientID());
		return true;
	}*/

	int Select = pResult->GetInteger(0);
	std::string Type = pResult->GetString(1);
	int Card = pResult->GetInteger(2);
	pGS->SetVoteExtraText(ClientID, "You placed {} on {}!", pGS->ItemHelper()->GetItemName(Card), pGS->ItemHelper()->GetItemName(Select));

	int Capacity = pGS->ItemHelper()->GetMaxCapacity(Card);
	int ExistCard = -1;

	if (!nlohmann::json::accept(pPlayer->m_AccData.m_aItems[Select].m_aExtra))
	{
		pGS->SetVoteExtraText(ClientID, "BUG! Contact Admin.");
		pGS->ClearVotes(pResult->GetClientID());
		return true;
	}

	nlohmann::json Json = nlohmann::json::parse(pPlayer->m_AccData.m_aItems[Select].m_aExtra);
	if (!Json["Extra"].contains(Type) || Json["Extra"][Type].empty())
	{
		if (Capacity <= pGS->ItemHelper()->GetMaxCapacity(Select))
		{
			Json["Extra"][Type].push_back({{"id", Card}, {"num", 1}});
			pPlayer->m_AccData.m_aItems[Select].m_aExtra = Json.dump();
			pPlayer->m_AccData.m_aItems[Select].m_Capacity = Capacity;
			// pGS->GetPlayerVote[ClientID].m_Confirm = false;

			pPlayer->m_AccData.m_aItems[Card].m_Num--;

			pGS->TW()->Account()->SaveAccountData(ClientID, TABLE_ITEM, pPlayer->m_AccData);
			pGS->ClearVotes(pResult->GetClientID());
			pGS->CreateSoundGlobal(SOUND_CTF_CAPTURE, ClientID);
			return true;
		}
		else
			pGS->SetVoteExtraText(ClientID, "Not enough capacity!");
		return false;
	}
	else
	{
		int CurrentIndex = -1;
		for (const auto &j : Json["Extra"][Type])
		{
			CurrentIndex++;
			Capacity += pGS->ItemHelper()->GetMaxCapacity(int(j["id"])) * int(j["num"]);
			if (j["id"] == Card)
				ExistCard = CurrentIndex;
		}
	}

	if (Capacity <= pGS->ItemHelper()->GetMaxCapacity(Select))
	{
		if (ExistCard != -1)
		{
			if (Json["Extra"][Type][ExistCard]["num"] >= pGS->ItemHelper()->GetMaxPlace(ExistCard))
			{
				pGS->SetVoteExtraText(ClientID, "You have reached the limit");
				pGS->ClearVotes(pResult->GetClientID());
				pGS->CreateSoundGlobal(SOUND_WEAPON_NOAMMO, ClientID);
				return true;
			}
			Json["Extra"][Type][ExistCard]["num"] = int(Json["Extra"][Type][ExistCard]["num"]) + 1;
		}
		else
			Json["Extra"][Type].push_back({{"id", Card}, {"num", 1}});
		pPlayer->m_AccData.m_aItems[Select].m_aExtra = Json.dump();
		pPlayer->m_AccData.m_aItems[Select].m_Capacity = Capacity;
		pPlayer->m_AccData.m_aItems[Card].m_Num--;
		pGS->CreateSoundGlobal(SOUND_CTF_CAPTURE, ClientID);
	}
	else
	{
		pGS->SetVoteExtraText(ClientID, "Not enough capacity!");
		pGS->CreateSoundGlobal(SOUND_WEAPON_NOAMMO, ClientID);
	}

	// pGS->GetPlayerVote[ClientID].m_Confirm = false;

	pGS->TW()->Account()->SaveAccountData(ClientID, TABLE_ITEM, pPlayer->m_AccData);
	pGS->ClearVotes(pResult->GetClientID());
	return true;
}

bool CCommandProcessor::VotCheckItem(IConsole::IResult *pResult, void *pUserData)
{
	const int ClientID = pResult->GetClientID();
	CGameContext *pGS = GetCommandResultGameServer(ClientID, pUserData);
	pGS->GetPlayerVote(ClientID)->m_Page = PAGE_CHECK_ITEM;
	pGS->GetPlayerVote(ClientID)->m_Select[CGameContext::SPlayerVote::ITEM] = pResult->GetInteger(0);
	pGS->ClearVotes(ClientID);
	return true;
}

bool CCommandProcessor::VotEquip(IConsole::IResult *pResult, void *pUserData)
{
	const int CID = pResult->GetClientID();
	CGameContext *pGS = GetCommandResultGameServer(CID, pUserData);
	pGS->GetPlayer(CID)->m_AccData.m_Holding[pGS->ItemHelper()->GetType(pResult->GetInteger(0))] = pResult->GetInteger(0);
	pGS->CreateSoundGlobal(SOUND_PICKUP_NINJA, CID);
	pGS->SetVoteExtraText(CID, "You have successfully equipped the {}", pGS->ItemHelper()->GetItemName(pResult->GetInteger(0)));
	pGS->ClearVotes(CID);
	pGS->TW()->Account()->SaveAccountData(CID, TABLE_ACCOUNT, pGS->GetPlayer(CID)->m_AccData);
	return true;
}

bool CCommandProcessor::VotSeparate(IConsole::IResult *pResult, void *pUserData)
{
	const int CID = pResult->GetClientID();
	CGameContext *pGS = GetCommandResultGameServer(CID, pUserData);

	int Select = pResult->GetInteger(0);
	std::string Type = pResult->GetString(1);
	int Item = pResult->GetInteger(2);

	if (!nlohmann::json::accept(pGS->GetPlayer(CID)->GetExtra(Select)))
	{
		pGS->TW()->Account()->SaveAccountData(CID, TABLE_ITEM, pGS->GetPlayer(CID)->m_AccData);
		pGS->Server()->Kick(CID, "服务器出现错误！请联系开发者QQ:1562151175！感谢！");
		return true;
	}

	nlohmann::json Json = nlohmann::json::parse(pGS->GetPlayer(CID)->GetExtra(Select));

	if (Json["Extra"].contains(Type) && !Json["Extra"][Type].empty())
	{
		int ToBeRemove = 0;
		for (const auto &j : Json["Extra"][Type])
		{
			if (Item == int(j["id"]))
			{
				pGS->SetVoteExtraText(CID, "You separate {} from {}!", pGS->ItemHelper()->GetItemName(int(j["id"])), pGS->ItemHelper()->GetItemName(Select));

				if (int(j["num"]) > 1)
					Json["Extra"][Type][ToBeRemove]["num"] = int(Json["Extra"][Type][ToBeRemove]["num"]) - 1;
				else
					Json["Extra"][Type].erase(ToBeRemove);

				pGS->GetPlayer(CID)->SetExtra(Select, Json.dump());
				pGS->GetPlayer(CID)->m_AccData.m_aItems[Item].m_Num++;
				break;
			}
			ToBeRemove++;
		}
	}
	else
	{
		pGS->TW()->Account()->SaveAccountData(CID, TABLE_ITEM, pGS->GetPlayer(CID)->m_AccData);
		pGS->Server()->Kick(CID, "服务器出现错误！请联系开发者QQ:1562151175！感谢！");
		return true;
	}

	pGS->CreateSoundGlobal(SOUND_CTF_RETURN, CID);
	pGS->TW()->Account()->SaveAccountData(CID, TABLE_ITEM, pGS->GetPlayer(CID)->m_AccData);
	pGS->ClearVotes(CID);
	return true;
}

bool CCommandProcessor::VotSetupTurret(IConsole::IResult *pResult, void *pUserData)
{
	const int ClientID = pResult->GetClientID();
	CGameContext *pGS = GetCommandResultGameServer(ClientID, pUserData);
	if (pGS->GetPlayer(ClientID))
		pGS->GetPlayer(ClientID)->CreateTurret();
	pGS->ClearVotes(ClientID);
	return true;
}

bool CCommandProcessor::VotTravel(IConsole::IResult *pResult, void *pUserData)
{
	const int ClientID = pResult->GetClientID();
	CGameContext *pGS = GetCommandResultGameServer(ClientID, pUserData);
	pGS->Server()->ChangeWorld(ClientID, pResult->GetInteger(0));
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