/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_PROJECTILE_H
#define GAME_SERVER_ENTITIES_PROJECTILE_H

class CProjectile : public CEntity
{
public:
	CProjectile(CGameWorld *pGameWorld, int Type, int Owner, vec2 Pos, vec2 Dir, int Span,
		int Damage, bool Explosive, float Force, int SoundImpact, int Weapon, int Team);

	vec2 GetPos(float Time);
	void FillInfo(CNetObj_Projectile *pProj);

	virtual void Reset();
	virtual void Tick();
	virtual void TickPaused();
	virtual void Snap(int SnappingClient);

	int GetOwner() { return m_Owner; }
	int GetWeapon() { return m_Weapon; }
	vec2 GetRealPos() { return m_Pos; }

	vec2 m_InitPos;
	vec2 m_Direction;

	int m_Weapon;
	int m_StartTick;
	int m_Damage;
	int m_Team;

private:
	int m_LifeSpan;
	int m_Owner;
	int m_Type;
	int m_SoundImpact;
	float m_Force;
	bool m_Explosive;
};

#endif
