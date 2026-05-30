#include <game/server/gamecontext.h>
#include <game/server/battle.h>
#include <game/server/entities/character.h>

#include "tank.h"
#include "vehicle_util.h"
#include "vehicle_visual.h"
#include "vehicle_weapon.h"

CTank::CTank(CGameWorld *pGameWorld, vec2 Pos, int Team)
	: CVehicle(pGameWorld, CGameWorld::ENTTYPE_TANK, Pos, Team, 100)
{
	m_BarrelDir = vec2(1.f, 0.f);
	m_CannonReload = 0;
	m_PrevDriverFire = 0;

	for (int i = 0; i < TANK_PART_IDS; i++)
		m_PartIds[i] = Server()->SnapNewID();
	for (int i = 0; i < TANK_LASER_IDS; i++)
		m_LaserIds[i] = Server()->SnapNewID();
	m_BarrelId = Server()->SnapNewID();
	GameWorld()->InsertEntity(this);
}

CTank::~CTank()
{
	for (int i = 0; i < TANK_PART_IDS; i++)
		Server()->SnapFreeID(m_PartIds[i]);
	for (int i = 0; i < TANK_LASER_IDS; i++)
		Server()->SnapFreeID(m_LaserIds[i]);
	Server()->SnapFreeID(m_BarrelId);
}

void CTank::TickDriver(CCharacter *pDriver)
{
	if (m_Gunner < 0)
	{
		if (pDriver->GetCore()->m_Direction < 0)
			m_BarrelDir = vec2(-1.f, 0.f);
		else if (pDriver->GetCore()->m_Direction > 0)
			m_BarrelDir = vec2(1.f, 0.f);
	}

	ApplyHorizontalInput(pDriver, TANK_MAX_SPEED, TANK_ACCEL, 0.82f);
	VehicleApplyGravity(GameServer(), m_Vel);
	MoveBox(CollisionSize());
}

void CTank::TickDriverExtras(CCharacter *pDriver)
{
	(void)pDriver;
	if (!BattleIsEnabled())
		return;

	vec2 Dir = m_BarrelDir;
	if (CCharacter *pDriverChar = DriverChar())
	{
		vec2 Aim = normalize(vec2(pDriverChar->GetCore()->m_Input.m_TargetX, pDriverChar->GetCore()->m_Input.m_TargetY));
		if (length(Aim) > 1e-3f)
			Dir = Aim;
	}

	VehicleTickTankBattleDriver(GameServer(), GameWorld(), m_Driver, GetBarrelTip(Dir), m_Team, m_CannonReload, m_PrevDriverFire);
}

void CTank::TickGunner(CCharacter *pGunner)
{
	vec2 Dir = normalize(vec2(pGunner->GetCore()->m_Input.m_TargetX, pGunner->GetCore()->m_Input.m_TargetY));
	if (length(Dir) < 1e-3f)
		Dir = vec2(1.f, 0.f);
	m_BarrelDir = Dir;
	CVehicle::TickGunner(pGunner);
}

vec2 CTank::GunnerFirePos(CCharacter *pGunner) const
{
	(void)pGunner;
	vec2 Dir = m_BarrelDir;
	if (length(Dir) < 1e-3f)
		Dir = vec2(1.f, 0.f);
	return GetBarrelTip(Dir);
}

void CTank::TickIdle()
{
	VehicleApplyGravity(GameServer(), m_Vel);
	m_Vel.x *= 0.9f;
	MoveBox(CollisionSize());
}

void CTank::OnDriverBoarded(CCharacter *pDriver)
{
	(void)pDriver;
	m_Pos.y -= VehicleScale(8.f);
}

void CTank::Snap(int SnappingClient)
{
	if (NetworkClipped(SnappingClient))
		return;

	VehicleVisual::SnapTank(Server(), GetID(), m_PartIds, TANK_PART_IDS, m_LaserIds, TANK_LASER_IDS, m_BarrelId, m_Pos, m_BarrelDir, Server()->Tick());
	SnapHealthBar(SnappingClient);
}
