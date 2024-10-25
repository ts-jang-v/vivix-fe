#ifndef _VW_TIME_H_
#define _VW_TIME_H_

#include "typedef.h"

s32 sleep_ex( s64 i_n_usec );

class CTime
{
private:
    struct timespec m_wait_t;

public:
	void    set(u32 i_n_msec);
	void    set_micro(u32 i_n_usec);
	bool    is_expired(void);
	u32		get_remain_time(void);
	
	CTime(u32 i_n_msec);
	CTime(void);
	~CTime(void);
};

#endif /* end of _VW_TIME_H_ */

