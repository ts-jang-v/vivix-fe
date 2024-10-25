/******************************************************************************/
/**
 * @file    micom.cpp
 * @brief   micom 통신
 * @author  
*******************************************************************************/

/*******************************************************************************
*   Include Files
*******************************************************************************/
#include <iostream>
#include <string.h>			// memset(), memcpy()
#include <errno.h>			// errno
#include <termios.h>
#include <sys/stat.h>       // O_RDWR, open
#include <fcntl.h>          // O_RDWR, open
#include <sys/poll.h>		// struct pollfd, poll()
#include <sys/ioctl.h>  	// ioctl()

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "micom.h"
#include "vw_time.h"
#include "vworks_ioctl.h"
#include "vw_file.h"

using namespace std;

/*******************************************************************************
*   Type Definitions
*******************************************************************************/
typedef enum
{
    eREBOOT_STATUS_FIRST_POWER_ON = 0,
    eREBOOT_STATUS_SECOND_REBOOT,
    eREBOOT_STATUS_BY_REBOOT_COMMAND,
    eREBOOT_STATUS_BY_POWER_OFF_COMMAND,
    eREBOOT_STATUS_NO_RESP_BOOTING,			// booting 이 완료되지 않음
    eREBOOT_STATUS_NO_RESP_COMMAND,			// 주기적인 command 갱신이 없어 abnormal 상태로 전이됨
        
} REBOOT_STATUS;

/*******************************************************************************
*   Macros (Inline Functions) Definitions
*******************************************************************************/


/*******************************************************************************
*   Variable Definitions
*******************************************************************************/
static u8 s_c_send = 0;

/*******************************************************************************
*   Function Prototypes
*******************************************************************************/


