#include "Task.h"
void Task::handle() {
	char buff[1024];
	getcwd(buff, sizeof(buff));
	if (jud == 0) {
		hptr->http_process();
	}
	else {
		bool ret = hptr->http_send();
		keep_live->set_value(ret);
	}
	return;
}