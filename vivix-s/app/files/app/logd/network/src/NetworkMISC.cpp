/* 
@ NetworkMISC.cpp
@ Author - kim  yong jin
@ 2012. 04. 27
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <net/ethernet.h>
#include <linux/ethtool.h>
#include <linux/sockios.h>

#include "NetworkMISC.h"



int USleep(long a_nUsec)
{
	struct timespec req, rem;
	int rtn;

	req.tv_sec = a_nUsec / 1000000;
	req.tv_nsec = (a_nUsec % 1000000 ) * 1000;

	do
	{
		rtn = nanosleep(&req, &rem);

		if(rtn < 0 && errno == EINTR)
				req = rem;
	}
	while(rtn < 0 && errno == EINTR);

	return rtn;
}

void PrintPacket(void* a_pPacket, int a_nLen)
{
	int i, j, count;
	unsigned char * buf = ( unsigned char * ) a_pPacket;

	for( i = 0, count = 0; i < a_nLen; ++i )
	{
		fprintf(stderr,"%02x ",( int )( buf[ i ] ) & 0xFF);

		if( ++count == 16 )
		{
			fprintf( stderr, "\t" );

			for( j = i - 15; j <= i; ++j )
			{
				if( isprint(( int ) buf[ j ] ))
				{
					fprintf( stderr, "%c", buf[ j ] );
				}
				else
				{
					fprintf( stderr, "." );
				}
			}

			fprintf( stderr, "\n" );

			count = 0;
		}
	}

	if( count != 16 )
	{
		for( j = 0; j < 16 - count; ++j )
		{
			fprintf( stderr, "   " );
		}

		fprintf( stderr, "\t" );

		for( j = i - count; j < i; ++j )
		{
			if( isprint(( int ) buf[ j ] ))
			{
				fprintf( stderr, "%c", buf[ j ] );
			}
			else
			{
				fprintf( stderr, "." );
			}
		}

		fprintf( stderr, "\n" );
	}
}

unsigned char* GetMac(const char* a_sDev, unsigned char* a_pBuf)
{
    int   i, sockfd;
    unsigned char *pdata;
    struct ifreq  ifr;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        return ((unsigned char*) -1);
    }

    strncpy(ifr.ifr_name, a_sDev, IFNAMSIZ);
    ifr.ifr_hwaddr.sa_family = AF_INET;

    if (ioctl(sockfd, SIOCGIFHWADDR, &ifr) < 0)
    {
        return ((unsigned char*) -1);
    }

    pdata = (unsigned char*)ifr.ifr_hwaddr.sa_data;

    for (i = 0; i < 6; i++)
        a_pBuf[i] = *pdata++;

    close(sockfd);
    return (a_pBuf);


}


unsigned int GetIP(const char* a_sDev)
{
    int   sockfd;
    unsigned int addr = (unsigned int)-1;

    struct ifreq ifr;
    struct sockaddr_in *sap;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        return ((unsigned int)-1);
    }

    strncpy(ifr.ifr_name, a_sDev, IFNAMSIZ);
    ifr.ifr_name[IFNAMSIZ-1] = '\0';
    ifr.ifr_addr.sa_family = AF_INET;

    if (ioctl(sockfd, SIOCGIFADDR, &ifr) < 0)
    {
    }
    else
    {
        sap  = (struct sockaddr_in*)&ifr.ifr_addr;
        addr = *((unsigned int*)&sap->sin_addr);
    }
    close(sockfd);

    return (addr);
}


int LinkStat(const char* a_sDev)
{
    int s;
    struct ifreq ifr;
    struct ethtool_value eth;

    if((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP)) == -1)
	{
		return false;
	}

    bzero(&ifr, sizeof(ifr));
    strcpy(ifr.ifr_name,a_sDev);
    ifr.ifr_data = (caddr_t) &eth;
    eth.cmd = ETHTOOL_GLINK;

    if(ioctl(s, SIOCETHTOOL, &ifr) == -1)
			return false;

	close(s);

    return (eth.data) ? true:false;
}


/*
int main(void)
{
	unsigned char* pMac = (unsigned char*)malloc(6* sizeof(unsigned char));
	unsigned int nIP=0;
	int nLinkState=-1;

	GetMac("eth1",pMac);
	printf("Mac address is %s\n",pMac);

	USleep(1000000);

	nIP = GetIP("eth0");
	printf("IP address is %u\n", nIP);

	nLinkState = LinkStat("eth0");
	printf("eth0 link status is %d\n",nLinkState);


	return 0; 



}
*/
