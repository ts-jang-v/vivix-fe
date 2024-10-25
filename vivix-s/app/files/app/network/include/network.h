#ifndef NETWORK_H_
#define NETWORK_H_

#include <stdio.h>              // fprintf()
#include <stdlib.h>             // malloc(), free(),exit(), EXIT_SUCCESS rand/rand 
#include <stdint.h>
#include <string.h>             // memset(), strcpy()
#include <unistd.h>             // getpid(), open(),close()
#include <stdbool.h>            // bool, true, false
#include <time.h>               // time_t, time()
#include <errno.h>              // errno
#include <fcntl.h>              // fcntl(),O_RDWR ,IOCTL
#include <ctype.h>              // isprint()
#include <sys/ioctl.h>          // ioctl 
#include <netinet/in.h>         // struct sockaddr_in, htons(), ntohs()
#include <net/if.h>             // struct ifconf, struct ifreq
#include <net/ethernet.h>       // ETHERMTU
#include <linux/sockios.h> 
#include <linux/ethtool.h>      // ETHTOOL_GLINK

#include "typedef.h"
#include "mutex.h"

#define MAX_NIC_NUM				3

class CNetwork
{
private:

protected :
    int     network_usleep(s64 a_nusec);
    void    network_print_packet(u8* a_ppacket, int a_nlen);
    
public:

    CNetwork(void);
    virtual ~CNetwork(void);
};

#endif /* end of NETWORK_H_*/

