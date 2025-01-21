#include <base/system.h>
#include "sql_connection.h"

CSqlConnection::CSqlConnection()
{
    m_pConnection = nullptr;
    m_pStatement = nullptr;
    m_pResult = nullptr;
}

CSqlConnection::~CSqlConnection()
{
    Release();
}

bool CSqlConnection::Connect(sql::Driver *pDriver, std::string User, std::string Pw, std::string Database, std::string Hostname, unsigned short Port)
{
    try
    {
        char aBuf[64];
        str_format(aBuf, sizeof(aBuf), "tcp://%s:%d", Hostname.c_str(), Port);
        m_pConnection = pDriver->connect(aBuf, User.c_str(), Pw.c_str());
        m_pStatement = m_pConnection->createStatement();
        m_pConnection->setSchema(Database.c_str());
    }
    catch (sql::SQLException &e)
    {
        dbg_msg("SQLError", "Error (%d) : %s", e.getErrorCode(), e.what());
        return false;
    }
    return true;
}

bool CSqlConnection::Options(std::string Option, std::string Value)
{
    if (!m_pConnection)
    {
        dbg_msg("MySQL", "Error: m_pConnection is nullptr");
        return false;
    }
    m_pConnection->setClientOption(Option, Value);
    return true;
}

bool CSqlConnection::SetCharacter(std::string StrCsName)
{
    return Options("OPT_CHARSET_NAME", StrCsName);
}

bool CSqlConnection::SetConnTimeout(int Second)
{
    return Options("OPT_CONNECT_TIMEOUT", std::to_string(Second));
}

bool CSqlConnection::SetReconnect(bool Reconn)
{
    return Options("OPT_RECONNECT", std::to_string(Reconn ? 1 : 0));
}

bool CSqlConnection::Query(std::string Sql)
{
    if (!m_pStatement)
        return false;

    // 在执行新查询前释放旧的结果集
    FreeResult();

    m_pResult = m_pStatement->executeQuery(Sql);
    return true;
}

bool CSqlConnection::Next()
{
    if (!m_pResult)
        return false;

    return m_pResult->next();
}

bool CSqlConnection::Execute(std::string Sql)
{
    return m_pStatement->execute(Sql);
}

void CSqlConnection::Release()
{
    // 释放结果集
    FreeResult();

    // 关闭连接并置空指针
    if (m_pConnection)
    {
        m_pConnection->close();
        delete m_pConnection;
        m_pConnection = nullptr;
    }

    // 清理语句对象
    if (m_pStatement)
    {
        delete m_pStatement;
        m_pStatement = nullptr;
    }
}

void CSqlConnection::FreeResult()
{
    if (m_pResult)
    {
        delete m_pResult;
        m_pResult = nullptr;
    }
}