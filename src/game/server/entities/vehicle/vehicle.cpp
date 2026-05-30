#include "vehicle.h"

#include "vehicle_util.h"
#include "vehicle_visual.h"

#include <game/server/battle.h>
#include <game/server/entities/character.h>
#include <game/server/gamecontext.h>
#include <game/server/player.h>

CVehicle::CVehicle(CGameWorld *pGameWorld, int ObjType, vec2 Pos, int Team, int Health)
	: CEntity(pGameWorld, ObjType, Pos)
{
	m_Health = Health;
	m_MaxHealth = Health;
	m_Team = Team;
	m_Driver = -1;
	m_Gunner = -1;
	m_Vel = vec2(0.f, 0.f);
	m_SpawnPos = Pos;
	m_Locked = BattleIsEnabled() && Team >= 0;
	m_Unlocked = false;
	m_IdleTicks = 0;
	m_GunnerReload = 0;
	m_PrevGunnerFire = 0;
	for (int i = 0; i < 3; i++)
		m_aHealthBarIds[i] = Server()->SnapNewID();
}

CVehicle::~CVehicle()
{
	for (int i = 0; i < 3; i++)
		Server()->SnapFreeID(m_aHealthBarIds[i]);
	ForceDriverLeave();
	ForceGunnerLeave();
}

CCharacter *CVehicle::DriverChar() const
{
	if (m_Driver < 0)
		return nullptr;
	return GameServer()->GetPlayerChar(m_Driver);
}

void CVehicle::ApplyHorizontalInput(CCharacter *pDriver, int MaxSpeed, int Accel, float IdleDecay)
{
	if (!pDriver)
		return;

	if (pDriver->GetCore()->m_Direction < 0)
		m_Vel.x = SaturatedAdd(-(float)MaxSpeed, (float)MaxSpeed, m_Vel.x, -(float)Accel);
	else if (pDriver->GetCore()->m_Direction > 0)
		m_Vel.x = SaturatedAdd(-(float)MaxSpeed, (float)MaxSpeed, m_Vel.x, (float)Accel);
	else
		m_Vel.x *= IdleDecay;
}

void CVehicle::MoveBox(vec2 Size)
{
	vec2 NewPos = m_Pos;
	GameServer()->Collision()->MoveBox(&NewPos, &m_Vel, Size, 0.f, MaxStepHeight());
	m_Pos = NewPos;
}

vec2 CVehicle::GunnerFirePos(CCharacter *pGunner) const
{
	if (!pGunner)
		return m_Pos;
	return pGunner->GetCore()->m_Pos;
}

bool CVehicle::CanBoard(int Team) const
{
	if (Team != m_Team)
	{
		if (m_Locked && !m_Unlocked)
			return false;
	}
	return true;
}

void CVehicle::TakeDamage(int Amount, int From)
{
	(void)From;
	if (Amount <= 0 || IsMarkedForDestroy())
		return;

	m_Health -= Amount;
	if (m_Health <= 0)
		Explode();
}

void CVehicle::Repair(int Amount)
{
	if (Amount <= 0)
		return;
	m_Health = minimum(m_MaxHealth, m_Health + Amount);
}

void CVehicle::SpawnShrapnel() const
{
	VehicleSpawnShrapnel(GameServer(), GameWorld(), m_Pos, m_Driver >= 0 ? m_Driver : -1, m_Team);
}

void CVehicle::SnapHealthBar(int SnappingClient) const
{
	if (NetworkClipped(SnappingClient) || IsMarkedForDestroy())
		return;

	const int Segments = 3;
	const int Filled = maximum(0, minimum(Segments, (m_Health * Segments + m_MaxHealth - 1) / maximum(1, m_MaxHealth)));
	for (int i = 0; i < Segments; i++)
	{
		if (i >= Filled)
			break;
		VehicleVisual::SnapPickup(Server(), m_aHealthBarIds[i],
			vec2(m_Pos.x + (i - 1) * VehicleScale(10.f), m_Pos.y - VehicleScale(38.f)), POWERUP_HEALTH);
	}
}

