#include <mysql/mysql.h>
#include <stdio.h>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <list>
#include <pthread.h>
#include <iostream>
#include "sql_connection_pool.h"

using namespace std;

connection_pool::connection_pool()
{
	_CurConn = 0;
	_FreeConn = 0;
}

connection_pool *connection_pool::GetInstance()
{
	static connection_pool connPool;
	return &connPool;
}

//构造初始化
void connection_pool::init(string url, string User, string PassWord, string DBName, int Port, int MaxConn, int close_log)
{
	url = url;
	Port = Port;
	User = User;
	PassWord = PassWord;
	DatabaseName = DBName;
	close_log = close_log;

	for (int i = 0; i < MaxConn; i++)
	{
		MYSQL *con = NULL;
		con = mysql_init(con);

		if (con == NULL)
		{
			LOG_ERROR("MySQL Error");
			exit(1);
		}
		con = mysql_real_connect(con, url.c_str(), User.c_str(), PassWord.c_str(), DBName.c_str(), Port, NULL, 0);

		if (con == NULL)
		{
			LOG_ERROR("MySQL Error");
			exit(1);
		}
		_connList.push_back(con);
		++_FreeConn;
	}

	_reserve = MySemaphore(_FreeConn);

	_MaxConn = _FreeConn;
}


//当有请求时，从数据库连接池中返回一个可用连接，更新使用和空闲连接数
MYSQL *connection_pool::GetConnection()
{
	MYSQL *con = NULL;

	if (0 == _connList.size())
		return NULL;

	_reserve.wait();
	
	MyLockGuard lk(_lock);

	con = _connList.front();
	_connList.pop_front();

	--_FreeConn;
	++_CurConn;

	return con;
}

//释放当前使用的连接
bool connection_pool::ReleaseConnection(MYSQL *con)
{
	if (NULL == con)
		return false;

	{	
		MyLockGuard lk(_lock);

		_connList.push_back(con);
		++_FreeConn;
		--_CurConn;
	}


	_reserve.post();
	return true;
}

//销毁数据库连接池
void connection_pool::DestroyPool()
{

	MyLockGuard lk(_lock);
	if (_connList.size() > 0)
	{
		for (auto it = _connList.begin(); it != _connList.end(); ++it)
		{
			mysql_close(*it);
		}
		_CurConn = 0;
		_FreeConn = 0;
		_connList.clear();
	}
}

//当前空闲的连接数
int connection_pool::GetFreeConn()
{
	return this->_FreeConn;
}

connection_pool::~connection_pool()
{
	DestroyPool();
}

connectionRAII::connectionRAII(MYSQL **SQL, connection_pool *connPool){
	*SQL = connPool->GetConnection();
	
	_conRAII = *SQL;
	_poolRAII = connPool;
}

connectionRAII::~connectionRAII(){
	_poolRAII->ReleaseConnection(_conRAII);
}