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


//GLOBAL VARIABLES
int alarmCount = 0;
int alarmEnabled = FALSE;
int timeout = 0;
int retransmitions = 3;


////////////////////////////////////////////////
// alarmHandler
////////////////////////////////////////////////
void alarmHandler(int signal)
{
    alarmEnabled = FALSE;  //this means that the alarm has been triggered
    alarmCount++;

    printf("Alarm #%d\n", alarmCount);
    
    if (alarmCount > retransmitions) 
    {
		return;
	}
}

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
    LinkLayerSM state = START;

    unsigned char byte;
    timeout = connectionParameters.timeout;
    

    switch (connectionParameters.role) 
    {
        case LlTx:
        {
            signal(alarmHandler);
            while (retransmitions < connectionParameters.nRetransmissions && state != STOP)
            {
                unsigned char message[5] = {FLAG, A_Tx, C_SET, A_Tx ^ C_SET, FLAG}; 
                writeBytesSerialPort(message, 5);
                alarm(connectionParameters.timeout);
                alarmEnabled = TRUE;

                while(alarmEnabled == TRUE && state != STOP)
                {
                    readByteSerialPort(byte);
                    switch (state)
                    {
                    case START:
                        if(byte == FLAG) {
                            state = FLAG_SM;
                        }
                        break;
                    case FLAG_SM:
                        if(byte == A_Rx) {
                            state = A_SM;
                        } else if (byte != FLAG) {
                            state = START;
                        }
                        break;
                    case A_SM:
                        if(byte == C_UA) {
                            state = C_SM;
                        } else if (byte == FLAG) {
                            state = FLAG_SM;
                        } else {
                            state = START;
                        }
                        break;
                    case C_SM:
                        if(byte == A_Rx ^ C_UA) {
                            state = BCC_SM;
                        } else if (byte == FLAG) {
                            state = FLAG_SM;
                        } else {
                            state = START;
                        }
                        break;
                    case BCC_SM:
                        if(byte == FLAG) {
                            state = STOP;
                            alarmEnabled = FALSE;
                            alarm(0);
                        } else {
                            state = START;
                        }
                        break;
                    default:
                        break;
                    }
                }
                retransmitions++;
            }
            if (state != STOP) {
                return -1;
            }
            break;
        }
        case LlRx:
        {
            while (state != STOP)
            {
                readByteSerialPort(byte);
                switch (state)
                {
                case START:
                    if(byte == FLAG) {
                        state = FLAG_SM;
                    }
                    break;
                case FLAG_SM:
                    if(byte == A_Tx) {
                        state = A_SM;
                    } else if (byte != FLAG) {
                        state = START;
                    }
                    break;
                case A_SM:
                    if(byte == C_SET) {
                        state = C_SM;
                    } else if (byte == FLAG) {
                        state = FLAG_SM;
                    } else {
                        state = START;
                    }
                    break;
                case C_SM:
                    if(byte == A_Tx ^ C_SET) {
                        state = BCC_SM;
                    } else if (byte == FLAG) {
                        state = FLAG_SM;
                    } else {
                        state = START;
                    }
                    break;
                case BCC_SM:
                    if(byte == FLAG) {
                        state = STOP;
                    } else {
                        state = START;
                    }
                    break;
                default:
                    break;
                }
            }
            unsigned char message[5] = {FLAG, A_Rx, C_UA, A_Rx ^ C_UA, FLAG};
            writeBytesSerialPort(message, 5);
            break;
        }
        default:
            printf("error in role");
            return -1;
    }
	
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
        LinkLayerSM state = START;
        int a = 0;
        int c = 0;

        while(state != STOP && alarmCount < 3) 
        {

            if (alarmEnabled == FALSE)
            {
                printf("alarme de 3 segundos \n");
                alarm(3); // Set alarm to be triggered in 3s
                alarmEnabled = TRUE;
            }

            unsigned char byte = 0;
            readByteSerialPort(byte);

            switch (state)
            {
            case START:
                if(byte == FLAG) {
                    state = FLAG_SM;
                }
                break;
            case FLAG_SM:
                if(byte == FLAG) {
                    buf2[0] = byte;
                } else if (byte == ADDRESS_RECIVE) {
                    buf2[1] = byte;
                    a_prov = byte;
                    state = A_STATE;
                } else {
                    state = START_STATE;
                }
                break;
            case A_STATE:
                if(byte == FLAG) {
                    buf2[0] = byte;
                    state = FLAG_STATE;
                } else if (byte == CONTROLL_RECIVE) {
                    buf2[2] = byte;
                    c_prov = byte;
                    state = C_STATE;
                } else {
                    state = START_STATE;
                }
                break;
            case C_STATE:
                if(byte == FLAG) {
                    buf2[0] = byte;
                    state = FLAG_STATE;
                } else if (byte == a_prov ^ c_prov) {
                    buf2[3] = byte;
                    state = BCC_STATE;
                } else {
                    state = START_STATE;
                }
                break;
            case BCC_STATE:
                if(byte == FLAG) {
                    buf2[4] = byte;
                    state = STOP_STATE;
                    alarm(0);
                } else {
                    state = START_STATE;
                }
                break;
            default:
                break;
            }
        }
        
        unsigned char message[5] = {FLAG, A_Rx, C_DISC, A_Rx ^ C_SET, FLAG};
        writeBytesSerialPort(message, 5);
        break;
    default:
        printf("error in role");
        return -1;
    }
    
	printf("The Number of frames is %d\n", showStatistics);
    int clstat = closeSerialPort();
    return clstat;
}
