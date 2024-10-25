#include <iostream>
#include "mutex.h"
using namespace std;

CMutex::CMutex(void)
{
   pthread_mutexattr_t mattr;
   pthread_mutexattr_init( &mattr );
   pthread_mutex_init(&m_mutex,&mattr);


}

CMutex::~CMutex(void)
{
	pthread_mutex_lock(&m_mutex);
	pthread_mutex_unlock(&m_mutex); 
	pthread_mutex_destroy(&m_mutex);
}

void CMutex::lock()
{
	pthread_mutex_lock(&m_mutex);
}

void CMutex::unlock()
{
   pthread_mutex_unlock(&m_mutex);
}

