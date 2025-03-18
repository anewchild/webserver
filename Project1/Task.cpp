#include "Task.h"
void Task::handle() {
	char buff[1024];
	getcwd(buff, sizeof(buff));
	if (jud == 0) {
		hptr->http_process();
	}
	else {
		hptr->http_send();
		//hptr->erasefd();
	}
	return;
}