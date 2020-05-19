#include "counter.h"

#define CODE_IDX 2
#define STR_LEN 32
#define N_CHN   32

const char *vars[6] = { "dac", "chs", "time", "step", "nSteps", "calb" };
const char *cmds[9] = { "hi", "help", "interrupt",
                        "addr", "raddr", "data", "rdata",
                        "reset", "dac" };

//______________________________________________________________________________
/**
 * Write start mark and the command code into the buffer.
 */
void writeCmd( unsigned char code, unsigned char *cmd )
{
    unsigned char *dataPtr;
    dataPtr = cmd;

    // StartMark
    *dataPtr++ = 0xde;
    *dataPtr++ = 0xaf;

    // Command code
    *dataPtr++ = code;
}

//______________________________________________________________________________
/** 
 * --- Function description ---
 */
void makeInitCmd( unsigned char *cmd )
{
    writeCmd( CMD_INIT, cmd );
}

//______________________________________________________________________________
/** 
 * --- Function description ---
 */
void makeHelpCmd( unsigned char *cmd )
{
    writeCmd( CMD_HELP, cmd );
}

//______________________________________________________________________________
/** 
 * --- Function description ---
 */
void makeInterruptCmd( unsigned char *cmd )
{
    writeCmd( CMD_INTR, cmd );
}

//______________________________________________________________________________
/** 
 * --- Function description ---
 */
void makeWriteAddressCmd( unsigned char addr, unsigned char *cmd )
{
    writeCmd( CMD_ADDR, cmd );
    *(cmd+3) = addr;
}

//______________________________________________________________________________
/** 
 * --- Function description ---
 */
void makeReadAddressCmd( unsigned char *cmd )
{
    writeCmd( CMD_RADDR, cmd );
}

//______________________________________________________________________________
/** 
 * --- Function description ---
 */
void makeWriteDataCmd( unsigned char data, unsigned char *cmd )
{
    writeCmd( CMD_DATA, cmd );
    *(cmd+3) = data;
}

//______________________________________________________________________________
/** 
 * --- Function description ---
 */
void makeReadDataCmd( unsigned char *cmd )
{
    writeCmd( CMD_RDATA, cmd );
}

//______________________________________________________________________________
/** 
 * --- Function description ---
 */
void makeResetCmd( unsigned char *cmd )
{
    writeCmd( CMD_RST, cmd );
}

//______________________________________________________________________________
/** 
 * Check if the channel is active
 */
bool isActiveChannel( unsigned char i, unsigned int channels )
{
    return (channels<<i & (0x01<<(N_CHN-1))) == 0x80000000;
}

//______________________________________________________________________________
/** 
 * --- Function description ---
 */
void writeDACValue( unsigned short dac, unsigned char **dataPtr )
{
    **dataPtr     = 0;
    *(*dataPtr)++ |= dac >> 8;
    **dataPtr     &= 0;
    *(*dataPtr)++ |= dac & 0xff;
}
    
//______________________________________________________________________________
/** 
 * --- Function description ---
 */
void makeRunCmd(unsigned short dac, unsigned int *channels, 
        unsigned char *cTimeChar, unsigned short *step,
        unsigned short *nSteps, unsigned char *calib, unsigned char *cmd)
{
    unsigned char *dataPtr;
    dataPtr = cmd+2;

    // DAC command
    writeCmd( CMD_DAC, cmd );
    dataPtr++;

    // Write channels mask
    writeInt( *channels, dataPtr );
    dataPtr+=4;

    // Initialize DAC
    writeDACValue( dac, &dataPtr );

    // Write counting time
    *dataPtr++ = *cTimeChar;
    
    // Write DAC step
    *dataPtr++ = *step>>8;
    *dataPtr++ = *step & 0xff;

    // Write number of steps
    *dataPtr++ = *nSteps>>8;
    *dataPtr++ = *nSteps & 0xff;

    // Write calibration tag
    *dataPtr = *calib<< 7;
}

//______________________________________________________________________________
/**
 *      Read one integer from the buffer.
 */
unsigned int readInt( unsigned char *dataPtr )
{
    unsigned char i;
    int res = 0;
    for (i=0; i<4; i++) { res |= *(dataPtr+i) << 8*(3-i); }
    return res;
}

//______________________________________________________________________________
/**
 *      Write one integer into the buffer.
 */
void writeInt( unsigned int value, unsigned char *buf )
{
    unsigned char i;
    unsigned char *dataPtr=buf;
    for (i=0; i<4; i++){
        *dataPtr++ = value >> ((3-i)*8);
    }
}

//______________________________________________________________________________
/**
 *      Parse channels, DAC value and calibration tag.
 */
