#include "battle.h"

#include "entities/buildings.h"
#include "entities/character.h"
#include "entities/laser.h"
#include "entities/projectile.h"
#include "entities/vehicle/vehicle.h"
#include "entities/vehicle/vehicle_util.h"
#include "entities/vehicle/vehicle_weapon.h"
#include "gamecontext.h"
#include "gameworld.h"
#include "gamecontroller.h"
#include "player.h"

#include <engine/shared/config.h>
#include <game/generated/protocol.h>
#include <game/generated/server_data.h>

static const char *s_apClassNames[] = {"Soldier", "Engineer", "Medic", "Sniper"};

static void BattleSnapLaserProjectile(IServer *pServer, int EntityId, vec2 Pos, int StartTick)
{
	CNetObj_Projectile *pProj = static_cast<CNetObj_Projectile *>(pServer->SnapNewItem(NETOBJTYPE_PROJECTILE, EntityId, sizeof(CNetObj_Projectile)));
	if (!pProj)
		return;
	pProj->m_X = (int)Pos.x;
	pProj->m_Y = (int)Pos.y;
	pProj->m_VelX = 0;
	pProj->m_VelY = 0;
	pProj->m_StartTick = StartTick;
	pProj->m_Type = WEAPON_RIFLE;
}

struct CInputCount
{
	int m_Presses;
	int m_Releases;
};

static CInputCount CountInput(int Prev, int Cur)
{
	CInputCount c = {0, 0};
	Prev &= INPUT_STATE_MASK;
	Cur &= INPUT_STATE_MASK;
	int i = Prev;

	while (i != Cur)
	{
		i = (i + 1) & INPUT_STATE_MASK;
		if (i & 1)
			c.m_Presses++;
		else
			c.m_Releases++;
	}

	return c;
}

const char *BattleClassName(int Class)
{
	if (Class < 0 || Class >= NUM_BATTLE_CLASS)
		return "Unknown";
	return s_apClassNames[Class];
}

bool BattleIsEnabled(CGameContext *pGS)
{
	(void)pGS;
	return g_Config.m_SvBattle != 0;
}

bool BattleIsEnabled()
{
	return g_Config.m_SvBattle != 0;
}

bool BattleCanPickupAtPoint(int PointTeam, int PlayerTeam)
{
	if (PointTeam < 0)
		return true;
	return PointTeam == PlayerTeam;
}

void BattleRestorePlayerAmmo(CCharacter *pChr)
{
	if (!pChr)
		return;

	CPlayer *pPlayer = pChr->GetPlayer();
	if (pPlayer->m_BattleClass == BATTLE_SOLDIER)
	{
		pPlayer->m_BattleMagAmmo = BATTLE_SOLDIER_MAG_SIZE;
		pPlayer->m_BattleMagazines = BATTLE_SOLDIER_MAGS;
		pPlayer->m_BattleMagReload = 0;
	}
	for (int w = 0; w < NUM_WEAPONS; w++)
	{
		if (pChr->GotWeapon(w) && pChr->GetWeaponAmmo(w) >= 0)
			pChr->GiveWeapon(w, 50);
	}
}

void BattleOnCharacterDeath(CGameContext *pGS, CCharacter *pVictim)
{
	if (!pGS || !pVictim || !BattleIsEnabled())
		return;

	CVehicle::ForEach(&pGS->m_World, [&](CVehicle *pVehicle) {
		if (!pVehicle->IsOccupiedBy(pVictim->GetPlayer()->GetCID()))
			return;
		if (pVehicle->GetGunner() >= 0 && pVehicle->GetDriver() == pVictim->GetPlayer()->GetCID())
		{
			if (CCharacter *pGunner = pGS->GetPlayerChar(pVehicle->GetGunner()))
				pGunner->Die(pVictim->GetPlayer()->GetCID(), WEAPON_SELF);
			pVehicle->ForceDriverLeave();
			pVehicle->Reset();
		}
	});
}

bool BattleIsInvisibleTo(int SnappingClient, CCharacter *pChr)
{
	if (!pChr || SnappingClient < 0)
		return false;

	CPlayer *pPlayer = pChr->GetPlayer();
	if (!pPlayer || !pPlayer->m_BattleInvisActive || pPlayer->m_BattleInvisEnergy <= 0)
		return false;

	if (SnappingClient == pChr->GetPlayer()->GetCID())
		return false;

	CPlayer *pSnap = pChr->GameServer()->m_apPlayers[SnappingClient];
	if (pSnap && pSnap->GetTeam() == pChr->GetPlayer()->GetTeam())
		return false;

	return true;
}

