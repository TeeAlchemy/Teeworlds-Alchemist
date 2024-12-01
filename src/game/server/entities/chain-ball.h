#ifndef GAME_SERVER_ENTITIES_CHAINBALL_H
#define GAME_SERVER_ENTITIES_CHAINBALL_H

#include <game/server/entity.h>

const int numIds = 8;

class CChainBall : public CEntity
{
public:
	CChainBall(CGameWorld *pGameWorld, int Owner, vec2 Pos);

	virtual void Reset();
	virtual void Tick();
	virtual void Snap(int SnappingClient);

    void SetAnchor(vec2 Pos);

    vec2 CalculateRopeForce(const vec2 Pos, const vec2 AnchorPos, float K, float Length);

private:
    int m_Owner;
    int m_aIDs[numIds];
    float m_Mass;
    float m_K;
    float m_Length;
    vec2 m_Anchor;
    vec2 m_Vel;
};

#endif
