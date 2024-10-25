#ifndef __FUNCERROR_H
#define __FUNCERROR_H




#define API_RETURN_CODE_STARTS              0x200   /* Starting return code */

typedef unsigned char	BOOL;


/* API Return Code Values */
typedef enum _RETURN_CODE
{
	ApiSuccess = API_RETURN_CODE_STARTS,
	ApiFailed,
	ApiAccessDenied,
	ApiUnsupportedFunction,
} RETURN_CODE;









#endif /* __FUNCERROR_H */