void BattleApplyLoadout(CCharacter *pChr, int Class)
{
	if (!pChr || Class < 0 || Class >= NUM_BATTLE_CLASS)
		return;

	pChr->BattleClearWeapons();
	CPlayer *pPlayer = pChr->GetPlayer();
	pPlayer->m_BattleGrenadeCount = 1;
	pPlayer->m_BattleSmokeCount = 1;
	pPlayer->m_BattleInvisEnergy = BATTLE_SNIPER_INVIS_ENERGY_MAX;
	pPlayer->m_BattleInvisActive = false;
	pPlayer->m_BattleNinjaRespawnTick = 0;

	// Every class gets hammer (special ability) plus their weapon set.
	pChr->GiveWeapon(WEAPON_HAMMER, -1);

	switch (Class)
	{
	case BATTLE_SOLDIER:
		pChr->GiveWeapon(WEAPON_GUN, -1);
		pChr->GiveWeapon(WEAPON_GRENADE, -1);
		pChr->SetWeapon(WEAPON_GUN);
		pPlayer->m_BattleMagAmmo = BATTLE_SOLDIER_MAG_SIZE;
		pPlayer->m_BattleMagazines = BATTLE_SOLDIER_MAGS;
		pPlayer->m_BattleMagReload = 0;
		break;
	case BATTLE_ENGINEER:
		pChr->GiveWeapon(WEAPON_GUN, -1);
		pChr->GiveWeapon(WEAPON_SHOTGUN, -1);
		pChr->GiveWeapon(WEAPON_GRENADE, -1);
		pChr->SetWeapon(WEAPON_GUN);
		break;
	case BATTLE_MEDIC:
		pChr->GiveWeapon(WEAPON_GUN, -1);
		pChr->GiveWeapon(WEAPON_SHOTGUN, -1);
		pChr->SetWeapon(WEAPON_GUN);
		break;
	case BATTLE_SNIPER:
		pChr->GiveWeapon(WEAPON_GUN, -1);
		pChr->GiveWeapon(WEAPON_RIFLE, -1);
		pChr->GiveWeapon(WEAPON_NINJA, -1);
		pChr->SetWeapon(WEAPON_GUN);
		break;
	}
}

class CC4Charge : public CEntity
{
	int m_Owner;
	int m_Team;
	CCharacter *m_pAttached;
	CVehicle *m_pAttachedVehicle;
	vec2 m_AttachOffset;
	int m_ProjStartTick;

public:
	CC4Charge(CGameWorld *pGameWorld, int Owner, vec2 Pos, int Team, CCharacter *pAttached = nullptr, CVehicle *pAttachedVehicle = nullptr)
		: CEntity(pGameWorld, CGameWorld::ENTTYPE_BATTLE_C4, Pos, 8.f)
	{
		m_Owner = Owner;
		m_Team = Team;
		m_pAttached = pAttached;
		m_pAttachedVehicle = pAttachedVehicle;
		if (m_pAttached)
			m_AttachOffset = Pos - m_pAttached->GetPos();
		else if (m_pAttachedVehicle)
			m_AttachOffset = Pos - m_pAttachedVehicle->GetPos();
		else
			m_AttachOffset = vec2(0.f, 0.f);
		m_ProjStartTick = Server()->Tick();
		GameWorld()->InsertEntity(this);
	}

	void Detonate()
	{
		GameServer()->CreateSound(m_Pos, SOUND_GRENADE_EXPLODE);
		GameServer()->CreateExplosion(m_Pos, m_Owner, WEAPON_GRENADE, false);
		GameWorld()->DestroyEntity(this);
	}

	int Owner() const { return m_Owner; }
	int Team() const { return m_Team; }

	void Tick() override
	{
		if (m_pAttached)
		{
			if (!m_pAttached->IsAlive())
			{
				m_pAttached = nullptr;
				return;
			}
			m_Pos = m_pAttached->GetPos() + m_AttachOffset;
		}
		else if (m_pAttachedVehicle)
		{
			if (m_pAttachedVehicle->IsMarkedForDestroy())
			{
				m_pAttachedVehicle = nullptr;
				return;
			}
			m_Pos = m_pAttachedVehicle->GetPos() + m_AttachOffset;
		}
		m_ProjStartTick = Server()->Tick();
	}

	void Snap(int SnappingClient) override
	{
		if (NetworkClipped(SnappingClient))
			return;
		BattleSnapLaserProjectile(Server(), GetID(), m_Pos, m_ProjStartTick);
	}
};

class CBattleC4Grenade : public CEntity
{
	vec2 m_InitPos;
	vec2 m_Direction;
	int m_StartTick;
	int m_LifeSpan;
	int m_Owner;
	int m_Team;

	vec2 GetPos(float Time) const
	{
		const float Curvature = GameServer()->Tuning()->m_GrenadeCurvature;
		const float Speed = GameServer()->Tuning()->m_GrenadeSpeed;
		return CalcPos(m_InitPos, m_Direction, Curvature, Speed, Time);
	}

	void Stick(vec2 Pos, CCharacter *pAttached, CVehicle *pAttachedVehicle)
	{
		BattleSpawnC4(GameWorld(), m_Owner, Pos, m_Team, pAttached, pAttachedVehicle);
		GameWorld()->DestroyEntity(this);
	}

public:
	CBattleC4Grenade(CGameWorld *pGameWorld, int Owner, vec2 Pos, vec2 Dir, int Team)
		: CEntity(pGameWorld, CGameWorld::ENTTYPE_BATTLE_C4_GRENADE, Pos, 8.f)
	{
		m_Owner = Owner;
		m_Team = Team;
		m_InitPos = Pos;
		m_Direction = Dir;
		m_StartTick = Server()->Tick();
		m_LifeSpan = (int)(Server()->TickSpeed() * GameServer()->Tuning()->m_GrenadeLifetime);
		GameWorld()->InsertEntity(this);
	}

