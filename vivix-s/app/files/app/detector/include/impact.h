/******************************************************************************/
/**
 * @file   	impact.h
 * @brief
 * @author  Jeongil 
*******************************************************************************/

#ifndef _IMPACT_H_
#define _IMPACT_H_

/*******************************************************************************
*	Include Files
*******************************************************************************/
#include "typedef.h"
#include "vw_system_def.h"
#include "vw_i2c.h"
#include "vw_spi.h"
#include "vw_time.h"
#include "micom.h"

/*******************************************************************************
*	Constant Definitions
*******************************************************************************/
#define ADXL375B_FIFO_SIZE		128
#define ADXL375B_BUF_SIZE		(ADXL375B_FIFO_SIZE * ADXL375B_DATA_SIZE)
#define ADXL375B_FIFO_SIZE_BIT	7
#define ADXL375B_PREBUF_SIZE	20
#define ADXL375B_QUEUE_SIZE		20

#define	ADXL375B_DEVID				0x00
#define	ADXL375B_THRESH_SHOCK		0x1D
#define	ADXL375B_OFSX				0x1E
#define	ADXL375B_OFSY				0x1F
#define	ADXL375B_OFSZ				0x20
#define	ADXL375B_DUR				0x21
#define	ADXL375B_LATENT             0x22
#define	ADXL375B_WINDOW             0x23
#define	ADXL375B_THRESH_ACT         0x24
#define	ADXL375B_THRESH_INACT       0x25
#define	ADXL375B_TIME_INACT         0x26
#define	ADXL375B_ACT_INACT_CTL      0x27
#define	ADXL375B_SHOCK_AXES         0x2A
#define	ADXL375B_ACT_SHOCK_STATUS   0x2B
#define	ADXL375B_BW_RATE            0x2C
#define	ADXL375B_POWER_CTL          0x2D
#define	ADXL375B_INT_ENABLE         0x2E
#define	ADXL375B_INT_MAP            0x2F
#define	ADXL375B_INT_SOURCE         0x30
#define	ADXL375B_DATA_FORMAT        0x31
#define	ADXL375B_DATAX0             0x32
#define	ADXL375B_DATAX1             0x33
#define	ADXL375B_DATAY0             0x34
#define	ADXL375B_DATAY1             0x35
#define	ADXL375B_DATAZ0             0x36
#define	ADXL375B_DATAZ1             0x37
#define	ADXL375B_FIFO_CTL           0x38
#define	ADXL375B_FIFO_STATUS        0x39

#define IMPACT_NUMBER_OF_BUF	(16)

#define SENSOR_VALUE_FILTER_SAMPLE_SIZE		80
#define SENSOR_VALUE_FILTER_CUTOFF_SIZE		8

#define clip_s16(x)	((((s16)x) > 127) ? 127:\
				(((s16)x) < -127) ? -127:((s16)x))

/*******************************************************************************
*	Type Definitions
*******************************************************************************/

typedef enum _fifo_mode
{
	eBYPASS = 0,
	eFIFO,
	eSTREAM,
	eTRIGGER,
}fifo_mode_e;

enum
{
	eINT1 = 0,
	eINT2 = 1,
};


enum
{
	eOFF = 0,
	eON = 1,
};

enum
{
	eADXL375B_OUTPUT_RATE_25HZ = 8,
	eADXL375B_OUTPUT_RATE_50HZ,
	eADXL375B_OUTPUT_RATE_100HZ,
	eADXL375B_OUTPUT_RATE_200HZ,
	eADXL375B_OUTPUT_RATE_400HZ,
	eADXL375B_OUTPUT_RATE_800HZ,
	eADXL375B_OUTPUT_RATE_1600HZ,
	eADXL375B_OUTPUT_RATE_3200HZ,
};


