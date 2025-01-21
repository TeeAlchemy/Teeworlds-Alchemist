#pragma once
#include <mysql/mysql.h>
#include <string>

class CMySQLConnection
{
public:
	CMySQLConnection();
	~CMySQLConnection();

	// 连接
	bool Connect(std::string User, std::string Password, std::string DbName, std::string Hostname, unsigned short Port, unsigned long MultiSqlFlag = 0);

	// 设置编码格式
	void SetCharacter(std::string StrCsName);

	// 设置连接超时时间
	bool SetConnTimeout(int Second);

	// 设置自动重连
	bool SetReconnect(bool Reconn = false);

	// 查询
	bool Query(std::string Sql);

	// 逐行遍历结果集
	bool Next();

	// 得到结果集中的字段值
	std::string Value(int Index);

	// 增、删、改
	bool Execute(std::string Sql);

	// 字符转换
	bool ConvertSqlString(char *DataFrom, char *DataTo, unsigned long FromLen, unsigned long *ToLen = nullptr);

	// 事务操作
	bool Transaction(bool Auto);

	// 提交事务
	bool Commit();

	// 事务回滚
	bool Rollback();

	// 判断连接是否可用
	bool IsConnect();

	int GetIntValue(int Index);
private:
	void Init();
	void Release();
	void FreeResult();
	bool Options(mysql_option opt, const void *arg);

private:
	MYSQL *m_pConnection = nullptr;
	MYSQL_RES *m_pResult = nullptr;
	MYSQL_ROW m_pRow = nullptr;
};
