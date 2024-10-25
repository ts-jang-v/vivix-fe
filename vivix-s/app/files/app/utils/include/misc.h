#ifndef _VW_MISC_H_
#define _VW_MISC_H_

#include "typedef.h"

enum PACKET_TYPE{
	PAC_LOG_MESSAGE = 0x01,
	PAC_LOG_REQ		= 0x02,
	PAC_CHANGE_CONF = 0xff
};


typedef struct LOG_PACKET
{
	char     	head[4];
	unsigned char		type;
	unsigned char		level;
	unsigned short		length;
	unsigned int		time;
	char	data[1088];
}__attribute__((packed)) log_packet_t;


typedef union{
	unsigned short LEN;
	struct{
		unsigned short  message_length	: 10;
		unsigned short	tag_length 		: 6;
	}BIT;
}LEN_DIV;


int lprintf(int a_nLevel,int print, const char * format, ...);
void memory_dump( u8 * buf, int len );

void sys_command(const s8* format, ...);

s32 process_get_result( const s8* i_p_cmd, s8* o_p_output);

void upper_str(s8 *str);
void lower_str(s8 *str);

bool get_time_string( u32 n_i_time, s8* o_p_string );
u32 get_shell_cmd_output(s8* i_p_cmd_buf,s32 i_n_output_buf_size, s8* o_p_output_buf);

int insmod(const char* i_p_filename);
int rmmod(const char* i_p_filename);


#endif /* end of _VW_MISC_H_ */