typedef union _adxl_reg
{
	union
	{
		u8 DATA;
		struct
		{
			u8	SZ_EN			: 1;        /* BIT0		: 	*/
			u8	SY_EN			: 1;		/* BIT1		: 	*/
			u8	SX_EN			: 1;   		/* BIT2		: 	*/
			u8	SUPPRESS		: 1;        /* BIT3		: 	*/
			u8					: 4;        /* BIT4~7	:  	*/
		}BIT;
	}SHOCK_AXES;							/* REG 0x2A */
	
	union
	{
		u8 DATA;
		struct
		{
			u8	SZ_SRC			: 1;        /* BIT0		: 	*/
			u8	SY_SRC			: 1;		/* BIT1		: 	*/
			u8	SX_SRC			: 1;   		/* BIT2		: 	*/
			u8	ASLEEP			: 1;        /* BIT3		: 	*/
			u8	ACTZ_SRC		: 1;        /* BIT4		:  	*/
			u8	ACTY_SRC		: 1;        /* BIT5		:  	*/
			u8	ACTX_SRC		: 1;        /* BIT6		:  	*/
			u8					: 1;        /* BIT7		:  	*/			
		}BIT;
	}SHOCK_STATUS;							/* REG 0x2B */
	

	union
	{
		u8 DATA;
		struct
		{
			u8	RATE			: 4;        /* BIT0~3	: 	*/
			u8	LOW_POW			: 1;		/* BIT4		: 	*/
			u8					: 3;   		/* BIT5~7	: 	*/
			
		}BIT;
	}BW_RATE;								/* REG 0x2C */
	
	union
	{
		u8 DATA;
		struct
		{
			u8	WAKEUP			: 2;        /* BIT0~1	: 	*/
			u8	SLEEP			: 1;		/* BIT2		: 	*/
			u8	MEASURE			: 1;   		/* BIT3		: 	*/
			u8	AUTO_SLEEP		: 1;        /* BIT4		: 	*/
			u8	LINK			: 1;        /* BIT5		:  	*/
			u8					: 2;        /* BIT6~7	:  	*/			
		}BIT;
	}POW_CTL;								/* REG 0x2D */
	
	union
	{
		u8 DATA;
		struct
		{
			u8	OVERRUN			: 1;        /* BIT0		: 	*/
			u8	WATERMARK		: 1;		/* BIT1		: 	*/
			u8					: 1;   		/* BIT2		: 	*/
			u8	INACT			: 1;        /* BIT3		: 	*/
			u8	ACT				: 1;        /* BIT4		:  	*/
			u8	DSHOCK			: 1;        /* BIT5		:  	*/			
			u8	SSHOCK			: 1;        /* BIT6		:  	*/
			u8	DATA_RDY		: 1;        /* BIT7		:  	*/
		}BIT;
	}INT_EN;								/* REG 0x2E */
	
	union
	{
		u8 DATA;
		struct
		{
			u8	OVERRUN			: 1;        /* BIT0		: 	*/
			u8	WATERMARK		: 1;		/* BIT1		: 	*/
			u8					: 1;   		/* BIT2		: 	*/
			u8	INACT			: 1;        /* BIT3		: 	*/
			u8	ACT				: 1;        /* BIT4		:  	*/
			u8	DSHOCK			: 1;        /* BIT5		:  	*/			
			u8	SSHOCK			: 1;        /* BIT6		:  	*/
			u8	DATA_RDY		: 1;        /* BIT7		:  	*/
		}BIT;
	}INT_MAP;								/* REG 0x2F */
	
	union
	{
		u8 DATA;
		struct
		{
			u8	OVERRUN			: 1;        /* BIT0		: 	*/
			u8	WATERMARK		: 1;		/* BIT1		: 	*/
			u8					: 1;   		/* BIT2		: 	*/
			u8	INACT			: 1;        /* BIT3		: 	*/
			u8	ACT				: 1;        /* BIT4		:  	*/
			u8	DSHOCK			: 1;        /* BIT5		:  	*/			
			u8	SSHOCK			: 1;        /* BIT6		:  	*/
			u8	DATA_RDY		: 1;        /* BIT7		:  	*/
		}BIT;
	}INT_SRC;								/* REG 0x30 */
	
	union
	{
		u8 DATA;
		struct
		{
			u8	RANGE			: 2;        /* BIT0~1		:  	*/
			u8	JUSTIFY			: 1;        /* BIT2		:  	*/	
			u8	FULL_RES		: 1;        /* BIT3		:  	*/	
			u8					: 3;   		/* BIT4~6	: 	*/
			u8	SELF_TEST		: 1;        /* BIT7		:  	*/
		}BIT;
	}DATA_FORMAT;								/* REG 0x31 */
	
	union
	{
		u8 DATA;
		struct
		{
			u8	SAMPLE			: 5;        /* BIT0~4	: 	*/
			u8	TRIGGER			: 1;		/* BIT5		: 	*/
			u8	FIFO_MODE		: 2;   		/* BIT6~7	: 	*/
		}BIT;
	}FIFO_CTL;								/* REG 0x38 */
	
	union
	{
		u8 DATA;
		struct
		{
			u8	ENTRIES			: 6;        /* BIT0~5	: 	*/
			u8					: 1;		/* BIT6		: 	*/
			u8	FIFO_TRIG		: 1;   		/* BIT7		: 	*/
		}BIT;
	}FIFO_STATUS;							/* REG 0x39 */
}adxl_reg_u;