	void Tick() override
	{
		const float Pt = (Server()->Tick() - m_StartTick - 1) / (float)Server()->TickSpeed();
		const float Ct = (Server()->Tick() - m_StartTick) / (float)Server()->TickSpeed();
		const vec2 PrevPos = GetPos(Pt);
		const vec2 CurPos = GetPos(Ct);
		m_Pos = CurPos;

		const int Collide = GameServer()->Collision()->IntersectLine(PrevPos, CurPos, &m_Pos, 0);
		CCharacter *pOwner = GameServer()->GetPlayerChar(m_Owner);
		CCharacter *pTarget = GameWorld()->IntersectCharacter(PrevPos, CurPos, 6.f, m_Pos, pOwner);
		CVehicle *pVehicle = VehicleIntersectLine(GameWorld(), PrevPos, CurPos, 6.f, m_Pos, pOwner);
		CBuilding *pBuilding = GameWorld()->IntersectBuilding(PrevPos, CurPos, 6.f, m_Pos, pOwner);

		m_LifeSpan--;
		if (pTarget || pVehicle || Collide || pBuilding || m_LifeSpan < 0 || GameLayerClipped(CurPos))
			Stick(m_Pos, pTarget, pVehicle);
	}

	void Snap(int SnappingClient) override
	{
		if (NetworkClipped(SnappingClient))
			return;
		BattleSnapLaserProjectile(Server(), GetID(), m_Pos, Server()->Tick());
	}
};

class CLandMine : public CEntity
{
	int m_Owner;
	int m_Team;

public:
	CLandMine(CGameWorld *pGameWorld, int Owner, vec2 Pos, int Team)
		: CEntity(pGameWorld, CGameWorld::ENTTYPE_BATTLE_MINE, Pos, 10.f)
	{
		m_Owner = Owner;
		m_Team = Team;
		GameWorld()->InsertEntity(this);
	}

	void Snap(int SnappingClient) override
	{
		if (NetworkClipped(SnappingClient))
			return;

		CNetObj_Pickup *pPickup = static_cast<CNetObj_Pickup *>(Server()->SnapNewItem(NETOBJTYPE_PICKUP, GetID(), sizeof(CNetObj_Pickup)));
		if (!pPickup)
			return;

		pPickup->m_X = (int)m_Pos.x;
		pPickup->m_Y = (int)m_Pos.y;
		pPickup->m_Type = POWERUP_WEAPON;
		pPickup->m_Subtype = WEAPON_GRENADE;
	}

	void Tick() override
	{
		for (CCharacter *pChr = (CCharacter *)GameWorld()->FindFirst(CGameWorld::ENTTYPE_CHARACTER); pChr; pChr = (CCharacter *)pChr->TypeNext())
		{
			if (!pChr->IsAlive() || pChr->GetPlayer()->GetTeam() == m_Team)
				continue;
			if (distance(pChr->GetPos(), m_Pos) > 20.f)
				continue;

			GameServer()->CreateSound(m_Pos, SOUND_GRENADE_EXPLODE);
			GameServer()->CreateExplosion(m_Pos, m_Owner, WEAPON_GRENADE, false);
			pChr->TakeDamage(vec2(0.f, -1.f), 4, m_Owner, WEAPON_GRENADE);
			GameWorld()->DestroyEntity(this);
			return;
		}
	}
};

void BattleSpawnC4(CGameWorld *pWorld, int Owner, vec2 Pos, int Team, CCharacter *pAttached, CVehicle *pAttachedVehicle)
{
	if (BattleCountC4(pWorld, Owner, Team) >= BATTLE_MAX_C4)
		return;

	new CC4Charge(pWorld, Owner, Pos, Team, pAttached, pAttachedVehicle);
	CGameContext *pGS = pWorld->GameServer();
	if (pGS && Owner >= 0)
	{
		const int Count = BattleCountC4(pWorld, Owner, Team);
		pGS->Chat(Owner, "C4 placed {}/{}!. Press right-mouse to explode!", Count, 3);
	}
}

void BattleSpawnMine(CGameWorld *pWorld, int Owner, vec2 Pos, int Team)
{
	new CLandMine(pWorld, Owner, Pos, Team);
}

void BattleDetonateC4(CGameWorld *pWorld, int Owner, int Team)
{
	for (CEntity *pEnt = pWorld->FindFirst(CGameWorld::ENTTYPE_BATTLE_C4); pEnt;)
	{
		CEntity *pNext = pEnt->TypeNext();
		CC4Charge *pC4 = static_cast<CC4Charge *>(pEnt);
		if (pC4->Team() == Team && (Owner < 0 || pC4->Owner() == Owner))
			pC4->Detonate();
		pEnt = pNext;
	}
}

void BattleDisarmMine(CGameWorld *pWorld, vec2 Pos, float Radius, int Team)
{
	(void)Team;
	CEntity *apEnts[8];
	const int Num = pWorld->FindEntities(Pos, Radius, apEnts, 8, CGameWorld::ENTTYPE_BATTLE_MINE);
	for (int i = 0; i < Num; i++)
	{
		pWorld->DestroyEntity(apEnts[i]);
		pWorld->GameServer()->CreateSound(Pos, SOUND_PICKUP_ARMOR);
	}
}

int BattleCountC4(CGameWorld *pWorld, int Owner, int Team)
{
	int Count = 0;
	for (CEntity *pEnt = pWorld->FindFirst(CGameWorld::ENTTYPE_BATTLE_C4); pEnt; pEnt = pEnt->TypeNext())
	{
		CC4Charge *pC4 = static_cast<CC4Charge *>(pEnt);
		if (pC4->Team() == Team && (Owner < 0 || pC4->Owner() == Owner))
			Count++;
	}
	return Count;
}

