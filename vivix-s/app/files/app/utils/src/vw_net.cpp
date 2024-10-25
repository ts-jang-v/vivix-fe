/******************************************************************************/
/**
 * @file    
 * @brief   
 * @author  
*******************************************************************************/

/*******************************************************************************
*   Include Files
*******************************************************************************/
#include <iostream>
#include <sys/ioctl.h>          // ioctl 
#include <netinet/in.h>         // struct sockaddr_in, htons(), ntohs()
//#include <net/if.h>             // struct ifconf, struct ifreq
#include <net/ethernet.h>       // ETHERMTU
#include <linux/sockios.h> 
#include <string.h>             // memset(), strcpy()
#include <linux/ethtool.h>      // ETHTOOL_GLINK
#include <unistd.h>
#include <ifaddrs.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/wireless.h>
#include <stdio.h>
#include <errno.h>          // errno
#include <stdlib.h>
#include <linux/mii.h>

#include "vw_net.h"
#include "misc.h"           // sys_command

using namespace std;

/*******************************************************************************
*   Constant Definitions
*******************************************************************************/
#define SIOCPHYREAD	(SIOCDEVPRIVATE + 12)
#define SIOCPHYWRITE    (SIOCDEVPRIVATE + 13)
#define PHY_ADDR   (0)

/*******************************************************************************
*   Type Definitions
*******************************************************************************/
struct phy_control_ioctl_data{
	u16	page;
	u16	reg_num;
	u16	val_in;
	u16	val_out;
};


/*******************************************************************************
*   Macros (Inline Functions) Definitions
*******************************************************************************/


/*******************************************************************************
*   Variable Definitions
*******************************************************************************/


/*******************************************************************************
*   Function Prototypes
*******************************************************************************/

