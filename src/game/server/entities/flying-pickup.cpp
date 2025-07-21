#include <game/server/gamecontext.h>
#include "flying-pickup.h"

CFlyingPickup::CFlyingPickup(CGameWorld *pGameWorld, vec2 Pos, int Type, int Subtype, int TargetID)
    : CEntity(pGameWorld, CGameWorld::ENTTYPE_PICKUP, Pos)
{
    m_Type = Type;
    m_Subtype = Subtype;
    m_TargetID = TargetID;
    m_Vel = {0.f, 0.f};
    m_InitialAmount = 1.0f;

    GameWorld()->InsertEntity(this);
}

void CFlyingPickup::Tick()
{
    CCharacter *pChr = GameServer()->GetPlayerChar(m_TargetID);

    CCharacter *OwnerChar = GameServer()->GetPlayerChar(m_TargetID);
    if (!OwnerChar)
        return Reset();

    float Dist = distance(m_Pos, OwnerChar->GetPos());
    if (Dist < 24.0f)
    {
        switch (m_Type)
        {
        case POWERUP_HEALTH:
            pChr->IncreaseHealth(1);
            break;

        case POWERUP_ARMOR:
            pChr->IncreaseArmor(1);
            break;

        case POWERUP_WEAPON:
            pChr->GiveWeapon(m_Subtype, 10);
            break;

        case POWERUP_NINJA:
            pChr->GiveWeapon(WEAPON_NINJA, -1);

        default:
            break;
        }

        Reset();
    }
    else
    {
        vec2 Dir = normalize(OwnerChar->GetPos() - m_Pos);
        m_Pos += Dir * clamp(Dist, 0.0f, 16.0f) * (1.0f - m_InitialAmount);

        m_InitialAmount *= 0.98f;
    }
}

void CFlyingPickup::Reset()
{
    GameServer()->CreateExtraEffect(GetPos(), 0);
    Destroy();
}

void CFlyingPickup::Snap(int SnappingClient)
{
    if (NetworkClipped(SnappingClient))
        return;

    CNetObj_DDNetPickup *pPickup = Server()->SnapNewItem<CNetObj_DDNetPickup>(GetID());
    if (pPickup)
    {
        pPickup->m_X = round_to_int(GetPos().x);
        pPickup->m_Y = round_to_int(GetPos().y);
        pPickup->m_Type = m_Type;
        pPickup->m_Subtype = m_Subtype;
    }
}