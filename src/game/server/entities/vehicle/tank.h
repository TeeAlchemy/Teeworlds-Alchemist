#pragma once

#include "vehicle.h"

enum
{
	TANK_PART_IDS = 8,
	TANK_LASER_IDS = 4,
	TANK_MAX_SPEED = 12,
	TANK_ACCEL = 1,
};

static const float TANK_TURRET_OFFSET_Y = 14.f * VEHICLE_SIZE_SCALE;
static const float TANK_BARREL_LEN = 42.f * VEHICLE_SIZE_SCALE;

class CTank : public CVehicle
{
	int m_PartIds[TANK_PART_IDS];
	int m_LaserIds[TANK_LASER_IDS];
	int m_BarrelId;
	vec2 m_BarrelDir;
	int m_CannonReload;
	int m_PrevDriverFire;

	vec2 GetTurretPos() const { return vec2(m_Pos.x, m_Pos.y - TANK_TURRET_OFFSET_Y); }
	vec2 GetBarrelTip(vec2 Dir) const { return GetTurretPos() + Dir * TANK_BARREL_LEN; }

protected:
	void TickDriver(CCharacter *pDriver) override;
	void TickDriverExtras(CCharacter *pDriver) override;
	void TickGunner(CCharacter *pGunner) override;
	float DriverOffsetY() const override { return VehicleScale(-2.f); }
	float BoardRadius() const override { return VehicleScale(40.f); }
	float MaxStepHeight() const override { return VEHICLE_TILE_HEIGHT; }
	vec2 CollisionSize() const override { return vec2(VehicleScale(62.f), VehicleScale(28.f)); }
	void TickIdle() override;
	void OnDriverBoarded(CCharacter *pDriver) override;
	bool HasGunnerSeat() const override { return true; }
	const char *GunnerBoardMessage() const override { return "You boarded the tank gunner seat!"; }
	int GunnerWeapon() const override { return WEAPON_GRENADE; }
	float GunnerProjSpawnOffset() const override { return 0.f; }
	vec2 GunnerFirePos(CCharacter *pGunner) const override;

public:
	CTank(CGameWorld *pGameWorld, vec2 Pos, int Team = -1);
	~CTank() override;

	void Snap(int SnappingClient) override;
};