/*******************************************************************************
*	Macros (Inline Functions) Definitions
*******************************************************************************/

/*******************************************************************************
*	Variable Definitions
*******************************************************************************/

/*******************************************************************************
*	Function Prototypes
*******************************************************************************/
void*     impact_routine( void * arg );
void*     impact_event_routine( void * arg );
void*     impact_logging_routine( void * arg );
void*	  impact_get_pos_routine( void * arg );

class CImpact
{
private:

    s8                  m_p_log_id[15];
    int			        (* print_log)(int level,int print,const char * format, ...);
	
	CVWSPI*				m_p_spi_c;
	bool				m_b_gyro_exist;

	virtual void		acc_gyro_combo_sensor_init(void) = 0;
	virtual void		impact_logging_proc(void) = 0;
	virtual void		impact_cal_proc(void) = 0;
	virtual void		impact_event_proc(void) = 0;
	virtual void		impact_get_pos_proc(void) = 0;
public:
	bool				m_b_stable_proc_hold;

	CImpact(CVWSPI* i_p_spi_c, int (*log)(int,int,const char *,...));
	~CImpact();
	
	void 				get_gyro_offset(gyro_cal_offset_t* o_p_gyro_offset_t);
	void 				set_gyro_offset(gyro_cal_offset_t* i_p_gyro_offset_t);
	bool				is_gyro_exist(void){return m_b_gyro_exist;}

	virtual void		initialize(void)= 0;
	
	virtual u8			reg_read(u8 i_c_addr) = 0;
	virtual void		reg_write(u8 i_c_addr, u8 i_c_data) = 0;

	virtual	void		impact_print(void) = 0;
	virtual	void		get_gyro_position( s32* o_p_x, s32* o_p_y, s32* o_p_z ) = 0;
    virtual	u8  		impact_get_threshold(void) = 0;
    virtual	void		impact_set_threshold( u8 i_c_threshold ) = 0;
    virtual	bool		impact_diagnosis(void) = 0;

    virtual void		impact_calibration_start(void) = 0;
    virtual void		impact_calibration_stop(void) = 0;
    virtual u8  		impact_calibration_get_progress(void) = 0;
    virtual bool		impact_calibration_get_result(void) = 0;


	virtual void        impact_log_save(int i_n_cnt) = 0;
	virtual void        impact_read_pos( s32* o_p_x, s32* o_p_y, s32* o_p_z, bool i_b_direct = false ) = 0;
	virtual bool		gyro_calibration_get_result(void) = 0;

	virtual u16			get_impact_count(void) = 0;
	virtual void		clear_impact_count(void) = 0;
	virtual void 	    get_impact_offset(impact_cal_offset_t* o_p_impact_offset_t) = 0;
    virtual void 	    set_impact_offset(impact_cal_offset_t* i_p_impact_offset_t) = 0;

	virtual void		set_impact_extra_detailed_offset(impact_cal_extra_detailed_offset_t* i_p_impact_cal_extra_detailed_offset) = 0;
    virtual void		get_impact_extra_detailed_offset(impact_cal_extra_detailed_offset_t* o_p_impact_cal_extra_detailed_offset) = 0;
	virtual void		stable_pos_proc_start(void) = 0;
	virtual void		stable_pos_proc_stop(void) = 0;
	virtual bool		is_stable_proc_is_running(void) = 0;
	virtual void		set_cal_direction(ACC_DIRECTION i_dir_e) = 0;
    virtual void		load_6plane_sensor_cal_done(u8 i_c_done) = 0;
    virtual u8			get_6plane_sensor_cal_done(void) = 0;
    virtual u8			get_6plane_sensor_cal_misdir(void) = 0;

