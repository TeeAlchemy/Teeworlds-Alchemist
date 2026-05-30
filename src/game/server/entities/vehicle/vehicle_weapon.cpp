#include "vehicle_weapon.h"

#include <game/generated/protocol.h>
#include <game/server/battle.h>
#include <game/server/entities/character.h>
#include <game/server/entities/laser.h>
#include <game/server/entities/projectile.h>
#include <game/server/gamecontext.h>
#include <game/server/gameworld.h>
#include <game/server/player.h>

#include "vehicle_util.h"

static int VehicleWeaponDelayTicks(CGameContext *pGS)
{
	return pGS->Server()->TickSpeed() / 5;
}

static CCharacter *FindClosestEnemy(CGameWorld *pWorld, CGameContext *pGS, vec2 Pos, int Team, float MaxRange)
{
	CCharacter *pBest = nullptr;
	float BestDist = MaxRange;

	for (CCharacter *pChr = (CCharacter *)pWorld->FindFirst(CGameWorld::ENTTYPE_CHARACTER); pChr; pChr = (CCharacter *)pChr->TypeNext())
	{
		if (!pChr->IsAlive())
			continue;
		if (pChr->GetPlayer()->GetTeam() == Team)
			continue;

		const float Dist = distance(pChr->GetPos(), Pos);
		if (Dist < BestDist)
		{
			BestDist = Dist;
			pBest = pChr;
		}
	}

	return pBest;
}

class CVehicleGrenade : public CEntity
{
	int m_Owner;
	int m_Team;
	int m_SpawnTick;
	int m_LifeSpan;
	vec2 m_Dir;
	float m_Speed;

public:
	CVehicleGrenade(CGameWorld *pGameWorld, int Owner, vec2 Pos, vec2 Dir, int Team)
		: CEntity(pGameWorld, CGameWorld::ENTTYPE_VEHICLE_GRENADE, Pos, 6.f)
	{
		m_Owner = Owner;
		m_Team = Team;
		m_SpawnTick = Server()->Tick();
		m_LifeSpan = Server()->TickSpeed() * 4;
		m_Dir = normalize(Dir);
		m_Speed = (float)VEHICLE_WEAPON_GRENADE_SPEED;
		GameWorld()->InsertEntity(this);
	}

	virtual void Tick() override
	{
		if (Server()->Tick() - m_SpawnTick < VehicleWeaponDelayTicks(GameServer()))
			return;

		vec2 PrevPos = m_Pos;
		vec2 NewPos = m_Pos + m_Dir * m_Speed;
		CCharacter *pOwner = GameServer()->GetPlayerChar(m_Owner);

		int Collide = GameServer()->Collision()->IntersectLine(PrevPos, NewPos, &NewPos, 0);
		CCharacter *pTarget = GameServer()->m_World.IntersectCharacter(PrevPos, NewPos, 6.f, NewPos, pOwner);
		m_Pos = NewPos;
		m_LifeSpan--;

		if (pTarget || Collide || m_LifeSpan < 0 || GameLayerClipped(m_Pos))
		{
			GameServer()->CreateSound(m_Pos, SOUND_GRENADE_EXPLODE);
			GameServer()->CreateExplosion(m_Pos, m_Owner, WEAPON_GRENADE, false);

			if (pTarget && pTarget->GetPlayer()->GetTeam() != m_Team)
				pTarget->TakeDamage(m_Dir, 1, m_Owner, WEAPON_GRENADE);

			GameWorld()->DestroyEntity(this);
		}
	}

	virtual void Snap(int SnappingClient) override
	{
		if (NetworkClipped(SnappingClient))
			return;

		CNetObj_Projectile *pProj = static_cast<CNetObj_Projectile *>(Server()->SnapNewItem(NETOBJTYPE_PROJECTILE, GetID(), sizeof(CNetObj_Projectile)));
		if (!pProj)
			return;

		pProj->m_X = (int)m_Pos.x;
		pProj->m_Y = (int)m_Pos.y;
		pProj->m_VelX = (int)(m_Dir.x * 100.f);
		pProj->m_VelY = (int)(m_Dir.y * 100.f);
		pProj->m_StartTick = Server()->Tick();
		pProj->m_Type = WEAPON_GRENADE;
	}
};

