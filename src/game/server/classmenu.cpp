#include "classmenu.h"

#include "battle.h"
#include "command_processor.h"
#include "gamecontext.h"
#include "player.h"

#include <engine/shared/config.h>

void CClassMenu::Clear()
{
	m_aEntries.clear();
}

void CClassMenu::AddInfo(const char *pLabel)
{
	SEntry Entry;
	str_copy(Entry.m_aLabel, pLabel, sizeof(Entry.m_aLabel));
	Entry.m_aCommand[0] = 0;
	m_aEntries.add(Entry);
}

void CClassMenu::AddAction(const char *pCmd, const char *pLabel)
{
	SEntry Entry;
	str_copy(Entry.m_aLabel, pLabel, sizeof(Entry.m_aLabel));
	str_copy(Entry.m_aCommand, pCmd, sizeof(Entry.m_aCommand));
	m_aEntries.add(Entry);
}

void CClassMenu::AppendMotdLine(std::string &Motd, const char *pLine) const
{
	if ((int)Motd.size() >= CLASSMENU_MOTD_MAX - 2 || !pLine)
		return;

	Motd.append(pLine);
	Motd.push_back('\n');
}

int CClassMenu::NumActions() const
{
	int Num = 0;
	for (int i = 0; i < m_aEntries.size(); i++)
	{
		if (m_aEntries[i].m_aCommand[0])
			Num++;
	}
	return Num;
}

void CClassMenu::Populate(CGameContext *pGameServer, int ClientID)
{
	CPlayer *pP = pGameServer->GetPlayer(ClientID);
	if (!pP)
		return;

	Clear();
	AddInfo("=== Battle Class ===");
	AddInfo("scroll select | fire confirm");
	AddInfo("");

	if (pP->m_BattleClass >= 0 && pP->m_BattleClass < NUM_BATTLE_CLASS)
	{
		char aCurrent[64];
		str_format(aCurrent, sizeof(aCurrent), "Current: %s", BattleClassName(pP->m_BattleClass));
		AddInfo(aCurrent);
		AddInfo("");
	}

	AddAction("ccv_battleclass 0", "1. Soldier");
	AddAction("ccv_battleclass 1", "2. Engineer");
	AddAction("ccv_battleclass 2", "3. Medic");
	AddAction("ccv_battleclass 3", "4. Sniper");
}

void CClassMenu::SendMotd(CGameContext *pGameServer, int ClientID, int SelectedAction)
{
	CPlayer *pPlayer = pGameServer->GetPlayer(ClientID);
	Populate(pGameServer, ClientID);

	const int NumActionItems = NumActions();
	if (SelectedAction < 0)
		SelectedAction = 0;
	else if (SelectedAction >= NumActionItems)
		SelectedAction = maximum(0, NumActionItems - 1);

	int WindowStart = 0;
	int WindowEnd = NumActionItems;
	if (NumActionItems > CLASSMENU_VISIBLE_ACTIONS)
	{
		WindowStart = SelectedAction - CLASSMENU_VISIBLE_ACTIONS / 2;
		if (WindowStart < 0)
			WindowStart = 0;
		WindowEnd = WindowStart + CLASSMENU_VISIBLE_ACTIONS;
		if (WindowEnd > NumActionItems)
		{
			WindowEnd = NumActionItems;
			WindowStart = WindowEnd - CLASSMENU_VISIBLE_ACTIONS;
			if (WindowStart < 0)
				WindowStart = 0;
		}
	}

	std::string Motd;
	for (int i = 0; i < m_aEntries.size(); i++)
	{
		if (m_aEntries[i].m_aCommand[0])
			continue;
		AppendMotdLine(Motd, m_aEntries[i].m_aLabel);
	}

	int Current = 0;
	for (int i = 0; i < m_aEntries.size(); i++)
	{
		if (!m_aEntries[i].m_aCommand[0])
			continue;

		if (Current >= WindowStart && Current < WindowEnd)
		{
			char aLine[160];
			str_format(aLine, sizeof(aLine), "%s%s", Current == SelectedAction ? "> " : "  ", m_aEntries[i].m_aLabel);
			AppendMotdLine(Motd, aLine);
		}
		Current++;
	}

	if ((int)Motd.size() >= CLASSMENU_MOTD_MAX)
		Motd.resize(CLASSMENU_MOTD_MAX - 1);

	m_MotdMessage = Motd;

	CNetMsg_Sv_Motd Msg;
	Msg.m_pMessage = m_MotdMessage.c_str();
	pGameServer->Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, ClientID, -1);
}

bool CClassMenu::ExecuteAction(CGameContext *pGameServer, CPlayer *pPlayer, int ActionIndex)
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
			if (str_comp_num(m_aEntries[i].m_aCommand, "ccv_", 4) == 0)
				pGameServer->CommandProcessor()->Process(m_aEntries[i].m_aCommand + 3, pPlayer, CFGFLAG_VOTE);
			return true;
		}
		Current++;
	}

	return false;
}
