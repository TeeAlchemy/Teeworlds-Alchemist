#include <game/server/gamecontext.h>

#include "flag.h"

CFlag::CFlag(CGameWorld *pGameWorld, int Team, vec2 StandPos)
	: CEntity(pGameWorld, CGameWorld::ENTTYPE_FLAG, StandPos, ms_PhysSize)
{
	m_Team = Team;
	m_StandPos = StandPos;
	m_pCarryingCharacter = nullptr;
	m_GrabTick = 0;
	Reset();
	GameWorld()->InsertEntity(this);
}

void CFlag::Reset()
{
	m_pCarryingCharacter = nullptr;
	m_AtStand = 1;
	m_Pos = m_StandPos;
	m_Vel = vec2(0.f, 0.f);
	m_GrabTick = 0;
	m_DropTick = 0;
}

void CFlag::Drop()
{
	if (!m_pCarryingCharacter)
		return;

	m_pCarryingCharacter = nullptr;
	m_AtStand = 0;
	m_DropTick = Server()->Tick();
	GameServer()->CreateSound(m_Pos, SOUND_CTF_DROP);
}

void CFlag::ReturnToStand()
{
	m_pCarryingCharacter = nullptr;
	m_AtStand = 1;
	m_Pos = m_StandPos;
	m_Vel = vec2(0.f, 0.f);
	m_DropTick = 0;
	GameServer()->CreateSound(m_Pos, SOUND_CTF_RETURN);
}

void CFlag::TickPaused()
{
	++m_DropTick;
	if (m_GrabTick)
		++m_GrabTick;
}

void CFlag::Tick()
{
}

void CFlag::Snap(int SnappingClient)
{
	if (NetworkClipped(SnappingClient))
		return;

	CNetObj_Flag *pFlag = static_cast<CNetObj_Flag *>(Server()->SnapNewItem(NETOBJTYPE_FLAG, m_Team, sizeof(CNetObj_Flag)));
	if (!pFlag)
		return;

	pFlag->m_X = (int)m_Pos.x;
	pFlag->m_Y = (int)m_Pos.y;
	pFlag->m_Team = m_Team;
}
