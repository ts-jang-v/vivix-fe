/******************************************************************************/
/**
 * @file    fpga_prom.cpp
 * @brief   FPGA configuration file을 PROM에 저장한다.
 * @author    
*******************************************************************************/

/*******************************************************************************
*   Include Files
*******************************************************************************/
#include <iostream>
#include <errno.h>      // errno
#include <stdio.h>      // fprintf()
#include <string.h>     // memset() memset/memcpy
#include <stdlib.h>     // malloc(), free(),exit(), EXIT_SUCCESS rand/rand 
#include <sys/ioctl.h>  // ioctl()
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>      // open() O_NDELAY
#include <unistd.h>

#include "fpga_prom.h"
#include "vworks_ioctl.h"
#include "vw_time.h"
#include "vw_file.h"

/*******************************************************************************
*   Constant Definitions
*******************************************************************************/
#define PROM_FULL_CHIP_ERASE_TIMEOUT        (210000)    // 210 sec. (Winbond - Max 200s)
#define PROM_PAGE_PROGRAM_TIMEOUT        	(3500)    // 3.5 sec. (Winbond - Max 3s)

/*******************************************************************************
*   Type Definitions
*******************************************************************************/
typedef union _flash_status_reg
{
	union
	{
		u8	DATA;
		struct
		{
			u8	WPROG			: 1;		/* BIT0		: write in progress		*/
			u8	WEN_LAT			: 1;   		/* BIT1		: write enable latch	*/
			u8	BLK_PROTECT		: 3;        /* BIT2:4	: block protect			*/
			u8	TB_PROTECT		: 1;        /* BIT5		: top/bottom protect 	*/
			u8	SECT_PROTECT	: 1;        /* BIT6		: sector protect		*/
			u8	SR_PROTECT		: 1;        /* BIT7		: status register protect */
		}BIT;
	}STATUS;
	
}flash_status_reg_u;

enum
{
    eREADY = 0,
    eBUSY = 1,
};

enum
{
    eSTATUS_REG = 0,
    eSTATUS_REG2 = 1,
    
};

