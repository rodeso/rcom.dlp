// Link layer protocol implementation

#include "link_layer.h"
#include "serial_port.h"

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source
#define FLAG 0x7E
#define ESC 0x7D
#define A_Tx 0x03
#define A_Rx 0x01
#define C_SET 0x03
#define C_UA 0x07
#define C_DISC 0x0B

#define C_RR(Nr) (0xAA | (Nr & 1))
#define C_REJ(Nr) (0x54 | (Nr & 1))
#define C_N(Ns) (Ns << 7) 
int Ns = 0;
int Nr = 1;

//GLOBAL VARIABLES
int alarmCount = 0;
int alarmEnabled = FALSE;
int timeout = 0;
int retransmitions = 0;




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
                        if(byte == FLAG) 
                        {
                            state = FLAG_SM;
                        }
                        break;
                    case FLAG_SM:
                        if(byte == A_Rx)
                        {
                            state = A_SM;
                        } else if (byte != FLAG)
                        {
                            state = START;
                        }
                        break;
                    case A_SM:
                        if(byte == C_UA) 
                        {
                            state = C_SM;
                        } else if (byte == FLAG) 
                        {
                            state = FLAG_SM;
                        } else {
                            state = START;
                        }
                        break;
                    case C_SM:
                        if(byte == A_Rx ^ C_UA) 
                        {
                            state = BCC_SM;
                        } else if (byte == FLAG) 
                        {
                            state = FLAG_SM;
                        } else
                        {
                            state = START;
                        }
                        break;
                    case BCC_SM:
                        if(byte == FLAG) 
                        {
                            state = STOP;
                            alarmEnabled = FALSE;
                            alarm(0);
                        } else 
                        {
                            state = START;
                        }
                        break;
                    default:
                        break;
                    }
                }
                retransmitions++;
            }
            if (state != STOP) 
            {
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
                    if(byte == FLAG) 
                    {
                        state = FLAG_SM;
                    }
                    break;
                case FLAG_SM:
                    if(byte == A_Tx) 
                    {
                        state = A_SM;
                    } else if (byte != FLAG) 
                    {
                        state = START;
                    }
                    break;
                case A_SM:
                    if(byte == C_SET) 
                    {
                        state = C_SM;
                    } else if (byte == FLAG)
                    {
                        state = FLAG_SM;
                    } else 
                    {
                        state = START;
                    }
                    break;
                case C_SM:
                    if(byte == A_Tx ^ C_SET) 
                    {
                        state = BCC_SM;
                    } else if (byte == FLAG) 
                    {
                        state = FLAG_SM;
                    } else 
                    {
                        state = START;
                    }
                    break;
                case BCC_SM:
                    if(byte == FLAG) 
                    {
                        state = STOP;
                    } else 
                    {
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
    int frames = 6+bufSize;
    unsigned char *frame = (unsigned char *) malloc(6+frames);
    frame[0] = FLAG;
    frame[1] = A_Tx;
    frame[2] = C_N(Ns);
    frame[3] = frame[1] ^frame[2];

    int j = 4;
    for (unsigned int i = 0 ; i < bufSize ; i++) 
    {
        if(buf[i] == FLAG) 
        {
            frame = realloc(frame,++frames);
            frame[j++] = ESC;
            frame[j++] = 0x5E;
        }
        if (buf[i] == ESC) 
        {
            frame = realloc(frame,++frames);
            frame[j++] = ESC;
            frame[j++] = 0x5D;
        }
        else 
        {
            frame[j++] = buf[i];
        }
    }

    unsigned char BCC = buf[0];
    for (unsigned int i = 1 ; i < bufSize ; i++) BCC ^= buf[i];
    frame[j++] = BCC;
    frame[j++] = FLAG;

    retransmitions = 0;
    int rej = 0;
    int ack = 0;

    while (retransmitions < connectionParameters.nRetransmissions) 
    {
        alarmEnabled = FALSE;
        alarm(connectionParameters.timeout);
        alarmEnabled = TRUE;
        while (alarmEnabled == TRUE && !rej && !ack) 
        {
            writeBytesSerialPort(frame, frames);
            int answer = waitAnswer();

            if(!answer)
            {
                continue;
            }
            if (answer == C_RR(0) || answer == C_RR(1)) 
            {
                ack = 1;
                Ns = !Ns;
            }
            else if (answer == C_REJ(0) || answer == C_REJ(1)) 
            {
                rej = 1;
            }
            else if (answer == C_DISC) 
            {
                return -1;
            }
            else 
                continue;
        }
        if (ack) 
        {
            break;
        }
        else if (rej)
        {
            retransmitions++;
            rej = 0;
        }
    }
    free(frame);
    if(ack) return frames;
    return -1;
}

////////////////////////////////////////////////
// waitAnswer
////////////////////////////////////////////////

unsigned char waitAnswer(){

    unsigned char byte, ret = 0;
    LinkLayerSM state = START;
    
    while (state != STOP && alarmEnabled == TRUE) 
    {  
        readByteSerialPort(byte);
        switch (state) 
        {
            case START:
                if (byte == FLAG) state = FLAG_SM;
                break;
            case FLAG_SM:
                if (byte == A_Rx) state = A_SM;
                else if (byte != FLAG) state = START;
                break;
            case A_SM:
                if (byte == C_RR(0) || byte == C_RR(1) || byte == C_REJ(0) || byte == C_REJ(1) || byte == C_DISC)
                {
                    state = C_SM;
                    ret = byte;   
                }
                else if (byte == FLAG) state = FLAG_SM;
                else state = START;
                break;
            case C_SM:
                if (byte == (A_Rx) ^ ret) state = BCC_SM;
                else if (byte == FLAG) state = FLAG_SM;
                else state = START;
                break;
            case BCC_SM:
                if (byte == FLAG)
                {
                    state = STOP;
                }
                else state = START;
                break;
            default: 
                break;
        }
    } 
    return ret;
}


////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{
    LinkLayerSM state = START;
    unsigned char byte, ret = 0;
    int i = 0;
    int frames = 0;
    while (state != STOP) 
    {
        readByteSerialPort(byte);
        switch (state) 
        {
            case START:
                if (byte == FLAG) state = FLAG_SM;
                break;
            case FLAG_SM:
                if (byte == A_Tx) state = A_SM;
                else if (byte != FLAG) state = START;
                break;
            case A_SM:
                if (byte == C_N(0) || byte == C_N(1))
                {
                    state = C_SM;
                    ret = byte;   
                }
                else if (byte == FLAG) state = FLAG_SM;
                else if (byte == C_DISC) 
                {
                    unsigned char message[5] = {FLAG, A_Rx, C_DISC, A_Rx ^ C_DISC, FLAG};
                    writeBytesSerialPort(message, 5);
                    return 0;
                }
                else state = START;
                break;
            case C_SM:
                if (byte == (A_Tx ^ ret)) state = READ_SM;
                else if (byte == FLAG) state = FLAG_SM;
                else state = START;
                break;
            case READ_SM:
                if (byte == ESC) state = ESC_SM; //vai desfazer o byte stuffing
                else if (byte == FLAG) //termina o pacote
                { 
                    unsigned char bcc2 = packet[i-1];
                    i--;
                    packet[i] = '\0';
                    unsigned char acc = packet[0];
                    for (unsigned int j = 1; j < i; j++)
                            acc ^= packet[j];
                    if (bcc2 == acc)
                    {
                        state = STOP;
                        unsigned char message[5] = {FLAG, A_Rx, C_RR(Nr), A_Rx ^ C_RR(Nr), FLAG};
                        writeBytesSerialPort(message, 5);
                        Nr = !Nr;
                        return i; 
                    }
                    else
                    {
                        printf("Error: need retransmition\n");
                        unsigned char message[5] = {FLAG, A_Rx, C_REJ(Nr), A_Rx ^ C_REJ(Nr), FLAG};
                        writeBytesSerialPort(message, 5);
                        return -1;
                    };

                }
                else
                {
                    packet[i++] = byte; //escreve o byte no pacote
                }
                break;
            case ESC_SM:
                state = READ_SM;
                packet[i++] = byte ^ 0x20;
                break;
            default: 
                break;
        }

    }

    return i;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics)
{
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
                unsigned char message[5] = {FLAG, A_Tx, C_DISC, A_Tx ^ C_DISC, FLAG}; 
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
            unsigned char message[5] = {FLAG, A_Rx, C_DISC, A_Rx ^ C_DISC, FLAG};
            writeBytesSerialPort(message, 5);
            break;
        }
        default:
            printf("error in role");
            return -1;
    }

    
	printf("The size of frames is %d\n", showStatistics);
    int clstat = closeSerialPort();
    return clstat;
}
