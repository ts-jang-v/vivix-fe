#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include "packet_format.h"

void remove_return(char* p)
{
	if(*p == '\n' || *p == '\r')
		*p = 0;
}


/***********************************************************************************************//**
* \brief	save packet info
* \param
* \return	TRUE : vaild packet, FALSE : invalid packet
* \note
***************************************************************************************************/

bool Packet_format::parse_string(char *stream)
{
	bool ret = true;
	char *pTemp = stream;
	PACKET pac;
	memcpy(header,pTemp,4);
	pTemp += 4;

	if(strncmp(header,"VWLD",4) != 0)
	{
		printf("[PACKET_FORMAT] header is %s\n",header);
		ret = false;
		goto END;
	}


	type = *(char*)pTemp++;
	level = *(char*)pTemp++;

	if(type == PAC_LOG_REQ)
	{
		port = *(unsigned short*)pTemp;
	}
	else
	{
		pac.LEN_DIV.DATA = (*(unsigned short*)pTemp);
		tag_length = pac.LEN_DIV.BIT.tag_length;
		meg_length = pac.LEN_DIV.BIT.message_length;
	}
	

	pTemp += 2;
	
	if(type == PAC_LOG_REQ)
	{
		ip = *(in_addr_t*)pTemp;
	}
	else if(type == PAC_LOG_MESSAGE)
	{
		time = *(int*)pTemp;
	}
	else if(type == PAC_CHANGE_CONF)
	{
		config = *(int*)pTemp;
	}


	pTemp += 4;

	if(tag_length > 0 && (type == PAC_LOG_MESSAGE|| type == PAC_CHANGE_CONF))
	{
		memset(tag,0,63);
		memcpy(tag,pTemp,tag_length);
		remove_return(&tag[tag_length-2]);
		remove_return(&tag[tag_length-1]);
		pTemp += tag_length;
	}

	if(type == PAC_LOG_MESSAGE || type == PAC_CHANGE_CONF)
	{
		memset(data,0,1023);
		memcpy(data,pTemp,meg_length);
		remove_return(&data[meg_length-2]);
		remove_return(&data[meg_length-1]);
	}


END:
	return ret;
}
