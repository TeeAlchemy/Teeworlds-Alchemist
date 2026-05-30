#pragma once

#include "vehicle.h"

enum
{
	AIRCRAFT_PART_IDS = 3,
	AIRCRAFT_LASER_IDS = 2,
	AIRCRAFT_MAX_SPEED = 18,
	AIRCRAFT_ACCEL = 2,
};

class CAircraft : public CVehicle
{
	int m_PartIds[AIRCRAFT_PART_IDS];
	int m_LaserIds[AIRCRAFT_LASER_IDS];

protected:
	void TickDriver(CCharacter *pDriver) override;
	float DriverOffsetY() const override { return VehicleScale(-8.f); }
	vec2 CollisionSize() const override { return vec2(VehicleScale(48.f), VehicleScale(24.f)); }
	void TickIdle() override;

public:
	CAircraft(CGameWorld *pGameWorld, vec2 Pos, int Team = -1);
	~CAircraft() override;

	void Snap(int SnappingClient) override;
};