	friend void *		impact_cal_routine( void * arg );
	friend void * 		impact_event_routine( void * arg );
	friend void * 		impact_logging_routine( void * arg );
	friend void * 		impact_get_pos_routine( void * arg );
};

class CRiscImpact: public CImpact
{
private:

    s8                  m_p_log_id[15];
    int			        (* print_log)(int level,int print,const char * format, ...);
	
	CVWSPI*				m_p_spi_c;
	CVWI2C*      		m_p_i2c_c;

    CMutex*				m_p_mutex_c;
    CTime*				m_p_motion_timer_c;
    s32                 m_n_fd;

    s32					m_n_x;
    s32					m_n_y;
    s32					m_n_z;

	impact_cal_extra_detailed_offset_t	m_impact_cal_extra_detailed_offset_t;	//프로세서에서 추가로 보정하는 offset
        
    u32					m_n_sample_num;
    u32					m_n_cut_num;
    u32					m_n_cur_idx;
    
    ACC_DIRECTION		m_cal_direction_e;
    
    s16					m_w_ax_p_zaxis;
    s16					m_w_ay_p_zaxis;
    
    s16					m_w_ax_n_zaxis;
    s16					m_w_ay_n_zaxis;
    
    s16					m_w_az_p_xaxis;
    s16					m_w_ay_p_xaxis;
    
    s16					m_w_az_n_xaxis;
    s16					m_w_ay_n_xaxis;
    
    s16					m_w_ax_p_yaxis;
    s16					m_w_az_p_yaxis;
    
    s16					m_w_ax_n_yaxis;
    s16					m_w_az_n_yaxis;  
    
    float				m_f_cur_val_weight;
	
	u8					m_c_6plane_cal_done;
	u8					m_c_6plane_cal_misdir;
	
    pthread_t			m_impact_cal_thread_t;
    pthread_t			m_impact_event_thread_t;
    pthread_t			m_impact_logging_thread_t;
    pthread_t			m_impact_get_pos_thread_t;
    
    // micom impact calibration
	bool				m_b_impact_cal_is_running;
    
    bool 				m_b_stable_pos_proc_is_running;
    
    u8                  m_c_cal_progress;
    bool                m_b_is_gyro_cal_result_valid;
	bool                m_b_is_impact_cal_result_valid;

	bool				m_b_gyro_exist;

	// IMPACT buff
    u8					m_p_impact_buf[ADXL375B_BUF_SIZE];
    
	u16				 	m_p_impact_queue[ADXL375B_QUEUE_SIZE][ADXL375B_FIFO_SIZE][ADXL375B_NUMBER_OF_AXIS];
	u32 				m_n_queue_rear;
	u32 				m_n_queue_front;
	s32 				m_n_queue_num_of_events;

    u16					m_w_impact_count;
        
    void                impact_log_init(void);
    void				acc_impact_sensor_init(void);

    void                impact_log_read( u8* o_p_log );
    void                impact_log_write( FILE* i_p_file, u8* i_p_log );
    u32 				impact_file_num(void);
    
    void 				impact_cal_log_save(impact_cal_offset_t* i_p_impact_cal_offset, gyro_cal_offset_t* i_p_gyro_cal_offset);
	
	void				read_axis_data(u8* o_p_axis_buf);
    void 				buffer_front_clear(void);
    u16 				impact_get_maximum_value( u8* i_p_log);
    void				queue_rear_inc(void);
    void				queue_front_inc(void);
	u16*				get_buffer_point(void);
	u32					get_num_of_events(void);
		    
    void				axis_data_pack_read(adxl_data_t* o_p_data_buf_t);
    void				get_axis(u8* o_p_buf);
    
    void				interrupt_disable(void);
	void				interrupt_enable(void);    	
	void				reset_6plane_cal_variables(void);
    virtual void		acc_gyro_combo_sensor_init(void);
	virtual void		impact_cal_proc(void);
	virtual void		impact_event_proc(void);
	virtual void		impact_logging_proc(void);
	virtual void		impact_get_pos_proc(void);

public:

	CRiscImpact(int i_n_fd, CVWI2C* i_p_i2c_c, CVWSPI* i_p_spi_c, int (*log)(int,int,const char *,...));
	~CRiscImpact();
	       
