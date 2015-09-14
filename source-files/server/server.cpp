/*----------------------------------------------------------------------
--SOURCE FILE: server.cpp - an application that handles multiple clients
--requesting file data by sending them the contents of the requested
--file through an IPC message queue.
--
--PROGRAM: message queue server
--
--FUNCTIONS:
--int main()
--void closeIPC(int signo)
--int send_message(int msgID, struct msgbuffer * buffer)
--int read_message(int msgID, long type, struct msgbuffer * buffer)
--
--DATE: February 19, 2014
--
--REVISIONS: none
--
--DESIGNER: Joshua Campbell
--
--PROGRAMMER: Joshua Campbell
--
--NOTES:
--Program asks for a message queue id to create. It then receives data
--from the client applications that it uses to send them file contents
--by using an IPC message queue.
--Program displays the order clients join and the order they finish.
----------------------------------------------------------------------*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/resource.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <fstream>
#include <signal.h>
#include <iostream>
#include "resource.h"

using namespace std;

struct msgbuffer {
	long mtype;
	char mtext[MSGSIZE];
	pid_t client;
	int priority;
};

int msq_id;
struct msqid_ds msq_status;

/*----------------------------------------------------------------------
--FUNCTION: main
--
--DATE: February 19, 2014
--
--REVISIONS: none
--
--DESIGNER: Joshua Campbell
--
--PROGRAMMER: Joshua Campbell
--
--RETURNS: int
--
--NOTES:
--Main server process, creates sending child processes to handle
--multiple clients. Reads and sends data to clients.
----------------------------------------------------------------------*/
int main() {
	key_t mkey;
	int tempkey = 0;
	struct msgbuffer readbuffer, sendbuffer;
	ifstream file;
	int i = 0;
	bool endoffile = false;
	pid_t tmpid, childpid;
	bool master = true;
	
	do {
		cout << "Please enter a message queue id for creation: ";
	} while(!(cin >> tempkey));
	
	mkey = (key_t)tempkey;
	
	if ((msq_id = msgget (mkey, IPC_CREAT)) < 0)
    {
		perror ("msgget failed!");
		exit(2);
    }
    
    sendbuffer.mtype = 2;
    sendbuffer.client = 0;
    sendbuffer.priority = 0;
    
    signal(SIGINT, closeIPC);
    
    while(master) {
    	if (read_message(msq_id, 1, &readbuffer)) {
    		printf("Client connected: %d\n", (int)readbuffer.client);
    		tmpid = fork();
    		if (tmpid == 0) {
    			childpid = getpid();
    			master = false;
    			if (readbuffer.priority >= -20 && readbuffer.priority <= 19) {
    				if (setpriority(PRIO_PROCESS, childpid, readbuffer.priority) == -1) {
    					perror("failed to change process priority.");
    				}
    			}
    		}
    	}
    }
    sendbuffer.mtype = (int)readbuffer.client;
    file.open(readbuffer.mtext);
    while(!endoffile) {
		for (i = 0; i < MSGSIZE; i++) {
			if ((sendbuffer.mtext[i] = file.get()) == EOF) {
				endoffile = true;
				break;
			}
		}
		send_message(msq_id, &sendbuffer);
    }
    sendbuffer.mtext[0] = 0x04;
	send_message(msq_id, &sendbuffer);    
    
    printf("Client %d disconnected\n", (int)readbuffer.client);
    file.close();
    if (tmpid != 0) {
		do {
			msgctl (msq_id, IPC_STAT, &msq_status);
		} while (((int)msq_status.msg_qnum) != 0);
		
		if (msgctl (msq_id, IPC_RMID, 0) < 0)
		{
			perror ("msgctl (remove queue) failed!");
			exit (3);
		}
	}
	
	return 0;
}

/*----------------------------------------------------------------------
--FUNCTION: send_message
--
--DATE: February 5, 2014
--
--REVISIONS: none
--
--DESIGNER: Aman Abdulla
--
--PROGRAMMER: Aman Abdulla and Joshua Campbell
--
--PARAMS: int msgID - the identifier of the message queue that is going
					  to be sending data.
		  struct msgbuffer * buffer - buffer that file content is stored
		  							  in before writing to the message
		  							  queue.
--
--RETURNS: int
--
--NOTES:
--Wrapper function for the msgsnd command. Places data in the message
--queue.
----------------------------------------------------------------------*/
int send_message(int msgID, struct msgbuffer * buffer) {
	int result = 0, length = 0;
	
	length = sizeof(struct msgbuffer) - sizeof(long);
	
	if ((result = msgsnd(msgID, buffer, length, 0)) == -1) {
		perror("Error occurred while sending message.");
	}
	return result;
}

/*----------------------------------------------------------------------
--FUNCTION: read_message
--
--DATE: February 5, 2014
--
--REVISIONS: none
--
--DESIGNER: Aman Abdulla
--
--PROGRAMMER: Aman Abdulla and Joshua Campbell
--
--PARAMS: int msgID - the identifier of the message queue that is going
					  to be read from.
		  long type - the message type that is received.
		  struct msgbuffer * buffer - buffer that client data is read
		  							  into.
--
--RETURNS: int
--
--NOTES:
--Wrapper function for the msgrcv command. Reads data out of the message
--queue.
----------------------------------------------------------------------*/
int read_message(int msgID, long type, struct msgbuffer * buffer) {
	int result = 0, length = 0;
	
	length = sizeof(struct msgbuffer) - sizeof(long);
	
	if ((result = msgrcv(msgID, buffer, length, type, 0)) == -1) {
		perror("Error occurred while receiving message.");
	}
	return result;
}

/*----------------------------------------------------------------------
--FUNCTION: closeIPC
--
--DATE: February 19, 2014
--
--REVISIONS: none
--
--DESIGNER: Joshua Campbell
--
--PROGRAMMER: Joshua Campbell
--
--PARAMS: int signo - signal interrupt that occurred
--
--RETURNS: void
--
--NOTES:
--Function that handles closing down the message queue and exiting the
--server upon receiving a SIGINT interrupt.
----------------------------------------------------------------------*/
void closeIPC(int signo) {
	do {
		msgctl (msq_id, IPC_STAT, &msq_status);
	} while (((int)msq_status.msg_qnum) != 0);
	
	if (msgctl (msq_id, IPC_RMID, 0) < 0)
	{
		perror ("msgctl (remove queue) failed!");
	}
	printf("\nClosed Message Queue.\n");
	exit(0);
}

