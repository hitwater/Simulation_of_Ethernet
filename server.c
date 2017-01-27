#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>
#include <fcntl.h>

#define BUFF_SIZE 10000

//Log types
#define RECEIVE 1
#define TRANSFER 2
#define COLLISION 3

//set the maximum connections to 10
#define MaxConn 10

typedef enum { false, true } boolean;

void dataProcess(int iStationProcesses[], int iStationID[], int iStationsCount);
void WriteLog(int iLogType, int iFramePart, int iFrame, int iSrcStatID, int iDestinationStation);

int main()
{

	int iStationsCount=0;
	int iStationsConnected;
	int iSockFD;
	int iPortNum;
	int iStationProcesses[MaxConn];
	int iStationID[MaxConn];
	int iTrue=1;
	int iSizeOfSocket;
	char cBuffer[BUFF_SIZE];
	struct sockaddr_in sockServAddr,sockClientAddr;

	printf("\nEnter port number: ");
	scanf("%d",&iPortNum);

	//Create socket
	if((iSockFD = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP)) < 0){
		printf("\nError : Socket Creation Failed.");
		exit(1);
	}

	//Set Socket
	setsockopt(iSockFD,SOL_SOCKET,SO_REUSEADDR,&iTrue,sizeof(int));
	sockServAddr.sin_family = AF_INET;
	sockServAddr.sin_port = htons(iPortNum);
	sockServAddr.sin_addr.s_addr = INADDR_ANY;
	bzero(&sockServAddr.sin_zero,8);

	//Binding
	if(bind(iSockFD,(struct sockaddr *)&sockServAddr, sizeof(struct sockaddr)) < 0){
		printf("\nError : Bind Faild.");
		exit(1);
	}

	//Setting maximum connections
	if(listen(iSockFD, MaxConn) == -1){
		printf("\nError : Maximum Connection Setting Failed.");
		exit(1);
	}

	//Getting number of Station Processes which will connect
	printf("\nEnter number of Station processes (up to 10): ");
	scanf("%d",&iStationsCount);

	if(iStationsCount > MaxConn)
	{
		printf("\nError : Maximum allowed connection(s) %d", MaxConn);
		exit(1);
	}

	printf("\nServer startup.");
	printf("\nWaiting for station processes to connect...");
	fflush(stdout);

	//Loop till all the station processes connects
	for(iStationsConnected=0; iStationsConnected < iStationsCount; iStationsConnected++){
		iSizeOfSocket = sizeof(struct sockaddr_in);
		iStationProcesses[iStationsConnected] = accept(iSockFD, (struct sockaddr *)&sockClientAddr,&iSizeOfSocket);
		read(iStationProcesses[iStationsConnected],cBuffer,1024);
		sscanf(cBuffer,"%d",&iStationID[iStationsConnected]);
		printf("\nConnected with station process of ID : %d",iStationID[iStationsConnected]);
		fflush(stdout);

	}

	printf("\nAll station processes connected. Begin of data transmission...");
	fflush(stdout);

	//handling  of data transmission
	dataProcess(iStationProcesses, iStationID, iStationsCount);
	close(iSockFD);
}

//Handle data between server and client
//input:
/*
 * int[] iStationProcesses - array of Station Processes
 * int[] iStationIDs - array of Station ID's Frame Number
 * int iStationsCount - total number of station processes involved
 *
 */
