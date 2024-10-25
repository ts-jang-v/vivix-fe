#ifndef __LOG_CONF__
#define __LOG_CONF__

/***************************************************************************************************
  * include Files																			
***************************************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/***************************************************************************************************
  *	Constant Definitions																			
  ***************************************************************************************************/
	
#define LOG_CONF_MAX	6
/***************************************************************************************************
  *	Type Definitions																			
  ***************************************************************************************************/
enum VALUE_TYPE{
	LOG_TYPE_INT,
	LOG_TYPE_STRING,
	LOG_TYPE_BOOL
};

enum CONF_TYPE{
	CONF_LEVEL,
	CONF_LOCATION,
	CONF_FILENAME,
	CONF_COMPRESS,
	CONF_CAPACITY,
	CONF_PERIOD
};


/***************************************************************************************************
  *	Macros (Inline Functions) Definitions														
  ***************************************************************************************************/


/***************************************************************************************************
  *	Variable Definitions																			
  ***************************************************************************************************/




class Log_parse_info
{
	public:	
		char 	value_name[20];
		char 	value_type;	//0 : int, 1 : string , 2 : bool
//		int  number;
		void 	*value_pointer;

		void init(const char* name, char type, void *pointer)
		{
			strcpy(value_name,name);
			value_type = type;
			value_pointer = pointer;
		}


};

class Log_conf
{
	private:
		int 	level;
		char 	location[30];
		char 	filename[30];
		int 	capacity;
		Log_parse_info parse[LOG_CONF_MAX]; 
	public:

		


		Log_conf()
		{
			level = 3;
			strcpy(location,"/tmp/");
			capacity = 100;
			memset(filename, 0x00, 30);

						//value_name	//type	//pointer
			parse[0].init( "level"		, 0		,(void*)&level);
			parse[1].init( "location"	, 1		,(void*)location);
			parse[2].init( "filename"	, 1		,(void*)filename);
			parse[3].init( "capacity"	, 0		,(void*)&capacity);

		}

		void	make_default_config(int level, char* location, char* filename, int capacity);
		void	set_value(char *log_name, char *log_value);
		void	set_value(int type_num,char *log_value);
		void 	set_level(int lev) { level = lev; }
		int		get_level()	{	return level; }
		char* 	get_location() { return location; }
		char* 	get_filename() { return filename; }
		int 	get_capacity() { return capacity; }
};





#endif
