#include "buildmenu.h"

#include "command_processor.h"
#include "entities/character.h"
#include "entities/buildings.h"
#include "entities/area-flag.h"
#include "battle.h"
#include "gamecontext.h"
#include "player.h"
#include "resources.h"
#include "votepage.h"

#include <base/math.h>
#include <engine/shared/config.h>
#include <game/generated/protocol.h>
#include <teeother/components/localization.h>

/*
placeholder("=== Build Menu ===");
placeholder("Press F3 to close, weapon keys 1-5 to select");
placeholder("No actions available on this page");
placeholder("☞ Announcement 📢");
placeholder("☪ Announcement");
placeholder("No announcement");
placeholder("scroll to read | hook back");
*/

void CBuildMenu::Clear()
{
	m_aEntries.clear();
}

void CBuildMenu::AddInfo(const char *pLabel)
{
	SEntry Entry;
	str_copy(Entry.m_aLabel, pLabel, sizeof(Entry.m_aLabel));
	Entry.m_aCommand[0] = 0;
	m_aEntries.add(Entry);
}

void CBuildMenu::AddAction(const char *pCmd, const char *pLabel)
{
	SEntry Entry;
	str_copy(Entry.m_aLabel, pLabel, sizeof(Entry.m_aLabel));
	str_copy(Entry.m_aCommand, pCmd, sizeof(Entry.m_aCommand));
	m_aEntries.add(Entry);
}

void CBuildMenu::AppendMotdLine(std::string &Motd, const char *pLine) const
{
	if ((int)Motd.size() >= BUILDMENU_MOTD_MAX - 2 || !pLine)
		return;

	Motd.append(pLine);
	Motd.push_back('\n');
}

int CBuildMenu::NumActions() const
{
	int Num = 0;
	for (int i = 0; i < m_aEntries.size(); i++)
	{
		if (m_aEntries[i].m_aCommand[0])
			Num++;
	}
	return Num;
}

