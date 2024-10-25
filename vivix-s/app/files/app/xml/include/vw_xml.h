/*
 * vw_xml.h
 *
 *  Created on: 2013. 12. 17.
 *      Author: myunghee
 */

 
#ifndef VW_XML_H_
#define VW_XML_H_

/*******************************************************************************
*	Include Files
*******************************************************************************/
#include "typedef.h"
#include "mxml.h"

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
class CVWXml
{
private:
    int			    (* print_log)(int level,int print,const char * format, ...);
    
	MXML_DOCUMENT* 	m_p_doc;
    
    MXML_NODE*      find_element(const s8* i_p_name);
    s8*             get_attr_string( MXML_NODE* i_p_node, const s8* i_p_attr_name );
    bool            set_attr_string( MXML_NODE* i_p_node, const s8* i_p_attr_name, const s8* i_p_value );

protected:
public:
    bool            get_attribute(const s8* i_p_element, const s8* i_p_attribute, s8* o_p_value);
    bool            set_attribute(const s8* i_p_element, const s8* i_p_attribute, const s8* i_p_value);
    
    bool            load_from_file(const s8* i_p_name);
    bool            save_to_file(const s8* i_p_name);
    
    CVWXml(int (*log)(int,int,const char *,...));
	CVWXml(void);
	~CVWXml();
};


#endif /* end of VW_XML_H_*/

