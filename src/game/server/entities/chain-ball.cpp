#include <engine/server.h>
#include <game/server/gamecontext.h>
#include <game/gamecore.h>

#include "chain-ball.h"

const float dt = 0.01f;

CChainBall::CChainBall(CGameWorld *pGameWorld, int Owner, vec2 Pos)
    : CEntity(pGameWorld, CGameWorld::ENTTYPE_CHAINBALL, Pos)
{
    m_Owner = Owner;
    m_Anchor = Pos;
    m_K = 60.f;
    m_Length = 10.f;
    m_Mass = 1.f;

    for (int i = 0; i < 8; i++)
        m_aIDs[i] = Server()->SnapNewID();

    GameWorld()->InsertEntity(this);
}

void CChainBall::Reset()
{
    MarkForDestroy();
    for (int i = 0; i < numIds; i++)
        Server()->SnapFreeID(m_aIDs[i]);
}

void CChainBall::Tick()
{
    if (IsMarkedForDestroy())
        return;

    if (!GameServer()->GetPlayer(m_Owner) || !GameServer()->GetPlayer(m_Owner)->GetCharacter())
    {
        Reset();
        return;
    }

    SetAnchor(GameServer()->GetPlayerChar(m_Owner)->GetPos());

    vec2 GravityForce = vec2(0.0f, m_Mass * 981.f);
    vec2 RopeForce = CalculateRopeForce(m_Pos, m_Anchor, m_K, m_Length);
    vec2 TotalForce = vec2(GravityForce.x + RopeForce.x, GravityForce.y + RopeForce.y);
    vec2 Accel = TotalForce / m_Mass;
    m_Vel += Accel * dt;

    m_Vel.y += 50.f;

    vec2 InoutVel = m_Vel * dt;
    GameServer()->Collision()->MoveBox(&m_Pos, &InoutVel, vec2(64.f, 64.f), 0.1f);
    m_Vel = InoutVel / dt;
}

vec2 CChainBall::CalculateRopeForce(const vec2 Pos, const vec2 AnchorPos, float K, float RestLength)
{
    vec2 Displacement = vec2(Pos.x - AnchorPos.x, Pos.y - AnchorPos.y);
    float Dist = sqrt(Displacement.x * Displacement.x + Displacement.y * Displacement.y);

    if (Dist < m_Length + 50.f)
        return Pos - AnchorPos;

    vec2 MaxLength = vec2(RestLength - Dist, RestLength - Dist);
    vec2 ForceDirection = vec2(Displacement.x / Dist, Displacement.y / Dist);
    vec2 Force = MaxLength * ForceDirection;
    return Force * K;
}

void CChainBall::SetAnchor(vec2 Pos)
{
    m_Anchor = Pos;
}

void CChainBall::Snap(int SnappingClient)
{
    if (NetworkClipped(SnappingClient, m_Anchor) && NetworkClipped(SnappingClient, m_Pos))
        return;

    for (int i = 0; i < numIds; i++)
    {
        float shiftedAngle = angle(normalize(m_Anchor - m_Pos)) + 2.0 * pi * static_cast<float>(i) / static_cast<float>(numIds);

        CNetObj_Projectile *pProj = Server()->SnapNewItem<CNetObj_Projectile>(m_aIDs[i]);
        pProj->m_X = (int)(m_Pos.x + 64.f * cos(shiftedAngle));
        pProj->m_Y = (int)(m_Pos.y + 64.f * sin(shiftedAngle));
        pProj->m_VelX = (int)(0.0f);
        pProj->m_VelY = (int)(0.0f);
        pProj->m_StartTick = Server()->Tick();
        pProj->m_Type = WEAPON_SHOTGUN;
    }

    CNetObj_Laser *pLaser = Server()->SnapNewItem<CNetObj_Laser>(GetID());
    if (!pLaser)
        return;

    pLaser->m_X = (int)m_Pos.x;
    pLaser->m_Y = (int)m_Pos.y;
    pLaser->m_FromX = (int)m_Anchor.x;
    pLaser->m_FromY = (int)m_Anchor.y;
    pLaser->m_StartTick = Server()->Tick();
}