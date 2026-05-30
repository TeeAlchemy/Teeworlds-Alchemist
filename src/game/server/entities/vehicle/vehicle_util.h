#ifndef GAME_SERVER_ENTITIES_VEHICLE_VEHICLE_UTIL_H
#define GAME_SERVER_ENTITIES_VEHICLE_VEHICLE_UTIL_H

#include <base/vmath.h>

class CCharacter;
class CEntity;
class CGameContext;
class CGameWorld;
class CVehicle;

enum
{
	VEHICLE_SEAT_NONE = 0,
	VEHICLE_SEAT_DRIVER = 1,
	VEHICLE_SEAT_GUNNER = 2,
	VEHICLE_SEAT_SPACING = 38,
};

static constexpr float VEHICLE_SIZE_SCALE = 1.35f;
static constexpr float VEHICLE_TILE_HEIGHT = 32.f;

inline float VehicleScale(float Value)
{
	return Value * VEHICLE_SIZE_SCALE;
}

bool VehicleSpotBlocked(CGameWorld *pWorld, vec2 Pos, float Radius = VehicleScale(56.f));
void VehicleClearOccupant(CCharacter *pChr);
void VehicleDismount(int &Owner, vec2 &Vel, CCharacter *pChr);
bool VehicleTryHookDismount(int &Owner, vec2 &Vel, CCharacter *pChr, int &PrevHookInput);
void VehicleSyncCharacter(CCharacter *pChr, vec2 Pos, vec2 Vel, float RiderOffsetY = -8.f);
void VehicleSyncGunnerCharacter(CCharacter *pChr, vec2 Pos, vec2 Vel, float DriverOffsetY);
void VehicleApplyGravity(CGameContext *pGS, vec2 &Vel);
void VehicleApplyFriction(vec2 &Vel, float Friction);
void VehicleApplyFlyingVertical(CGameContext *pGS, vec2 &Vel, CCharacter *pDriver, int MaxSpeed, int Accel);
bool VehicleIsOnGround(CGameContext *pGS, vec2 Pos, vec2 Size);
bool VehicleTryAutoBoard(CGameWorld *pWorld, CEntity *pVehicle, int &Owner, int Team, float BoardRadius = VehicleScale(32.f));
void VehicleHandleHeartsDismount(CGameContext *pGS, int ClientId);
void VehicleHandleEmoteDismount(CGameContext *pGS, int ClientId);

bool VehicleTryHammerBoardGunner(CCharacter *pHammerer, CCharacter *pDriver);
void VehicleDriverLeave(CGameContext *pGS, int &Driver, int &Gunner, vec2 &Vel);
bool VehicleTryGunnerLeave(CGameContext *pGS, int &Gunner, int &PrevGunnerHook);
void VehicleTickGunnerWeapon(CGameContext *pGS, CGameWorld *pWorld, int GunnerId, vec2 FirePos, int Weapon, int &Reload, int &PrevFire, int Team, float ProjSpawnOffset = VehicleScale(28.f));
void VehicleOnCharacterDie(CGameContext *pGS, int ClientId);

CVehicle *VehicleIntersectLine(CGameWorld *pWorld, vec2 Pos0, vec2 Pos1, float Radius, vec2 &NewPos, CEntity *pNotThis = nullptr);
CVehicle *VehicleFindByOccupant(CGameWorld *pWorld, int ClientId);
void VehicleSpawnShrapnel(CGameContext *pGS, CGameWorld *pWorld, vec2 Pos, int Owner, int Team);
void VehicleApplyExplosionDamage(CGameWorld *pWorld, vec2 Pos, float Radius, float InnerRadius, int Owner);

#endif