void readCountPars(unsigned int *channels, unsigned short *dac, 
        unsigned char *calib, unsigned char **dataPtr)
{
    // Enabled channels
    *channels = readInt(*dataPtr);
    *dataPtr+=4;

    // Parse DAC value
    *dac = 0;
    *dac |= *(*dataPtr)++ << 8;
    *dac |= *(*dataPtr)++;

    // Parse calibration tag
    *calib = *(*dataPtr)++ >> 7;
}

//______________________________________________________________________________
/**
 *  Read DAC values, calibration tag and counter values (if necessary).
 */
void readCountResults(unsigned int *channels, unsigned int *count,
        unsigned char **dataPtr)
{
    unsigned char i;
    if ( *channels ){
        for (i=0; i<N_CHN; i++){
            if ( ! isActiveChannel(i, *channels) ) continue;
            *(count+i) = readInt(*dataPtr);
            *dataPtr+=4;
        }
    }
}

//______________________________________________________________________________
/**
 *  Process the message
 */
void processCountResponse(unsigned int *channels, unsigned short *dac, 
        unsigned char *calib, unsigned int *count, unsigned char *data, 
        char *res, size_t n)
{
    unsigned char i;
    int pos = 0;
    unsigned char val = *(data+3);
    unsigned char *dataPtr;
    dataPtr = data+4;
    *channels = 0;
    readCountPars(channels, dac, calib, &dataPtr);
    if (!(*calib)) readCountResults(channels, count, &dataPtr);
    if ( ! (*channels) ){
        if (val==MARK_BUSY){
            snprintf(res, n, "Please wait ~%u sec or interrupt run!\n",
                    readInt(data+4));
        }
        else {
            if (val==MARK_OK) snprintf(res, n, "Ok\n");
        }
    }
    else {
        if (*calib) pos += snprintf( res, n, "***DAC calibration*** \n" );
        for (i=0; i<N_CHN; i++){
            if ( ! isActiveChannel(i, *channels) ) continue;
            pos += snprintf(res+pos, n-pos, "CH%u", i+1);
            if (*calib){
                pos += snprintf(res+pos, n-pos, " ");
            }
            else {
                pos += snprintf(res+pos, n-pos, ": %8u ", *(count+i));
            }
        }
        pos+=snprintf(res+pos, n-pos, "\n");
    }
}

//______________________________________________________________________________
/**
 *
 */
/*int readBinData(unsigned char *cmd, unsigned char *cVal, unsigned int *iVal, 
        unsigned char *channels, unsigned short *dac, 
        unsigned int *count, unsigned char *calib, unsigned char *buf)*/
int translateBinData( unsigned char *buf, char *res, size_t n )
{
    unsigned char *cmd = (unsigned char*)malloc(1);
    *cmd = 0;
    unsigned char *cVal = (unsigned char*)malloc(1);
    *cVal = 0;
    unsigned int *iVal = (unsigned int*)malloc(4);
    *iVal = 0;
    unsigned int *channels = (unsigned int*)malloc(4);
    *channels = 0;
    unsigned short dac = 0;
    unsigned int count[N_CHN] = {0};
    unsigned char *calib = (unsigned char*)malloc(1);
    *calib = 0;
    if ( ! (*buf==(MARK_IN>>8) && *(buf+1)==(MARK_IN&0xff)) ){
        snprintf(res, n, "No input marker has been found.\n");
        return -1;
    }
    *cmd  = *(buf+2) & 0xf0;
    *cVal = *(buf+3);
    *iVal = readInt(buf+4);

    switch(*cmd){
        case CMD_INIT:
            if (*cVal==MARK_OK){
                snprintf(res, n, "Hi, I'm not high yet!\n");
            }
            break;
        case CMD_HELP:
            if (*cVal==MARK_OK){
                snprintf(res, n, "Information: \n"
                        "/dac=[0-4095code]&chs=[32bitmask]&time=[0-255sec]&"
                        "step=[0-4095code]&nSteps=[0-4095]&\n"
                        "/dac=[0-4095code]&chs=[32bitmask]&time=[0-255sec]&"
                        "step=[0-4095code]&nSteps=[0-4095]&calb&\n");
            }
            break;
        case CMD_INTR:
            snprintf(res, n, "nSteps = 0, please wait a few second.\n");
            break;
        case CMD_ADDR:
            snprintf(res, n, "setAddr=%u\n", *cVal);
            break;
        case CMD_RADDR:
            snprintf(res, n, "addr=%u\n", *cVal);
            break;
        case CMD_DATA:
            snprintf(res, n, "setData=%u\n", *cVal);
            break;
        case CMD_RDATA:
            snprintf(res, n, "data=%u\n", *cVal);
            break;
        case CMD_RST:
            if (*cVal==MARK_OK){
                snprintf(res, n, "I'm not high already!\n");
            }
            break;
        case CMD_DAC:
            processCountResponse(channels, &dac, calib, count, buf, res, n);
            break;
        default:
            snprintf(res, n,
                "No such command! help - for information about system.\n");
            break;
    }
    free(cmd);
    free(cVal);
    free(iVal);
    free(channels);
    free(calib);
    return 0;
}

