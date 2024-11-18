/* Copyright(C) 2022 - 2024 ST-Chara */
#include <engine/shared/config.h>
#include <game/server/gamecontext.h>
#include "DB.h"

CDB::CDB()
{
    if (SQL_Lock == 0)
        SQL_Lock = lock_create();
}

bool CDB::Connect()
{
    try
    {
        m_Driver = get_driver_instance();
        char aBuf[64];
        str_format(aBuf, sizeof(aBuf), "tcp://%s:%d", g_Config.m_SvSqlIp, g_Config.m_SvSqlPort);
        m_Connection = m_Driver->connect(aBuf, g_Config.m_SvSqlUser, g_Config.m_SvSqlPw);
        m_Statement = m_Connection->createStatement();
        m_Connection->setSchema(g_Config.m_SvSqlDatabase);
    }
    catch (sql::SQLException &e)
    {
        dbg_msg("SQLError", "Error (%d) : %s", e.getErrorCode(), e.what());
        return false;
    }
    return true;
}

void CDB::Disconnect()
{
    try
    {
        // Delete Connection
        delete m_Connection;
    }
    catch (sql::SQLException &e)
    {
        dbg_msg("SQLError", "错误 (%d) : %s", e.getErrorCode(), e.what());
    }
}

bool CDB::Execute(const char *Sql)
{
    return m_Statement->execute(Sql);
}

sql::ResultSet *CDB::ExecuteQuery(const char *Sql)
{
    return m_Statement->executeQuery(Sql);
}

void CDB::FreeData(sql::ResultSet *Result)
{
    delete m_Statement;
    delete Result;
    Disconnect();
}