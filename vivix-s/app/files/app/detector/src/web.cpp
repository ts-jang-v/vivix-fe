/*******************************************************************************
 모  듈 : capture
 작성자 : 한명희
 버  전 : 0.0.1
 설  명 : 촬영 모드에 따라 영상을 수집한다.
 참  고 : 

 버  전:
         0.0.1  최초 작성
*******************************************************************************/



/*******************************************************************************
*	Include Files
*******************************************************************************/
#include <iostream>
#include <stdio.h>			// fprintf()
#include <string.h>			// memset() memset/memcpy
#include <stdlib.h>			// malloc(), free(),exit(), EXIT_SUCCESS rand/rand 
#include <stdbool.h>		// bool, true, false
#include <assert.h>
#include <errno.h>            // errno
#include <sys/poll.h>		// struct pollfd, poll()

#include <linux/unistd.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>		// mkfifo
#include <fcntl.h>			// open

#include "web.h"
#include "misc.h"
#include "vw_time.h"
#include "sha1.h"
#include "vw_file.h"

using namespace std;

/*******************************************************************************
*	Constant Definitions
*******************************************************************************/
#define LUT_FILE				"/vivix/preview.lut"
#define GAIN_FILE				"/mnt/mmc/p2/preview_gain.bin"
#define DEFECT_FILE				"/mnt/mmc/p2/preview_defect.bin"

/*******************************************************************************
*	Type Definitions
*******************************************************************************/

/*******************************************************************************
*	Macros (Inline Functions) Definitions
*******************************************************************************/

/*******************************************************************************
*	Variable Definitions
*******************************************************************************/

/*******************************************************************************
*	Function Prototypes
*******************************************************************************/



/******************************************************************************/
/**
* \brief        constructor
* \param        log         log function
* \return       none
* \note
*******************************************************************************/
CWeb::CWeb(int (*log)(int,int,const char *,...))
{
    print_log = log;
    strcpy( m_p_log_id, "WEB           " );
    
    m_p_system_c = NULL;
    
    m_p_patient_xml_doc			= NULL;
    m_p_portable_config_xml_doc = NULL;
    
    m_b_invert_image = false;
    m_b_use_for_human = true;
    
    m_p_preview_image_process_c = NULL;

    m_b_is_running = false;

    m_thread_t = (pthread_t)0;
	m_p_net_manager_c = NULL;
    
    print_log( DEBUG, 1, "[%s] CWeb\n", m_p_log_id );
}
/******************************************************************************/
/**
* \brief        constructor
* \param        none
* \return       none
* \note
*******************************************************************************/
CWeb::CWeb(void)
{
}