class CVehicleMissile : public CEntity
{
	int m_Owner;
	int m_Team;
	int m_SpawnTick;
	int m_LifeSpan;
	vec2 m_Dir;
	float m_Speed;
	bool m_Homing;
	float m_Rotation;
	int m_LaserIds[2];

public:
	CVehicleMissile(CGameWorld *pGameWorld, int Owner, vec2 Pos, vec2 Dir, int Team, bool Homing)
		: CEntity(pGameWorld, CGameWorld::ENTTYPE_VEHICLE_MISSILE, Pos, 8.f)
	{
		m_Owner = Owner;
		m_Team = Team;
		m_SpawnTick = Server()->Tick();
		m_LifeSpan = Server()->TickSpeed() * 5;
		m_Dir = normalize(Dir);
		m_Speed = (float)VEHICLE_WEAPON_MISSILE_SPEED;
		m_Homing = Homing;
		m_Rotation = 0.f;
		m_LaserIds[0] = Server()->SnapNewID();
		m_LaserIds[1] = Server()->SnapNewID();
		GameWorld()->InsertEntity(this);
	}

	~CVehicleMissile()
	{
		for (int i = 0; i < 2; i++)
			Server()->SnapFreeID(m_LaserIds[i]);
	}

	virtual void Tick() override
	{
		m_Rotation += 0.18f;

		if (Server()->Tick() - m_SpawnTick < VehicleWeaponDelayTicks(GameServer()))
			return;

		if (m_Homing)
		{
			if (CCharacter *pTarget = FindClosestEnemy(GameWorld(), GameServer(), m_Pos, m_Team, (float)VEHICLE_WEAPON_MISSILE_RANGE))
			{
				vec2 Desired = normalize(pTarget->GetPos() - m_Pos);
				m_Dir = normalize(m_Dir + Desired * 0.12f);
			}
		}

		vec2 PrevPos = m_Pos;
		vec2 NewPos = m_Pos + m_Dir * m_Speed;
		CCharacter *pOwner = GameServer()->GetPlayerChar(m_Owner);

		int Collide = GameServer()->Collision()->IntersectLine(PrevPos, NewPos, &NewPos, 0);
		CCharacter *pTarget = GameServer()->m_World.IntersectCharacter(PrevPos, NewPos, 8.f, NewPos, pOwner);
		m_Pos = NewPos;
		m_LifeSpan--;

		if (pTarget || Collide || m_LifeSpan < 0 || GameLayerClipped(m_Pos))
		{
			GameServer()->CreateSound(m_Pos, SOUND_GRENADE_EXPLODE);
			GameServer()->CreateExplosion(m_Pos, m_Owner, WEAPON_GRENADE, false);

			if (pTarget && pTarget->GetPlayer()->GetTeam() != m_Team)
				pTarget->TakeDamage(m_Dir, 2, m_Owner, WEAPON_RIFLE);

			GameWorld()->DestroyEntity(this);
		}
	}

	virtual void Snap(int SnappingClient) override
	{
		if (NetworkClipped(SnappingClient))
			return;

		const float CrossLen = 24.f;
		vec2 AxisA = vec2(cosf(m_Rotation), sinf(m_Rotation)) * CrossLen;
		vec2 AxisB = vec2(cosf(m_Rotation + pi / 2.f), sinf(m_Rotation + pi / 2.f)) * CrossLen;

		const vec2 aOrigins[2] = {AxisA, AxisB};
		for (int i = 0; i < 2; i++)
		{
			CNetObj_Laser *pLaser = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_LaserIds[i], sizeof(CNetObj_Laser)));
			if (!pLaser)
				return;

			pLaser->m_FromX = (int)(m_Pos.x - aOrigins[i].x);
			pLaser->m_FromY = (int)(m_Pos.y - aOrigins[i].y);
			pLaser->m_X = (int)(m_Pos.x + aOrigins[i].x);
			pLaser->m_Y = (int)(m_Pos.y + aOrigins[i].y);
			pLaser->m_StartTick = Server()->Tick();
		}
	}
};

void VehicleSpawnDelayedGrenade(CGameWorld *pWorld, int Owner, vec2 Pos, vec2 Dir, int Team)
{
	if (length(Dir) < 1e-3f)
		Dir = vec2(1.f, 0.f);
	new CVehicleGrenade(pWorld, Owner, Pos, Dir, Team);
}

