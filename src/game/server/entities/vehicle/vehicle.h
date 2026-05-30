#pragma once

#include <game/generated/protocol.h>
#include <game/server/entity.h>

#include "vehicle_util.h"

class CCharacter;

class CVehicle : public CEntity
{
protected:
	int m_Health;
	int m_MaxHealth;
	int m_Team;
	int m_Driver;
	int m_Gunner;
	vec2 m_Vel;
	vec2 m_SpawnPos;
	bool m_Locked;
	bool m_Unlocked;
	int m_IdleTicks;
	int m_GunnerReload;
	int m_PrevGunnerFire;
	int m_aHealthBarIds[3];

	CVehicle(CGameWorld *pGameWorld, int ObjType, vec2 Pos, int Team, int Health);

	CCharacter *DriverChar() const;
	void ApplyHorizontalInput(CCharacter *pDriver, int MaxSpeed, int Accel, float IdleDecay);
	void MoveBox(vec2 Size);
	void Explode();
	void SpawnShrapnel() const;
	void SnapHealthBar(int SnappingClient) const;

	virtual float DriverOffsetY() const { return VehicleScale(-8.f); }
	virtual float BoardRadius() const { return VehicleScale(32.f); }
	virtual vec2 CollisionSize() const { return vec2(VehicleScale(48.f), VehicleScale(24.f)); }
	virtual bool HasGunnerSeat() const { return false; }
	virtual const char *GunnerBoardMessage() const { return nullptr; }
	virtual int GunnerWeapon() const { return WEAPON_GUN; }
	virtual float GunnerProjSpawnOffset() const { return VehicleScale(28.f); }
	virtual vec2 GunnerFirePos(CCharacter *pGunner) const;

	virtual float MaxStepHeight() const { return 0.f; }
	virtual void TickDriver(CCharacter *pDriver) = 0;
	virtual void TickDriverExtras(CCharacter *pDriver) {}
	virtual void TickGunner(CCharacter *pGunner);
	virtual void TickIdle() {}
	virtual void OnDriverBoarded(CCharacter *pDriver);

public:
	virtual ~CVehicle();

	void Tick() override;
	void Reset() override;

	int GetDriver() const { return m_Driver; }
	int GetOwner() const { return m_Driver; }
	int GetGunner() const { return m_Gunner; }
	int &GunnerSlot() { return m_Gunner; }
	int GetTeam() const { return m_Team; }
	int GetHealth() const { return m_Health; }
	int GetMaxHealth() const { return m_MaxHealth; }
	vec2 &Vel() { return m_Vel; }
	vec2 GetSpawnPos() const { return m_SpawnPos; }
	vec2 GetCollisionSize() const { return CollisionSize(); }

	void TakeDamage(int Amount, int From);
	void Repair(int Amount);
	bool CanBoard(int Team) const;

	void ForceDriverLeave();
	void ForceGunnerLeave();
	bool IsOccupiedBy(int ClientId) const;
	void HandleOccupantDismount(int ClientId);
	bool SupportsGunnerSeat() const { return HasGunnerSeat(); }
	const char *GetGunnerBoardMessage() const { return GunnerBoardMessage(); }

	template<typename F>
	static void ForEach(CGameWorld *pWorld, F &&Callback)
	{
		const int aTypes[] = {
			CGameWorld::ENTTYPE_CAR,
			CGameWorld::ENTTYPE_AIRCRAFT,
			CGameWorld::ENTTYPE_HELICOPTER,
			CGameWorld::ENTTYPE_JET,
			CGameWorld::ENTTYPE_TANK,
			CGameWorld::ENTTYPE_MOTORCYCLE,
			CGameWorld::ENTTYPE_BOAT,
			CGameWorld::ENTTYPE_SUBMARINE,
			CGameWorld::ENTTYPE_SKATEBOARD,
		};

		for (int Type : aTypes)
		{
			for (CEntity *pEnt = pWorld->FindFirst(Type); pEnt; pEnt = pEnt->TypeNext())
				Callback(static_cast<CVehicle *>(pEnt));
		}
	}
};
