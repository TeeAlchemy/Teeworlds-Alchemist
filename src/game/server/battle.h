#ifndef GAME_SERVER_BATTLE_H
#define GAME_SERVER_BATTLE_H

#include <base/vmath.h>

class CCharacter;
class CGameContext;
class CGameControllerWorkbenches;
class CGameWorld;
class CPlayer;
class CVehicle;

enum EBattleClass
{
	BATTLE_SOLDIER = 0,
	BATTLE_ENGINEER,
	BATTLE_MEDIC,
	BATTLE_SNIPER,
	NUM_BATTLE_CLASS,
};

enum EBattleThrowable
{
	BATTLE_THROW_NONE = 0,
	BATTLE_THROW_GRENADE,
	BATTLE_THROW_SMOKE,
	BATTLE_THROW_WATER,
};

enum
{
	BATTLE_SOLDIER_MAGS = 6,
	BATTLE_SOLDIER_MAG_SIZE = 30,
	BATTLE_MAX_C4 = 3,
	BATTLE_MAX_MINES = 3,
	BATTLE_SNIPER_INVIS_ENERGY_MAX = 250,
	BATTLE_SNIPER_LASER_RELOAD = 5,
	BATTLE_SNIPER_NINJA_COOLDOWN = 3,
	BATTLE_SOLDIER_AMMO_PACK_CD = 10,
};

const char *BattleClassName(int Class);
bool BattleIsEnabled(CGameContext *pGS);
bool BattleIsEnabled();
void BattleApplyLoadout(CCharacter *pChr, int Class);
bool BattleFireWeapon(CCharacter *pChr);
bool BattleHammer(CCharacter *pChr);
bool BattleHook(CCharacter *pChr);
void BattleTickCharacter(CCharacter *pChr);
void BattleHandleEmote(CGameContext *pGS, int ClientId);
void BattleHandleECommand(CGameContext *pGS, CPlayer *pPlayer);
bool BattlePickupThrowable(CCharacter *pChr, int PickupSubtype);
bool BattleIsInvisibleTo(int SnappingClient, CCharacter *pChr);
bool BattleIsInSmoke(CCharacter *pChr);

void BattleDetonateC4(CGameWorld *pWorld, int Owner, int Team);
void BattleSpawnC4(CGameWorld *pWorld, int Owner, vec2 Pos, int Team, CCharacter *pAttached = nullptr, CVehicle *pAttachedVehicle = nullptr);
int BattleCountC4(CGameWorld *pWorld, int Owner, int Team);
void BattleSpawnMine(CGameWorld *pWorld, int Owner, vec2 Pos, int Team);
int BattleCountMines(CGameWorld *pWorld, int Owner, int Team);
void BattleDisarmMine(CGameWorld *pWorld, vec2 Pos, float Radius, int Team);

bool BattleCanPickupAtPoint(int PointTeam, int PlayerTeam);
bool BattleOnMapEntity(int Index, vec2 Pos, CGameContext *pGS, CGameControllerWorkbenches *pCtrl);
void BattleRestorePlayerAmmo(CCharacter *pChr);
void BattleOnCharacterDeath(CGameContext *pGS, CCharacter *pVictim);

void BattleSpawnSmoke(CGameWorld *pWorld, vec2 Pos);
void BattleSpawnAmmoPack(CGameWorld *pWorld, int Owner, vec2 Pos, int Team);
void BattleSpawnHeart(CGameWorld *pWorld, int Owner, vec2 Pos, vec2 Dir, int Team);

#endif
