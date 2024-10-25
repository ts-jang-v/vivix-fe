/******************************************************************************/
/**
 * @file    
 * @brief   
 * @author  
*******************************************************************************/


#ifndef _VW_WEB_DEF_H_
#define _VW_WEB_DEF_H_

/*******************************************************************************
*   Include Files
*******************************************************************************/
#include "typedef.h"

/*******************************************************************************
*   Constant Definitions
*******************************************************************************/
#define FIFO_FILE1 "/tmp/fifo1"

#define Q_TEXT_BUFF_SIZE 1024

#define WEB_DIR				"/var/www/lighttpd/"
#define WEB_SAVED_DIR		"/vivix/www/"

#define WEB_CGI_FILE			"/var/www/lighttpd/cgi-bin/vieworks.cgi"
#define WEB_XML_FILE_DIR		"/var/www/lighttpd/xml/"
#define WEB_XML_FILE_PREVIEW	"/var/www/lighttpd/xml/preview.xml"
#define WEB_XML_FILE_PATIENT	"/var/www/lighttpd/xml/patient.xml"
#define WEB_XML_FILE_CURRENT	"/var/www/lighttpd/xml/current.xml"
#define WEB_XML_FILE_PORTABLE_CONFIG	"/var/www/lighttpd/xml/portable_config.xml"
#define WEB_XML_FILE_STATUS		"/var/www/lighttpd/xml/status.xml"
#define WEB_SAVED_XML_DIR		"/vivix/www/xml/"
#define WEB_SAVED_XML_FILE_PATIENT		"/vivix/www/xml/patient.xml"
#define WEB_SAVED_XML_FILE_STEP			"/vivix/www/xml/step.xml"

#define WEB_DATA_FILE_DIR		"/var/www/lighttpd/data/"
#define WEB_DATA_FILE_ACCOUNT	"/var/www/lighttpd/data/account.dat"
#define WEB_SAVED_DATA_DIR		"/vivix/www/data/"
#define WEB_SAVED_FILE_ACCOUNT	"/vivix/www/data/account.dat"

#define WEB_PREVIEW_DIR			"/var/www/lighttpd/preview/"

#define WEB_SESSION_TIMEOUT_FILE		"/tmp/session.timeout"

/*******************************************************************************
*   Type Definitions
*******************************************************************************/
enum EMSGTYPE{
	EMSGTYPE_LOGIN = 0x01,
	EMSGTYPE_CHANGE_PASSWORD,
	EMSGTYPE_SEND_PATIENT_INFO,
	EMSGTYPE_LOGOUT,
	EMSGTYPE_CHECK_SESSION = 0x05,
	EMSGTYPE_MODIFY_CONFIG,
	EMSGTYPE_INIT_CONFIG,
	EMSGTYPE_ADD_PATIENT_INFO,
	EMSGTYPE_ADD_STEP,
	EMSGTYPE_COPY_LOGIN_DATA = 0x0a,
	EMSGTYPE_COPY_DIAG_XML,
	EMSGTYPE_REMOVE_IMG,
	EMSGTYPE_INIT,
	EMSGTYPE_DEL_PATIENT_INFO,
	EMSGTYPE_INIT_PASSWORD,
	EMSGTYPE_DEL_ONE_STUDY = 0x10,
	EMSGTYPE_UPDATE_SESSION_TIMEOUT,
	EMSGTYPE_CHANGE_LAGUAGE,
	EMSGTYPE_CHANGE_IMAGE_INVERT,
	EMSGTYPE_USE_FOR_HUMAN,
};

typedef struct _qData
{
	int type;
	int cnt;
	char text[Q_TEXT_BUFF_SIZE];

}qData;

typedef struct _qBuf
{
    long type;
    qData data;
}qBuf;



/*******************************************************************************
*   Macros (Inline Functions) Definitions
*******************************************************************************/


/*******************************************************************************
*   Variable Definitions
*******************************************************************************/


/*******************************************************************************
*   Function Prototypes
*******************************************************************************/


#endif /* _VW_WEB_DEF_H_ */
