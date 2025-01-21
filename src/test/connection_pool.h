#ifndef __CONNECTION_POOL_H__
#define __CONNECTION_POOL_H__
 
#include <mysql_connection.h>
#include <mysql_driver.h>
#include <cppconn/exception.h>
#include <cppconn/driver.h>
#include <cppconn/connection.h>
#include <cppconn/resultset.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/statement.h>
#include <pthread.h>
#include <list>
#include <string>
 
using namespace std;
using namespace sql;
 
class ConnPool
{
public:
    ~ConnPool();
    // 获取数据库连接
    Connection* GetConnection();
    // 将数据库连接放回到连接池的容器中
    void ReleaseConnection(Connection *conn);
    // 获取数据库连接池对象
    static ConnPool* GetInstance();
 
private:
    // 当前已建立的数据库连接数量
    int curSize;
    // 连接池定义的最大数据库连接数
    int maxSize;
    string username;
    string password;
    string url;
    // 连接池容器
    list<Connection*> connList;
    // 线程锁
    pthread_mutex_t lock;
    static ConnPool* connPool;
    Driver* driver;
 
    // 创建一个连接
    Connection* CreateConnection();
    // 初始化数据库连接池
    void InitConnection(int iInitialSize);
    // 销毁数据库连接对象
    void DestoryConnection(Connection *conn);
    // 销毁数据库连接池
    void DestoryConnPool();
    // 构造方法
    ConnPool(string url, string user,string password, int maxSize);
};
 
#endif/*_CONNECTION_POOL_H */
 