#ifndef _VW_FILE_H_
#define _VW_FILE_H_

#include "typedef.h"

typedef enum
{
    eFILE_ERR_NO     = 0,
    eFILE_ERR_SIZE,
    eFILE_ERR_CRC,
    eFILE_ERR_SAVE,
    eFILE_ERR_OPEN,
    eFILE_ERR_COMPARE,
} FILE_ERR;


typedef enum
{
	eVALID_NO_ERR     = 0,
    eVALID_FILE_ERR,
    eVALID_MAGIC_KEY_ERR,
    eVALID_HEADER_CRC_ERR,
    eVALID_DATA_CRC_ERR,
    eVALID_IVT_TAG_ERR,

} VALID_ERR;


#define FDT_MAGIC				0xd00dfeed	/* 4: version, 4: total size */

#define IH_MAGIC				0x27051956	/* Image Magic Number		*/
#define IH_NMLEN				32	/* Image Name Length		*/

#define IVT_HEADER_TAG 			0xD1
#define IVT_VERSION 			0x40



typedef struct _fdt_header {
	u32 magic;			 /* magic word FDT_MAGIC */
	u32 totalsize;		 /* total size of DT block */
	u32 off_dt_struct;		 /* offset to structure */
	u32 off_dt_strings;		 /* offset to strings */
	u32 off_mem_rsvmap;		 /* offset to memory reserve map */
	u32 version;		 /* format version */
	u32 last_comp_version;	 /* last compatible version */

	/* version 2 fields below */
	u32 boot_cpuid_phys;	 /* Which physical CPU id we're
					    booting on */
	/* version 3 fields below */
	u32 size_dt_strings;	 /* size of the strings block */

	/* version 17 fields below */
	u32 size_dt_struct;		 /* size of the structure block */
} __attribute__((packed))fdt_header_t;

typedef struct image_file_header {
	u32		ih_magic;	/* Image Header Magic Number	*/
	u32		ih_hcrc;	/* Image Header CRC Checksum	*/
	u32		ih_time;	/* Image Creation Timestamp	*/
	u32		ih_size;	/* Image Data Size		*/
	u32		ih_load;	/* Data	 Load  Address		*/
	u32		ih_ep;		/* Entry Point Address		*/
	u32		ih_dcrc;	/* Image Data CRC Checksum	*/
	u8		ih_os;		/* Operating System		*/
	u8		ih_arch;	/* CPU architecture		*/
	u8		ih_type;	/* Image Type			*/
	u8		ih_comp;	/* Compression Type		*/
	u8		ih_name[IH_NMLEN];	/* Image Name		*/
}  __attribute__((packed))image_file_header_t;

typedef struct {
	u8 	tag;
	u16 length;
	u8 	version;
} __attribute__((packed)) ivt_header_t;


u32 make_crc32(u8 *buf, int len);
u32 make_crc32_with_init_crc(u8 *buf,int len, u32 i_n_crc);
u32 make_crc32_boot(u8 *buf,int len);

u32	file_get_size(FILE * fp);

FILE_ERR file_check_crc(FILE* fp, u8* buf, int size);
FILE_ERR file_write_bin(FILE* fp, u8* buf, int size);
FILE_ERR file_write_bin_with_intial_crc32(FILE* fp, u8* buf, int size, u32 i_n_crc );

FILE_ERR file_copy_to_flash(const s8* src_file, const s8* dest_dir);
FILE_ERR file_compare( const s8* i_p_file1, const s8* i_p_file2, bool* o_p_result );
//FILE_ERR file_get_directory_size( const s8* i_p_file, u32* o_p_dir_size );


 // valid chek
VALID_ERR 				image_file_valid_confirm(const s8* i_p_file_name);
VALID_ERR 				fdt_file_valid_confirm(const s8* i_p_file_name);
VALID_ERR 				boot_file_valid_confirm(const s8* i_p_file_name);

void	file_delete_oldest( const s8* i_p_path );
u16		file_get_count( const s8* i_p_path, const s8* i_p_name );

#endif /* end of _VW_FILE_H_ */

