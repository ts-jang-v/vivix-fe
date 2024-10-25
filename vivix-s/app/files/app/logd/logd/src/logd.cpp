#include <sys/stat.h>
#include <time.h>
#include <dirent.h>
#include "logd.h"

#define DEFAULT_LEVEL 3
#define DEFAULT_LOCATION "/mnt/mmc/p3/log/"
#define DEFAULT_CAPACITY 300

/******************************************************************************
* \brief        sOnConnect
* \param        
* \return       
* \note
*******************************************************************************/
static void sOnConnect(void* a_pThis,int a_nIndex, int a_nCurClientCnt)
{
	a_pThis = a_pThis; // WARNING 제거
	printf("[LOGD(tcp)]  %s, a_nIndex :% d , a_nCurClientCnt: %d\n",__FUNCTION__, a_nIndex, a_nCurClientCnt);
}

/******************************************************************************
* \brief        sOnDisconnect
* \param        
* \return       
* \note
*******************************************************************************/
static void sOnDisconnect(void* a_pThis, int a_nIndex, int a_nCurClientCnt)
{
	a_pThis = a_pThis; // WARNING 제거
	printf("[LOGD(tcp)]  %s, a_nIndex :% d , a_nCurClientCnt: %d\n", __FUNCTION__, a_nIndex, a_nCurClientCnt);
}

/******************************************************************************
* \brief        sOnStatus
* \param        
* \return       
* \note
*******************************************************************************/
static void sOnStatus(void* a_pThis, int a_nStatus)
{
	a_pThis = a_pThis; // WARNING 제거
	printf("[LOGD(tcp)]  %s, a_nStatus:%d\n",__FUNCTION__, a_nStatus);
}

/******************************************************************************
* \brief        sOnError
* \param        
* \return       
* \note
*******************************************************************************/
static void sOnError(void* a_pThis, int a_nServerErrno, int a_nSystemErrono)
{
	a_pThis = a_pThis; // WARNING 제거
	printf("[LOGD(tcp)]  %s, a_nServerErrno: %d, a_nSystemErrono: %d\n", __FUNCTION__, a_nServerErrno, a_nSystemErrono);
}

/******************************************************************************
* \brief        udp receive procedure
* \param        
* \return       
* \note
*******************************************************************************/
static bool sOnUDPProc(void* a_pThis, int a_nIndex, volatile unsigned char * a_sData, int a_nLength)
{
	a_pThis = a_pThis;
	a_nIndex = a_nIndex;
	a_sData = a_sData;
	a_nLength = a_nLength;	
	LOGD* p_logd_c = (LOGD*) a_pThis;

	p_logd_c->proc_logd((char*)a_sData);
	
	return true;
}

/******************************************************************************
* \brief        tcp receive procedure
* \param        
* \return       
* \note
*******************************************************************************/
static bool sOnTCPProc(void* a_pThis, int a_nIndex, volatile unsigned char * a_sData, int a_nLength)
{
	a_pThis = a_pThis;
	a_nIndex = a_nIndex;
	a_sData = a_sData;
	a_nLength = a_nLength;	
	LOGD* p_logd_c = (LOGD*) a_pThis;
	
	return p_logd_c->tcp_proc(a_nIndex,(char*)a_sData, a_nLength);
}

