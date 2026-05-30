#include "vehicle_util.h"

#include "vehicle.h"

#include <game/generated/protocol.h>
#include <game/server/entities/character.h>
#include <game/server/entities/laser.h>
#include <game/server/entities/projectile.h>
#include <game/server/gamecontext.h>
#include <game/server/gameworld.h>
#include <game/server/player.h>

bool VehicleSpotBlocked(CGameWorld *pWorld, vec2 Pos, float Radius)
{
	bool Blocked = false;
	CVehicle::ForEach(pWorld, [&](CVehicle *pVehicle) {
		if (!Blocked && distance(pVehicle->GetPos(), Pos) < Radius)
			Blocked = true;
	});
	return Blocked;
}

void VehicleClearOccupant(CCharacter *pChr)
{
	if (!pChr)
		return;

	if (pChr->m_OnVehicle)
		pChr->m_VehicleDismountTick = pChr->Server()->Tick();

	pChr->m_OnVehicle = false;
	pChr->m_VehicleSeat = VEHICLE_SEAT_NONE;
}

void VehicleDismount(int &Owner, vec2 &Vel, CCharacter *pChr)
{
	if (Owner < 0 || !pChr)
		return;

	VehicleClearOccupant(pChr);
	pChr->GetCore()->m_Vel = Vel;
	Owner = -1;
	Vel *= 0.5f;
}

bool VehicleTryHookDismount(int &Owner, vec2 &Vel, CCharacter *pChr, int &PrevHookInput)
{
	if (!pChr)
		return false;

	const int Hook = pChr->GetCore()->m_Input.m_Hook;
	if ((Hook & 1) && !(PrevHookInput & 1))
	{
		VehicleDismount(Owner, Vel, pChr);
		return true;
	}

	PrevHookInput = Hook;
	return false;
}

void VehicleSyncCharacter(CCharacter *pChr, vec2 Pos, vec2 Vel, float RiderOffsetY)
{
	if (!pChr)
		return;

	pChr->GetCore()->m_Pos = vec2(Pos.x, Pos.y + RiderOffsetY);
	pChr->GetCore()->m_Vel = Vel;
	pChr->GetCore()->m_HookState = HOOK_IDLE;
}

void VehicleSyncGunnerCharacter(CCharacter *pChr, vec2 Pos, vec2 Vel, float DriverOffsetY)
{
	VehicleSyncCharacter(pChr, Pos, Vel, DriverOffsetY - (float)VEHICLE_SEAT_SPACING);
}

void VehicleApplyGravity(CGameContext *pGS, vec2 &Vel)
{
	if (!pGS)
		return;

	Vel.y += pGS->Tuning()->m_Gravity;
}

void VehicleApplyFriction(vec2 &Vel, float Friction)
{
	Vel.x *= Friction;
	Vel.y *= Friction;
}

void VehicleApplyFlyingVertical(CGameContext *pGS, vec2 &Vel, CCharacter *pDriver, int MaxSpeed, int Accel)
{
	VehicleApplyGravity(pGS, Vel);
	if (!pDriver)
		return;

	const CNetObj_PlayerInput &Input = pDriver->GetCore()->m_Input;
	if (Input.m_Hook & 1)
		Vel.y = SaturatedAdd(-(float)MaxSpeed, (float)MaxSpeed, Vel.y, -(float)Accel);
}

bool VehicleIsOnGround(CGameContext *pGS, vec2 Pos, vec2 Size)
{
	if (!pGS)
		return false;

	const vec2 Half = Size * 0.5f;
	const float ProbeY = Pos.y + Half.y + 2.f;

	if (pGS->Collision()->CheckPoint(Pos.x, ProbeY))
		return true;
	if (pGS->Collision()->CheckPoint(Pos.x - Half.x * 0.6f, ProbeY))
		return true;
	if (pGS->Collision()->CheckPoint(Pos.x + Half.x * 0.6f, ProbeY))
		return true;
	return false;
}

