/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_FLAG_H
#define GAME_SERVER_ENTITIES_FLAG_H

#include <game/server/entity.h>
#include <game/server/resources.h>

class CAreaFlag : public CEntity
{
public:
    enum
    {
        DUP = 0,
        DDOWN,
        DLEFT,
        DRIGHT,
    };

    enum ELEVEL
    {
        LWOODCOALCOPPER = 1,
        LIRONGOLD,
        LDIAMONDENEGRY,
    };
public:
    /* Constants */
    static int const ms_PhysSize = 14;

    /* Constructor */
    CAreaFlag(CGameWorld *pGameWorld, vec2 Pos0, vec2 Pos1, int MaxProgress, int Level);
    ~CAreaFlag();

    /* CEntity functions */
    virtual void Reset();
    virtual void Snap(int SnappingClient);
    virtual void Tick();
    virtual void TickDefered();

    int GetTeam();
    int GetProgress() { return m_Progress; };
    int GetMaxProgress() { return m_MaxProgress; }
    int GetProduceTeam();

    void InitArea();
    void Search(vec2 StartPos, int DirType);
    void HandleProduce();

    bool InArea(vec2 Pos);

private:
    int m_Progress;
    int m_MaxProgress;
    int m_LaserSnap[2];
    int m_Radius;
    int m_ProduceTeam;
    int m_Level;
    int m_ProduceTick;
    int m_Product[NUM_RESOURCE];
    vec2 m_LowerPos;
    vec2 m_UpperPos;
    bool m_pArea[1024 * 1024 + 1024];

    float m_StepX;
    float m_StepY;
};

#endif