void VehicleSpawnFallingGrenade(CGameContext *pGS, CGameWorld *pWorld, int Owner, vec2 Pos, vec2 Dir, int Team)
{
	if (length(Dir) < 1e-3f)
		Dir = vec2(1.f, 0.f);

	new CProjectile(pWorld, WEAPON_GRENADE, Owner, Pos, Dir,
		(int)(pGS->Server()->TickSpeed() * pGS->Tuning()->m_GrenadeLifetime), 1, true, 0, SOUND_GRENADE_EXPLODE, WEAPON_GRENADE, Team);
}

void VehicleSpawnHomingMissile(CGameWorld *pWorld, int Owner, vec2 Pos, vec2 Dir, int Team)
{
	if (length(Dir) < 1e-3f)
		Dir = vec2(1.f, 0.f);

	const bool Homing = FindClosestEnemy(pWorld, pWorld->GameServer(), Pos, Team, (float)VEHICLE_WEAPON_MISSILE_RANGE) != nullptr;
	new CVehicleMissile(pWorld, Owner, Pos, Dir, Team, Homing);
}

void VehicleTickHelicopterDriverWeapons(CGameContext *pGS, CGameWorld *pWorld, int DriverId, vec2 Pos, int Team,
	int &GrenadeReload, int &MissileReload, int &PrevFire, int &PrevJump)
{
	CCharacter *pDriver = pGS->GetPlayerChar(DriverId);
	if (!pDriver)
		return;

	if (GrenadeReload > 0)
		GrenadeReload--;
	if (MissileReload > 0)
		MissileReload--;

	const CNetObj_PlayerInput &Input = pDriver->GetCore()->m_Input;
	vec2 Dir = normalize(vec2(Input.m_TargetX, Input.m_TargetY));
	if (length(Dir) < 1e-3f)
		Dir = vec2(1.f, 0.f);

	const int Fire = Input.m_Fire;
	if ((Fire & 1) && !(PrevFire & 1) && GrenadeReload <= 0)
	{
		VehicleSpawnDelayedGrenade(pWorld, DriverId, Pos + Dir * VehicleScale(32.f), Dir, Team);
		pGS->CreateSound(Pos, SOUND_GRENADE_FIRE);
		GrenadeReload = pGS->Server()->TickSpeed() / 2;
	}
	PrevFire = Fire;

	const int Jump = Input.m_Jump;
	if ((Jump & 1) && !(PrevJump & 1) && MissileReload <= 0)
	{
		VehicleSpawnHomingMissile(pWorld, DriverId, Pos + Dir * VehicleScale(32.f), Dir, Team);
		pGS->CreateSound(Pos, SOUND_RIFLE_FIRE);
		MissileReload = pGS->Server()->TickSpeed();
	}
	PrevJump = Jump;
}

static void VehicleFireJetShotgunPellet(CGameContext *pGS, CGameWorld *pWorld, int DriverId, vec2 Pos, vec2 Dir, int Team)
{
	const vec2 ProjStartPos = Pos + Dir * VehicleScale(32.f);
	new CProjectile(pWorld, WEAPON_SHOTGUN, DriverId, ProjStartPos, Dir * 2.5f,
		(int)(pGS->Server()->TickSpeed() * pGS->Tuning()->m_ShotgunLifetime), 1, 0, 0, -1, WEAPON_SHOTGUN, Team);
	pGS->CreateSound(Pos, SOUND_SHOTGUN_FIRE);
}