bool VehicleTryAutoBoard(CGameWorld *pWorld, CEntity *pVehicle, int &Owner, int Team, float BoardRadius)
{
	CCharacter *pChr = pWorld->ClosestCharacter(pVehicle->GetPos(), BoardRadius, pVehicle);
	if (!pChr || pChr->m_OnVehicle)
		return false;

	CVehicle *pVehicleCast = static_cast<CVehicle *>(pVehicle);
	if (!pVehicleCast->CanBoard(pChr->GetPlayer()->GetTeam()))
		return false;
	if (pChr->GetPlayer()->GetTeam() != Team && Team >= 0)
		return false;

	CGameContext *pGS = pWorld->GameServer();
	if (pGS->Server()->Tick() < pChr->m_VehicleDismountTick + pGS->Server()->TickSpeed())
		return false;
	if (pChr->GetPlayer()->NeedsClassSelection())
		return false;

	pChr->m_OnVehicle = true;
	pChr->m_VehicleSeat = VEHICLE_SEAT_DRIVER;
	Owner = pChr->GetPlayer()->GetCID();
	return true;
}

static bool TryBoardGunnerOnVehicle(CCharacter *pHammerer, CCharacter *pDriver, int &GunnerSlot)
{
	if (GunnerSlot >= 0)
		return false;

	GunnerSlot = pHammerer->GetPlayer()->GetCID();
	pHammerer->m_OnVehicle = true;
	pHammerer->m_VehicleSeat = VEHICLE_SEAT_GUNNER;
	return true;
}

bool VehicleTryHammerBoardGunner(CCharacter *pHammerer, CCharacter *pDriver)
{
	if (!pHammerer || !pDriver || pHammerer->m_OnVehicle)
		return false;
	if (!pDriver->m_OnVehicle || pDriver->m_VehicleSeat != VEHICLE_SEAT_DRIVER)
		return false;
	if (pHammerer->GetPlayer()->GetTeam() != pDriver->GetPlayer()->GetTeam())
		return false;

	CGameWorld *pWorld = &pHammerer->GameServer()->m_World;
	const int DriverId = pDriver->GetPlayer()->GetCID();
	bool Boarded = false;

	CVehicle::ForEach(pWorld, [&](CVehicle *pVehicle) {
		if (Boarded || !pVehicle->SupportsGunnerSeat())
			return;
		if (pVehicle->GetDriver() != DriverId)
			return;
		if (!TryBoardGunnerOnVehicle(pHammerer, pDriver, pVehicle->GunnerSlot()))
			return;

		const char *pMsg = pVehicle->GetGunnerBoardMessage();
		if (pMsg)
			pHammerer->GameServer()->Chat(pHammerer->GetPlayer()->GetCID(), pMsg);
		Boarded = true;
	});

	return Boarded;
}

void VehicleDriverLeave(CGameContext *pGS, int &Driver, int &Gunner, vec2 &Vel)
{
	if (Gunner >= 0)
	{
		if (CCharacter *pGunner = pGS->GetPlayerChar(Gunner))
			VehicleClearOccupant(pGunner);
		Gunner = -1;
	}

	if (Driver >= 0)
	{
		if (CCharacter *pDriver = pGS->GetPlayerChar(Driver))
			VehicleDismount(Driver, Vel, pDriver);
		else
			Driver = -1;
	}
}

bool VehicleTryGunnerLeave(CGameContext *pGS, int &Gunner, int &PrevGunnerHook)
{
	if (Gunner < 0)
		return false;

	CCharacter *pGunner = pGS->GetPlayerChar(Gunner);
	if (!pGunner)
	{
		Gunner = -1;
		return false;
	}

	const int Hook = pGunner->GetCore()->m_Input.m_Hook;
	if ((Hook & 1) && !(PrevGunnerHook & 1))
	{
		VehicleClearOccupant(pGunner);
		Gunner = -1;
		return true;
	}

	PrevGunnerHook = Hook;
	return false;
}

