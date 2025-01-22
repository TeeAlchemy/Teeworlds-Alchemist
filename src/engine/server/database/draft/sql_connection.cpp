/* Copyright(C) 2025 - 2025 Comet */
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

bool CSqlConnection::Next()
{
    if (!m_pResult)
        return false;

    return m_pResult->next();
}

void CSqlConnection::Release()
{
    // Free results
    FreeResult();

    // Close connect and delete pointer
    if (m_pConnection)
    {
        m_pConnection->close();
        delete m_pConnection;
        m_pConnection = nullptr;
    }

    // Clean
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

bool CSqlConnection::PrepareQuery(const std::string& Sql)
{
    if (!m_pConnection)
        return false;

    // 准备SQL语句
    m_pPreparedStatement = m_pConnection->prepareStatement(Sql);
    return true;
}

bool CSqlConnection::BindInt(int ParameterIndex, int Value)
{
    if (!m_pPreparedStatement)
        return false;

    m_pPreparedStatement->setInt(ParameterIndex, Value);
    return true;
}

bool CSqlConnection::BindString(int ParameterIndex, const std::string& Value)
{
    if (!m_pPreparedStatement)
        return false;

    m_pPreparedStatement->setString(ParameterIndex, Value);
    return true;
}

bool CSqlConnection::ExecutePrepared()
{
    if (!m_pPreparedStatement)
        return false;

    // 执行准备好的语句
    m_pPreparedStatement->executeUpdate();
    return true;
}

bool CSqlConnection::QueryPrepared()
{
    if (!m_pPreparedStatement)
        return false;

    // 执行准备好的查询并获取结果集
    m_pResult = m_pPreparedStatement->executeQuery();
    return true;
}