    void                gyro_impact_calibration(void);
	void                sixplane_impact_calibration(void);
    void				set_6plane_sensor_cal_done(ACC_DIRECTION i_dir_e);
    void				set_6plane_sensor_cal_misdir(ACC_DIRECTION i_dir_e, u8 i_c_misdirected);
	
	virtual void		initialize(void);
	
	virtual u8			reg_read(u8 i_c_addr);
	virtual void		reg_write(u8 i_c_addr, u8 i_c_data);

	virtual void        impact_print(void);
    virtual u8          impact_get_threshold(void);
    virtual void        impact_set_threshold( u8 i_c_threshold );
    virtual bool        impact_diagnosis(void);
						
	virtual void 		get_gyro_position( s32* o_p_x, s32* o_p_y, s32* o_p_z );
    virtual void        impact_log_save(int i_n_cnt);
    virtual void        impact_read_pos( s32* o_p_x, s32* o_p_y, s32* o_p_z, bool i_b_direct = false );
    virtual void 		get_impact_offset(impact_cal_offset_t* o_p_impact_offset_t);
    virtual void 		set_impact_offset(impact_cal_offset_t* i_p_impact_offset_t);

    virtual void        impact_calibration_start(void);
    virtual void        impact_calibration_stop(void);

    virtual u8          impact_calibration_get_progress(void){return m_c_cal_progress;}
    virtual bool        impact_calibration_get_result(void){return m_b_is_impact_cal_result_valid;}
	virtual bool		gyro_calibration_get_result(void){return m_b_is_gyro_cal_result_valid;}

    virtual u16			get_impact_count(void){return m_w_impact_count;}
    virtual void		clear_impact_count(void){m_w_impact_count = 0;}
    
    virtual void		set_impact_extra_detailed_offset(impact_cal_extra_detailed_offset_t* i_p_impact_cal_extra_detailed_offset);
    virtual void		get_impact_extra_detailed_offset(impact_cal_extra_detailed_offset_t* o_p_impact_cal_extra_detailed_offset);

    virtual u8			get_6plane_sensor_cal_done(void);
    virtual u8			get_6plane_sensor_cal_misdir(void);
    virtual void		load_6plane_sensor_cal_done(u8 i_c_done);

    virtual void		stable_pos_proc_start(void);
    virtual void		stable_pos_proc_stop(void);
   	virtual bool		is_stable_proc_is_running(void);
    virtual void		set_cal_direction(ACC_DIRECTION i_dir_e);
};

class CMicomImpact: public CImpact
{
private:

    s8                  m_p_log_id[15];
    int			        (* print_log)(int level,int print,const char * format, ...);

	CVWSPI*				m_p_spi_c;
	CMicom*				m_p_micom_c;

    pthread_t			m_cal_thread_t;
    pthread_t			m_log_thread_t;

    bool				m_b_log_is_running;
    // micom impact calibration
    bool				m_b_cal_is_running;
    
    u8                  m_c_cal_progress;
    bool                m_b_cal_result;
    bool                m_b_impact_cal_result;

	// IMPACT buff
    u8					m_p_impact_buf[ADXL375B_MICOM_BUF_SIZE];
	s8 					m_p_offset_50Hz[ADXL375B_NUMBER_OF_AXIS];
    
    u32					m_w_impact_count;
        
    void                impact_log_init(void);
    void				impact_reg_init(void);
    bool                log_read( u8 i_c_idx, u8* o_p_log, u16 i_w_bw_rate );
    void                log_write( FILE* i_p_file, u8* i_p_log, u16 i_w_bw_rate );
    u32 				get_log_num(void);
    void                write_impactV2_header(FILE* i_p_file, u16 i_w_bw_rate);
	void                write_impactV2_data(FILE* i_p_file, u8* i_p_log, u16 i_w_idx);

    void 				cal_log_save(void);
	
	void				read_axis_data(u8* o_p_axis_buf);
    u16 				get_max_value( u8* i_p_log);
		    
    void				get_axis(adxl_data_t* o_p_axis_data_t);
    
    void				interrupt_disable(void);
	void				interrupt_enable(void);    	
    	
