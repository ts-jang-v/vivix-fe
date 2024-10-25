#include <iostream>
#include <sys/time.h>
#include <errno.h>          // errno
#include <stdio.h>
#include <string.h>
#include "vw_time.h"


using namespace std;

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
CTime::CTime(void)
{
    m_wait_t.tv_sec = 0;
    m_wait_t.tv_nsec = 0;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
CTime::~CTime(void)
{
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
CTime::CTime(u32 i_n_msec)
{
    set(i_n_msec);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CTime::set(u32 i_n_msec)
{
    //struct timespec now;
    
    if( clock_gettime(CLOCK_MONOTONIC, &m_wait_t) < 0 )
	{
	    cerr << "[CTime] clock_gettime error\n";
	}
	else
	{
	    m_wait_t.tv_sec = m_wait_t.tv_sec + i_n_msec/1000;
    	m_wait_t.tv_nsec = m_wait_t.tv_nsec + ((i_n_msec % 1000) * 1000000);
    	
    	if( m_wait_t.tv_nsec >= 1000000000 )
    	{
    	    m_wait_t.tv_sec++;
    	    m_wait_t.tv_nsec -= 1000000000;
    	}
    	//printf("[CTime] target sec: %ld, nsec: %ld\n", m_wait_t.tv_sec, m_wait_t.tv_nsec);
	}
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CTime::is_expired(void)
{
    struct timespec now;
    struct timespec timeout;
    
 	if( clock_gettime(CLOCK_MONOTONIC, &now) < 0 )
	{
	    cerr << "[CTime] is_expired: clock_gettime error\n";
	    return true;
	}
	else
	{
	    
    	timeout.tv_sec = m_wait_t.tv_sec - now.tv_sec;
    	timeout.tv_nsec = m_wait_t.tv_nsec - now.tv_nsec;
    	
    	if( timeout.tv_nsec < 0 )
    	{
    	    timeout.tv_nsec += 1000000000;
    	    --timeout.tv_sec;
    	}
    	
    	if( timeout.tv_sec < 0 )
    	{
    	    //printf("[CTime] expired sec: %ld, nsec: %ld\n", now.tv_sec, now.tv_nsec);
    	    return true;
    	}
	}
   
	return false;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u32 CTime::get_remain_time(void)
{
    struct timespec now;
    struct timespec remain_time;
    
 	if( clock_gettime(CLOCK_MONOTONIC, &now) < 0 )
	{
	    cerr << "[CTime] is_expired: clock_gettime error\n";
	    return 0;
	}
	else
	{
	    
    	remain_time.tv_sec = m_wait_t.tv_sec - now.tv_sec;
    	remain_time.tv_nsec = m_wait_t.tv_nsec - now.tv_nsec;
    	
    	if( remain_time.tv_nsec < 0 )
    	{
    	    remain_time.tv_nsec += 1000000000;
    	    --remain_time.tv_sec;
    	}
    	
    	if( remain_time.tv_sec < 0 )
    	{
    	    return 0;
    	}
	}
   
	return (remain_time.tv_sec * 1000 + remain_time.tv_nsec / 1000000);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CTime::set_micro(u32 i_n_usec)
{
    if( clock_gettime(CLOCK_MONOTONIC, &m_wait_t) < 0 )
	{
	    cerr << "[CTime] clock_gettime error\n";
	}
	else
	{
	    m_wait_t.tv_nsec = m_wait_t.tv_nsec + (i_n_usec * 1000);
    	
    	if( m_wait_t.tv_nsec >= 1000000000 )
    	{
    	    m_wait_t.tv_sec++;
    	    m_wait_t.tv_nsec -= 1000000000;
    	}
    	//printf("[CTime] target sec: %ld, nsec: %ld\n", m_wait_t.tv_sec, m_wait_t.tv_nsec);
	}
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
s32 sleep_ex( s64 i_n_usec )
{
	struct timespec req, rem;
	int rtn;

	req.tv_sec = i_n_usec / 1000000;
	req.tv_nsec = ( i_n_usec % 1000000 ) * 1000;

	do
    {
    	rtn = nanosleep( & req, & rem );

    	if( rtn < 0 && errno == EINTR )
        {
        	req = rem;
        }
    }
	while( rtn < 0 && errno == EINTR );

	return rtn;
}
