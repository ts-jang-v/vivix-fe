/******************************************************************************/
/**
 * @file    vw_update.cpp
 * @brief   system update
 * @author  
*******************************************************************************/

/*******************************************************************************
*   Include Files
*******************************************************************************/
#include <errno.h>          // errno
#include <stdio.h>
#include <string.h>			// memset() memset/memcpy
#include <stdlib.h>			// malloc(), free(),exit(), EXIT_SUCCESS rand/rand 
#include <iostream>
#include "vw_download.h"
#include "vw_file.h"

/*******************************************************************************
*   Constant Definitions
*******************************************************************************/


/*******************************************************************************
*   Type Definitions
*******************************************************************************/


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
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
CVWDownload::CVWDownload( const s8* i_p_file_name, s32 (*log)(s32, s32, const s8*,...))
{
    print_log = log;
    strcpy( m_p_log_id, "VWDownload    " );
    
    strcpy( m_p_file_name, i_p_file_name );
    
    m_p_data = NULL;
    m_n_current_size = 0;
    m_n_total_size = 0;
    
    print_log(DEBUG, 1, "[%s] CVWDownload(path: %s)\n", m_p_log_id, m_p_file_name);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
CVWDownload::CVWDownload(void)
{
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
CVWDownload::~CVWDownload(void)
{
    safe_free( m_p_data );
    print_log(DEBUG, 1, "[%s] ~CVWDownload\n", m_p_log_id);
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
bool CVWDownload::start( u32 i_n_crc, u32 i_n_total_size )
{
    print_log(DEBUG, 1, "[%s] download(crc: 0x%08X, size: %d) start\n", \
                                    m_p_log_id, i_n_crc, i_n_total_size );
    
    reset();
    
    if( i_n_total_size > 0 )
    {
        m_n_total_size = i_n_total_size;
        
        m_p_data = (u8*)malloc(m_n_total_size);
        
        if( m_p_data == NULL )
        {
            print_log(DEBUG, 1, "[%s] malloc failed.\n", m_p_log_id );
            
            return false;
        }
        
        memset( m_p_data, 0, m_n_total_size );
        
        m_n_current_size    = 0;
        m_n_crc             = i_n_crc;
        
        return true;
    }
    
    return false;
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
bool CVWDownload::set_data( u8* i_p_data, u32 i_n_size )
{
    if( m_n_total_size == 0
        || i_n_size == 0 )
    {
        print_log(ERROR, 1, "[%s] invalid parameters(size: %d, total size: %d\n", \
                            m_p_log_id, i_n_size, m_n_total_size  );
        return false;
    }
    
    if( m_n_current_size + i_n_size > m_n_total_size )
    {
        u32 n_last = m_n_total_size - m_n_current_size;
        
        memcpy( &m_p_data[m_n_current_size], i_p_data, n_last );
        m_n_current_size += n_last;
    }
    else
    {
        memcpy( &m_p_data[m_n_current_size], i_p_data, i_n_size );
        m_n_current_size += i_n_size;
    }
    
    if( m_n_total_size == m_n_current_size )
    {
        print_log(DEBUG, 1, "[%s] dowload(total size: %d) complete\n", \
                               m_p_log_id, m_n_total_size );
    }
    
    return true;
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
bool CVWDownload::verify(void)
{
    if( m_n_total_size == 0
        || m_n_total_size != m_n_current_size )
    {
        print_log(ERROR, 1, "[%s] invalid size(%d/%d)\n", \
                            m_p_log_id, m_n_current_size, m_n_total_size );
        return false;
    }
    
    if( m_n_total_size == m_n_current_size )
    {
        u32 n_crc;
    
        n_crc = make_crc32( m_p_data, m_n_total_size );
        
        if( n_crc != m_n_crc )
        {
            print_log(DEBUG, 1, "[%s] dowload data crc error(0x%08X/0x%08X)\n", \
                                 m_p_log_id, n_crc, m_n_crc );
            reset();
            return false;
        }
        
        return true;
    }
    
    return false;
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
bool CVWDownload::save(u32 i_n_data_offset, u32 i_n_data_size)
{
    if( m_n_total_size == 0
        || m_n_current_size == 0
        || m_n_total_size != m_n_current_size )
    {
        print_log(ERROR, 1, "[%s] invalid download size(%d/%d)\n", \
                            m_p_log_id, m_n_current_size, m_n_total_size);
        reset();
        return false;
    }

    u32 target_binary_offset = 0x0000;
    u32 target_binary_size = 0x0000;
    if( (i_n_data_offset == 0x0000) && (i_n_data_size == 0x0000) )
    {
        target_binary_offset = 0x0000;
        target_binary_size = m_n_total_size;
    }
    else
    {
        target_binary_offset = i_n_data_offset;
        target_binary_size = i_n_data_size;
    }

    print_log(DEBUG, 1, "[%s] save file(%s) (binary offset: %d binary size: %d)\n", m_p_log_id, m_p_file_name, target_binary_offset, target_binary_size);
    print_log(DEBUG, 1, "[%s] save file (start: 0x%02x end: 0x%02x\n", m_p_log_id, (u8)m_p_data[target_binary_offset], (u8)(m_p_data[target_binary_offset + target_binary_size - 1]));

    FILE* p_file;
    p_file = fopen( m_p_file_name, "wb" );
    
    if( p_file != NULL )
    {
        fwrite( &(m_p_data[target_binary_offset]), 1, target_binary_size, p_file );
	    fclose( p_file );
        
    }
    else
    {
        print_log(ERROR, 1, "[%s] file open is failed.\n", m_p_log_id);
        reset();
        return false;
    }
    
    safe_free( m_p_data );
    return true;
    
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void CVWDownload::reset(void)
{
    print_log(DEBUG, 1, "[%s] download reset\n", m_p_log_id);
    
    m_n_current_size = 0;
    m_n_total_size = 0;
    safe_free( m_p_data );
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CVWDownload::self_test(const s8* i_p_file_name)
{
    print_log(DEBUG, 1, "[%s] self test start(file: %s)\n", m_p_log_id, i_p_file_name);
    
    FILE*   p_file;
    s32     n_file_size;
    s32     n_crc;
    s32     i;
    u8*     p_data;
    
    p_file = fopen( i_p_file_name, "rb" );
    
    if( p_file == NULL )
    {
        print_log(DEBUG, 1, "[%s] file(%s) open failed.\n", m_p_log_id, i_p_file_name);
        return false;
    }
    
    n_file_size = file_get_size( p_file );
    
    p_data = (u8*)malloc(n_file_size);
    
    if( p_data == NULL )
    {
        print_log(DEBUG, 1, "[%s] malloc failed.\n", m_p_log_id );
        
        fclose( p_file );
        return false;
    }
    
    fread( p_data, 1, n_file_size, p_file );
    fclose( p_file );
    
    n_crc = make_crc32( p_data, n_file_size );
    
    start( n_crc, n_file_size );
    
    for( i = 0; i < n_file_size; i+=1420 )
    {
        if( i + 1420 < n_file_size )
        {
            set_data( &p_data[i], 1420 );
        }
        else
        {
            set_data( &p_data[i], (n_file_size - i) );
        }
    }
    
    save(0x0000, 0x0000);
    
    safe_free( p_data );
    
    return true;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u32 CVWDownload::search_fpga_major_version_loc(const s8* i_p_target_major_version, u32* i_p_binary_offset, u32* i_p_binary_size)
{
    unsigned int	n_index;	
    const char*		p_magic = "0x20140731";
    unsigned int	n_magic_len = strlen(p_magic);
    const char*		p_target_ver = i_p_target_major_version;
    unsigned int	n_target_ver_len = strlen(p_target_ver);
	//unsigned int	end = 0x0000;
	unsigned int	start = 0xFFFFFFFF;	
    bool            found = 0;
    unsigned int    binary_offset = 0;
    unsigned int    binary_size = 0;

    print_log(DEBUG, 1, "[%s] p_target_ver: %c n_target_ver_len: %d\n", m_p_log_id, *(p_target_ver + n_target_ver_len - 1), n_target_ver_len);

    if( m_n_total_size == 0
        || m_n_current_size == 0
        || m_n_total_size != m_n_current_size )
    {
        print_log(ERROR, 1, "[%s] invalid download size(%d/%d)\n", \
                            m_p_log_id, m_n_current_size, m_n_total_size);
        reset();
        return 0;
    }

    for(n_index =  0; n_index < m_n_current_size - n_magic_len + 1  ; n_index++)
    {	
        if(!memcmp(m_p_data + n_index, p_magic, n_magic_len))    //같으면 if 문 진입 
        {
			if(start == 0xFFFFFFFF)
			{
				start = n_index;
				continue;
			}

			//end = n_index;
			binary_size = n_index - binary_offset + n_magic_len;

            print_log(DEBUG, 1, "[%s] embedded fpga binary at (0x%08X, 0x%08X).\n", m_p_log_id, binary_offset, binary_size);

            //print_log(DEBUG, 1, "[%s] p_start + n_magic_len [0]: %c [1]: %c [2]: %c\n", m_p_log_id, m_p_data[start + n_magic_len], m_p_data[start + n_magic_len + 1], m_p_data[start + n_magic_len + 2]);

            //FOUND version
            if(memcmp( &(m_p_data[start + n_magic_len]), p_target_ver, n_target_ver_len ) == 0)  //FOUND COMPATIBLE
            {
                found = true;
                break;
            }
            else    //FOUND UNCOMPATIBLE
            {
                binary_offset = n_index + n_magic_len;
                start = 0xFFFFFFFF;
            }
        }
    }

    if(found)
    {
        *i_p_binary_offset = binary_offset; //이전 end 의 +  n_magic_len + 1?
        *i_p_binary_size = binary_size; // offset + n_magic_len + 2 + 1 ?
        print_log(DEBUG, 1, "[%s] search_fpga_major_version_loc success(0x%08X, 0x%08X).\n", m_p_log_id, *i_p_binary_offset, *i_p_binary_size);
        return 0;
    }
    else
    {
        *i_p_binary_offset = 0;
        *i_p_binary_size = 0;
        print_log(DEBUG, 1, "[%s] search_fpga_major_version_loc failed(0x%08X, 0x%08X).\n", m_p_log_id, *i_p_binary_offset, *i_p_binary_size);
        return 1;
    }
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
 *          
*******************************************************************************/
bool CVWDownload::get_version(char* s_version)
{	
    unsigned int	n_index;	
    const char*		p_magic = "0x20140731";
    unsigned int	n_magic_len = strlen(p_magic);
    unsigned int	n_version_len = 0;

    unsigned int	end = 0x0000;
	unsigned int	start = 0xFFFFFFFF;	
    
    bool            found = false;

    //print_log(DEBUG, 1, "[%s] Bootloader compability policy: If ADI ROIC, Prohibit Btldr version lower than 0.0.5\n", m_p_log_id);

    //version discover logic. 
    //version 사양: 버전 정보는 magic string 사이에 string 형태로 존재
    //              버전 정보는 xxx.yyy.zzz 형태의 string 형태로 시작함
    //              버전 정보는 128byte를 초과하지않음.
    for(n_index =  0; n_index < m_n_current_size - n_magic_len + 1; n_index++)
    {	
        if(!memcmp(m_p_data + n_index, p_magic, n_magic_len))    //같으면 if 문 진입 
        {
			if(start == 0xFFFFFFFF)
			{
				start = n_index;            //leader magic string 시작 index
				continue;
			}

            found = true;
			
            end = n_index;      //trailer magic string 시작 index
            n_version_len = end - start - n_magic_len;
			memcpy(s_version, &(m_p_data[start + n_magic_len]), n_version_len); 
            
            print_log(DEBUG, 1, "[%s] version: %s\n", m_p_log_id, s_version);
            break;
        }
    }
    return found;
}