/******************************************************************************/
/**
 * @file    web.h
 * @brief   
 * @author  한명희
*******************************************************************************/
#ifndef _VW_WEB_H_
#define _VW_WEB_H_

/*******************************************************************************
*	Include Files
*******************************************************************************/
#include "web_def.h"
#include "vw_xml_util.h"
#include "vw_system.h"
#include "image_process.h"
#include "net_manager.h"

/*******************************************************************************
*	Constant Definitions
*******************************************************************************/

/*******************************************************************************
*	Type Definitions
*******************************************************************************/


/*******************************************************************************
*	Macros (Inline Functions) Definitions
*******************************************************************************/


/*******************************************************************************
*	Variable Definitions
*******************************************************************************/


/*******************************************************************************
*	Function Prototypes
*******************************************************************************/
void *         	web_routine( void * arg );

class CWeb
{
private:
    
    int			        (* print_log)(int level,int print,const char * format, ...);
    s8                  m_p_log_id[15];
    
    // capture thread
    bool	            m_b_is_running;
    pthread_t	        m_thread_t;
    bool                message_proc_start(void);
	void				message_proc_stop(void);
	
	MXML_DOCUMENT*		m_p_patient_xml_doc;
	MXML_DOCUMENT*		m_p_portable_config_xml_doc;
	
	void				modify_config( s8* i_p_data );
	bool				add_patient_info( s8* i_p_data, bool i_b_enable );
	bool				add_step( s8* i_p_data, bool i_b_enable );
	bool				send_patient_info( s8* i_p_data );
	void				make_current_patient( time_t i_time_t );
	
	patient_info_t		m_patient_info_t;
	
	void				delete_xml( MXML_DOCUMENT* doc );
	bool				info_change_portable_config( const s8* i_p_element, const s8* i_p_attribute, const s8* i_p_value );
	void				change_language( s8* i_p_data );
	void				change_image_invert( s8* i_p_data );
	void				change_use_for_human( s8* i_p_data );
	void				del_study( s8* i_p_data );
	
	bool				m_b_invert_image;
	bool				m_b_use_for_human;
	
	int					unisize(short *unicode);
	int					utf2uni(unsigned char *utf, unsigned char *uni, int i);
	int					uni2utf8(unsigned char *unicode, unsigned char *utf, int i);
	unsigned short		make_syllable(unsigned char *p);
	int					make_utf8(unsigned char *q, unsigned short k);
	
	void				mark_exp_end(char* id, char* name, char* accno, char* birth, char* studydate, char* studytime, int key, char* viewerserial, char* species, char* breed, char* responsible, int neutered);
	
	void				init_config(void);
	void				read_config(void);
	
	void				update_status(void);
	
	CImageProcess*		m_p_preview_image_process_c;
	void				preview_init(void);
	void				preview_save(void);
	
	void				config_ap( u8 i_c_select=0 );		// 0: read from config.xml, 1: AP on, 2: AP off
    
protected:
public:
    
	CWeb(int (*log)(int,int,const char *,...));
	CWeb(void);
	~CWeb();
    
	// system
	CVWSystem*          m_p_system_c;
	CNetManager*		m_p_net_manager_c;
    
    friend void *       web_routine( void * arg );
    void                message_proc(void);
	
    bool				initialize(void);
	void				set_patient_info(patient_info_t i_info_t);
	void				init_patient_info(void);
	void				update_patient_list(void);
	void				preview_save( u8* i_p_data, s32 i_n_size, s32 i_n_time );

	void				make_backup_image_list( const char* i_p_file_name );
	int					reset_admin_password(void);
	
	void				defect_correction_on( bool i_b_on=true ){m_p_preview_image_process_c->defect_set_enable(i_b_on);}
	void				gain_on( bool i_b_on=true ){ m_p_preview_image_process_c->gain_on(i_b_on); }
	void				defect_print(void){m_p_preview_image_process_c->defect_print();}
	
};


#endif /* end of _VW_WEB_H_*/