void VehicleTickJetDriverWeapons(CGameContext *pGS, CGameWorld *pWorld, int DriverId, vec2 Pos, int Team,
	int &BombTicksLeft, int &BombInterval, int &BombCooldown, int &PrevFire, int &ShotgunReload)
{
	CCharacter *pDriver = pGS->GetPlayerChar(DriverId);
	if (!pDriver)
		return;

	if (BombCooldown > 0)
		BombCooldown--;
	if (ShotgunReload > 0)
		ShotgunReload--;

	const CNetObj_PlayerInput &Input = pDriver->GetCore()->m_Input;
	vec2 Dir = normalize(vec2(Input.m_TargetX, Input.m_TargetY));
	if (length(Dir) < 1e-3f)
		Dir = vec2(1.f, 0.f);

	const int Fire = Input.m_Fire;
	const bool FirePress = (Fire & 1) && !(PrevFire & 1);
	PrevFire = Fire;

	if (BombInterval > 0)
		BombInterval--;

	if (BombTicksLeft > 0)
	{
		if (BombInterval <= 0)
		{
			VehicleSpawnFallingGrenade(pGS, pWorld, DriverId, Pos + Dir * VehicleScale(32.f), Dir, Team);
			pGS->CreateSound(Pos, SOUND_GRENADE_FIRE);
			BombInterval = maximum(1, (int)(pGS->Server()->TickSpeed() * 0.2f));
		}
		BombTicksLeft--;
	}

	if (FirePress && BombTicksLeft <= 0 && BombCooldown <= 0)
	{
		BombTicksLeft = pGS->Server()->TickSpeed() * 3;
		BombInterval = 0;
		BombCooldown = pGS->Server()->TickSpeed() * 10;
	}

	if ((Input.m_Hook & 1) && ShotgunReload <= 0)
	{
		VehicleFireJetShotgunPellet(pGS, pWorld, DriverId, Pos, Dir, Team);
		ShotgunReload = maximum(1, pGS->Server()->TickSpeed() / 8);
	}
}

void VehicleBattleFireGun(CGameContext *pGS, CGameWorld *pWorld, int DriverId, vec2 Pos, vec2 Dir, int Team, int &Reload)
{
	if (Reload > 0)
	{
		Reload--;
		return;
	}

	vec2 ProjStartPos = Pos + Dir * VehicleScale(32.f);
	new CProjectile(pWorld, WEAPON_GUN, DriverId, ProjStartPos, Dir,
		(int)(pGS->Server()->TickSpeed() * pGS->Tuning()->m_GunLifetime), 1, 0, 0, -1, WEAPON_GUN, Team);
	pGS->CreateSound(Pos, SOUND_GUN_FIRE);
	Reload = maximum(1, pGS->Server()->TickSpeed() / 10);
}

void VehicleBattleFireHeavyGun(CGameContext *pGS, CGameWorld *pWorld, int DriverId, vec2 Pos, vec2 Dir, int Team, int &Reload)
{
	if (Reload > 0)
	{
		Reload--;
		return;
	}

	new CLaser(pWorld, Pos, Dir, pGS->Tuning()->m_LaserReach, DriverId);
	pGS->CreateSound(Pos, SOUND_RIFLE_FIRE);
	Reload = maximum(1, pGS->Server()->TickSpeed() / 6);
}

void VehicleBattleFireHeavyCannon(CGameContext *pGS, CGameWorld *pWorld, int DriverId, vec2 Pos, vec2 Dir, int Team, int &Reload)
{
	if (Reload > 0)
	{
		Reload--;
		return;
	}

	vec2 ProjStartPos = Pos + Dir * VehicleScale(40.f);
	new CProjectile(pWorld, WEAPON_GRENADE, DriverId, ProjStartPos, Dir * 1.5f,
		(int)(pGS->Server()->TickSpeed() * pGS->Tuning()->m_GrenadeLifetime), 1, true, 0, SOUND_GRENADE_EXPLODE, WEAPON_GRENADE, Team);
	pGS->CreateSound(Pos, SOUND_GRENADE_FIRE);
	Reload = pGS->Server()->TickSpeed();
}

void VehicleTickHelicopterBattleDriver(CGameContext *pGS, CGameWorld *pWorld, int DriverId, vec2 Pos, int Team,
	int &GunReload, int &MissileReload, int &PrevFire, int &PrevJump)
{
	CCharacter *pDriver = pGS->GetPlayerChar(DriverId);
	if (!pDriver)
		return;

	if (MissileReload > 0)
		MissileReload--;

	const CNetObj_PlayerInput &Input = pDriver->GetCore()->m_Input;
	vec2 Dir = normalize(vec2(Input.m_TargetX, Input.m_TargetY));
	if (length(Dir) < 1e-3f)
		Dir = vec2(1.f, 0.f);

	if ((Input.m_Fire & 1) && GunReload <= 0)
		VehicleBattleFireGun(pGS, pWorld, DriverId, Pos, Dir, Team, GunReload);
	PrevFire = Input.m_Fire;

	const int Jump = Input.m_Jump;
	if ((Jump & 1) && !(PrevJump & 1) && MissileReload <= 0)
	{
		VehicleSpawnHomingMissile(pWorld, DriverId, Pos + Dir * VehicleScale(32.f), Dir, Team);
		pGS->CreateSound(Pos, SOUND_RIFLE_FIRE);
		MissileReload = pGS->Server()->TickSpeed();
	}
	PrevJump = Jump;
}

