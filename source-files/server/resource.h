#ifndef RESOURCE_H
#define RESOURCE_H

#include <stdio.h>
#include <iostream>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <errno.h>
#include <cstring>
#include <pthread.h>

#define MSGSIZE 1024

struct msgbuffer;

int send_message(int msgID, struct msgbuffer * buffer);
int read_message(int msgID, long type, struct msgbuffer * buffer);
void closeIPC(int signo);

#endif
