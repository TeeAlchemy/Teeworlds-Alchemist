#if defined(CONF_SQL)
// I don't think this is smart
// So please rewrite it
// I will not rewrite it because I don't know how to do it(im poor u know)
// Someone else please help me.

#ifndef ENGINE_SERVER_DATABASE_CONNECTION_POOL_H
#define ENGINE_SERVER_DATABASE_CONNECTION_POOL_H

#include <mysql_connection.h>
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>
#include <iostream>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>

class CMySQLConnectionPool
{
public:
    CMySQLConnectionPool(const std::string &Server,
                         const std::string &Username,
                         const std::string &Password,
                         const std::string &Database,
                         int maxConn = 10)
    {
        for (int i = 0; i < maxConn; ++i)
            m_Connections.push(CreateConnection(Server, Username, Password, Database));
    }

    std::shared_ptr<sql::Connection> GetConnection()
    {
        std::unique_lock<std::mutex> lock(m_SqlMutex);
        while (m_Connections.empty())
        {
            m_SqlCondition.wait(lock);
        }
        std::shared_ptr<sql::Connection> pConnect = m_Connections.front();
        m_Connections.pop();
        return pConnect;
    }

    void ReleaseConnection(std::shared_ptr<sql::Connection> pConnection)
    {
        std::lock_guard<std::mutex> lock(m_SqlMutex);
        m_Connections.push(pConnection);
        m_SqlCondition.notify_one();
    }

private:
    std::queue<std::shared_ptr<sql::Connection>> m_Connections;
    std::mutex m_SqlMutex;
    std::condition_variable m_SqlCondition;

    std::shared_ptr<sql::Connection> CreateConnection(const std::string &Server,
                                                      const std::string &Username,
                                                      const std::string &Password,
                                                      const std::string &Database)
    {
        sql::Driver *pDriver = get_driver_instance();
        std::shared_ptr<sql::Connection> pConnect(pDriver->connect(Server, Username, Password));
        pConnect->setSchema(Database);
        return pConnect;
    }
};

#endif // !
#endif