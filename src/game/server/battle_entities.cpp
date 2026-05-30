#include "battle.h"

#include "entities/area-flag.h"
#include "entities/character.h"
#include "entities/projectile.h"
#include "gamecontroller.h"
#include "gamecontext.h"
#include "gameworld.h"
#include "player.h"

#include <engine/shared/config.h>
#include <game/generated/protocol.h>
#include <game/mapitems.h>

class CBattleHeart : public CEntity
{
public:
	CBattleHeart(CGameWorld *pWorld, int Owner, vec2 Pos, vec2 Dir, int Team, bool FromStation = false)
		: CEntity(pWorld, CGameWorld::ENTTYPE_BATTLE_HEART, Pos, 8.f)
	{
		m_Owner = Owner;
		m_Team = Team;
		m_Dir = normalize(Dir);
		m_Life = Server()->TickSpeed() * 2;
		(void)FromStation;
		GameWorld()->InsertEntity(this);
	}

	void Tick() override
	{
		vec2 Prev = m_Pos;
		m_Pos += m_Dir * 10.f;
		m_Life--;

		CCharacter *pHit = GameWorld()->IntersectCharacter(Prev, m_Pos, 8.f, m_Pos, nullptr);
		if (pHit || m_Life <= 0)
		{
			if (pHit)
			{
				if (pHit->GetPlayer()->GetTeam() == m_Team)
					pHit->IncreaseHealth(2);
				else
					pHit->TakeDamage(m_Dir, 2, m_Owner, WEAPON_HAMMER);
			}
			GameWorld()->DestroyEntity(this);
		}
	}

	void Snap(int SnappingClient) override
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
		pProj->m_Type = WEAPON_HAMMER;
	}

private:
	int m_Owner;
	int m_Team;
	vec2 m_Dir;
	int m_Life;
};

class CBattleSmoke : public CEntity
{
public:
	CBattleSmoke(CGameWorld *pWorld, vec2 Pos)
		: CEntity(pWorld, CGameWorld::ENTTYPE_BATTLE_SMOKE, Pos, 120.f)
	{
		m_Life = Server()->TickSpeed() * 15;
		GameWorld()->InsertEntity(this);
	}

	void Tick() override
	{
		if (--m_Life <= 0)
			GameWorld()->DestroyEntity(this);
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
		pPickup->m_Type = POWERUP_ARMOR;
		pPickup->m_Subtype = 0;
	}

private:
	int m_Life;
};

class CBattleAAGun : public CEntity
{
public:
	CBattleAAGun(CGameWorld *pWorld, vec2 Pos)
		: CEntity(pWorld, CGameWorld::ENTTYPE_BATTLE_AA_GUN, Pos, 40.f)
	{
		m_Operator = -1;
		m_Reload = 0;
		GameWorld()->InsertEntity(this);
	}

	void Tick() override
	{
		if (m_Operator < 0)
		{
			CCharacter *pChr = GameWorld()->ClosestCharacter(m_Pos, 40.f, this);
			if (pChr && !pChr->m_OnVehicle)
				m_Operator = pChr->GetPlayer()->GetCID();
			return;
		}

		CCharacter *pOp = GameServer()->GetPlayerChar(m_Operator);
		if (!pOp || !pOp->IsAlive() || distance(pOp->GetPos(), m_Pos) > 48.f)
		{
			m_Operator = -1;
			return;
		}

		pOp->GetCore()->m_Pos = m_Pos;
		pOp->GetCore()->m_Vel = vec2(0.f, 0.f);

		if (m_Reload > 0)
		{
			m_Reload--;
			return;
		}

		if (!(pOp->GetCore()->m_Input.m_Fire & 1))
			return;

		vec2 Dir = normalize(vec2(pOp->GetCore()->m_Input.m_TargetX, pOp->GetCore()->m_Input.m_TargetY));
		if (length(Dir) < 1e-3f)
			Dir = vec2(1.f, 0.f);

		new CProjectile(GameWorld(), WEAPON_GUN, m_Operator, m_Pos, Dir,
			(int)(Server()->TickSpeed() * GameServer()->Tuning()->m_GunLifetime), 1, 0, 0, -1, WEAPON_GUN, pOp->GetPlayer()->GetTeam());
		GameServer()->CreateSound(m_Pos, SOUND_GUN_FIRE);
		m_Reload = Server()->TickSpeed() / 8;
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
		pPickup->m_Subtype = WEAPON_RIFLE;
	}

private:
	int m_Operator;
	int m_Reload;
};

