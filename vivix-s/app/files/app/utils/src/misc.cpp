#include <iostream>
#include <sys/time.h>
#include <errno.h>          // errno
#include <stdio.h>
#include <string.h>
#include <stdarg.h>         // va_start
#include <stdlib.h>         // system()
#include <stdarg.h> 	// va_list
#include <unistd.h>			// getpid()

#include <sys/socket.h>	// socket(), setsocketopt(), connect()
#include <arpa/inet.h>		// inet_ntoa()
#include <netinet/in.h>
#include <fcntl.h>          	// open() O_NDELAY
#include <ctype.h>			// touppe#include <fcntl.h>

#include <asm/unistd.h>
#include <sys/mount.h>
#include <stddef.h>

//#include <sys/syscall.h>
//#include <unistd.h>

#include "misc.h"


using namespace std;

/*******************************************************************************
*	Macros (Inline Functions) Definitions
*******************************************************************************/
#define delete_module(mod, flags) syscall(__NR_delete_module, mod, flags)
#define init_module(mod, len, opts) syscall(__NR_init_module, mod, len, opts)

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
int lprintf(int a_nLevel,int print, const char * format, ...)
{
#if 0
	va_list 		args;
	char			message[1024]={0,};

	//message
	if( print > 0 )
	{
	    va_start(args, format);
    	vsprintf(message, format, args);
    	va_end(args);
    	
    	printf("%s", message);
    }

#else
	int				sock;
	log_packet_t 	*packet;
	va_list 		args;
	struct sockaddr_in	server_addr;	
	char			message[1024]={0,};
	LEN_DIV len;



	sock = socket(PF_INET, SOCK_DGRAM, 0);

	if( sock < 0 )
	{
		printf("log socket error\n");
		return -1;
	}

	server_addr.sin_family		= AF_INET;
	server_addr.sin_port		= htons(6123);
	server_addr.sin_addr.s_addr	=inet_addr("127.0.0.1");


	packet = (log_packet_t *)malloc(sizeof(log_packet_t));

	if(packet != NULL)
	{
		memset(packet,0,sizeof(log_packet_t));
		//head
		strncpy(packet->head,"VWLD",4);

		//type
		packet->type = 0x01 ; // PAC_LOG_MESSAGE;

		//level
		packet->level = a_nLevel;

		//time or ip or config
		packet->time = time(NULL);


		//message
		va_start(args, format);
		vsprintf(message, format, args);
		va_end(args);
		sprintf(packet->data,"%s", message);

		//length or port
		len.LEN = 0x00;
		len.BIT.message_length = strlen(message); 
		len.BIT.tag_length = 0;
		packet->length = len.LEN;


		if(print == 1)
		{
			printf("%s",packet->data);
		}
		sendto(sock,
				(void *)packet, 
				sizeof(log_packet_t), 
				0, 
				(struct sockaddr *)&server_addr,
				sizeof(server_addr));

		free(packet);
	}
	close(sock);
#endif
	return 1;
}
/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void memory_dump( u8 * buf, int len )
{
	u8		i, j;
	u8		rows;
	u16		count;
	short	data[16];

	rows	= (len + 15) / 16;
	count	= 0;

	lprintf(DEBUG, 1, "\r\n");
	lprintf(DEBUG, 1, "  00 01 02 03 04 05 06 07  08 09 0A 0B 0C 0D 0E 0F        ASCII\r\n");
	lprintf(DEBUG, 1, "  -------------------------------------------------------------------\r\n");

	for(i = 0; i < rows; i++)
	{
		for(j = 0; j < 16; j++)
		{
			if(count < len)
			{
				data[j]	= (u8 )buf[count];
				lprintf(DEBUG, 1, "%s%02X", (j & 0x07) ? " " : "  ", data[j]);
			}
			else
			{
				data[j]	= -1;
				lprintf(DEBUG, 1, "%s  ", (j & 0x07) ? " " : "  ");
			}
			count++;
		}
		lprintf(DEBUG, 1, " ");
		for(j = 0; j < 16; j++)
		{
			if(data[j] >= 0)
			{
				lprintf(DEBUG, 1, "%s%c", (j & 0x07) ? "" : " ", isgraph((u8)data[j]) ? (u8)data[j] : '.');
			}
			else
			{
				lprintf(DEBUG, 1, "%s%c", (j & 0x07) ? "" : " ", ' ');
			}
		}
		lprintf(DEBUG, 1, "\r\n");fflush(stdout);
	}
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void sys_command(const s8* format, ...)
{
	s8      p_cmd[512];
	va_list args;

	va_start( args, format );
	vsprintf( p_cmd, format, args );
	va_end( args );
	
	//printf( "%s\n", p_cmd );
	//fflush( stdout );
	system( p_cmd );
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
s32 process_get_result( const s8* i_p_cmd, s8* o_p_output )
{
    s32     n_length=0;
    FILE*   p_fp = NULL;
    s32     n_err = 0;
    
    //fflush(stdout);//fflush(NULL);
    
    if( (p_fp = popen( i_p_cmd, "r" )) == NULL )
    {
        return -1;
    }
    
    while( feof(p_fp) == 0 ) 
    {
        fgets( o_p_output, 511, p_fp );		
        n_length = strlen(o_p_output);
        
        if(ferror(p_fp))
        {
            n_err = errno;
            if(n_err == EINTR)
            {
                continue;
            }
            else
            {
                pclose( p_fp );
                return -1;
            }
        }
    }
    
    pclose( p_fp );
    
    return n_length;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void upper_str(s8 *str)
{
	while(*str) 
	{ 
		*str = toupper(*str); 
		str++; 
	}
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void lower_str(s8 *str)
{
	while(*str) 
	{ 
		*str = tolower(*str); 
		str++; 
	}
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool get_time_string( u32 n_i_time, s8* o_p_string )
{
	struct tm*  p_gmt_tm_t;
	
	p_gmt_tm_t = localtime( (time_t*)&n_i_time );
	
	sprintf( o_p_string, "%d.%d.%d-%d:%d:%d", \
	            p_gmt_tm_t->tm_year+1900, p_gmt_tm_t->tm_mon+1, p_gmt_tm_t->tm_mday,	\
                p_gmt_tm_t->tm_hour, p_gmt_tm_t->tm_min, p_gmt_tm_t->tm_sec );

    return true;
}


/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u32 get_shell_cmd_output(s8* i_p_cmd_buf,s32 i_n_output_buf_size, s8* o_p_output_buf)
{
	FILE *fp;
	fp = popen(i_p_cmd_buf,"r");
    if( fp == NULL)
    {
     	return -1;
    }
    if(fgets(o_p_output_buf,i_n_output_buf_size,fp)== NULL)
	{
		return -1;
	}
	pclose(fp);
	
	return 0;
}

/******************************************************************************/
/**
 * @brief   insmod error description
 * @param   
 * @return  
 * @note    insmod 명령의 에러 설명. 
*******************************************************************************/
static const char *moderror(int err)
{
	switch (err) {
	case ENOEXEC:
		return "Invalid module format";
	case ENOENT:
		return "Unknown symbol in module";
	case ESRCH:
		return "Module has wrong symbol version";
	case EINVAL:
		return "Invalid parameters";
	default:
		return strerror(err);
	}
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
static void *insmod_grab_file(const char *i_p_filename, unsigned long *i_n_size)
{
	unsigned int n_max = 16384;
	int n_ret, n_fd, n_err_save;
	void *p_buffer;
	
	n_fd = open(i_p_filename, O_RDONLY, 0);

	if (n_fd < 0)
		return NULL;

	p_buffer = malloc(n_max);
	if (!p_buffer)
		goto ERR;

	*i_n_size = 0;
	while ((n_ret = read(n_fd, (void*)((intptr_t)p_buffer + *i_n_size), n_max - *i_n_size)) > 0) {
		*i_n_size += n_ret;
		if (*i_n_size == n_max) {
			void *p_temp;

			p_temp = realloc(p_buffer, n_max *= 2);
			if (!p_temp)
				goto ERR;
			p_buffer = p_temp;
		}
	}
	if (n_ret < 0)
		goto ERR;

	close(n_fd);
	return p_buffer;

ERR:
	n_err_save = errno;
	free(p_buffer);
	close(n_fd);
	errno = n_err_save;
	return NULL;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
int insmod(const char* i_p_filename)
{
	unsigned long		n_len;
	int							n_ret = 0 ;
	void*						p_file = NULL;
	char*						p_options = strdup("");
	
	p_file = insmod_grab_file(i_p_filename, &n_len);
	if (!p_file) 
	{
		fprintf(stderr, "insmod: can't read '%s': %s\n",i_p_filename, strerror(errno));
		return -1;
	}

	n_ret = init_module(p_file, n_len, p_options);
	if (n_ret != 0) 
	{
		fprintf(stderr, "insmod: error inserting '%s': %d %s\n",	i_p_filename, n_ret, moderror(errno));
	}
	
	safe_free(p_file);
	
	return n_ret;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
int rmmod(const char* i_p_filename)
{
	int							n_ret = 0 ;
	
	n_ret = delete_module(i_p_filename, O_NONBLOCK);
	if (n_ret != 0) {
		fprintf(stderr, "rmsmod: error inserting '%s': %d %s\n",	i_p_filename, n_ret, moderror(errno));
	}
	return n_ret;  
}
