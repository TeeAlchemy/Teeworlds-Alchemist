#ifndef ALCHEMY_DATA_H
#define ALCHEMY_DATA_H

#include <base/tl/array.h>
#include <vector>

#include <engine/external/json-parser/json.h>

struct Alchemy
{
    int m_AlchemyID;
    const char *m_pName;
    const char *m_pUsage;
    array<const char*> m_Formula;
};

class CAlchemyData
{
public:
    CAlchemyData();
    ~CAlchemyData();

    json_value *LoadJson(class IStorage * pStorage, const char *pFileName);
};

class CAlchemy
{
    class IStorage *m_pStorage;

private:
    IStorage *Storage() { return m_pStorage; }
    std::vector<Alchemy *> m_vpAlchemy;

    Alchemy *NewAlchemy(const char *pName, const char *pUsage);

public:
    CAlchemy(class IStorage *pStorage);

    void LoadAlchemy();

public:
    CAlchemyData *m_pData;
};

#endif