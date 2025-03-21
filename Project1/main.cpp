#include"WebServer.h"
#include"Sql_pool.h"
#include<stdio.h>
int main() {
	WebServer* server = WebServer::get_instance();
	auto sql_pool = Sql_pool::get_instance();
	sql_pool->init("localhost", 9006, "root", "123456", "tinyweb", 4);
	http_init_sql();
	if (!server->init()) {
		return -1;
	}
	if (!server->listen_loop()) {
		return -1;
	}
	return 0;
}