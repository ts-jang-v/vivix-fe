#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>

#include "logd.h"

#define NETWORK_PORT	6123
#define MAX_CLIENT_NUM	10

int		g_run_main = false;

void signal_handler( int signo )
{
#if defined( _DEBUG_MAIN )
	if(signo != SIGCHLD)
		fprintf( stderr, "VIVIX_SCU: signal %d received.\n", signo );
#endif

	if( signo == SIGCHLD )
	{
		// wait until child process has exited.
		while( waitpid( ( pid_t ) -1, NULL, WNOHANG ) > ( pid_t ) 0 )
		{
			sleep( 1 );
		}
	}
	else if( signo == SIGUSR1 )
	{
		g_run_main = false;
	}
	else
	{
		sleep(1);
		g_run_main = false;
	}
}

void set_signal_handler( void ( * func )( int ), int sig, int flags )
{
	struct sigaction act;
	
	
	act.sa_handler = func;
	sigemptyset( & act.sa_mask );
	sigaddset( & act.sa_mask, sig );
	act.sa_flags = flags;
	sigaction( sig, & act, NULL );
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void format_ext4(char * i_p_dev)
{
    char        p_cmd[100];
    sprintf(p_cmd,"mkfs.ext4 %s -E nodiscard",i_p_dev);
    system(p_cmd);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool mount_ext4(char * i_p_dev,char * i_p_dest)
{
    char        p_cmd[100];

    memset(p_cmd,0,100);
    sprintf(p_cmd,"mount -t ext4 %s %s", i_p_dev, i_p_dest);

    if(system(p_cmd) == 0)
    {
        return true;
    }
    else
    {
        return false;
    }

    return true;
}

int main( int argc, char ** argv )
{
	argc = argc;
	int ret = 0;
	int n_port = NETWORK_PORT;
	LOGD log;
	
	if( mount_ext4( (char*)SYS_MOUNT_MMCBLK_P3, (char*)SYS_LOG_PATH ) == false )
	{        
		format_ext4( (char*)SYS_MOUNT_MMCBLK_P3 );
		mount_ext4( (char*)SYS_MOUNT_MMCBLK_P3, (char*)SYS_LOG_PATH );
	}
	
	if(argc == 2)
		n_port = atoi(argv[1]);
	
	if(log.initialize(n_port, MAX_CLIENT_NUM) == false)
	{
		printf("[LOGD main] init failed\n");	
		ret = -1;
		goto STOP;
	}

	set_signal_handler( signal_handler, SIGINT, 0 );
	set_signal_handler( signal_handler, SIGTERM, 0 );
	set_signal_handler( signal_handler, SIGQUIT, 0 );
	set_signal_handler( signal_handler, SIGCHLD, 0 );
	set_signal_handler( signal_handler, SIGUSR1, 0 );
	set_signal_handler( SIG_IGN, SIGPIPE, 0 );


	

	if(log.get_config()->get_level() <= 0)
	{
		log.get_config()->set_level(5);
	}

	printf("log_level %d\n",log.get_config()->get_level());
	log.check_location(log.get_config()->get_location());
	log.check_capacity();
	
	if(log.start() == false)
	{
		printf("[LOGD main] start failed\n");
		ret = -1;	
		goto STOP;
	}
	
	g_run_main = true;
	sleep(2);
	while(g_run_main == true)
	{
		sleep(1);
	}
		
STOP:
	if(log.stop() == false)
	{
		printf("[LOGD main] stop failed\n");	
	}

	exit( EXIT_SUCCESS );	

	return ret;
}


