/*******************************************************************************
 모  듈 : login
 작성자 : 한명희
 버  전 : 0.0.1
 설  명 : web server cgi module
 참  고 : 

 버  전:
         0.0.1  1417W login.c 복사
*******************************************************************************/



/*******************************************************************************
*	Include Files
*******************************************************************************/
#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<string.h>
#include<unistd.h>
#include<fcntl.h>
#include<math.h>
#include <dirent.h>
#include "qdecoder.h"
#include "sha1.h"
#include "web_def.h"


/*******************************************************************************
*	Constant Definitions
*******************************************************************************/
#define		safe_free(p)		if(p!=NULL) {free(p);p=NULL;}

#define SUCCESS "SUCCESS"
#define FAIL	"FAIL"

#define ACCOUNT_DAT_FILE		"../data/account.dat"

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
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void print_header()
{
	printf("Content-type:text/xml\n\n");
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void makeResponse(int value, char* error)
{
	printf("<?xml version=\"1.0\"?>");
	printf("<RESPONSE>");
	printf("<VALUE>%d</VALUE>",value);
	printf("<ETC>%s</ETC>",error);
	printf("</RESPONSE>");
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
char hexStrTobyte(char* hex)
{

	char firstByte;
	char secondByte;
	char cTemp;
	char ret=0;
	int value;
	int i;

	firstByte = hex[0];
	secondByte = hex[1];

	for(i=0; i < 2; i++)
	{
		cTemp = hex[i];

		if(isalpha(cTemp))
		{
			value = (cTemp - 55);
		}
		else if(isdigit(cTemp))
		{
			value = (cTemp - 48);
		}
		else
			return -1;
		
		ret += (pow(16,1-i)*(value));

	}
	
	return ret;


}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void hexStrTobyteStream(int8_t* value, const char* id, const char* passwd)
{
	int i=0;
	int len=0;
	char pTemp;
	len = strlen(id);

	for(i=0; i< len; i=i+2)
	{
		pTemp =  hexStrTobyte(id+i);
		memcpy(value++, &pTemp, 1);
	}

	
	for(i=0; i< len; i=i+2)
	{
		pTemp =  hexStrTobyte(passwd+i);
		memcpy(value++, &pTemp, 1);
	}

}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
int checkIDandPASSWD(const char* id,const char* passwd, int* user)
{
	FILE* fp = NULL;

	int8_t sha1_value[SHA1HashSize*2];
	int8_t saved_value[SHA1HashSize*4];
	SHA1Context context;
	*user = 1;
/*
	SHA1Reset(&context);
	SHA1Input(&context,id, strlen(id));
	SHA1Result(&context, sha1_value);

	SHA1Reset(&context);
	SHA1Input(&context,passwd, strlen(passwd));
	SHA1Result(&context, sha1_value + SHA1HashSize);

	*/
	hexStrTobyteStream(sha1_value, id, passwd);

	//fp = fopen("/boa/www/data/account.dat","rb");
	fp = fopen(ACCOUNT_DAT_FILE,"rb");

//	fwrite(sha1_value, sizeof(uint8_t), SHA1HashSize*2, fp);
//	fclose(fp);
//	return 0x00;

	if(fp == NULL)
		return 0x03;
	
	fread(saved_value, sizeof(uint8_t), SHA1HashSize*4, fp);

	fclose(fp);

	if(memcmp(sha1_value, saved_value, SHA1HashSize))
	{
		if(memcmp(sha1_value , saved_value + (SHA1HashSize * 2), SHA1HashSize))
			return 0x04;

		if(memcmp(sha1_value + SHA1HashSize , saved_value + (SHA1HashSize * 3), SHA1HashSize))
			return 0x05;

		*user = 2;
		return 0x00;
	}

	if(memcmp(sha1_value + SHA1HashSize, saved_value + SHA1HashSize, SHA1HashSize))
		return 0x05;

	return 0x00;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
int changePassword(const int user, const char* passwd, const char* new)
{
	FILE* fp = NULL;

	int8_t sha1_value[SHA1HashSize*2];
	int8_t _passwd[SHA1HashSize];
	int8_t _new[SHA1HashSize];
	int8_t saved_value[SHA1HashSize*4];
	SHA1Context context;
	int idx = ((user - 1 ) * 2) + 1;


	hexStrTobyteStream(sha1_value, passwd, new);

	memcpy(_passwd, sha1_value, SHA1HashSize);
	memcpy(_new, sha1_value + SHA1HashSize, SHA1HashSize);

	/*
	SHA1Reset(&context);
	SHA1Input(&context,passwd, strlen(passwd));
	SHA1Result(&context, _passwd);

	SHA1Reset(&context);
	SHA1Input(&context,new, strlen(new));
	SHA1Result(&context, _new);
	*/

	// read data
	//fp = fopen("/boa/www/data/account.dat","rb");
	fp = fopen(ACCOUNT_DAT_FILE,"rb");

	if(fp == NULL)
		return 0x01;

	fread(saved_value, sizeof(uint8_t), SHA1HashSize*4, fp);
	fclose(fp);
	fp = NULL;

	// check password
	if(memcmp(saved_value + (SHA1HashSize * idx), _passwd , SHA1HashSize))
		return 0x02;

	// change password
	if(!memcpy(saved_value + (SHA1HashSize * idx), _new , SHA1HashSize))
		return 0x03;

	// write data
	//fp = fopen("/boa/www/data/account.dat","wb");
	fp = fopen(ACCOUNT_DAT_FILE,"wb");

	if(fp == NULL)
		return 0x04;

	fwrite(saved_value, sizeof(uint8_t), SHA1HashSize*4, fp);
	fclose(fp);

	//system("cp /boa/www/data/account.dat /vivix/boa/www/");

	return 0x00;


}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
int initPassword(const int user, const char* id,const char* new)
{
	FILE* fp = NULL;

	int8_t _new[SHA1HashSize];
	int8_t _id[SHA1HashSize];
	int8_t saved_value[SHA1HashSize*4];
	SHA1Context context;
	int idx = ((user - 1 ) * 2) ;

	SHA1Reset(&context);
	SHA1Input(&context,id, strlen(id));
	SHA1Result(&context, _id);

	SHA1Reset(&context);
	SHA1Input(&context,new, strlen(new));
	SHA1Result(&context, _new);

	// read data
	//fp = fopen("/boa/www/data/account.dat","rb");
	fp = fopen(ACCOUNT_DAT_FILE,"rb");

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
	//fp = fopen("/boa/www/data/account.dat","wb");
	fp = fopen(ACCOUNT_DAT_FILE,"wb");

	if(fp == NULL)
		return 0x01;

	fwrite(saved_value, sizeof(uint8_t), SHA1HashSize*4, fp);
	fclose(fp);

	//system("cp /boa/www/data/account.dat /vivix/boa/www/");

	return 0x00;


}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool checkConnect(char* remoteIP)
{
	char fname[256];
	char buf[256];
	char cmd[256];
	char ip[20];
	FILE* pFile;
	DIR* dir_info;
	struct dirent* dir_entry;
	bool ret = false;
	time_t curTime = 0;
	time_t expireTime = 0;
	char* cur=NULL;

	dir_info = opendir("/tmp/");

	if(dir_info)
	{
		while(dir_entry = readdir(dir_info))
		{
			if((dir_entry->d_type == DT_REG) )
			{
				if(strstr(dir_entry->d_name, "qsession") && strstr(dir_entry->d_name, "properties"))
				{
					memset(fname, 0x00, 256);
					memset(buf, 0x00, 256);
					sprintf(fname,"/tmp/%s",dir_entry->d_name);
					pFile = fopen(fname, "r");
					if(pFile == NULL)
					{
						closedir(dir_info);
						return false;
					}
					
					while(fscanf(pFile,"%s",buf)> 0)
					{
						if(cur = strstr(buf,"ip"))
						{
							memset(ip,0x00, 20);					
							strcpy(ip, cur + 3);
							strchr(ip,'%')[0]='\0';	
							if(!strcmp(ip, remoteIP))
							{
								system("cp /tmp/qsession-* /tmp/aaa.properties.txt");
								system("rm -rf /tmp/qsession-*");
								ret = false;
							}
							else
								ret = true;
						}

						if(strstr(buf,"conn"))
						{
							ret = true;
						}
						memset(buf, 0x00, 256);
					}
					fclose(pFile);
					
				}

				if(strstr(dir_entry->d_name, "qsession") && strstr(dir_entry->d_name, "expire"))
				{
					memset(fname, 0x00, 256);
					memset(buf, 0x00, 256);
					sprintf(fname,"/tmp/%s",dir_entry->d_name);
					pFile = fopen(fname, "r");
					if(pFile == NULL)
					{
						closedir(dir_info);
						return false;
					}
					
					if(fscanf(pFile,"%s",buf)> 0)
					{
						expireTime = (time_t)atoi(buf);
						curTime = time(NULL);

						//printf("curTime : %d, expireTime : %d, cal : %d\n", curTime, expireTime,(int)curTime > (int)expireTime );
						if((int)curTime > (int)expireTime)
						{
							//delete cookie
							system("cp /tmp/qsession-* /tmp/aaa.expire.txt");
							system("rm -rf /tmp/qsession-*");
							ret = false;
							break;
						}
					}
					fclose(pFile);
					
				}
			}
		}
	}

	closedir(dir_info);
	return ret;

}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
int check_session(void)
{
	FILE* p_file;
	
	p_file = fopen( WEB_SESSION_TIMEOUT_FILE, "r");
	
	if( p_file == NULL )
	{
		return 0x04;
	}
	
	fclose(p_file);
	
	return 0x00;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void update_timeout(int nType)
{
	int fd;
	qData data;
	
	if ( -1 == ( fd = open(FIFO_FILE1, O_WRONLY)))
	{
		perror( "open() error");
		return;
	}
	
	memset( &data, 0, sizeof(qData) );
	
	data.type = nType;
	write( fd, &data, sizeof(qData) );
	close( fd);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
int main()
{
	int ret = 0;
	bool session = false;
#if 0
	qentry_t *pReq = qcgireq_parse(NULL,0);

	//int nType = pReq->getint(pReq, "TYPE");
	//int nCnt = pReq->getint(pReq, "CNT");
	//const char *pPARAMS = pReq->getstr(pReq, "PARAMS", false);

	qentry_t *pSession = qcgisess_init(pReq, NULL);
	//printf("111111111111");
	qcgires_setcontenttype(pReq, "text/xml");

	printf("<?xml version=\"1.0\"?>");
	printf("<RESPONSE>");
	printf("<VALUE>1</VALUE>");
	printf("<ETC>test.html</ETC>");
	printf("</RESPONSE>");

	if(pSession)
	{
		qcgisess_save(pSession);
		pSession->free(pSession);
	}
	pReq->free(pReq);

#else

	qentry_t *pReq = qcgireq_parse(NULL,0);

	int nType = pReq->getint(pReq, "TYPE");
	int nCnt = pReq->getint(pReq, "CNT");
	const char *pPARAMS = pReq->getstr(pReq, "PARAMS", false);

	qentry_t *pSession = qcgisess_init(pReq, NULL);
	qcgires_setcontenttype(pReq, "text/xml");

	char token[]="||";
	char* ptr;
	int len=0;
	char* id;
	char* passwd;
	char* newpasswd;
	char* login ;
	key_t key;
	qBuf mybuf;
	int fd;
	int fd2;
	char buff[1024];
	qData data;

	switch(nType){

		case EMSGTYPE_LOGIN:
			//id
			ptr = strtok((char*)pPARAMS,token);
			len = strlen(ptr) + 1;
			id = (char*)malloc(sizeof(char) * len);
			strcpy(id, ptr);

			//passwd
			ptr = strtok(NULL,token);
			len = strlen(ptr) + 1;
			passwd = (char*)malloc(sizeof(char) * len);
			strcpy(passwd, ptr);

			//check id, password and start login logic
			int user;
			ret = checkIDandPASSWD(id, passwd, &user);
			if(ret == 0)
			{
				if(checkConnect(getenv("REMOTE_ADDR")))
				{
				//	qcgisess_destroy(pSession);
					pSession->free(pSession);
					pSession = NULL;
					makeResponse(0x06,"error.html");
					break;
				}
				makeResponse(user,"vieworks.html");
				qcgisess_settimeout(pSession, 1800);
				pReq->putstr(pSession, "conn",id, true);
				pReq->putstr(pSession, "ip",getenv("REMOTE_ADDR"), true);
				update_timeout(nType);
			}
			else
			{
				//qcgisess_destroy(pSession);
				pSession->free(pSession);
				pSession = NULL;
				makeResponse(ret,"error.html");
			}
			break;

		case EMSGTYPE_LOGOUT:
			//logout
			qcgisess_settimeout(pSession, 1);
			qcgisess_destroy(pSession);
			pSession = NULL;
			makeResponse(0x00,"");
			update_timeout(nType);
			return 0;
			break;
		case EMSGTYPE_UPDATE_SESSION_TIMEOUT:
			// update session timeout
			login = pReq->getstr(pSession, "conn", false);
			if(login)
			{
				ret = 0x00;
				qcgisess_settimeout(pSession, 1800);
				printf("update session\n");
			}
			else
			{
				ret = 0x04;
				qcgisess_destroy(pSession);
				pSession = NULL;
			}
			makeResponse(ret ,"");
			update_timeout(nType);

			break;
		case EMSGTYPE_CHECK_SESSION:
			// check session
#if 1
			ret = check_session();
#else
			login = pReq->getstr(pSession, "conn", false);
			if(login)
			{
				ret = 0x00;
				pSession->free(pSession);
			}
			else
			{
				ret = 0x04;
				qcgisess_destroy(pSession);
			}
#endif
			makeResponse(ret ,"");
			pReq->free(pReq);

			return 0;
		case EMSGTYPE_CHANGE_PASSWORD:
			//user id
			ptr = strtok((char*)pPARAMS,token);
			len = strlen(ptr) + 1;
			id = (char*)malloc(sizeof(char) * len);
			strcpy(id, ptr);

			//passwd
			ptr = strtok(NULL,token);
			len = strlen(ptr) + 1;
			passwd = (char*)malloc(sizeof(char) * len);
			strcpy(passwd, ptr);

			//newpasswd
			ptr = strtok(NULL,token);
			len = strlen(ptr) + 1;
			newpasswd = (char*)malloc(sizeof(char) * len);
			strcpy(newpasswd, ptr);


			// change password
			ret = changePassword(atoi(id),passwd, newpasswd);
			makeResponse(ret,"");
			if ( -1 == ( fd = open(FIFO_FILE1, O_WRONLY)))
			{
				perror( "open() error");
				break;
			}

			strcpy(data.text, pPARAMS);
			data.type = nType;
			data.cnt = nCnt;
			write( fd,&data, sizeof(qData));
			close( fd);

			break;
		case EMSGTYPE_INIT_PASSWORD:
			makeResponse(ret,"");
			ret = initPassword(atoi(pPARAMS), "user","user");
			if ( -1 == ( fd = open(FIFO_FILE1, O_WRONLY)))
			{
				perror( "open() error");
				break;
			}

			strcpy(data.text, pPARAMS);
			data.type = nType;
			data.cnt = nCnt;
			write( fd,&data, sizeof(qData));
			close( fd);

			break;
		default:
			makeResponse(0,data.text);
			if ( -1 == ( fd = open(FIFO_FILE1, O_WRONLY)))
			{
				perror( "open() error");
				break;
			}
			strcpy(data.text, pPARAMS);
			
			while((ptr = strchr(data.text,' ')) !=NULL )
				ptr[0]='+';

			data.type = nType;
			data.cnt = nCnt;
			write( fd,&data, sizeof(qData));
			close( fd);
			break;

	}

//	pReq->print(pSession, stdout, true);

	if(pSession)
	{
		qcgisess_save(pSession);
		pSession->free(pSession);
	}
	pReq->free(pReq);
#endif
	return 0;
}
