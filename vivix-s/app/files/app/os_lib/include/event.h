#ifndef _VW_EVENT_H_
#define _VW_EVENT_H_

class CEvent
{
private:
	pthread_cond_t m_ready;
	pthread_mutex_t m_lock;
public:
	void set();
	void raise();
	
	bool wait();
	bool wait(int time);
	void reset();
	CEvent(void);
	~CEvent(void);
};

#endif /* end of _VW_EVENT_H_ */

