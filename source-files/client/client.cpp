/*----------------------------------------------------------------------
--SOURCE FILE: client.cpp - an application for requesting file contents
--from a server application. Reads and sends information using an IPC
--message queue.
--
--PROGRAM: message queue client
--
--FUNCTIONS:
--int main()
--void * readThread(void * data)
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
--Program asks for certain data such as a filename, a message queue id
--to join, and a priority setting. It then sends this data to the server
--application. The program then initiates a reading thread to read the
--response data from the server.
----------------------------------------------------------------------*/

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
#include "resource.h"

#define MSGSIZE 1024

using namespace std;

struct threadData {
	int id;
	int msq_id;
};

struct msgbuffer {
	long mtype;
	char mtext[MSGSIZE];
	pid_t client;
	int priority;
};

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
--Main client process, sends file name to server and creates reading
--thread in order to read response data.
----------------------------------------------------------------------*/
int main() {
	string filename;
	int prio = 0;
	key_t mkey;
	int tempkey = 0;
	int msq_id;
	struct msgbuffer sendbuffer;
	pid_t client = getpid();
	pthread_t readerThread;
	struct threadData data;
	
	do {
		cout << "Please enter a message queue id to join: ";
	} while(!(cin >> tempkey));
	
	mkey = (key_t)tempkey;
	
	if ((msq_id = msgget (mkey, IPC_CREAT)) < 0)
    {
		perror ("msgget failed!");
		exit(2);
    }
    
    do {
		cout << "Please set your priority --0 (low) to 10 (high)--: ";
		cin >> prio;
		if (!(prio >= 0 && prio <= 10)) {
			cout << "Your priority level is not valid." << endl;
		}
    } while (!(prio >= 0 && prio <= 10));
    
    prio = 10 - prio;
	
	cout << "Please enter a file name: ";
	cin >> filename;
	
	sendbuffer.mtype = 1;
	
	if (filename.size() <= MSGSIZE) {
    	strcpy(sendbuffer.mtext, filename.c_str());
    }
    sendbuffer.client = client;
    sendbuffer.priority = prio;
	
	send_message(msq_id, &sendbuffer);
	
	
	data.id = (int)client;
	data.msq_id = msq_id;
	
	pthread_create(&readerThread, NULL, readThread, &data);
	
	pthread_join(readerThread, NULL);
	
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
		  struct msgbuffer * buffer - buffer that client data is stored
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
		  struct msgbuffer * buffer - buffer that server data is read
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
--FUNCTION: readThread
--
--DATE: February 19, 2014
--
--REVISIONS: none
--
--DESIGNER: Joshua Campbell
--
--PROGRAMMER: Joshua Campbell
--
--RETURNS: void *
--
--NOTES:
--Thread for reading data/file contents out of the message queue.
--Reads until it finds an EOT ASCII character at the front of the text
--buffer.
----------------------------------------------------------------------*/
void * readThread(void * data) {
	struct threadData * tdata = (struct threadData *)data;
	int client = tdata->id;
	int msq_id = tdata->msq_id;
	struct msgbuffer recvbuffer;
	bool endoftransmission = false;

	while(!endoftransmission) {
		read_message(msq_id, client, &recvbuffer);
		if (recvbuffer.mtext[0] == 0x04) {
			endoftransmission = true;
		} else {
			printf("%s", recvbuffer.mtext);
		}
	}
	
	return NULL;
}

