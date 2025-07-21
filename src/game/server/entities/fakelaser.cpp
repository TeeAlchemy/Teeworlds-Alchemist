#include <engine/server.h>
#include "fakelaser.h"

CFakeLaser::CFakeLaser(CGameWorld *pGameWorld, vec2 From, vec2 To, int Type, int Span)
    : CEntity(pGameWorld, CGameWorld::ENTTYPE_LASER, From)
{
    m_From = From;
    m_To = To;
    m_Type = Type;
    m_Span = Span;

    GameWorld()->InsertEntity(this);
}

void CFakeLaser::Tick()
{
    if (m_Span == 0)
        Reset();
    if (m_Span > 0)
        m_Span--;
}

void CFakeLaser::Reset()
{
    MarkForDestroy();
}

void CFakeLaser::Snap(int SnappingClient)
{
    if (NetworkClipped(SnappingClient))
        return;

    CNetObj_DDNetLaser *pLaser = Server()->SnapNewItem<CNetObj_DDNetLaser>(GetID());
    if (!pLaser)
        return;

    pLaser->m_FromX = round_to_int(m_From.x);
    pLaser->m_FromY = round_to_int(m_From.y);
    pLaser->m_ToX = round_to_int(m_To.x);
    pLaser->m_ToY = round_to_int(m_To.y);
    pLaser->m_Type = m_Type;

    // Snap a tick
    if(m_Span == -1)
        Reset();
}