/***********************************************************************************************//**
* \brief	initailize		
* \param
* \return	
* \note
***************************************************************************************************/
bool LOGD::initialize(int i_n_port, int i_n_max_client) 
{
	get_file_name_format(time(NULL),m_old_filename);
	
	m_p_udp_c = new CNetworkUDP();
	m_p_tcp_c = new CNetworkTCPServer();

	if(m_p_udp_c->Init(this,NULL,i_n_port,eTYPE_SERVER,NULL,sOnUDPProc,NULL) == 0)
	{
		printf("[LOGD] udp init failed\n");
		return false;
	}
	
	//if(m_p_tcp_c->Init(this, i_n_port + 1, i_n_max_client, NULL, NULL, NULL, sOnTCPProc, NULL) == 0)
	if(m_p_tcp_c->Init(this, i_n_port + 1, i_n_max_client, sOnStatus, sOnConnect, sOnDisconnect, sOnTCPProc, sOnError) == 0)
	{
		printf("[LOGD] tcp init failed\n");
		return false;
	}
	
	m_n_client_seq_id = new int[i_n_max_client];
	memset(m_n_client_seq_id, 0x00, i_n_max_client);
	
	m_p_client_t = new client_info_t[i_n_max_client];
	memset(m_p_client_t, 0x00, i_n_max_client);
	
	return true;
}
/***********************************************************************************************//**
* \brief	start		
* \param
* \return	
* \note
***************************************************************************************************/
bool LOGD::start() 
{	
	if(m_p_udp_c->Start() == 0 )
	{
		printf("[LOGD] udp start failed\n");	
		return false;		
	}
	
	if(m_p_tcp_c->Start() == 0 )
	{
		printf("[LOGD] tcp start failed\n");	
		return false;		
	}
	return true;
}

/***********************************************************************************************//**
* \brief	start		
* \param
* \return	
* \note
***************************************************************************************************/
bool LOGD::stop() 
{	
	if(m_p_udp_c->Stop() == 0 )
	{
		printf("[LOGD] udp stop failed\n");		
		return false;		
	}
	
	if(m_p_tcp_c->Stop() == 0 )
	{
		printf("[LOGD] tcp stop failed\n");		
		return false;		
	}
	
	safe_delete(m_p_udp_c);
	safe_delete(m_p_tcp_c)
	safe_delete(m_p_client_t);
	safe_delete(m_n_client_seq_id);
	return true;
}

/***********************************************************************************************//**
* \brief	time to string		
* \param
* \return	
* \note
***************************************************************************************************/
void LOGD::get_time_string(int uval,char * date) 
{
	time_t timeval;
	struct tm tminfo;
	
	timeval = uval;
	tminfo = *localtime(&timeval);
	memset(date,0,30);
//	sprintf(date, "%04d/%02d/%02d_%02d:%02d:%02d",
//			1900 + tminfo.tm_year, tminfo.tm_mon + 1, tminfo.tm_mday,
//			tminfo.tm_hour, tminfo.tm_min, tminfo.tm_sec);
	sprintf(date, "%02d:%02d:%02d",tminfo.tm_hour, tminfo.tm_min, tminfo.tm_sec);
}


/***********************************************************************************************//**
* \brief	filename 		
* \param
* \return	
* \note
***************************************************************************************************/
void LOGD::get_file_name_format(time_t time, char *str)
{
	struct tm		tm_time;
	
	tm_time = *localtime(&time);
	if(strlen(config.get_filename()) > 0)
	{
		sprintf(str, "%s%s_%04d_%02d_%02d.log",config.get_location(),config.get_filename(),
				1900 + tm_time.tm_year, tm_time.tm_mon + 1, tm_time.tm_mday);	
	}
	else	
	{
		sprintf(str, "%s%04d_%02d_%02d.log",config.get_location(),
				1900 + tm_time.tm_year, tm_time.tm_mon + 1, tm_time.tm_mday);	
	}

	//printf("time %d,%s\n",time,str);
}


/***********************************************************************************************//**
* \brief	fileopen		
* \param
* \return	
* \note
***************************************************************************************************/
FILE* LOGD::get_file_open(const char *filename, const char *op)
{
	FILE *fp;
	if( (fp = fopen(filename,op)) == NULL)
	{
		printf("[LOGD] file open failed : %s\n",filename);
		return NULL;
	}
	return fp;
}

/***********************************************************************************************//**
* \brief	location directory check		
* \param
* \return	
* \note
***************************************************************************************************/
void LOGD::check_location(const char* location)
{
	struct stat filestat;
	char cmd[128];

	if( stat(location,&filestat) != 0) 
	{
		printf("[LOGD] log error : not found location\n");
		sprintf(cmd,"mkdir -p %s",location);
		system(cmd);
	}
}

/***********************************************************************************************//**
* \brief	directory capacity check		
* \param
* \return	
* \note
***************************************************************************************************/
void LOGD::check_capacity(void)
{
	while(get_dir_size() > (config.get_capacity()*1024*1024))
	{
		if(old_file_del())	break;
	}
}

