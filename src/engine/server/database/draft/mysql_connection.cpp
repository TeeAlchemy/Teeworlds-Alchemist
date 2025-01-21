#include <base/system.h>
#include "mysql_connection.h"

CMySQLConnection::CMySQLConnection()
{
    Init();
}

CMySQLConnection::~CMySQLConnection()
{
    Release();
}

bool CMySQLConnection::Connect(std::string user, std::string passWord, std::string dbName, std::string ip, unsigned short port, unsigned long multiSqlFlag)
{
    if (!m_pConnection)
    {
        if (mysql_errno(m_pConnection))
            dbg_msg("MySQL", "Error: m_pConnection is nullptr");
        return false;
    }
    if (!mysql_real_connect(m_pConnection, ip.c_str(), user.c_str(), passWord.c_str(), dbName.c_str(), port, nullptr, multiSqlFlag))
    {
        if (mysql_errno(m_pConnection))
            dbg_msg("MySQL", "Error %d: %s", mysql_errno(m_pConnection), mysql_error(m_pConnection));
        return false;
    }

    return true;
}

void CMySQLConnection::SetCharacter(std::string StrCsName)
{
    if (m_pConnection)
        mysql_set_character_set(m_pConnection, StrCsName.c_str());
}

bool CMySQLConnection::SetConnTimeout(int Second)
{
    return Options(MYSQL_OPT_CONNECT_TIMEOUT, &Second);
}

bool CMySQLConnection::SetReconnect(bool Reconn)
{
    return Options(MYSQL_OPT_RECONNECT, &Reconn);
}

bool CMySQLConnection::Query(std::string Sql)
{
    FreeResult();
    if (mysql_real_query(m_pConnection, Sql.c_str(), Sql.length()))
    {
        if (mysql_errno(m_pConnection))
            dbg_msg("MySQL", "Error %d: %s", mysql_errno(m_pConnection), mysql_error(m_pConnection));
        return false;
    }

    m_pResult = mysql_store_result(m_pConnection);
    if (!m_pResult)
    {
        if (mysql_errno(m_pConnection))
            dbg_msg("MySQL", "Error %d: %s", mysql_errno(m_pConnection), mysql_error(m_pConnection));
        return false;
    }
    return true;
}

bool CMySQLConnection::Next()
{
    m_pRow = mysql_fetch_row(m_pResult);
    if (m_pResult && m_pRow)
        return true;

    if (mysql_errno(m_pConnection))
        dbg_msg("MySQL", "Error %d: %s", mysql_errno(m_pConnection), mysql_error(m_pConnection));
    return false;
}

std::string CMySQLConnection::Value(int Index)
{
    int ColCount = mysql_num_fields(m_pResult);
    if (Index >= ColCount || Index < 0)
        return std::string();

    return std::string(m_pRow[Index], mysql_fetch_lengths(m_pResult)[Index]);
}

bool CMySQLConnection::Execute(std::string Sql)
{
    if (!m_pConnection)
    {
        if (mysql_errno(m_pConnection))
            dbg_msg("MySQL", "Error %d: %s", mysql_errno(m_pConnection), mysql_error(m_pConnection));
        return false;
    }
    if (mysql_real_query(m_pConnection, Sql.c_str(), Sql.length()))
    {
        if (mysql_errno(m_pConnection))
            dbg_msg("MySQL", "Error %d: %s", mysql_errno(m_pConnection), mysql_error(m_pConnection));
        return false;
    }
    return true;
}

bool CMySQLConnection::ConvertSqlString(char *DataFrom, char *DataTo, unsigned long FromLen, unsigned long *ToLen)
{
    if (!m_pConnection || !DataFrom || !DataTo)
        return false;

    unsigned long DataToSize = mysql_real_escape_string(m_pConnection, DataTo, DataFrom, FromLen);
    if (ToLen)
        *ToLen = DataToSize;

    return true;
}

bool CMySQLConnection::Transaction(bool Auto)
{
    return mysql_autocommit(m_pConnection, Auto);
}

bool CMySQLConnection::Commit()
{
    return mysql_commit(m_pConnection);
}

bool CMySQLConnection::Rollback()
{
    return mysql_rollback(m_pConnection);
}

bool CMySQLConnection::IsConnect()
{
    if (m_pConnection)
        return (0 == mysql_ping(m_pConnection));
    return false;
}

void CMySQLConnection::Init()
{
    m_pConnection = mysql_init(nullptr);
}

void CMySQLConnection::Release()
{
    FreeResult();
    if (m_pConnection)
    {
        mysql_close(m_pConnection);
        m_pConnection = nullptr;
    }
}

void CMySQLConnection::FreeResult()
{
    if (m_pResult)
    {
        mysql_free_result(m_pResult);
        m_pResult = nullptr;
    }
}

bool CMySQLConnection::Options(mysql_option opt, const void *arg)
{
    if (!m_pConnection)
    {
        if (mysql_errno(m_pConnection))
            dbg_msg("MySQL", "Error: m_pConnection is nullptr");
        return false;
    }

    if (mysql_options(m_pConnection, opt, arg))
    {
        if (mysql_errno(m_pConnection))
            dbg_msg("MySQL", "Error %d: %s", mysql_errno(m_pConnection), mysql_error(m_pConnection));
        return false;
    }
    return true;
}

int CMySQLConnection::GetIntValue(int Index)
{
    return std::atoi(Value(Index).c_str());
}
