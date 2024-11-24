#include "item.h"
#include <game/server/gamecontext.h>
#include <engine/external/json-parser/json.h>

CItem_F::CItem_F(CGameContext *pGameServer)
{
    m_pGameServer = pGameServer;
}

void CItem_F::LoadIndex()
{
    const char *pIndex = "./server_items/index.json";
    IOHANDLE File = GameServer()->Storage()->OpenFile(pIndex, IOFLAG_READ, IStorage::TYPE_ALL);
    dbg_assert(!(!File), "Can't open 'server_items/index.json'");
    int FileSize = (int)io_length(File);
    char *pFileData = new char[FileSize + 1];
    io_read(File, pFileData, FileSize);
    pFileData[FileSize] = 0;
    io_close(File);

    // parse json data
    json_settings JsonSettings;
    mem_zero(&JsonSettings, sizeof(JsonSettings));
    char aError[256];
    json_value *pJsonData = json_parse_ex(&JsonSettings, pFileData, aError);
    dbg_assert(!(!pJsonData), "Can't open 'server_items/index.json'");

    const json_value &rStart = (*pJsonData)["item indices"];
    if (rStart.type == json_array)
    {
        for (size_t i = 0; i < rStart.u.array.length; ++i)
        {
            if (rStart[i])
                LoadItem((const char *)rStart[i]);
        }
        for (size_t i = 0; i < rStart.u.array.length; ++i)
        {
            if (rStart[i])
                LoadFormula((const char *)rStart[i]);
        }
    }

    // clean up
    json_value_free(pJsonData);
    delete[] pFileData;
}

void CItem_F::LoadItem(const char *FileName)
{
    IOHANDLE File = GameServer()->Storage()->OpenFile(FileName, IOFLAG_READ, IStorage::TYPE_ALL);
    if (!File)
    {
        dbg_msg("Item Loader", "Can't open '%s'", FileName);
        return;
    }
    int FileSize = (int)io_length(File);
    char *pFileData = new char[FileSize + 1];
    io_read(File, pFileData, FileSize);
    pFileData[FileSize] = 0;
    io_close(File);

    // parse json data
    json_settings JsonSettings;
    mem_zero(&JsonSettings, sizeof(JsonSettings));
    char aError[256];
    json_value *pJsonData = json_parse_ex(&JsonSettings, pFileData, aError);
    if (!pJsonData)
    {
        dbg_msg("Item Loader", "Can't open '%s'", FileName);
        return;
    }

    const json_value &rStart = (*pJsonData)["item"];
    if (rStart)
    {
        int Type = rStart["type"].u.integer;
        if (rStart["multiple"])
        {
            for (size_t i = 0; i < rStart["multiple"].u.object.length; i++)
            {
                const json_value &rMultiple = rStart["multiple"][i];
                int ID = rMultiple["id"].u.integer;

                switch (Type)
                {
                case ITYPE_SWORD:
                case ITYPE_TURRET:
                case ITYPE_PICKAXE:
                case ITYPE_AXE:
                    m_aItems[ID] = new CItem_Tool();
                    ((CItem_Tool *)m_aItems[ID])->m_Capacity = rMultiple["capacity"].u.integer;
                    ((CItem_Tool *)m_aItems[ID])->m_Damage = rMultiple["damage"].u.integer;
                    break;

                case ITYPE_CARD:
                    m_aItems[ID] = new CItem_Card();
                    ((CItem_Card *)m_aItems[ID])->m_Capacity = rMultiple["capacity"].u.integer;
                    for (size_t i = 0; i < rMultiple["placeable"].u.array.length; i++)
                        ((CItem_Card *)m_aItems[ID])->m_Placeable[rMultiple["placeable"][i].u.integer] = true;

                    break;

                default:
                    m_aItems[ID] = new CItem();
                    break;
                }

                m_aItems[ID]->m_Type = Type;
                m_aItems[ID]->m_ID = ID;
                str_copy(m_aItems[ID]->m_ItemName, rMultiple["name"], sizeof(m_aItems[ID]->m_ItemName));
                m_aItems[ID]->m_Proba = rMultiple["proba"].u.integer;
                m_aItems[ID]->m_MaxHealth = rMultiple["health"].u.integer;
            }
        }
        else
        {
            int ID = rStart["id"].u.integer;
            switch (Type)
            {
            case ITYPE_SWORD:
            case ITYPE_TURRET:
            case ITYPE_PICKAXE:
            case ITYPE_AXE:
                m_aItems[ID] = new CItem_Tool();
                ((CItem_Tool *)m_aItems[ID])->m_Capacity = rStart["capacity"].u.integer;
                ((CItem_Tool *)m_aItems[ID])->m_Damage = rStart["damage"].u.integer;
                break;

            case ITYPE_CARD:
                m_aItems[ID] = new CItem_Card();
                ((CItem_Card *)m_aItems[ID])->m_Capacity = rStart["capacity"].u.integer;
                for (size_t i = 0; i < rStart["placeable"].u.array.length; i++)
                    ((CItem_Card *)m_aItems[ID])->m_Placeable[rStart["placeable"][i].u.integer] = true;
                break;

            default:
                m_aItems[ID] = new CItem();
                break;
            }
            m_aItems[ID]->m_Type = rStart["type"].u.integer;
            m_aItems[ID]->m_ID = ID;
            str_copy(m_aItems[ID]->m_ItemName, rStart["name"], sizeof(m_aItems[ID]->m_ItemName));
            m_aItems[ID]->m_Proba = rStart["proba"].u.integer;
            m_aItems[ID]->m_MaxHealth = rStart["health"].u.integer;
        }
    }
}

