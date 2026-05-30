#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include "character.h"
#include "door_laser.h"

CDoorLaser::CDoorLaser(CGameWorld *pGameWorld, vec2 From, vec2 To, CDoor *pDoor)
	: CEntity(pGameWorld, CGameWorld::ENTTYPE_LASER, To)
{
	m_pDoor = pDoor;
	m_From = From;
	GameWorld()->InsertEntity(this);
}

void CDoorLaser::HitCharacter(vec2 From, vec2 To)
{
	vec2 At;
	CCharacter *Hit[MAX_CLIENTS] = {0};

	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		Hit[i] = GameWorld()->LaserIntersectCharacter(From, To, 2.5f, At, Hit);
		if (Hit[i])
		{
			Hit[i]->m_HittingDoor = true;
			Hit[i]->m_PushDirection = normalize(Hit[i]->m_OldPos - At);
		}
	}
}

void CDoorLaser::Tick()
{
	if (m_pDoor->m_TurnedOn)
		HitCharacter(m_From, m_Pos);
}

void CDoorLaser::Snap(int SnappingClient)
{
	if (NetworkClipped(SnappingClient) && distance(m_From, m_Pos) < 1600.0f &&
		(!GameServer()->m_apPlayers[SnappingClient] || distance(m_From, GameServer()->m_apPlayers[SnappingClient]->m_ViewPos) > 800.0f))
		return;

	if (!m_pDoor->m_TurnedOn)
		return;

	CNetObj_Laser *pObj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, GetID(), sizeof(CNetObj_Laser)));
	if (!pObj)
		return;
	pObj->m_X = (int)m_Pos.x;
	pObj->m_Y = (int)m_Pos.y;
	pObj->m_FromX = (int)m_From.x;
	pObj->m_FromY = (int)m_From.y;
	if (!GameServer()->m_apPlayers[SnappingClient] || GameServer()->m_apPlayers[SnappingClient]->GetTeam() == m_pDoor->m_Team || m_pDoor->m_Team == -1)
		pObj->m_StartTick = Server()->Tick() - 3;
	else
		pObj->m_StartTick = Server()->Tick();
}
