#pragma once
#include<mutex>
#include<condition_variable>
#define TIMEOUT 5
bool setnoblockfd(int sockfd);