void CBuildMenu::Populate(CGameContext *pGameServer, int ClientID)
{
	CPlayer *pP = pGameServer->GetPlayer(ClientID);
	if (!pP)
		return;

	CCharacter *pChr = pP->GetCharacter();
	if (!pChr)
		return;

	const char *pLanguage = pGameServer->GetClientLanguage(ClientID);
	CLocalization *pLocalization = pGameServer->Server()->Localization();
	CGameContext::SPlayerVote *pVote = pGameServer->GetPlayerVote(ClientID);

	Clear();

	float HpP = (float(pGameServer->m_pController->GetWorkbenchHealth(pP->GetTeam())) / float(g_Config.m_SvWorkbenchesHealth)) * 100.f;

	AddInfo(pLocalization->Format(pLanguage, "Workbench Health: {}%", HpP).c_str());
	AddInfo("");

	switch (pVote->m_Page)
	{
	case PAGE_MENU:
		AddInfo(pLocalization->Format(pLanguage, "☪ Menu").c_str());
		AddAction("ccv_goto 6", pLocalization->Format(pLanguage, "☞ Announcement 📢").c_str());
		AddAction("ccv_goto 1", pLocalization->Format(pLanguage, "☞ Inventory ✪").c_str());
		AddAction("ccv_goto 2", pLocalization->Format(pLanguage, "☞ Base ☺").c_str());
		AddAction("ccv_goto 4", pLocalization->Format(pLanguage, "☞ Free Market ㊮").c_str());
		AddAction("ccv_goto 5", pLocalization->Format(pLanguage, "☞ Build ☭").c_str());
		break;

	case PAGE_INVENTORY:
		pVote->m_LastPage = PAGE_MENU;
		AddInfo(pLocalization->Format(pLanguage, "☪ Inventory").c_str());
		AddInfo(pLocalization->Format(pLanguage, "#WARNING: Lose all after death/quit)").c_str());
		for (int i = 0; i < NUM_RESOURCE; i++)
			AddInfo(pLocalization->Format(pLanguage, "➳ {} x{}", pLocalization->Localize(pLanguage, GetResourceName(i)), pChr->m_Resource[i]).c_str());
		AddAction("ccv_goto 0", pLocalization->Format(pLanguage, "⏎ Back").c_str());
		break;

	case PAGE_HOME:
		pVote->m_LastPage = PAGE_MENU;
		AddInfo(pLocalization->Format(pLanguage, "☪ Base").c_str());
		if (g_Config.m_SvBattle)
		{
			AddAction("ccv_opclassmenu", pLocalization->Format(pLanguage, "☞ Battle Class").c_str());
			AddAction("ccv_goto 7", pLocalization->Format(pLanguage, "☞ Teleport to Checkpoint").c_str());
		}
		if (pP->m_VotePage[PAGE_HOME])
		{
			AddInfo(pLocalization->Format(pLanguage, "- You can use the workbench now").c_str());
			AddInfo("");
			AddInfo(pLocalization->Format(pLanguage, "▾ Workbench ▾").c_str());
			for (int i = 0; i < NUM_BUILDING; i++)
			{
				if (i == BUILDING_WORKBENCH)
					continue;

				char aCmd[32];
				str_format(aCmd, sizeof(aCmd), "ccv_makebuilding %d", i);
				AddAction(aCmd, pLocalization->Format(pLanguage, "☞ Make {}({})", pGameServer->m_pBuildingsInfo->m_aBuildingsInfo[i].m_aName, pGameServer->m_pController->m_aTeamBuildings[pP->GetTeam()][i]).c_str());
			}
		}
		else
		{
			AddInfo(pLocalization->Format(pLanguage, "You need to return to the base to use this!").c_str());
		}
		AddAction("ccv_goto 0", pLocalization->Format(pLanguage, "⏎ Back").c_str());
		break;

	case PAGE_MAKE:
		pVote->m_LastPage = PAGE_HOME;
		AddInfo(pLocalization->Format(pLanguage, "☪ Make").c_str());
		AddInfo(pLocalization->Format(pLanguage, "Building: {}", pGameServer->m_pBuildingsInfo->m_aBuildingsInfo[pVote->m_Select].m_aName).c_str());
		AddInfo(pLocalization->Format(pLanguage, "Description: {}", pGameServer->m_pBuildingsInfo->m_aBuildingsInfo[pVote->m_Select].m_aDesc).c_str());
		AddInfo(pLocalization->Format(pLanguage, "Your team have: {}", pGameServer->m_pController->m_aTeamBuildings[pP->GetTeam()][pVote->m_Select]).c_str());
		AddInfo(pLocalization->Format(pLanguage, "㊮ Formula:").c_str());
		for (int i = 0; i < NUM_RESOURCE; i++)
		{
			if (pGameServer->m_pBuildingsInfo->m_aBuildingsInfo[pVote->m_Select].m_Formula[i])
			{
				AddInfo(pLocalization->Format(pLanguage, "# {} {}/{}", pLocalization->Localize(pLanguage, GetResourceName(i)), pGameServer->m_pController->m_aTeamResources[pP->GetTeam()][i], pGameServer->m_pBuildingsInfo->m_aBuildingsInfo[pVote->m_Select].m_Formula[i]).c_str());
			}
		}
		{
			char aCmd[32];
			str_format(aCmd, sizeof(aCmd), "ccv_makebuilding %d", pVote->m_Select);
			AddAction(aCmd, pLocalization->Format(pLanguage, "- Make!").c_str());
		}
		AddAction("ccv_goto 2", pLocalization->Format(pLanguage, "⏎ Back").c_str());
		break;

	case PAGE_SHOP:
		pVote->m_LastPage = PAGE_MENU;
		AddInfo(pLocalization->Format(pLanguage, "☪ Free Market").c_str());
		if (pP->m_VotePage[PAGE_SHOP])
		{
			AddInfo(pLocalization->Format(pLanguage, "Free market, capitalism, without big hands").c_str());
			AddInfo(pLocalization->Format(pLanguage, "...").c_str());
			AddInfo(pLocalization->Format(pLanguage, "This place has been liberated by the Communist Party").c_str());
			AddInfo(pLocalization->Format(pLanguage, "The free market has been abolished.").c_str());
		}
		else
		{
			AddInfo(pLocalization->Format(pLanguage, "You need to be in the market area to use this!").c_str());
		}
		AddAction("ccv_goto 0", pLocalization->Format(pLanguage, "⏎ Back").c_str());
		break;

	case PAGE_BUILD:
		pVote->m_LastPage = PAGE_MENU;
		AddInfo(pLocalization->Format(pLanguage, "☪ Node").c_str());
		{
			int NumNodes = pGameServer->m_pController->m_aTeamBuildings[pP->GetTeam()][BUILDING_NODE];
			if (pChr->m_CanBuild)
			{
				AddInfo(pLocalization->Format(pLanguage, "Choose the building you want to build and hammer the ground").c_str());
				for (int i = 0; i < NUM_BUILDING; i++)
				{
					if (i == BUILDING_WORKBENCH)
						continue;

					int Num = pGameServer->m_pController->m_aTeamBuildings[pP->GetTeam()][i];
					if (Num > 0)
					{
						char aCmd[32];
						str_format(aCmd, sizeof(aCmd), "ccv_selectbuilding %d", i);
						AddAction(aCmd, pLocalization->Format(pLanguage, "☞ Build {}({})", pGameServer->m_pBuildingsInfo->m_aBuildingsInfo[i].m_aName, Num).c_str());
					}
				}
			}
			else if (NumNodes > 0)
			{
				AddInfo(pLocalization->Format(pLanguage, "Starting from building a node.").c_str());
				AddInfo(pLocalization->Format(pLanguage, "Choose 'Build Node' and hammer the ground").c_str());
				AddAction("ccv_selectbuilding 0", pLocalization->Format(pLanguage, "☞ Build {}({})", pGameServer->m_pBuildingsInfo->m_aBuildingsInfo[BUILDING_NODE].m_aName, NumNodes).c_str());
			}
			else
			{
				AddInfo(pLocalization->Format(pLanguage, "You have to make a Node first!").c_str());
				AddInfo(pLocalization->Format(pLanguage, "Tip: Back to the menu and choose 'Base'").c_str());
			}
		}
		AddAction("ccv_goto 0", pLocalization->Format(pLanguage, "⏎ Back").c_str());
		break;

	case PAGE_ANNOUNCE:
		pVote->m_LastPage = PAGE_MENU;
		AddInfo(pLocalization->Format(pLanguage, "☪ Announcement").c_str());
		AddInfo("");
		if (g_Config.m_SvMotd[0])
		{
			const char *pLine = g_Config.m_SvMotd;
			while (*pLine)
			{
				const char *pEnd = str_find(pLine, "\n");
				char aLine[256];
				if (pEnd)
				{
					const int LineLen = minimum((int)(pEnd - pLine), (int)sizeof(aLine) - 1);
					str_copy(aLine, pLine, LineLen + 1);
					AddInfo(aLine);
					pLine = pEnd + 1;
				}
				else
				{
					AddInfo(pLine);
					break;
				}
			}
		}
		else
		{
			AddInfo(pLocalization->Format(pLanguage, "No announcement").c_str());
		}
		AddAction("ccv_goto 0", pLocalization->Format(pLanguage, "⏎ Back").c_str());
		break;

	case PAGE_BATTLE_TELEPORT:
		pVote->m_LastPage = PAGE_HOME;
		AddInfo(pLocalization->Format(pLanguage, "☪ Teleport").c_str());
		{
			int Index = 0;
			for (CAreaFlag *pFlag = (CAreaFlag *)pGameServer->m_World.FindFirst(CGameWorld::ENTTYPE_AREA_FLAG); pFlag; pFlag = (CAreaFlag *)pFlag->TypeNext())
			{
				if (pFlag->GetTeam() != pP->GetTeam())
					continue;
				char aCmd[32];
				char aLabel[64];
				str_format(aLabel, sizeof(aLabel), "Checkpoint %d", Index + 1);
				str_format(aCmd, sizeof(aCmd), "ccv_battleteleport %d", Index);
				AddAction(aCmd, aLabel);
				Index++;
			}
			if (Index == 0)
				AddInfo(pLocalization->Format(pLanguage, "No captured checkpoints").c_str());
		}
		AddAction("ccv_goto 2", pLocalization->Format(pLanguage, "⏎ Back").c_str());
		break;

	default:
		break;
	}
}

