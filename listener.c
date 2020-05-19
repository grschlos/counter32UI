#include <pthread.h>
#include <unistd.h>
//#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <syscall.h> //??
#include "counter.h"

#define PORT_FPGA       1200
#define PORT_USR_IN     1300
#define PORT_USR_OUT    1400
#define BUFLEN          600

#define FPGA         "192.168.1.3"

struct listener_args { /* Container for listen()'s args */ 
    bool isPC;
    // source 
    int ssockfd;
    struct sockaddr_in *ssa;
    // destination
    int dsockfd; 
    struct sockaddr_in *dsa;
};

//unsigned char msg[BUFLEN] = "\\dac1=101&dac2=102&time=1&step=0&nSteps=0&calb&";

void *_listen( void *args )
{
    //pthread_t self;
    struct listener_args *la = args;
    int n, ssockfd, dsockfd;
    unsigned int len;
    struct sockaddr_in ssa, dsa;
    unsigned char  in[BUFLEN] = {0};
    unsigned char out[BUFLEN] = {0};
    bool isPC = la->isPC;
    ssockfd = la->ssockfd;
    dsockfd = la->dsockfd;
    memcpy( &ssa, la->ssa, sizeof(struct listener_args) );
    memcpy( &dsa, la->dsa, sizeof(struct listener_args) );
    while(1)
    {
        memset(in,  0, BUFLEN);
        memset(out, 0, BUFLEN);
        len = sizeof(struct sockaddr_in);
        n = recvfrom(ssockfd, in, BUFLEN, 0, 
            (struct sockaddr*)&ssa, &len);
        if (isPC){                              // Translate text command
            translateTextCmd(in, out);
            n = 50;
        }
        else {                                  // Translate binary command
            translateBinData(in, (char*)out, BUFLEN);
            n = strlen((char*)out);
        }
        sendto(dsockfd, out, n, 0, 
            (struct sockaddr*)&dsa, len);
    }
    return NULL;
}

int main()
{
    pthread_t th[2];
    int sockfd_usr, sockfd_fpga;
    struct sockaddr_in fpga_in, fpga_out, usr_in, usr_out;
    void *res;

    // Creating socket file descriptor
    if ( (sockfd_usr = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
    if ( (sockfd_fpga = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&fpga_in,    0, sizeof(fpga_in) );
    memset(&fpga_out,   0, sizeof(fpga_out));
    memset(&usr_in,     0, sizeof(usr_in)  );
    memset(&usr_out,    0, sizeof(usr_out) );

    // USR
    usr_in.sin_family       = AF_INET; // IPv4
    usr_in.sin_addr.s_addr  = INADDR_ANY;
    usr_in.sin_port         = htons(PORT_USR_IN);
    usr_out.sin_family      = AF_INET; // IPv4
    usr_out.sin_addr.s_addr = INADDR_ANY;
    usr_out.sin_port        = htons(PORT_USR_OUT);
    // FPGA
    fpga_in.sin_family      = AF_INET;
    fpga_in.sin_addr.s_addr = INADDR_ANY;
    fpga_in.sin_port        = htons(PORT_FPGA);
    fpga_out.sin_family     = AF_INET;
    inet_aton(FPGA, &(fpga_out.sin_addr));
    fpga_out.sin_port       = htons(PORT_FPGA);

    // Bind the socket with the server address
    if ( bind(sockfd_usr, (const struct sockaddr *)&usr_in,
            sizeof(usr_in)) < 0 )
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if ( bind(sockfd_fpga, (const struct sockaddr *)&fpga_in,
            sizeof(fpga_in)) < 0 )
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    struct listener_args sinfo[2];
    sinfo[0].isPC       = 1;
    sinfo[0].ssockfd    = sockfd_usr;
    sinfo[0].dsockfd    = sockfd_fpga;
    sinfo[0].ssa        = &usr_in;
    sinfo[0].dsa        = &fpga_out;
    sinfo[1].isPC       = 0;
    sinfo[1].ssockfd    = sockfd_fpga;
    sinfo[1].dsockfd    = sockfd_usr;
    sinfo[1].ssa        = &fpga_in;
    sinfo[1].dsa        = &usr_out;
    pthread_create(th,      NULL, &_listen, &sinfo[0]);
    pthread_create(th+1,    NULL, &_listen, &sinfo[1]);
    pthread_join( *th,      &res);
    pthread_join(*(th+1),   &res);

    return 0;
}
