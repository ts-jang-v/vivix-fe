#include <iostream>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>

#include "event.h"
#include "mutex.h"
#include "message.h"

using namespace std;

#define MESSAGE_QUEUE_SIZE          (20)


CQueue::CQueue(void)
{
}

CQueue::~CQueue(void)
{
	size = rear = front = 0;
	safe_delete_array( buffer );
}

void CQueue::createq(int qsize)
{
	size = qsize + 1;
	rear = front = 0;
	buffer = new vw_msg_t[size];
	memset(buffer,0,sizeof(vw_msg_t) * size); 
}

bool CQueue::addq(vw_msg_t *  item)
{
	memcpy(&buffer[rear],item,sizeof(vw_msg_t));
	rear = (rear + 1) % size;
	if(front == rear)
		return false;
	
	return true;
}

bool CQueue::deleteq(void)
{
	if(front == rear)
		return false;
	
	front = (front + 1) % size;
	
	return true;
}

bool CQueue::emptyq(void)
{
	if(front == rear)
		return true;
	
	return false;
}

int CQueue::get_qcount(void)
{
	if(rear < front)
		return ((size - front) + rear);
	return (rear - front);
}


CMessage::CMessage(void)
{
    m_queue_c.createq(MESSAGE_QUEUE_SIZE);
}

CMessage::~CMessage(void)
{
}

vw_msg_t * CMessage::get(int wait)
{
    vw_msg_t * msg;
    
    if( m_queue_c.get_qcount() > 0 )
    {
        m_event_c.set();
        
        msg = new vw_msg_t;
        memcpy( msg, &m_queue_c.buffer[m_queue_c.front], sizeof(vw_msg_t) );
        m_queue_c.deleteq();
        
        m_event_c.reset();
        
        //printf("msg pop\n");
        
        return msg;
    }
    
    if( m_event_c.wait( wait ) == false )
    {
       m_event_c.reset();
       return NULL;
    }
    
    msg = new vw_msg_t;
    memcpy( msg, &m_queue_c.buffer[m_queue_c.front], sizeof(vw_msg_t) );
    m_queue_c.deleteq();
    
    m_event_c.reset();
//printf("msg pop\n");
    
    return msg;
}

bool CMessage::add(vw_msg_t * msg)
{
    if( m_queue_c.get_qcount() >= MESSAGE_QUEUE_SIZE )
        return false;
    
    m_event_c.set();
    
    m_queue_c.addq( msg );
  
    m_event_c.raise();
    return true;
}


