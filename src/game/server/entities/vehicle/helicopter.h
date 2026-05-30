#pragma once

#include "vehicle.h"

enum
{
	HELICOPTER_PART_IDS = 4,
	HELICOPTER_LASER_IDS = 4,
	HELICOPTER_MAX_SPEED = 14,
	HELICOPTER_ACCEL = 2,
};

class CHelicopter : public CVehicle
{
	int m_PartIds[HELICOPTER_PART_IDS];
	int m_LaserIds[HELICOPTER_LASER_IDS];
	int m_GrenadeReload;
	int m_MissileReload;
	int m_PrevDriverFire;
	int m_PrevDriverJump;
	float m_MainRotorAngle;
	float m_TailRotorAngle;

	void TickRotors(bool Ascending);

protected:
	void TickDriver(CCharacter *pDriver) override;
	void TickDriverExtras(CCharacter *pDriver) override;
	float DriverOffsetY() const override { return VehicleScale(-10.f); }
	vec2 CollisionSize() const override { return vec2(VehicleScale(52.f), VehicleScale(28.f)); }
	void TickIdle() override;
	bool HasGunnerSeat() const override { return true; }
	const char *GunnerBoardMessage() const override { return "You boarded the helicopter gunner seat!"; }
	int GunnerWeapon() const override { return WEAPON_SHOTGUN; }

public:
	CHelicopter(CGameWorld *pGameWorld, vec2 Pos, int Team = -1);
	~CHelicopter() override;

	void Snap(int SnappingClient) override;
};
