/*
 * typedef.h
 *
 *  Created on: 2014. 2. 13.
 *      Author: 2012121
 */

#ifndef _TYPEDEF_H_
#define _TYPEDEF_H_


/***************************************************************************************************
*	Include Files
***************************************************************************************************/
#include <stdbool.h>    // bool, true, false


/***************************************************************************************************
*	Constant Definitions
***************************************************************************************************/
#define HIGH	1
#define LOW		0


/***************************************************************************************************
*	Type Definitions
***************************************************************************************************/
typedef char					    s8;

typedef unsigned char				u8;

typedef signed short				s16;

typedef unsigned short				u16;

typedef signed int					s32;

typedef unsigned int				u32;

typedef signed long long			s64;

typedef unsigned long long			u64;

typedef enum
{
	ERROR = 0,
	OPERATE,
	DIAG,
	INFO,
	DEBUG,
} LOG_LEVEL;

#define		DEBUG_INFO			__FILE__,__LINE__,__func__

#define		safe_free(p)		if(p!=NULL) {free(p);p=NULL;}

#ifndef __cplusplus 
#ifndef bool //org
typedef enum
{
    false,
    true
}bool;
#endif
#endif

#define        DEBUG_INFO            __FILE__,__LINE__,__func__


#define        safe_free(p)            if(p!=NULL) {free(p);p=NULL;}
#define        safe_delete(p)            if(p!=NULL) {delete p;p=NULL;}
#define        safe_delete_array(p)    if(p!=NULL) {delete [] p;p=NULL;}
#define		   safe_free(p)		        if(p!=NULL) {free(p);p=NULL;}
//// bool 선언은 enum 규칙 예외
//#ifndef false
//typedef enum
//{
//	false,
//	true
//}bool;
//#endif




/***************************************************************************************************
*	Macros (Inline Functions) Definitions
***************************************************************************************************/


/***************************************************************************************************
*	Variable Definitions
***************************************************************************************************/



/***************************************************************************************************
*	Function Prototypes
***************************************************************************************************/




#endif /* _TYPEDEF_H_ */