int BattleCountMines(CGameWorld *pWorld, int Owner, int Team)
{
	(void)Owner;
	(void)Team;
	int Count = 0;
	for (CEntity *pEnt = pWorld->FindFirst(CGameWorld::ENTTYPE_BATTLE_MINE); pEnt; pEnt = pEnt->TypeNext())
		Count++;
	return Count;
}

static bool BattleSoldierHammer(CCharacter *pChr)
{
	CPlayer *pPlayer = pChr->GetPlayer();
	if (pChr->Server()->Tick() < pPlayer->m_BattleAmmoPackTick)
		return false;

	vec2 Dir = normalize(vec2(pChr->BattleLatestInput().m_TargetX, pChr->BattleLatestInput().m_TargetY));
	if (length(Dir) < 1e-3f)
		Dir = vec2(0.f, -1.f);
	vec2 Pos = pChr->GetPos() + Dir * 24.f;
	BattleSpawnAmmoPack(pChr->GameWorld(), pChr->GetPlayer()->GetCID(), Pos, pChr->GetPlayer()->GetTeam());
	pChr->GameServer()->CreateSound(pChr->GetPos(), SOUND_PICKUP_ARMOR);
	pChr->BattleSetReload(pChr->Server()->TickSpeed() / 3);
	return true;
}

static bool BattleEngineerHammer(CCharacter *pChr)
{
	if (!pChr->IsGrounded())
		return false;
	if (BattleCountMines(pChr->GameWorld(), pChr->GetPlayer()->GetCID(), pChr->GetPlayer()->GetTeam()) >= BATTLE_MAX_MINES)
		return false;

	BattleSpawnMine(pChr->GameWorld(), pChr->GetPlayer()->GetCID(), pChr->GetPos(), pChr->GetPlayer()->GetTeam());
	pChr->GameServer()->CreateHammerHit(pChr->GetPos());
	pChr->BattleSetReload(pChr->Server()->TickSpeed() / 2);
	return true;
}

static bool BattleMedicHammer(CCharacter *pChr)
{
	vec2 Dir = normalize(vec2(pChr->BattleLatestInput().m_TargetX, pChr->BattleLatestInput().m_TargetY));
	if (length(Dir) < 1e-3f)
		Dir = vec2(1.f, 0.f);
	vec2 ProjStartPos = pChr->GetPos() + Dir * pChr->GetProximityRadius() * 0.75f;
	BattleSpawnHeart(pChr->GameWorld(), pChr->GetPlayer()->GetCID(), ProjStartPos, Dir, pChr->GetPlayer()->GetTeam());
	pChr->GameServer()->CreateSound(pChr->GetPos(), SOUND_HAMMER_FIRE);
	pChr->BattleSetReload(pChr->Server()->TickSpeed() / 3);
	return true;
}

static bool BattleSniperHammer(CCharacter *pChr)
{
	CGameContext *pGS = pChr->GameServer();
	CPlayer *pPlayer = pChr->GetPlayer();

	if (pPlayer->m_BattleInvisActive)
	{
		pPlayer->m_BattleInvisActive = false;
		pGS->Chat(pPlayer->GetCID(), "You are no longer invisible.");
		pGS->CreateSound(pChr->GetPos(), SOUND_PICKUP_NINJA);
		pChr->BattleSetReload(pChr->Server()->TickSpeed() / 3);
		return true;
	}

	if (pPlayer->m_BattleInvisEnergy <= 0)
		return false;

	pPlayer->m_BattleInvisActive = true;
	pGS->Chat(pPlayer->GetCID(), "You are now invisible.");
	pGS->Broadcast(pPlayer->GetCID(), "Invisiblepower: {} | {}", pPlayer->m_BattleInvisEnergy, 250);
	pGS->CreateSound(pChr->GetPos(), SOUND_PICKUP_NINJA);
	pChr->BattleSetReload(pChr->Server()->TickSpeed() / 3);
	return true;
}

static bool BattleEngineerGun(CCharacter *pChr)
{
	CGameContext *pGS = pChr->GameServer();
	CPlayer *pPlayer = pChr->GetPlayer();
	vec2 Dir = normalize(vec2(pChr->BattleLatestInput().m_TargetX, pChr->BattleLatestInput().m_TargetY));
	if (length(Dir) < 1e-3f)
		Dir = vec2(1.f, 0.f);

	vec2 At = pChr->GetPos() + Dir * pChr->GetProximityRadius();
	const bool HeldLong = pPlayer->m_BattleEngineerFiring &&
		pChr->Server()->Tick() - pPlayer->m_BattleEngineerFireStart > pChr->Server()->TickSpeed() / 3;

	if (!HeldLong)
	{
		pGS->CreateSound(pChr->GetPos(), SOUND_GUN_FIRE);
		pChr->BattleSetReload(pChr->Server()->TickSpeed() / 8);
		return true;
	}

	BattleDisarmMine(pChr->GameWorld(), At, 32.f, pChr->GetPlayer()->GetTeam());
	pGS->m_pController->SwitchDoorAt(At, pChr->GetPlayer(), true);

	CVehicle::ForEach(&pGS->m_World, [&](CVehicle *pVehicle) {
		if (distance(pVehicle->GetPos(), At) > 80.f)
			return;
		if (pVehicle->GetTeam() == pChr->GetPlayer()->GetTeam())
			pVehicle->Repair(5);
		else
			pVehicle->TakeDamage(5, pChr->GetPlayer()->GetCID());
	});

	for (CCharacter *pTarget = (CCharacter *)pGS->m_World.FindFirst(CGameWorld::ENTTYPE_CHARACTER); pTarget; pTarget = (CCharacter *)pTarget->TypeNext())
	{
		if (!pTarget->IsAlive() || pTarget->GetPlayer()->GetTeam() == pChr->GetPlayer()->GetTeam())
			continue;
		if (distance(pTarget->GetPos(), At) > 48.f)
			continue;
		pTarget->TakeDamage(Dir, 1, pChr->GetPlayer()->GetCID(), WEAPON_GUN);
	}

	pGS->CreateSound(pChr->GetPos(), SOUND_GUN_FIRE);
	pChr->BattleSetReload(pChr->Server()->TickSpeed() / 4);
	return true;
}