/***********************************************************************************************//**
* \brief	init and read configuration	
* \param
* \return	
* \note
***************************************************************************************************/
LOGD::LOGD()
{
	FILE *fp;
	char buffer[60];

	fp = get_file_open("./logd.conf","r");
	if(fp == 0) 
	{
		config.make_default_config(DEFAULT_LEVEL, (char*)DEFAULT_LOCATION, (char*)NULL, DEFAULT_CAPACITY);
		fp = get_file_open("./logd.conf","r");
	}

	while(feof(fp) == 0)
	{

		char *token;
		char log_name[60];
		char log_value[60];


		if(fgets(buffer,60,fp) == NULL) break;

		token = strtok(buffer," \t\n");
		if(token)
			strcpy(log_name,token);

		token = strtok(NULL," \t\n");

		if(token)
			strcpy(log_value,token);

		config.set_value(log_name,log_value);
		
	}
	fclose(fp);

#if 0
	while(get_dir_size() > (config.get_capacity()*1024*1024))
	{
		if(old_file_del())	break;
	}
#endif
	
	return;
}


/***********************************************************************************************//**
* \brief	get file size
* \param
* \return	
* \note
***************************************************************************************************/
int LOGD::get_file_size(char *filename)
{
	struct stat stat1;

	if( stat(filename,&stat1) != 0) 
	{
		printf("[LOGD] log error : not found file\n");
		return 0;
	}
	else
	{
		return stat1.st_size;
	}
	return 0;
}
/***********************************************************************************************//**
* \brief	get dir size
* \param
* \return	
* \note
***************************************************************************************************/
int LOGD::get_dir_size()
{
	DIR *dir_info;
	struct dirent *dir_entry;
	int total_size = 0;
	
	dir_info = opendir(config.get_location());
	if(NULL != dir_info)
	{
		while((dir_entry = readdir(dir_info)))
		{
			//if(strstr(dir_entry->d_name,config.get_filename()) != 0)
			if(dir_entry->d_type == DT_REG)
			{
				char filename[60];
				sprintf(filename,"%s%s",config.get_location(),dir_entry->d_name);
				total_size += get_file_size(filename);
			}
		}
		closedir(dir_info);
	}
	else
	{
		printf("[LOGD] dir failed\n");
	}
	return total_size;
}

/***********************************************************************************************//**
* \brief	old file del
* \param
* \return	
* \note
***************************************************************************************************/
int LOGD::old_file_del()
{
	DIR *dir_info;
	
	struct dirent *dir_entry;
	struct dirent *dir_old = 0;
	int file_cnt = 0;

	dir_info = opendir(config.get_location());
	if(NULL != dir_info)
	{
		char sys_cmd[60];
		while((dir_entry = readdir(dir_info)))
		{
			if(dir_old ==0)
			{
			//	if(strstr(dir_entry->d_name,config.get_filename()) != 0)
				if(dir_entry->d_type == DT_REG)
				{	
					dir_old = dir_entry;
					file_cnt++;
				}
			}
			else
			{
			//	if(strstr(dir_entry->d_name,config.get_filename()) != 0)
				if(dir_entry->d_type == DT_REG)
				{
					file_cnt++;
					if(strcmp(dir_old->d_name,dir_entry->d_name) > 0 )
					{
						dir_old = dir_entry;
					}
				}
			}
		}
		if(file_cnt > 1)
		{
			sprintf(sys_cmd,"rm -rf %s%s",config.get_location(),dir_old->d_name);
			printf("[LOGD] delete log : %s%s\n", config.get_location(), dir_old->d_name);
			system(sys_cmd);
		}
		else
		{
			return 1;
		}

		closedir(dir_info);
	}
	return 0;
}