class CBattleAmmoPack : public CEntity
{
public:
	CBattleAmmoPack(CGameWorld *pWorld, int Owner, vec2 Pos, int Team)
		: CEntity(pWorld, CGameWorld::ENTTYPE_BATTLE_AMMO_PACK, Pos, 24.f)
	{
		m_Owner = Owner;
		m_Team = Team;
		m_Life = Server()->TickSpeed() * 30;
		GameWorld()->InsertEntity(this);
	}

	void Tick() override
	{
		if (--m_Life <= 0)
		{
			GameWorld()->DestroyEntity(this);
			return;
		}

		for (CCharacter *pChr = (CCharacter *)GameWorld()->FindFirst(CGameWorld::ENTTYPE_CHARACTER); pChr; pChr = (CCharacter *)pChr->TypeNext())
		{
			if (!pChr->IsAlive() || distance(pChr->GetPos(), m_Pos) > 24.f)
				continue;
			if (pChr->GetPlayer()->GetTeam() != m_Team)
				continue;

			BattleRestorePlayerAmmo(pChr);
			if (pChr->GetPlayer()->GetCID() == m_Owner)
			{
				CPlayer *pOwner = GameServer()->m_apPlayers[m_Owner];
				if (pOwner && Server()->Tick() < pOwner->m_BattleAmmoPackTick)
					continue;
				if (pOwner)
					pOwner->m_BattleAmmoPackTick = Server()->Tick() + Server()->TickSpeed() * BATTLE_SOLDIER_AMMO_PACK_CD;
			}
			GameServer()->CreateSound(m_Pos, SOUND_PICKUP_ARMOR);
			GameWorld()->DestroyEntity(this);
			return;
		}
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
		pPickup->m_Subtype = WEAPON_HAMMER;
	}

private:
	int m_Owner;
	int m_Team;
	int m_Life;
};

bool BattleOnMapEntity(int Index, vec2 Pos, CGameContext *pGS, CGameControllerWorkbenches *pCtrl)
{
	if (!pGS || !pCtrl)
		return false;

	switch (Index)
	{
	case ENTITY_BATTLE_AAGUN:
		new CBattleAAGun(&pGS->m_World, Pos);
		return true;
	default:
		return false;
	}
}

bool BattleIsInSmoke(CCharacter *pChr)
{
	if (!pChr)
		return false;

	for (CEntity *pEnt = pChr->GameWorld()->FindFirst(CGameWorld::ENTTYPE_BATTLE_SMOKE); pEnt; pEnt = pEnt->TypeNext())
	{
		if (distance(pEnt->GetPos(), pChr->GetPos()) < 100.f)
			return true;
	}
	return false;
}

void BattleSpawnSmoke(CGameWorld *pWorld, vec2 Pos)
{
	new CBattleSmoke(pWorld, Pos);
}

void BattleSpawnAmmoPack(CGameWorld *pWorld, int Owner, vec2 Pos, int Team)
{
	new CBattleAmmoPack(pWorld, Owner, Pos, Team);
}

void BattleSpawnHeart(CGameWorld *pWorld, int Owner, vec2 Pos, vec2 Dir, int Team)
{
	new CBattleHeart(pWorld, Owner, Pos, Dir, Team);
}
