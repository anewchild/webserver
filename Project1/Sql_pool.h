#pragma once
#include<condition_variable>
#include<mutex>
#include<list>
#include<mysql/mysql.h>
#include<string>
#include<iostream>
class Sql_pool
{
private:
	Sql_pool() {};
	~Sql_pool();
	int max_con;
	std::mutex mtx;
	std::condition_variable cv;
	std::string m_url;
	int m_port;
	std::string m_user;
	std::string m_password;
	std::string m_Database;
	std::list<MYSQL*>sql_list;
public:
	static Sql_pool* get_instance() {
		static Sql_pool s_sql_pool;
		return &s_sql_pool;
	}
	void init(std::string url, int port, std::string user, std::string password, std::string database, int max_c);
	MYSQL* get();
	void pop(MYSQL*);
};

class sqlconnRAII {
public:
	sqlconnRAII(MYSQL **con,Sql_pool* sql_p);
	~sqlconnRAII();
private:
	MYSQL* conRAII;
	Sql_pool* sql_pool;
};