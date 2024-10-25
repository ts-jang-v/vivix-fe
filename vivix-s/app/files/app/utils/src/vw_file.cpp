/*******************************************************************************
 모  듈 : vw_file
 작성자 : 한명희
 버  전 : 0.0.1
 설  명 : 
 참  고 : 

 버  전:
         0.0.1  최초 작성
*******************************************************************************/



/*******************************************************************************
*	Include Files
*******************************************************************************/
#include <iostream>
#include <sys/time.h>
#include <errno.h>          // errno
#include <stdio.h>
#include <string.h>
#include <stdlib.h>         // system()
#include <arpa/inet.h>		// socklen_t, inet_ntoa() ,ntohs(),htons()

#include "vw_file.h"
#include "misc.h"

using namespace std;

/*******************************************************************************
*	Constant Definitions
*******************************************************************************/
#define DO1(buf) crc = crc32_table[((int)crc ^ (*buf++)) & 0xff] ^ (crc >> 8);
#define DO2(buf)  DO1(buf); DO1(buf);
#define DO4(buf)  DO2(buf); DO2(buf);
#define DO8(buf)  DO4(buf); DO4(buf);


/*******************************************************************************
*	Type Definitions
*******************************************************************************/

/*******************************************************************************
*	Macros (Inline Functions) Definitions
*******************************************************************************/