/***********************************************************************************************//**
* \brief	log processing
* \param
* \return	
* \note
***************************************************************************************************/
void LOGD::proc_logd(char* stream)
{
	Packet_format packet;
	FILE *fp;
	
    packet.parse_string(stream);
    
    switch(packet.get_type())
    {
    	case PAC_LOG_MESSAGE:
    	{
    	    char filename[60];
    		char time_string[30];
    		int i;
    
    		// log print to file
    		get_file_name_format(packet.get_time(),filename);
    		
    		if( strcmp( filename, m_old_filename ) != 0 )
    		{
    			check_capacity();
			}
			
			strcpy( m_old_filename, filename );

    		fp = get_file_open(filename,"a");
    		if(fp == NULL) goto END;
    		get_time_string(packet.get_time(),time_string);
    	
    		fprintf(fp,"[%s][%d]%s\n",time_string,packet.get_level(),
    										packet.get_data());
    		fflush(fp);
    		fclose(fp);
    		//system("sync");
    		
    		// log sendto
    		for(i = 0 ; i < LOGD_SOC_LIST_SIZE ; i++)
    		{
    			if(cInfo[i].get_sel_flag() == true)
    			{
    				sendto(cInfo[i].get_soc(),
    						(void*)stream,
    						packet.get_meg_length() + packet.get_tag_length() +12 ,
    						0,
    						(struct sockaddr *)cInfo[i].get_server_addr(),
    						sizeof(struct sockaddr));
    			}
    		}
    
    	    break;
        }
    	case PAC_LOG_REQ:
    	{	
    		if(!packet.get_level())
    		{
    			int i;
    			
    			for(i = 0; i < LOGD_SOC_LIST_SIZE ; i++)
    			{
    				if(cInfo[i].get_sel_flag() == true)
    				{
    					if(packet.get_ip() == cInfo[i].get_ip() && packet.get_port() == cInfo[i].get_port())
    					{
    						goto END;
    					}
    				}
    			}
    			
    			for(i = 0; i < LOGD_SOC_LIST_SIZE ; i++)
    			{
    				if(cInfo[i].get_sel_flag() == false)
    				{
    				    break;
    				}
    			}
    			
    			if(i == LOGD_SOC_LIST_SIZE)
    			{
    				printf("[LOGD] CLIENT socket is FULL\n");
    			}
    			else
    			{
    				cInfo[i].set_sel_flag(true);
    				cInfo[i].set_soc(socket(AF_INET,SOCK_DGRAM,0));
    				cInfo[i].set_server_addr(AF_INET,packet.get_port(),packet.get_ip());
    				
    				printf("[LOGD] IP : %d.%d.%d.%d, PORT: %u is connected\n", \
    				            packet.get_ip()&0xff, (packet.get_ip()>>8)&0xff, (packet.get_ip()>>16)&0xff, \
    						    packet.get_ip()>>24, ntohs(packet.get_port()));
    			}
    		}
    		else
    		{
    			int i;
    
    			for(i = 0; i < LOGD_SOC_LIST_SIZE ;i++)
    			{
    				if(packet.get_ip() == cInfo[i].get_ip() && packet.get_port() == cInfo[i].get_port())
    				{
    					cInfo[i].set_sel_flag(false);
    					
    					printf("[LOGD] IP : %d.%d.%d.%d, PORT: %u is disconnected\n", \
    					        packet.get_ip()&0xff, (packet.get_ip()>>8)&0xff, (packet.get_ip()>>16)&0xff, \
                                packet.get_ip()>>24, ntohs(packet.get_port()));
    					
    					break;
    				}
    			}
    			if(i == LOGD_SOC_LIST_SIZE)
    			{
    				printf("[LOGD] resend off request error : not found IP or Port\n");
    			}
    		}
            break;
        }
    	case PAC_CHANGE_CONF:
    	{
    	    fp = get_file_open("./logd.conf","w");
    		
    		if(fp == NULL) goto END;
    	    
    	    config.set_value(packet.get_config(),packet.get_data());
            config.make_default_config(config.get_level(), config.get_location(), config.get_filename(), config.get_capacity());
    		
    		fclose(fp);
            
            break;
    	}
    	default:
    	{
    	    printf("[LOGD] invaild packet type : 0x%.2x\n",packet.get_type());
    	    break;
    	}
    }
    
END:
	return;
}

