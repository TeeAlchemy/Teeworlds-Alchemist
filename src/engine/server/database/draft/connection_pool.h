#pragma once
#include "mysql_connection.h"
#include <mutex>
#include <deque>

class CConnectionPool
{
public:
    static CConnectionPool *GetConnPool();
    CConnectionPool(const CConnectionPool &obj) = delete;
    CConnectionPool &operator=(const CConnectionPool &obj) = delete;
    ~CConnectionPool();

    bool Create(std::string user, std::string passWord, std::string dbName, std::string ip, unsigned short port = 3306, unsigned long multiSqlFlag = 0, int poolSize = 10, int timeOut = 60);
    CMySQLConnection *GetOneConn();
    void ReleaseOneConn(CMySQLConnection *pConn);

private:
    CConnectionPool();
    void Destroy();
    void AddIdelQueue();

    std::string m_User;
    std::string m_Password;
    std::string m_DbName;
    std::string m_Hostname;

    unsigned short m_Port;
    unsigned short m_MultiSqlFlag = 0;
    int m_nPoolSize = 0;
    int m_nTimeOut = 0;

    std::mutex m_mtx;

    std::deque<CMySQLConnection *> m_qIdle;
    std::deque<CMySQLConnection *> m_qBusy;
};
