/* Copyright(C) 2025 - 2025 Comet */
#ifndef GAME_SERVER_GAMECORE_DATABASE_SQL_CONNECTION_H
#define GAME_SERVER_GAMECORE_DATABASE_SQL_CONNECTION_H

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

    // Connect
	bool Connect(sql::Driver *pDriver, std::string User, std::string Password, std::string DbName, std::string Hostname, unsigned short Port);

	// Query
	bool Query(std::string Sql);

	// Next
	bool Next();

	// Execute
	bool Execute(std::string Sql);

private:
	void Release();
	void FreeResult();
	bool Options(std::string Option, std::string Value);

};

#endif