static bool BattleFireSoldier(CCharacter *pChr)
{
	CGameContext *pGS = pChr->GameServer();
	vec2 Dir = normalize(vec2(pChr->BattleLatestInput().m_TargetX, pChr->BattleLatestInput().m_TargetY));
	if (length(Dir) < 1e-3f)
		Dir = vec2(1.f, 0.f);
	vec2 ProjStartPos = pChr->GetPos() + Dir * pChr->GetProximityRadius() * 0.75f;

	switch (pChr->GetActiveWeapon())
	{
	case WEAPON_GUN:
	{
		CPlayer *pPlayer = pChr->GetPlayer();
		if (pPlayer->m_BattleMagReload > 0)
			return false;
		if (pPlayer->m_BattleMagAmmo <= 0)
		{
			if (pPlayer->m_BattleMagazines <= 0)
				return false;
			pPlayer->m_BattleMagazines--;
			pPlayer->m_BattleMagAmmo = BATTLE_SOLDIER_MAG_SIZE;
			pPlayer->m_BattleMagReload = pChr->Server()->TickSpeed();
			return false;
		}

		new CProjectile(pChr->GameWorld(), WEAPON_GUN, pChr->GetPlayer()->GetCID(), ProjStartPos, Dir,
			(int)(pChr->Server()->TickSpeed() * pGS->Tuning()->m_GunLifetime), 1, 0, 0, -1, WEAPON_GUN, pChr->GetPlayer()->GetTeam());
		pGS->CreateSound(pChr->GetPos(), SOUND_GUN_FIRE);
		pPlayer->m_BattleMagAmmo--;
		pChr->BattleSetReload(maximum(1, pChr->Server()->TickSpeed() / 12));
		return true;
	}
	case WEAPON_GRENADE:
		if (BattleCountC4(pChr->GameWorld(), pChr->GetPlayer()->GetCID(), pChr->GetPlayer()->GetTeam()) >= BATTLE_MAX_C4)
			return false;
		new CBattleC4Grenade(pChr->GameWorld(), pChr->GetPlayer()->GetCID(), ProjStartPos, Dir, pChr->GetPlayer()->GetTeam());
		pGS->CreateSound(pChr->GetPos(), SOUND_GRENADE_FIRE);
		pChr->BattleSetReload(pChr->Server()->TickSpeed() / 2);
		return true;
	default:
		return false;
	}
}

static bool BattleFireEngineer(CCharacter *pChr)
{
	CGameContext *pGS = pChr->GameServer();
	vec2 Dir = normalize(vec2(pChr->BattleLatestInput().m_TargetX, pChr->BattleLatestInput().m_TargetY));
	if (length(Dir) < 1e-3f)
		Dir = vec2(1.f, 0.f);

	switch (pChr->GetActiveWeapon())
	{
	case WEAPON_GUN:
		return BattleEngineerGun(pChr);
	case WEAPON_SHOTGUN:
	{
		vec2 ProjStartPos = pChr->GetPos() + Dir * pChr->GetProximityRadius() * 0.75f;
		const int ShotSpread = 2;
		for (int i = -ShotSpread; i <= ShotSpread; ++i)
		{
			float Spreading[] = {-0.185f, -0.070f, 0, 0.070f, 0.185f};
			float a = GetAngle(Dir) + Spreading[i + 2];
			float v = 1 - (absolute(i) / (float)ShotSpread);
			float Speed = mix((float)pGS->Tuning()->m_ShotgunSpeeddiff, 1.0f, v);
			new CProjectile(pChr->GameWorld(), WEAPON_SHOTGUN, pChr->GetPlayer()->GetCID(), ProjStartPos, vec2(cosf(a), sinf(a)) * Speed,
				(int)(pChr->Server()->TickSpeed() * pGS->Tuning()->m_ShotgunLifetime), 1, 0, 0, -1, WEAPON_SHOTGUN, pChr->GetPlayer()->GetTeam());
		}
		pGS->CreateSound(pChr->GetPos(), SOUND_SHOTGUN_FIRE);
		pChr->BattleSetReload(pChr->Server()->TickSpeed() / 2);
		return true;
	}
	case WEAPON_GRENADE:
	{
		if (pChr->BattleInput().m_Direction < 0)
			Dir = rotate(Dir, -0.25f);
		else if (pChr->BattleInput().m_Direction > 0)
			Dir = rotate(Dir, 0.25f);
		vec2 ProjStartPos = pChr->GetPos() + Dir * pChr->GetProximityRadius() * 0.75f;
		VehicleSpawnHomingMissile(pChr->GameWorld(), pChr->GetPlayer()->GetCID(), ProjStartPos, Dir, pChr->GetPlayer()->GetTeam());
		pGS->CreateSound(pChr->GetPos(), SOUND_GRENADE_FIRE);
		pChr->BattleSetReload(pChr->Server()->TickSpeed());
		return true;
	}
	default:
		return false;
	}
}

