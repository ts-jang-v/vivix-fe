/*
 * fpga_prom.h
 *
 *  Created on: 2014. 8. 7.
 *      Author: myunghee
 */

 
#ifndef _FPGA_PROM_H_
#define _FPGA_PROM_H_

/*******************************************************************************
*	Include Files
*******************************************************************************/
#include "typedef.h"

/*******************************************************************************
*	Constant Definitions
*******************************************************************************/
#define FLASH_PAGE_SIZE	256

/*******************************************************************************
*	Type Definitions
*******************************************************************************/

/*******************************************************************************
*	Macros (Inline Functions) Definitions
*******************************************************************************/


/*******************************************************************************
*	Variable Definitions
*******************************************************************************/


/*******************************************************************************
*	Function Prototypes
*******************************************************************************/

class CPROM
{
private:
    s8          m_p_log_id[15];
    int			(* print_log)(int level,int print,const char * format, ...);
	
    // driver file descriptor
    s32         m_n_fd;
    u8          get_state( u8 i_c_type );
    
    u16         m_w_progress;
    
protected:
public:
    
	CPROM(int (*log)(int,int,const char *,...));
	CPROM(void);
	~CPROM();
    
    void	read_id( u8* o_p_mf_id, u16* o_p_dev_id );
    void	set_qpi_mode( bool i_b_enable );
    bool    erase(void);
    bool    write_page( u32 i_n_addr, u8* i_p_data, u16 i_w_len );
    bool    verify( const s8* i_p_file_name );
    u16     get_progress(void){return m_w_progress;}
    void 	set_flash_access_enable( bool i_b_onoff );
};


#endif /* end of _FPGA_PROM_H_*/