/*******************************************************************************
*	Variable Definitions
*******************************************************************************/
/*
 Table of CRC-32's of all single-byte values (made by make_crc_table)
*/
static u32 crc32_table[256] = {
 0x00000000L, 0x77073096L, 0xee0e612cL, 0x990951baL,
 0x076dc419L, 0x706af48fL, 0xe963a535L, 0x9e6495a3L,
 0x0edb8832L, 0x79dcb8a4L, 0xe0d5e91eL, 0x97d2d988L,
 0x09b64c2bL, 0x7eb17cbdL, 0xe7b82d07L, 0x90bf1d91L,
 0x1db71064L, 0x6ab020f2L, 0xf3b97148L, 0x84be41deL,
 0x1adad47dL, 0x6ddde4ebL, 0xf4d4b551L, 0x83d385c7L,
 0x136c9856L, 0x646ba8c0L, 0xfd62f97aL, 0x8a65c9ecL,
 0x14015c4fL, 0x63066cd9L, 0xfa0f3d63L, 0x8d080df5L,
 0x3b6e20c8L, 0x4c69105eL, 0xd56041e4L, 0xa2677172L,
 0x3c03e4d1L, 0x4b04d447L, 0xd20d85fdL, 0xa50ab56bL,
 0x35b5a8faL, 0x42b2986cL, 0xdbbbc9d6L, 0xacbcf940L,
 0x32d86ce3L, 0x45df5c75L, 0xdcd60dcfL, 0xabd13d59L,
 0x26d930acL, 0x51de003aL, 0xc8d75180L, 0xbfd06116L,
 0x21b4f4b5L, 0x56b3c423L, 0xcfba9599L, 0xb8bda50fL,
 0x2802b89eL, 0x5f058808L, 0xc60cd9b2L, 0xb10be924L,
 0x2f6f7c87L, 0x58684c11L, 0xc1611dabL, 0xb6662d3dL,
 0x76dc4190L, 0x01db7106L, 0x98d220bcL, 0xefd5102aL,
 0x71b18589L, 0x06b6b51fL, 0x9fbfe4a5L, 0xe8b8d433L,
 0x7807c9a2L, 0x0f00f934L, 0x9609a88eL, 0xe10e9818L,
 0x7f6a0dbbL, 0x086d3d2dL, 0x91646c97L, 0xe6635c01L,
 0x6b6b51f4L, 0x1c6c6162L, 0x856530d8L, 0xf262004eL,
 0x6c0695edL, 0x1b01a57bL, 0x8208f4c1L, 0xf50fc457L,
 0x65b0d9c6L, 0x12b7e950L, 0x8bbeb8eaL, 0xfcb9887cL,
 0x62dd1ddfL, 0x15da2d49L, 0x8cd37cf3L, 0xfbd44c65L,
 0x4db26158L, 0x3ab551ceL, 0xa3bc0074L, 0xd4bb30e2L,
 0x4adfa541L, 0x3dd895d7L, 0xa4d1c46dL, 0xd3d6f4fbL,
 0x4369e96aL, 0x346ed9fcL, 0xad678846L, 0xda60b8d0L,
 0x44042d73L, 0x33031de5L, 0xaa0a4c5fL, 0xdd0d7cc9L,
 0x5005713cL, 0x270241aaL, 0xbe0b1010L, 0xc90c2086L,
 0x5768b525L, 0x206f85b3L, 0xb966d409L, 0xce61e49fL,
 0x5edef90eL, 0x29d9c998L, 0xb0d09822L, 0xc7d7a8b4L,
 0x59b33d17L, 0x2eb40d81L, 0xb7bd5c3bL, 0xc0ba6cadL,
 0xedb88320L, 0x9abfb3b6L, 0x03b6e20cL, 0x74b1d29aL,
 0xead54739L, 0x9dd277afL, 0x04db2615L, 0x73dc1683L,
 0xe3630b12L, 0x94643b84L, 0x0d6d6a3eL, 0x7a6a5aa8L,
 0xe40ecf0bL, 0x9309ff9dL, 0x0a00ae27L, 0x7d079eb1L,
 0xf00f9344L, 0x8708a3d2L, 0x1e01f268L, 0x6906c2feL,
 0xf762575dL, 0x806567cbL, 0x196c3671L, 0x6e6b06e7L,
 0xfed41b76L, 0x89d32be0L, 0x10da7a5aL, 0x67dd4accL,
 0xf9b9df6fL, 0x8ebeeff9L, 0x17b7be43L, 0x60b08ed5L,
 0xd6d6a3e8L, 0xa1d1937eL, 0x38d8c2c4L, 0x4fdff252L,
 0xd1bb67f1L, 0xa6bc5767L, 0x3fb506ddL, 0x48b2364bL,
 0xd80d2bdaL, 0xaf0a1b4cL, 0x36034af6L, 0x41047a60L,
 0xdf60efc3L, 0xa867df55L, 0x316e8eefL, 0x4669be79L,
 0xcb61b38cL, 0xbc66831aL, 0x256fd2a0L, 0x5268e236L,
 0xcc0c7795L, 0xbb0b4703L, 0x220216b9L, 0x5505262fL,
 0xc5ba3bbeL, 0xb2bd0b28L, 0x2bb45a92L, 0x5cb36a04L,
 0xc2d7ffa7L, 0xb5d0cf31L, 0x2cd99e8bL, 0x5bdeae1dL,
 0x9b64c2b0L, 0xec63f226L, 0x756aa39cL, 0x026d930aL,
 0x9c0906a9L, 0xeb0e363fL, 0x72076785L, 0x05005713L,
 0x95bf4a82L, 0xe2b87a14L, 0x7bb12baeL, 0x0cb61b38L,
 0x92d28e9bL, 0xe5d5be0dL, 0x7cdcefb7L, 0x0bdbdf21L,
 0x86d3d2d4L, 0xf1d4e242L, 0x68ddb3f8L, 0x1fda836eL,
 0x81be16cdL, 0xf6b9265bL, 0x6fb077e1L, 0x18b74777L,
 0x88085ae6L, 0xff0f6a70L, 0x66063bcaL, 0x11010b5cL,
 0x8f659effL, 0xf862ae69L, 0x616bffd3L, 0x166ccf45L,
 0xa00ae278L, 0xd70dd2eeL, 0x4e048354L, 0x3903b3c2L,
 0xa7672661L, 0xd06016f7L, 0x4969474dL, 0x3e6e77dbL,
 0xaed16a4aL, 0xd9d65adcL, 0x40df0b66L, 0x37d83bf0L,
 0xa9bcae53L, 0xdebb9ec5L, 0x47b2cf7fL, 0x30b5ffe9L,
 0xbdbdf21cL, 0xcabac28aL, 0x53b39330L, 0x24b4a3a6L,
 0xbad03605L, 0xcdd70693L, 0x54de5729L, 0x23d967bfL,
 0xb3667a2eL, 0xc4614ab8L, 0x5d681b02L, 0x2a6f2b94L,
 0xb40bbe37L, 0xc30c8ea1L, 0x5a05df1bL, 0x2d02ef8dL
};

