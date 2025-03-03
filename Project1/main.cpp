#include <iostream>
#include <unistd.h>
#include<stdio.h>
//#include<iostream>
int main() {

    int pipefd[2];
	pipe(pipefd);
    int pid = fork();
    if (pid == 0) {
        // son
		close(pipefd[0]);
        printf("Imson\n");
        write(pipefd[1], "haijimi", 7);
    }
    else {
        //sleep(5);
        close(pipefd[1]);
        printf("Imfather\n");
        char buf[8]; 
		read(pipefd[0], buf, 7);
		printf("%s\n", buf);
    }
    return 0;
}
//#include<stdio.h>
//int main() {
//	printf("hello,world\n");
//	return 0;
//}