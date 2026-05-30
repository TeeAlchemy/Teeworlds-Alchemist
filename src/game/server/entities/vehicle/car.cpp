#include <game/server/gamecontext.h>
#include <game/server/battle.h>
#include <game/server/entities/character.h>

#include "car.h"
#include "vehicle_visual.h"
#include "vehicle_weapon.h"

CCar::CCar(CGameWorld *pGameWorld, vec2 Pos, int Team)
	: CVehicle(pGameWorld, CGameWorld::ENTTYPE_CAR, Pos, Team, 50)
{
	m_GunReload = 0;
	m_MgReload = 0;
	m_PrevDriverFire = 0;
	m_PrevDriverHook = 0;
	for (int i = 0; i < CAR_PART_IDS; i++)
		m_PartIds[i] = Server()->SnapNewID();
	for (int i = 0; i < CAR_LASER_IDS; i++)
		m_LaserIds[i] = Server()->SnapNewID();
	GameWorld()->InsertEntity(this);
}

CCar::~CCar()
{
	for (int i = 0; i < CAR_PART_IDS; i++)
		Server()->SnapFreeID(m_PartIds[i]);
	for (int i = 0; i < CAR_LASER_IDS; i++)
		Server()->SnapFreeID(m_LaserIds[i]);
}

void CCar::TickDriver(CCharacter *pDriver)
{
	ApplyHorizontalInput(pDriver, CAR_MAX_SPEED, CAR_ACCEL, 0.85f);
	VehicleApplyGravity(GameServer(), m_Vel);
	MoveBox(CollisionSize());
}

void CCar::TickDriverExtras(CCharacter *pDriver)
{
	(void)pDriver;
	if (!BattleIsEnabled())
		return;

	VehicleTickCarBattleDriver(GameServer(), GameWorld(), m_Driver, m_Pos, m_Team,
		m_GunReload, m_MgReload, m_PrevDriverFire, m_PrevDriverHook);
}

void CCar::TickIdle()
{
	VehicleApplyGravity(GameServer(), m_Vel);
	m_Vel.x *= 0.9f;
	MoveBox(CollisionSize());
}

void CCar::OnDriverBoarded(CCharacter *pDriver)
{
	(void)pDriver;
	m_Pos.y -= 16.f;
}

void CCar::Snap(int SnappingClient)
{
	if (NetworkClipped(SnappingClient))
		return;

	VehicleVisual::SnapCar(Server(), GetID(), m_PartIds, CAR_PART_IDS, m_LaserIds, m_Pos, Server()->Tick());
	SnapHealthBar(SnappingClient);
}
