/******************************************************************************/
/**
 * @file    vw_download.h
 * @brief   디텍터에 대용량의 데이터(update files, offset and gain data)를
 *          저장할 때 사용할 수 있다.
 * @author  
*******************************************************************************/


#ifndef _VW_DOWNLOAD_H_
#define _VW_DOWNLOAD_H_

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

/*******************************************************************************
*   Macros (Inline Functions) Definitions
*******************************************************************************/


/*******************************************************************************
*   Variable Definitions
*******************************************************************************/


/*******************************************************************************
*   Function Prototypes
*******************************************************************************/

/**
 * @class   
 * @brief   
 */
class CVWDownload
{
private:
    s8                  m_p_log_id[15];
    s32			        (* print_log)(s32 level, s32 print, const s8* format, ...);
    u8*                 m_p_data;
    s8                  m_p_file_name[256];
    u32                 m_n_current_size;
    u32                 m_n_total_size;
    u32                 m_n_crc;
    
    void                reset(void);
    
protected:
    
public:
    CVWDownload( const s8* i_p_file_name, s32 (*log)(s32, s32, const s8*,...));
    CVWDownload(void);
    ~CVWDownload(void);
    
    bool        start( u32 i_n_crc, u32 i_n_total_size );
    bool        set_data( u8* i_p_data, u32 i_n_size);
    bool        verify(void);
    bool        save(u32 i_n_data_offset, u32 i_n_data_size);
    bool        self_test(const s8* i_p_file_name);

    u32         search_fpga_major_version_loc(const s8* i_p_target_major_version, u32* i_p_binary_offset, u32* i_p_binary_size);

    bool        get_version(char* s_version);

};



#endif /* _VW_DOWNLOAD_H_ */
