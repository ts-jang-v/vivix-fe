#include <string.h>
#include "log_conf.h"
#include "CommonDef.h"




/***********************************************************************************************//**
* \brief	set value
* \param
* \return	
* \note
***************************************************************************************************/
void Log_conf::set_value(char *log_name, char *log_value)
{
	int i = 0;

			
	strlwr(log_name);
	for(i = 0; i < LOG_CONF_MAX; i++)
	{
		if(strcmp(parse[i].value_name,log_name) == 0)
		{
			switch(parse[i].value_type)
			{
				case LOG_TYPE_INT:
					*(int*)parse[i].value_pointer = atoi(log_value);
					break;
				case LOG_TYPE_STRING:
					strcpy((char*)parse[i].value_pointer,log_value);
					break;
				case LOG_TYPE_BOOL:
					strlwr(log_value);
					if(strcmp(log_value,"on") == 0)
					{
						*(bool*)parse[i].value_pointer = true;
					}
					else if(strcmp(log_value,"off") == 0)
					{
						*(bool*)parse[i].value_pointer = false;
					}
					else
					{
						printf("[Log_conf] invalid format : %s	%s\n",log_name,log_value);
					}
				default:
					printf("[Log_conf] invalid format : %s	%s\n",log_name,log_value);
					break;
			}
		}
	}
}
/***********************************************************************************************//**
* \brief	set value
* \param
* \return	
* \note
***************************************************************************************************/
void Log_conf::set_value(int type_num, char *log_value)
{
	switch(parse[type_num].value_type)
	{
		case LOG_TYPE_INT:
			*(int*)parse[type_num].value_pointer = atoi(log_value);
			break;
		case LOG_TYPE_STRING:
			strcpy((char*)parse[type_num].value_pointer,log_value);
			break;
		case LOG_TYPE_BOOL:
			strlwr(log_value);
			if(strcmp(log_value,"on") == 0)
			{
				*(bool*)parse[type_num].value_pointer = true;
			}
			else if(strcmp(log_value,"off") == 0)
			{
				*(bool*)parse[type_num].value_pointer = false;
			}
			else
			{
				printf("[Log_conf] invalid format : %d	%s\n",type_num,log_value);
			}
		default:
			printf("[Log_conf] invalid format : %d	%s\n",type_num,log_value);
			break;
		
	}
}

/***********************************************************************************************//**
* \brief	set value
* \param
* \return	
* \note
***************************************************************************************************/
void	Log_conf::make_default_config(int level, char* location, char* filename, int capacity)
{
	FILE* p_file;
	p_file = fopen("./logd.conf","w");
	
	fprintf( p_file, "%s\t%d\n", "level",level);
	if(location != NULL)
		fprintf( p_file, "%s\t%s\n", "location",location);
	if(filename != NULL)
		fprintf( p_file, "%s\t%s\n", "filename",filename);
	fprintf( p_file, "%s\t%d\n", "capacity",capacity);
	fclose(p_file);

	system("cp ./logd.conf /vivix/");
}