//______________________________________________________________________________
/**
 * Parse the parameter value starting from the i-th position. Returns
 * the length of the value.
 */

unsigned char parseValue(unsigned char i, unsigned char *buf, 
        unsigned char *value)
{
    unsigned char j, tmp;
    for (j = 0; j < 32; j++){
        tmp = *(buf+i+j);
        if (tmp != 0x26) { // 0x26 is an "&" ASCII code
            *(value+j) = tmp;
        }
        else{
            break;
        }
    }
    return j;
}

//______________________________________________________________________________
/**
 *      Parse command.
 */
bool parseName(unsigned short i, unsigned char *buf, 
        const char *tmp, unsigned char *value, bool isRun)
{
    if (!value) return 0;
    memset(value, 0, STR_LEN);
    unsigned char t, k;
    unsigned short j;
    unsigned short len = strlen(tmp);
    short n = strlen((char*)buf)-i;
    for (j = 0; j < n; j++){
        t = *(buf+i+j);
        if ( t < 0x61 ) continue;
        for (k=0; k<len; k++) {
            if (*(buf+i+j+k)==*(tmp+k)) continue;
            else break;
        }
        if (k==len){
            if (*(buf+i+j+k)==0x3d){
                parseValue(i+j+len+1, buf, value);
            }
            return true;
        }
        if (!isRun) return false;
    }
    //if (*(buf+j+len) == 0x26) return true;
    return false;
}

//______________________________________________________________________________
/**
 *      Parse query.
 */
void parseRun(unsigned int i, unsigned char *buf, 
        unsigned short *dac, unsigned int *channels, unsigned char *time, 
        unsigned short *step, unsigned short *nSteps, unsigned char *calib)
{
    unsigned char value[STR_LEN] = {0};

    // Parse initial DAC values
    if ( parseName(i, buf, vars[DAC_IDX], value, 1) ) {
        *dac = atoi((char*)value);
    }

    // Parse channels mask
    parseName(i, buf, vars[CHS_IDX], value, 1);
    *channels = strtol((char*)value, NULL, 16);

    // Parse count time
    parseName(i, buf, vars[TIME_IDX], value, 1);
    *time = atoi((char*)value);

    // Parse DAC step
    parseName(i, buf, vars[STEP_IDX], value, 1);
    *step = atoi((char*)value);

    // Parse number of steps
    parseName(i, buf, vars[NSTP_IDX], value, 1);
    *nSteps = atoi((char*)value);

    // Parse calibration tag
    *calib = parseName(i, buf, vars[CALB_IDX], value, 1);
}

//______________________________________________________________________________
/**
 *      Translate text command into binary command
 */
void translateTextCmd(unsigned char *buf, unsigned char *out)
{
    unsigned char str[32] = {0};
    if ( parseName(0, buf, *cmds, str, 0) ){
        makeInitCmd(out);
    }
    else if ( parseName(0, buf, cmds[1], str, 0) ){
        makeHelpCmd(out);
    }
    else if (parseName(0, buf, cmds[2], str, 0) ){
        makeInterruptCmd(out);
    }
    else if (parseName(0, buf, cmds[3], str, 0) ){
        makeWriteAddressCmd( atoi((char*)str), out );
    }
    else if (parseName(0, buf, cmds[4], str, 0) ){
        makeReadAddressCmd( out );
    }
    else if (parseName(0, buf, cmds[5], str, 0) ){
        makeWriteDataCmd( atoi((char*)str), out );
    }
    else if (parseName(0, buf, cmds[6], str, 0) ){
        makeReadDataCmd( out );
    }
    else if (parseName(0, buf, cmds[7], str, 0) ){
        makeResetCmd( out );
    }
    else if (parseName(0, buf, cmds[8], str, 1) ){
        unsigned short dac = 0;
        unsigned int channels = 0;
        unsigned char time = 0;
        unsigned short step = 0;
        unsigned short nSteps = 0;
        unsigned char calib = 0;
        //unsigned char i;
        parseRun( 0, buf, &dac, &channels, &time, &step, 
                &nSteps, &calib );
        makeRunCmd( dac, &channels, &time, &step, 
                &nSteps, &calib, out );
        //for (i=0; i<14; i++) printf("%02x ", out[i]);
        //printf("\n");
        //printf("dac=%u channels=%u time=%u step=%u nSteps=%u\n", dac, channels, time, step, nSteps);
    }
    else {
        ;
    }
}

//______________________________________________________________________________
/**
 *
 */
/*
void functionPrototype()
{
    ;
}
*/
