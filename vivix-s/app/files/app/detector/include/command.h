/*
 * command.h
 *
 *  Created on: 2013. 12. 17.
 *      Author: myunghee
 */

 
#ifndef COMMAND_H_
#define COMMAND_H_

/*******************************************************************************
*	Include Files
*******************************************************************************/
#include "typedef.h"
#include "capture.h"
#include "control_server.h"
#include "scu_server.h"
#include "calibration.h"
#include "net_manager.h"
#include "diagnosis.h"
#include "web.h"

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
class CCommand
{
private:
	bool	m_bis_running;

	s32		get_line(s8 *s, s32 i_n_max);
	u32 	cmd_parse(s8 *pStrCmd);
	s64 	strtol(const s8 *cp, s8 **endp, u32 base);
	u32 	strtoul(const s8 *cp, s8 **endp, u32 base);
	
	s32 	parse_args(s8 *line, s8 *argv[]);
	void    print_error( u32 i_n_err );
	
	u32		cmd_test_tcp(int argc,char *argv[]);
	u32		cmd_test_udp(int argc,char *argv[]);
	u32		cmd_env(int argc,char *argv[]);
	u32		cmd_cal(int argc,char *argv[]);
	u32		cmd_offset(int argc,char *argv[]);
	u32 	cmd_logical_digital_gain(int argc,char *argv[]);
	u32 	cmd_gain_compensation(int argc,char *argv[]);
	u32		cmd_exp_req(int argc,char *argv[]);
	u32		cmd_cipher(int argc,char *argv[]);
	u32		cmd_server_test(int argc,char *argv[]);
	u32		cmd_resend(int argc,char *argv[]);
	
	u32     cmd_fpga(int argc,char *argv[]);
	u32     cmd_roic(int argc,char *argv[]);
	u32     cmd_test_pattern(int argc,char *argv[]);
	u32     cmd_panel(int argc,char *argv[]);	

	u32     cmd_system(int argc,char *argv[]);
	u32     cmd_version(int argc,char *argv[]);
	
	u32     cmd_network(int argc,char *argv[]);
	u32     cmd_discovery(int argc,char *argv[]);
	
	manufacture_info_t  m_factory_info_t;
	u32     cmd_factory(int argc,char *argv[]);
	void    parse_mac( s8* i_p_mac, u8* o_p_mac );
	void    factory_print(void);
	
	u32     cmd_debug(int argc,char *argv[]);
	u32     cmd_aed(int argc,char *argv[]);
	u32     cmd_state(int argc,char *argv[]);
	u32     cmd_test(int argc,char *argv[]);
	u32     cmd_update(int argc,char *argv[]);
	
	u32     cmd_power_manage(int argc,char *argv[]);
	u32     cmd_gain(int argc,char *argv[]);
	
	u32     cmd_battery(int argc,char *argv[]);
	u32     cmd_oper_info(int argc,char *argv[]);
	u32 	cmd_uart_display(int argc,char *argv[]);
	u32     cmd_interrupt(int argc,char *argv[]);
	
	u32     cmd_image(int argc,char *argv[]);
	
	u32 	cmd_get_impact_reg(int argc,char *argv[]);
	u32 	cmd_send_cmd_to_micom(int argc,char *argv[]);
	
	u32 	cmd_monitor(int argc,char *argv[]);
	u32 	cmd_pmic(int argc,char *argv[]);
	
	u32 	cmd_crc(int argc,char *argv[]);
	u32     cmd_diag(int argc,char *argv[]);
	u32		cmd_phy(int argc,char *argv[]);
	
	u32     cmd_tact(int argc,char *argv[]);
	
	u32     cmd_packet(int argc,char *argv[]);
	
	u32     cmd_ap(int argc,char *argv[]);
	u32     cmd_web(int argc,char *argv[]);

	u32 	cmd_oled(int argc,char *argv[]);
	
	u32     cmd_xml(int argc,char *argv[]);
	u32 	cmd_generate_defect(int argc,char *argv[]);

	u32 	cmd_power_ctrl(int argc,char *argv[]);
	
	u32 	cmd_double_readout(int argc,char *argv[]);
	
	u32		cmd_impact(int argc,char *argv[]);
	
	u32		cmd_eeprom(int argc,char *argv[]);
	u32		cmd_prom(int argc,char *argv[]);
	u32 	cmd_micom(int argc,char *argv[]);
	u32		cmd_custom_set(int argc,char *argv[]);

protected:

public:
    void	exit(void);
    u32		proc(void);
    
    CCapture*           m_p_capture_c;
	// system
	CVWSystem*          m_p_system_c;
    CControlServer*     m_p_control_svr_c;
    CSCUServer*         m_p_scu_svr_c;
    CCalibration*       m_p_calibration_c;
    CNetManager*		m_p_net_manager_c;
    CDiagnosis*			m_p_diagnosis_c;
    CWeb*				m_p_web_c;
	CMicom*				m_p_micom_c;
	
    CCommand(void);
    ~CCommand(void);
};

#endif /* end of COMMAND_H_ */