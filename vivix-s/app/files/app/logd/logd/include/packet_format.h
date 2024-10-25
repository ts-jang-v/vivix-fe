#ifndef __PACKET_FORMAT__
#define __PACKET_FORMAT__

/***************************************************************************************************
  * include Files																			
***************************************************************************************************/
#include <netinet/in.h>
#include <string.h>
/***************************************************************************************************
  *	Constant Definitions																			
  ***************************************************************************************************/
	
/***************************************************************************************************
  *	Type Definitions																			
  ***************************************************************************************************/
enum PACKET_TYPE{
	PAC_LOG_MESSAGE = 0x01,
	PAC_LOG_REQ		= 0x02,
	PAC_CHANGE_CONF = 0xff
};

typedef union{
	union{
		unsigned short DATA;
		struct{
			unsigned short  message_length	: 10;
			unsigned short	tag_length 		: 6;
		}BIT;
	}LEN_DIV;
}PACKET;
/***************************************************************************************************
  *	Macros (Inline Functions) Definitions														
  ***************************************************************************************************/


/***************************************************************************************************
  *	Variable Definitions																			
  ***************************************************************************************************/

class Packet_format
{
	private:
		char header[4];
		unsigned char type;
		unsigned char level;
		short tag_length;
		short meg_length;
		unsigned short port;
		int time;
		in_addr_t ip;
		int config;
		char tag[63];
		char data[1023];
	public:
		Packet_format()
		{
			memset(header,0,4);
			type = 0;
			level = 0;
			tag_length = 0;
			meg_length = 0;
			port = 0;
			time  = 0;
			ip = 0;
			config  = 0;
			memset(tag,0,63);
			memset(data,0,1023);
		}
		bool parse_string(char *stream);
		in_addr_t	get_ip()			{ return ip; }
		int		get_config()		{ return config; }
		char* 	get_header() 		{ return header; }
		unsigned char 	get_type()			{ return type; }
		unsigned char	get_level()			{ return level; }
		short 	get_tag_length()	{ return tag_length; }
		short 	get_meg_length()	{ return meg_length; }
		unsigned short 	get_port()			{ return port; }
		int 	get_time()			{ return time; }
		char*	get_tag()			{ return tag; }
		char*	get_data()			{ return data; }
		
		
};

#endif