/*******************************************************************************
*	Function Prototypes
*******************************************************************************/


/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
u32 make_crc32(u8 *buf,int len)
{
    int i;
    int table_idx;
    u32 curr_crc=0;
    u8 *curr_data = buf;
    
    for( i = 0 ; i < len ; i++ )
    {
			table_idx = ((int)(curr_crc >> 24) ^ *curr_data++) & 0xff;
			curr_crc = (curr_crc << 8) ^ crc32_table[table_idx];
    }

    return curr_crc;
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
u32 make_crc32_with_init_crc(u8 *buf,int len, u32 i_n_crc)
{
    int i;
    int table_idx;
    u32 curr_crc=i_n_crc;
    u8 *curr_data = buf;
    
    for( i = 0 ; i < len ; i++ )
    {
			table_idx = ((int)(curr_crc >> 24) ^ *curr_data++) & 0xff;
			curr_crc = (curr_crc << 8) ^ crc32_table[table_idx];
    }

    return curr_crc;
}

/******************************************************************************/
/**
* \brief        bootloader crc32 for ramdis image and kernel image
* \param        none
* \return       none
* \note
*******************************************************************************/
u32 make_crc32_boot(u8 *buf,int len)
{
    int i;
    int table_idx;
    u32 curr_crc=0xffffffff;
    u8 *curr_data = buf;
    
    for( i = 0 ; i < len ; i++ )
    {
		table_idx = ((int)curr_crc ^ *curr_data++) & 0xff;
		curr_crc = (curr_crc >> 8) ^ (crc32_table[table_idx]);
    }
	curr_crc ^= 0xffffffff;
    return curr_crc;
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
u32	file_get_size(FILE * fp)
{
	u32 length ;

	fseek(fp,0,SEEK_END);
	length = ftell(fp);
	fseek(fp,0,SEEK_SET);

	return length;
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note			1. fp 에 저장된 데이터에 대한 crc 체크
				2. fp 에 저장된 데이터를 buf로 복사
*******************************************************************************/
FILE_ERR file_check_crc(FILE* fp, u8* buf, int size)
{
	int length;
	u32 crc;

	length = file_get_size(fp);

	if((length - 4) != size )
	{
		return eFILE_ERR_SIZE;
	}

	fseek(fp,0,SEEK_SET);
	fread(buf,1,size,fp);
	fread(&crc,1,4,fp);

	if(crc != make_crc32(buf,size))
	{
		return eFILE_ERR_CRC;
	}

	return eFILE_ERR_NO;
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
FILE_ERR file_write_bin(FILE* fp, u8* buf, int size)
{
	u32 crc;

	crc =  make_crc32(buf,size);

	fseek(fp,0,SEEK_SET);
	fwrite(buf,1,size,fp);
	fwrite(&crc,1,4,fp);

	return eFILE_ERR_NO;
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
FILE_ERR file_write_bin_with_intial_crc32(FILE* fp, u8* buf, int size, u32 i_n_crc )
{
	u32 crc;

	crc =  make_crc32_with_init_crc(buf, size, i_n_crc);

	fseek(fp,0,SEEK_SET);
	fwrite(buf,1,size,fp);
	fwrite(&crc,1,4,fp);

	return eFILE_ERR_NO;
}


/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
FILE_ERR file_copy_to_flash(const char* src_file, const char* dest_dir)
{
	char cmd[512];
	
	memset( cmd, 0, 512 );
	
	sprintf( cmd, "cp %s %s", src_file, dest_dir );
	system( cmd );
	system("sync");

    return eFILE_ERR_NO;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    file1을 기준으로 하여 비교한다.
*******************************************************************************/
FILE_ERR file_compare( const s8* i_p_file1, const s8* i_p_file2, bool* o_p_result )
{
    FILE*   p_file1;
    FILE*   p_file2;
    
    u32     n_file1_size;
    u32     n_file2_size;
    u8      p_buf1[1024];
    u8      p_buf2[1024];
    u32     i;
    u32     n_last_size;
    
    *o_p_result = false;
    p_file1 = fopen( i_p_file1, "rb" );
    
    if( p_file1 == NULL )
    {
        printf("open error (%s)\n", i_p_file1);
        return eFILE_ERR_OPEN;
    }
    
    p_file2 = fopen( i_p_file2, "rb" );
    
    if( p_file2 == NULL )
    {
        printf("open error (%s)\n", i_p_file1);
        return eFILE_ERR_OPEN;
    }
    
    n_file1_size = file_get_size( p_file1 );
    n_file2_size = file_get_size( p_file2 );
    
    if( n_file1_size > n_file2_size )
    {
        fclose( p_file1 );
        fclose( p_file2 );
        printf("length error(%d/%d)\n", n_file1_size, n_file2_size);
        return eFILE_ERR_COMPARE;
    }
    
    i = 0;
    while( i < n_file1_size )
    {
        if( i + 1024 < n_file1_size )
        {
            fread( p_buf1, 1, 1024, p_file1 );
            fread( p_buf2, 1, 1024, p_file2 );
            
            if( memcmp( p_buf1, p_buf2, 1024 ) != 0 )
            {
                fclose( p_file1 );
                fclose( p_file2 );
                printf("memcmp error(%d)\n", i);
                *o_p_result = false;
                
                return eFILE_ERR_NO;
            }
            i += 1024;
        }
        else
        {
            n_last_size = n_file1_size - i;
            
            fread( p_buf1, 1, n_last_size, p_file1 );
            fread( p_buf2, 1, n_last_size, p_file2 );
            
            if( memcmp( p_buf1, p_buf2, n_last_size ) != 0 )
            {
                fclose( p_file1 );
                fclose( p_file2 );
                printf("memcmp error(%d)\n", i);
                *o_p_result = false;
                
                return eFILE_ERR_NO;
            }
            
            break;
        }
    }
    fclose( p_file1 );
    fclose( p_file2 );
    
    *o_p_result = true;
    
    return eFILE_ERR_NO;
}


/******************************************************************************/
/**
 * @brief   kernel and ramdisk image valid chk
 * @param   
 * @return  
 * @note    
*******************************************************************************/
VALID_ERR image_file_valid_confirm(const s8* i_p_file_name)
{
	FILE *fp;
	u8	*c_buffer;
	u32 n_length;
	u32 n_save_crc;
	u32 n_gen_crc;
	image_file_header_t *image_t;
	VALID_ERR e_ret = eVALID_NO_ERR;
	
	
	fp = fopen(i_p_file_name,"rb");
	if( fp == NULL)
	{
		e_ret = eVALID_FILE_ERR;
		return e_ret;
	}
	
	n_length = file_get_size(fp);
	c_buffer = (u8*)malloc(n_length);
	fread(c_buffer,1,n_length,fp);
	
	
	image_t = (image_file_header_t*)c_buffer;
	
	// confirm magic key
	if( ntohl(image_t->ih_magic) != IH_MAGIC )
	{
		e_ret = eVALID_MAGIC_KEY_ERR;
		return e_ret;
	}
	
	n_save_crc = image_t->ih_hcrc;
	
	image_t->ih_hcrc = 0;
	
	// confirm header crc[image_file_header_t size is 64(0x40)]
	n_gen_crc = make_crc32_boot((u8*)c_buffer,sizeof(image_file_header_t));
	
	if( ntohl(n_save_crc) != n_gen_crc)
	{
		e_ret = eVALID_HEADER_CRC_ERR;
		return e_ret;
	}
	
	n_gen_crc = make_crc32_boot((u8*)(c_buffer + sizeof(image_file_header_t)),ntohl(image_t->ih_size));
	if( ntohl(image_t->ih_dcrc) != n_gen_crc)
	{
		e_ret = eVALID_DATA_CRC_ERR;
		return e_ret;
	}
	
	fclose(fp);
	free(c_buffer);
	return e_ret;
}

/******************************************************************************/
/**
 * @brief   dtb file valid chk
 * @param   
 * @return  
 * @note    
*******************************************************************************/
VALID_ERR fdt_file_valid_confirm(const s8* i_p_file_name)
{	
	FILE *fp;
	u8	*c_buffer;
	u32 n_length;
	fdt_header_t	*fdt_t;
	VALID_ERR e_ret = eVALID_NO_ERR;
	
	fp = fopen(i_p_file_name,"rb");
	if( fp == NULL)
	{
		e_ret = eVALID_FILE_ERR;
		return e_ret;
	}
	
	n_length = file_get_size(fp);
	c_buffer = (u8*)malloc(n_length);
	fread(c_buffer,1,n_length,fp);
	
	fdt_t = (fdt_header_t*)c_buffer;
	
	if(  ntohl(fdt_t->magic) != FDT_MAGIC )
	{
		e_ret = eVALID_MAGIC_KEY_ERR;
		return e_ret;
	}
	
	return e_ret;

}

/******************************************************************************/
/**
 * @brief   boot file valid chk
 * @param   
 * @return  
 * @note    
*******************************************************************************/
VALID_ERR boot_file_valid_confirm(const s8* i_p_file_name)
{	
	FILE *fp;
	u8	*c_buffer;
	u32 n_length;	
	ivt_header_t	*ivt_t;
	VALID_ERR e_ret = eVALID_NO_ERR;
	
	fp = fopen(i_p_file_name,"rb");
	if( fp == NULL)
	{
		e_ret = eVALID_FILE_ERR;
		return e_ret;
	}
	
	n_length = file_get_size(fp);
	c_buffer = (u8*)malloc(n_length);
	fread(c_buffer,1,n_length,fp);
	
	ivt_t = (ivt_header_t*)c_buffer;
	
	if(  ivt_t->tag != IVT_HEADER_TAG || ivt_t->version != IVT_VERSION)
	{
		e_ret = eVALID_IVT_TAG_ERR;
		return e_ret;
	}

	//20210903. Youngjun Han	
	//TI ROIC only package의 bootloader면, downgrade 불가 패키지로서 실패 처리 해야함

	return e_ret;

}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void file_delete_oldest( const s8* i_p_path )
{
    s8  p_cmd[256];
    s8  p_result[256];
    
    memset( p_result, 0, sizeof(p_result) );
    
    sprintf( p_cmd, "ls -t1 %s | tail -n 1", i_p_path );
    process_get_result( p_cmd, p_result );
    
    if( strlen( p_result ) > 0 )
    {
        sprintf( p_cmd, "rm %s%s", i_p_path, p_result );
        sys_command( p_cmd );
        sys_command("sync");
    }
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u16 file_get_count( const s8* i_p_path, const s8* i_p_name )
{
    s8  p_cmd[256];
    s8  p_result[256];
    
    memset( p_result, 0, sizeof(p_result) );
    
    sprintf( p_cmd, "ls -1 %s%s | wc -l", i_p_path, i_p_name );
    process_get_result( p_cmd, p_result );
    
    return (u16)atoi(p_result);
}
