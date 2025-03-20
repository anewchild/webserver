#include "Sql_pool.h"
#include<mysql/mysql.h>
#include<error.h>
void Sql_pool::init(std::string url,int port,std::string user,std::string password,std::string database,int max_c) {
	m_url = url;
	m_port = port;
	m_user = user;
	m_password = password;
	m_Database = database;
	max_con = max_c;
	for (int i = 0; i < max_c; i++) {
		MYSQL* con = nullptr;
		con = mysql_init(con);
		if (con == nullptr) {
			std::cout << "error: " << mysql_error(con) << "\n";
			exit(0);
		}
		con = mysql_real_connect(con, m_url.c_str(), m_user.c_str(), m_password.c_str(), m_Database.c_str(), m_port, NULL, 0);
		if (con == nullptr) {
			std::cout << "error: " << mysql_error(con) << "\n";
		}
		sql_list.push_back(con);
	}
}
MYSQL* Sql_pool::get() {
	std::unique_lock<std::mutex>lck(mtx);
	cv.wait(lck, [this]() {
		return !this->sql_list.empty();
		});
	MYSQL* con = sql_list.front();
	sql_list.pop_front();
	return con;
}
void Sql_pool::pop(MYSQL* con) {
	if (con == NULL)return;
	std::unique_lock<std::mutex>lck(mtx);
	sql_list.push_back(con);
	return;
}
Sql_pool::~Sql_pool() {
	std::unique_lock<std::mutex>lck(mtx);
	auto it = sql_list.begin();
	for (; it != sql_list.end(); it++) {
		MYSQL* con = *it;
		mysql_close(con);
	}
	sql_list.clear();
	return;
}

sqlconnRAII::sqlconnRAII(MYSQL**con,Sql_pool* sql_p) {
	conRAII = sql_p->get();
	(*con) = conRAII;
	sql_pool = sql_p;
}
sqlconnRAII::~sqlconnRAII() {
	sql_pool->pop(conRAII);
}