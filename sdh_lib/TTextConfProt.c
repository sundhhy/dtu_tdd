/**
* @file 		TTextConfProt.c
* @brief		文本配置协议，参考AT之类的功能.
* @details	本协议源自dtu的配置协议,解析之后会把传入命令的分隔符替换成'\0'.一条命令处理过程中，不允许插入其他处理
* @author		author
* @date		date
* @version	A001
* @par Copyright (c): 
* 		XXX公司
* @par History:         
*	version: author, date, desc

*/
#include "TTextConfProt.h"
#include "string.h"
#include "sdhError.h"
#include "string.h"
static char *Eliminate_the_space( char *str);

static Atcmd_t	Decode_Atcmd;

void get_TTCPVer(char *buf)
{
	
	strcpy( buf, TTCP_VER);
}
 
int enter_TTCP(char *cmd)
{
	
	int i = 0;
		//进入配置模式
	while( cmd[i] != '\0')
	{
		if( cmd[i] == ' ')
		{
			
			if( i > 6)
			{
				return ERR_OK;
			}
			
		}
		else
			break;
		
		i++;
		
		
	}
	
	return ERR_FAIL;
	
}

int decodeTTCP_begin (char *cmd)
{
	int	local = 0;
	int cmd_len = strlen( cmd);
	
	char *pdeal = cmd;
	char* ptmp = NULL;
	
	TTCP_MUTEX_LOCK;
	Decode_Atcmd.cmd_type = CONFCMD_TYPE_NONE;
	if( cmd == NULL)
	{
		goto exit_fail;
	}
	
	local = strcspn( pdeal, CMD_LETTER_SET);	
	if( local >= cmd_len)		//字符串开头到结尾都找不到
		goto exit_fail;
	
	pdeal += local;
	
	//ATC或atc
	ptmp = strstr((const char*)pdeal,"ATC");	
	if( ptmp == NULL)
	{
		
		ptmp = strstr((const char*)pdeal,"atc");	
		if( ptmp == NULL)
			goto exit_fail;
	}
	ptmp += 3;
	pdeal = ptmp;
	
	//ATC+
	if( *pdeal != '+')
		goto exit_fail;
	pdeal ++;
	
	//ATC+XXX=YYY
	Decode_Atcmd.cmd_type = CONFCMD_TYPE_ATC;
	Decode_Atcmd.cmd = pdeal;
	while( *pdeal)
	{
		if( *pdeal == OFS_CMD_ARG || *pdeal == '\r' || *pdeal == '\n')
		{
			*pdeal = '\0';
			pdeal ++;
			Decode_Atcmd.arg = Eliminate_the_space(pdeal);
			return ERR_OK;
		}
		
		pdeal ++;
		
	}
	
		
	Decode_Atcmd.arg = NULL;
	return ERR_OK;
	
	exit_fail:
	TTCP_MUTEX_UNLOCK;
	return ERR_BAD_PARAMETER;
	
}
int get_cmdtype(void)
{
	return Decode_Atcmd.cmd_type;
}
char	*get_cmd(void)
{
	if( Decode_Atcmd.cmd_type == CONFCMD_TYPE_NONE)
	{
		TTCP_MUTEX_UNLOCK;
		return NULL;
	}
	
	return Decode_Atcmd.cmd;
	
}

char *get_firstarg(void)
{
	char *pdeal;
	char	*parg;
	if( Decode_Atcmd.cmd_type == CONFCMD_TYPE_NONE)
	{
		TTCP_MUTEX_UNLOCK;
		return NULL;
	}
	if( Decode_Atcmd.arg == NULL)
	{
		return NULL;
	}
	pdeal = Decode_Atcmd.arg;
	parg =  Decode_Atcmd.arg;
	
	while( *pdeal)
	{
		if( *pdeal == OFS_ARG_ARG)
		{
			*pdeal = '\0';		//给前一个参数的结尾加上0
			pdeal ++;
			Decode_Atcmd.arg = Eliminate_the_space(pdeal);
			break;
		}
		else
			pdeal ++;
		
	}
	

	if( *pdeal == '\0')
		Decode_Atcmd.arg = NULL;
	
	return parg;
}

void decodeTTCP_finish(void)
{
	
	TTCP_MUTEX_UNLOCK;
	Decode_Atcmd.cmd_type = CONFCMD_TYPE_NONE;
	Decode_Atcmd.arg = NULL;
	Decode_Atcmd.cmd = NULL;
}

static char *Eliminate_the_space( char *str)
{
	while( *str == ' ' && *str != '\0')
		str ++;
	
	return str;
	
}

