/* Copyright(C) 2022 - 2024 ST-Chara */
#ifndef GAME_SERVER_GAMECORE_DATABASE_DB_H
#define GAME_SERVER_GAMECORE_DATABASE_DB_H

#include <mysql_connection.h>

#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>

class CSqlConnection
{
public:
    sql::ResultSet *m_pResult;
	sql::Connection *m_pConnection;
	sql::Statement *m_pStatement;

public:
    CSqlConnection();
    ~CSqlConnection();

    // 连接
	bool Connect(sql::Driver *pDriver, std::string User, std::string Password, std::string DbName, std::string Hostname, unsigned short Port);

    // 设置编码格式
	bool SetCharacter(std::string StrCsName);

	// 设置连接超时时间
	bool SetConnTimeout(int Second);

	// 设置自动重连
	bool SetReconnect(bool Reconn);

	// 查询
	bool Query(std::string Sql);

	// 逐行遍历结果集
	bool Next();

	// 增、删、改
	bool Execute(std::string Sql);

private:
	void Release();
	void FreeResult();
	bool Options(std::string Option, std::string Value);

};

#endif