void CItem_F::LoadFormula(const char *FileName)
{
    IOHANDLE File = GameServer()->Storage()->OpenFile(FileName, IOFLAG_READ, IStorage::TYPE_ALL);
    if (!File)
    {
        dbg_msg("Item Loader", "Can't open '%s'", FileName);
        return;
    }
    int FileSize = (int)io_length(File);
    char *pFileData = new char[FileSize + 1];
    io_read(File, pFileData, FileSize);
    pFileData[FileSize] = 0;
    io_close(File);

    // parse json data
    json_settings JsonSettings;
    mem_zero(&JsonSettings, sizeof(JsonSettings));
    char aError[256];
    json_value *pJsonData = json_parse_ex(&JsonSettings, pFileData, aError);
    if (!pJsonData)
    {
        dbg_msg("Item Loader", "Can't open '%s'", FileName);
        return;
    }

    const json_value &rStart = (*pJsonData)["item"];
    if (rStart)
    {
        if (rStart["multiple"])
        {
            for (size_t i = 0; i < rStart["multiple"].u.object.length; i++)
            {
                int ID = rStart["multiple"][i]["id"].u.integer;
                const json_value &rFormula = rStart["multiple"][i]["formula"];
                for (size_t j = 0; j < rFormula.u.object.length; j++)
                    m_aItems[ID]->m_Formula[FindItem(rFormula.u.object.values[j].name)] = rFormula.u.object.values[j].value->u.integer;
            }
        }
        else if (rStart["formula"])
        {
            int ID = rStart["id"].u.integer;
            const json_value &rFormula = rStart["formula"];
            for (size_t j = 0; j < rFormula.u.object.length; j++)
                m_aItems[ID]->m_Formula[FindItem(rFormula.u.object.values[j].name)] = rFormula.u.object.values[j].value->u.integer;
        }
    }
}

int CItem_F::FindItem(const char *ItemName)
{
    for (int i = 1; i < int(NUM_ITEM); i++)
    {
        if (!Items(i))
            continue;

        if (str_comp(Items(i)->m_ItemName, ItemName) == 0)
            return Items(i)->m_ID;
    }
    return ITEM_LOG;
}

int CItem_F::GetType(int ID)
{
    if (!CheckItemVaild(ID))
        return int(ITYPE_MATERIAL);
    return m_aItems[ID]->m_Type;
}

const char *CItem_F::GetItemName(int ID)
{
    if (!CheckItemVaild(ID))
        return "Log";
    return m_aItems[ID]->m_ItemName;
}

int CItem_F::GetItemID(const char ItemName[64])
{
    short ID = FindItem(ItemName);
    if (!CheckItemVaild(ID))
        return ITEM_LOG;
    return ID;
}

int CItem_F::GetDmg(int ID)
{
    if (!CheckItemVaild(ID))
        return 0;
    return ((CItem_Tool *)m_aItems[ID])->m_Damage;
}

int CItem_F::GetProba(int ID)
{
    if (!CheckItemVaild(ID))
        return 0;
    return m_aItems[ID]->m_Proba;
}

int CItem_F::GetCapacity(int ID)
{
    if (!CheckItemVaild(ID))
        return 0;
    return ((CItem_Tool *)m_aItems[ID])->m_Capacity;
}

void CItem_F::GetFormula(int ID, int *Formula)
{
    if (!CheckItemVaild(ID))
        return;
    for (int i = 1; i < NUM_ITEM; i++)
    {
        if (m_aItems[ID]->m_Formula[i])
            Formula[i] = m_aItems[ID]->m_Formula[i];
    }
}

int CItem_F::GetMax(int ID)
{
    if (!CheckItemVaild(ID))
        return 0;
    return m_aItems[ID]->m_Max;
}

int CItem_F::GetMaxHealth(int ID)
{
    if (!CheckItemVaild(ID))
        return 0;
    return m_aItems[ID]->m_MaxHealth;
}