#ifndef COUNTER_H
#define COUNTER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define CMD_INIT            0x10        /**< INIT command code.         */
#define CMD_HELP            0x20        /**< HELP command code.         */
#define CMD_INTR            0x30        /**< INTERRUPT command code.    */
#define CMD_ADDR            0x40        /**< ADDR command code.         */
#define CMD_RADDR           0x50        /**< RADDR command code.        */
#define CMD_DATA            0x60        /**< DATA command code.         */
#define CMD_RDATA           0x70        /**< RDATA command code.        */
#define CMD_RST             0xC0        /**< RESET command code.        */
#define CMD_DAC             0xD0        /**< DAC command code.          */

#define MARK_OUT          0xDEAF        /**< Output data start marker   */
#define MARK_IN           0xDEAD        /**< Input data start marker    */
#define MARK_OK             0x08        /**< Success mark               */
#define MARK_ERR            0x01        /**< Error mark                 */
#define MARK_BUSY           0x02        /**< Counter busy mark          */

#define DAC_IDX             0x00
#define CHS_IDX		    0x01
#define TIME_IDX            0x02
#define STEP_IDX            0x03
#define NSTP_IDX            0x04
#define CALB_IDX            0x05

// Output buffer to FPGA
void writeCmd(unsigned char code, unsigned char *cmd);  // internal
void makeInitCmd(unsigned char *cmd);
void makeHelpCmd(unsigned char *cmd);
void makeInterruptCmd(unsigned char *cmd);
void makeWriteAddressCmd(unsigned char addr, unsigned char *cmd);
void makeReadAddressCmd(unsigned char *cmd);
void makeWriteDataCmd(unsigned char data, unsigned char *cmd);
void makeReadDataCmd(unsigned char *cmd);
void makeResetCmd(unsigned char *cmd);
bool isActiveChannel(unsigned char i, unsigned int channels);
void writeDACValues(unsigned int channels, unsigned short dac, 
        unsigned char **dataPtr);                                   // internal
void makeRunCmd(unsigned short dac, unsigned int *channels,
        unsigned char *time, unsigned short *step, 
        unsigned short *nSteps, unsigned char *calib, unsigned char *cmd);
// Input buffer from FPGA
unsigned int readInt(unsigned char *dataPtr);                      // internal
void readCountPars(unsigned int *channels, unsigned short *dac, 
        unsigned char *calib, unsigned char **dataPtr);             // internal
void readCountResults(unsigned int *channels, unsigned int *count,
        unsigned char **dataPtr);                                   // internal
void processCountResponse(unsigned int *channels, unsigned short *dac,
        unsigned char *calib, unsigned int *count, unsigned char *data,
        char *res, size_t n);     // internal
int translateBinData( unsigned char *buf, char *res, size_t n );
/*int readBinData(unsigned char *cmd, unsigned char *cVal, 
        unsigned int *iVal, unsigned char *channels, 
        unsigned short *dac, unsigned int *count, 
        unsigned char *calib, unsigned char *buf); */
// Input buffer from user
void writeInt(unsigned int value, unsigned char *buf);
unsigned char parseValue(unsigned char i, unsigned char *buf, 
        unsigned char *value);
bool parseName(unsigned short i, unsigned char *buf, 
        const char *tmp, unsigned char *value, bool isRun);
void parseRun(unsigned int i, unsigned char *buf,
        unsigned short *dac, unsigned int *channels, unsigned char *time,
        unsigned short *step, unsigned short *nSteps, unsigned char *calib);
void translateTextCmd(unsigned char *buf, unsigned char *out);

#endif /* COUNTER_H */
