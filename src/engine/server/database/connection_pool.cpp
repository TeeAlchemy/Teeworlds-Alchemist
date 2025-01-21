#include <algorithm>
#include <base/system.h>

#include "connection_pool.h"

CConnectionPool *CConnectionPool::GetConnPool()
{
    static CConnectionPool sPool;
    return &sPool;
}

CConnectionPool::~CConnectionPool()
{
    Destroy();
}

bool CConnectionPool::Create(std::string User, std::string Password, std::string DbName, std::string Hostname, unsigned short Port, int PoolSize, int Timeout)
{
    m_User = User;
    m_Password = Password;
    m_DbName = DbName;
    m_Hostname = Hostname;
    m_Port = Port;
    m_nPoolSize = PoolSize;
    m_nTimeOut = Timeout;

    {
        std::lock_guard<std::mutex> locker(m_mtx);

        for (int i = 0; i < m_nPoolSize; i++)
            AddIdelQueue();
    }

    return m_nPoolSize == m_qIdle.size();
}

CSqlConnection *CConnectionPool::GetOneConn()
{
    CSqlConnection *pConn = nullptr;

    {
        std::lock_guard<std::mutex> locker(m_mtx);

        if (!m_qIdle.empty())
        {
            pConn = m_qIdle.front();
            m_qIdle.pop_front();

            if (pConn && pConn->m_pConnection->isValid())
                m_qBusy.push_back(pConn);
            else
            {
                // 删除无效的连接
                delete pConn;
                pConn = nullptr;

                // 创建新的连接
                pConn = new CSqlConnection;
                if (pConn && pConn->Connect(m_pDriver, m_User, m_Password, m_DbName, m_Hostname, m_Port))
                    m_qBusy.push_back(pConn);
                else
                {
                    // 如果连接失败，删除新创建的连接
                    delete pConn;
                    pConn = nullptr;
                }
            }
        }
        else
        {
            // 如果没有空闲连接，创建新的连接
            pConn = new CSqlConnection;
            if (pConn && pConn->Connect(m_pDriver, m_User, m_Password, m_DbName, m_Hostname, m_Port))
                m_qBusy.push_back(pConn);
            else
            {
                // 如果连接失败，删除新创建的连接
                delete pConn;
                pConn = nullptr;
            }
        }
    }

    return pConn;
}

void CConnectionPool::ReleaseOneConn(CSqlConnection *pConn)
{
    if (!pConn)
        return;

    std::lock_guard<std::mutex> locker(m_mtx);

    auto it = std::find(m_qBusy.begin(), m_qBusy.end(), pConn);
    if (it != m_qBusy.end())
    {
        m_qBusy.erase(it);
        m_qIdle.push_back(pConn);
    }

    // 修正删除连接的逻辑
    while (m_qIdle.size() > m_nPoolSize)
    {
        CSqlConnection *pConn = m_qIdle.front();
        m_qIdle.pop_front();
        delete pConn;
    }
}

CConnectionPool::CConnectionPool()
{
    m_pDriver = get_driver_instance();
}

void CConnectionPool::Destroy()
{
    std::lock_guard<std::mutex> locker(m_mtx);

    // 删除所有空闲连接
    while (!m_qIdle.empty())
    {
        CSqlConnection *pConn = m_qIdle.front();
        m_qIdle.pop_front();
        delete pConn;
    }

    // 删除所有忙碌连接
    while (!m_qBusy.empty())
    {
        CSqlConnection *pConn = m_qBusy.front();
        m_qBusy.pop_front();
        delete pConn;
    }
}

void CConnectionPool::AddIdelQueue()
{
    CSqlConnection *pConn = new CSqlConnection;
    if (pConn && pConn->Connect(m_pDriver, m_User, m_Password, m_DbName, m_Hostname, m_Port))
        m_qIdle.push_back(pConn);
    else
    {
        // 如果连接失败，删除新创建的连接
        delete pConn;
    }
}
