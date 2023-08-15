#include "Alchemy.h"

CAlchemy::CAlchemy(IStorage *pStorage)
{
    m_pStorage = pStorage;
    m_pData = new CAlchemyData();
}

Alchemy *CAlchemy::NewAlchemy(const char *pName, const char *pUsage)
{
    Alchemy *pTemp;
    pTemp->m_AlchemyID = m_vpAlchemy.size();
    pTemp->m_pName = pName;
    pTemp->m_pUsage = pUsage;
    return pTemp;
}

void CAlchemy::LoadAlchemy()
{
    // Load data from json file
    json_value *pJsonData = m_pData->LoadJson(m_pStorage, "alchemy.json");

    // extract data
    const json_value &rStart = (*pJsonData)["Alchemy"];
    if (rStart.type == json_array)
    {
        for (unsigned i = 0; i < rStart.u.array.length; ++i)
        {
            const char *pName = rStart[i]["Potion"];
            const char *pUsage = rStart[i]["Usage"];

            Alchemy *pAlchemy = NewAlchemy(pName, pUsage);

            const json_value &rFormula = (*pJsonData)["Alchemy"][i]["Formula"];
            for (int j = 0; j < rFormula.u.array.length; ++j)
                pAlchemy->m_Formula.add(rFormula[j].u.string.ptr);

            m_vpAlchemy.push_back(pAlchemy);

            dbg_msg("test", "%d %s %s %s", m_vpAlchemy[i]->m_AlchemyID, m_vpAlchemy[i]->m_pName, m_vpAlchemy[i]->m_pUsage, m_vpAlchemy[i]->m_Formula[1]);
        }
    }

    // Clean up
    json_value_free(pJsonData);
}