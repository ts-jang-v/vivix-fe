/*******************************************************************************
 모  듈 : xml
 작성자 : 한명희
 버  전 : 0.0.1
 설  명 : mxml을 c++로 wrapping
 참  고 : 

 버  전:
         0.0.1  최초 작성
            사용법: 
            - load_from_file로 XML 파일을 읽어 들인다.
            - get_attr_string/set_attr_string으로 attribute의 값을 다룬다.
            - save_to_file로 XML 파일을 저장한다.
*******************************************************************************/



/*******************************************************************************
*	Include Files
*******************************************************************************/
#include <iostream>
#include <stdio.h>			// fprintf() fopen, fclose
#include <assert.h>			// assert()
#include <string.h>
#include <stdlib.h>			// malloc(), free(),exit(), EXIT_SUCCESS rand/rand 
#include <stdbool.h>		// bool, true, false
#include "vw_xml_util.h"

using namespace std;

/*******************************************************************************
*	Constant Definitions
*******************************************************************************/

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
MXML_NODE *xml_find_element(MXML_DOCUMENT *doc, const char * elem)
{
   MXML_ITERATOR iter;
   MXML_NODE *node;

   mxml_iterator_setup( &iter, doc );

   node = mxml_iterator_scan_node( &iter, (char*)elem );


   return node;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
char *xml_get_attr_string(MXML_NODE * node, const char * attr_name)
{
	MXML_ATTRIBUTE * attr;

	assert(node);

	attr = node->attributes;

	do
	{
		if(strcmp(attr->name,attr_name) == 0)
		{
			return attr->value;
		}
		attr = attr->next;
	}while(attr != NULL);


	return NULL;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
int xml_set_attr_string(MXML_NODE * node, const char * attr_name, const char * value)
{
	MXML_ATTRIBUTE * attr;

	assert(node);

	attr = node->attributes;

	do
	{
		if(strcmp(attr->name,attr_name) == 0)
		{
			free(attr->value);
			attr->value = (char*)malloc(sizeof(char) * strlen(value) + 1);
			strcpy(attr->value,value);
			return 1;
		}
		attr = attr->next;
	}while(attr != NULL);

	return 0;
}