/***********************************************************************************************//**
* \brief	get file name to time
* \param
* \return	
* \note
***************************************************************************************************/
time_t LOGD::get_file_name_to_time(char * i_p_filename)
{
	char 			*p_name;
	char			*p_date_str;
	int				p_date[3];
	int				n_index=0;
	struct tm		tm_time_t;
	time_t			cur_time_t;

	p_name = strtok(i_p_filename,".");

	if(p_name != NULL)
	{
		p_date_str = strtok(p_name,"_");

		while(p_date_str != NULL)
		{
			if(p_date_str == NULL) break;
			p_date[n_index++] = atoi(p_date_str);
			p_date_str = strtok(NULL,"_");
			if(n_index >= 3) break;
		}
	}
	if(n_index != 3) return 0;

	cur_time_t = time(NULL);	
	
	tm_time_t = *localtime(&cur_time_t);
	tm_time_t.tm_year = p_date[0] - 1900;
	tm_time_t.tm_mon = p_date[1] - 1;
	tm_time_t.tm_mday = p_date[2] ;
	tm_time_t.tm_sec = 0;
	tm_time_t.tm_min = 0;
	tm_time_t.tm_hour = 0;

	return mktime(&tm_time_t);
}

bool LOGD::tcp_send(int i_n_index, ePAYLOAD i_n_payload, unsigned char* i_p_buff, unsigned int i_n_length, unsigned char* i_p_option, unsigned int i_n_opt_len)
{
	unsigned char       p_send_buf[PACKET_SIZE];
    packet_header_t*    p_hdr_t;    
    unsigned int		n_send_size = 0;
    unsigned int		n_index = 0;
    unsigned int		n_total_slice = 0;
    
    //printf("[LOGD(tcp)] packet_send: payload(0x%04X), data size(%d)\n", i_n_payload, i_n_length);                                        
        
    n_total_slice = i_n_length / PACKET_DATA_SIZE + ((i_n_length % PACKET_DATA_SIZE) > 0 ? 1 : 0);
   	if(n_total_slice == 0)
    	n_total_slice = 1;
        
    memset( p_send_buf, 0, sizeof(p_send_buf) );
    p_hdr_t = (packet_header_t *)p_send_buf;
    
    memcpy( p_hdr_t->head, PACKET_HEAD, sizeof(p_hdr_t->head) );    
    p_hdr_t->seq_id         = htonl( m_n_client_seq_id[i_n_index] );
    p_hdr_t->payload        = htonl( i_n_payload );
    p_hdr_t->slice_total    = htons( n_total_slice);
    
    if( i_p_option != NULL )
    {
        memcpy( p_hdr_t->payload_opt, i_p_option, i_n_opt_len );
    }
        
    for(n_index = 0; n_index <  n_total_slice; n_index ++)
    {
    	p_hdr_t->slice_num = htons(n_index);
    	n_send_size = PACKET_DATA_SIZE;
    	if( ( n_index == n_total_slice - 1) && ((i_n_length % PACKET_DATA_SIZE) > 0 ))
    		n_send_size = i_n_length % PACKET_DATA_SIZE;
    	
    	if(i_p_buff)
    		memcpy( &p_send_buf[sizeof(packet_header_t)], &i_p_buff[n_index * PACKET_DATA_SIZE], n_send_size ); 	
    	m_p_tcp_c->Send(i_n_index, p_send_buf, PACKET_SIZE);
     	
    } 
   	
	return true;	
}
	