void VehicleTickGunnerWeapon(CGameContext *pGS, CGameWorld *pWorld, int GunnerId, vec2 FirePos, int Weapon, int &Reload, int &PrevFire, int Team, float ProjSpawnOffset)
{
	CCharacter *pGunner = pGS->GetPlayerChar(GunnerId);
	if (!pGunner)
		return;

	if (Reload > 0)
	{
		Reload--;
		return;
	}

	const int Fire = pGunner->GetCore()->m_Input.m_Fire;
	const bool FirePress = (Fire & 1) && !(PrevFire & 1);
	PrevFire = Fire;
	if (!FirePress)
		return;

	vec2 Direction = normalize(vec2(pGunner->GetCore()->m_Input.m_TargetX, pGunner->GetCore()->m_Input.m_TargetY));
	if (length(Direction) < 1e-3f)
		Direction = vec2(1.f, 0.f);

	vec2 ProjStartPos = FirePos + Direction * ProjSpawnOffset;

	switch (Weapon)
	{
	case WEAPON_SHOTGUN:
	{
		const int ShotSpread = 2;
		for (int i = -ShotSpread; i <= ShotSpread; ++i)
		{
			float Spreading[] = {-0.185f, -0.070f, 0, 0.070f, 0.185f};
			float a = GetAngle(Direction) + Spreading[i + 2];
			float v = 1 - (absolute(i) / (float)ShotSpread);
			float Speed = mix((float)pGS->Tuning()->m_ShotgunSpeeddiff, 1.0f, v);
			new CProjectile(pWorld, WEAPON_SHOTGUN, GunnerId, ProjStartPos, vec2(cosf(a), sinf(a)) * Speed,
				(int)(pGS->Server()->TickSpeed() * pGS->Tuning()->m_ShotgunLifetime), 1, 0, 0, -1, WEAPON_SHOTGUN, Team);
		}
		pGS->CreateSound(FirePos, SOUND_SHOTGUN_FIRE);
		Reload = pGS->Server()->TickSpeed() / 2;
	}
	break;

	case WEAPON_GRENADE:
		new CProjectile(pWorld, WEAPON_GRENADE, GunnerId, ProjStartPos, Direction,
			(int)(pGS->Server()->TickSpeed() * pGS->Tuning()->m_GrenadeLifetime), 1, true, 0, SOUND_GRENADE_EXPLODE, WEAPON_GRENADE, Team);
		pGS->CreateSound(FirePos, SOUND_GRENADE_FIRE);
		Reload = pGS->Server()->TickSpeed();
		break;

	case WEAPON_RIFLE:
		new CLaser(pWorld, FirePos, Direction, pGS->Tuning()->m_LaserReach, GunnerId);
		pGS->CreateSound(FirePos, SOUND_RIFLE_FIRE);
		Reload = pGS->Server()->TickSpeed() / 3;
		break;

	default:
		break;
	}
}

void VehicleOnCharacterDie(CGameContext *pGS, int ClientId)
{
	CVehicle::ForEach(&pGS->m_World, [&](CVehicle *pVehicle) {
		if (pVehicle->IsOccupiedBy(ClientId))
			pVehicle->HandleOccupantDismount(ClientId);
	});
}

void VehicleHandleEmoteDismount(CGameContext *pGS, int ClientId)
{
	VehicleHandleHeartsDismount(pGS, ClientId);
}

void VehicleHandleHeartsDismount(CGameContext *pGS, int ClientId)
{
	CCharacter *pChr = pGS->GetPlayerChar(ClientId);
	if (!pChr || !pChr->m_OnVehicle)
		return;

	CVehicle::ForEach(&pGS->m_World, [&](CVehicle *pVehicle) {
		if (pVehicle->IsOccupiedBy(ClientId))
			pVehicle->HandleOccupantDismount(ClientId);
	});
}

