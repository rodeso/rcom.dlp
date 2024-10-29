// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"


#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>


void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
	printf("Starting application layer\n");
    int frames = 0;
	LinkLayerRole rolex;
    //Role
    switch (role[0])
    {
		case 't':
			rolex = LlTx;
			break;
		case 'r':
			rolex = LlRx;
			break;
		default:
			printf("Invalid Role\n");
			return;
	}
	
	//LinkLayer
    LinkLayer connectionParameters = {
		.role = rolex,
		.baudRate = baudRate,
		.nRetransmissions = nTries,
		.timeout = timeout
	};
	strcpy(connectionParameters.serialPort,serialPort);
    
    //llopen
	int error1 = llopen(connectionParameters);
	printf("Error 1: %d\n",error1);
	if (error1 == -1)
	{
		printf("Error in llopen\n");
		return;
	}
	
    switch (rolex)
    {
    case LlTx:
		FILE* file = fopen(filename, "rb");
		if (file == NULL)
		{
			printf("Error opening file\n");
			return;
		}
		printf("File opened\n");

		//prepare Control Packet 1
		fseek(file, 0, SEEK_END);
		long int fileSize = ftell(file);
		fseek(file, 0, SEEK_SET);

		printf("File size: %ld\n", fileSize);

		//create Control Packet 1
		unsigned int cpSize;
		const int L1 = sizeof(fileSize);
		const int L2 = strlen(filename);
		cpSize = 1+2+L1+2+L2;
		unsigned char *controlPacketStart = (unsigned char*)malloc(cpSize);
		
		unsigned int pos = 0;
		controlPacketStart[pos++]=1; //start
		controlPacketStart[pos++]=0; //type (file size)
		controlPacketStart[pos++]=L1; //length (file size)

		printf("File size: %ld\n", fileSize);

		for (int i = 0; i < L1; i++) 
		{
			controlPacketStart[pos + i] = (fileSize >> (8 * (L1 - 1 - i))) & 0xFF; // MSB first
		}
		pos += L1; // Move the position forward

		controlPacketStart[pos++]=1; //type (file name)
		controlPacketStart[pos++]=L2; //length (file name)
		memcpy(controlPacketStart+pos, filename, L2); //file name

		//write Control Packet 1
		if (llwrite(controlPacketStart, cpSize)<0)
		{
			printf("Error writing Control Packet 1\n");
			return;
		}

		frames++;
		//prepare Data Packets
		unsigned char sequence = 0;
        long int bytesLeft = fileSize;

		unsigned char* content = (unsigned char*)malloc(sizeof(unsigned char) * fileSize);
    	fread(content, sizeof(unsigned char), fileSize, file);

		while (bytesLeft > 0)
		{
			//create Data Packet
			int dataSize = bytesLeft > (long int) MAX_PAYLOAD_SIZE ? MAX_PAYLOAD_SIZE : bytesLeft;
            unsigned char* data = (unsigned char*) malloc(dataSize);
            memcpy(data, content, dataSize);
            int packetSize;

			packetSize = 1 + 1 + 2 + dataSize;
			unsigned char* packet = (unsigned char*)malloc(packetSize);

			packet[0] = 2; //control field
			packet[1] = sequence; //sequence number
			//256 * L2 + L1
			packet[2] = (dataSize >> 8) & 0xFF; //L2
			packet[3] = dataSize & 0xFF; 		//L1
			memcpy(packet+4, data, dataSize);

            
            if(llwrite(packet, packetSize) == -1) 
			{
                printf("Exit: error in data packets\n");
                exit(-1);
            }                
            bytesLeft -= (long int) MAX_PAYLOAD_SIZE; 
            content += dataSize; 
            sequence = (sequence + 1) % 255;
			frames++;
		}


		//create Control Packet 3
		unsigned char *controlPacketEnd = (unsigned char*)malloc(cpSize);
		
		unsigned int sop = 0;
		controlPacketEnd[sop++]=3; //end
		controlPacketEnd[sop++]=0; //type (file size)
		controlPacketEnd[sop++]=L1; //length (file size)

		for (int i = 0; i < L1; i++) 
		{
			controlPacketEnd[sop + i] = (fileSize >> (8 * (L1 - 1 - i))) & 0xFF; // MSB first
		}
		sop += L1; // Move the position forward

		controlPacketEnd[sop++]=1; //type (file name)
		controlPacketEnd[sop++]=L2; //length (file name)
		memcpy(controlPacketEnd+sop, filename, L2); //file name

		//write Control Packet 3
		if (llwrite(controlPacketEnd, cpSize)<0)
		{
			printf("Error writing Control Packet 1\n");
			return;
		}

		frames++;
        llclose(frames);

		break;
	case LlRx:
		//create packet
		unsigned char *packet = (unsigned char *)malloc(MAX_PAYLOAD_SIZE);
        int packetSize = -1;
        while ((packetSize = llread(packet)) < 0) 
		{
			printf("Reading\n");
		}

		//parse Control Packet 1

		long rxFileSize = 0;
		unsigned char *name;

		// File Size
		unsigned char fileSizeNBytes = packet[2]; // Length of the file size (L1)

		// Dynamically allocate memory for the file size
		unsigned char* fileSizeAux = (unsigned char*)malloc(fileSizeNBytes);

		memcpy(fileSizeAux, packet + 3, fileSizeNBytes); // Copy the file size from the packet

		for (unsigned int i = 0; i < fileSizeNBytes; i++) {
			rxFileSize |= (fileSizeAux[fileSizeNBytes - i - 1] << (8 * i)); // Reconstruct the file size
		}

		//free(fileSizeAux); // Free the dynamically allocated memory after use


		// File Name
		unsigned char fileNameNBytes = packet[3 + fileSizeNBytes + 1]; // L2
		name = (unsigned char *)malloc(fileNameNBytes);

		memcpy(name, packet + 3 + fileSizeNBytes + 2, fileNameNBytes); // Copy file name
		name[fileNameNBytes] = '\0'; // Null-terminate the string if needed


        FILE* newFile = fopen((char *) name, "wb+");
        while (1) {    
    	    while ((packetSize = llread(packet)) < 0);
            if(packetSize == 0) break;
            else if(packet[0] != 2){
                unsigned char *buffer = (unsigned char*)malloc(packetSize);

				//parse Data Packet
                memcpy(buffer,packet+4,packetSize-4);
   				buffer += packetSize+4;
				
                fwrite(buffer, sizeof(unsigned char), packetSize-4, newFile);
                //free(buffer); 3
            }
            else continue;
        }

        fclose(newFile);
		break;
	default:
		printf("Invalid role\n");
		break;
	}
	if (llclose(frames)<0)
	{
		printf("Error Closing\n");
		return;
	}
}