/******************************************************************************/
/**
 * @brief   네트워크 장치의 MAC 주소를 알려준다.
 * @param   i_p_dev_name    장치 이름
 * @param   o_p_mac         장치의 MAC 주소
 * @return  장치의 MAC 주소
 * @note    
*******************************************************************************/
u8* vw_net_if_get_mac( const s8* i_p_dev_name, u8* o_p_mac )
{
    s32     i, n_sock_fd;
    u8*     p_data;
    struct ifreq  ifr;

    if ((n_sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        return ((u8*) -1);
    }

    strncpy(ifr.ifr_name, i_p_dev_name, IFNAMSIZ);
    ifr.ifr_hwaddr.sa_family = AF_INET;

    if (ioctl(n_sock_fd, SIOCGIFHWADDR, &ifr) < 0)
    {
        return ((u8*) -1);
    }

    p_data = (u8*)ifr.ifr_hwaddr.sa_data;

    for (i = 0; i < 6; i++)
        o_p_mac[i] = *p_data++;

    close(n_sock_fd);
    return (o_p_mac);
}

/******************************************************************************/
/**
 * @brief   네트워크 장치의 IP 주소를 알려준다.
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u32 vw_net_if_get_ip( const s8* i_p_dev_name )
{
    s32 n_sock_fd;
    u32 n_addr = (u32)-1;

    struct ifreq ifr;
    struct sockaddr_in *sap;

    if ((n_sock_fd = socket( AF_INET, SOCK_DGRAM, 0 )) < 0)
    {
        return ((u32)-1);
    }

    strncpy( ifr.ifr_name, i_p_dev_name, IFNAMSIZ );
    ifr.ifr_name[IFNAMSIZ-1] = '\0';
    ifr.ifr_addr.sa_family = AF_INET;

    if (ioctl( n_sock_fd, SIOCGIFADDR, &ifr ) < 0)
    {
    }
    else
    {
        sap  = (struct sockaddr_in*)&ifr.ifr_addr;
        n_addr = *((u32*)&sap->sin_addr);
    }
    close(n_sock_fd);

    return (n_addr);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool vw_net_link_state( const s8* i_p_dev_name )
{
    s32     n_sock_fd;
    struct ifreq ifr;
    struct ethtool_value eth;

    if((n_sock_fd = socket( AF_INET, SOCK_DGRAM, IPPROTO_IP )) == -1)
	{
		return false;
	}

    bzero( &ifr, sizeof(ifr) );
    strcpy( ifr.ifr_name, i_p_dev_name );
    ifr.ifr_data = (caddr_t) &eth;
    eth.cmd = ETHTOOL_GLINK;

    if(ioctl( n_sock_fd, SIOCETHTOOL, &ifr ) == -1)
	{
		close(n_sock_fd);
		return false;
	}

	close(n_sock_fd);

    return (eth.data) ? true:false;
}

/******************************************************************************/
/**
 * @brief   WLAN 설정 전달
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool vw_net_wlan_state( const s8* i_p_dev_name, vw_wlan_state_t* o_p_state_t )
{
    s32                     n_sock = -1;
    struct iwreq            req_t;
    struct iw_statistics    stats_t;
    s8                      p_buffer[32];
    bool                    b_result = true;
	
    if( (n_sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1 )
	{
	    b_result = false;
    }

    // AP MAC address
    memset(&req_t, 0, sizeof(req_t));
    strncpy(req_t.ifr_name, i_p_dev_name, IFNAMSIZ);
    if( ioctl(n_sock, SIOCGIWAP, &req_t) != -1 )
    {
        memcpy( o_p_state_t->p_ap_mac, req_t.u.ap_addr.sa_data, sizeof(o_p_state_t->p_ap_mac) );
    }
    else
    {
        b_result = false;
    }
    
    // ESSID
	memset(&req_t, 0, sizeof(req_t));
	memset(p_buffer, 0, sizeof(p_buffer));
	strncpy(req_t.ifr_name, i_p_dev_name, IFNAMSIZ);
    req_t.u.essid.pointer   = p_buffer;
    req_t.u.essid.length    = sizeof(p_buffer);
    if( ioctl(n_sock, SIOCGIWESSID, &req_t) != -1 )
    {
        strcpy( o_p_state_t->p_essid, p_buffer );
    }
    else
    {
        b_result = false;
    }
    
    // frequency
	memset(&req_t, 0, sizeof(req_t));
	strncpy(req_t.ifr_name, i_p_dev_name, IFNAMSIZ);
    if( ioctl(n_sock, SIOCGIWFREQ, &req_t) != -1 )
    {
        o_p_state_t->n_frequency = req_t.u.freq.m;
    }
    else
    {
        b_result = false;
    }
    
	// link quality and level
	memset(&req_t, 0, sizeof(req_t));
	strncpy(req_t.ifr_name, i_p_dev_name, IFNAMSIZ);
	req_t.u.data.pointer =  &stats_t;
	req_t.u.data.length = sizeof(stats_t);
	if( ioctl(n_sock, SIOCGIWSTATS, &req_t) != -1 )
	{
		o_p_state_t->c_quality    = ((struct iw_statistics *)req_t.u.data.pointer)->qual.qual;
		o_p_state_t->n_level      = ((struct iw_statistics *)req_t.u.data.pointer)->qual.level - 256;
	}
    else
    {
        b_result = false;
    }
	
	// bit rate
#if 0	
	memset(&req_t, 0, sizeof(req_t));
	strncpy(req_t.ifr_name, i_p_dev_name, IFNAMSIZ);
	if( ioctl(n_sock, SIOCGIWRATE, &req_t) != -1 )
	{
	    o_p_state_t->n_bitrate = req_t.u.bitrate.value / 1000000;
    }
    else
    {
        b_result = false;
    }
#else
	{
		u8 p_buffer[30];
		if( process_get_result("iw wlan0 station dump | awk '/rx bitrate:/ {print $3}'", (char*)p_buffer) != 0)
		{
			o_p_state_t->n_bitrate = atof((char*)p_buffer);
		}
		else
		{
			o_p_state_t->n_bitrate = 0;
		}
	}
#endif	
    
    // tx power
	memset(&req_t, 0, sizeof(req_t));
	strncpy(req_t.ifr_name, i_p_dev_name, IFNAMSIZ);
	if( ioctl(n_sock, SIOCGIWTXPOW, &req_t) != -1 )
	{
	     o_p_state_t->n_txpower = req_t.u.txpower.value;
    }
    else
    {
        b_result = false;
    }
	
	close(n_sock);
	
	return b_result;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool vw_net_ip_exist( const s8* i_p_dev_name, const s8* i_p_ip_address )
{
    s8      p_buff[256];
    FILE*   p_file;
    bool    b_result = false;
    
	memset( p_buff, 0, sizeof(p_buff) );
	sprintf( p_buff, "arping -q -D -I %s -c 1 %s; echo $? > /tmp/ip_exist", \
	                    i_p_dev_name, i_p_ip_address );
	sys_command(p_buff);
	
	memset( p_buff, 0, sizeof(p_buff) );
	
	p_file = fopen("/tmp/ip_exist", "r");
	
	if( p_file == NULL )
	{
		return true;
	}
	
	fscanf( p_file, "%s", p_buff );
	fclose( p_file );
	
	if( atoi( p_buff ) > 0 )
	{
	    b_result = true;
	}
	
	return b_result;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
int vw_net_switch_phy_read( const s8* i_p_dev_name, u16 i_w_page, u16 i_w_addr, u16* o_p_value )
{
	s32 n_sock;
	struct ifreq ifr_t; 
	struct phy_control_ioctl_data *control_data_t;
	
	n_sock = socket(AF_INET, SOCK_DGRAM, 0);
	if( n_sock < 0) 
	{    
		printf("failed to create socket (errno: %d)", errno);
		return errno;
	}   
	
	memset(&ifr_t, 0, sizeof(ifr_t));
	strcpy(ifr_t.ifr_name, i_p_dev_name);

	control_data_t              = (struct phy_control_ioctl_data *)&ifr_t.ifr_data;
	control_data_t->page        = i_w_page;
	control_data_t->reg_num     = i_w_addr;

	//printf("read page : 0x%x, reg num : 0x%x\n", control_data_t->page, control_data_t->reg_num);
	
	if (ioctl(n_sock, SIOCPHYREAD, &ifr_t) < 0) 
	{    
		printf("failed to get register value(errno : %d)\n", errno);
		close (n_sock);
		return errno;
	}   
	
	*o_p_value = control_data_t->val_out;
	//printf("read success, value : 0x%x\n", *o_p_value);
	
	close( n_sock );
	return 0;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool vw_net_switch_phy_write( const s8* i_p_dev_name, u16 i_w_page, u16 i_w_addr, u16 i_w_value )
{
	s32 n_sock;
	struct ifreq ifr_t; 
	struct phy_control_ioctl_data *control_data_t;
	
	n_sock = socket(AF_INET, SOCK_DGRAM, 0);
	if( n_sock < 0) 
	{    
		printf("failed to create socket (errno: %d)", errno);
		return false;
	}   
	
	memset(&ifr_t, 0, sizeof(ifr_t));
	strcpy(ifr_t.ifr_name, i_p_dev_name);

	control_data_t              = (struct phy_control_ioctl_data *)&ifr_t.ifr_data;
	control_data_t->page        = i_w_page;
	control_data_t->reg_num     = i_w_addr;
    control_data_t->val_in      = i_w_value;
	
	//printf("write page : 0x%x, reg num : 0x%x, value: 0x%x\n", 
	//            control_data_t->page, control_data_t->reg_num, control_data_t->val_in);
	
	if (ioctl(n_sock, SIOCPHYWRITE, &ifr_t) < 0) 
	{    
		perror("failed to write register value");
		close (n_sock);
		return false;
	}
	//printf("write success\n");
	
	close( n_sock );
	return true;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool vw_net_phy_read( const s8* i_p_dev_name, u16 i_w_addr, u16* o_p_value )
{
	s32     n_sock;
	struct ifreq ifr_t; 
	struct mii_ioctl_data *mii_t;

	n_sock = socket(AF_INET, SOCK_DGRAM, 0);
	if( n_sock < 0) 
	{    
		printf("failed to create socket (errno: %d)", errno);
		return false;
	}   
	
	memset(&ifr_t, 0, sizeof(ifr_t));
	strcpy(ifr_t.ifr_name, i_p_dev_name);
	
	mii_t             = (struct mii_ioctl_data *)&ifr_t.ifr_data;
	mii_t->phy_id     = PHY_ADDR;

	mii_t->reg_num    = i_w_addr;
	
	if (ioctl(n_sock, SIOCGMIIREG, &ifr_t) < 0) 
	{    
		printf("failed to get register value(errno : %d)\n", errno);
		close (n_sock);
		return false;
	}   
	
	*o_p_value = mii_t->val_out;
	//printf("read success, value : 0x%x\n", *o_p_value);
	
	close( n_sock );
	return true;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool vw_net_phy_write( const s8* i_p_dev_name, u16 i_w_addr, u16 i_w_value )
{
	s32 n_sock;
	struct ifreq ifr_t; 
	struct mii_ioctl_data *mii_t;
	
	n_sock = socket(AF_INET, SOCK_DGRAM, 0);
	if( n_sock < 0) 
	{    
		printf("failed to create socket (errno: %d)", errno);
		return false;
	}   
	
	memset(&ifr_t, 0, sizeof(ifr_t));
	strcpy(ifr_t.ifr_name, i_p_dev_name);

	mii_t              = (struct mii_ioctl_data *)&ifr_t.ifr_data;
	mii_t->reg_num     = i_w_addr;
    mii_t->val_in      = i_w_value;
	
	//printf("write page : 0x%x, reg num : 0x%x, value: 0x%x\n", 
	//            control_data_t->page, control_data_t->reg_num, control_data_t->val_in);
	
	if (ioctl(n_sock, SIOCSMIIREG, &ifr_t) < 0) 
	{    
		perror("failed to write register value");
		close (n_sock);
		return false;
	}
	//printf("write success\n");
	
	close( n_sock );
	return true;
}

