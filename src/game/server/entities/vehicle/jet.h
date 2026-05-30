#pragma once

#include "vehicle.h"

enum
{
	JET_PART_IDS = 4,
	JET_LASER_IDS = 2,
	JET_MAX_SPEED = 32,
	JET_MIN_ACCEL = 0,
	JET_MAX_ACCEL = 10,
	JET_ACCEL_STEP = 1,
};

static constexpr float JET_CRASH_SPEED = 20.f;

class CJet : public CVehicle
{
	int m_PartIds[JET_PART_IDS];
	int m_LaserIds[JET_LASER_IDS];
	vec2 m_FacingDir;
	float m_ThrustAccel;
	int m_BombTicksLeft;
	int m_BombInterval;
	int m_BombCooldown;
	int m_PrevDriverFire;
	int m_ShotgunReload;
	int m_PrevDriverHook;
	int m_SpeedBroadcastTicks;
	int m_ZeroSpeedTicks;

	void MoveAndHandleCrash();
	void CrashExplode();

protected:
	void TickDriver(CCharacter *pDriver) override;
	void TickDriverExtras(CCharacter *pDriver) override;
	float DriverOffsetY() const override { return VehicleScale(-6.f); }
	vec2 CollisionSize() const override { return vec2(VehicleScale(56.f), VehicleScale(20.f)); }
	void TickIdle() override;

public:
	CJet(CGameWorld *pGameWorld, vec2 Pos, int Team = -1);
	~CJet() override;

	void Snap(int SnappingClient) override;
};
