#include <game/server/gamecontext.h>
#include "door.h"
#include "entities/door_laser.h"
#include "entities/door_switch.h"

CDoor::CDoor(CGameContext *pGameServer, int Team, int SwitchNum, array<CDoor::CDoorNode> lNodes, array<CDoor::CDoorNode> lSwitch)
{
	m_pGameServer = pGameServer;
	m_Team = Team;
	m_SwitchNum = SwitchNum;
	m_lNodes = lNodes;
	m_lSwitch = lSwitch;
	m_TurnedOn = true;
	m_SwitchTick = 0;
}

void CDoor::Reset()
{
	m_TurnedOn = true;
	m_SwitchTick = 0;
}

void CDoor::Init()
{
	for (int i = 0; i < m_lNodes.size(); i++)
		new CDoorLaser(&GameServer()->m_World, m_lNodes[0].m_Pos, m_lNodes[i].m_Pos, this);

	bool Lights = true;
	if (GameServer()->m_pController->m_Switches[m_SwitchNum - 1])
		Lights = false;

	for (int i = 0; i < m_lSwitch.size(); i++)
		new CDoorSwitch(&GameServer()->m_World, m_lSwitch[i].m_Pos, this, Lights);

	GameServer()->m_pController->m_Switches[m_SwitchNum - 1] = true;
}

bool CDoor::Switch(vec2 Pos, bool Silent)
{
	m_TurnedOn ^= 1;

	if (!Silent)
		GameServer()->CreateSound(Pos, SOUND_WEAPON_NOAMMO);

	return true;
}
