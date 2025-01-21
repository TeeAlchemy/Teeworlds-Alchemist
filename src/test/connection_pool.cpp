#include <stdexcept>
#include <exception>
#include <stdio.h>
#include "connection_pool.h"
 
using namespace std;
using namespace sql;
 
ConnPool* ConnPool::connPool = NULL;
 
// 获取连接池对象，单例模式
ConnPool* ConnPool::GetInstance()
{
    if (connPool == NULL)
    {
        connPool = new ConnPool("tcp://192.168.213.130:3306", "root", "", 20);
    }
    
    return connPool;
}
 
// 数据库连接池的构造函数
ConnPool::ConnPool(string url, string userName, string password, int maxSize)
{
    this->maxSize = maxSize;
    this->curSize = 0;
    this->username = userName;
    this->password = password;
    this->url = url;
    
    try
    {
        this->driver = sql::mysql::get_driver_instance();
    }
    catch (sql::SQLException& e)
    {
        perror("get driver error.\n");
    }
    catch (std::runtime_error& e)
    {
        perror("[ConnPool] run time error.\n");
    }
 
    // 在初始化连接池时，建立一定数量的数据库连接
    this->InitConnection(maxSize/2);
}
 
// 初始化数据库连接池，创建最大连接数一半的连接数量
void ConnPool::InitConnection(int iInitialSize)
{
    Connection* conn;
    pthread_mutex_lock(&lock);
    
    for (int i = 0; i < iInitialSize; i++)
    {
        conn = this->CreateConnection();
        if (conn)
        {
            connList.push_back(conn);
            ++(this->curSize);
        }
        else
        {
            perror("Init connection error.");
        }
    }
    
    pthread_mutex_unlock(&lock);
}
 
// 创建并返回一个连接
Connection* ConnPool::CreateConnection()
{
    Connection* conn;
    try
    {
        // 建立连接
        conn = driver->connect(this->url, this->username, this->password);
        return conn;
    }
    catch (sql::SQLException& e)
    {
        perror("create connection error.");
        return NULL;
    }
    catch (std::runtime_error& e)
    {
        perror("[CreateConnection] run time error.");
        return NULL;
    }
}
 
// 从连接池中获得一个连接
Connection* ConnPool::GetConnection()
{
    Connection* con;
    pthread_mutex_lock(&lock);
 
    // 连接池容器中还有连接
    if (connList.size() > 0)
    {
        // 获取第一个连接
        con = connList.front();
        // 移除第一个连接
        connList.pop_front();
        // 判断获取到的连接的可用性
        // 如果连接已经被关闭，删除后重新建立一个
        if (con->isClosed())
        {
            delete con;
            con = this->CreateConnection();
            // 如果连接为空，说明创建连接出错
            if (con == NULL)
            {
                // 从容器中去掉这个空连接
                --curSize;
            }
        }
        
        pthread_mutex_unlock(&lock);
        return con;
    }
    // 连接池容器中没有连接
    else
    {
        // 当前已创建的连接数小于最大连接数，则创建新的连接
        if (curSize < maxSize)
        {
            con = this->CreateConnection();
            if (con)
            {
                ++curSize;
                pthread_mutex_unlock(&lock);
                return con;
            }
            else
            {
                pthread_mutex_unlock(&lock);
                return NULL;
            }
        }
        // 当前建立的连接数已经达到最大连接数
        else
        {
            perror("[GetConnection] connections reach the max number.");
            pthread_mutex_unlock(&lock);
            return NULL;
        }
    }
}
 
// 释放数据库连接，将该连接放回到连接池中
void ConnPool::ReleaseConnection(sql::Connection* conn)
{
    if (conn)
    {
        pthread_mutex_lock(&lock);
        
        connList.push_back(conn);
        
        pthread_mutex_unlock(&lock);
    }
}
 
// 数据库连接池的析构函数
ConnPool::~ConnPool()
{
    this->DestoryConnPool();
}
 
// 销毁连接池，需要先销毁连接池的中连接
void ConnPool::DestoryConnPool()
{
    list<Connection*>::iterator itCon;
    pthread_mutex_lock(&lock);
    
    for (itCon = connList.begin(); itCon != connList.end(); ++itCon)
    {
        // 销毁连接池中的连接
        this->DestoryConnection(*itCon);
    }
    curSize = 0;
    // 清空连接池中的连接
    connList.clear();
 
    pthread_mutex_unlock(&lock);
}
 
// 销毁数据库连接
void ConnPool::DestoryConnection(Connection* conn)
{
    if (conn)
    {
        try
        {
            // 关闭连接
            conn->close();
        }
        catch(sql::SQLException& e)
        {
            perror(e.what());
        }
        catch(std::exception& e)
        {
            perror(e.what());
        }
        // 删除连接
        delete conn;
    }
}