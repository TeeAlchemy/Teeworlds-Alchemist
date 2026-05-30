#include <engine/shared/config.h>
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include <game/server/gamecontroller.h>
#include "character.h"
#include "door_switch.h"

CDoorSwitch::CDoorSwitch(CGameWorld *pGameWorld, vec2 Pos, CDoor *pDoor, bool Lights)
	: CEntity(pGameWorld, CGameWorld::ENTTYPE_LASER, Pos)
{
	m_pDoor = pDoor;
	m_ID1 = Server()->SnapNewID();
	m_ID2 = Server()->SnapNewID();
	m_Lights = Lights;
	GameWorld()->InsertEntity(this);
}

void CDoorSwitch::Tick()
{
	CCharacter *pChar = GameWorld()->ClosestCharacter(m_Pos, 20, 0x0);
	if (!pChar || !pChar->GetPlayer())
		return;

	GameServer()->m_pController->SwitchDoor(m_pDoor, pChar->GetPlayer(), m_Pos, !m_Lights);
}

void CDoorSwitch::Snap(int SnappingClient)
{
	if (NetworkClipped(SnappingClient) || !m_Lights)
		return;

	if (m_pDoor->m_TurnedOn)
	{
		vec2 Direction = normalize(vec2(0.f, -1.f));
		vec2 TmpDir;
		float Angle = 0;

		for (int i = 0; i < 3; i++)
		{
			if (i != 0)
			{
				Angle = 120 * (3.14159265f / 180.0f);
				TmpDir.x = (Direction.x * cos(Angle)) - (Direction.y * sin(Angle));
				TmpDir.y = (Direction.x * sin(Angle)) + (Direction.y * cos(Angle));
				Direction = TmpDir;
			}

			vec2 To = m_Pos + Direction * 20;

			int ID = GetID();
			switch (i)
			{
			case 1: ID = m_ID1; break;
			case 2: ID = m_ID2; break;
			default: break;
			}

			CNetObj_Laser *pObj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, ID, sizeof(CNetObj_Laser)));
			if (!pObj)
				continue;
			pObj->m_X = (int)To.x;
			pObj->m_Y = (int)To.y;
			pObj->m_FromX = (int)m_Pos.x;
			pObj->m_FromY = (int)m_Pos.y;
			pObj->m_StartTick = Server()->Tick() - 4;
		}
	}

	if (!m_pDoor->m_TurnedOn)
	{
		vec2 Direction = normalize(vec2(0.f, -1.f));
		vec2 TmpDir;
		float Angle = 0;

		vec2 To[3];
		for (int i = 0; i < 3; i++)
		{
			if (i != 0)
			{
				Angle = 120 * (3.14159265f / 180.0f);
				TmpDir.x = (Direction.x * cos(Angle)) - (Direction.y * sin(Angle));
				TmpDir.y = (Direction.x * sin(Angle)) + (Direction.y * cos(Angle));
				Direction = TmpDir;
			}

			To[i] = m_Pos + Direction * 20;
		}

		for (int i = 0; i < 3; i++)
		{
			int ID = GetID();
			switch (i)
			{
			case 1: ID = m_ID1; break;
			case 2: ID = m_ID2; break;
			default: break;
			}

			CNetObj_Laser *pObj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, ID, sizeof(CNetObj_Laser)));
			if (!pObj)
				continue;
			if (i == 2)
			{
				pObj->m_X = (int)To[0].x;
				pObj->m_Y = (int)To[0].y;
			}
			else
			{
				pObj->m_X = (int)To[i + 1].x;
				pObj->m_Y = (int)To[i + 1].y;
			}
			pObj->m_FromX = (int)To[i].x;
			pObj->m_FromY = (int)To[i].y;
			pObj->m_StartTick = Server()->Tick() - 4;
		}
	}
}
