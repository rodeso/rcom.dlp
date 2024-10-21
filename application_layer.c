// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"


#include <stdio.h>

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
    // TODO
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
		.serialPort = serialPort,
		.role = rolex,
		.baudRate = baudRate,
		.nRetransmissions = nTries,
		.timeout = timeout
	};
    
    //llopen
	if (llopen(connectionParameters)<0);
	{
		printf("Error in llopen\n");
		return;
	}
	
    switch (rolex)
    {
    case LlTx:
		//code
		break;
	case LlRx:
		//code
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
