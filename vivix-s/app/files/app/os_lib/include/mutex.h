#ifndef MUTEX_CLASS
#define MUTEX_CLASS

class CMutex
{
private:
	pthread_mutex_t m_mutex;
public:
	void lock();
	void unlock();
	CMutex(void);
	~CMutex(void);
};

#endif

