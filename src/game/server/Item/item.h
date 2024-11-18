#pragma once
#include <cstring>

enum
{
    ITEM_LOG = 1,
    ITEM_COAL,
    ITEM_COPPER,
    ITEM_IRON,
    ITEM_GOLDEN,
    ITEM_DIAMOND,
    ITEM_ENEGRY,
    ITEM_ZOMBIEHEART,

    ITEM_SWORD_LOG = 9,
    ITEM_AXE_LOG,
    ITEM_PICKAXE_LOG,

    ITEM_SWORD_IRON = 12,
    ITEM_AXE_COPPER,
    ITEM_PICKAXE_COPPER,

    ITEM_AXE_IRON = 15,
    ITEM_PICKAXE_IRON,

    ITEM_SWORD_GOLDEN = 17,
    ITEM_AXE_GOLDEN,
    ITEM_PICKAXE_GOLDEN,

    ITEM_SWORD_DIAMOND = 20,
    ITEM_AXE_DIAMOND,
    ITEM_PICKAXE_DIAMOND,

    ITEM_SWORD_ENEGRY = 23,
    ITEM_PICKAXE_ENEGRY,

    ITEM_TURRET_BEGINNER = 25,
    ITEM_TURRET_INTERMEDIATE,
    ITEM_TURRET_ADVANCED,

    ITEM_CARD_QUICKLY_FIRE = 28,
    ITEM_CARD_QUICKLY_LOADING,
    ITEM_CARD_DAMAGE,
    ITEM_CARD_EXPLOSION,
    ITEM_CARD_ELECTRON,
    ITEM_CARD_FUSION,
    ITEM_CARD_FORCE,
    ITEM_CARD_MANUAL,
    NUM_ITEM,
};

enum
{
    LEVEL_LOG = 1,
    LEVEL_COAL,
    LEVEL_COPPER,
    LEVEL_IRON,
    LEVEL_GOLD,
    LEVEL_DIAMOND,
    LEVEL_ENEGRY,
    NUM_LEVELS
};

enum
{
    ITYPE_PICKAXE = 0,
    ITYPE_AXE,
    ITYPE_SWORD,
    ITYPE_TURRET,
    ITYPE_MATERIAL,
    ITYPE_CARD,
    NUM_ITYPE,
};

struct CItem
{
    int m_Type;
    int m_ID;
    char m_ItemName[128];
    int m_Proba;
    int m_Formula[NUM_ITEM];
    int m_Max;
    int m_MaxHealth;

    CItem()
    {
        m_Type = 0;
        m_ID = 0;
        strncpy(m_ItemName, "", sizeof(m_ItemName));
        m_Proba = 0;
        for (int i = 0; i < int(NUM_ITEM); i++)
            m_Formula[i] = 0;
        m_Max = 0;
        m_MaxHealth = 0;        
    }
};

struct CItem_Tool : public CItem
{
    int m_Capacity;
    int m_Damage;

    CItem_Tool()
    {
        m_Capacity = 0;
        m_Damage = 0;
    }
};

struct CItem_Card : public CItem_Tool
{
    bool m_Placeable[NUM_ITYPE];

    CItem_Card()
    {
        for (int i = 0; i < int(NUM_ITYPE); i++)
            m_Placeable[i] = 0;
    }
};

struct SPlayerItemData
{
    int m_Num;
    int m_Cards;
};

class CItem_F
{
private:
    class CGameContext *m_pGameServer;

    class CGameContext *GameServer() const { return m_pGameServer; }

    CItem *m_aItems[NUM_ITEM];

public:
    CItem *Items(int ID) { return m_aItems[ID]; }
    CItem_F(class CGameContext *pGameServer);
    void LoadIndex();
    void LoadItem(const char *FileName);
    void LoadFormula(const char *FileName);

    int FindItem(const char *ItemName);

    int GetType(int ID);
    const char *GetItemName(int ID);
    int GetItemID(const char ItemName[64]);
    int GetDmg(int ID);
    int GetProba(int ID);
    int GetCapacity(int ID);
    void GetFormula(int ID, int *Formula);
    int GetMax(int ID);
    int GetMaxHealth(int ID);

    bool CheckItemVaild(int ID, int Type = -1) // -1 for all
    {
        if (ID <= 0 || ID >= NUM_ITEM || !Items(ID))
            return false;
        return true;
    }
};