/***********************************************************************************************//**
* \brief	log sender proc
* \param
* \return	
* \note
***************************************************************************************************/
bool LOGD::tcp_proc(int i_n_index, char* i_p_stream, int i_n_len)
{
	client_info_t* cur_client = &m_p_client_t[i_n_index];
	packet_header_t * phdr;
	
	int i=0;
	int remain=0;
	int total_len;

	if(cur_client->n_buf_end_pos + i_n_len >= CLIENT_BUFF_SIZE)
	{		
		cur_client->n_buf_end_pos = 0;
		return false;
	}

	memcpy( &cur_client->p_packet_buf[cur_client->n_buf_end_pos], i_p_stream, i_n_len);
	cur_client->n_buf_end_pos += i_n_len;
	
	total_len = cur_client->n_buf_end_pos;
	
	// find header
	for( i = 0; i < cur_client->n_buf_end_pos; i++ )
	{
		phdr = (packet_header_t *)&cur_client->p_packet_buf[i];
		if(memcmp( phdr->head, PACKET_HEAD, 4 ) == 0)
		{
		    if( PACKET_SIZE  <= (cur_client->n_buf_end_pos - (i)))
			{
				if( tcp_packet_parse( i_n_index, &cur_client->p_packet_buf[i] ) == 0 )
				{
					cur_client->n_buf_end_pos = 0;
					return false;
				}
				if( PACKET_SIZE < (cur_client->n_buf_end_pos - (i)) )
				{
					remain = i + PACKET_SIZE;
					i = remain-1;
					continue;
				}
				else
				{
					cur_client->n_buf_end_pos = 0;
					return true;
				}
			}
			else
			{
				memcpy( cur_client->p_packet_buf, &cur_client->p_packet_buf[remain], (cur_client->n_buf_end_pos - i) );
				cur_client->n_buf_end_pos = cur_client->n_buf_end_pos  - i;
				return true;
			}
		}
		else
		{
			printf("[LOGD(tcp)] Header error (%d: 0x%04X)\n",  i, cur_client->p_packet_buf[i]);
			printf("[LOGD(tcp)] total_len: %d, buf_end_pos: %d, i: %d, remain: %d\n", total_len, cur_client->n_buf_end_pos, i, remain );
		
		}
	}

	return true;
}

/******************************************************************************/
/**
* \brief        tcp parsing packet
* \param        none
* \return       none
* \note
*******************************************************************************/
bool LOGD::tcp_packet_parse( int i_n_index, unsigned char* i_p_data )
{
	bool 				ret = true;
	packet_header_t* 	p_hdr_t;
	unsigned char*		p_data = &i_p_data[sizeof(packet_header_t)];	
    
	p_hdr_t = (packet_header_t *)i_p_data;
	p_hdr_t->payload       = ntohl(p_hdr_t->payload);
	p_hdr_t->seq_id        = ntohl(p_hdr_t->seq_id);
	p_hdr_t->slice_num     = ntohs(p_hdr_t->slice_num);
	p_hdr_t->slice_total   = ntohs(p_hdr_t->slice_total);
	
    m_n_client_seq_id[i_n_index] = p_hdr_t->seq_id;    

	printf("[LOGD(tcp)] rx_do: payload(0x%04X), seq_id(0x%04X)\n", p_hdr_t->payload, p_hdr_t->seq_id);
	switch( p_hdr_t->payload )
	{
		case PAYLOAD_REQ_LOG_LIST:
	    {	
	    	unsigned int	n_log_count = 0;        
	    	unsigned char*	p_log_list = NULL;
	    	
	    	n_log_count = get_log_count();
	    	printf("[LOGD(tcp)] PAYLOAD_REQ_LOG_LIST : log count : %d\n", n_log_count);
	    	if(n_log_count > 0)
	    		make_log_list(&p_log_list, n_log_count);
	    	
	    	tcp_send(i_n_index, PAYLOAD_REQ_LOG_LIST, p_log_list, n_log_count * sizeof(time_t), (unsigned char*)&n_log_count, 4);
	    	
	    	safe_delete(p_log_list)
	    	
	    	ret = true;
	        break;
	    }
	    case PAYLOAD_REQ_LOG:
	    {	   
	    	
	  		unsigned int 	n_operation = 0;
	  		unsigned int 	n_file_size = 0;
	  		unsigned char	p_operation[8];
	  		unsigned char*	p_log_data = NULL;
	  		char			p_filename[60]={0,};
	  		char			sys_cmd[80]={0,};
	  		time_t			cur_time_t;	  		
	  			  		
	  		memcpy(&n_operation, p_hdr_t->payload_opt,4);	  			  		
	  		memcpy(&cur_time_t, p_data, 4);									
			get_file_name_format(cur_time_t,p_filename);
			
			printf("[LOGD(tcp)] PAYLOAD_REQ_LOG - operation : %d, file : %s(0x%x)\n",n_operation , p_filename, (unsigned int)cur_time_t);
			
			
			n_file_size = get_file_size(p_filename);
			if(n_file_size == 0)
				printf("[LOGD(tcp)] PAYLOAD_REQ_LOG - can not found file : %s\n",p_filename);
				
			if(n_operation == 0)
			{			
				if((n_file_size > 0) && load_file(p_filename, n_file_size, &p_log_data))
				{
					n_operation = 0;
					printf("[LOGD(tcp)] PAYLOAD_REQ_LOG - Load log file : %s\n",p_filename);
				}
				else
				{
					n_operation = 1;
					p_log_data = NULL;										
				}
							
			}
			else if(n_operation == 1)
			{
				if(n_file_size > 0)
				{
					n_operation = 0;
					sprintf(sys_cmd,"rm -rf %s",p_filename);
					system(sys_cmd);
					printf("[LOGD(tcp)] PAYLOAD_REQ_LOG - delete file : %s\n",sys_cmd);
				}
				else
				{
					n_operation = 1;	
				}
				n_file_size = 0;				
			}
			
			memcpy(p_operation, &n_operation, 4);
			memcpy(&p_operation[4], &n_file_size, 4);
			
			tcp_send(i_n_index, PAYLOAD_REQ_LOG, p_log_data, n_file_size, (unsigned char*)&p_operation, 8);						
			safe_delete(p_log_data);
				
	    	ret = true;
	        break;
	    }
	    case PAYLOAD_BYE:
	    {	        
	    	printf("[LOGD(tcp)] Bye\n");
	    	ret = false;
	        break;
	    }
	    default:
	    {
	        printf("[LOGD(tcp)] invalid payload(0x%04X)\n", p_hdr_t->payload);
	    }
	}

	return ret;
}

