/* (c) FlowerFell-Sans. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                 */
#ifndef GAME_SERVER_ENTITIES_CKS
#define GAME_SERVER_ENTITIES_CKS

#include <game/server/entity.h>

const int PhysSize = 14;

class CKs : public CEntity
{
public:
	CKs(CGameWorld *pGameWorld, int Type, vec2 Pos);

	virtual void Reset();
	virtual void Tick();
	virtual void TickPaused();
	virtual void Snap(int SnappingClient);
	virtual void Picking(int Time, class CPlayer *Player);

	int m_Health;

	int GetMaxHealth();
	void HandleLock(class CCharacter *pChr);

private:
	int m_CKID;
	int m_Type;
	int m_Subtype;
	int m_SpawnTick;
	int m_LockedPlayer;
};

#endif