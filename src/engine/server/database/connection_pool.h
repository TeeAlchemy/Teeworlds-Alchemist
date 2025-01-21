#pragma once
#include "sql_connection.h"
#include <mutex>
#include <deque>

class CConnectionPool
{
public:
    static CConnectionPool *GetConnPool();
    CConnectionPool(const CConnectionPool &obj) = delete;
    CConnectionPool &operator=(const CConnectionPool &obj) = delete;
    ~CConnectionPool();

    bool Create(std::string User, std::string Password, std::string DbName, std::string Hostname, unsigned short port, int PoolSize, int Timeout);
    CSqlConnection *GetOneConn();
    void ReleaseOneConn(CSqlConnection *pConn);

    void Destroy();
private:
    CConnectionPool();
    void AddIdelQueue();

    sql::Driver *m_pDriver;

    std::string m_User;
    std::string m_Password;
    std::string m_DbName;
    std::string m_Hostname;

    unsigned short m_Port;
    int m_nPoolSize = 0;
    int m_nTimeOut = 0;

    std::mutex m_mtx;

    std::deque<CSqlConnection *> m_qIdle;
    std::deque<CSqlConnection *> m_qBusy;
};
