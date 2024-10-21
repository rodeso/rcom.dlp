// Link layer protocol implementation

#include "link_layer.h"
#include "serial_port.h"

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source
#define FLAG 0x7E
#define A_Tx 0x03
#define A_Rx 0x01
#define C_SET 0x03
#define C_UA 0x07
#define C_RR0 0xAA
#define C_RR1 0xAB
#define C_REJ0 0x54
#define C_REJ1 0x55
#define C_DISC 0x0B
#define C_INF0 0x00
#define C_INF1 0x80


////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters)
{
    if (openSerialPort(connectionParameters.serialPort,
                       connectionParameters.baudRate) < 0)
    {
        return -1;
    }
    // TODO
	
    return 1;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize)
{
    // TODO

    return 0;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{
    // TODO

    return 0;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics)
{
    // TODO
    switch (connectionParameters.role) 
    {
    case LlTx:
        unsigned char message[5] = {FLAG, A_Tx, C_DISC, A_Tx ^ C_SET, FLAG}; 
        writeBytesSerialPort(message, 5);
        while() {
            while() {
                if (readByteSerialPort)
                switch()
                case START
            }
        }
        break;
    case LlRx:
        while()
        readByteSerialPort()
        unsigned char message[5] = {FLAG, A_Rx, C_DISC, A_Rx ^ C_SET, FLAG};
        writeBytesSerialPort(message, 5);
        break;
    default:
        printf("error in role");
        break;
    }
    
	printf("The Number of frames is %d\n", showStatistics);
    int clstat = closeSerialPort();
    return clstat;
}
