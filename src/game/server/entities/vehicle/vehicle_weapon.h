#ifndef GAME_SERVER_ENTITIES_VEHICLE_VEHICLE_WEAPON_H
#define GAME_SERVER_ENTITIES_VEHICLE_VEHICLE_WEAPON_H

#include <base/vmath.h>

class CGameContext;
class CGameWorld;

enum
{
	VEHICLE_WEAPON_GRENADE_SPEED = 15,
	VEHICLE_WEAPON_MISSILE_SPEED = 13,
	VEHICLE_WEAPON_MISSILE_RANGE = 900,
};

void VehicleSpawnDelayedGrenade(CGameWorld *pWorld, int Owner, vec2 Pos, vec2 Dir, int Team);
void VehicleSpawnFallingGrenade(CGameContext *pGS, CGameWorld *pWorld, int Owner, vec2 Pos, vec2 Dir, int Team);
void VehicleSpawnHomingMissile(CGameWorld *pWorld, int Owner, vec2 Pos, vec2 Dir, int Team);
void VehicleTickHelicopterDriverWeapons(CGameContext *pGS, CGameWorld *pWorld, int DriverId, vec2 Pos, int Team,
	int &GrenadeReload, int &MissileReload, int &PrevFire, int &PrevJump);
void VehicleTickJetDriverWeapons(CGameContext *pGS, CGameWorld *pWorld, int DriverId, vec2 Pos, int Team,
	int &BombTicksLeft, int &BombInterval, int &BombCooldown, int &PrevFire, int &ShotgunReload);

void VehicleBattleFireGun(CGameContext *pGS, CGameWorld *pWorld, int DriverId, vec2 Pos, vec2 Dir, int Team, int &Reload);
void VehicleBattleFireHeavyGun(CGameContext *pGS, CGameWorld *pWorld, int DriverId, vec2 Pos, vec2 Dir, int Team, int &Reload);
void VehicleBattleFireHeavyCannon(CGameContext *pGS, CGameWorld *pWorld, int DriverId, vec2 Pos, vec2 Dir, int Team, int &Reload);

void VehicleTickHelicopterBattleDriver(CGameContext *pGS, CGameWorld *pWorld, int DriverId, vec2 Pos, int Team,
	int &GunReload, int &MissileReload, int &PrevFire, int &PrevJump);
void VehicleTickTankBattleDriver(CGameContext *pGS, CGameWorld *pWorld, int DriverId, vec2 Pos, int Team, int &Reload, int &PrevFire);
void VehicleTickCarBattleDriver(CGameContext *pGS, CGameWorld *pWorld, int DriverId, vec2 Pos, int Team,
	int &GunReload, int &MgReload, int &PrevFire, int &PrevHook);
void VehicleTickJetBattleDriver(CGameContext *pGS, CGameWorld *pWorld, int DriverId, vec2 Pos, vec2 Dir, int Team,
	int &BombTicksLeft, int &BombInterval, int &BombCooldown, int &GunReload, int &PrevFire, int &PrevHook);

#endif
