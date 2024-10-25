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
#include "vw_xml.h"

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
* \brief        constructor
* \param        none
* \return       none
* \note
*******************************************************************************/
CVWXml::CVWXml(int (*log)(int,int,const char *,...))
{
    print_log = log;
    m_p_doc = NULL;
}
/******************************************************************************/
/**
* \brief        constructor
* \param        none
* \return       none
* \note
*******************************************************************************/
CVWXml::CVWXml(void)
{
    m_p_doc = NULL;
}

/******************************************************************************/
/**
* \brief        destructor
* \param        none
* \return       none
* \note
*******************************************************************************/
CVWXml::~CVWXml(void)
{
    if( m_p_doc != NULL )
    {
        mxml_document_destroy( m_p_doc );
    }
}

/******************************************************************************/
/**
* \brief        이름 문자열로 element를 찾아준다.
* \param        a_pname     element name
* \return       none
* \note         중복된 element를 trace하지 못한다. 수정 필요
*******************************************************************************/
MXML_NODE* CVWXml::find_element(const s8* i_p_name)
{
    MXML_ITERATOR iter;
    
    MXML_NODE *node;
    
    mxml_iterator_setup( &iter, m_p_doc );
    node = mxml_iterator_scan_node( &iter, (s8*)i_p_name );
    
    return node;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
#if 0
MXML_NODE* CVWXml::find_next_element(const s8* i_p_name)
{
    while(1)
	{
		mxml_iterator_next( &m_iterator );
		if(m_iterator.node != NULL)
		{
			if( strcmp( m_iterator.node->name, i_p_name ) == 0 )
			{
				return m_iterator.node;
			}
		}
		else
		{
			break;
		}
		
		if(m_iterator.node->next == NULL)
		{
		    break;
		}
	}

	return NULL;
}
#endif

/******************************************************************************/
/**
* \brief        node에서 요청하는 attribute의 값을 찾아준다.
* \param        i_p_node           node
* \param        i_p_attr_name      attribute 문자열
* \return       attribute의 값(문자열)
* \note
*******************************************************************************/
s8* CVWXml::get_attr_string( MXML_NODE* i_p_node, const s8* i_p_attr_name )
{
	MXML_ATTRIBUTE * attr;

	assert(i_p_node);

	attr = i_p_node->attributes;

	while( attr != NULL )
	{
		if(strcmp(attr->name,i_p_attr_name) == 0)
		{
			return attr->value;
		}
		attr = attr->next;
	}

	return NULL;
}

/******************************************************************************/
/**
* \brief        node에서 요청하는 attribute의 값을 설정한다.
* \param        i_p_node           node
* \param        i_p_attr_name      attribute 문자열
* \return       attribute의 값(문자열)
* \note
*******************************************************************************/
bool CVWXml::set_attr_string( MXML_NODE* i_p_node, const s8* i_p_attr_name, const s8* i_p_value )
{
	MXML_ATTRIBUTE * attr;

	assert(i_p_node);

	attr = i_p_node->attributes;

	do
	{
		if( strcmp( attr->name, i_p_attr_name ) == 0 )
		{
			free(attr->value);
			
			attr->value = (s8*)malloc( sizeof(s8) * strlen(i_p_value) + 1 );
			strcpy( attr->value, i_p_value );
			
			return true;
		}
		attr = attr->next;
	}while(attr != NULL);
	
	print_log(ERROR, 1, "[XML] attribute(%s) is not found.\n", i_p_attr_name);
	return false;
}

/******************************************************************************/
/**
* \brief        attribute의 값을 문자열로 알려준다.
* \param        i_p_element     찾으려는 element의 이름
* \param        i_p_attribute   attribute name
* \return       o_p_value       attribute의 값
* \note
*******************************************************************************/
bool CVWXml::get_attribute(const s8* i_p_element, const s8* i_p_attribute, s8* o_p_value)
{
    MXML_NODE* node = find_element( i_p_element );
    s8* p_value;
    
    if( node == NULL )
    {
        print_log(ERROR, 1, "[XML] element(%s) is not found.\n", i_p_element);
        return false;
    }
    
    p_value = get_attr_string( node, i_p_attribute );
    if( p_value != NULL )
    {
        strcpy( o_p_value, p_value );
        return true;
    }
    
    print_log(ERROR, 1, "[XML] attribute (%s) value is not defined.\n", i_p_attribute);
    return false;
}

/******************************************************************************/
/**
* \brief        attribute의 값을 설정한다.
* \param        i_p_element     찾으려는 element의 이름
* \param        i_p_attribute   설정하려는 attribute 이름
* \param        i_p_value       설정하려는 attribute의 값
* \return       성공 시 true, 실패 시 false
* \note
*******************************************************************************/
bool CVWXml::set_attribute(const s8* i_p_element, const s8* i_p_attribute, const s8* i_p_value)
{
    //return set_attr_string( node, attr, value );
    return false;
}

/******************************************************************************/
/**
* \brief        xml 파일을 parsing하여 메모리에 올린다.
* \param        a_pname     파일 이름
* \return       성공 시 true, 실패 시 false
* \note
*******************************************************************************/
bool CVWXml::load_from_file(const s8* i_p_name)
{
    FILE *fp;
    m_p_doc = mxml_document_new();
    
    fp = fopen( i_p_name, "r" );
	
	if(fp == NULL)
	{
	   print_log(ERROR, 1, "%s:%d %s [XML] xml file(%s) open error.\n", DEBUG_INFO, i_p_name);
	   return false;
	}

	mxml_read_file( fp, m_p_doc, 0 );
	
	fclose(fp);

 	if ( m_p_doc->status == MXML_STATUS_ERROR ) 
	{
	    print_log(ERROR, 1, "[XML] ERROR while reading the document: (%d) %s\n",
	                        m_p_doc->error, mxml_error_desc( m_p_doc->error ) );
	    
	    return false;
	}
	else if ( m_p_doc->status == MXML_STATUS_MALFORMED ) 
	{
	    print_log(ERROR, 1, "[XML] Invalid XML document. Line %d: (%d) %s\n",
	                        m_p_doc->iLine, m_p_doc->error, mxml_error_desc( m_p_doc->error ) );
        
        return false;
	}
	
    return true;
}

/******************************************************************************/
/**
* \brief        메모리에 있는 xml을 파일로 저장한다.
* \param        a_pname     파일 이름
* \return       성공 시 true, 실패 시 false
* \note
*******************************************************************************/
bool CVWXml::save_to_file(const s8* i_p_name)
{
    MXML_STATUS status = mxml_write( m_p_doc, (s8*)i_p_name, MXML_STYLE_INDENT | MXML_STYLE_THREESPACES );
    
    if( status != MXML_STATUS_OK )
    {
        print_log(ERROR, 1, "[XML] xml file(%s) write is failed.\n", i_p_name);
        return false;
    }
    
    return true;
}