/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
CMicom::CMicom(s32 (*log)(s32,s32,const s8 *,...))
{
	print_log = log;
	strcpy( m_p_log_id, "MICOM         " );
    
    m_n_fd = -1;
	m_n_fd_self = -1;
    
    m_b_debug = false;

    m_n_index = 0;
    m_p_data = NULL;
    m_c_crc = 0;
    
    m_p_mutex_c = new CMutex;
    m_b_msg_stop = false;
	m_b_update_started = false;
    
    print_log(DEBUG, 1, "[%s] CMicom\n", m_p_log_id);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
CMicom::CMicom(void)
{
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
CMicom::~CMicom()
{
    close( m_n_fd );
	close( m_n_fd_self );
    
    safe_delete( m_p_mutex_c );

    print_log(DEBUG, 1, "[%s] ~CMicom\n", m_p_log_id);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CMicom::initialize(void)
{
		m_n_fd = open( "/dev/vivix_spi", O_RDWR|O_NDELAY );
		if( m_n_fd < 0 )
		{
			print_log(ERROR,1,"[%s] /dev/vivix_spi(spi2) device open error.\n", m_p_log_id);
			m_n_fd = -1;
			return false;
		}
		print_log(DEBUG, 1, "[%s] spi port(communication) is opened.\n", m_p_log_id);


#if USE_MICOM_XYMODEM
		m_n_fd_self = open_serial("/dev/ttymxc1");

		if(m_n_fd_self < 0)
		{
			print_log(ERROR,1,"[%s] /dev/ttymxc1(uart2) open error(%d)\n", m_p_log_id, m_n_fd_self);
			return false;
		}
		print_log(DEBUG, 1, "[%s] uart port(self-programming) is opened.\n", m_p_log_id);

#else
		m_n_fd_self = open("/dev/ttymxc1", O_RDWR|O_NOCTTY);
		if(m_n_fd_self < 0)
		{
			print_log(ERROR,1,"[%s] /dev/ttymxc1(uart2) open error\n", m_p_log_id);
			return false;
		}
		print_log(DEBUG, 1, "[%s] uart port(self-programming) is opened.\n", m_p_log_id);
		struct termios 	oldIo, newIo;
		tcgetattr(m_n_fd_self, &oldIo);
		memset(&newIo, 0, sizeof(newIo));

		newIo.c_cflag = B115200 | CS8 | CLOCAL | CREAD;
		//newIo.c_cflag = B9600 | CS8 | CLOCAL | CREAD;
		//newIo.c_cflag = B4800 | CS8 | CLOCAL | CREAD;
		//newIo.c_cflag = B2400 | CS8 | CLOCAL | CREAD;
		newIo.c_iflag = IGNPAR;
		newIo.c_oflag = 0;
		newIo.c_lflag = 0;
		newIo.c_cc[VTIME] = 0;		// No wait time, return immediately
		newIo.c_cc[VMIN] = 0;		// No minimum size. return immediately
		tcflush(m_n_fd_self, TCIFLUSH);
		tcsetattr(m_n_fd_self, TCSANOW, &newIo);
		print_log(DEBUG, 1, "[%s] uart port is opened.\n", m_p_log_id);
#endif
		return true;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CMicom::recv( MICOM_EVENT* o_p_evnet_e, u8* o_p_event_data, u16 i_w_wait_time, bool big_cmd, u16 save_len)
{
	ioctl_data_t iod_t;
	u16 w_wait_time;
	u8 cmd_size, data_size;
	u8 rx_buf[COMMAND_BIG_BUFFER_LEN];
	bool ret;
	
	if(i_w_wait_time < 300)
	    w_wait_time = 300;
	else
	    w_wait_time = i_w_wait_time;
	CTime* p_wait_c = new CTime(w_wait_time);
	
	if(big_cmd){
		cmd_size = COMMAND_BIG_BUFFER_LEN;
		data_size = COMMAND_BIG_DATA_LEN;
	}
	else{
		cmd_size = COMMAND_BUFFER_LEN;
		data_size = COMMAND_DATA_LEN;
	}
	
	while(1){
	
		if(big_cmd)
			sleep_ex(MCU_RDELAY_LONG);
		else
			sleep_ex(MCU_RDELAY_SHORT);
	
		iod_t.size = cmd_size;
		memset(iod_t.data, 0, sizeof(iod_t.data));
		
		ioctl(m_n_fd, VW_MSG_IOCTL_SPI_RX, &iod_t);
		memcpy(rx_buf, iod_t.data, cmd_size);
	
	//rx some command
		if(rx_buf[0] == STX && rx_buf[1] != 0x00 && rx_buf[cmd_size - 1] == ETX){
			u8 sum = get_check_sum(&rx_buf[1], cmd_size - 3);
			if(sum != rx_buf[cmd_size - 2]){
				print_log(DEBUG, 1, "[Micom] packet(0x%02X) check sum error(calc: 0x%02X, recv: 0x%02X)\n", \
						rx_buf[1], sum, rx_buf[cmd_size - 2] );
				ret = false;
				break;
			}
			else{
				*o_p_evnet_e = (MICOM_EVENT)rx_buf[1];
				if(o_p_event_data != NULL){
					if(big_cmd && save_len > 0)
						memcpy(o_p_event_data, &rx_buf[COMMAND_HEADER_LEN], save_len);		//case over writing(eeprom read)
					else
						memcpy(o_p_event_data, &rx_buf[COMMAND_HEADER_LEN], data_size);
				}
				ret = true;
	
				if( s_c_send != rx_buf[1] && s_c_send != eMICOM_EVENT_EEPROM_REQUEST && rx_buf[1] != eMICOM_EVENT_EEPROM_DATA )
				{
					print_log(DEBUG, 1, "[%s] send recv error : 0x%02X, 0x%02X\n", m_p_log_id, s_c_send, rx_buf[1]);
				}
				
				#if 0
				print_log(DEBUG, 1, "[Micom] packet ok(%d:0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X)\n", \
				cmd_size,\
				rx_buf[0], rx_buf[1],\
				rx_buf[2], rx_buf[3],\
				rx_buf[4], rx_buf[5],\
				rx_buf[6], rx_buf[7],\
				rx_buf[8], rx_buf[9],\
				rx_buf[cmd_size - 2], rx_buf[cmd_size - 1]);
				#endif
				break;
			}
		}
	
	#if 0
	//rx nothing data(spi)
		else if(rx_buf[0] == STX && rx_buf[1] == 0x00 && rx_buf[cmd_size - 1] == ETX){
			if(!m_b_update_started && !m_b_msg_stop)
			{
				print_log(DEBUG, 1, "[Micom] invalid packet error(event: %#x, %d:0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X)\n", \
				*o_p_evnet_e,
				cmd_size,\
				rx_buf[0], rx_buf[1],\
				rx_buf[2], rx_buf[3],\
				rx_buf[4], rx_buf[5],\
				rx_buf[6], rx_buf[7],\
				rx_buf[8], rx_buf[9],\
				rx_buf[cmd_size - 2], rx_buf[cmd_size - 1]);
				ret = false;	//none data
			}
		}
	#endif
	
	//rx error data
		else{
			
			if(!m_b_update_started && !m_b_msg_stop)
			{
				print_log(DEBUG, 1, "[Micom] packet error(%d:0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X)\n", \
				cmd_size,\
				rx_buf[0], rx_buf[1],\
				rx_buf[2], rx_buf[3],\
				rx_buf[4], rx_buf[5],\
				rx_buf[6], rx_buf[7],\
				rx_buf[8], rx_buf[9],\
				rx_buf[cmd_size - 2], rx_buf[cmd_size - 1]);
				ret = false;
				break;
			}
		}
	
	//check timeout
		if(p_wait_c->is_expired() == true){
			if( !m_b_update_started && !m_b_msg_stop )
			{
				print_log(DEBUG, 1, "[Micom] recv retry time out\n");
			}
			ret = false;
			break;
		}
	}
	
	safe_delete(p_wait_c);
	return ret;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CMicom::send(u8 i_c_payload, u8* i_b_data, bool big_cmd)
{
	ioctl_data_t iod_t;
	s32 data_size;
	
	//print_log(DEBUG, 1, "[%s] send(0x%02X), %d\n", m_p_log_id, i_c_payload, big_cmd);
	s_c_send = i_c_payload;
	
	if(big_cmd){
		iod_t.size = COMMAND_BIG_BUFFER_LEN;
		data_size = COMMAND_BIG_DATA_LEN;
	}
	else{
		iod_t.size = COMMAND_BUFFER_LEN;
		data_size = COMMAND_DATA_LEN;
	}
	memset(&iod_t.data[2], 0, data_size);
	
	iod_t.data[0] = STX;
	iod_t.data[1] = i_c_payload;
	if(i_b_data != NULL)
	{
	    memcpy(&iod_t.data[2], i_b_data, data_size);
	}
	iod_t.data[data_size + 2] = get_check_sum(&iod_t.data[1], data_size + 1);
	iod_t.data[data_size + 3] = ETX;
	
	memcpy(m_resend_buf, iod_t.data, iod_t.size);	//resend backup
	m_resend_size = iod_t.size;
	
	#if 0
	print_log(DEBUG, 1, "[Micom] packet send(%d:0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X)\n", \
				iod_t.size,\
				iod_t.data[0], iod_t.data[1],\
				iod_t.data[2], iod_t.data[3],\
				iod_t.data[4], iod_t.data[5],\
				iod_t.data[6], iod_t.data[7],\
				iod_t.data[8], iod_t.data[9],\
				iod_t.data[iod_t.size - 2], iod_t.data[iod_t.size - 1]);
	#endif
	
	if(big_cmd)
		sleep_ex(MCU_RDELAY_LONG);
	else
		sleep_ex(MCU_RDELAY_SHORT);
	
	ioctl(m_n_fd, VW_MSG_IOCTL_SPI_TX, &iod_t);
	
	return true;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u8 CMicom::get_check_sum( u8* i_p_buffer, u16 i_w_len )
{
	u8	c_sum = 0;
	u16 i;
	
	for( i = 0; i < i_w_len; i++)
	{
		c_sum += i_p_buffer[i];
	}
	
	return c_sum;
}

/******************************************************************************/
/**			X/Y Modem
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
int CMicom::open_serial(const char *path)
{
		int fd;
		struct termios tty;

		fd = open(path, O_RDWR | O_SYNC);
		if(fd < 0){
			//perror("open");
			return -errno;
		}

		memset(&tty, 0, sizeof(tty));
		if(tcgetattr(fd, &tty) != 0){
			//perror("tcgetattr");
			return -errno;
		}

		cfsetospeed(&tty, B115200);
		cfsetispeed(&tty, B115200);

		tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;	// 8-bit chars
		tty.c_iflag &= ~IGNBRK;						// disable break processing
		tty.c_lflag = 0;							// no signaling chars, no echo,
													// no canonical processing
		tty.c_oflag = 0;							// no remapping, no delays
		tty.c_cc[VMIN]  = 1;						// read doesn't block
		tty.c_cc[VTIME] = 5;						// 0.5 seconds read timeout

		tty.c_iflag &= ~(IXON | IXOFF | IXANY);		// shut off xon/xoff ctrl

		tty.c_cflag |= (CLOCAL | CREAD);			// ignore modem controls,
													// enable reading
		tty.c_cflag &= ~(PARENB | PARODD);			// shut off parity
		tty.c_cflag &= ~CSTOPB;
		tty.c_cflag &= ~CRTSCTS;

		if(tcsetattr(fd, TCSANOW, &tty) != 0){
			//perror("tcsetattr");
			return -errno;
		}
		return fd;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u16 crc_update(u16 crc_in, int incr)
{
	u16 _xor = crc_in & 0x8000;		//u16 _xor = crc_in >> 15;
	u16 out = crc_in << 1;
	
	if(incr)
		out++;
	if(_xor)
		out ^= CRC_POLY;
	return out;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u16 crc16(const u8 *data, u16 size)
{
	u16 crc, i;
	
	for(crc = 0; size > 0; size--, data++){
		for (i = 0x80; i; i >>= 1)
			crc = crc_update(crc, *data & i);
	}
	
	for(i = 0; i < 16; i++)
		crc = crc_update(crc, 0);
	
	return crc;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u16 swap16(u16 in)
{
	return (in >> 8) | ((in & 0xff) << 8);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
int CMicom::xymodem_send(int serial_fd, const s8 *filename, int protocol, int wait, u16 *progress)
{
		size_t len;
		int ret;
		u8 answer;
		struct stat stat;
		u8 *buf;
		u8 eof = X_EOF;
		struct xmodem_chunk chunk;
		int skip_payload = 0;
		int fd;

		fd = open(filename, O_RDONLY);
		if(fd < 0){
			print_log(DEBUG, 1, "[%s] file(%s) open failed.\n", m_p_log_id, filename);
			return -1;
		}

		fstat(fd, &stat);
		len = stat.st_size;
		buf = static_cast<u8*>(mmap(NULL, len, PROT_READ, MAP_PRIVATE, fd, 0));
		if(!buf){
			print_log(DEBUG, 1, "[%s] err : mmap\n", m_p_log_id);
			goto fclose_err;
		}

		if(wait){
			print_log(DEBUG, 1, "[%s] Waiting for receiver ping...", m_p_log_id);
			fflush(stdout);

			do{
				ret = read(serial_fd, &answer, sizeof(answer));
				if(ret != sizeof(answer)){
					print_log(DEBUG, 1, "err : read\n");
					return -errno;
				}
			}while(answer != 'C');

			print_log(DEBUG, 1, "done\n");
		}
		//print_log(DEBUG, 1, "[%s] Sending(%d)...\n", m_p_log_id, len);

		if(protocol == PROTOCOL_YMODEM){
			strncpy((char *) chunk.payload, filename, sizeof(chunk.payload));
			chunk.block = 0;
			skip_payload = 1;
		}
		else{
			chunk.block = 1;
		}

		chunk.start = X_STX;
		*progress = 0;
		print_log(DEBUG, 1, "[%s] update(iap) - write %3d%% ", m_p_log_id, *progress);


		while(len){
			size_t z = 0;
			int next = 0;
			//char status;

			if(!skip_payload){
				z = min(len, sizeof(chunk.payload));
				memcpy(chunk.payload, buf, z);
				memset(chunk.payload + z, 0xff, sizeof(chunk.payload) - z);
			}
			else{
				skip_payload = 0;
			}

			chunk.crc = swap16(crc16(chunk.payload, sizeof(chunk.payload)));
			chunk.block_neg = 0xff - chunk.block;

			//print_log(DEBUG, 1, "[%s] write size=%d", m_p_log_id, sizeof(chunk));
			ret = write(serial_fd, &chunk, sizeof(chunk));
			if(ret != sizeof(chunk)){
				print_log(DEBUG, 1, " err, ret = %d\n", ret);
				goto fclose_err;
			}
			//else
			//	print_log(DEBUG, 1, " ok, ret len = %d\n", ret);

			//print_log(DEBUG, 1, "[%s] read answer", m_p_log_id);
			ret = read(serial_fd, &answer, sizeof(answer));
			if(ret != sizeof(answer)){
				print_log(DEBUG, 1, " err, ret_len = %d\n", ret);
				goto fclose_err;
			}
			//else
			//	print_log(DEBUG, 1, " ok, ret = %d\n", answer);

			switch(answer){
				case X_NAK:
					//status = 'N';
					break;
				case X_ACK:
					//status = '.';
					next = 1;
					break;
				default:
					//status = '?';
					break;
			}

			//print_log(DEBUG, 1, "[%s] %c\n", m_p_log_id, status);

			fflush(stdout);

			if(next){
				chunk.block++;
				len -= z;
				buf += z;
			}

			// 0 ~ 90%. 이값을 읽어 SDK에서 업데이트 완료 여부를 판단함.
			// 따라서, progress가 100이 되는 시점은 내부 resource가 완전히 정리되는 update_using_uart 호출 마지막 부분임.
			*progress = (stat.st_size - len) * 90 / stat.st_size;			
			print_log(DEBUG, 1, "\b\b\b\b\b%3d%% ", *progress);				
		}
		print_log(DEBUG, 1, "\n");

		ret = write(serial_fd, &eof, sizeof(eof));
		if(ret != sizeof(eof)){
			print_log(DEBUG, 1, "[%s] eof err, size=%d, ret=%d", m_p_log_id, sizeof(eof), ret);
			goto fclose_err;
		}

	#if 0
		/* send EOT again for YMODEM */
		if(protocol == PROTOCOL_YMODEM){
			ret = write(serial_fd, &eof, sizeof(eof));
			if(ret != sizeof(eof))
				goto fclose_err;
		}
	#endif
		//print_log(DEBUG, 1, "[%s] send done.\n", m_p_log_id);
		close(fd);
		return 0;

		fclose_err:
			close(fd);
			return -errno;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CMicom::dump_serial(void)
{
		char in;

		lock();

		for( ; ; ){
			read(m_n_fd_self, &in, sizeof(in));
			print_log(DEBUG, 1, "[%s] %c", m_p_log_id, in);
			fflush(stdout);
		}

		unlock();

		print_log(DEBUG, 1, "\n");
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CMicom::send_file(const s8 *filename, u16 *progress)
{
		lock();

		//print_log(DEBUG, 1, "[%s] Ymodem tx start...\n", m_p_log_id);
		int ret = xymodem_send(m_n_fd_self, filename, PROTOCOL_YMODEM, 0, progress);

		unlock();
		//dump_serial(m_n_fd_self);

		if(ret < 0)
			return false;
		return true;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CMicom::uart_tx(u8 *tx_addr, u8 len)
{
		lock();

		u8 ret = write( m_n_fd_self, tx_addr, len );

		unlock();

		//print_log(DEBUG, 1, "[%s] uart_tx 0x%02x, 0x%02x\n", m_p_log_id, tx_addr[0], tx_addr[1]);
		if(ret != len){
			print_log(ERROR, 1, "[%s] uart_tx : %d/%d\n", m_p_log_id, len, ret);
			return false;
		}

		sleep_ex(MCU_RDELAY_LONG * 2);
		return true;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CMicom::uart_rx(u8 *rx_addr, u8 len)
{
		lock();

		read( m_n_fd_self, rx_addr, len );

		unlock();

		//print_log(DEBUG, 1, "[%s] uart_rx 0x%02x, 0x%02x\n", m_p_log_id, rx_addr[0], rx_addr[1]);
		sleep_ex(MCU_RDELAY_LONG * 2);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CMicom::uart_update_stm32f(const s8 *filename, u16* progress)
{
		u8 p_send[UCMD_SIZE] = {0,};
		u8 p_recv[UCMD_SIZE] = {0,};
		u8 protect_status = 0xFF;


		//sleep_ex(MCU_RDELAY_LONG * 2);
		uart_rx(p_recv, 1);					//flash micom uart tx


	//read flash-protect status
		p_send[0] = MASTER_HEADER;
		p_send[1] = PROTECT_R_CMD;
		uart_tx(p_send, 2);


	//set write-protect disable
		while(1){
			//slave_uart_ready();

			uart_rx(p_recv, 2);
			if(p_recv[0] == SLAVE_HEADER 
				&& p_recv[1] >= FLASHIF_PROTECTION_NONE &&  p_recv[1] <= FLASHIF_PROTECTION_RDPENABLED){

				protect_status = p_recv[1];
				print_log(ERROR, 1, "[%s] update(iap) - protection : 0x%02X\n", m_p_log_id, protect_status);

				if(protect_status == FLASHIF_PROTECTION_NONE)
					break;
			}

			p_send[0] = MASTER_HEADER;
			p_send[1] = PROTECT_W_CMD;
			uart_tx(p_send, 2);
			sleep_ex(MCU_RDELAY_LONG * 2);
		}


	//set ready send/receive
		while(1){
			//slave_uart_ready();

			p_send[0] = MASTER_HEADER;
			p_send[1] = DOWNLOAD_CMD;
			uart_tx(p_send, 2);

			uart_rx(p_recv, 2);
			if(p_recv[0] == SLAVE_HEADER && p_recv[1] == DOWNLOAD_CMD)
				break;
		}
		print_log(ERROR, 1, "[%s] update(iap) - ready to send\n", m_p_log_id);


	//send file
		if(send_file(filename, progress)){
			;//print_log(ERROR, 1, "[%s] Micom update err : send_file\n", m_p_log_id);
			//return false;
		}

		while(1){
			uart_rx(p_recv, 1);

			if(p_recv[0] != SLAVE_HEADER)
				continue;

			//sleep_ex(MCU_RDELAY_LONG);

			uart_rx(&p_recv[1], 1);

			if(p_recv[1] == (DOWNLOAD_CMD | COMPLETE_CMD))
				break;
		}
		//slave_uart_ready();


	//reboot
		print_log(ERROR, 1, "[%s] update(iap) - need to reboot.. \n\n\n\n\n", m_p_log_id);
		return true;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CMicom::uart_update_stm32f_finish(void)
{
		u8 p_send[UCMD_SIZE] = {0,};

		p_send[0] = MASTER_HEADER;
		p_send[1] = REBOOT_CMD;
		uart_tx(p_send, 2);

		sleep_ex(MCU_RDELAY_LONG * 20);
		dump_serial();
}


/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u32 CMicom::eeprom_unit_read( u16 i_w_addr, u8* o_p_data, u32 i_n_len, eeprom_blk_num_t i_n_eeprom_blk_num )
{
	bool b_result = false;
	MICOM_EVENT micom_event_e = eMICOM_EVENT_IDLE;
	
	static u8 s_p_recv[COMMAND_BIG_DATA_LEN];
	
	u8 tx_addr[8] = {0,};
	tx_addr[0] = 1;			//0-none, 1-read, 2-write
	tx_addr[1] = (u8)((i_w_addr >> 8) & 0x00FF);
	tx_addr[2] = (u8)(i_w_addr & 0x00FF);
	tx_addr[3] = (u8)i_n_len;
	tx_addr[4] = (u8)i_n_eeprom_blk_num;
	
	memset(s_p_recv, 0, sizeof(s_p_recv) );
	
	//lock();
	
	send( (u8)eMICOM_EVENT_EEPROM_REQUEST, tx_addr, 0 );
	micom_event_e = eMICOM_EVENT_EEPROM_REQUEST;
	sleep_ex(MCU_RDELAY_LONG);
	
	b_result = recv(&micom_event_e, s_p_recv, 500, 1, i_n_len);
	
	//unlock();
	
	if( b_result == false )
	{
		print_log(DEBUG, 1, "[%s] eeprom_read: recv error, addr: 0x%04X, req len: %d\n", m_p_log_id, i_w_addr, i_n_len);
	}
	
	memcpy( o_p_data, s_p_recv, i_n_len );

	return 0;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u32 CMicom::eeprom_read( u16 i_w_addr, u8* o_p_data, u32 i_n_len, eeprom_blk_num_t i_n_eeprom_blk_num )
{	
	u32 n_read_count;
	u32 i;
	u16 w_addr;
	u8* p_data = NULL;
	u32 n_data_pos = 0, n_data_len;;
	
	w_addr			= i_w_addr;
	n_read_count	= i_n_len / COMMAND_BIG_DATA_LEN + 1;	//@pst, sizeof(iod_t.data) -> PACKET_SIZE
    
    n_data_len		= n_read_count * COMMAND_BIG_DATA_LEN;
    
	p_data = (u8*)malloc( n_data_len );
	
	if( p_data == NULL )
    {
        print_log(ERROR, 1, "[%s] eeprom_read malloc failed(size: %d)\n", m_p_log_id, i_n_len);
        return 0;
    }
    
    memset( p_data, 0, n_data_len );
    
    print_log(DEBUG, 1, "[%s] eeprom_read(addr: 0x%04X) len: %d(%d)\n", m_p_log_id, i_w_addr, i_n_len, (n_read_count * COMMAND_BIG_DATA_LEN) );
    
    lock();
    
    for( i = 0; i < n_read_count; i++ )
    {
		if( i == (n_read_count - 1) )
		{
			eeprom_unit_read(w_addr, &p_data[n_data_pos], (u8)i_n_len - n_data_pos, i_n_eeprom_blk_num);
			
			n_data_pos = i_n_len - n_data_pos;
		}
		else
		{
			eeprom_unit_read(w_addr, &p_data[n_data_pos], (u8)COMMAND_BIG_DATA_LEN, i_n_eeprom_blk_num);
			
			n_data_pos += COMMAND_BIG_DATA_LEN;
			w_addr += COMMAND_BIG_DATA_LEN;
		}
    }
	
	unlock();
	
    memcpy( o_p_data, p_data, i_n_len );
    
    safe_free( p_data );
    
    return n_data_pos;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u32 CMicom::eeprom_unit_write( u16 i_w_addr, u8* i_p_data, u32 i_n_len, eeprom_blk_num_t i_n_eeprom_blk_num )
{
	u8 tx_addr[8] = {0,};
	tx_addr[0] = 2;			//0-none, 1-read, 2-write
	tx_addr[1] = (u8)((i_w_addr >> 8) & 0x00FF);
	tx_addr[2] = (u8)(i_w_addr & 0x00FF);
	tx_addr[3] = (u8)i_n_len;
	tx_addr[4] = (u8)i_n_eeprom_blk_num;
	
	//lock();
	
	send( (u8)eMICOM_EVENT_EEPROM_REQUEST, tx_addr, 0 );
	send( (u8)eMICOM_EVENT_EEPROM_DATA, i_p_data, 1 );
	
	sleep_ex(MCU_NVM_DELAY_LONG);
	
	//unlock();
	
	return 0;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u32 CMicom::eeprom_write( u16 i_w_addr, u8* i_p_data, u32 i_n_len, eeprom_blk_num_t i_n_eeprom_blk_num )
{
	u32 n_write_count;
	u32 i;
	u16 w_addr;
	u8* p_data = NULL;
	u32 n_data_pos = 0, n_data_len;
	
	w_addr          = i_w_addr;
	n_write_count   = i_n_len / COMMAND_BIG_DATA_LEN + 1;	//@pst, sizeof(iod_t.data) -> PACKET_SIZE
    
    n_data_len		= n_write_count * COMMAND_BIG_DATA_LEN;
    
    p_data = (u8*)malloc( n_data_len );
	
	if( p_data == NULL )
    {
        print_log(ERROR, 1, "[%s] eeprom_write malloc failed(size: %d)\n", m_p_log_id, i_n_len);
        return 0;
    }
	
	memset( p_data, 0, n_data_len );
	
	print_log(DEBUG, 1, "[%s] eeprom_write(addr: 0x%04X) len: %d(%d)\n", m_p_log_id, i_w_addr, i_n_len, n_data_len );
    
    lock();
    for( i = 0; i < n_write_count; i++ )
    {
		if( i == (n_write_count - 1) )
		{
			memcpy(&p_data[n_data_pos], &i_p_data[n_data_pos], i_n_len - n_data_pos);
			eeprom_unit_write(w_addr, &p_data[n_data_pos], (u8)i_n_len - n_data_pos, i_n_eeprom_blk_num);
			
			n_data_pos = i_n_len - n_data_pos;
		}
		else
		{
			memcpy(&p_data[n_data_pos], &i_p_data[n_data_pos], COMMAND_BIG_DATA_LEN);
			eeprom_unit_write(w_addr, &p_data[n_data_pos], (u8)COMMAND_BIG_DATA_LEN, i_n_eeprom_blk_num);
			
			n_data_pos += COMMAND_BIG_DATA_LEN;
        	w_addr += COMMAND_BIG_DATA_LEN;
		}
    }
    unlock();
    
    safe_free( p_data );
    
    return n_data_pos;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CMicom::aed_read_trig_volt( AED_ID i_aed_id_e, u32* o_p_volt )
{
	u8 p_recv[COMMAND_DATA_LEN] = {0,};
	u16 w_data;
	
	u32 n_adc = 0;
	float raw_volt;
	
	if( get(eMICOM_EVENT_AED_ADC, NULL, p_recv, 0) == true )
	{
		w_data = (u16)(p_recv[(u8)i_aed_id_e * 2] << 8 & 0xFF00) | (u16)(p_recv[((u8)i_aed_id_e * 2) + 1] & 0x00FF);
		
		raw_volt = w_data * ADC_MICOM_VEF_VOLT / (float)(ADC_MICOM_RESOLUTION);
		n_adc = 1000 * raw_volt * (25.0f/15.0f);
		
		*o_p_volt = n_adc;
		
		return true;
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
void CMicom::impact_read_pos( s16* o_p_x, s16* o_p_y, s16* o_p_z )
{
    MICOM_EVENT micom_event_e = eMICOM_EVENT_IDLE;
    bool b_result;
    u8      p_data[COMMAND_BIG_DATA_LEN];
    micom_state_t   state_t;

    lock();

    if( m_b_msg_stop == true )
    {
    	print_log(DEBUG, 1, "[%s] impact_get_pos micom communication stop\n", m_p_log_id);
    	unlock();
    	return;
    }
    
    memset( p_data, 0, sizeof(p_data) );
    micom_event_e = eMICOM_EVENT_STATE;
    
    send( (u8)eMICOM_EVENT_STATE );
   	b_result = recv(&micom_event_e, p_data);
        
    if( b_result == true )
    {
        memcpy( &state_t, p_data, COMMAND_DATA_LEN );
        *o_p_x = state_t.adxl_t.w_x;
        *o_p_y = state_t.adxl_t.w_y;
        *o_p_z = state_t.adxl_t.w_z;
    }
    
    unlock();
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CMicom::_get( MICOM_EVENT i_event_e, u8* a_p_send, u8* a_p_recv, bool big_size )
{
    MICOM_EVENT micom_event_e = eMICOM_EVENT_IDLE;
    u16 i;
    bool b_result;
    u8  p_recv[COMMAND_BIG_DATA_LEN];
	u8  p_trans[COMMAND_BIG_DATA_LEN];
    
    memset( p_recv, 0, sizeof(p_recv) );
	if(a_p_send != NULL)
	{
		memcpy( p_trans, a_p_send, sizeof(p_trans) );
	}
	else
	{
		memset( p_trans, 0, sizeof(p_trans) );
		p_trans[0] = STX;
		p_trans[1] = i_event_e;
		p_trans[COMMAND_BUFFER_LEN - 2] = (u8)(STX + i_event_e + ETX);
		p_trans[COMMAND_BUFFER_LEN - 1] = ETX;
	}
   
    for( i = 0; i < 5; i++ )
    {
		send( (u8)i_event_e, p_trans, 0 );
		micom_event_e = i_event_e;
		
        b_result = recv(&micom_event_e, p_recv, 500, big_size, COMMAND_BIG_DATA_LEN);
        
        if( b_result == true )
        {
            if( a_p_recv != NULL )
            {
            	if(big_size)
				{
					memcpy( a_p_recv, p_recv, COMMAND_BIG_DATA_LEN );
				}
				else
				{
					memcpy( a_p_recv, p_recv, COMMAND_DATA_LEN );
				}
			}
			
			//delay for nvram erase
			if(i_event_e == eMICOM_EVENT_IMPACT_BUF_ONE_CLEAR || i_event_e == eMICOM_EVENT_IMPACT_BUF_RESET)
			{
				sleep_ex(MCU_NVM_DELAY_LONG);
			}
			return true;
        }

		print_log(DEBUG, 1, "[%s] req(0x%02X) response try: %d\n", m_p_log_id, i_event_e, i + 1);
    }
	
	print_log(DEBUG, 1, "[%s] req(0x%02X) response timeout\n", m_p_log_id, i_event_e);
    return false;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CMicom::get( MICOM_EVENT i_event_e, u8* a_p_send, u8* a_p_recv, bool big_size, bool i_b_update )
{
	bool b_result;
	
	lock();
	if( i_b_update == false
		&& m_b_msg_stop == true )
	{
		if( !m_b_update_started )
			print_log(DEBUG, 1, "[%s] micom_get micom communication stop(0x%02X)\n", m_p_log_id, i_event_e);
		unlock();
		return false;
	}
	
	b_result = _get(i_event_e, a_p_send, a_p_recv, big_size);
	
    unlock();
	
    return b_result;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    command.cpp용. 일반적인 로직 상에서 구동되지 않음
*******************************************************************************/
void CMicom::print_adc(void)
{
	u16 i;
	u16 p_adc[POWER_VOLT_MAX] = {0,};
	power_volt_t volt_t;
	float raw_volt[POWER_VOLT_MAX], result[POWER_VOLT_MAX];
	
	u8 p_recv[COMMAND_DATA_LEN * 2] = {0,};	//2개의 command 응답을 저장함. 
	
	get(eMICOM_EVENT_BAT_ADC, NULL, p_recv, 0);
	get(eMICOM_EVENT_AED_ADC, NULL, &p_recv[COMMAND_DATA_LEN], 0);
	
	for(i = 0; i < COMMAND_DATA_LEN; i++)
	{
		p_adc[i] = (u16)(p_recv[i * 2] << 8 & 0xFF00) | (u16)(p_recv[i * 2 + 1] & 0x00FF);
	}
	
	// update voltage
	memset( &volt_t, 0, sizeof(power_volt_t) );
    for( i = 0; i < POWER_VOLT_MAX; i++ )
	{
	    raw_volt[i] = p_adc[i] * ADC_MICOM_VEF_VOLT / (float)(ADC_MICOM_RESOLUTION);
	    if( i == POWER_VOLT_VBATT )
	    {
			result[i]   = raw_volt[i] * (ADC_VBATT_VOLTAGE_DIVIDER_NUMERATOR / ADC_VBATT_VOLTAGE_DIVIDER_DENOMINATOR);
	    }
		else if( i == POWER_VOLT_AED_TRIG_A || i == POWER_VOLT_AED_TRIG_B )
		{
		result[i]   = raw_volt[i] * (ADC_AED_TRIG_VOLTAGE_DIVIDER_NUMERATOR / ADC_AED_TRIG_VOLTAGE_DIVIDER_DENOMINATOR);
		}
		else if( i == POWER_VOLT_VS )
		{
			result[i]   = raw_volt[i] * (ADC_VS_TRIG_VOLTAGE_DIVIDER_NUMERATOR / ADC_VS_TRIG_VOLTAGE_DIVIDER_DENOMINATOR);
		}
	    else
	    {
	    	result[i] = raw_volt[i];
	    }
	    
	    volt_t.volt[i]   = (u16)(result[i] * 100);
	}
	
	print_log(DEBUG, 1, "[%s] =============== ADC ===============\n", m_p_log_id);
	print_log(DEBUG, 1, "[%s] B_ID_A	:  %2.2fV(raw: %2.2fV, %d)\n", m_p_log_id, result[0], raw_volt[0], p_adc[0] );
	print_log(DEBUG, 1, "[%s] B_ID_B	:  %2.2fV(raw: %2.2fV, %d)\n", m_p_log_id, result[1], raw_volt[1], p_adc[1] );
	print_log(DEBUG, 1, "[%s] B_THM_A	:  %2.2fV(raw: %2.2fV, %d)\n", m_p_log_id, result[2], raw_volt[2], p_adc[2] );
	print_log(DEBUG, 1, "[%s] B_THM_B	:  %2.2fV(raw: %2.2fV, %d)\n", m_p_log_id, result[3], raw_volt[3], p_adc[3] );
	print_log(DEBUG, 1, "[%s] AED_TRIG_A:  %2.2fV(raw: %2.2fV, %d)\n", m_p_log_id, result[4], raw_volt[4], p_adc[4] );
	print_log(DEBUG, 1, "[%s] AED_TRIG_B:  %2.2fV(raw: %2.2fV, %d)\n", m_p_log_id, result[5], raw_volt[5], p_adc[5] );
	print_log(DEBUG, 1, "[%s] VBATT		:  %2.2fV(raw: %2.2fV, %d)\n", m_p_log_id, result[6], raw_volt[6], p_adc[6] );
	print_log(DEBUG, 1, "[%s] VS		:  %2.2fV(raw: %2.2fV, %d)\n", m_p_log_id, result[7], raw_volt[7], p_adc[7] );
	
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CMicom::print_state(void)
{
    u8      p_data[COMMAND_DATA_LEN];
    micom_state_t	state_t;
    
    memset( p_data, 0, sizeof(p_data) );
    memset( &state_t, 0, sizeof(state_t) );
    
    get(eMICOM_EVENT_STATE, NULL, p_data );
    
    memcpy( &state_t, p_data, sizeof(micom_state_t) );
    
    print_log(DEBUG, 1, "[%s] state  :  0x%02X\n", m_p_log_id, state_t.c_state );
    
    print_log(DEBUG, 1, "[%s] pwr button low:  %d, tether low: %d\n", \
    					m_p_log_id, state_t.IO_STATUS.BIT.M_BTN, state_t.IO_STATUS.BIT.POE );
    					
    print_log(DEBUG, 1, "[%s] impact:  %d, power off: %d, wake up: %d\n", \
    					m_p_log_id, state_t.IO_STATUS.BIT.ADXL_FULL, \
    					state_t.IO_STATUS.BIT.POWER_OFF, state_t.IO_STATUS.BIT.WAKEUP_REQ );

	print_log(DEBUG, 1, "[%s] wpt_final_state: %s, wpt_detected_old: %s\n", \
    					m_p_log_id, state_t.IO_STATUS.BIT.WPT_FINAL_STATE ? "on" : "off", \
    					state_t.IO_STATUS.BIT.WPT_HALL_SENSOR_ACTIVE ? "active" : "non-active" );
    					
	print_log(DEBUG, 1, "[%s] external_wpt_off_request:  %d\n", \
						m_p_log_id, state_t.IO_STATUS.BIT.EXT_WPT_REQ);
	
    print_log(DEBUG, 1, "[%s] acc X  :  %d\n", m_p_log_id, state_t.adxl_t.w_x*49 );
    print_log(DEBUG, 1, "[%s] acc Y  :  %d\n", m_p_log_id, state_t.adxl_t.w_y*49 );
    print_log(DEBUG, 1, "[%s] acc Z  :  %d\n", m_p_log_id, state_t.adxl_t.w_z*49 );
    
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CMicom::send_payload(u8 *a_b_payload, bool i_b_ack)
{
	u8 p_send[COMMAND_DATA_LEN] = {0,};
	u8 p_recv[COMMAND_BIG_DATA_LEN] = {0,};
	MICOM_EVENT micom_event_e = (MICOM_EVENT)(a_b_payload[0]);
	p_send[0] = a_b_payload[1];
	p_send[1] = a_b_payload[2];

	if(micom_event_e >= eMICOM_EVENT_IMPACT_0  && micom_event_e <= eMICOM_EVENT_IMPACT_0 + IMPACT_GROUP_SIZE*ADXL375B_DATA_SIZE)
	{
		if( get(micom_event_e,p_send,p_recv, true) == false )
		{
		    print_log(DEBUG, 1, "[%s] micom send failed: 0x%02X\n", m_p_log_id, micom_event_e );
		    return;
		}
	}
	else
	{
		if( get(micom_event_e,p_send,p_recv) == false )
		{
		    print_log(DEBUG, 1, "[%s] micom send failed: 0x%02X\n", m_p_log_id, micom_event_e );
		    return;
		}
	}		

	if( micom_event_e == eMICOM_EVENT_STATE )
    {
    	adxl_data_t 	p_adxl_data_t;
    	micom_state_t   state_t;
    	memcpy(&p_adxl_data_t, &p_recv[2], 6);
    	
    	memcpy(&state_t, p_recv, COMMAND_DATA_LEN);
        print_log(DEBUG, 1, "[%s] micom state: 0x%02X\n\
        M_BUTTON(%s)\n\
        POE(%s)\n\
        Impact data(%d)\n\
        AXIS : %4.3fG,%4.3fG,%4.3fG\n",
        	m_p_log_id, state_t.c_state, \
        	state_t.IO_STATUS.BIT.M_BTN > 0 ? "off":"on",\
        	state_t.IO_STATUS.BIT.POE >0 ? "not connected":"connected",\
        	state_t.IO_STATUS.BIT.ADXL_FULL,\
        	state_t.adxl_t.w_x * 0.049,\
			state_t.adxl_t.w_y * 0.049,\
			state_t.adxl_t.w_z * 0.049);
        return;             
    }
    else if( micom_event_e == eMICOM_EVENT_ERROR )
    {
   
    	print_log(DEBUG, 1, "[%s] micom error: debug, payload num(0x%02X)(0x%02X)(0x%02X)(0x%02X) \n", m_p_log_id, p_recv[0],p_recv[1],p_recv[2],p_recv[3] );   
        
        return;
    }
    else if( micom_event_e >= eMICOM_EVENT_IMPACT_0  && micom_event_e <= eMICOM_EVENT_IMPACT_0 + 24)
    {
    	u32 n_idx = 	(micom_event_e - eMICOM_EVENT_IMPACT_0);
    	u32 			j_idx;
    	
    	// ADXL375B_BUF_SIZE = 128 * 6
    	// ADXL375B_FIFO_SIZE = 128
    	u8				p_impact_buf[ADXL375B_MICOM_BUF_SIZE];
    	adxl_data_t* 	p_adxl_data_t = (adxl_data_t*)p_impact_buf;
    	
    	memcpy(&p_impact_buf[n_idx * 128], p_recv, 128);
    	printf("[%s] micom got a impact package %d \n", m_p_log_id, n_idx);
    	
    	if( 0 )
    	{
    		for(j_idx = 0 ; j_idx < 128; j_idx++)
			{
				printf("AXIS%3d : %6d,%6d,%6d\n",j_idx,(p_adxl_data_t[j_idx].w_x)
										,(p_adxl_data_t[j_idx].w_y)
										,(p_adxl_data_t[j_idx].w_z));									
			}
    	}
    	return;
    }
    else if( micom_event_e == eMICOM_EVENT_IMPACT_BUF_ONE_CLEAR)
    {
     	printf("[%s] micom clears a impact buffer\n", m_p_log_id);
        	
    	return;
    }
    else if( micom_event_e == eMICOM_EVENT_IMPACT_REG_READ)
    {
    	printf("[%s] micom impact sensor reg read(0x%02X): 0x%02X\n", m_p_log_id, p_send[0], p_recv[0] );
    	return;
    }
    else if( micom_event_e == eMICOM_EVENT_IMPACT_REG_WRITE)
    {
    	printf("[%s] micom impact sensor write success(addr:0x%02X), data:0x%02X)\n", m_p_log_id, p_send[0], p_send[1]);
    	return;
    }
    else if( micom_event_e == eMICOM_EVENT_IMPACT_BUF_CNT)
    {
    	printf("[%s] micom impact bitmap : 0x%04X\n", m_p_log_id, *(u16*)&p_recv[0]);
    	return;    	
    }
    else if( micom_event_e == eMICOM_EVENT_IMPACT_NO_DATA)
    {
    	printf("[%s] micom there is no data.\n", m_p_log_id );
    	return;
    }
    else if( micom_event_e == eMICOM_EVENT_IMPACT_REG_SAVE)
    {
    	printf("[%s] micom impact reg save completed.\n", m_p_log_id );
    	return;
    }
    else if( micom_event_e == eMICOM_EVENT_TIMESTAMP_GET)
    {
    	u32 n_timestamp = (p_recv[0] << 24) + \
    					(p_recv[1] << 16) + \
    					(p_recv[2] << 8) + \
    					(p_recv[3] << 0);
    	
    	
    	printf("[%s] micom timestamp : %d\n", m_p_log_id, n_timestamp);
    	return;
    }
    else if( micom_event_e == eMICOM_EVENT_TIMESTAMP_SET)
    {
    	printf("[%s] micom timestamp saved.\n", m_p_log_id);
    	return;
    }
    else if( micom_event_e == eMICOM_EVENT_VERSION )
    {
    	printf("[%s] micom version: %d.%d\n", m_p_log_id, ((p_recv[0] >> 4) & 0x0F), (p_recv[0] & 0x0F));
    	return;
    }
	else
		;
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void* micom_update_routine( void * arg )
{
	CMicom * micom = (CMicom *)arg;
	micom->update_proc();
	pthread_exit( NULL );
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CMicom::update_start( const s8* i_p_file_name)
{	
	strcpy( m_p_update_file_name, i_p_file_name );
    
    if( m_b_update_is_running == true )
    {
        print_log( ERROR, 1, "[%s] micom_update_proc is already started.\n", m_p_log_id);
        return false;
    }
   	
    lock();
    m_b_update_started = true;
    m_b_msg_stop = true;
	unlock();
	
   	m_w_update_progress = 0;
   	m_b_update_result = false;
   	m_b_updated = false;
	
    // launch capture thread
    m_b_update_is_running = true;
	if( pthread_create(&m_update_thread_t, NULL, micom_update_routine, ( void * ) this)!= 0 )
    {
    	print_log( ERROR, 1, "[%s] pthread_create:%s\n", m_p_log_id, strerror( errno ));

    	m_b_update_is_running = false;
    	
    	return false;
    }
    
    return true;
}


/******************************************************************************/
/**
* \brief        stop configuration thread
* \param        none
* \return       none
* \note
*******************************************************************************/
void CMicom::update_stop(void)
{
	print_log(DEBUG, 1, "[%s] update_proc...\n", m_p_log_id );
	
    if( m_b_update_is_running == false )
    {
        print_log( ERROR, 1, "[%s] update_proc is already stopped.\n", m_p_log_id);
        return;
    }
    
    m_b_update_is_running = false;
	if( pthread_join( m_update_thread_t, NULL ) != 0 )
	{
		print_log( ERROR, 1,"[%s] pthread_join: update_proc:%s\n", \
		                    m_p_log_id, strerror( errno ));
	}
   	
   	m_w_update_progress = 100;
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void CMicom::update_proc(void)
{
    print_log(DEBUG, 1, "[%s] start update_proc(file: %s)...\n", \
                        m_p_log_id, m_p_update_file_name );

    FILE*   p_file = 0; 
    u32     n_file_size ;
    u8*		p_data = 0;
    
    p_file = fopen( m_p_update_file_name, "rb" );
    if( p_file == NULL )
    {
        print_log(ERROR, 1, "[%s] file(%s) open failed.\n", \
                             m_p_log_id, m_p_update_file_name);
        m_b_update_result = false;
        return;
    }
    
    n_file_size = file_get_size( p_file );
	p_data = (u8*)malloc(n_file_size);
	
	if( p_data == NULL )
	{
	    fclose( p_file );
	    print_log(ERROR, 1, "[%s] file(%s) malloc failed.\n", \
                             m_p_log_id, m_p_update_file_name);
        return;
	}
	else
	{
	    fread(p_data,1,n_file_size,p_file);	
	}

	safe_free(p_data);
    fclose( p_file );

	update_using_uart();

    print_log(DEBUG, 1, "[%s] update_proc...(result: %d)\n", m_p_log_id, m_b_update_result );

	m_b_update_is_running = false;
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
bool CMicom::update_using_uart(void)
{
		u8 p_send[COMMAND_DATA_LEN] = {0,};
		u8 p_recv[COMMAND_DATA_LEN] = {0,};

		print_log(DEBUG, 1, "[%s] update start\n", m_p_log_id);
		
		p_send[0] = eUPDATE_CMD_START;
		get(eMICOM_EVENT_UPDATE_REQ, p_send, p_recv, false, true);


		m_b_update_result = uart_update_stm32f(m_p_update_file_name, &m_w_update_progress);

		m_b_updated = true;
		m_w_update_progress = 100;

		return m_b_update_result;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CMicom::reboot(void)
{
    if( m_b_updated == true )
    {
		print_log(DEBUG, 1, "[%s] update reboot\n", m_p_log_id);
    	uart_update_stm32f_finish();
    }
    else
    {
        print_log(DEBUG, 1, "[%s] reboot\n", m_p_log_id);
	    
	    lock();
	
		m_b_msg_stop = true;
	    _get( eMICOM_EVENT_REBOOT );
	    
	    unlock();
    }
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CMicom::power_off(void)
{
	print_log(DEBUG, 1, "[%s] power off\n", m_p_log_id);
	
	lock();
	
	m_b_msg_stop = true;
	_get( eMICOM_EVENT_POWER_OFF );
	
	unlock();
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CMicom::is_power_button_on(void)
{
    u8      p_data[COMMAND_DATA_LEN];
    micom_state_t	state_t;
    
    memset( p_data, 0, sizeof(p_data) );
    memset( &state_t, 0, sizeof(state_t) );
    
    get(eMICOM_EVENT_STATE, NULL, p_data );
    
    memcpy( &state_t, p_data, sizeof(micom_state_t) );
    
    if( state_t.IO_STATUS.BIT.M_BTN == 0 )
    {
    	return true;
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
void CMicom::set_sleep_led( bool i_b_on )
{
    if( i_b_on == true )
    {
        get( eMICOM_EVENT_SLEEP_ON);
    }
    else
    {
		get( eMICOM_EVENT_SLEEP_OFF);
    }
    
    print_log(DEBUG, 1, "[%s] sleep: %d\n", m_p_log_id, i_b_on);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CMicom::power_led_ready(void)
{
    u8      p_data[COMMAND_DATA_LEN];
    
    memset( p_data, 0, sizeof(p_data) );
    
	//MICOM에서 해당 메시지 수신 시, AP가 eMSTATE_RISC_BOOTING 상태에서 eMSTATE_PON_NORMAL 진입하는것으로 판단함.
    get( eMICOM_EVENT_CHK_REBOOT, NULL, p_data );
    
    print_reboot_status( p_data[0] );
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CMicom::print_reboot_status( u8 i_c_code )
{
	s8 p_buff[128];
	
	switch( i_c_code )
	{
    	case eREBOOT_STATUS_FIRST_POWER_ON:
    	{
    		sprintf( p_buff, "first power on" );
    		break;
    	}
		case eREBOOT_STATUS_SECOND_REBOOT:
		{
			sprintf( p_buff, "second power on(RISC only reboot)" );
			break;
		}
		case eREBOOT_STATUS_BY_REBOOT_COMMAND:
		{
			sprintf( p_buff, "by reboot command" );
			break;
		}
		case eREBOOT_STATUS_BY_POWER_OFF_COMMAND:
		{
			sprintf( p_buff, "by power off command" );
			break;
		}
		case eREBOOT_STATUS_NO_RESP_BOOTING:
		{
			sprintf( p_buff, "no response from RISC while booting" );
			break;
		}
		case eREBOOT_STATUS_NO_RESP_COMMAND:
		{
			sprintf( p_buff, "no response from RISC after booting" );
			break;
		}
		default:
		{
			sprintf( p_buff, "unknown" );
			break;
		}
	}
	
	print_log(DEBUG, 1, "[%s] reboot status: %s(%d)\n", m_p_log_id, p_buff, i_c_code);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u16 CMicom::get_version(void)
{
	u8 p_send[COMMAND_DATA_LEN] = {0,};
	u8 p_recv[COMMAND_DATA_LEN] = {0,};
	u16 res;

	if( get(eMICOM_EVENT_VERSION, p_send, p_recv ) == false )
    {
        return 0;
    }
    

	res = p_recv[0]|p_recv[1]<<4;
    print_log(DEBUG, 1, "[%s] micom version: 0x%04X\n", m_p_log_id, res );
    
    return res;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CMicom::set_timestamp(void)
{
	u8 p_send[COMMAND_DATA_LEN] = {0,};
	struct tm*  p_gmt_tm_t;
	
	u32 n_time = time(NULL);
	
	p_send[0] = ( n_time ) & 0xff;
	p_send[1] = ( n_time >> 8) & 0xff;
	p_send[2] = ( n_time >> 16) & 0xff;
	p_send[3] = ( n_time >> 24) & 0xff;
	
	p_gmt_tm_t = localtime( (time_t*)&n_time );
	print_log(INFO,1,"[%s] set timestamp to micom %04d-%02d-%02d %02d:%02d:%02d(0x%08X)\n",\
					 m_p_log_id, p_gmt_tm_t->tm_year+1900, p_gmt_tm_t->tm_mon+1, p_gmt_tm_t->tm_mday, \
		             p_gmt_tm_t->tm_hour, p_gmt_tm_t->tm_min, p_gmt_tm_t->tm_sec, n_time);

	get(eMICOM_EVENT_TIMESTAMP_SET, p_send, NULL );
}


/******************************************************************************/
/**
 * @brief   power control 방법 관련 함수
 * @param	hw_config_t* i_p_hw_config_t: power control 관련 구조체 포인터 변수, u8* i_p_recv: micom과의 통신으로 받은 응답을 저장할 주소
 * @return
 * @note    hw_config_t의 멤버를 data로 만들어 micom에게 eMICOM_EVENT_AUTO_POWER_ON_OFF_CONFIG을 통해 전달한다
 * 			i_p_recv에 유효한 주소가 있다면 eMICOM_EVENT_AUTO_POWER_ON_OFF_CONFIG의 응답을 저장한다.
 * 			eMICOM_EVENT_AUTO_POWER_ON_OFF_CONFIG
 * 			--> i_p_recv[0]: power off일 때, rising edge trigger을 했다면 true
 * 			--> i_p_recv[1]: power on일 때, falling edge trigger을 했다면 true
 *******************************************************************************/
void CMicom::set_power_ctrl_method(hw_config_t* i_p_hw_config_t)
{
	u8 p_send[COMMAND_DATA_LEN] = {0,};

	p_send[7] = true; //write
	p_send[0] = i_p_hw_config_t->power_source_e;
	//p_send[1] = i_p_hw_config_t->b_auto_power_on;
	//p_send[2] = i_p_hw_config_t->b_auto_power_off;

	get(eMICOM_EVENT_AUTO_POWER_ON_OFF_CONFIG, p_send, NULL);


	//printf("####set power config\n");
}


/******************************************************************************/
/**
 * @brief   power control 방법 관련 함수
 * @param	hw_config_t* i_p_hw_config_t: power control 관련 구조체 포인터 변수, u8* i_p_recv: micom과의 통신으로 받은 응답을 저장할 주소
 * @return
 * @note    hw_config_t의 멤버를 data로 만들어 micom에게 eMICOM_EVENT_AUTO_POWER_ON_OFF_CONFIG을 통해 전달한다
 * 			i_p_recv에 유효한 주소가 있다면 eMICOM_EVENT_AUTO_POWER_ON_OFF_CONFIG의 응답을 저장한다.
 * 			eMICOM_EVENT_AUTO_POWER_ON_OFF_CONFIG
 * 			--> i_p_recv[0]: power off일 때, rising edge trigger을 했다면 true
 * 			--> i_p_recv[1]: power on일 때, falling edge trigger을 했다면 true
 *******************************************************************************/
void CMicom::get_power_ctrl_method( u8* i_p_recv )
{
	u8 p_send[COMMAND_DATA_LEN] = {0,};

	p_send[7] = false; // read

	get(eMICOM_EVENT_AUTO_POWER_ON_OFF_CONFIG, p_send, i_p_recv);

#if 0
	//응답 debugging
	if (i_p_recv != NULL)
	{
		printf("####get power config\n");
		for (int i = 0; i < 8; i++)
		{
			printf("%d ", *(i_p_recv+i));
		}
	}
	printf("\n");
#endif
}

/******************************************************************************/
/**
 * @brief   lifespan extenable set
 * @param	i_b_en true: enable
 * @param	i_b_en false: disable
 * @return	None
 * @note    
 *******************************************************************************/
void CMicom::set_lifespan_extension( bool i_b_en )
{
	u8 p_send[COMMAND_DATA_LEN] = {0,};

	p_send[0] = i_b_en; 

	get(eMICOM_EVENT_LIFESPAN_EXTENSION, p_send, NULL);
}

void CMicom::ControlLedBlinking(u8 enable)
{
	u8 p_send[COMMAND_DATA_LEN] = {0,};

	p_send[0] = enable; 

	get(eMICOM_EVENT_LED_CONTROL, p_send, NULL);
}