int CBuildMenu::NumInfoLines() const
{
	int Num = 0;
	for (int i = 0; i < m_aEntries.size(); i++)
	{
		if (!m_aEntries[i].m_aCommand[0])
			Num++;
	}
	return Num;
}

void CBuildMenu::SendMotd(CGameContext *pGameServer, int ClientID, int SelectedAction)
{
	const char *pLanguage = pGameServer->GetClientLanguage(ClientID);
	CLocalization *pLocalization = pGameServer->Server()->Localization();
	CPlayer *pPlayer = pGameServer->GetPlayer(ClientID);
	const CGameContext::SPlayerVote *pVote = pGameServer->GetPlayerVote(ClientID);
	const bool AnnouncePage = pVote->m_Page == PAGE_ANNOUNCE;

	Populate(pGameServer, ClientID);

	const int NumInfoItems = NumInfoLines();
	const int NumActionItems = NumActions();
	int TextScroll = pPlayer ? pPlayer->m_BuildMenuTextScroll : 0;
	if (TextScroll < 0)
		TextScroll = 0;
	else if (TextScroll > maximum(0, NumInfoItems - 1))
		TextScroll = maximum(0, NumInfoItems - BUILDMENU_VISIBLE_INFO);
	if (SelectedAction < 0)
		SelectedAction = 0;
	else if (SelectedAction >= NumActionItems)
		SelectedAction = maximum(0, NumActionItems - 1);

	int WindowStart = 0;
	int WindowEnd = NumActionItems;
	if (NumActionItems > BUILDMENU_VISIBLE_ACTIONS)
	{
		WindowStart = SelectedAction - BUILDMENU_VISIBLE_ACTIONS / 2;
		if (WindowStart < 0)
			WindowStart = 0;
		WindowEnd = WindowStart + BUILDMENU_VISIBLE_ACTIONS;
		if (WindowEnd > NumActionItems)
		{
			WindowEnd = NumActionItems;
			WindowStart = WindowEnd - BUILDMENU_VISIBLE_ACTIONS;
			if (WindowStart < 0)
				WindowStart = 0;
		}
	}

	const int InfoWindowEnd = NumInfoItems > BUILDMENU_VISIBLE_INFO ? TextScroll + BUILDMENU_VISIBLE_INFO : NumInfoItems;

	std::string Motd;
	AppendMotdLine(Motd, pLocalization->Format(pLanguage, "=== Build Menu ===").c_str());
	if (AnnouncePage)
		AppendMotdLine(Motd, pLocalization->Format(pLanguage, "scroll to read | hook back").c_str());
	else
		AppendMotdLine(Motd, pLocalization->Format(pLanguage, "F3 close | scroll select | fire confirm | hook back").c_str());
	if (NumActionItems > BUILDMENU_VISIBLE_ACTIONS)
	{
		char aPage[32];
		str_format(aPage, sizeof(aPage), "[%d/%d]", SelectedAction + 1, NumActionItems);
		AppendMotdLine(Motd, aPage);
	}
	else if (AnnouncePage && NumInfoItems > BUILDMENU_VISIBLE_INFO)
	{
		const int MaxScroll = maximum(0, NumInfoItems - BUILDMENU_VISIBLE_INFO);
		char aPage[32];
		str_format(aPage, sizeof(aPage), "[%d/%d]", TextScroll + 1, MaxScroll + 1);
		AppendMotdLine(Motd, aPage);
	}
	AppendMotdLine(Motd, "");

	if (TextScroll > 0)
		AppendMotdLine(Motd, "  ...");

	int InfoIndex = 0;
	for (int i = 0; i < m_aEntries.size(); i++)
	{
		if (m_aEntries[i].m_aCommand[0])
			continue;

		if (InfoIndex >= TextScroll && InfoIndex < InfoWindowEnd)
			AppendMotdLine(Motd, m_aEntries[i].m_aLabel);
		InfoIndex++;
	}

	if (InfoWindowEnd < NumInfoItems)
		AppendMotdLine(Motd, "  ...");

	if (NumActionItems > 0)
		AppendMotdLine(Motd, "");

	if (WindowStart > 0)
		AppendMotdLine(Motd, "  ...");

	int ActionIndex = 0;
	for (int i = 0; i < m_aEntries.size(); i++)
	{
		if (!m_aEntries[i].m_aCommand[0])
			continue;

		if (ActionIndex >= WindowStart && ActionIndex < WindowEnd)
		{
			char aLine[192];
			if (ActionIndex == SelectedAction)
				str_format(aLine, sizeof(aLine), "> %s", m_aEntries[i].m_aLabel);
			else
				str_format(aLine, sizeof(aLine), "  %s", m_aEntries[i].m_aLabel);
			AppendMotdLine(Motd, aLine);
		}
		ActionIndex++;
	}

	if (WindowEnd < NumActionItems)
		AppendMotdLine(Motd, "  ...");

	if (NumActionItems == 0)
		AppendMotdLine(Motd, pLocalization->Format(pLanguage, "No actions available on this page").c_str());

	if ((int)Motd.size() >= BUILDMENU_MOTD_MAX)
		Motd.resize(BUILDMENU_MOTD_MAX - 1);

	m_MotdMessage = std::move(Motd);
	CNetMsg_Sv_Motd Msg;
	Msg.m_pMessage = m_MotdMessage.c_str();
	pGameServer->Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, ClientID, -1);
}

bool CBuildMenu::ExecuteAction(CGameContext *pGameServer, CPlayer *pPlayer, int ActionIndex)
{
	if (ActionIndex < 0 || ActionIndex >= NumActions())
		return false;

	int Current = 0;
	for (int i = 0; i < m_aEntries.size(); i++)
	{
		if (!m_aEntries[i].m_aCommand[0])
			continue;

		if (Current == ActionIndex)
		{
			if (str_comp(m_aEntries[i].m_aCommand, "ccv_null") == 0)
				return true;

			if (str_comp_num(m_aEntries[i].m_aCommand, "ccv_", 4) == 0)
				pGameServer->CommandProcessor()->Process(m_aEntries[i].m_aCommand + 3, pPlayer, CFGFLAG_VOTE);
			return true;
		}
		Current++;
	}

	return false;
}