/******************************************************************************/
/**
* \brief        destructor
* \param        none
* \return       none
* \note
*******************************************************************************/
CWeb::~CWeb()
{
	message_proc_stop();
	
	
    print_log( DEBUG, 1, "[%s] ~CWeb\n", m_p_log_id );
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CWeb::initialize(void)
{
	// copy web files
	FILE* fp;
	struct stat sts;
	
	m_p_system_c->web_set_ready( false );
	
	if ((stat (WEB_SAVED_DIR , &sts)) == -1 && errno == ENOENT)
	{
		sys_command( "tar zxvf /vivix/www.tar.gz" );
		sys_command( "mkdir %s", WEB_SAVED_DIR );
		sys_command( "cp -ar /run/www/* %s", WEB_SAVED_DIR );
		sys_command( "sync" );
	}
	
	sys_command( "chmod 777 %s*", WEB_SAVED_XML_DIR );
	sys_command( "chmod 777 %s*", WEB_SAVED_DATA_DIR );
	
	sys_command( "cp -ar %s* %s", WEB_SAVED_DIR, WEB_DIR );
	sys_command( "chmod +x %s", WEB_CGI_FILE );

	init_config();
	read_config();

	if ((stat (WEB_XML_FILE_PATIENT , &sts)) == -1 && errno == ENOENT)
	{
		init_patient_info();
	}
	else
	{
	fp = fopen(WEB_XML_FILE_PATIENT, "rb");
	if(fp != NULL)
	{
		m_p_patient_xml_doc = mxml_document_new();
		mxml_read_file( fp, m_p_patient_xml_doc, 0 );
		fclose(fp);
		}
	}
	
	memset(&m_patient_info_t, 0x00, sizeof(patient_info_t));
	make_current_patient(0);
	
	m_p_system_c->patient_info_set(m_patient_info_t);
	
	//sys_command( "cp -ar /run/config.xml %s", WEB_XML_FILE_DIR );
	config_ap();
	
	preview_init();
	
	message_proc_start();
	
	// start web server
	//sys_command( "lighttpd -f /etc/lighttpd/lighttpd.conf -m /lib/" );
	
	return true;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CWeb::set_patient_info(patient_info_t i_info_t)
{
	memcpy( &m_patient_info_t, &i_info_t, sizeof(patient_info_t) );
	make_current_patient(0);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CWeb::update_patient_list(void)
{
	MXML_NODE *listNode;
	MXML_NODE *childNode;
	MXML_NODE *tempNode;
	
	s8		p_file_name[256];
	
	sprintf(p_file_name, "%s", WEB_SAVED_XML_FILE_PATIENT);
	
	FILE* fp = fopen( p_file_name, "rb" );
	
	MXML_DOCUMENT *doc = mxml_document_new();
	MXML_NODE *Node;
	MXML_ITERATOR iter;
	
	mxml_read_file( fp, doc, 0 );
	
	fclose(fp);
	
	MXML_NODE *currentNode;
	MXML_ITERATOR currentIter;
	
	mxml_iterator_setup(&iter,doc);
	Node = mxml_iterator_scan_node(&iter,(char*)"patient");
	while( Node != NULL)
	{
		mxml_iterator_setup( &currentIter, m_p_patient_xml_doc );
		currentNode = mxml_iterator_scan_node( &currentIter, (char*)"patient" );

		while(currentNode !=NULL)
		{
			if(!strcmp( xml_get_attr_string(Node,"id"),xml_get_attr_string(currentNode,"id") ) &&
					!strcmp( xml_get_attr_string(Node,"name"), xml_get_attr_string(currentNode, "name")) &&
					!strcmp( xml_get_attr_string(Node,"accno"), xml_get_attr_string(currentNode, "accno")) &&
					!strcmp( xml_get_attr_string(Node,"birth"), xml_get_attr_string(currentNode, "birth")) &&
					!strcmp( xml_get_attr_string(Node,"scheduled"), xml_get_attr_string(currentNode, "scheduled")) )
			{
				childNode = currentNode->child;
				while(childNode != NULL)
				{
					tempNode = childNode->next;
					mxml_node_unlink(childNode);
					mxml_node_destroy(childNode);
					childNode = tempNode;
				}
				mxml_node_unlink(currentNode);
				mxml_node_destroy(currentNode);
				break;
			}
			currentNode = mxml_iterator_next_brother(&currentIter);
		}

		listNode = xml_find_element(m_p_patient_xml_doc, "list");
		mxml_node_unlink(Node);
		mxml_node_add_below(listNode, Node);

		mxml_iterator_top(&iter);
		Node = mxml_iterator_scan_node(&iter,(char*)"patient");

	}

	mxml_write( m_p_patient_xml_doc, (char*)"/tmp/patient.xml", MXML_STYLE_INDENT | MXML_STYLE_THREESPACES );
	mxml_document_destroy(doc);
	
	sys_command( "cp /tmp/patient.xml %s", WEB_SAVED_XML_FILE_PATIENT );
	sys_command( "cp /tmp/patient.xml %s", WEB_XML_FILE_DIR );
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CWeb::init_config(void)
{
	FILE* pFile;

	pFile = fopen( WEB_XML_FILE_PORTABLE_CONFIG, "r");
	if(pFile == NULL)
	{
		pFile = fopen( "/tmp/portable_config.xml", "w" );
		//pFile = fopen( WEB_XML_FILE_PORTABLE_CONFIG, "w");

		if( pFile != NULL )
		{
			fprintf( pFile, "%s\n", "<?xml version=\"1.0\"?>");
			fprintf( pFile, "%s\n", "<config>");
			fprintf( pFile, "<locale country=\"us\"/>\n");
			fprintf( pFile, "<image invert=\"false\"/>\n");
			fprintf( pFile, "<usage human=\"false\"/>\n");
			fprintf( pFile, "%s\n", "</config>");
			fclose( pFile );
			
			sys_command( "cp /tmp/portable_config.xml %s", WEB_XML_FILE_PORTABLE_CONFIG );
			sys_command( "cat %s", WEB_XML_FILE_PORTABLE_CONFIG );
			sys_command( "cp %s %s", WEB_XML_FILE_PORTABLE_CONFIG, WEB_SAVED_XML_DIR );
		}
	}
	else
	{
		fclose(pFile);
	}
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CWeb::read_config(void)
{
	FILE* fp;	
	MXML_NODE *Node;
	MXML_NODE *newNode;
	MXML_ATTRIBUTE *xmlAttr;
	s8	p_file_name[256];
	
	m_b_invert_image = false;
	m_b_use_for_human = true;
	
	sys_command( "cp %s /tmp/", WEB_XML_FILE_PORTABLE_CONFIG );
	sprintf( p_file_name, "/tmp/portable_config.xml" );
	fp = fopen(p_file_name, "rb");
	if(fp != NULL)
	{
		m_p_portable_config_xml_doc = mxml_document_new();
		mxml_read_file( fp, m_p_portable_config_xml_doc, 0 );
		fclose(fp);

		Node = xml_find_element(m_p_portable_config_xml_doc,"image");
		
		if(Node == NULL)
		{
			newNode = mxml_node_new();
			newNode->name = (char*)malloc(6);
			strcpy(newNode->name, "image");

			xmlAttr = mxml_attribute_new();
			xmlAttr->name = (char*)malloc(7);
			strcpy(xmlAttr->name, "invert");
			xmlAttr->value = (char*)malloc(6);
			strcpy(xmlAttr->value, "false");

			newNode->attributes = xmlAttr;

			Node = xml_find_element(m_p_portable_config_xml_doc,"config");
			if(Node != NULL)
			{
				mxml_node_add_below(Node, newNode);
			}
			m_b_invert_image = false;

			//mxml_write( m_p_portable_config_xml_doc, (s8*)WEB_XML_FILE_PORTABLE_CONFIG, MXML_STYLE_INDENT | MXML_STYLE_THREESPACES );
			mxml_write( m_p_portable_config_xml_doc, (char*)"/tmp/poratble_config.xml", MXML_STYLE_INDENT | MXML_STYLE_THREESPACES );
			sys_command( "cp /tmp/poratble_config.xml %s", WEB_XML_FILE_PORTABLE_CONFIG );
			sys_command( "cp %s %s", WEB_XML_FILE_PORTABLE_CONFIG, WEB_SAVED_XML_DIR );
		}
		else if(strcmp(xml_get_attr_string(Node,"invert"), "false") == 0)
		{
			m_b_invert_image = false;
		}
		else if(strcmp(xml_get_attr_string(Node,"invert"), "true") == 0)
		{
			m_b_invert_image = true;
		}
		
		Node = xml_find_element(m_p_portable_config_xml_doc,"usage");
		
		if(Node == NULL)
		{
			newNode = mxml_node_new();
			newNode->name = (char*)malloc(6);
			strcpy(newNode->name, "usage");

			xmlAttr = mxml_attribute_new();
			xmlAttr->name = (char*)malloc(7);
			strcpy(xmlAttr->name, "human");
			xmlAttr->value = (char*)malloc(6);
			strcpy(xmlAttr->value, "true");
			
			m_b_use_for_human = true;
			
			newNode->attributes = xmlAttr;

			Node = xml_find_element(m_p_portable_config_xml_doc,"config");
			if(Node != NULL)
			{
				mxml_node_add_below(Node, newNode);
			}

			//mxml_write( m_p_portable_config_xml_doc, (s8*)WEB_XML_FILE_PORTABLE_CONFIG, MXML_STYLE_INDENT | MXML_STYLE_THREESPACES );
			mxml_write( m_p_portable_config_xml_doc, (char*)"/tmp/portable_config.xml", MXML_STYLE_INDENT | MXML_STYLE_THREESPACES );
			
			sys_command( "cp /tmp/portable_config.xml %s", WEB_XML_FILE_PORTABLE_CONFIG );
			sys_command( "cp %s %s", WEB_XML_FILE_PORTABLE_CONFIG, WEB_SAVED_XML_DIR );
		}
		else if(strcmp(xml_get_attr_string(Node,"human"), "false") == 0)
		{
			m_b_use_for_human = false;
		}
		else if(strcmp(xml_get_attr_string(Node,"human"), "true") == 0)
		{
			m_b_use_for_human = true;
		}
	}
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
bool CWeb::message_proc_start(void)
{
    // launch web message thread
    m_b_is_running = true;
	if( pthread_create(&m_thread_t, NULL, web_routine, ( void * ) this)!= 0 )
    {
    	print_log( ERROR, 1, "[%s] pthread_create:%s\n", m_p_log_id, strerror( errno ));

    	m_b_is_running = false;
    	
    	return false;
    }
    
    return true;
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void CWeb::message_proc_stop(void)
{
	if( m_b_is_running == true )
	{
		m_b_is_running = false;
		
		if( pthread_join( m_thread_t, NULL ) != 0)
	    {
	    	print_log( ERROR, 1,"[%s] pthread_join:(capture_proc):%s\n", m_p_log_id, strerror( errno ));
	    }
	}
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void * web_routine( void * arg )
{
	CWeb * web = (CWeb *)arg;
	web->message_proc();
	pthread_exit( NULL );
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CWeb::message_proc(void)
{
    print_log(DEBUG, 1, "[%s] start message_proc...(%d)\n", m_p_log_id, syscall( __NR_gettid ));
	
	int		fd;
	qData	msg_qData;
	CTime*	p_update_status_time_c = NULL;
	CTime*	p_session_time_c = NULL;
	FILE*	p_session_timeout;
	
    struct pollfd		events[1];
    
    memset(events,0,sizeof(events));

	if ( -1 == mkfifo(FIFO_FILE1, 0666))
	{
		perror( "mkfifo() error");
	}

	sys_command( "chmod 666 %s", FIFO_FILE1 );

	if ( -1 == ( fd = open(FIFO_FILE1, O_RDWR)))           
	{
		perror( "fifo open() error");
		exit( 1);
	}
    
    events[0].fd = fd;
    events[0].events = POLLIN;
    
    sys_command( "lighttpd -f /etc/lighttpd/lighttpd.conf -m /usr/lib/lighttpd" );
    
    p_update_status_time_c = new CTime( 5000 );
    
    while( m_b_is_running )
    {
    	if( p_update_status_time_c->is_expired() == true )
    	{
    		update_status();
    		
    		safe_delete( p_update_status_time_c );
    		p_update_status_time_c = new CTime( 5000 );
    	}
    	
    	if( p_session_time_c != NULL
    		&& p_session_time_c->is_expired() == true )
    	{
    		safe_delete(p_session_time_c);
    		sys_command("rm %s", WEB_SESSION_TIMEOUT_FILE);
    		print_log(DEBUG, 1, "[%s] session is expired.\n", m_p_log_id);
    	}
    	
		if( poll( (struct pollfd *)&events, 1, 100 ) <= 0 )
        {
        	continue;
        }
        
        memset( msg_qData.text, 0, Q_TEXT_BUFF_SIZE);
		
		if(read(fd, &msg_qData, sizeof(qData)) > 0)
		{
			int nType = EMSGTYPE_SEND_PATIENT_INFO;
			
			nType = msg_qData.type;
			
			print_log(DEBUG, 1, "[%s] fifo recv : %s, type 0x%02X\n", m_p_log_id, msg_qData.text, nType);
			
			switch(nType)
			{
				case EMSGTYPE_MODIFY_CONFIG:
				{
					modify_config(msg_qData.text);
					break;
				}
				case EMSGTYPE_INIT_CONFIG:
				{
					break;
				}
				case EMSGTYPE_ADD_PATIENT_INFO:
				{
					add_patient_info(msg_qData.text, true);

					break;
				}
				case EMSGTYPE_ADD_STEP:
				{
					add_step(msg_qData.text, true);
					break;
				}
				case EMSGTYPE_CHANGE_PASSWORD:
				case EMSGTYPE_INIT_PASSWORD:
				case EMSGTYPE_COPY_LOGIN_DATA:
				{
					sys_command( "cp -ar %s %s", WEB_DATA_FILE_ACCOUNT, WEB_SAVED_DATA_DIR );
					break;
				}
				case EMSGTYPE_SEND_PATIENT_INFO:
				{
					m_p_system_c->web_set_ready( true );
					
					send_patient_info(msg_qData.text);

					sys_command( "rm -rf %s", WEB_XML_FILE_PREVIEW );

					break;
				}
				case EMSGTYPE_COPY_DIAG_XML:
				{
					//sys_command( "cp /tmp/diag.xml %s", WEB_XML_FILE_DIR );
					break;
				}
				case EMSGTYPE_REMOVE_IMG:
				{
					sys_command( "rm -rf %s*", WEB_PREVIEW_DIR );
					break;
				}
				case EMSGTYPE_INIT:
				{
					//sys_command( "cp -ar /run/config.xml %s", WEB_XML_FILE_DIR );
					config_ap(0);
					//make_current_patient(0);
					break;
				}
				case EMSGTYPE_DEL_PATIENT_INFO:
				{
					init_patient_info();
					break;
				}
				case EMSGTYPE_DEL_ONE_STUDY:
				{
					del_study(msg_qData.text);
					break;
				}
				case EMSGTYPE_CHANGE_LAGUAGE:
				{
					change_language(msg_qData.text);
					break;
				}
				case EMSGTYPE_CHANGE_IMAGE_INVERT:
				{
					change_image_invert(msg_qData.text);
					break;
				}
				case EMSGTYPE_USE_FOR_HUMAN:
				{
					change_use_for_human(msg_qData.text);
					break;
				}
				case EMSGTYPE_LOGIN:
				{
					safe_delete(p_session_time_c);
					p_session_time_c = new CTime(30 * 60 * 1000);
					
					p_session_timeout = fopen(WEB_SESSION_TIMEOUT_FILE, "w");
					
					fprintf(p_session_timeout, "%d\n", (int)time(NULL));
					fclose(p_session_timeout);
					
					break;
				}
				case EMSGTYPE_LOGOUT:
				{
					if( p_session_time_c != NULL )
					{
						safe_delete(p_session_time_c);
						sys_command("rm %s", WEB_SESSION_TIMEOUT_FILE);
					}
					break;
				}
				case EMSGTYPE_UPDATE_SESSION_TIMEOUT:
				{
					safe_delete(p_session_time_c);
					p_session_time_c = new CTime(30 * 60 * 1000);
					break;
				}
			}
		}
	} /* end of while( m_b_is_running ) */
	
	safe_delete( p_update_status_time_c );
	
	print_log(DEBUG, 1, "[%s] stop message_proc...\n", m_p_log_id);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CWeb::modify_config( s8* i_p_data )
{
	printf("modify_config: %s\n", i_p_data );

	char token[]="||";
	int len=0;

	char* ptr;
	char element[20]={0,};
	char name[20]={0,};
	char value[20]={0,};

	//element
	ptr = strtok(i_p_data,token);
	len = strlen(ptr) + 1;
	strncpy(element,ptr, len); 
	
	//name
	ptr = strtok(NULL,token);
	len = strlen(ptr) + 1;
	strncpy(name,ptr, len); 

	//value
	ptr = strtok(NULL,token);
	len = strlen(ptr) + 1;
	strncpy(value,ptr, len);
	
	if( strcmp( value, "on" ) == 0 )
	{
		config_ap(1);
	}
	else
	{
		config_ap(2);
	}
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CWeb::add_patient_info( s8* i_p_data, bool i_b_enable )
{
	char token[]="||";
	int len=0;

	char* ptr;

	MXML_NODE *node;
	MXML_NODE *newNode;
	MXML_ATTRIBUTE *xmlAttr;
	
	newNode = mxml_node_new();
	newNode->name = (char*)malloc(8);
	strcpy(newNode->name, "patient");

	//name
	xmlAttr = mxml_attribute_new();
	xmlAttr->name = (char*)malloc(5);
	strcpy(xmlAttr->name, "name");

	ptr = strtok(i_p_data,token);
	len = strlen(ptr) + 1;
	xmlAttr->value = (char*)malloc(sizeof(char) * len);
	strcpy(xmlAttr->value, ptr);
	newNode->attributes = xmlAttr;

	//id
	xmlAttr->next = mxml_attribute_new();
	xmlAttr = xmlAttr->next;
	xmlAttr->name = (char*)malloc(3);
	strcpy(xmlAttr->name, "id");

	ptr = strtok(NULL,token);
	len = strlen(ptr) + 1;
	xmlAttr->value = (char*)malloc(sizeof(char) * len);
	strcpy(xmlAttr->value, ptr);
	

	//accNo
	xmlAttr->next = mxml_attribute_new();
	xmlAttr = xmlAttr->next;
	xmlAttr->name = (char*)malloc(6);
	strcpy(xmlAttr->name, "accno");

	ptr = strtok(NULL,token);
	if(!strcmp(ptr,"none"))
	{
		len = 1;
		xmlAttr->value = (char*)malloc(sizeof(char) * len);
		memset(xmlAttr->value, 0, len);
	}
	else
	{
		len = strlen(ptr) + 1;
		xmlAttr->value = (char*)malloc(sizeof(char) * len);
		strcpy(xmlAttr->value, ptr);
	}

	//studydate
	xmlAttr->next = mxml_attribute_new();
	xmlAttr = xmlAttr->next;
	xmlAttr->name = (char*)malloc(10);
	strcpy(xmlAttr->name, "studydate");

	ptr = strtok(NULL,token);
	len = strlen(ptr) + 1;
	xmlAttr->value = (char*)malloc(sizeof(char) * len);
	strcpy(xmlAttr->value, ptr);


	//studytime
	xmlAttr->next = mxml_attribute_new();
	xmlAttr = xmlAttr->next;
	xmlAttr->name = (char*)malloc(10);
	strcpy(xmlAttr->name, "studytime");

	ptr = strtok(NULL,token);
	len = strlen(ptr) + 1;
	xmlAttr->value = (char*)malloc(sizeof(char) * len);
	strcpy(xmlAttr->value, ptr);

	//birth
	xmlAttr->next = mxml_attribute_new();
	xmlAttr = xmlAttr->next;
	xmlAttr->name = (char*)malloc(6);
	strcpy(xmlAttr->name, "birth");

	ptr = strtok(NULL,token);

	if(!strcmp(ptr,"none"))
	{
		len = 1;
		xmlAttr->value = (char*)malloc(sizeof(char) * len);
		memset(xmlAttr->value, 0, len);
	}
	else
	{
		len = strlen(ptr) + 1;
		xmlAttr->value = (char*)malloc(sizeof(char) * len);
		strcpy(xmlAttr->value, ptr);
	}



	//etc
	xmlAttr->next = mxml_attribute_new();
	xmlAttr = xmlAttr->next;
	xmlAttr->name = (char*)malloc(8);
	strcpy(xmlAttr->name, "reserve");

	ptr = strtok(NULL,token);

	if(!strcmp(ptr,"none"))
	{
		len = 1;
		xmlAttr->value = (char*)malloc(sizeof(char) * len);
		memset(xmlAttr->value, 0, len);
	}
	else
	{
		len = strlen(ptr) + 1;
		xmlAttr->value = (char*)malloc(sizeof(char) * len);
		strcpy(xmlAttr->value, ptr);
	}

	// species description
	xmlAttr->next = mxml_attribute_new();
	xmlAttr = xmlAttr->next;
	xmlAttr->name = (char*)malloc(20);
	strcpy(xmlAttr->name, "species_description");

	ptr = strtok(NULL,token);

	if(!strcmp(ptr,"none"))
	{
		len = 1;
		xmlAttr->value = (char*)malloc(sizeof(char) * len);
		memset(xmlAttr->value, 0, len);
	}
	else
	{
		len = strlen(ptr) + 1;
		xmlAttr->value = (char*)malloc(sizeof(char) * len);
		strcpy(xmlAttr->value, ptr);
	}

	// breed description
	xmlAttr->next = mxml_attribute_new();
	xmlAttr = xmlAttr->next;
	xmlAttr->name = (char*)malloc(18);
	strcpy(xmlAttr->name, "breed_description");

	ptr = strtok(NULL,token);

	if(!strcmp(ptr,"none"))
	{
		len = 1;
		xmlAttr->value = (char*)malloc(sizeof(char) * len);
		memset(xmlAttr->value, 0, len);
	}
	else
	{
		len = strlen(ptr) + 1;
		xmlAttr->value = (char*)malloc(sizeof(char) * len);
		strcpy(xmlAttr->value, ptr);
	}

	// responsible person
	xmlAttr->next = mxml_attribute_new();
	xmlAttr = xmlAttr->next;
	xmlAttr->name = (char*)malloc(19);
	strcpy(xmlAttr->name, "responsible_person");

	ptr = strtok(NULL,token);

	if(!strcmp(ptr,"none"))
	{
		len = 1;
		xmlAttr->value = (char*)malloc(sizeof(char) * len);
		memset(xmlAttr->value, 0, len);
	}
	else
	{
		len = strlen(ptr) + 1;
		xmlAttr->value = (char*)malloc(sizeof(char) * len);
		strcpy(xmlAttr->value, ptr);
	}

	//neutered
	xmlAttr->next = mxml_attribute_new();
	xmlAttr = xmlAttr->next;
	xmlAttr->name = (char*)malloc(9);
	strcpy(xmlAttr->name, "neutered");

	ptr = strtok(NULL,token);

	if( !strcmp(ptr,"none") )
	{
		len = 1;
		xmlAttr->value = (char*)malloc(sizeof(char) * len);
		memset(xmlAttr->value, 0, len);
	}
	else
	{
		len = strlen(ptr) + 1;
		xmlAttr->value = (char*)malloc(sizeof(char) * len);
		strcpy(xmlAttr->value, ptr);
	}
	//enable
	xmlAttr->next = mxml_attribute_new();
	xmlAttr = xmlAttr->next;
	xmlAttr->name = (char*)malloc(8);
	strcpy(xmlAttr->name, "enable");
	
	if( i_b_enable == true )
	{
		xmlAttr->value = (char*)malloc(sizeof(char) * 5);
		strcpy(xmlAttr->value, "true");
	}
	else if( i_b_enable == false )
	{
		xmlAttr->value = (char*)malloc(sizeof(char) * 6);
		strcpy(xmlAttr->value, "false");
	}

	//scheduled
	xmlAttr->next = mxml_attribute_new();
	xmlAttr = xmlAttr->next;
	xmlAttr->name = (char*)malloc(10);
	strcpy(xmlAttr->name, "scheduled");
	
	xmlAttr->value = (char*)malloc(sizeof(char) * 15);
	strcpy(xmlAttr->value, "19700101121212");

	node = xml_find_element( m_p_patient_xml_doc, "list" );
	if(node != NULL)
	{
		mxml_node_add_below(node, newNode);
	}
	
	//mxml_write( m_p_patient_xml_doc, (s8*)WEB_XML_FILE_PATIENT, MXML_STYLE_INDENT | MXML_STYLE_THREESPACES );
	mxml_write( m_p_patient_xml_doc, (char*)"/tmp/patient.xml", MXML_STYLE_INDENT | MXML_STYLE_THREESPACES );
	sys_command( "cp /tmp/patient.xml %s", WEB_XML_FILE_PATIENT );
	sys_command( "cp %s %s", WEB_XML_FILE_PATIENT, WEB_SAVED_XML_DIR );
	
	return true;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CWeb::add_step( s8* i_p_data, bool i_b_enable )
{
	char token[]="||";
	int len=0;

	char* ptr;
	char* name;
	char* id;
	char* accno;
	char* birth;
	char* studydate;
	char* studytime;

	MXML_NODE *node;
	MXML_NODE *newNode;
	MXML_ATTRIBUTE *xmlAttr;

	newNode = mxml_node_new();
	newNode->name = (char*)malloc(8);
	strcpy(newNode->name, "step");

	//name
	ptr = strtok(i_p_data,token);
	len = strlen(ptr) + 1;
	name = (char*)malloc(sizeof(char) * len);
	strcpy(name, ptr);

	//id
	ptr = strtok(NULL,token);
	len = strlen(ptr) + 1;
	id = (char*)malloc(sizeof(char) * len);
	strcpy(id, ptr);
	

	//accNo
	ptr = strtok(NULL,token);
	if(!strcmp(ptr,"none"))
	{
		len = 1;
		accno = (char*)malloc(sizeof(char) * len);
		memset(accno, 0x00, len);
	}
	else
	{
		len = strlen(ptr) + 1;
		accno = (char*)malloc(sizeof(char) * len);
		strcpy(accno, ptr);
	}

	//birth
	ptr = strtok(NULL,token);

	if(!strcmp(ptr,"none"))
	{
		len = 1;
		birth = (char*)malloc(sizeof(char) * len);
		memset(birth, 0x00, len);
	}
	else
	{
		len = strlen(ptr) + 1;
		birth = (char*)malloc(sizeof(char) * len);
		strcpy(birth, ptr);
	}


	//studydate
	ptr = strtok(NULL,token);
	len = strlen(ptr) + 1;
	studydate = (char*)malloc(sizeof(char) * len);
	strcpy(studydate, ptr);


	//studytime
	ptr = strtok(NULL,token);
	len = strlen(ptr) + 1;
	studytime = (char*)malloc(sizeof(char) * len);
	strcpy(studytime, ptr);

	//key
	xmlAttr = mxml_attribute_new();
	xmlAttr->name = (char*)malloc(4);
	strcpy(xmlAttr->name, "key");

	ptr = strtok(NULL,token);
	len = strlen(ptr) + 1;
	xmlAttr->value = (char*)malloc(sizeof(char) * len);
	strcpy(xmlAttr->value, ptr);
	newNode->attributes = xmlAttr;

	//enable
	xmlAttr->next = mxml_attribute_new();
	xmlAttr = xmlAttr->next;
	xmlAttr->name = (char*)malloc(8);
	strcpy(xmlAttr->name, "enable");
	
	if( i_b_enable == true )
	{
		xmlAttr->value = (char*)malloc(sizeof(char) * 5);
		strcpy(xmlAttr->value, "true");
	}
	else if( i_b_enable == false )
	{
		xmlAttr->value = (char*)malloc(sizeof(char) * 6);
		strcpy(xmlAttr->value, "false");
	}


	//viewer serial
	ptr = strtok(NULL,token);
	xmlAttr->next = mxml_attribute_new();
	xmlAttr = xmlAttr->next;
	xmlAttr->name = (char*)malloc(13);
	strcpy(xmlAttr->name, "viewerserial");

	if(!strcmp(ptr,"none"))
	{
		len = 1;
		xmlAttr->value = (char*)malloc(sizeof(char) * len);
		memset(xmlAttr->value, 0x00, 1);
	}
	else
	{
		len = strlen(ptr) + 1;
		xmlAttr->value = (char*)malloc(sizeof(char) * len);
		strcpy(xmlAttr->value, ptr);
	}


	node = xml_find_element( m_p_patient_xml_doc, "list" );
	if(node != NULL)
	{
		node = xml_find_element( m_p_patient_xml_doc, "patient" );
		while(node!=NULL)
		{
			if(!strcmp(id,xml_get_attr_string(node,"id")) &&
				!strcmp(name,xml_get_attr_string(node,"name")) &&
				!strcmp(accno,xml_get_attr_string(node,"accno")) &&
				!strcmp(birth,xml_get_attr_string(node,"birth")) &&
				!strcmp(studydate,xml_get_attr_string(node,"studydate")) &&
				!strcmp(studytime,xml_get_attr_string(node,"studytime")) )
				{
					mxml_node_add_below(node, newNode);
					break;
				}

			node = node->next;
		}
	}

	//mxml_write( m_p_patient_xml_doc, (s8*)WEB_XML_FILE_PATIENT, MXML_STYLE_INDENT | MXML_STYLE_THREESPACES );
	mxml_write( m_p_patient_xml_doc, (char*)"/tmp/patient.xml", MXML_STYLE_INDENT | MXML_STYLE_THREESPACES );
	sys_command( "cp /tmp/patient.xml %s", WEB_XML_FILE_PATIENT );
	sys_command( "cp %s %s", WEB_XML_FILE_PATIENT, WEB_SAVED_XML_DIR );

	safe_free(name);
	safe_free(id);
	safe_free(accno);
	safe_free(birth);
	safe_free(studytime);
	safe_free(studydate);
	
	return true;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CWeb::send_patient_info( s8* i_p_data )
{
	char token[]="||";
	int len=0;

	char* name = NULL;
	char* id = NULL;
	char* accNo = NULL;
	char* birth = NULL;
	char* etc = NULL;
	char* species = NULL;
	char* breed = NULL;
	char* responsible = NULL;
	char* studydate = NULL;
	char* studytime = NULL;
	char* viewerserial = NULL;
	char* ptr;
	bool TokenStart = false;
	char* data_buffer_addr;
	char* data_buffer= (char*)malloc(sizeof(char) * strlen(i_p_data) + 1);
	
	patient_info_t patient_info;
	
	data_buffer_addr = data_buffer;
	strcpy(data_buffer, i_p_data);

	memset(&m_patient_info_t, 0x00, sizeof(patient_info_t));
	m_p_system_c->patient_info_set(m_patient_info_t);
	
	memset(&patient_info, 0x00, sizeof(patient_info_t));
	//id
	if(!strncmp(data_buffer, token, 2))
	{
		i_p_data += 2;
		data_buffer += 2;
	}
	else
	{
		ptr = strtok(i_p_data,token);
		
		len = strlen(ptr);
		strcpy(patient_info.id, ptr);
		printf("=======> id: %s\n", patient_info.id);
		
		id = (char*)malloc(sizeof(char) * 130);
		len = utf2uni((u8*)ptr, (u8*)id, len);
		memcpy(m_patient_info_t.id , id , len);
		
		TokenStart = true;
		data_buffer += (strlen(ptr) + 2);
	}

	//accNo
	if(!strncmp(data_buffer, token, 2))
	{
		i_p_data += 2;
		data_buffer += 2;
	}
	else
	{
		if(TokenStart)
			ptr = strtok(NULL,token);
		else
			ptr = strtok(i_p_data,token);

		len = strlen(ptr) ;
		strcpy(patient_info.accNo, ptr);
		printf("=======> accNo: %s\n", patient_info.accNo);
		
		accNo = (char*)malloc(sizeof(char) * 34);
	 	len = utf2uni((u8*)ptr, (u8*)accNo, len);
		memcpy(m_patient_info_t.accNo , accNo , len);
		
		TokenStart = true;
		data_buffer += (strlen(ptr)+2);
	}

	//birth
	if(!strncmp(data_buffer, token, 2))
	{
		i_p_data += 2;
		data_buffer += 2;
	}
	else
	{
		if(TokenStart)
			ptr = strtok(NULL,token);
		else
			ptr = strtok(i_p_data,token);

		len = strlen(ptr) ;
		strcpy(patient_info.birth, ptr);
		printf("=======> birth: %s\n", patient_info.birth);
		
		birth = (char*)malloc(sizeof(char) * 34);
	 	len = utf2uni((u8*)ptr, (u8*)birth, len);
		memcpy(m_patient_info_t.birth , birth , len);
		
		TokenStart = true;
		data_buffer += (strlen(ptr)+2);
	}

	//name
	if(!strncmp(data_buffer, token, 2))
	{
		i_p_data += 2;
		data_buffer += 2;
	}
	else
	{

		if(TokenStart)
			ptr = strtok(NULL,token);
		else
			ptr = strtok(i_p_data,token);


		len = strlen(ptr) ;
		strcpy(patient_info.name, ptr);
		
		printf("=======> name: %s\n", patient_info.name);
		
		name = (char*)malloc(sizeof(char) * 130);
		len = utf2uni((u8*)ptr, (u8*)name, len);
		memcpy(m_patient_info_t.name , name , len);
		
		TokenStart = true;
		data_buffer += (strlen(ptr) +2);
	}


	//studydate
	if(!strncmp(data_buffer, token, 2))
	{
		i_p_data += 2;
		data_buffer += 2;
	}
	else
	{
		if(TokenStart)
			ptr = strtok(NULL,token);
		else
			ptr = strtok(i_p_data,token);

		len = strlen(ptr) ;
		strcpy(patient_info.studyDate, ptr);
		printf("=======> studydate: %s\n", patient_info.studyDate);
		
		studydate = (char*)malloc(sizeof(char) * 18);
	 	len = utf2uni((u8*)ptr, (u8*)studydate, len);
		memcpy(m_patient_info_t.studyDate , studydate , len);
		
		TokenStart = true;
		data_buffer += (strlen(ptr)+2);
	}

	//studytime
	if(!strncmp(data_buffer, token, 2))
	{
		i_p_data += 2;
		data_buffer += 2;
	}
	else
	{
		if(TokenStart)
			ptr = strtok(NULL,token);
		else
			ptr = strtok(i_p_data,token);

		len = strlen(ptr) ;
		strcpy(patient_info.studyTime, ptr);
		printf("=======> studytime: %s\n", patient_info.studyTime);
		
		studytime = (char*)malloc(sizeof(char) * 14);
	 	len = utf2uni((u8*)ptr, (u8*)studytime, len);
		memcpy(m_patient_info_t.studyTime , studytime , len);
		
		TokenStart = true;
		data_buffer += (strlen(ptr)+2);
	}

	//etc
	if(!strncmp(data_buffer, token, 2))
	{
		i_p_data += 2;
		data_buffer += 2;
	}
	else
	{
		if(TokenStart)
			ptr = strtok(NULL,token);
		else 
			ptr = strtok(i_p_data,token);
		len = strlen(ptr) ;
		strcpy((s8*)patient_info.reserve, ptr);
		
		printf("=======> etc: %s\n", patient_info.reserve);
		
		etc = (char*)malloc(sizeof(char) * 24);
		len = utf2uni((u8*)ptr, (u8*)etc, len);
		memcpy(m_patient_info_t.reserve , etc , len);
		
		TokenStart = true;
		data_buffer += (strlen(ptr)+2);
	}
	// species description
	if(!strncmp(data_buffer, token, 2))
	{
		i_p_data += 2;
		data_buffer += 2;
	}
	else
	{
		if(TokenStart)
			ptr = strtok(NULL,token);
		else 
			ptr = strtok(i_p_data,token);
		len = strlen(ptr) ;
		strcpy((s8*)patient_info.species_description, ptr);
		
		printf("=======> species_description: %s\n", patient_info.species_description);
		
		species = (char*)malloc(sizeof(char) * 130);
		len = utf2uni((u8*)ptr, (u8*)species, len);
		memcpy(m_patient_info_t.species_description , species , len);
		
		TokenStart = true;
		data_buffer += (strlen(ptr)+2);
	}

	// breed description
	if(!strncmp(data_buffer, token, 2))
	{
		i_p_data += 2;
		data_buffer += 2;
	}
	else
	{
		if(TokenStart)
			ptr = strtok(NULL,token);
		else 
			ptr = strtok(i_p_data,token);
		len = strlen(ptr) ;
		strcpy((s8*)patient_info.breed_description, ptr);
		
		printf("=======> breed_description: %s\n", patient_info.breed_description);
		
		breed = (char*)malloc(sizeof(char) * 130);
		len = utf2uni((u8*)ptr, (u8*)breed, len);
		memcpy(m_patient_info_t.breed_description , breed , len);
		
		TokenStart = true;
		data_buffer += (strlen(ptr)+2);
	}
	
	// responsible person
	if(!strncmp(data_buffer, token, 2))
	{
		i_p_data += 2;
		data_buffer += 2;
	}
	else
	{
		if(TokenStart)
			ptr = strtok(NULL,token);
		else 
			ptr = strtok(i_p_data,token);
		len = strlen(ptr) ;
		strcpy((s8*)patient_info.responsible_person, ptr);
		
		printf("=======> responsible_person: %s\n", patient_info.responsible_person);
		
		responsible = (char*)malloc(sizeof(char) * 130);
		len = utf2uni((u8*)ptr, (u8*)responsible, len);
		memcpy(m_patient_info_t.responsible_person , responsible , len);
		
		TokenStart = true;
		data_buffer += (strlen(ptr)+2);
	}
	
	// neutered
	m_patient_info_t.neutered = 0;
	if(!strncmp(data_buffer, token, 2))
	{
		i_p_data += 2;
		data_buffer += 2;
	}
	else
	{
		if(TokenStart)
			ptr = strtok(NULL,token);
		else 
			ptr = strtok(i_p_data,token);
		
		patient_info.neutered = 0;
		if( strcmp( ptr, "altered" ) == 0 )
		{
			patient_info.neutered = 1;
		}
		else if( strcmp( ptr, "unaltered" ) == 0 )
		{
			patient_info.neutered = 2;
		}
		
		m_patient_info_t.neutered = patient_info.neutered;
		printf("=======> neutered: %d\n", patient_info.neutered);
		TokenStart = true;
		data_buffer += (strlen(ptr)+2);
	}

	//viewer serial
	if(!strncmp(data_buffer, token, 2))
	{
		i_p_data += 2;
		data_buffer += 2;
	}
	else
	{
		if(TokenStart)
			ptr = strtok(NULL,token);
		else
			ptr = strtok(i_p_data,token);

		len = strlen(ptr) ;
		strcpy(patient_info.viewerSerial, ptr);
		printf("=======> viewerserial: %s\n", patient_info.viewerSerial);
		
		viewerserial = (char*)malloc(sizeof(char) * 16);
	 	len = utf2uni((u8*)ptr, (u8*)viewerserial, len);
		memcpy(m_patient_info_t.viewerSerial , viewerserial , len);
		
		TokenStart = true;
		data_buffer += (strlen(ptr)+2);
	}


	//step key
	if(strlen(data_buffer)!=0)
	{
		if(TokenStart)
			ptr = strtok(NULL,token);
		else
			ptr = strtok(i_p_data,token);

		m_patient_info_t.stepKey = atoi(ptr);
		patient_info.stepKey = m_patient_info_t.stepKey;
	}

	print_log(INFO,1,"[%s] name : %s, id : %s, accNo : %s,birth : %s, reserve : %s, viewer serial : %s, key: %d\n", \
						m_p_log_id, patient_info.name, patient_info.id, patient_info.accNo,  patient_info.birth, patient_info.reserve, patient_info.viewerSerial, patient_info.stepKey);
	
	safe_free(name);
	safe_free(id);
	safe_free(accNo);
	safe_free(birth);
	safe_free(studydate);
	safe_free(studytime);
	safe_free(viewerserial);
	safe_free(etc);
	safe_free(species);
	safe_free(breed);
	safe_free(responsible);
	safe_free(data_buffer_addr);
	
	m_p_system_c->patient_info_set(m_patient_info_t);
	
	return true;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CWeb::make_current_patient( time_t i_time_t )
{
	FILE* pFile;
	FILE* pPreviewFile;
	u8*	id;
	u8*	accNo;
	u8*	birth;
	u8*	studyDate;
	u8*	studyTime;
	u8*	name;
	u8* reserve; 
	u8* viewerserial;
	u8*	species;
	u8* breed;
	u8* responsible;
	u8	buf[1024]={0,};
	int nUniSize;
	int nUTF8Size;
	
	patient_info_t patient_info;

	//pFile = fopen( WEB_XML_FILE_CURRENT, "w" );
	pFile = fopen( "/tmp/current.xml", "w" );

	if( pFile == NULL )
	{
		return;
	}
	memcpy(&patient_info, &m_patient_info_t, sizeof(patient_info_t));

	//for unicode
	memset(buf, 0x00, 1024);
	nUniSize = unisize((short*)patient_info.id)*2 < 64*2 ? unisize((short*)patient_info.id)*2 : 64*2;
	nUTF8Size = uni2utf8((u8*)patient_info.id,buf,nUniSize);
	id = (unsigned char*)malloc(nUTF8Size + 1);
	memset(id,0, nUTF8Size + 1);
	memcpy(id, buf, nUTF8Size);

	memset(buf, 0x00, 1024);
	nUniSize = unisize((short*)patient_info.accNo)*2 < 16*2 ? unisize((short*)patient_info.accNo)*2 : 16*2;
	nUTF8Size = uni2utf8((u8*)patient_info.accNo,buf,nUniSize);
	accNo = (unsigned char*)malloc(nUTF8Size + 1);
	memset(accNo,0, nUTF8Size + 1);
	memcpy(accNo, buf,nUTF8Size);

	memset(buf, 0x00, 1024);
	nUniSize = unisize((short*)patient_info.studyDate)*2 < 8*2 ? unisize((short*)patient_info.studyDate)*2 : 8*2;
	nUTF8Size = uni2utf8((u8*)patient_info.studyDate,buf,nUniSize);
	studyDate = (unsigned char*)malloc(nUTF8Size + 1);
	memset(studyDate,0, nUTF8Size + 1);
	memcpy(studyDate, buf,nUTF8Size);

	memset(buf, 0x00, 1024);
	nUniSize = unisize((short*)patient_info.studyTime)*2 < 6*2 ? unisize((short*)patient_info.studyTime)*2 : 6*2;
	nUTF8Size = uni2utf8((u8*)patient_info.studyTime,buf,nUniSize);
	studyTime = (unsigned char*)malloc(nUTF8Size + 1);
	memset(studyTime,0, nUTF8Size + 1);
	memcpy(studyTime, buf,nUTF8Size);


	memset(buf, 0x00, 1024);
	nUniSize = unisize((short*)patient_info.name)*2 < 64*2 ? unisize((short*)patient_info.name)*2 : 64*2;
	nUTF8Size = uni2utf8((u8*)patient_info.name,buf,nUniSize);
	name = (unsigned char*)malloc(nUTF8Size + 1);
	memset(name,0, nUTF8Size + 1);
	memcpy(name, buf,nUTF8Size);

	memset(buf, 0x00, 1024);
	nUniSize = unisize((short*)patient_info.reserve)*2 < 11*2 ? unisize((short*)patient_info.reserve)*2 : 11*2; 
	nUTF8Size = uni2utf8((u8*)patient_info.reserve,buf,nUniSize); 
	reserve = (unsigned char*)malloc(nUTF8Size + 1);
	memset(reserve,0, nUTF8Size + 1);
	memcpy(reserve, buf,nUTF8Size); 

	memset(buf, 0x00, 1024);
	nUniSize = unisize((short*)patient_info.birth)*2 < 8*2 ? unisize((short*)patient_info.birth)*2 : 8*2; 
	nUTF8Size = uni2utf8((u8*)patient_info.birth,buf,nUniSize); 
	birth = (unsigned char*)malloc(nUTF8Size + 1);
	memset(birth,0, nUTF8Size + 1);
	memcpy(birth, buf,nUTF8Size); 

	memset(buf, 0x00, 1024);
	nUniSize = unisize((short*)patient_info.viewerSerial)*2 < 8*2 ? unisize((short*)patient_info.viewerSerial)*2 : 8*2; 
	nUTF8Size = uni2utf8((u8*)patient_info.viewerSerial,buf,nUniSize); 
	viewerserial = (unsigned char*)malloc(nUTF8Size + 1);
	memset(viewerserial,0, nUTF8Size + 1);
	memcpy(viewerserial, buf,nUTF8Size); 

	memset(buf, 0x00, 1024);
	nUniSize = unisize((short*)patient_info.species_description)*2 < 64*2 ? unisize((short*)patient_info.species_description)*2 : 64*2;
	nUTF8Size = uni2utf8((u8*)patient_info.species_description,buf,nUniSize);
	species = (unsigned char*)malloc(nUTF8Size + 1);
	memset(species,0, nUTF8Size + 1);
	memcpy(species, buf, nUTF8Size);

	memset(buf, 0x00, 1024);
	nUniSize = unisize((short*)patient_info.breed_description)*2 < 64*2 ? unisize((short*)patient_info.breed_description)*2 : 64*2;
	nUTF8Size = uni2utf8((u8*)patient_info.breed_description,buf,nUniSize);
	breed = (unsigned char*)malloc(nUTF8Size + 1);
	memset(breed,0, nUTF8Size + 1);
	memcpy(breed, buf, nUTF8Size);

	memset(buf, 0x00, 1024);
	nUniSize = unisize((short*)patient_info.responsible_person)*2 < 64*2 ? unisize((short*)patient_info.responsible_person)*2 : 64*2;
	nUTF8Size = uni2utf8((u8*)patient_info.responsible_person,buf,nUniSize);
	responsible = (unsigned char*)malloc(nUTF8Size + 1);
	memset(responsible,0, nUTF8Size + 1);
	memcpy(responsible, buf, nUTF8Size);


	fprintf( pFile, "%s\n", "<?xml version=\"1.0\"?>");
	fprintf( pFile, "%s\n", "<patient>");
	fprintf( pFile, "<info id=\"%s\" accno=\"%s\" studydate=\"%s\" studytime=\"%s\" name=\"%s\" birth=\"%s\" reserve=\"%s\" neutered=\"%d\" species_description=\"%s\" breed_description=\"%s\" responsible_person=\"%s\"/>\n", \
							id, accNo, studyDate, studyTime, name, birth, reserve, patient_info.neutered, species, breed, responsible );
	fprintf( pFile, "<step key=\"%ld\" bodypart=\"\" projection=\"\" viewerserial=\"%s\"/>\n", patient_info.stepKey, viewerserial);
	fprintf( pFile, "%s\n", "</patient>");
	fclose( pFile );
	
	sys_command( "cp /tmp/current.xml %s", WEB_XML_FILE_CURRENT );

	//pPreviewFile = fopen( WEB_XML_FILE_PREVIEW, "w" );
	pPreviewFile = fopen( "/tmp/preview.xml", "w" );

	if( pPreviewFile != NULL )
	{

		fprintf( pPreviewFile, "%s\n", "<?xml version=\"1.0\"?>");
		fprintf( pPreviewFile, "%s\n", "<preview>");
		fprintf( pPreviewFile, "<info id=\"%s\" accno=\"%s\" birth=\"%s\" studydate=\"%s\" studytime=\"%s\" name=\"%s\"/>\n", id, accNo,birth, studyDate, studyTime, name);
		fprintf( pPreviewFile, "<step key=\"%ld\"/>\n", patient_info.stepKey);
		fprintf( pPreviewFile, "<image time=\"%x\"/>\n", (s32)i_time_t);
		fprintf( pPreviewFile, "%s\n", "</preview>");
		fclose( pPreviewFile );
		
		sys_command( "cp /tmp/preview.xml %s", WEB_XML_FILE_PREVIEW );
	}

	if(i_time_t)
	{
		mark_exp_end((s8*)id,(s8*)name,(s8*)accNo,(s8*)birth,(s8*)studyDate,(s8*)studyTime, patient_info.stepKey, (s8*)viewerserial, (s8*)species, (s8*)breed, (s8*)responsible, patient_info.neutered);
	}

	free(id);
	free(accNo);
	free(birth);
	free(studyDate);
	free(studyTime);
	free(name);
	free(reserve);
	free(viewerserial);
	free(species);
	free(breed);
	free(responsible);

	m_p_system_c->web_set_ready( false );
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CWeb::init_patient_info(void)
{
	FILE* pFile;
	
	pFile = fopen( "/tmp/init_patient.xml", "w" );
	
	if( pFile == NULL )
	{
		return;
	}
	
	fprintf( pFile, "%s\n", "<?xml version=\"1.0\"?>");
	fprintf( pFile, "%s\n", "<list>");
	fprintf( pFile, "%s\n", "</list>");
	fclose( pFile );

	sys_command( "chmod 666 /tmp/init_patient.xml" );
	sys_command( "cp /tmp/init_patient.xml %s", WEB_XML_FILE_PATIENT );
	sys_command( "cp /tmp/init_patient.xml %s", WEB_SAVED_XML_FILE_PATIENT );
	
	if( m_p_patient_xml_doc != NULL )
	{
		delete_xml( m_p_patient_xml_doc );
	}

	pFile = fopen(WEB_XML_FILE_PATIENT, "rb");
	m_p_patient_xml_doc = mxml_document_new();
	mxml_read_file( pFile, m_p_patient_xml_doc, 0 );
	
	fclose(pFile);
	
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CWeb::delete_xml( MXML_DOCUMENT* doc )
{
	if(doc == NULL)
		return;

	MXML_NODE *Node;
	MXML_NODE *childNode;
	MXML_NODE *tempNode;
	MXML_ITERATOR iter;

	mxml_iterator_setup(&iter,doc);
	Node = mxml_iterator_scan_node(&iter,(s8*)"patient");
	while( Node != NULL)
	{
		childNode = Node->child;
		while(childNode != NULL)
		{
			tempNode = childNode->next;
			mxml_node_unlink(childNode);
			mxml_node_destroy(childNode);
			childNode = tempNode;
		}

		mxml_node_unlink(Node);
		mxml_node_destroy(Node);
		break;
		Node = Node->next;
	}
	Node = xml_find_element(doc,"list");
	if(Node != NULL)
	{
		mxml_node_unlink(Node);
		mxml_node_destroy(Node);
	}

	mxml_document_destroy(doc);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CWeb::del_study( s8* i_p_data )
{
	char token[]="||";
	int len=0;

	char* ptr;
	char* name;
	char* id;
	char* accno;
	char* birth;
	char* studydate;
	char* studytime;
	MXML_NODE *Node;
	MXML_NODE *childNode;
	MXML_NODE *tempNode;
	MXML_ITERATOR iter;


	//id
	ptr = strtok(i_p_data,token);
	len = strlen(ptr) + 1;
	id = (char*)malloc(sizeof(char) * len);
	strcpy(id, ptr);

	//name
	ptr = strtok(NULL,token);
	len = strlen(ptr) + 1;
	name = (char*)malloc(sizeof(char) * len);
	strcpy(name, ptr);

	//accNo
	ptr = strtok(NULL,token);
	if(!strcmp(ptr,"none"))
	{
		len = 1;
		accno = (char*)malloc(sizeof(char) * len);
		memset(accno, 0x00, len);
	}
	else
	{
		len = strlen(ptr) + 1;
		accno = (char*)malloc(sizeof(char) * len);
		strcpy(accno, ptr);
	}

	//birth
	ptr = strtok(NULL,token);
	if(!strcmp(ptr,"none"))
	{
		len = 1;
		birth = (char*)malloc(sizeof(char) * len);
		memset(birth, 0x00, len);
	}
	else
	{
		len = strlen(ptr) + 1;
		birth = (char*)malloc(sizeof(char) * len);
		strcpy(birth, ptr);
	}


	//studydate
	ptr = strtok(NULL,token);
	len = strlen(ptr) + 1;
	studydate = (char*)malloc(sizeof(char) * len);
	strcpy(studydate, ptr);


	//studytime
	ptr = strtok(NULL,token);
	len = strlen(ptr) + 1;
	studytime = (char*)malloc(sizeof(char) * len);
	strcpy(studytime, ptr);


	mxml_iterator_setup(&iter,m_p_patient_xml_doc);
	Node = mxml_iterator_scan_node(&iter,(s8*)"patient");
	while( Node != NULL)
	{
		if(!strcmp( xml_get_attr_string(Node,"id"),id ) &&
				!strcmp( xml_get_attr_string(Node,"name"), name) &&
				!strcmp( xml_get_attr_string(Node,"accno"), accno) &&
				!strcmp( xml_get_attr_string(Node,"birth"), birth) &&
				!strcmp( xml_get_attr_string(Node,"studytime"), studytime) &&
				!strcmp( xml_get_attr_string(Node,"studydate"), studydate) )
		{
			childNode = Node->child;
			while(childNode != NULL)
			{
				tempNode = childNode->next;
				mxml_node_unlink(childNode);
				mxml_node_destroy(childNode);
				childNode = tempNode;
			}

			mxml_node_unlink(Node);
			mxml_node_destroy(Node);
			break;
		}
		Node = Node->next;
	}

	//mxml_write( m_p_patient_xml_doc, (s8*)WEB_XML_FILE_PATIENT, MXML_STYLE_INDENT | MXML_STYLE_THREESPACES );
	mxml_write( m_p_patient_xml_doc, (char*)"/tmp/patient.xml", MXML_STYLE_INDENT | MXML_STYLE_THREESPACES );
	sys_command( "cp /tmp/patient.xml %s", WEB_XML_FILE_PATIENT );
	sys_command( "cp %s %s", WEB_XML_FILE_PATIENT, WEB_SAVED_XML_DIR );

	safe_free(id);
	safe_free(name);
	safe_free(accno);
	safe_free(birth);
	safe_free(studydate);
	safe_free(studytime);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CWeb::info_change_portable_config( const s8* i_p_element, const s8* i_p_attribute, const s8* i_p_value )
{
	MXML_NODE *Node;

	if(m_p_portable_config_xml_doc)
	{
		Node = xml_find_element( m_p_portable_config_xml_doc, i_p_element );
		xml_set_attr_string( Node, i_p_attribute, i_p_value );
		//mxml_write( m_p_portable_config_xml_doc, (s8*)WEB_XML_FILE_PORTABLE_CONFIG, MXML_STYLE_INDENT | MXML_STYLE_THREESPACES );
		mxml_write( m_p_portable_config_xml_doc, (char*)"/tmp/portable_config.xml", MXML_STYLE_INDENT | MXML_STYLE_THREESPACES );
		
		sys_command( "cp /tmp/portable_config.xml %s", WEB_XML_FILE_PORTABLE_CONFIG );
		sys_command( "cp %s %s", WEB_XML_FILE_PORTABLE_CONFIG, WEB_SAVED_XML_DIR );

		return true;
	}
	return false;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CWeb::change_language( s8* i_p_data )
{
	info_change_portable_config( "locale", "country", i_p_data );
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CWeb::change_image_invert( s8* i_p_data )
{
	if( info_change_portable_config("image", "invert", i_p_data)  == true )
	{
		m_b_invert_image = (strcmp(i_p_data, "false") == 0) ? false : true;
	}
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
int CWeb::unisize(short *unicode)
{
	int i=0;
	while(unicode[i] != 0x0000)
	{
		i++;
	}
	return i;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
int CWeb::utf2uni(unsigned char *utf, unsigned char *uni, int i)
{
	int t;
	
	unsigned char *p, *q;
	unsigned char k;
	
	p = utf;
	q = uni;

	
	for( t = 0; t < i; ) {
		if( (*p & 0x80) == 0x00 ) {
			*q = *p;
			*(q + 1) = 0x00;
			p++;
			t += 1;
		}
		else if( ((*p & 0xC0) == 0xC0) && ((*p & 0xE0) != 0xE0)) {
			*q = (*p & 0x1C) >> 2;
			*(q + 1) = (*p & 0x03) << 6;
			*(q + 1) = *(q + 1) | (*(p + 1) & 0x3F);

			k = *(q + 1);
			*(q + 1) = *q;
			*q = k;

			p += 2;
			t += 2;
		}
		else if( (*p & 0xE0) == 0xE0 ) {
			*q = *p << 4;
			*q = *q | ((*(p + 1) & 0x3C) >> 2);

			*(q + 1) = (*(p + 1) & 0x03) << 6;
			*(q + 1) = *(q + 1) | (*(p + 2) & 0x3F);

			k = *(q + 1);
			*(q + 1) = *q;
			*q = k;
			//

			p += 3;
			t += 3;
		}
		else {
		}
		q += 2;
	}

	return q-uni;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
int CWeb::uni2utf8(unsigned char *unicode, unsigned char *utf, int i)
{
	int t, j;

	unsigned char *p, *q;
	unsigned short k;

	p = unicode;
	q = utf;


	for(t = 0; t < i; t += 2) {
		k = make_syllable((p+t));
		j = make_utf8(q, k);
		q += j;
	}

	return q-utf;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
unsigned short CWeb::make_syllable(unsigned char *p)
{
	unsigned short k;
	k = ((unsigned short)*(p+1) << 8);
	k = k | (unsigned short)*p;

	return k;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
int CWeb::make_utf8(unsigned char *q, unsigned short k)
{
	if( k == 0x0000 ) {
		*q = (unsigned char)k;
		return 1;
	}
	else if( (k > 0x0000) && (k <= 0x007F) ) {
		*q = (unsigned char)k;
		return 1;
	}
	else if( (k >= 0x0080) && (k <= 0x07FF) ) {
		*q = *q | 0xC0;
		*q = *q | (unsigned char)((k & 0x07C0) >> 6);

		*(q+1) = *(q+1) | 0x80;
		*(q+1) = *(q+1) | (unsigned char)(k & 0x003F);

		return 2;
	}
	else if( (k >= 0x0800) && (k <= 0xFFFF) ) {
		*q = *q | 0xE0;
		*q = *q | (unsigned char)((k & 0xF000) >> 12);

		*(q+1) = *(q+1) | 0x80;
		*(q+1) = *(q+1) | (unsigned char)((k & 0x0FC0) >> 6);

		*(q+2) = *(q+2) | 0x80;
		*(q+2) = *(q+2) | (unsigned char)(k & 0x003F);

		return 3;
	}
	else {
	}

	return 0;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CWeb::make_backup_image_list( const char* i_p_file_name )
{
	FILE*	p_list_file;
	FILE*	p_xml_file;
	char	p_file_name[256];
    backup_image_header_t header_t;

    p_xml_file = fopen( i_p_file_name, "w" );

    if( p_xml_file == NULL )
    {
        return;
    }

    fprintf( p_xml_file, "%s\n", "<?xml version=\"1.0\"?>");
    fprintf( p_xml_file, "%s\n", "<BACKUP_IMAGE_LIST>");
	
	sys_command( "ls -1 %s*.bin* > /tmp/list", SYS_IMAGE_DATA_PATH );
	
	p_list_file = fopen("/tmp/list", "r");
	if( p_list_file == NULL )
	{
    	print_log( ERROR, 1, "[%s] No image in %s folder\n", m_p_log_id, SYS_IMAGE_DATA_PATH);
        goto ALLOC_BUF;
	}
	
	while( fscanf( p_list_file, "%s", p_file_name ) > 0 )
	{
		FILE* p_image_file;
		patient_info_t  info;
		u8*	id;
		u8*	accNo;
		u8*	birth;
		u8*	studyDate;
		u8*	studyTime;
		u8*	name;
		u8* reserve; 
		u8* viewerSerial;
		u8* species;
		u8* breed;
		u8* responsible;
		u8	buf[1024]={0,};
		int nUniSize;
		int nUTF8Size;
		u32 n_image_file_size;
		
		p_image_file = fopen( p_file_name, "rb" );

		if( p_image_file != NULL )
		{
			n_image_file_size = file_get_size( p_image_file );
			
			if( n_image_file_size <= m_p_system_c->get_image_size_byte() + 4 )
			{
				fclose(p_image_file);
				continue;
			}
			
			memset( &header_t, 0, sizeof(backup_image_header_t) );
			
			fseek( p_image_file, -1 * (sizeof(backup_image_header_t) + 4), SEEK_END);
			fread( &header_t, 1, sizeof(backup_image_header_t), p_image_file );
			fclose( p_image_file );
			
			fprintf( p_xml_file, "%s\n", "<PATIENT>");
			fprintf( p_xml_file, "\t<IMAGE id=\"0x%08X\" timestamp=\"0x%08X\" header_size=\"%d\" image_size=\"%d\" exp_time=\"%d\" swing_time=\"%d\" pos_x=\"%d\" pos_y=\"%d\" pos_z=\"%d\"/>\n", 
					 header_t.information.n_id,
					 header_t.information.n_time,
					 sizeof(backup_image_header_t),
					 header_t.information.n_total_size,
					 (header_t.information.w_exposure_time_high << 16) + header_t.information.w_exposure_time_low,
					 header_t.information.n_vcom_swing_time,
					 header_t.information.n_pos_x,
					 header_t.information.n_pos_y,
					 header_t.information.n_pos_z );
			
			memcpy( &info, &header_t.information.patient_t, sizeof(patient_info_t) );
			
			//for unicode
			memset(buf, 0x00, 1024);
			nUniSize = unisize((short*)info.id)*2 < 64*2 ? unisize((short*)info.id)*2 : 64*2;
			nUTF8Size = uni2utf8((u8*)info.id,buf,nUniSize);
			id = (unsigned char*)malloc(nUTF8Size + 1);
			memset(id,0, nUTF8Size + 1);
			memcpy(id, buf, nUTF8Size);

			memset(buf, 0x00, 1024);
			nUniSize = unisize((short*)info.accNo)*2 < 16*2 ? unisize((short*)info.accNo)*2 : 16*2;
			nUTF8Size = uni2utf8((u8*)info.accNo,buf,nUniSize);
			accNo = (unsigned char*)malloc(nUTF8Size + 1);
			memset(accNo,0, nUTF8Size + 1);
			memcpy(accNo, buf,nUTF8Size);

			memset(buf, 0x00, 1024);
			nUniSize = unisize((short*)info.studyDate)*2 < 8*2 ? unisize((short*)info.studyDate)*2 : 8*2;
			nUTF8Size = uni2utf8((u8*)info.studyDate,buf,nUniSize);
			studyDate = (unsigned char*)malloc(nUTF8Size + 1);
			memset(studyDate,0, nUTF8Size + 1);
			memcpy(studyDate, buf,nUTF8Size);

			memset(buf, 0x00, 1024);
			nUniSize = unisize((short*)info.studyTime)*2 < 6*2 ? unisize((short*)info.studyTime)*2 : 6*2;
			nUTF8Size = uni2utf8((u8*)info.studyTime,buf,nUniSize);
			studyTime = (unsigned char*)malloc(nUTF8Size + 1);
			memset(studyTime,0, nUTF8Size + 1);
			memcpy(studyTime, buf,nUTF8Size);


			memset(buf, 0x00, 1024);
			nUniSize = unisize((short*)info.name)*2 < 64*2 ? unisize((short*)info.name)*2 : 64*2;
			nUTF8Size = uni2utf8((u8*)info.name,buf,nUniSize);
			name = (unsigned char*)malloc(nUTF8Size + 1);
			memset(name,0, nUTF8Size + 1);
			memcpy(name, buf,nUTF8Size);

			memset(buf, 0x00, 1024);
			nUniSize = unisize((short*)info.reserve)*2 < 11*2 ? unisize((short*)info.reserve)*2 : 11*2; 
			nUTF8Size = uni2utf8((u8*)info.reserve,buf,nUniSize); 
			reserve = (unsigned char*)malloc(nUTF8Size + 1);
			memset(reserve,0, nUTF8Size + 1);
			memcpy(reserve, buf,nUTF8Size); 
			
			memset(buf, 0x00, 1024);
			nUniSize = unisize((short*)info.viewerSerial)*2 < 8*2 ? unisize((short*)info.viewerSerial)*2 : 8*2; 
			nUTF8Size = uni2utf8((u8*)info.viewerSerial,buf,nUniSize); 
			viewerSerial = (unsigned char*)malloc(nUTF8Size + 1);
			memset(viewerSerial,0, nUTF8Size + 1);
			memcpy(viewerSerial, buf,nUTF8Size); 
	
			memset(buf, 0x00, 1024);
			nUniSize = unisize((short*)info.birth)*2 < 8*2 ? unisize((short*)info.birth)*2 : 8*2; 
			nUTF8Size = uni2utf8((u8*)info.birth,buf,nUniSize); 
			birth = (unsigned char*)malloc(nUTF8Size + 1);
			memset(birth,0, nUTF8Size + 1);
			memcpy(birth, buf,nUTF8Size); 

			memset(buf, 0x00, 1024);
			nUniSize = unisize((short*)info.species_description)*2 < 64*2 ? unisize((short*)info.species_description)*2 : 64*2;
			nUTF8Size = uni2utf8((u8*)info.species_description,buf,nUniSize);
			species = (unsigned char*)malloc(nUTF8Size + 1);
			memset(species,0, nUTF8Size + 1);
			memcpy(species, buf,nUTF8Size);

			memset(buf, 0x00, 1024);
			nUniSize = unisize((short*)info.breed_description)*2 < 64*2 ? unisize((short*)info.breed_description)*2 : 64*2;
			nUTF8Size = uni2utf8((u8*)info.breed_description,buf,nUniSize);
			breed = (unsigned char*)malloc(nUTF8Size + 1);
			memset(breed,0, nUTF8Size + 1);
			memcpy(breed, buf,nUTF8Size);

			memset(buf, 0x00, 1024);
			nUniSize = unisize((short*)info.responsible_person)*2 < 64*2 ? unisize((short*)info.responsible_person)*2 : 64*2;
			nUTF8Size = uni2utf8((u8*)info.responsible_person,buf,nUniSize);
			responsible = (unsigned char*)malloc(nUTF8Size + 1);
			memset(responsible,0, nUTF8Size + 1);
			memcpy(responsible, buf,nUTF8Size);

			fprintf( p_xml_file, "\t<INFO id=\"%s\" accno=\"%s\" birth=\"%s\" studydate=\"%s\" studytime=\"%s\" name=\"%s\" time=\"%d\" serialno=\"%s\" type=\"%d\" reserve=\"%s\" viewerserial=\"%s\" stepkey=\"%ld\" neutered=\"%d\" species_description=\"%s\" breed_description=\"%s\" responsible_person=\"%s\"/>\n", 
					 id,
					 accNo,
					 birth,
					 studyDate,
					 studyTime,
					 name,
					 (s32)info.time,
					 info.serialNum,
					 info.type,
					 reserve,
					 viewerSerial,
					 info.stepKey,
					 info.neutered,
					 species,
					 breed,
					 responsible
					 );
			fprintf( p_xml_file, "%s\n", "</PATIENT>");
			
			free(id);
			free(accNo);
			free(birth);
			free(studyDate);
			free(studyTime);
			free(name);
			free(reserve);
			free(viewerSerial);
			free(species);
			free(breed);
			free(responsible);
		}
	}
	
	fclose( p_list_file );

ALLOC_BUF:
    
    fprintf( p_xml_file, "%s\n", "</BACKUP_IMAGE_LIST>");
	fclose( p_xml_file );
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CWeb::mark_exp_end(char* id, char* name, char* accno, char* birth, char* studydate, char* studytime, int key, char* viewerserial, char* species, char* breed, char* responsible, int neutered)
{
	bool bExpEnd;
	bool bExistStudy = false;
	bool bExistStep = false;
	bool bChanged = false;
	char str[3072]={0,};
	MXML_NODE *childNode;
	MXML_NODE *Node;
	MXML_ITERATOR iter;
	
	
	mxml_iterator_setup(&iter,m_p_patient_xml_doc);
	Node = mxml_iterator_scan_node(&iter, (s8*)"patient");
	while(Node)
	{

		bExpEnd = false;

		if(!strcmp( xml_get_attr_string(Node,"id"),id ) &&
				!strcmp( xml_get_attr_string(Node,"name"), name) &&
				!strcmp( xml_get_attr_string(Node,"accno"), accno) &&
				!strcmp( xml_get_attr_string(Node,"birth"), birth) &&
				!strcmp( xml_get_attr_string(Node,"studydate"),studydate) &&
				!strcmp( xml_get_attr_string(Node,"studytime"),studytime) )
		{
			bExistStudy = true;
			childNode = Node->child;
			while(childNode != NULL)
			{
				if((atoi(xml_get_attr_string(childNode, "key")) == key) && !strcmp(xml_get_attr_string(childNode,"enable"),"true"))
				{
					xml_set_attr_string(childNode,"enable","false");
					bChanged = true;
					bExistStep = true;
				}

				if(!strcmp(xml_get_attr_string(childNode, "enable"), "true"))
				{
					bExpEnd = true;
				}
				childNode = childNode->next;
			}
			if( (bExpEnd == false) && !strcmp(xml_get_attr_string(Node,"enable"),"true"))
			{
				bChanged = true;
				xml_set_attr_string(Node,"enable","false");
			}


			break;
		}
		Node = mxml_iterator_next_brother(&iter);
	}

	if(bChanged)
	{
		print_log(INFO,1,"[%s] update %s\n", m_p_log_id, WEB_XML_FILE_PATIENT );
		//mxml_write( m_p_patient_xml_doc, (s8*)WEB_XML_FILE_PATIENT, MXML_STYLE_INDENT | MXML_STYLE_THREESPACES );
		mxml_write( m_p_patient_xml_doc, (char*)"/tmp/patient.xml", MXML_STYLE_INDENT | MXML_STYLE_THREESPACES );
		sys_command( "cp /tmp/patient.xml %s", WEB_XML_FILE_PATIENT );
		sys_command( "cp %s %s", WEB_XML_FILE_PATIENT, WEB_SAVED_XML_DIR );
	}

	if(bExistStudy == false)
	{
		char p_neutered[10];
		
		if( neutered == 1 )
		{
			sprintf(p_neutered, "altered" );
		}
		else if( neutered == 2 )
		{
			sprintf(p_neutered, "unaltered" );
		}
		else
		{
			sprintf(p_neutered, "none" );
		}
		sprintf(str,"%s||%s||%s||%s||%s||%s||%s||%s||%s||%s||%s",name, id, strlen(accno) < 1 ? "none" : accno ,studydate, studytime,  strlen(birth) < 1 ? "none" : birth,"none", strlen(species) < 1 ? "none" : species, strlen(breed) < 1 ? "none" : breed, strlen(responsible) < 1 ? "none" : responsible, p_neutered);
		add_patient_info(str, false);
	}

	if( bExistStep == false)
	{
		sprintf(str,"%s||%s||%s||%s||%s||%s||%d||%s",name, id, strlen(accno) < 1 ? "none" : accno,  strlen(birth) < 1 ? "none" : birth,studydate, studytime,key, strlen(viewerserial) < 1 ? "none" : viewerserial);
		add_step(str, false);
	}

}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CWeb::update_status(void)
{
	FILE*		p_file;
	sys_state_t state_t;
	
	memset( &state_t, 0, sizeof(sys_state_t) );
	
	//p_file = fopen( WEB_XML_FILE_STATUS, "w" );
	p_file = fopen( "/tmp/status.xml", "w" );

	if( p_file != NULL )
	{
		m_p_system_c->get_state( &state_t );
		
		fprintf( p_file, "%s\n", "<?xml version=\"1.0\"?>");
		fprintf( p_file, "%s\n", "<status>");
		fprintf( p_file, "<bat volt=\"%2.1f\" remain=\"%d\" />\n", state_t.w_battery_vcell/10.0f, state_t.w_battery_gauge/10 );
		fprintf( p_file, "<image cnt=\"%d\" total=\"%d\"/>\n", m_p_system_c->image_get_saved_count(), 200);
		fprintf( p_file, "%s\n", "</status>");
		fclose( p_file );
		
		sys_command( "cp /tmp/status.xml %s", WEB_XML_FILE_DIR );
	}
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CWeb::preview_init(void)
{
	m_p_preview_image_process_c = m_p_system_c->get_image_process();
	
	m_p_preview_image_process_c->load_lut( LUT_FILE );
	//m_p_preview_image_process_c->load_gain( GAIN_FILE );
	//m_p_preview_image_process_c->defect_load( DEFECT_FILE );
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CWeb::preview_save( u8* i_p_data, s32 i_n_size, s32 i_n_time )
{
	u16* p_preview_image;
	s8 p_file_name[256];
	
	p_preview_image = (u16*)malloc(i_n_size);
	
	memcpy( p_preview_image, i_p_data, i_n_size );
	
	m_p_preview_image_process_c->process( p_preview_image, m_b_invert_image );
	
	sys_command( "rm -rf %s*", WEB_PREVIEW_DIR );
	
	sprintf( p_file_name, "%spreview_%x.bmp", WEB_PREVIEW_DIR, i_n_time );
	
	if( m_p_preview_image_process_c->SaveMono14ToBMP( (const s8*)p_file_name, (u8*)p_preview_image ) == true )
	{
		make_current_patient( i_n_time );
	}
	
	safe_free( p_preview_image );

}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CWeb::change_use_for_human( s8* i_p_data )
{
	if( info_change_portable_config("usage", "human", i_p_data) == true )
	{
		m_b_use_for_human = (strcmp(i_p_data, "false") == 0) ? false : true;
	}
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
int CWeb::reset_admin_password(void)
{
	FILE* fp = NULL;

	u8 _new[SHA1HashSize];
	u8 _id[SHA1HashSize];
	u8 saved_value[SHA1HashSize*4];
	SHA1Context context;
	s8 p_new[10];
	s8 p_id[10];
	//int idx = ((user - 1 ) * 2) ;
	int idx = 0;
	
	strcpy( p_new,	"admin" );
	strcpy( p_id,	"admin" );

	SHA1Reset(&context);
	SHA1Input(&context,  (const u8*)p_id, strlen(p_id));
	SHA1Result(&context, _id);

	SHA1Reset(&context);
	SHA1Input(&context, (const u8*)p_new, strlen(p_new));
	SHA1Result(&context, _new);

	// read data
	fp = fopen(WEB_DATA_FILE_ACCOUNT,"rb");

	if(fp == NULL)
		return 0x01;

	fread(saved_value, sizeof(uint8_t), SHA1HashSize*4, fp);
	fclose(fp);
	fp = NULL;

	// change password
	if(!memcpy(saved_value + (SHA1HashSize*idx), _id , SHA1HashSize))
		return 0x03;

	// change password
	if(!memcpy(saved_value + (SHA1HashSize*(idx+1)), _new , SHA1HashSize))
		return 0x04;

	// write data
	fp = fopen(WEB_DATA_FILE_ACCOUNT,"wb");

	if(fp == NULL)
		return 0x01;

	fwrite(saved_value, sizeof(uint8_t), SHA1HashSize*4, fp);
	fclose(fp);
	
	sys_command( "chmod 777 %s", WEB_DATA_FILE_ACCOUNT );
	sys_command( "cp %s %s", WEB_DATA_FILE_ACCOUNT, WEB_SAVED_FILE_ACCOUNT );

	return 0x00;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CWeb::config_ap( u8 i_c_select )
{
	FILE* pFile;
	s8		p_onoff[10];
	s8		p_config[10];
	
	strcpy( p_config, m_p_system_c->config_get( eCFG_AP_ENABLE ) );
	
	if( i_c_select == 0 )
	{
		strcpy( p_onoff, p_config );
	}
	else if( i_c_select == 1 )
	{
		if( strcmp( p_config, "on" ) != 0 )
		{
			m_p_net_manager_c->net_state_proc_stop();
			m_p_system_c->config_set( eCFG_AP_ENABLE, "on" );
			m_p_system_c->config_save();
			m_p_system_c->config_reload(true);
			m_p_net_manager_c->net_state_proc_start();
		}
		strcpy( p_onoff, "on" );
	}
	else if( i_c_select == 2 )
	{
		if( strcmp( p_config, "off" ) != 0 )
		{
			m_p_net_manager_c->net_state_proc_stop();
			m_p_system_c->config_set( eCFG_AP_ENABLE, "off" );
			m_p_system_c->config_save();
			m_p_system_c->config_reload(true);
			m_p_net_manager_c->net_state_proc_start();
		}
		strcpy( p_onoff, "off" );
	}
	else
	{
		print_log( DEBUG, 1, "[%s] Unknown AP configuration\n", m_p_log_id );
		return;
	}
	
	pFile = fopen( "/tmp/config.xml", "w");
	
	fprintf( pFile, "%s\n", "<?xml version=\"1.0\"?>");
	fprintf( pFile, "%s\n", "<DEVICE>");
	fprintf( pFile, "\t<AP ap_onoff=\"%s\"/>\n", p_onoff );
	fprintf( pFile, "%s\n", "</DEVICE>");
	
	fclose( pFile );
	sys_command( "cp /tmp/config.xml %s", WEB_XML_FILE_DIR );
}