/******************************************************************************
* \brief        get log count
* \param        none
* \return       none
* \note
*******************************************************************************/
unsigned int LOGD::get_log_count()
{
	DIR*			p_dir_info;	
	struct dirent*	p_dir_entry;
	unsigned int	n_count = 0;
	
	p_dir_info = opendir(config.get_location());
	if(NULL != p_dir_info)
	{		
		while((p_dir_entry = readdir(p_dir_info)))
		{		
				if(p_dir_entry->d_type == DT_REG)
				{			
					n_count++;
				}		
		}		
		closedir(p_dir_info);
	}
		
	return n_count;	
}

/******************************************************************************
* \brief        make log list
* \param        none
* \return       none
* \note
*******************************************************************************/
bool LOGD::make_log_list(unsigned char** o_p_buf, unsigned int i_n_log_count)
{
	DIR*			p_dir_info;	
	struct dirent*	p_dir_entry;
	int				n_index = 0;
	time_t			log_time_t;
	char			p_filename[30];
	
	*o_p_buf = new unsigned char[i_n_log_count * sizeof(time_t)];
	memset(*o_p_buf, 0x00, i_n_log_count * sizeof(time_t));		

	p_dir_info = opendir(config.get_location());
	if(NULL != p_dir_info)
	{		
		while((p_dir_entry = readdir(p_dir_info)))
		{		
				if(p_dir_entry->d_type == DT_REG)
				{	
					memset(p_filename, 0x00, 30);
					strcpy(p_filename, p_dir_entry->d_name);
					log_time_t = get_file_name_to_time(p_filename);		
					
					memcpy(*o_p_buf + n_index , &log_time_t, sizeof(time_t));					
					n_index += sizeof(time_t);				
				}
		
		}		
		closedir(p_dir_info);
	}	
	return true;	
}



/******************************************************************************
* \brief        tcp parsing packet
* \param        none
* \return       none
* \note
*******************************************************************************/
bool LOGD::load_file(char* i_p_filename, int i_n_size, unsigned char** o_p_buf)
{
	FILE* p_fp;	
	
	p_fp = get_file_open(i_p_filename,"rb");
	if(p_fp)
	{	
		*o_p_buf = new unsigned char[i_n_size];
		fread(*o_p_buf, i_n_size, 1, p_fp);	
		fclose(p_fp);			
	}
	else
	{
		*o_p_buf = NULL;
		return false;	
	}
	return true;	
}