void VehicleTickTankBattleDriver(CGameContext *pGS, CGameWorld *pWorld, int DriverId, vec2 Pos, int Team, int &Reload, int &PrevFire)
{
	CCharacter *pDriver = pGS->GetPlayerChar(DriverId);
	if (!pDriver)
		return;

	const CNetObj_PlayerInput &Input = pDriver->GetCore()->m_Input;
	vec2 Dir = normalize(vec2(Input.m_TargetX, Input.m_TargetY));
	if (length(Dir) < 1e-3f)
		Dir = vec2(1.f, 0.f);

	const int Fire = Input.m_Fire;
	const bool FirePress = (Fire & 1) && !(PrevFire & 1);
	PrevFire = Fire;

	if (FirePress)
		VehicleBattleFireHeavyCannon(pGS, pWorld, DriverId, Pos, Dir, Team, Reload);
	else if (Reload > 0)
		Reload--;
}

void VehicleTickCarBattleDriver(CGameContext *pGS, CGameWorld *pWorld, int DriverId, vec2 Pos, int Team,
	int &GunReload, int &MgReload, int &PrevFire, int &PrevHook)
{
	CCharacter *pDriver = pGS->GetPlayerChar(DriverId);
	if (!pDriver)
		return;

	const CNetObj_PlayerInput &Input = pDriver->GetCore()->m_Input;
	vec2 Dir = normalize(vec2(Input.m_TargetX, Input.m_TargetY));
	if (length(Dir) < 1e-3f)
		Dir = vec2(1.f, 0.f);

	const int Fire = Input.m_Fire;
	if ((Fire & 1) && !(PrevFire & 1))
		VehicleBattleFireGun(pGS, pWorld, DriverId, Pos, Dir, Team, GunReload);
	else if ((Fire & 1) && GunReload <= 0)
		VehicleBattleFireGun(pGS, pWorld, DriverId, Pos, Dir, Team, GunReload);
	PrevFire = Fire;

	if ((Input.m_Hook & 1) && MgReload <= 0)
		VehicleBattleFireHeavyGun(pGS, pWorld, DriverId, Pos, Dir, Team, MgReload);
	else if (MgReload > 0)
		MgReload--;
	PrevHook = Input.m_Hook;
}

void VehicleTickJetBattleDriver(CGameContext *pGS, CGameWorld *pWorld, int DriverId, vec2 Pos, vec2 Dir, int Team,
	int &BombTicksLeft, int &BombInterval, int &BombCooldown, int &GunReload, int &PrevFire, int &PrevHook)
{
	CCharacter *pDriver = pGS->GetPlayerChar(DriverId);
	if (!pDriver)
		return;

	if (BombCooldown > 0)
		BombCooldown--;

	const CNetObj_PlayerInput &Input = pDriver->GetCore()->m_Input;
	if (length(Dir) < 1e-3f)
		Dir = vec2(1.f, 0.f);

	const int Fire = Input.m_Fire;
	const bool FirePress = (Fire & 1) && !(PrevFire & 1);
	PrevFire = Fire;

	if (BombInterval > 0)
		BombInterval--;

	if (BombTicksLeft > 0)
	{
		if (BombInterval <= 0)
		{
			VehicleSpawnFallingGrenade(pGS, pWorld, DriverId, Pos + Dir * VehicleScale(32.f), Dir, Team);
			pGS->CreateSound(Pos, SOUND_GRENADE_FIRE);
			BombInterval = maximum(1, (int)(pGS->Server()->TickSpeed() * 0.2f));
		}
		BombTicksLeft--;
	}

	if (FirePress && BombTicksLeft <= 0 && BombCooldown <= 0)
	{
		BombTicksLeft = pGS->Server()->TickSpeed() * 3;
		BombInterval = 0;
		BombCooldown = pGS->Server()->TickSpeed() * 10;
	}

	if ((Input.m_Hook & 1) && GunReload <= 0)
		VehicleBattleFireGun(pGS, pWorld, DriverId, Pos, Dir, Team, GunReload);
	else if (GunReload > 0)
		GunReload--;

	PrevHook = Input.m_Hook;
}
