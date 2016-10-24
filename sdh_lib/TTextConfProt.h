#ifndef _TTEXTCNFPT_H__
#define _TTEXTCNFPT_H__
#include "stdint.h"

#define CONFCMD_TYPE_NONE	0
#define CONFCMD_TYPE_ATC	1


#define TTCP_MUTEX_LOCK
#define TTCP_MUTEX_UNLOCK

#define OFS_CMD_ARG			'='		
#define OFS_ARG_ARG			','
#define CMD_LETTER_SET		"AaTtCc"	//命令可能的起始字符集

typedef struct
{
	int		cmd_type;
	
	char	*cmd;
	char	*arg;
	
	
	
}Atcmd_t;
int enter_TTCP(char *cmd);
int decodeTTCP_begin (char *cmd);
int get_cmdtype(void);
char	*get_cmd(void);
char *get_firstarg(void);
void decodeTTCP_finish(void);


#endif
