#ifndef __LOGD__
#define __LOGD__




/***************************************************************************************************
  * include Files																			
***************************************************************************************************/
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "NetworkUDP.h"
#include "NetworkTCPServer.h"
#include "packet_format.h"
#include "log_conf.h"
/***************************************************************************************************
  *	Constant Definitions																			
  ***************************************************************************************************/
#define LOGD_PAC_BUF_SIZE	10
#define LOGD_SOC_LIST_SIZE	10

#define CLIENT_BUFF_SIZE		(50 * 1024)

#define PACKET_DATA_SIZE        (1420)
#define PACKET_HEAD             ("VWLG")
#define PACKET_SIZE      		(PACKET_DATA_SIZE + sizeof(packet_header_t))

#define SYS_LOG_PATH           "/mnt/mmc/p3/"
//#define SYS_MOUNT_MMCBLK_NUM	"3"
#define SYS_MOUNT_MMCBLK_NUM	"2"
#define SYS_MOUNT_MMCBLK_P2		"/dev/mmcblk" SYS_MOUNT_MMCBLK_NUM "p2"
#define SYS_MOUNT_MMCBLK_P3		"/dev/mmcblk" SYS_MOUNT_MMCBLK_NUM "p3"

/***************************************************************************************************
  *	Type Definitions																			
  ***************************************************************************************************/
typedef struct 
{	
	unsigned char              	p_packet_buf[CLIENT_BUFF_SIZE];
	int	                    	n_buf_end_pos;	
}client_info_t;

typedef struct
{
	unsigned char 	head[4];
	unsigned int	seq_id;
	unsigned char	option[4];
	unsigned int	payload;
	unsigned short	slice_num;
	unsigned short	slice_total;
	unsigned char	payload_opt[8];
}__attribute__((packed)) packet_header_t;

typedef enum {
	PAYLOAD_REQ_LOG_LIST = 0x01,
	PAYLOAD_REQ_LOG,
	PAYLOAD_BYE,		
}ePAYLOAD;

/***************************************************************************************************
  *	Macros (Inline Functions) Definitions														
  ***************************************************************************************************/
#define		safe_delete(p)		if(p!=NULL) {free(p);p=NULL;}

/***************************************************************************************************
  *	Variable Definitions																			
  ***************************************************************************************************/
class Client_info
{
	private:
		struct sockaddr_in 	server_addr;
		int 				soc;
		bool				sel_flag;
	public:
		bool 				get_sel_flag() { return sel_flag; }
		struct sockaddr_in* get_server_addr() { return &server_addr; }
		in_addr_t			get_ip() { return server_addr.sin_addr.s_addr; }
		Client_info()
		{ 
			server_addr.sin_family	= 0;
			server_addr.sin_port	= 0;
			server_addr.sin_addr.s_addr = 0;
			soc = 0;
			sel_flag = false;

		}
		unsigned short 		get_port() { return server_addr.sin_port; }
		int 				get_soc() { return soc; }	

		void 				set_sel_flag(bool sel) { sel_flag = sel; }
		void 				set_server_addr(short sin_family, unsigned short port, in_addr_t ip)
		{
			server_addr.sin_family	= sin_family;
			server_addr.sin_port	= port;
			server_addr.sin_addr.s_addr = ip;
		}
		void				set_soc(int _soc) { soc = _soc;}

};

class LOGD
{
	private:
		Log_conf	config;
		char 		packet_data[1024 + 12];
//		int count;
		Client_info cInfo[10];
		
		CNetworkUDP*		m_p_udp_c;
		CNetworkTCPServer*	m_p_tcp_c;
		client_info_t*		m_p_client_t;
		int*				m_n_client_seq_id;
		
		char				m_old_filename[60];
	public:

		LOGD();
#if 0
		void save_packet(char *stream,int length)
		{
			memcpy(packet_data,stream,length);
			count++;
		}
#endif		
		void		check_location(const char* location);
		void		check_capacity(void);
		void 		proc_logd(char *stream);
		void 		get_file_name_format(time_t time, char *str);

		void 		get_time_string(int uval,char * date) ;
		FILE* 		get_file_open(const char *filename, const char *op);
		Log_conf* 	get_config() { return &config; }
		int			get_file_size(char* filename);
		int			get_dir_size();
		int			old_file_del();
		
		bool		initialize(int i_n_port, int i_n_max_client); 
		bool		start();
		bool		stop();
		
		bool			load_file(char* i_p_filename, int i_n_size, unsigned char** o_p_buf);
		unsigned int	get_log_count();
		bool			make_log_list(unsigned char** o_p_buf, unsigned int i_n_log_count);
		time_t			get_file_name_to_time(char * i_p_filename);
		
		//tcp				
		bool 		tcp_proc(int i_n_index, char* i_p_stream, int i_n_len);
		bool		tcp_send(int i_n_index, ePAYLOAD i_n_payload, unsigned char* i_p_buff, unsigned int i_n_length, unsigned char* i_p_option, unsigned int i_n_opt_len);
		bool		tcp_packet_parse(int i_n_index,   unsigned char* i_p_data );

};










#endif