void CVehicle::Explode()
{
	if (IsMarkedForDestroy())
		return;

	const int Owner = m_Driver >= 0 ? m_Driver : -1;
	const vec2 ExplosionPos = m_Pos;
	const int Team = m_Team;

	m_Health = 0;
	MarkForDestroy();

	GameServer()->CreateExplosion(ExplosionPos, Owner, WEAPON_GRENADE, false);
	VehicleSpawnShrapnel(GameServer(), GameWorld(), ExplosionPos, Owner, Team);
	ForceDriverLeave();
	ForceGunnerLeave();
}

void CVehicle::OnDriverBoarded(CCharacter *pDriver)
{
	(void)pDriver;
	m_Unlocked = true;
	m_Locked = false;
	m_IdleTicks = 0;
}

void CVehicle::TickGunner(CCharacter *pGunner)
{
	if (!HasGunnerSeat() || !pGunner)
		return;

	VehicleTickGunnerWeapon(GameServer(), GameWorld(), m_Gunner, GunnerFirePos(pGunner), GunnerWeapon(),
		m_GunnerReload, m_PrevGunnerFire, m_Team, GunnerProjSpawnOffset());
}

void CVehicle::Tick()
{
	if (IsMarkedForDestroy())
		return;

	if (m_Driver >= 0)
	{
		m_IdleTicks = 0;
		CCharacter *pDriver = DriverChar();
		if (!pDriver || pDriver->GetPlayer()->GetTeam() != m_Team)
		{
			ForceDriverLeave();
			return;
		}

		TickDriver(pDriver);
		VehicleSyncCharacter(pDriver, m_Pos, m_Vel, DriverOffsetY());
		pDriver->m_VehicleSeat = VEHICLE_SEAT_DRIVER;
		TickDriverExtras(pDriver);

		if (m_Gunner >= 0)
		{
			if (CCharacter *pGunner = GameServer()->GetPlayerChar(m_Gunner))
			{
				VehicleSyncGunnerCharacter(pGunner, m_Pos, m_Vel, DriverOffsetY());
				pGunner->m_VehicleSeat = VEHICLE_SEAT_GUNNER;
				TickGunner(pGunner);
			}
			else
				m_Gunner = -1;
		}
	}
	else
	{
		TickIdle();
		if (BattleIsEnabled())
		{
			m_IdleTicks++;
			if (m_IdleTicks > Server()->TickSpeed() * 30)
			{
				GameWorld()->DestroyEntity(this);
				return;
			}
		}
		if (VehicleTryAutoBoard(GameWorld(), this, m_Driver, m_Team, BoardRadius()))
		{
			if (CCharacter *pDriver = DriverChar())
			{
				pDriver->m_VehicleSeat = VEHICLE_SEAT_DRIVER;
				OnDriverBoarded(pDriver);
			}
		}
	}
}

void CVehicle::Reset()
{
	GameWorld()->DestroyEntity(this);
}

void CVehicle::ForceDriverLeave()
{
	if (m_Driver < 0)
		return;

	if (HasGunnerSeat())
	{
		VehicleDriverLeave(GameServer(), m_Driver, m_Gunner, m_Vel);
		return;
	}

	if (CCharacter *pDriver = DriverChar())
		VehicleDismount(m_Driver, m_Vel, pDriver);
	else
		m_Driver = -1;
}

void CVehicle::ForceGunnerLeave()
{
	if (m_Gunner < 0)
		return;

	if (CCharacter *pGunner = GameServer()->GetPlayerChar(m_Gunner))
		VehicleClearOccupant(pGunner);
	m_Gunner = -1;
}

bool CVehicle::IsOccupiedBy(int ClientId) const
{
	return m_Driver == ClientId || m_Gunner == ClientId;
}

void CVehicle::HandleOccupantDismount(int ClientId)
{
	if (m_Gunner == ClientId)
		ForceGunnerLeave();
	else if (m_Driver == ClientId)
		ForceDriverLeave();
}