enum
{
    eWRITE_DISABLE = 0,
    eWRITE_ENABLE = 1,
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
* \brief        constructor
* \return       none
* \note
*******************************************************************************/
CPROM::CPROM(int (*log)(int,int,const char *,...))
{
    print_log = log;
    m_w_progress = 0;

    strcpy( m_p_log_id, "PROM          " );
    
    m_n_fd = open("/dev/vivix_flash",O_RDWR|O_NDELAY);
    
    if( m_n_fd < 0 )
    {
        print_log(ERROR, 1, "[%s] /dev/vivix_flash device open error.\n", m_p_log_id);
        return;
    }

    print_log(DEBUG, 1, "[%s] CPROM\n", m_p_log_id);

}
/******************************************************************************/
/**
* \brief        constructor
* \param        none
* \return       none
* \note
*******************************************************************************/
CPROM::CPROM(void)
{
}

/******************************************************************************/
/**
* \brief        destructor
* \param        none
* \return       none
* \note
*******************************************************************************/
CPROM::~CPROM()
{
    if( m_n_fd != -1 )
    {
        close(m_n_fd);
    }
    
    print_log(DEBUG, 1, "[%s] ~CPROM\n", m_p_log_id );
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CPROM::set_qpi_mode( bool i_b_enable )
{
    ioctl_data_t    iod_t;
    
    memcpy( iod_t.data, &i_b_enable, 1 );
    print_log(DEBUG, 1, "[%s] Set qpi mode : %d\n", m_p_log_id, iod_t.data[0] );   
    ioctl( m_n_fd , VW_MSG_IOCTL_FLASH_QPI_ENABLE ,&iod_t );
}   

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CPROM::set_flash_access_enable( bool i_b_onoff )
{
    ioctl_data_t    iod_t;
	
    memcpy( iod_t.data, &i_b_onoff, 1 );    
    print_log(DEBUG, 1, "[%s] Set flash enable : %d\n", m_p_log_id, iod_t.data[0] );   
    ioctl( m_n_fd , VW_MSG_IOCTL_FLASH_EN ,&iod_t );
}    

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return    
 * @note    
*******************************************************************************/
void CPROM::read_id( u8* o_p_mf_id, u16* o_p_dev_id )
{
    ioctl_data_t    iod_t;
    
    memset(&iod_t, 0, sizeof(ioctl_data_t));
    ioctl( m_n_fd, VW_MSG_IOCTL_FLASH_READ_ID, &iod_t );    
    memcpy( o_p_mf_id, iod_t.data, 1 );    
    memcpy( o_p_dev_id, &iod_t.data[1], 2 );    
    
}

/******************************************************************************/
/**
 * @brief   
 * @param    asdfsadf
 * @return    dfasf
 * @note    
*******************************************************************************/
u8 CPROM::get_state( u8 i_c_type )
{
    ioctl_data_t    iod_t;
    u8                c_state;
    
    memcpy( iod_t.data, &i_c_type, 1 );
    ioctl( m_n_fd, VW_MSG_IOCTL_FLASH_STATUS, &iod_t );    
    memcpy( &c_state, iod_t.data, 1 );    
    
    return c_state;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CPROM::erase(void)
{
    ioctl_data_t        iod_t;
    flash_status_reg_u	reg_u;
    CTime*              p_wait_c = NULL;
    
    m_w_progress = 0;
    ioctl( m_n_fd, VW_MSG_IOCTL_FLASH_ERASE, &iod_t );
    
    p_wait_c = new CTime(PROM_FULL_CHIP_ERASE_TIMEOUT);
    while(1)
    {
        reg_u.STATUS.DATA = get_state(eSTATUS_REG);
        
        if( reg_u.STATUS.BIT.WPROG == eBUSY )
        {
            sleep_ex(1000000);
            m_w_progress++;
        }
        else
        {
            break;
        }
        if( p_wait_c->is_expired() == true )
        {
            safe_delete( p_wait_c );
            print_log(ERROR, 1, "[%s] ERASE WPROG busy time out\n", m_p_log_id);
            return false;
        }
    }
	safe_delete( p_wait_c );
    
    return true;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CPROM::write_page( u32 i_n_addr, u8* i_p_data, u16 i_w_len )
{
    u8                  p_buf[FLASH_PAGE_SIZE + 4];
    flash_status_reg_u	reg_u;
    CTime*              p_wait_c = NULL;
    
    if( i_w_len > FLASH_PAGE_SIZE )
    {
        print_log(ERROR, 1, "[%s] page size overflow(%d/%d)\n", \
                            m_p_log_id, i_w_len, FLASH_PAGE_SIZE);
        return false;
    }
    
    // set flash address to write
    p_buf[1] = (i_n_addr & 0x00ff0000) >> 16;
    p_buf[2] = (i_n_addr & 0x0000ff00) >> 8;
    p_buf[3] = (i_n_addr & 0x000000ff);
    
    memcpy( &p_buf[4], i_p_data, i_w_len );
    
    write( m_n_fd, p_buf, (i_w_len + 4) );
    
    // check result
    p_wait_c = new CTime(PROM_PAGE_PROGRAM_TIMEOUT);
    while(1)
    {
        reg_u.STATUS.DATA = get_state(eSTATUS_REG);
        
        if( reg_u.STATUS.BIT.WPROG == eBUSY )
        {
            sleep_ex(100);
        }
        else
        {
            break;
        }
        if( p_wait_c->is_expired() == true )
        {
            safe_delete( p_wait_c );
            print_log(ERROR, 1, "[%s] WRITE WPROG busy time out\n", m_p_log_id);
            return false;
        }
    }
    safe_delete( p_wait_c );
        
    return true;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    prom에 FPGA BIN write 후, read하여 FIGA BPIN과 memcmp 수행 
*******************************************************************************/
bool CPROM::verify( const s8* i_p_file_name )
{
    FILE*   p_file;
    s32     n_file_size;
    u8      p_file_data[FLASH_PAGE_SIZE];
    u8		p_flash_data[FLASH_PAGE_SIZE];
    s32     i;
    s32     n_last_size;
    bool    b_result = true;
    CTime*  p_wait_c = NULL;
    
    m_w_progress = 80;
    
    p_file = fopen( i_p_file_name, "rb" );
    
    if( p_file == NULL )
    {
        print_log(ERROR, 1, "[%s] file(%s) open failed.\n", \
                            m_p_log_id, i_p_file_name);
        return false;
    }
                                  
    n_file_size = file_get_size( p_file );
 
    //fread( p_data, 1, n_file_size, p_file );
    p_wait_c = new CTime(1000);
    i = 0;
    while( b_result == true )
    {
        if( i + FLASH_PAGE_SIZE < n_file_size )
        {
            fread( p_file_data, 1, FLASH_PAGE_SIZE, p_file );
            
            p_flash_data[1] = (i >> 16) & 0xff;
	    	p_flash_data[2] = (i >> 8) & 0xff;
	    	p_flash_data[3] = (i >> 0) & 0xff;
	    	
	    	read( m_n_fd, p_flash_data,FLASH_PAGE_SIZE );
            
            if( memcmp( p_flash_data, p_file_data, FLASH_PAGE_SIZE ) != 0 )
            {
                print_log(DEBUG, 1, "[%s] verification failed(0x%08X/0x%08X).\n", \
                                    m_p_log_id, i, n_file_size );
                b_result = false;
            }            
            i += FLASH_PAGE_SIZE;
        }
        else
        {
            n_last_size = n_file_size - i;
            fread( p_file_data, 1, n_last_size, p_file );
            
            p_flash_data[1] = (i >> 16) & 0xff;
	    	p_flash_data[2] = (i >> 8) & 0xff;
	    	p_flash_data[3] = (i >> 0) & 0xff;
	    	
	    	read( m_n_fd, p_flash_data,n_last_size );
            
            if( memcmp( p_flash_data, p_file_data, n_last_size ) != 0 )
            {
                print_log(DEBUG, 1, "[%s] verification failed(0x%08X/0x%08X).\n", \
                                    m_p_log_id, i, n_file_size );
                b_result = false;
            }
            
            i += n_last_size;
            
            break;
        }
        if( p_wait_c->is_expired() == true )
        {
            safe_delete( p_wait_c );
            p_wait_c = new CTime(1000);
            m_w_progress++;
        }
    }
    fclose( p_file );
    
    //safe_free( p_flash_malloc );
    safe_delete( p_wait_c );
    
    return b_result;
}
