#include"WebServer.h"
#include<stdio.h>
int main() {
	WebServer* server = WebServer::get_instance();
	//WebServer server;
	printf("hello,world\n");
	if (!server->init()) {
		return -1;
	}
	if (!server->listen_loop()) {
		return -1;
	}
	return 0;
}