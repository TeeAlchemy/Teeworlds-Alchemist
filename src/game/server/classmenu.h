#ifndef GAME_SERVER_CLASSMENU_H
#define GAME_SERVER_CLASSMENU_H

#include <base/tl/array.h>
#include <string>

class CGameContext;
class CPlayer;

class CClassMenu
{
public:
	static constexpr int CLASSMENU_MOTD_MAX = 900;
	static constexpr int CLASSMENU_VISIBLE_ACTIONS = 9;

	struct SEntry
	{
		char m_aLabel[128];
		char m_aCommand[256];
	};

	void Populate(CGameContext *pGameServer, int ClientID);
	void SendMotd(CGameContext *pGameServer, int ClientID, int SelectedAction);
	bool ExecuteAction(CGameContext *pGameServer, CPlayer *pPlayer, int ActionIndex);
	int NumActions() const;

private:
	array<SEntry> m_aEntries;
	std::string m_MotdMessage;

	void Clear();
	void AddInfo(const char *pLabel);
	void AddAction(const char *pCmd, const char *pLabel);
	void AppendMotdLine(std::string &Motd, const char *pLine) const;
};

#endif
