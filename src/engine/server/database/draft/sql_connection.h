/* Copyright(C) 2025 - 2025 Comet */
#ifndef GAME_SERVER_GAMECORE_DATABASE_SQL_CONNECTION_H
#define GAME_SERVER_GAMECORE_DATABASE_SQL_CONNECTION_H

#include <mysql_connection.h>
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>
#include <deque>
#include <type_traits>
#include <string>
#include <utility> // For std::forward

class CSqlConnection
{
public:
    sql::ResultSet *m_pResult;
    sql::Connection *m_pConnection;
    sql::Statement *m_pStatement;
    sql::PreparedStatement *m_pPreparedStatement;

private:
	void Release();
	void FreeResult();
	bool Options(std::string Option, std::string Value);

private:
	template <typename T>
    typename std::enable_if<std::is_integral<T>::value>::type
    BindValue(int index, T value)
    {
        // 这里应该是设置整数值的代码
        m_pPreparedStatement->setInt(index, value);
    }

    // 通用模板函数
    template <typename T>
    typename std::enable_if<!std::is_integral<T>::value>::type
    BindValue(int index, T value)
    {
        // 这里应该是设置非整数值的代码
        m_pPreparedStatement->setString(index, value);
    }

	inline bool QueryImpl(std::deque<std::string> &vStrPack)
    {
        int Index = 0;
        for (const auto &value : vStrPack)
        {
            BindValue(Index, value);
            ++Index;
        }
        return true;
    }

public:
	CSqlConnection();
	~CSqlConnection();

	// Connect
	bool Connect(sql::Driver *pDriver, std::string User, std::string Password, std::string DbName, std::string Hostname, unsigned short Port);

	// Next
	bool Next();

	template <typename... Ts>
	bool Query(const std::string &Sql, Ts &&...args)
	{
		if (!PrepareQuery(Sql))
			return false;

		std::deque<std::string> vStrPack;
        AddToDeque(vStrPack, std::forward<Ts>(args)...);
	
		if (!QueryImpl(vStrPack))
			return false;

		return QueryPrepared();
	}

	template <typename... Ts>
	bool Execute(const std::string &Sql, Ts &&...args)
	{
		if (!PrepareQuery(Sql))
			return false;

		std::deque<std::string> vStrPack;
        AddToDeque(vStrPack, std::forward<Ts>(args)...);

		if (!QueryImpl(vStrPack))
			return false;

		return ExecutePrepared();
	}

	// 辅助函数，用于将参数包中的每个参数添加到 deque 中
    template <typename T, typename... Ts>
    void AddToDeque(std::deque<std::string>& deque, T&& arg, Ts&&... args)
    {
        deque.emplace_back(std::forward<T>(arg));
        AddToDeque(deque, std::forward<Ts>(args)...);
    }

    // 递归终止函数
    void AddToDeque(std::deque<std::string>&) {}

	bool PrepareQuery(const std::string &Sql);
	bool BindInt(int ParameterIndex, int Value);
	bool BindString(int ParameterIndex, const std::string &Value);
	bool ExecutePrepared();
	bool QueryPrepared();
};

#endif