static bool VehicleIntersectSegmentCircle(vec2 Pos0, vec2 Pos1, vec2 Center, float Radius, vec2 *pOutCollision)
{
	vec2 Seg = Pos1 - Pos0;
	const float SegLenSq = dot(Seg, Seg);
	if (SegLenSq <= 0.0001f)
	{
		if (distance(Pos0, Center) > Radius)
			return false;
		*pOutCollision = Pos0;
		return true;
	}

	const float t = clamp(dot(Center - Pos0, Seg) / SegLenSq, 0.0f, 1.0f);
	const vec2 Closest = Pos0 + Seg * t;
	if (distance(Closest, Center) > Radius)
		return false;

	*pOutCollision = Closest;
	return true;
}

CVehicle *VehicleIntersectLine(CGameWorld *pWorld, vec2 Pos0, vec2 Pos1, float Radius, vec2 &NewPos, CEntity *pNotThis)
{
	float ClosestLen = distance(Pos0, Pos1) * 100.0f;
	CVehicle *pClosest = nullptr;

	CVehicle::ForEach(pWorld, [&](CVehicle *pVehicle) {
		if (pVehicle == pNotThis || pVehicle->IsMarkedForDestroy())
			return;

		const vec2 HalfSize = pVehicle->GetCollisionSize() * 0.5f;
		const float HitRadius = length(HalfSize) + Radius;
		vec2 Col;
		if (!VehicleIntersectSegmentCircle(Pos0, Pos1, pVehicle->GetPos(), HitRadius, &Col))
			return;

		const float Len = distance(Col, Pos0);
		if (Len < ClosestLen)
		{
			NewPos = Col;
			ClosestLen = Len;
			pClosest = pVehicle;
		}
	});

	return pClosest;
}

CVehicle *VehicleFindByOccupant(CGameWorld *pWorld, int ClientId)
{
	CVehicle *pFound = nullptr;
	CVehicle::ForEach(pWorld, [&](CVehicle *pVehicle) {
		if (!pFound && !pVehicle->IsMarkedForDestroy() && pVehicle->IsOccupiedBy(ClientId))
			pFound = pVehicle;
	});
	return pFound;
}

void VehicleSpawnShrapnel(CGameContext *pGS, CGameWorld *pWorld, vec2 Pos, int Owner, int Team)
{
	const int Pellets = 12;
	for (int i = 0; i < Pellets; i++)
	{
		const float Angle = (float)i / (float)Pellets * 2.f * pi;
		const vec2 Dir = vec2(cosf(Angle), sinf(Angle));
		const float Speed = mix((float)pGS->Tuning()->m_ShotgunSpeeddiff, 1.0f, 0.85f);
		new CProjectile(pWorld, WEAPON_SHOTGUN, Owner, Pos, Dir * Speed,
			(int)(pGS->Server()->TickSpeed() * pGS->Tuning()->m_ShotgunLifetime), 1, 0, 0, -1, WEAPON_SHOTGUN, Team);
	}
	pGS->CreateSound(Pos, SOUND_SHOTGUN_FIRE);
}

void VehicleApplyExplosionDamage(CGameWorld *pWorld, vec2 Pos, float Radius, float InnerRadius, int Owner)
{
	CVehicle *apVehicles[16];
	int Num = 0;

	CVehicle::ForEach(pWorld, [&](CVehicle *pVehicle) {
		if (pVehicle->IsMarkedForDestroy())
			return;
		if (distance(pVehicle->GetPos(), Pos) > Radius)
			return;
		if (Num < (int)(sizeof(apVehicles) / sizeof(apVehicles[0])))
			apVehicles[Num++] = pVehicle;
	});

	for (int i = 0; i < Num; i++)
	{
		CVehicle *pVehicle = apVehicles[i];
		if (pVehicle->IsMarkedForDestroy())
			continue;

		const float l = distance(pVehicle->GetPos(), Pos);
		float DmgFactor = 1.f - clamp((l - InnerRadius) / (Radius - InnerRadius), 0.0f, 1.0f);
		const int Dmg = (int)(6.f * DmgFactor);
		if (Dmg > 0)
			pVehicle->TakeDamage(Dmg, Owner);
	}
}