static bool BattleFireMedic(CCharacter *pChr)
{
	CGameContext *pGS = pChr->GameServer();
	vec2 Dir = normalize(vec2(pChr->BattleLatestInput().m_TargetX, pChr->BattleLatestInput().m_TargetY));
	if (length(Dir) < 1e-3f)
		Dir = vec2(1.f, 0.f);
	vec2 ProjStartPos = pChr->GetPos() + Dir * pChr->GetProximityRadius() * 0.75f;

	switch (pChr->GetActiveWeapon())
	{
	case WEAPON_GUN:
	{
		if (!CountInput(pChr->BattleLatestPrevInput().m_Fire, pChr->BattleLatestInput().m_Fire).m_Presses)
			return false;
		new CProjectile(pChr->GameWorld(), WEAPON_GUN, pChr->GetPlayer()->GetCID(), ProjStartPos, Dir,
			(int)(pChr->Server()->TickSpeed() * pGS->Tuning()->m_GunLifetime), 1, 0, 0, -1, WEAPON_GUN, pChr->GetPlayer()->GetTeam());
		pGS->CreateSound(pChr->GetPos(), SOUND_GUN_FIRE);
		pChr->BattleSetReload(pChr->Server()->TickSpeed() / 3);
		return true;
	}
	case WEAPON_SHOTGUN:
	{
		const int ShotSpread = 3;
		for (int i = -ShotSpread; i <= ShotSpread; ++i)
		{
			float Spreading[] = {-0.25f, -0.12f, -0.05f, 0, 0.05f, 0.12f, 0.25f};
			float a = GetAngle(Dir) + Spreading[i + 3];
			float v = 1 - (absolute(i) / (float)ShotSpread);
			float Speed = mix((float)pGS->Tuning()->m_ShotgunSpeeddiff, 1.0f, v);
			new CProjectile(pChr->GameWorld(), WEAPON_SHOTGUN, pChr->GetPlayer()->GetCID(), ProjStartPos, vec2(cosf(a), sinf(a)) * Speed,
				(int)(pChr->Server()->TickSpeed() * pGS->Tuning()->m_ShotgunLifetime), 1, 0, 0, -1, WEAPON_SHOTGUN, pChr->GetPlayer()->GetTeam());
		}
		pGS->CreateSound(pChr->GetPos(), SOUND_SHOTGUN_FIRE);
		pChr->BattleSetReload(pChr->Server()->TickSpeed() / 2);
		return true;
	}
	default:
		return false;
	}
}

static bool BattleFireSniper(CCharacter *pChr)
{
	CGameContext *pGS = pChr->GameServer();
	CPlayer *pPlayer = pChr->GetPlayer();
	vec2 Dir = normalize(vec2(pChr->BattleLatestInput().m_TargetX, pChr->BattleLatestInput().m_TargetY));
	if (length(Dir) < 1e-3f)
		Dir = vec2(1.f, 0.f);
	vec2 ProjStartPos = pChr->GetPos() + Dir * pChr->GetProximityRadius() * 0.75f;

	switch (pChr->GetActiveWeapon())
	{
	case WEAPON_GUN:
	{
		new CProjectile(pChr->GameWorld(), WEAPON_GUN, pChr->GetPlayer()->GetCID(), ProjStartPos, Dir,
			(int)(pChr->Server()->TickSpeed() * pGS->Tuning()->m_GunLifetime), 1, 0, 0, -1, WEAPON_GUN, pChr->GetPlayer()->GetTeam());
		pGS->CreateSound(pChr->GetPos(), SOUND_GUN_FIRE);
		pChr->BattleSetReload(maximum(1, g_pData->m_Weapons.m_aId[WEAPON_GUN].m_Firedelay * pChr->Server()->TickSpeed() / 1000));
		return true;
	}
	case WEAPON_RIFLE:
	{
		new CLaser(pChr->GameWorld(), pChr->GetPos(), Dir, pGS->Tuning()->m_LaserReach, pChr->GetPlayer()->GetCID());
		pGS->CreateSound(pChr->GetPos(), SOUND_RIFLE_FIRE);
		pChr->BattleSetReload(maximum(1, pChr->Server()->TickSpeed() * BATTLE_SNIPER_LASER_RELOAD / 10));
		return true;
	}
	case WEAPON_NINJA:
	{
		if (!pChr->GotWeapon(WEAPON_NINJA))
			return false;

		vec2 At = pChr->GetPos() + Dir * pChr->GetProximityRadius();
		for (CCharacter *pTarget = (CCharacter *)pChr->GameWorld()->FindFirst(CGameWorld::ENTTYPE_CHARACTER); pTarget; pTarget = (CCharacter *)pTarget->TypeNext())
		{
			if (!pTarget->IsAlive() || pTarget->GetPlayer()->GetTeam() == pChr->GetPlayer()->GetTeam())
				continue;
			if (distance(pTarget->GetPos(), At) > pChr->GetProximityRadius() * 1.25f)
				continue;

			pGS->CreateSound(pTarget->GetPos(), SOUND_NINJA_HIT);
			pTarget->Die(pChr->GetPlayer()->GetCID(), WEAPON_NINJA);
			break;
		}

		pChr->BattleRemoveWeapon(WEAPON_NINJA);
		if (pChr->GetActiveWeapon() == WEAPON_NINJA)
			pChr->SetWeapon(WEAPON_GUN);
		pPlayer->m_BattleNinjaRespawnTick = pChr->Server()->Tick() + pChr->Server()->TickSpeed() * BATTLE_SNIPER_NINJA_COOLDOWN;
		pGS->CreateSound(pChr->GetPos(), SOUND_NINJA_FIRE);
		pChr->BattleSetReload(1);
		return true;
	}
	default:
		return false;
	}
}