	bool				is_log_exist(void);
	u16					get_bit_map(void);
	void				get_timestamp(u32 i_n_idx, u32* o_p_timestamp, u16* o_p_bw_rate, bool* o_p_keep_flag);
	void				clear_buffer(u16 i_w_max_val, u32 i_n_idx, bool i_b_keep_flag = false);
	void				gyro_sensor_calibration(void);
	void				impact_sensor_calibration(u16 i_w_bw_rate, s8* i_p_offset);

    virtual void		acc_gyro_combo_sensor_init(void);
	virtual void		impact_logging_proc(void);
	virtual void		impact_cal_proc(void);
	virtual void		impact_event_proc(void)											{print_invalid_compatibility(__func__);}
	virtual void		impact_get_pos_proc(void)										{print_invalid_compatibility(__func__);}
public:

	CMicomImpact(CVWSPI* i_p_spi_c, CMicom* i_p_micom_c, int (*log)(int,int,const char *,...));
	~CMicomImpact();
	
	void				init_impact_cal_done(bool i_b_done){m_b_impact_cal_result = i_b_done;}
    bool                get_impact_cal_done(void){return m_b_impact_cal_result;}
    bool                check_impact_cal(void);
    void                accel_sensors_calibration(void);
    bool                impact_log_collection_routine(void);
    void				save_config(s8 i_c_offs_x, s8 i_c_offs_y, s8 i_c_offs_z);
	void       			impact_read_pos( s16* o_p_x, s16* o_p_y, s16* o_p_z, bool i_b_direct = false ){m_p_micom_c->impact_read_pos(o_p_x, o_p_y, o_p_z);}
	void				print_invalid_compatibility(const char* func_name);

	virtual void		initialize(void);
	
	virtual u8			reg_read(u8 i_c_addr);
	virtual void		reg_write(u8 i_c_addr, u8 i_c_data);

	virtual void        impact_print(void);
    virtual u8          impact_get_threshold(void);
    virtual void        impact_set_threshold( u8 i_c_threshold );
    virtual bool        impact_diagnosis(void);

	virtual void		get_gyro_position( s32* o_p_x, s32* o_p_y, s32* o_p_z );
	virtual void        impact_log_save(int i_n_cnt)									{print_invalid_compatibility(__func__);}
	virtual void        impact_read_pos( s32* o_p_x, s32* o_p_y, s32* o_p_z, bool i_b_direct = false );
	virtual void 	    get_impact_offset(impact_cal_offset_t* o_p_impact_offset_t);
    virtual void 	    set_impact_offset(impact_cal_offset_t* i_p_impact_offset_t);

    virtual void        impact_calibration_start(void);
    virtual void        impact_calibration_stop(void);
    virtual u8          impact_calibration_get_progress(void){return m_c_cal_progress;}
    virtual bool        impact_calibration_get_result(void){return m_b_cal_result;}

	virtual bool		gyro_calibration_get_result(void){return m_b_cal_result;}

	virtual u16			get_impact_count(void){return m_w_impact_count;}
	virtual void		clear_impact_count(void){m_w_impact_count = 0;}
	

	virtual void		set_impact_extra_detailed_offset(impact_cal_extra_detailed_offset_t* i_p_impact_cal_extra_detailed_offset) {print_invalid_compatibility(__func__);}
    virtual void		get_impact_extra_detailed_offset(impact_cal_extra_detailed_offset_t* o_p_impact_cal_extra_detailed_offset) {print_invalid_compatibility(__func__);}

    virtual u8			get_6plane_sensor_cal_done(void)								{return get_impact_cal_done();}	//need for compatibility
    virtual u8			get_6plane_sensor_cal_misdir(void)								{return get_impact_cal_done();} //need for compatibility
    virtual void		load_6plane_sensor_cal_done(u8 i_c_done)						{init_impact_cal_done(i_c_done);} //need for compatibility
	
	virtual void		stable_pos_proc_start(void)										{print_invalid_compatibility(__func__);}
    virtual void		stable_pos_proc_stop(void)										{print_invalid_compatibility(__func__);}
   	virtual bool		is_stable_proc_is_running(void)									{print_invalid_compatibility(__func__); return false;}
	virtual void		set_cal_direction(ACC_DIRECTION i_dir_e)						{print_invalid_compatibility(__func__);}
};

#endif /* end of _AED_H_*/