void dataProcess(int iStationProcesses[], int iStationIDs[], int iStationsCount){

	int iIndex;
	int iFrameBuff;
	int iFramePartBuff;
	int iSrcStatIDBuff;
	int iDestStatIDBuff;

	int iFrame;	//Frame ID
	int iFramePart;	//Frame part ID

	int iSrcStatID;	//Source station ID
	int iDestStatID;	//Destination station ID

	int iMessagesRecv;
	int iStatProcIndex;
	boolean bufferHasData=false;
	boolean isFirstFrameSent=false;
	char cReceiveBuffer[BUFF_SIZE];
	char cResponseBuffer[BUFF_SIZE];

	//Network related variables
	int iHighestSocket=0;

	//to use with select()
	fd_set fdSocks;

	//for storing timeout, which will be used in select()
	struct timeval strTimeout;

	//highest socket required to use in select
	for(iIndex = 0; iIndex < iStationsCount; iIndex++)
		if(iStationProcesses[iIndex] > iHighestSocket)
			iHighestSocket = iStationProcesses[iIndex];

	//Data simulation starts
	while(1)
	{
		//Initialize fd_set structure
		FD_ZERO(&fdSocks);
		for(iIndex = 0; iIndex < iStationsCount; iIndex++) {
			FD_SET(iStationProcesses[iIndex],&fdSocks);
		}

		//Initialize time out
		strTimeout.tv_sec = 1;
		strTimeout.tv_usec = 0;

		//Check for incoming messages
		iMessagesRecv = select(iHighestSocket + 1, &fdSocks, (fd_set *) 0, (fd_set *) 0, &strTimeout);


		if(iMessagesRecv == 0){	//No messages
			//Continue with the buffered data
			if(bufferHasData){
				//Write a transfer log
				WriteLog(TRANSFER, iFramePartBuff, iFrameBuff, iSrcStatIDBuff, iDestStatIDBuff);
				if(iFramePartBuff == 2){
					for(iIndex = 0; iIndex < iStationsCount; iIndex++){
						if(iStationIDs[iIndex]==iSrcStatIDBuff){
							sprintf(cResponseBuffer,"%s","SUCC");
							//Transmission successfully completed
							write(iStationProcesses[iIndex],cResponseBuffer,strlen(cResponseBuffer));
							isFirstFrameSent=false;
						}
					}
				}
				else {
					isFirstFrameSent=true;
				}
				//Now there should be no data in buffer
				bufferHasData=false;
			}
		}	else if(iMessagesRecv == 1) //One message
		{
			//Identify the station process
			for(iIndex = 0; iIndex < iStationsCount; iIndex++) {
				if(FD_ISSET(iStationProcesses[iIndex],&fdSocks)) {
					iStatProcIndex=iIndex;
				}
			}

			//Read message
			read(iStationProcesses[iStatProcIndex],cReceiveBuffer,1024);
			sprintf(cResponseBuffer,"%s","RECE");
			sscanf(cReceiveBuffer,"%d %d %d %d", &iFramePart, &iFrame, &iSrcStatID, &iDestStatID);
			//Write a log about receiving
			WriteLog(RECEIVE, iFramePart, iFrame, iSrcStatID, iDestStatID);

			//Checking frames in buffer
			if(bufferHasData == false){

				if((isFirstFrameSent && (iSrcStatIDBuff!=iSrcStatID || iDestStatIDBuff!= iDestStatID))  ){
									sprintf(cResponseBuffer,"%s","COLL");
									for(iIndex = 0; iIndex < iStationsCount; iIndex++){
										if(iStationIDs[iIndex]==iSrcStatIDBuff || iStationIDs[iIndex]==iSrcStatID){
											write(iStationProcesses[iIndex],cResponseBuffer,strlen(cResponseBuffer));
										}
									}
									WriteLog(COLLISION, iFramePartBuff, iFrameBuff, iSrcStatIDBuff, -1);
									WriteLog(COLLISION, iFramePartBuff, iFrameBuff, iSrcStatID, -1);
									bufferHasData = false;
									isFirstFrameSent=false;
								} else{
									iFramePartBuff = iFramePart;
									iFrameBuff = iFrame;
									iSrcStatIDBuff = iSrcStatID;
									iDestStatIDBuff = iDestStatID;
									bufferHasData = true;
								}
			} else {
				if(iSrcStatIDBuff == iSrcStatID){
					WriteLog(TRANSFER, iFramePartBuff, iFrameBuff, iSrcStatIDBuff, iDestStatIDBuff);
					iFramePartBuff = iFramePart;
					iFrameBuff = iFrame;
					isFirstFrameSent=true;
				}	else{
						sprintf(cResponseBuffer,"%s","COLL");
						for(iIndex = 0; iIndex < iStationsCount; iIndex++){
							if(iStationIDs[iIndex]==iSrcStatIDBuff || iStationIDs[iIndex]==iSrcStatID){
								write(iStationProcesses[iIndex],cResponseBuffer,strlen(cResponseBuffer));
							}
						}
						WriteLog(COLLISION, iFramePartBuff, iFrameBuff, iSrcStatIDBuff, -1);
						WriteLog(COLLISION, iFramePartBuff, iFrameBuff, iSrcStatID, -1);
						bufferHasData = false;
						isFirstFrameSent=false;
				}
			}
			if(strcmp(cReceiveBuffer,"COLL")!=0)
				write(iStationProcesses[iStatProcIndex],cResponseBuffer,strlen(cResponseBuffer));
		} else { //More than 1 message (Collision)
			sprintf(cResponseBuffer,"%s","COLL");
			for(iIndex = 0; iIndex < iStationsCount; iIndex++){
				if(FD_ISSET(iStationProcesses[iIndex],&fdSocks) || (bufferHasData && iStationIDs[iIndex]==iSrcStatIDBuff)){
					read(iStationProcesses[iIndex],cReceiveBuffer,1024);
					write(iStationProcesses[iIndex],cResponseBuffer,strlen(cResponseBuffer));
					WriteLog(COLLISION, iFramePartBuff, iFrameBuff, iStationIDs[iIndex], -1);
				}
			}

			bufferHasData=false;
			isFirstFrameSent=false;
		}
	}
}

//Write log to output.txt
//input:
/*
 * int iLogType: currently support receive, transfer and collision
 * int iFramePart: the ID of the part of frame that is sent
 * int iFrame: Frame ID
 * int iSrcStatID: ID of the source station
 * int iDestinationStation: ID of the destination station
 *
 */
void WriteLog(int iLogType, int iFramePart, int iFrame, int iSrcStatID, int iDestinationStation){

	FILE *fp;

	fp = fopen("output.txt","a");

	switch(iLogType){

		case RECEIVE:
			fprintf(fp,"\nReceive part %d of frame %d from Station %d, to Station %d", iFramePart, iFrame, iSrcStatID, iDestinationStation);
			break;
		case TRANSFER:
			fprintf(fp,"\nTransfer part %d of frame %d from Station %d, to Station %d", iFramePart, iFrame, iSrcStatID, iDestinationStation);
			break;
		case COLLISION:
			fprintf(fp,"\nInform Station %d a collision",iSrcStatID);
			break;

	}
	fclose(fp);
}
