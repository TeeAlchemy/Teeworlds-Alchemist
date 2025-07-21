#ifndef GAME_SERVER_ENTITIES_FAKELASER_H
#define GAME_SERVER_ENTITIES_FAKELASER_H

#include <game/server/entity.h>

class CFakeLaser : public CEntity
{
public:
    CFakeLaser(CGameWorld *pGameWorld, vec2 From, vec2 To, int Type, int Span = -1);

    virtual void Tick();
    virtual void Reset();
    virtual void Snap(int SnappingClient);

private:
    vec2 m_From;
    vec2 m_To;
    int m_Type;
    int m_Span;
};

#endif