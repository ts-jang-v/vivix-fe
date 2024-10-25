
#ifndef __DRIVERDEFS_H
#define __DRIVERDEFS_H

#include <linux/fs.h>


#define FLASH_DRIVER_NAME								"vivix_flash"
#define	FLASH_MAJOR_NUMBER								250

#define ROIC_DRIVER_NAME								"vivix_roic"
#define ROIC_MAJOR_NUMBER								249

#define TG_DRIVER_NAME									"vivix_tg"
#define TG_MAJOR_NUMBER									248

#define TI_AFE2256_DRIVER_NAME							"vivix_ti_afe2256"
#define TI_AFE2256_MAJOR_NUMBER							240

#define ADI_ADAS1256_DRIVER_NAME						"vivix_adi_adas1256"
#define ADI_ADAS1256_MAJOR_NUMBER						241

#define SPI_DRIVER_NAME									"vivix_spi"
#define SPI_MAJOR_NUMBER								247

#define GPIO_DRIVER_NAME								"vivix_gpio"
#define GPIO_MAJOR_NUMBER								246

#define SWAP( a ) \
		( \
			( ( (a) & 0xFF000000 ) >> 24 ) | \
			( ( (a) & 0x00FF0000 ) >> 8 ) | \
			( ( (a) & 0x0000FF00 ) << 8 ) | \
			( ( (a) & 0x000000FF ) << 24 ) \
		)


// defines the basic types: BOOLEAN
typedef uint8_t	BOOLEAN;


// Main driver object
typedef struct _DRIVER_OBJECT
{
	uint16_t					device_count;	   	// Number of devices in 
	spinlock_t					lock_device; 	  	// Spinlock for device 
	int32_t						major_id;		   	// The OS-assigned driver Major ID
	int							open_count;			//
} driver_object_t;

// FPGA <---> ROIC driver 간 공유 목적의 구조체
typedef struct _exp_wait_queue_ctx
{
    u32 evt_thread_stt;					//power down thread 생성 여부 확인용 0: thread not running 1: thread is running
    u32 evt_wait_cond;					//power down condition. 1: ROIC POWER UP 2: ROIC POWER DOWN
    struct task_struct *evt_kthread;	//power down thread handler
    spinlock_t evt_thread_lock;			//power down thread lock

} exp_wait_queue_ctx;

enum
{
	eBD_2530VW = 0,
	eBD_3643VW,
	eBD_4343VW,
} ;

enum
{
	eLOW = 0,
	eHIGH = 1,
};


#endif /* __DRIVERDEFS_H */

