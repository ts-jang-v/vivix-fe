#ifndef __DISPATCH_H
#define __DISPATCH_H

#include <linux/fs.h>
#include <linux/poll.h>

int vivix_tg_open(struct inode *inode,struct file  *filp);
int vivix_tg_release(struct inode *inode,struct file  *filp);
ssize_t vivix_tg_read(struct file *filp,char *buff,size_t count,loff_t *offp);
ssize_t vivix_tg_write(struct file *filp,const char  *buff,size_t count,loff_t *offp);
long vivix_tg_ioctl(struct file  *filp, unsigned int  cmd, unsigned long  args);
int vivix_tg_mmap(struct file *filp, struct vm_area_struct *vma);
unsigned int vivix_tg_poll(struct file * filp,poll_table * wait);

#endif /* __DISPATCH_H */

