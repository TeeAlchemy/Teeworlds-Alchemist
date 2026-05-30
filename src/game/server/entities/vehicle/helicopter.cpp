#include <game/server/gamecontext.h>
#include <game/server/battle.h>
#include <game/server/entities/character.h>

#include "helicopter.h"
#include "vehicle_util.h"
#include "vehicle_visual.h"
#include "vehicle_weapon.h"

CHelicopter::CHelicopter(CGameWorld *pGameWorld, vec2 Pos, int Team)
	: CVehicle(pGameWorld, CGameWorld::ENTTYPE_HELICOPTER, Pos, Team, 60)
{
	m_GrenadeReload = 0;
	m_MissileReload = 0;
	m_PrevDriverFire = 0;
	m_PrevDriverJump = 0;
	m_MainRotorAngle = 0.f;
	m_TailRotorAngle = 0.f;

	for (int i = 0; i < HELICOPTER_PART_IDS; i++)
		m_PartIds[i] = Server()->SnapNewID();
	for (int i = 0; i < HELICOPTER_LASER_IDS; i++)
		m_LaserIds[i] = Server()->SnapNewID();
	GameWorld()->InsertEntity(this);
}

CHelicopter::~CHelicopter()
{
	for (int i = 0; i < HELICOPTER_PART_IDS; i++)
		Server()->SnapFreeID(m_PartIds[i]);
	for (int i = 0; i < HELICOPTER_LASER_IDS; i++)
		Server()->SnapFreeID(m_LaserIds[i]);
}

void CHelicopter::TickRotors(bool Ascending)
{
	if (Ascending)
	{
		m_MainRotorAngle += 2.f * pi / 10.f;
		m_TailRotorAngle += 2.f * pi / 5.f;
	}
	else
	{
		m_MainRotorAngle -= 2.f * pi / 48.f;
		m_TailRotorAngle -= 2.f * pi / 24.f;
	}
}

void CHelicopter::TickDriver(CCharacter *pDriver)
{
	const bool Ascending = (pDriver->GetCore()->m_Input.m_Hook & 1) != 0;
	const bool OnGround = VehicleIsOnGround(GameServer(), m_Pos, CollisionSize());

	if (!OnGround)
		ApplyHorizontalInput(pDriver, HELICOPTER_MAX_SPEED, HELICOPTER_ACCEL, 0.75f);
	else
	{
		m_Vel.x *= 0.4f;
		if (fabsf(m_Vel.x) < 0.25f)
			m_Vel.x = 0.f;
	}

	VehicleApplyFlyingVertical(GameServer(), m_Vel, pDriver, HELICOPTER_MAX_SPEED, HELICOPTER_ACCEL);
	if (!OnGround)
		VehicleApplyFriction(m_Vel, 0.97f);
	TickRotors(Ascending);
	MoveBox(CollisionSize());
}

void CHelicopter::TickDriverExtras(CCharacter *pDriver)
{
	(void)pDriver;
	if (BattleIsEnabled())
	{
		VehicleTickHelicopterBattleDriver(GameServer(), GameWorld(), m_Driver, m_Pos, m_Team,
			m_GrenadeReload, m_MissileReload, m_PrevDriverFire, m_PrevDriverJump);
		return;
	}

	VehicleTickHelicopterDriverWeapons(GameServer(), GameWorld(), m_Driver, m_Pos, m_Team,
		m_GrenadeReload, m_MissileReload, m_PrevDriverFire, m_PrevDriverJump);
}

void CHelicopter::TickIdle()
{
	VehicleApplyGravity(GameServer(), m_Vel);
	if (VehicleIsOnGround(GameServer(), m_Pos, CollisionSize()))
		m_Vel.x = 0.f;
	else
		VehicleApplyFriction(m_Vel, 0.9f);
	MoveBox(CollisionSize());
	TickRotors(false);
}

void CHelicopter::Snap(int SnappingClient)
{
	if (NetworkClipped(SnappingClient))
		return;

	vec2 RotorHub = vec2(m_Pos.x, m_Pos.y - VehicleScale(44.f));
	if (CCharacter *pDriver = DriverChar())
	{
		float HeadTopY = pDriver->GetCore()->m_Pos.y - 20.f;
		if (m_Gunner >= 0)
		{
			if (CCharacter *pGunner = GameServer()->GetPlayerChar(m_Gunner))
				HeadTopY = minimum(HeadTopY, pGunner->GetCore()->m_Pos.y - 20.f);
		}
		RotorHub = vec2(m_Pos.x, HeadTopY - VehicleScale(16.f));
	}

	VehicleVisual::SnapHelicopter(Server(), GetID(), m_PartIds, HELICOPTER_PART_IDS, m_LaserIds, HELICOPTER_LASER_IDS, m_Pos, RotorHub, m_Vel, m_MainRotorAngle, m_TailRotorAngle, Server()->Tick());
	SnapHealthBar(SnappingClient);
}
