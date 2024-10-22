/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/server/gamecontext.h>
#include "flag.h"

CFlag::CFlag(CGameWorld *pGameWorld, int Team, int MapID)
	: CEntity(pGameWorld, CGameWorld::ENTTYPE_FLAG, MapID)
{
	m_Team = Team;
	m_ProximityRadius = ms_PhysSize;
	m_pCarryingCharacter = NULL;
	m_GrabTick = 0;

	Reset();
}

void CFlag::Reset()
{
	m_pCarryingCharacter = NULL;
	m_AtStand = 1;
	m_Pos = m_StandPos;
	m_Vel = vec2(0, 0);
	m_GrabTick = 0;
}

void CFlag::TickPaused()
{
	++m_DropTick;
	if (m_GrabTick)
		++m_GrabTick;
}

void CFlag::Snap(int SnappingClient)
{
	if (GameServer()->Server()->ClientMapID(SnappingClient) != GetMapID())
		return;

	if (NetworkClipped(SnappingClient))
		return;

	CNetObj_Flag *pFlag = Server()->SnapNewItem<CNetObj_Flag>(m_Team);
	if (!pFlag)
		return;

	pFlag->m_X = (int)m_Pos.x;
	pFlag->m_Y = (int)m_Pos.y;
	pFlag->m_Team = m_Team;
}
