#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <sys/time.h>
#include <fcntl.h>

//Log types
#define RECEIVE 1 //NOT REALLY USED
#define SEND 2
#define COLLISION 3

#define BUFF_SIZE 10000

typedef enum { false, true } boolean;

int iSockFD;
int iStatID;
char cSendBuffer[BUFF_SIZE];
char cReceiveBuffer[BUFF_SIZE];

void WriteLog(int iLogType, int iFramePart, int iFrame, int iStationID, int iTimeSlot);
void sendData();

int main(){

	int iPortNum;
	int iStationID;
	char cServerIP[50];
	struct 	hostent *host;
	struct  sockaddr_in sockServAddr;

	//Input the ID of this client station
	printf("\nEnter Client ID: ");
	scanf("%d",&iStatID);


	//Input the server's IP address and port
	printf("\nEnter SERVER\'s IP: ");
	scanf("%s",cServerIP);
	printf("\nEnter SERVER\'s port number: ");
	scanf("%d",&iPortNum);

	//Create Socket
	if((iSockFD = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP)) < 0)
	{
		printf("\nError : Socket Creation Failed.");
		exit(1);
	}

	//Set Socket
	host = gethostbyname(cServerIP);
	sockServAddr.sin_family = AF_INET;
	sockServAddr.sin_port = htons(iPortNum);
	bcopy((char *)host->h_addr, (char *) &sockServAddr.sin_addr.s_addr, host->h_length);
	bzero(&(sockServAddr.sin_zero),8);

	//Connect to server
	if(connect(iSockFD,(struct sockaddr *)&sockServAddr, sizeof(struct sockaddr)) == -1)
	{
		printf("\nError : Connection Failed.");
		exit(1);
	}

	sprintf(cSendBuffer,"%d",iStatID);

	write(iSockFD,cSendBuffer,strlen(cSendBuffer));

	sendData();

	close(iSockFD);
}


void sendData()
{

	FILE *fp;
	char cTemp[3][100]; //To save the unneeded string from input
	char cFileName[20];
	char cLine[100];
	char iRespCode[BUFF_SIZE];
	char ch;
	int iCharCount=0;

	int iFrame;	//Frame ID
	int iFramePart;	//Frame part ID

	int iDestStatID;	//Destination station ID
	int iRandomTimePeriod; //Random number to be used to calculate the wait time slots
	int iCollisionCount=0; //How many times a collision happened
	int iNumOfTimeSlots=0; //Actual wait time slots
	int i2Powers[10]={2,4,8,16,32,64,128,256,512,1024}; //Just to use easily...
	boolean isFrameSent=false;

	//Try to read input.txt
	sprintf(cFileName,"input.txt");
	fp = fopen(cFileName,"r");
	if(!fp)
	{
		printf("\nError : Unable to open input.txt.");
		exit(1);
	}

    //Generate random time slot period
    iRandomTimePeriod=random()%1000+1;

    //Read and process the input information
    while (!feof(fp)) {
    	ch = getc(fp);
    	iCharCount=0;
    	while ((ch != '\n') && (ch != EOF)) {
            cLine[iCharCount++] = ch;
            ch = getc(fp);
        }
        cLine[iCharCount]='\0';

        if(iCharCount==0){
    		break;
    	}

    	sscanf(cLine,"%s %d, %s %s %d",cTemp[0], &iFrame, cTemp[1], cTemp[2], &iDestStatID);

		fflush(stdout);

		iFramePart=1;
		sprintf(cSendBuffer, "%d %d %d %d",iFramePart,iFrame,iStatID, iDestStatID);

		//Send the data
		write(iSockFD,cSendBuffer,strlen(cSendBuffer));
		WriteLog(SEND, iFramePart, iFrame, iDestStatID, 0);

		read(iSockFD,cReceiveBuffer,1024);
		sscanf(cReceiveBuffer,"%s", &iRespCode);
		iFramePart=2;
		isFrameSent=false;

		while(strcmp(iRespCode,"SUCC")!=0 && isFrameSent==false)
		{
			if(strcmp(iRespCode,"COLL")==0){
				iCollisionCount++;
				iFramePart=1;
				isFrameSent=false;
				//calculate number of time slots to sleep

				if(iCollisionCount>10){	//Too much collision
					//After 16 collisions terminate data transmission
					if(iCollisionCount == 16){
						fclose(fp);
						return;
					}
					iNumOfTimeSlots=random()%1024;
				}
				else
					iNumOfTimeSlots=random()%(i2Powers[iCollisionCount-1]);

				//Write Collision log
				WriteLog(COLLISION, 0, 0, 0, iNumOfTimeSlots);
			}
			else {
				iFramePart=2;
			}

			//Wait after collision
			if(strcmp(iRespCode,"COLL")==0) {
				usleep(iRandomTimePeriod*iNumOfTimeSlots);
			}

			//Check to avoid repetitive sending of part 2 of a frame till receiving 'success' acknowledge
			if(iFramePart==1||isFrameSent==false){
				sprintf(cSendBuffer, "%d %d %d %d",iFramePart,iFrame,iStatID, iDestStatID);
				write(iSockFD,cSendBuffer,strlen(cSendBuffer));
				WriteLog(SEND, iFramePart, iFrame, iDestStatID, 0);
			}
			if(iFramePart==2 && isFrameSent==false)
				isFrameSent=true;

			read(iSockFD,cReceiveBuffer,BUFF_SIZE);
			sscanf(cReceiveBuffer,"%s", &iRespCode);
		}
	}
	fclose(fp);
}

//Write log to output.txt
//input:
/*
 * int iLogType: currently support send and collision
 * int iFramePart: the ID of the part of frame that is sent
 * int iFrame: Frame ID
 * int iStationID: ID of the destination station
 * int iTimeSlot: number of time slot this client will wait after collision
 *
 */
void WriteLog(int iLogType, int iFramePart, int iFrame, int iStationID, int iTimeSlot)
{
	FILE *fp;
	char cFileName[20];

	sprintf(cFileName,"output.txt");
	fp = fopen(cFileName,"a");

	switch(iLogType)
	{
		case SEND:		//Send  log
			fprintf(fp,"\nSend part %d of frame %d to Station %d", iFramePart, iFrame, iStationID);
			break;
		case COLLISION:	//Collision log
			fprintf(fp,"\nA collision informed, wait for %d time slot",iTimeSlot);
			break;
	}
	fclose(fp);
}
