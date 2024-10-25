#include <iostream>
#include <sys/time.h>
#include <errno.h>          // errno
#include <stdio.h>
#include <string.h>
#include "event.h"


using namespace std;

CEvent::CEvent(void)
{
	pthread_mutexattr_t mattr;
	
	pthread_mutexattr_init(&mattr);
	pthread_mutex_init(&m_lock,&mattr);
	pthread_cond_init(&m_ready,NULL);
}

CEvent::~CEvent(void)
{
	pthread_cond_destroy(&m_ready);
	pthread_mutex_destroy(&m_lock);
}


void CEvent::set()
{
	pthread_mutex_lock(&m_lock);
}

void CEvent::raise()
{
	pthread_cond_signal(&m_ready);
	reset();
}

bool CEvent::wait()
{

	try
	{
		pthread_mutex_lock(&m_lock);
		pthread_cond_wait(&m_ready,&m_lock);
		return true;
	}
	catch( char *psz )
	{
		cerr << "Fatal exception CEvent::wait: " << psz;
	}
	return true;
}

bool CEvent::wait(int time)
{
	int					ret;
	struct timeval      now;
	struct timespec 	timeout;

	try
	{
		gettimeofday(&now, NULL); 
		
        timeout.tv_sec = now.tv_sec + time/1000;
        timeout.tv_nsec = now.tv_usec * ((time % 1000) * 1000000);
    	
    	if( timeout.tv_nsec >= 1000000000 )
    	{
    	    timeout.tv_sec++;
    	    timeout.tv_nsec -= 1000000000;
    	}
    	
    	pthread_mutex_lock(&m_lock);
    	ret = pthread_cond_timedwait(&m_ready,&m_lock,&timeout);
    	
    	if(ret != 0)
    	{
    	    return false;
    	}
    	return true;
    	
	}
	catch( char *psz )
	{
		cerr << "Fatal exception CEvent::wait: " << psz;
	}
	return true;
}

void CEvent::reset()
{
	try 
	{
		pthread_mutex_unlock(&m_lock);
	}
	catch( char *psz )
	{
		cerr << "Fatal exception CEventClass::Reset: " << psz;
	}
}

