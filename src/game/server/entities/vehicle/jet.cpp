#include <game/server/gamecontext.h>
#include <game/server/battle.h>
#include <game/server/entities/character.h>

#include "jet.h"
#include "vehicle_util.h"
#include "vehicle_visual.h"
#include "vehicle_weapon.h"

CJet::CJet(CGameWorld *pGameWorld, vec2 Pos, int Team)
	: CVehicle(pGameWorld, CGameWorld::ENTTYPE_JET, Pos, Team, 45)
{
	m_FacingDir = vec2(1.f, 0.f);
	m_ThrustAccel = 0.f;
	m_BombTicksLeft = 0;
	m_BombInterval = 0;
	m_BombCooldown = 0;
	m_PrevDriverFire = 0;
	m_ShotgunReload = 0;
	m_PrevDriverHook = 0;
	m_SpeedBroadcastTicks = 0;
	m_ZeroSpeedTicks = 0;

	for (int i = 0; i < JET_PART_IDS; i++)
		m_PartIds[i] = Server()->SnapNewID();
	for (int i = 0; i < JET_LASER_IDS; i++)
		m_LaserIds[i] = Server()->SnapNewID();
	GameWorld()->InsertEntity(this);
}

CJet::~CJet()
{
	for (int i = 0; i < JET_PART_IDS; i++)
		Server()->SnapFreeID(m_PartIds[i]);
	for (int i = 0; i < JET_LASER_IDS; i++)
		Server()->SnapFreeID(m_LaserIds[i]);
}

void CJet::TickDriver(CCharacter *pDriver)
{
	const CNetObj_PlayerInput &Input = pDriver->GetCore()->m_Input;
	vec2 Dir = normalize(vec2(Input.m_TargetX, Input.m_TargetY));
	if (length(Dir) > 1e-3f)
		m_FacingDir = Dir;
	else if (length(m_FacingDir) < 1e-3f)
		m_FacingDir = vec2(1.f, 0.f);

	if (BattleIsEnabled())
	{
		const float BattleThrust = (float)JET_ACCEL_STEP * 1.5f;
		if (pDriver->GetCore()->m_Direction < 0)
			m_Vel -= m_FacingDir * BattleThrust;
		if (pDriver->GetCore()->m_Direction > 0)
			m_Vel += m_FacingDir * BattleThrust;

		if (length(m_Vel) < 2.f)
			m_ZeroSpeedTicks++;
		else
			m_ZeroSpeedTicks = 0;

		if (m_ZeroSpeedTicks > Server()->TickSpeed() * 2)
		{
			ForceDriverLeave();
			m_ZeroSpeedTicks = 0;
		}
	}
	else
	{
		if (pDriver->GetCore()->m_Direction > 0)
			m_ThrustAccel = SaturatedAdd((float)JET_MIN_ACCEL, (float)JET_MAX_ACCEL, m_ThrustAccel, (float)JET_ACCEL_STEP);
		else if (pDriver->GetCore()->m_Direction < 0)
			m_ThrustAccel = SaturatedAdd((float)JET_MIN_ACCEL, (float)JET_MAX_ACCEL, m_ThrustAccel, -(float)JET_ACCEL_STEP);

		m_Vel += m_FacingDir * m_ThrustAccel;
	}
	VehicleApplyGravity(GameServer(), m_Vel);

	const float Speed = length(m_Vel);
	if (Speed > (float)JET_MAX_SPEED)
		m_Vel = normalize(m_Vel) * (float)JET_MAX_SPEED;

	MoveAndHandleCrash();
}

void CJet::MoveAndHandleCrash()
{
	const vec2 PrePos = m_Pos;
	const vec2 PreVel = m_Vel;
	const float PreSpeed = length(PreVel);

	MoveBox(CollisionSize());

	if (PreSpeed < JET_CRASH_SPEED)
		return;

	const float Moved = distance(PrePos, m_Pos);
	if (Moved + 2.f < PreSpeed * 0.4f)
		CrashExplode();
}

void CJet::CrashExplode()
{
	Explode();
}

void CJet::TickDriverExtras(CCharacter *pDriver)
{
	(void)pDriver;
	if (BattleIsEnabled())
	{
		VehicleTickJetBattleDriver(GameServer(), GameWorld(), m_Driver, m_Pos, m_FacingDir, m_Team,
			m_BombTicksLeft, m_BombInterval, m_BombCooldown, m_ShotgunReload, m_PrevDriverFire, m_PrevDriverHook);

		if (--m_SpeedBroadcastTicks <= 0)
		{
			m_SpeedBroadcastTicks = Server()->TickSpeed();
			GameServer()->Broadcast(m_Driver, "飞机速度: {:.0f}", length(m_Vel));
		}
		return;
	}

	VehicleTickJetDriverWeapons(GameServer(), GameWorld(), m_Driver, m_Pos, m_Team, m_BombTicksLeft, m_BombInterval,
		m_BombCooldown, m_PrevDriverFire, m_ShotgunReload);

	if (--m_SpeedBroadcastTicks <= 0)
	{
		m_SpeedBroadcastTicks = Server()->TickSpeed();
		GameServer()->Broadcast(m_Driver, "飞机速度: {:.0f} 加速度: {:.1f}", length(m_Vel), m_ThrustAccel);
	}
}

void CJet::TickIdle()
{
	m_Vel += m_FacingDir * m_ThrustAccel;
	VehicleApplyGravity(GameServer(), m_Vel);
	MoveAndHandleCrash();
}

void CJet::Snap(int SnappingClient)
{
	if (NetworkClipped(SnappingClient))
		return;

	vec2 FacingDir = m_FacingDir;
	if (length(FacingDir) < 1e-3f)
	{
		if (length(m_Vel) > 1e-3f)
			FacingDir = normalize(m_Vel);
		else
			FacingDir = vec2(1.f, 0.f);
	}

	VehicleVisual::SnapJet(Server(), GetID(), m_PartIds, JET_PART_IDS, m_LaserIds, m_Pos, FacingDir, Server()->Tick());
	SnapHealthBar(SnappingClient);
}
