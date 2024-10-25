#ifndef _SYNC_H
#define _SYNC_H

extern "C" {

#include "typedef.h"

#define SYNC_PORT									5010

typedef enum 
{
	SYNC_COMMAND_DISCOVERY_CMD =	0x0002,
	SYNC_COMMAND_DISCOVERY_ACK =	0x0003,
} sync_command_e;

typedef struct 
{
	u16 command;
	u16 size;
	u16 count;
}  __attribute__((__packed__)) sync_header_t;

typedef struct 
{
	sync_header_t header;
	u8 		*data;
} __attribute__((__packed__))  sync_packet_t;


typedef struct 
{
	u16 status;
	u16 answer;
	u16 length;
	u16 ack_id;
	
	u8	ssid[32];
	u8	key[64];
	u8	mac[32];
	
	u8  reserved[144];
	
} __attribute__((__packed__)) sync_ack_t;

}

#endif /* end of _SYNC_H */