bool BattleHammer(CCharacter *pChr)
{
	if (!pChr || !BattleIsEnabled())
		return false;

	const int Class = pChr->GetPlayer()->m_BattleClass;
	if (Class < 0 || Class >= NUM_BATTLE_CLASS)
		return false;

	if (pChr->GetActiveWeapon() != WEAPON_HAMMER)
		return false;

	if (!CountInput(pChr->BattleLatestPrevInput().m_Fire, pChr->BattleLatestInput().m_Fire).m_Presses)
		return false;

	switch (Class)
	{
	case BATTLE_SOLDIER: return BattleSoldierHammer(pChr);
	case BATTLE_ENGINEER: return BattleEngineerHammer(pChr);
	case BATTLE_MEDIC: return BattleMedicHammer(pChr);
	case BATTLE_SNIPER: return BattleSniperHammer(pChr);
	}
	return false;
}

bool BattleFireWeapon(CCharacter *pChr)
{
	if (!pChr || !BattleIsEnabled())
		return false;

	const int Class = pChr->GetPlayer()->m_BattleClass;
	if (Class < 0 || Class >= NUM_BATTLE_CLASS)
		return false;

	if (pChr->GetActiveWeapon() == WEAPON_HAMMER)
		return false;

	bool WillFire = false;
	if (CountInput(pChr->BattleLatestPrevInput().m_Fire, pChr->BattleLatestInput().m_Fire).m_Presses)
		WillFire = true;

	const bool FullAuto = Class == BATTLE_SOLDIER && pChr->GetActiveWeapon() == WEAPON_GUN;
	if (FullAuto && (pChr->BattleLatestInput().m_Fire & 1))
		WillFire = true;

	if (!WillFire)
		return false;

	switch (Class)
	{
	case BATTLE_SOLDIER: return BattleFireSoldier(pChr);
	case BATTLE_ENGINEER: return BattleFireEngineer(pChr);
	case BATTLE_MEDIC: return BattleFireMedic(pChr);
	case BATTLE_SNIPER: return BattleFireSniper(pChr);
	}
	return false;
}

bool BattleHook(CCharacter *pChr)
{
	if (!pChr || !BattleIsEnabled())
		return false;

	if (pChr->GetPlayer()->m_BattleClass != BATTLE_SOLDIER)
		return false;

	if (pChr->GetActiveWeapon() != WEAPON_GRENADE)
		return false;

	if (!CountInput(pChr->BattlePrevInput().m_Hook, pChr->BattleInput().m_Hook).m_Presses)
		return false;

	if (BattleCountC4(pChr->GameWorld(), pChr->GetPlayer()->GetCID(), pChr->GetPlayer()->GetTeam()) <= 0)
		return false;

	BattleDetonateC4(pChr->GameWorld(), pChr->GetPlayer()->GetCID(), pChr->GetPlayer()->GetTeam());
	pChr->GameServer()->CreateSound(pChr->GetPos(), SOUND_GRENADE_EXPLODE);
	return true;
}

void BattleTickCharacter(CCharacter *pChr)
{
	if (!pChr || !BattleIsEnabled())
		return;

	CGameContext *pGS = pChr->GameServer();
	CPlayer *pPlayer = pChr->GetPlayer();
	if (pPlayer->m_BattleClass == BATTLE_SNIPER)
	{
		if (pPlayer->m_BattleInvisActive)
		{
			if (pPlayer->m_BattleInvisEnergy > 0)
				pPlayer->m_BattleInvisEnergy--;
			pGS->Broadcast(pPlayer->GetCID(), "Invisiblepower: {} | {}", pPlayer->m_BattleInvisEnergy, 250);
			if (pPlayer->m_BattleInvisEnergy <= 0)
			{
				pPlayer->m_BattleInvisActive = false;
				pGS->Chat(pPlayer->GetCID(), "You are no longer invisible.");
			}
		}
		else if (pPlayer->m_BattleInvisEnergy < BATTLE_SNIPER_INVIS_ENERGY_MAX)
		{
			if (pChr->Server()->Tick() % (pChr->Server()->TickSpeed() / 4) == 0)
				pPlayer->m_BattleInvisEnergy = minimum(250, pPlayer->m_BattleInvisEnergy + 5);
		}

		if (!pChr->GotWeapon(WEAPON_NINJA) && pPlayer->m_BattleNinjaRespawnTick > 0 &&
			pChr->Server()->Tick() >= pPlayer->m_BattleNinjaRespawnTick)
		{
			pChr->GiveWeapon(WEAPON_NINJA, -1);
			pPlayer->m_BattleNinjaRespawnTick = 0;
		}
	}

	if (pPlayer->m_BattleMagReload > 0)
		pPlayer->m_BattleMagReload--;

	if (pPlayer->m_BattleClass == BATTLE_ENGINEER && pChr->GetActiveWeapon() == WEAPON_GUN)
	{
		const bool FireHeld = (pChr->BattleLatestInput().m_Fire & 1) != 0;
		if (FireHeld && !pPlayer->m_BattleEngineerFiring)
		{
			pPlayer->m_BattleEngineerFiring = true;
			pPlayer->m_BattleEngineerFireStart = pChr->Server()->Tick();
		}
		else if (!FireHeld)
			pPlayer->m_BattleEngineerFiring = false;
	}
}

