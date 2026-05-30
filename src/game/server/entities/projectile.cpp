/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include "buildings.h"
#include "projectile.h"
#include <game/server/entities/vehicle/vehicle.h>
#include <game/server/entities/vehicle/vehicle_util.h>

CProjectile::CProjectile(CGameWorld *pGameWorld, int Type, int Owner, vec2 Pos, vec2 Dir, int Span,
						 int Damage, bool Explosive, float Force, int SoundImpact, int Weapon, int Team)
	: CEntity(pGameWorld, CGameWorld::ENTTYPE_PROJECTILE, Pos)
{
	m_Type = Type;
	m_InitPos = Pos;
	m_Pos = Pos;
	m_Direction = Dir;
	m_LifeSpan = Span;
	m_Owner = Owner;
	m_Force = Force;
	m_Damage = Damage;
	m_SoundImpact = SoundImpact;
	m_Weapon = Weapon;
	m_StartTick = Server()->Tick();
	m_Explosive = Explosive;
	m_Team = Team;

	GameWorld()->InsertEntity(this);
}

void CProjectile::Reset()
{
	GameServer()->m_World.DestroyEntity(this);
}

vec2 CProjectile::GetPos(float Time)
{
	float Curvature = 0;
	float Speed = 0;

	switch (m_Type)
	{
	case WEAPON_GRENADE:
		Curvature = GameServer()->Tuning()->m_GrenadeCurvature;
		Speed = GameServer()->Tuning()->m_GrenadeSpeed;
		break;

	case WEAPON_SHOTGUN:
		Curvature = GameServer()->Tuning()->m_ShotgunCurvature;
		Speed = GameServer()->Tuning()->m_ShotgunSpeed;
		break;

	case WEAPON_GUN:
		Curvature = GameServer()->Tuning()->m_GunCurvature;
		Speed = GameServer()->Tuning()->m_GunSpeed;
		break;
	}

	return CalcPos(m_InitPos, m_Direction, Curvature, Speed, Time);
}

void CProjectile::Tick()
{
	float Pt = (Server()->Tick() - m_StartTick - 1) / (float)Server()->TickSpeed();
	float Ct = (Server()->Tick() - m_StartTick) / (float)Server()->TickSpeed();
	vec2 PrevPos = GetPos(Pt);
	vec2 CurPos = GetPos(Ct);
	m_Pos = CurPos;

	int Collide = GameServer()->Collision()->IntersectLine(PrevPos, CurPos, &CurPos, 0);
	CCharacter *OwnerChar = GameServer()->GetPlayerChar(m_Owner);
	CCharacter *TargetChr = GameServer()->m_World.IntersectCharacter(PrevPos, CurPos, 6.0f, CurPos, OwnerChar);

	CBuilding *TargetBuilding = 0;
	TargetBuilding = GameServer()->m_World.IntersectBuilding(PrevPos, CurPos, 6.0f, CurPos, OwnerChar);

	CVehicle *TargetVehicle = VehicleIntersectLine(&GameServer()->m_World, PrevPos, CurPos, 6.0f, CurPos, OwnerChar);

	m_LifeSpan--;

	if (TargetChr || TargetVehicle || Collide || m_LifeSpan < 0 || GameLayerClipped(CurPos) || (TargetBuilding && TargetBuilding->GetTeam() != m_Team))
	{
		if (m_LifeSpan >= 0 || m_Weapon == WEAPON_GRENADE)
			GameServer()->CreateSound(CurPos, m_SoundImpact);

		if (m_Explosive)
			GameServer()->CreateExplosion(CurPos, m_Owner, m_Weapon, false);

		if ((TargetChr && TargetChr->GetPlayer()->GetTeam() != m_Team)
				|| (TargetBuilding && TargetBuilding->GetTeam() != m_Team)
				|| (TargetVehicle && TargetVehicle->GetTeam() != m_Team))
		{
			if (TargetChr && !TargetBuilding && !TargetVehicle)
				TargetChr->TakeDamage(m_Direction * max(0.001f, m_Force), m_Damage, m_Owner, m_Weapon);
			else if (!TargetChr && TargetBuilding && !TargetVehicle)
				TargetBuilding->TakeDamageAt(CurPos, m_Damage, m_Owner, m_Weapon);
			else if (!TargetChr && !TargetBuilding && TargetVehicle)
				TargetVehicle->TakeDamage(m_Damage, m_Owner);
			else
			{
				const float ChrDist = TargetChr ? distance(m_Pos, TargetChr->GetPos()) : 1e30f;
				const float BuildingDist = TargetBuilding ? distance(m_Pos, TargetBuilding->GetDamageCenter()) : 1e30f;
				const float VehicleDist = TargetVehicle ? distance(m_Pos, TargetVehicle->GetPos()) : 1e30f;
				if (ChrDist <= BuildingDist && ChrDist <= VehicleDist)
					TargetChr->TakeDamage(m_Direction * max(0.001f, m_Force), m_Damage, m_Owner, m_Weapon);
				else if (BuildingDist <= VehicleDist)
					TargetBuilding->TakeDamageAt(CurPos, m_Damage, m_Owner, m_Weapon);
				else
					TargetVehicle->TakeDamage(m_Damage, m_Owner);
			}
		}

		GameServer()->m_World.DestroyEntity(this);
	}
}

void CProjectile::TickPaused()
{
	++m_StartTick;
}

void CProjectile::FillInfo(CNetObj_Projectile *pProj)
{
	pProj->m_X = (int)m_InitPos.x;
	pProj->m_Y = (int)m_InitPos.y;
	pProj->m_VelX = (int)(m_Direction.x * 100.0f);
	pProj->m_VelY = (int)(m_Direction.y * 100.0f);
	pProj->m_StartTick = m_StartTick;
	pProj->m_Type = m_Type;
}

void CProjectile::Snap(int SnappingClient)
{
	float Ct = (Server()->Tick() - m_StartTick) / (float)Server()->TickSpeed();

	if (NetworkClipped(SnappingClient, GetPos(Ct)))
		return;

	CNetObj_Projectile *pProj = static_cast<CNetObj_Projectile *>(Server()->SnapNewItem(NETOBJTYPE_PROJECTILE, GetID(), sizeof(CNetObj_Projectile)));
	if (pProj)
		FillInfo(pProj);
}
