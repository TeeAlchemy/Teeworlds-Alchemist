/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include "growingexplosion.h"
#include "lightning.h"
#include "projectile.h"

CProjectile::CProjectile(CGameWorld *pGameWorld, int Type, int Owner, vec2 Pos, vec2 Dir, int Span,
		int Damage, bool Explosive, float Force, int SoundImpact, int Weapon)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_PROJECTILE, Pos)
{
	m_Type = Type;
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

	switch(m_Type)
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

	return CalcPos(m_Pos, m_Direction, Curvature, Speed, Time);
}


void CProjectile::Tick()
{
	float Pt = (Server()->Tick()-m_StartTick-1)/(float)Server()->TickSpeed();
	float Ct = (Server()->Tick()-m_StartTick)/(float)Server()->TickSpeed();
	vec2 PrevPos = GetPos(Pt);
	vec2 CurPos = GetPos(Ct);
	int Collide = GameServer()->Collision()->IntersectLine(PrevPos, CurPos, &CurPos, 0);
	CCharacter *OwnerChar = GameServer()->GetPlayerChar(m_Owner);
	CCharacter *TargetChr = GameServer()->m_World.IntersectCharacter(PrevPos, CurPos, 6.0f, CurPos, OwnerChar);

	if(!OwnerChar || !OwnerChar->GetPlayer())
		return MarkForDestroy();


	if ((Server()->Tick() - m_StartTick) % 10 == 0 || Server()->Tick() - m_StartTick < 3)
	{
		int Electron = GameServer()->ItemHelper()->GetCard(GameServer()->GetPlayer(m_Owner)->GetExtraHolding(ITYPE_SWORD), ITEM_CARD_ELECTRON);
		if (Electron)
		{
			if(m_Type == WEAPON_SHOTGUN)
			{
				float a = GetAngle(normalize(CurPos - PrevPos));
				new CLightning(GameWorld(), CurPos, vec2(cosf(a), sinf(a)), 100, 25, m_Owner, clamp(m_Damage, 1, m_Damage/2));
			}
			else
			{
				for (int i = 0; i < 3; i++)
				{
					float Spreading[] = {-0.185f, -0.130f, -0.050f, 0.050f, 0.130f, 0.185f};
					float a = GetAngle(normalize(CurPos - PrevPos));
					a += Spreading[i + 3];
					new CLightning(GameWorld(), CurPos, vec2(cosf(a), sinf(a)), 200, 100, m_Owner, clamp(m_Damage, 1, m_Damage/2));
				}
			}
		}
	}

	m_LifeSpan--;

	if(TargetChr || Collide || m_LifeSpan < 0 || GameLayerClipped(CurPos))
	{
		if(m_LifeSpan >= 0 || m_Weapon == WEAPON_GRENADE)
			GameServer()->CreateSound(CurPos, m_SoundImpact);

		int Fusion = GameServer()->ItemHelper()->GetCard(GameServer()->GetPlayer(m_Owner)->GetExtraHolding(ITYPE_SWORD), ITEM_CARD_FUSION);
		if (Fusion && m_Type != WEAPON_SHOTGUN)
			new CGrowingExplosion(GameWorld(), CurPos, vec2(0, 0), m_Owner, 24.f * Fusion, GROWINGEXPLOSIONEFFECT_BOOM, Fusion);

		if(m_Explosive || GameServer()->ItemHelper()->GetCard(OwnerChar->GetPlayer()->GetExtraHolding(ITYPE_SWORD), ITEM_CARD_EXPLOSION))
				GameServer()->CreateExplosion(CurPos, m_Owner, m_Weapon, false, Fusion);

		else if(TargetChr)
			TargetChr->TakeDamage(m_Direction * max(0.001f, m_Force), m_Damage, m_Owner, m_Weapon);


		GameServer()->m_World.DestroyEntity(this);
	}
}

void CProjectile::TickPaused()
{
	++m_StartTick;
}

void CProjectile::FillInfo(CNetObj_Projectile *pProj)
{
	pProj->m_X = (int)m_Pos.x;
	pProj->m_Y = (int)m_Pos.y;
	pProj->m_VelX = (int)(m_Direction.x*100.0f);
	pProj->m_VelY = (int)(m_Direction.y*100.0f);
	pProj->m_StartTick = m_StartTick;
	pProj->m_Type = m_Type;
}

void CProjectile::Snap(int SnappingClient)
{
	float Ct = (Server()->Tick()-m_StartTick)/(float)Server()->TickSpeed();

	if(NetworkClipped(SnappingClient, GetPos(Ct)))
		return;

	CNetObj_Projectile *pProj = Server()->SnapNewItem<CNetObj_Projectile>(GetID());
	if(pProj)
		FillInfo(pProj);
}