void BattleHandleEmote(CGameContext *pGS, int ClientId)
{
	VehicleHandleEmoteDismount(pGS, ClientId);
}

void BattleFireThrowable(CGameContext *pGS, CCharacter *pChr, int Type)
{
	vec2 Dir = normalize(vec2(pChr->BattleLatestInput().m_TargetX, pChr->BattleLatestInput().m_TargetY));
	if (length(Dir) < 1e-3f)
		Dir = vec2(1.f, 0.f);
	vec2 ProjStartPos = pChr->GetPos() + Dir * pChr->GetProximityRadius() * 0.75f;

	switch (Type)
	{
	case BATTLE_THROW_GRENADE:
		new CProjectile(pChr->GameWorld(), WEAPON_GRENADE, pChr->GetPlayer()->GetCID(), ProjStartPos, Dir,
			(int)(pChr->Server()->TickSpeed() * pGS->Tuning()->m_GrenadeLifetime), 1, true, 0, SOUND_GRENADE_EXPLODE, WEAPON_GRENADE, pChr->GetPlayer()->GetTeam());
		break;
	case BATTLE_THROW_SMOKE:
		BattleSpawnSmoke(pChr->GameWorld(), ProjStartPos + Dir * 40.f);
		break;
	case BATTLE_THROW_WATER:
		new CProjectile(pChr->GameWorld(), WEAPON_GRENADE, pChr->GetPlayer()->GetCID(), ProjStartPos, Dir,
			(int)(pChr->Server()->TickSpeed() * 2), 1, true, 0, SOUND_GRENADE_EXPLODE, WEAPON_GRENADE, pChr->GetPlayer()->GetTeam());
		break;
	default:
		return;
	}

	pGS->CreateSound(pChr->GetPos(), SOUND_GRENADE_FIRE);
}

void BattleHandleECommand(CGameContext *pGS, CPlayer *pPlayer)
{
	if (!pPlayer || !BattleIsEnabled(pGS))
		return;

	CCharacter *pChr = pPlayer->GetCharacter();
	if (pChr && pChr->m_OnVehicle)
	{
		BattleHandleEmote(pGS, pPlayer->GetCID());
		return;
	}

	if (!pChr || pPlayer->m_BattleThrowableCount <= 0)
	{
		if (pPlayer->m_BattleSmokeCount > 0 || pPlayer->m_BattleGrenadeCount > 0)
		{
			if (length(pChr->GetVel()) < 1.f)
			{
				pGS->Chat(pPlayer->GetCID(), "Move while using /e.");
				return;
			}
			if (pPlayer->m_BattleSmokeCount > 0)
			{
				BattleFireThrowable(pGS, pChr, BATTLE_THROW_SMOKE);
				pPlayer->m_BattleSmokeCount--;
			}
			else
			{
				BattleFireThrowable(pGS, pChr, BATTLE_THROW_GRENADE);
				pPlayer->m_BattleGrenadeCount--;
			}
			return;
		}
		if (pChr && pChr->m_OnVehicle)
			BattleHandleEmote(pGS, pPlayer->GetCID());
		else
			pGS->Chat(pPlayer->GetCID(), "Nothing to use.");
		return;
	}

	BattleFireThrowable(pGS, pChr, pPlayer->m_BattleThrowableType);
	pPlayer->m_BattleThrowableCount--;
	if (pPlayer->m_BattleThrowableCount <= 0)
		pPlayer->m_BattleThrowableType = BATTLE_THROW_NONE;
}

bool BattlePickupThrowable(CCharacter *pChr, int PickupSubtype)
{
	if (!pChr || !BattleIsEnabled())
		return false;

	CPlayer *pPlayer = pChr->GetPlayer();
	switch (PickupSubtype)
	{
	case WEAPON_GRENADE:
		pPlayer->m_BattleThrowableType = BATTLE_THROW_GRENADE;
		pPlayer->m_BattleThrowableCount = minimum(3, pPlayer->m_BattleThrowableCount + 1);
		return true;
	case WEAPON_SHOTGUN:
		pPlayer->m_BattleThrowableType = BATTLE_THROW_SMOKE;
		pPlayer->m_BattleThrowableCount = minimum(3, pPlayer->m_BattleThrowableCount + 1);
		return true;
	case WEAPON_RIFLE:
		pPlayer->m_BattleThrowableType = BATTLE_THROW_WATER;
		pPlayer->m_BattleThrowableCount = minimum(3, pPlayer->m_BattleThrowableCount + 1);
		return true;
	default:
		return false;
	}
}
