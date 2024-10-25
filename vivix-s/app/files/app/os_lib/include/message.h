#ifndef _MESSAGE_H_
#define _MESSAGE_H_

#include "typedef.h"
#include "event.h"

#define VW_PARAM_MAX_LEN		512

typedef struct _VW_MSG_
{
    u16     type;
	u16		size;
	u8		contents[VW_PARAM_MAX_LEN];
}vw_msg_t;

class CQueue
{
public:
	int		size;
	int		rear;
	int		front;
	vw_msg_t * buffer;
	
	CQueue(void);	
	~CQueue(void);
	
	void createq(int qsize);	
	bool addq(vw_msg_t *  item);
	bool deleteq(void);
	bool emptyq(void);
	int get_qcount(void);
};


class CMessage
{
private:
	CQueue  m_queue_c;
	CEvent  m_event_c;
	
	
public:
	vw_msg_t * get(int wait);
	bool add(vw_msg_t * msg);
	
	CMessage(void);
	~CMessage(void);
};


#endif /* _MESSAGE_H_ */

