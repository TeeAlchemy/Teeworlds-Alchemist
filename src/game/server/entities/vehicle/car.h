#pragma once

#include "vehicle.h"

enum
{
	CAR_PART_IDS = 4,
	CAR_LASER_IDS = 1,
	CAR_MAX_SPEED = 25,
	CAR_ACCEL = 2,
};

class CCar : public CVehicle
{
	int m_PartIds[CAR_PART_IDS];
	int m_LaserIds[CAR_LASER_IDS];
	int m_GunReload;
	int m_MgReload;
	int m_PrevDriverFire;
	int m_PrevDriverHook;

protected:
	void TickDriver(CCharacter *pDriver) override;
	void TickDriverExtras(CCharacter *pDriver) override;
	float DriverOffsetY() const override { return VehicleScale(-2.f); }
	float MaxStepHeight() const override { return VEHICLE_TILE_HEIGHT; }
	vec2 CollisionSize() const override { return vec2(VehicleScale(93.f), VehicleScale(16.f)); }
	void TickIdle() override;
	void OnDriverBoarded(CCharacter *pDriver) override;

public:
	CCar(CGameWorld *pGameWorld, vec2 Pos, int Team = -1);
	~CCar() override;

	void Snap(int SnappingClient) override;
};
