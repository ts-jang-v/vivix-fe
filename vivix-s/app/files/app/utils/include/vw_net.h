/******************************************************************************/
/**
 * @file    
 * @brief   
 * @author  
*******************************************************************************/


#ifndef _VW_NET_H_
#define _VW_NET_H_

/*******************************************************************************
*   Include Files
*******************************************************************************/
#include "typedef.h"


/*******************************************************************************
*   Constant Definitions
*******************************************************************************/


/*******************************************************************************
*   Type Definitions
*******************************************************************************/

/**
 * @brief   wireless state
 */
typedef struct _vw_wlan_state
{
    u8      p_ap_mac[6];
    s8      p_essid[32];
    s32     n_frequency;
    u8      c_quality;
    s32     n_level;
    s32     n_bitrate;
    s32     n_txpower;
} vw_wlan_state_t;


/*******************************************************************************
*   Macros (Inline Functions) Definitions
*******************************************************************************/


/*******************************************************************************
*   Variable Definitions
*******************************************************************************/


/*******************************************************************************
*   Function Prototypes
*******************************************************************************/
u8*     vw_net_if_get_mac( const s8* i_p_dev_name, u8* o_p_mac );
u32     vw_net_if_get_ip( const s8* i_p_dev_name );
bool    vw_net_link_state( const s8* i_p_dev_name );
bool    vw_net_wlan_state( const s8* i_p_dev_name, vw_wlan_state_t* o_p_state );
bool    vw_net_ip_exist( const s8* i_p_dev_name, const s8* i_p_ip_address );

int     vw_net_switch_phy_read( const s8* i_p_dev_name, u16 i_w_page, u16 i_w_addr, u16* o_p_value );
bool    vw_net_switch_phy_write( const s8* i_p_dev_name, u16 i_w_page, u16 i_w_addr, u16 i_w_value );

bool    vw_net_phy_read( const s8* i_p_dev_name, u16 i_w_addr, u16* o_p_value );
bool    vw_net_phy_write( const s8* i_p_dev_name, u16 i_w_addr, u16 i_w_value );

#endif /* _VW_NET_H_ */
