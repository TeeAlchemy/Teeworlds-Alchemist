#include "connection_pool.h"
#include <algorithm>

CConnectionPool *CConnectionPool::GetConnPool()
{
    static CConnectionPool sPool;
    return &sPool;
}

CConnectionPool::~CConnectionPool()
{
    Destroy();
}

bool CConnectionPool::Create(std::string User, std::string Password, std::string DbName, std::string Hostname, unsigned short Port, unsigned long MultiSqlFlag, int PoolSize, int Timeout)
{
    m_User = User;
    m_Password = Password;
    m_DbName = DbName;
    m_Hostname = Hostname;
    m_Port = Port;
    m_MultiSqlFlag = MultiSqlFlag;
    m_nPoolSize = PoolSize;
    m_nTimeOut = Timeout;

    {
        std::lock_guard<std::mutex> locker(m_mtx);

        for (int i = 0; i < m_nPoolSize; i++)
            AddIdelQueue();
    }

    return m_nPoolSize == m_qIdle.size();
}

CMySQLConnection *CConnectionPool::GetOneConn()
{
    CMySQLConnection *pConn = nullptr;

    {
        std::lock_guard<std::mutex> locker(m_mtx);

        if (!m_qIdle.empty())
        {
            pConn = m_qIdle.front();
            m_qIdle.pop_front();

            if (pConn && pConn->IsConnect())
                m_qBusy.push_back(pConn);
            else
            {
                if (pConn)
                {
                    delete pConn;
                    pConn = nullptr;
                }

                pConn = new CMySQLConnection;
                if (pConn && pConn->Connect(m_User, m_Password, m_DbName, m_Hostname, m_Port, m_MultiSqlFlag))
                    m_qBusy.push_back(pConn);
                else
                {
                    if (pConn)
                        delete pConn;
                    pConn = nullptr;
                }
            }
        }
        else
        {
            pConn = new CMySQLConnection;
            if (pConn && pConn->Connect(m_User, m_Password, m_DbName, m_Hostname, m_Port, m_MultiSqlFlag))
                m_qBusy.push_back(pConn);
            else
            {
                if (pConn)
                    delete pConn;
                pConn = nullptr;
            }
        }
    }

    return pConn;
}

void CConnectionPool::ReleaseOneConn(CMySQLConnection *pConn)
{
    if (nullptr == pConn)
        return;

    std::lock_guard<std::mutex> locker(m_mtx);

    std::deque<CMySQLConnection *>::iterator it = find(m_qBusy.begin(), m_qBusy.end(), pConn);
    if (it != m_qBusy.end())
    {
        m_qBusy.erase(it);
        m_qIdle.push_back(pConn);
    }

    int nDelNum = m_qIdle.size() - m_nPoolSize;
    for (int i = 0; i < nDelNum; i++)
    {
        CMySQLConnection *pConn = m_qIdle.front();
        m_qIdle.pop_front();
        if (pConn)
            delete pConn, pConn = nullptr;
    }

    m_qBusy.shrink_to_fit();
    m_qIdle.shrink_to_fit();
}

CConnectionPool::CConnectionPool()
{
}

void CConnectionPool::Destroy()
{
    std::lock_guard<std::mutex> locker(m_mtx);

    while (!m_qIdle.empty())
    {
        CMySQLConnection *pConn = m_qIdle.front();
        m_qIdle.pop_front();
        if (pConn)
            delete pConn;
    }

    while (!m_qBusy.empty())
    {
        CMySQLConnection *pConn = m_qBusy.front();
        m_qBusy.pop_front();
        if (pConn)
            delete pConn;
    }
}

void CConnectionPool::AddIdelQueue()
{
    CMySQLConnection *pConn = new CMySQLConnection;
    if (pConn && pConn->Connect(m_User, m_Password, m_DbName, m_Hostname, m_Port, m_MultiSqlFlag))
        m_qIdle.push_back(pConn);
    else if (pConn)
        delete pConn, pConn = nullptr;
}
