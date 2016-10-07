/*************************************************
Copyright (C), 
File name: filesys.c
Author:		sundh
Version:	v1.0
Date: 		14-03-04
Description: 
在w25q上实现一个简易的文件系统。
在访问文件系统的时候，使用了互斥锁，来避免对flash访问出现竞争
为每个任务提供一个文件描述符，来避免多个任务访问同一个文件时产生混乱。
不支持目录结构。所有存放在flash中的文件都是平级的。
扇区0保存文件信息。称为特殊扇区
扇区1,2保存每个页的使用情况，每一个bit代表一个页的情况，如果该页已经被使用，则对应的bit清0.
特殊扇区的起始位置存放的一定是文件信息
flash中最后512kB的内存要用于存储系统镜像，不能用于文件的存储。
缓存一个扇区的大小，当本次对flash的访问扇区与上次不相同的时候，才去把缓存刷新到flash中去.

扇区0的文件信息由3部分组成：
1、扇区头信息：已经创建的文件数量及版本号
2、文件信息存储区
3、文件存储区间存储区


Others: 
Function List: 
1. fs_write
History: 
1. Date:
Author:
Modification:
16-03-16
在空间不够时，新分配空间后返回一个INCREASE_PAGE，来提示调用者
2. ...
*************************************************/
#include "w25q.h"
#include "list.h"
#include "filesys.h"
#include <stdio.h>
#include <string.h>
#include "fsl_debug_console.h"
#include "power_manager.h"
#include "OsTime.h"
#include "DataProxy.h"
#include <stdarg.h>



static List L_File_opened;
static int Task_state = 0;
static int Task_old_state = 0;
static Osmutex_t	*FS_Mutex;
static task_block my_tb[F_State_Number] = {NULL};
static short WR_addr = 0, MEM_sector = FLASH_USEE_SECTOR1;
uint8_t	Flash_buf[SECTOR_SIZE];
uint8_t	*Ptr_Flash_buf;		//用于读取扇区1或2的内存使用情况
static area_t *The_area;


static	uint8_t		Buf_chg_flag = 0;				//缓存内容被修改的标志,缓存被写入flash时标志清零
static uint16_t	Src_sector = INVALID_SECTOR;		//缓存中的内容的扇区
static uint16_t	The_sector = 0, The_page = 0;
static int The_int = 0;
static char Flash_err_flag = 0;

static int Create_file_easy(char *name, file_Descriptor_t **fd);
static int Create_file_hard(char *name, file_Descriptor_t **fd);
static bool check_bit(uint8_t *data, int bit);
static void clear_bit(uint8_t *data, int bit);
static void set_bit(uint8_t *data, int bit);
static int get_available_page( uint8_t *data, int start, int end, int len);
static int page_malloc( area_t *area, int len);
static int page_free( area_t *area, int area_num);
static int modify_file_area( file_Descriptor_t *fd, area_t *area);
static error_t read_flash( uint16_t sector);
static error_t flush_flash( uint16_t sector);

static int mach_file(const void *key, const void *data)
{
	uint8_t *name = ( uint8_t*)key;
	file_Descriptor_t	*fd = (file_Descriptor_t *)data;
	return strcmp( key, fd->name);
	
	
	
}

error_t filesys_init(void)
{
	
	if( w25q_init() != RET_SUCCEED)
	{
		
		Task_state = F_State_Err;
		Flash_err_flag = 1;
		return ERROR_T(ERR_FLASH_UNAVAILABLE);
		
	}
	else
		Flash_err_flag = 0;
	FS_Mutex = malloc( sizeof(Osmutex_t));
	if( FS_Mutex == NULL)
		return ERROR_T(ERROR_MEM_UNAVAILABLE);
	Os_MutexCreate( FS_Mutex);
	
	list_init( &L_File_opened, free, mach_file);
	return RET_SUCCEED;
	
}

error_t filesys_close(void)
{
	
	if( w25q_close() != RET_SUCCEED)
	{
		
		Task_state = F_State_Err;
		return ERROR_T(ERR_FLASH_UNAVAILABLE);
		
	}
	free( FS_Mutex);
	return RET_SUCCEED;
	
}

//从文件记录扇区0中找到指定名字的文件记录信息
error_t fs_open(char *name, file_Descriptor_t **fd)
{
	error_t ret = 0;
	file_info_t	*file_in_storage;
	file_Descriptor_t *pfd;
	os_status_t os_ret;
	storage_area_t	*src_area;
	area_t					*dest_area;	
	sup_sector_head_t	*sup_head;
	ListElmt			*ele;
	short 				i,j, k;
	static char step = 0;
	if( Flash_err_flag )
		return ERROR_T(ERR_FLASH_UNAVAILABLE);
	switch(step)
	{
		case 0:
			//不能允许多个任务同时执行flash的读写操作，会引起混乱
			os_ret = Os_MutexLock( FS_Mutex, 0);
			if( os_ret != kStatus_OS_Success)
				return ERROR_T(ERR_FILESYS_BUSY);
			
			ele = list_get_elmt( &L_File_opened,name);		//先从已经打开的文件中查找是否已经被其他任务打开过
			if(  ele!= NULL)
			{
				pfd = list_data(ele);
				pfd->reference_count ++;
				*fd = pfd;
				pfd->rd_pstn[get_current_task_pid()] = 0;
				pfd->wr_pstn[get_current_task_pid()] = 0;
				Os_MutexUnlock( FS_Mutex);
				return RET_SUCCEED;
				
			}
			step = 1;			
		case 1:
			ret = read_flash(FILE_INFO_SECTOR);
			if( ret == RET_SUCCEED)
				step = 2;
			else
				return RET_PROCESSING;
		case 2:
			sup_head = ( sup_sector_head_t *)Flash_buf;
			if( sup_head->ver[0] != 'V')			//第一次上电，擦除整块flash并写入头信息
			{
				Os_MutexUnlock( FS_Mutex);
				return ERROR_T(ERR_FILESYS_ERROR);
				
			}	
			file_in_storage = ( file_info_t *)( Flash_buf + sizeof(sup_sector_head_t));	
			for( i = 0; i < FILE_NUMBER_MAX; i ++)
			{
				
				if( strcmp(file_in_storage->name, name) == 0x00 )	
				{
					//找到了文件
					pfd = (file_Descriptor_t *)malloc(sizeof( file_Descriptor_t));
					dest_area = malloc( file_in_storage->area_total * sizeof( area_t));
					
					pfd->area = dest_area;
					//把文件的存储区间赋值给文件描述符
					src_area = ( storage_area_t *)( Flash_buf + sizeof(sup_sector_head_t) + FILE_NUMBER_MAX * sizeof(file_info_t));
					k = 0;
					for( j = 0; j < file_in_storage->area_total ;)
					{
						if( src_area->file_id == file_in_storage->file_id)		
						{
							
							pfd->area[src_area->seq].start_pg = src_area->area.start_pg;
							pfd->area[src_area->seq].pg_number = src_area->area.pg_number;
							j ++;
						}
						k++;
						if( k > STOREAGE_AREA_NUMBER_MAX)
						{
							if( j == 0)		//文件被损坏
							{
								free( dest_area);
//								free( pfd);
								pfd->reference_count = 1;
								strcpy( pfd->name, name);
								pfd->area = NULL;
								*fd = pfd;
								
								step = 0;
								Os_MutexUnlock( FS_Mutex);
								return ERROR_T(ERR_FILE_ERROR);
								
							}
							
						}
						src_area ++;						
					}				
//					pfd->wr_size = file_in_storage->wr_bytes;
					pfd->area_total = j ;			//把实际找到的区间数量赋值给文件描述符
					file_in_storage->area_total = j;			//更新实际的区间数量
					strcpy( pfd->name, name);
					pfd->reference_count = 1;
					memset( pfd->rd_pstn, 0, sizeof( pfd->rd_pstn));
					memset( pfd->wr_pstn, 0, sizeof( pfd->wr_pstn));
					*fd = pfd;
					step = 0;
					list_ins_next( &L_File_opened, L_File_opened.tail, pfd);
					Os_MutexUnlock( FS_Mutex);
					return RET_SUCCEED;
				}
				
				file_in_storage ++;
				
				
				
			}
			*fd = NULL;
			step = 0;
			Os_MutexUnlock( FS_Mutex);
			return ERROR_T(ERR_OPEN_FILE_FAIL);
	}
}
error_t wait_fs(void)
{
	error_t ret;
	int count = 0;
	while(1)
	{
		ret = filesys_dev_check();
		if( ret == RET_SUCCEED)
			break;
		count ++;
		
		if( count >20000)
			break;
	}
	
}

//格式化文件系统，删除所有的内容（保留区除外）
error_t fs_format(void)
{
	sup_sector_head_t	*sup_head;
	error_t ret;
	os_status_t os_ret;
	int count = 0;
	WR_addr = 0;
	os_ret = Os_MutexLock( FS_Mutex, 0);
	if( os_ret != kStatus_OS_Success)
		return ERROR_T(ERR_FILESYS_BUSY);
	
	//擦除整个文件系统
	while(1)
	{
		ret = w25q_Erase_block(WR_addr);
		if( ret == RET_SUCCEED)
		{
			if( 	WR_addr < W25Q_flash.block_num - PAGE_REMIN_FOR_IMG/BLOCK_HAS_SECTORS/SECTOR_HAS_PAGES )
			{
				WR_addr ++;
				
				
			}
			else
				break;
			
		}
		
		
	}
	//读取扇区FILE_INFO_SECTOR
	//因为刚擦除过，所有的flash内容都是0xff，也就不必去真的读取了
	memset( Flash_buf, 0xff, SECTOR_SIZE);		
	//设置上次读取的扇区为FILE_INFO_SECTOR
	Src_sector = FILE_INFO_SECTOR;
	
	sup_head = ( sup_sector_head_t *)Flash_buf;
	
	sup_head->file_count = 0;
	memcpy( sup_head->ver, Ver, 6);
	Buf_chg_flag = 1;
	
	Os_MutexUnlock( FS_Mutex);
	return RET_SUCCEED;
	
}


//文件的存储信息结构体都是4字节对齐的，所以不必考虑字节对齐问题
//前置条件:在创建文件的时候,特殊块是不能够存在空洞的,所以在删除操作的时候,要去整理特殊扇区中的内存
//长度是可选参数
error_t fs_creator(char *name, file_Descriptor_t **fd, int len)
{
	static char step = 0;
	
	int i = 0;
	error_t ret = 0;
	os_status_t os_ret;
	
	sup_sector_head_t	*sup_head;
	file_info_t	*file_in_storage;
	file_info_t	*creator_file;
	storage_area_t	*target_area;
	file_Descriptor_t *p_fd;
	
	//存储区不正确就不处理直接返回
	if( Flash_err_flag )
		return RET_SUCCEED;
	if (len == 0)
		len = 1;
	switch( step)
	{
		case 0:
			os_ret = Os_MutexLock( FS_Mutex, 0);
			if( os_ret != kStatus_OS_Success)
				return ERROR_T(ERR_FILESYS_BUSY);
			
			step ++;
			
		case 1:
			The_area = malloc( sizeof( storage_area_t));
			if( The_area == NULL)
				goto err1;
			step ++;
		case 2:
	
			ret = page_malloc( The_area, len);
			if( ret < 0) {
				free( The_area);
				goto err1;
			}
			else if( ret == RET_SUCCEED)
				step ++;
			else
				return ret;
		
		case 3:
			ret = read_flash(FILE_INFO_SECTOR);
			if( ret == RET_SUCCEED)
				step ++;
			else
				return ret;
		case 4:
			sup_head = (sup_sector_head_t *)Flash_buf;
			creator_file = ( file_info_t *)( Flash_buf+ sizeof( sup_sector_head_t));
			
			
			
			if( sup_head->file_count > FILE_NUMBER_MAX)
				goto err2;
			
			
			//检查文件是否已经存在
			file_in_storage = ( file_info_t *)( Flash_buf + sizeof(sup_sector_head_t));
			
			for( i = 0; i < FILE_NUMBER_MAX; i ++)
			{
				
				if( strcmp(file_in_storage->name, name) == 0x00 )	
				{
					
						
						goto err2;
						
					
					
				}
				
				file_in_storage ++;
				
				
				
			}
			
			
			//找到第一个没有被使用的文件信息存储区
			i = 0;
			for( i = 0; i < FILE_NUMBER_MAX; i ++)
			{
				if( creator_file->file_id == 0xff)
					break;
				creator_file ++;
				
			}
			if ( i == 	FILE_NUMBER_MAX)
				goto err2;
			
			
			
			
			target_area = ( storage_area_t *)( Flash_buf+ sizeof( sup_sector_head_t) + FILE_NUMBER_MAX * sizeof(file_info_t));
			
			for( i = 0; i < STOREAGE_AREA_NUMBER_MAX; i ++)
			{
				if( target_area[i].file_id == 0xff)
				{
					target_area[i].file_id = sup_head->file_count;
					target_area[i].seq = 0;
					target_area[i].area.start_pg = The_area->start_pg;
					target_area[i].area.pg_number = The_area->pg_number;
					break;
				}
				
				
			}
			if( i == STOREAGE_AREA_NUMBER_MAX)
				goto err2;
			
			
			strcpy( creator_file->name, name);
			creator_file->file_id = sup_head->file_count;
			creator_file->area_total = 1;
//			creator_file->wr_bytes = 0;
			sup_head->file_count ++;
			
			Buf_chg_flag = 1;
			
			p_fd = malloc( sizeof( file_Descriptor_t));
			if( p_fd == NULL) {
				step = 0;
				return ERROR_T(ERROR_MEM_UNAVAILABLE);
			}
			
			strcpy( p_fd->name, name);
			memset( p_fd->rd_pstn, 0 , sizeof( p_fd->rd_pstn));
			memset( p_fd->wr_pstn, 0 , sizeof( p_fd->wr_pstn));
			p_fd->wr_size = 0;
			p_fd->reference_count = 1;
			p_fd->area = malloc( sizeof( area_t));
			
			//装载存储区的地址区间到文件描述符中的存储区间链表中去
			p_fd->area->start_pg = The_area->start_pg;
			p_fd->area->pg_number = The_area->pg_number;
			p_fd->area_total = 1;
			*fd = p_fd;
			step += 1;
			
			list_ins_next( &L_File_opened, L_File_opened.tail, p_fd);
			
			
			
		//保存信息到flash中
		case 5:
				step = 0;
				free( The_area);
				os_ret = Os_MutexUnlock( FS_Mutex);
				return RET_SUCCEED;
	}
err1:
	step = 0;
	Os_MutexUnlock( FS_Mutex);
	return ERROR_T(ERR_CREATE_FILE_FAIL);
err2:
	step = 8;
	ret = page_free( The_area, 1);
	if( ret == RET_SUCCEED )
	{
			step = 0;		
			Os_MutexUnlock( FS_Mutex);
			return ERROR_T(ERR_NO_SUPSECTOR_SPACE);
	}
	else
		return ret;
	
	
	
}

//定位当前位置在存储区间的起始页,并返回区间
static area_t *locate_page( file_Descriptor_t *fd, uint32_t pstn, uint16_t *pg)
{
	short i = 0;
	uint32_t offset = pstn;
	int	lct_page = 0;

	
	
	for( i = 0; i < fd->area_total; i ++)
	{
		//位置位于本区间内部
		if( fd->area[i].pg_number *PAGE_SIZE > offset)
		{
			
			lct_page = fd->area[i].start_pg + offset/PAGE_SIZE ;
			*pg = lct_page;
			return &fd->area[i];
		}
		//不知本区间范围内，去下一个区间查找
		offset -= fd->area[i].pg_number * PAGE_SIZE;
		
		
	}
	

	
	return NULL;		//无法定位，这种情况出现的话，应该新分配flash空间
	
	
}

//flash不够时要能够给文件新分配flash页
//
error_t fs_write( file_Descriptor_t *fd, uint8_t *data, int len)
{
	
	
//	ListElmt			*ele;
	error_t 			ret;
	static char step = 0;
	os_status_t os_ret;
	int 				i, limit;
	int  				myid = get_current_task_pid();
	//存储区不正确就不处理直接返回
	if( Flash_err_flag )
		return RET_SUCCEED;
	switch( step)
	{
		case 0:
			os_ret = Os_MutexLock( FS_Mutex, 0);
			if( os_ret != kStatus_OS_Success)
				return ERROR_T(ERR_FILESYS_BUSY);
			The_int = len;
			
			
find_location:		
			The_area = locate_page( fd, fd->wr_pstn[myid], &The_page);
			if( The_area == NULL)
			{
				
				step = 0;
				Os_MutexUnlock( FS_Mutex);
				return ERROR_T(ERR_FILE_FULL);
				
			}
			else
			{
				The_sector = 	The_page/SECTOR_HAS_PAGES;
				if( The_sector < SUP_SECTOR_NUM)
				{
					step = 0;
					Os_MutexUnlock( FS_Mutex);
					return ERROR_T(ERR_FILE_ERROR);
					
				}
				step ++;	
				
			}
			
		case 1:
			ret = read_flash(The_sector);
			if( ret == RET_SUCCEED)
				step ++;
			else
				return ret; 
		case 2:
			//计算在当前扇区中的偏移
			i = ( The_page % SECTOR_HAS_PAGES)*PAGE_SIZE + fd->wr_pstn[myid] % PAGE_SIZE;
			if( The_area->pg_number + The_area->start_pg > ( The_sector + 1 ) *  SECTOR_HAS_PAGES )				//文件的结束位置超出本扇区，以扇区的大小为上限
					limit  = SECTOR_SIZE;
			else		//文件的结束位置在本扇区内部，以文件结束位置在本扇区中的相对位置为上限
					limit = ( The_area->start_pg +  The_area->pg_number - The_sector *  SECTOR_HAS_PAGES) * PAGE_SIZE;
			while( The_int)
			{
				if( i >= SECTOR_SIZE || i > limit)		 //超出了当前的扇区或者区间范围
				{
					break;
				}
				

				if( Flash_buf[i] != *data)
				{
					Flash_buf[i] = *data;
					Buf_chg_flag = 1;	
				}
				i ++;
				fd->wr_pstn[myid] ++;
				The_int --;
				data++;
				
			}
			
			
//			if( fd->wr_size < fd->wr_pstn[myid])
			fd->wr_size =  fd->wr_pstn[myid];
			
			//将当前扇区的内容先写回flash
			step++;
		case 3:
			step = 0;
			if( The_int)			//还有数据没写完
			{
				
				goto find_location;
				
			}
			else {
				
				Os_MutexUnlock( FS_Mutex);
				return RET_SUCCEED;
			}
	}
	

	
	
}


//文件增加flsh内存,未验证！
error_t fs_expansion(file_Descriptor_t *fd, int len)
{
	
	error_t 			ret;
	static char step = 0;
	os_status_t os_ret;
	int 				i, limit;
	int  				myid = get_current_task_pid();
	
	switch( step)
	{
		case 0:
			os_ret = Os_MutexLock( FS_Mutex, 0);
			if( os_ret != kStatus_OS_Success)
				return ERROR_T(ERR_FILESYS_BUSY);
			The_area = malloc( sizeof( area_t));
			step ++;			
		case 1:
			ret = page_malloc( The_area, len );		
			if( ret < 0) {
				step += 2;
				return RET_DELAY;
			}
			else if( ret == RET_SUCCEED)
				step ++;
			else
				return ret;
		case 2:
				ret = modify_file_area( fd, The_area);
				if( ret < 0) {
					step ++;
					
					
				}
				else if( ret == RET_SUCCEED) {
					step = 0;
					free(The_area);
					Os_MutexUnlock( FS_Mutex);
					return RET_SUCCEED;
				}
				else
					return ret;
		case 3:			//
			ret = page_free( The_area, 1);
			if( ret == RET_SUCCEED )
			{
					step = 0;		
					Os_MutexUnlock( FS_Mutex);
					free(The_area);
					return ERROR_T(ERR_NO_FLASH_SPACE);	
			}
			else
				return ret;
		
		
		
	}

	
	
	
	
}


int fs_read( file_Descriptor_t *fd, uint8_t *data, int len)
{
	
		error_t 			ret;
		static char step = 0;
		os_status_t os_ret;
		int 				i, limit;
		int  				myid = get_current_task_pid();
		//存储区不正确就不处理直接返回
	if( Flash_err_flag )
		return RET_SUCCEED;
	
		switch( step)
		{
			case 0:
				os_ret = Os_MutexLock( FS_Mutex, 0);
				if( os_ret != kStatus_OS_Success)
					return ERROR_T(ERR_FILESYS_BUSY);
				
				The_int = len;
	find_location:			
				The_area = locate_page( fd, fd->rd_pstn[myid], &The_page);
				if( The_area == NULL)
				{
					Os_MutexUnlock( FS_Mutex);
					return ERROR_T(ERR_FILE_EMPTY);
				}
				else
				{
					The_sector = 	The_page/SECTOR_HAS_PAGES;
					step += 1;	
					
				}
			case 1:
				ret = read_flash(The_sector);
				if( ret == RET_SUCCEED)
					step ++;
				else
					return ret; 
			case 2:
				//读写的时候，要考虑文件的结尾不在本扇区的情况
				i = ( The_page % SECTOR_HAS_PAGES)*PAGE_SIZE + fd->rd_pstn[myid] % PAGE_SIZE;
				
				if( The_area->start_pg +  The_area->pg_number > ( The_sector + 1) *  SECTOR_HAS_PAGES)	//结尾位于本扇区外
					limit = SECTOR_SIZE;
				else	//结尾位于本扇区内
					limit = ( The_area->start_pg +  The_area->pg_number - The_sector *  SECTOR_HAS_PAGES) * PAGE_SIZE;
				
				while( The_int)
				{
					if( i >= SECTOR_SIZE || i > limit)		 //超出了当前的扇区或者区间范围
					{
						break;
					}
					*data++ = Flash_buf[i];
					i ++;
					fd->rd_pstn[myid] ++;
					The_int --;
						
					
				}
				
				//将当前扇区的内容先写回flash
				step ++;
		case 3:
			step = 0;
			if( The_int)			//还有数据没读完
			{
				
				goto find_location;
				
			}
			else {
				len -= The_int;
				Os_MutexUnlock( FS_Mutex);
				return RET_SUCCEED;
			}	
			
			
		}
	
}

error_t fs_lseek( file_Descriptor_t *fd, int offset, int whence)
{
	error_t ret = 0;
	char myid = get_current_task_pid();
	uint16_t 	pos = 0;
	uint8_t data[4];
	//存储区不正确就不处理直接返回
	if( Flash_err_flag )
		return RET_SUCCEED;
	switch( whence)
	{
		case WR_SEEK_SET:
			fd->wr_pstn[ myid] = offset;
			break;
		case WR_SEEK_CUR:
			fd->wr_pstn[ myid] += offset;
			break;
		
		
		case WR_SEEK_END:			//连续5个0xff作为结尾
			fd->rd_pstn[ myid] = 0;
			while(1)
			{
				
				while(1)
				{
					ret = fs_read( fd,data, 1);
					if( ret == RET_SUCCEED)
					{
						if( data[0] == 0xff)
						{
							pos = fd->rd_pstn[ myid];
							break;
						}
					}
					else if( ret == RET_PROCESSING) 
					{
						wait_fs();
					}	
					else if( INVERSE_ERROR( ret) == ERR_FILE_EMPTY)
					{
						fd->wr_pstn[ myid] = 0;
						return RET_SUCCEED;
						
					}
				}
				
				while(1)
				{
					ret = fs_read( fd, data, 4);
					if( ret == RET_SUCCEED)
					{
						if( data[0] == 0xff && data[1] == 0xff && data[2] == 0xff && data[3] == 0xff)
						{
							fd->wr_pstn[ myid] = pos;
							return RET_SUCCEED;
						}
						break;
					}
					else if( ret == RET_PROCESSING) 
					{
						wait_fs();
					}	
					else if( INVERSE_ERROR( ret) == ERR_FILE_EMPTY)
					{
						fd->wr_pstn[ myid] = 0;
						return RET_SUCCEED;
						
					}
				}
				
				
				
			}
			
//			fd->wr_pstn[ myid] = offset + fd->wr_size;
		
			break;
		
		case RD_SEEK_SET:
			fd->rd_pstn[ myid] = offset;
			break;
		case RD_SEEK_CUR:
			fd->rd_pstn[ myid] += offset;
			break;
		
		
		case RD_SEEK_END:
			
		
			break;
		case GET_WR_END:
			return fd->wr_pstn[ myid];
		
		default:
			break;
		
	}
	
	
	
	
}

error_t fs_close( file_Descriptor_t *fd)
{
	char myid = get_current_task_pid();
	void 							*data = NULL;
	ListElmt           *elmt;
	int i, ret;
	os_status_t os_ret;
	file_info_t	*file_in_storage;
	static char step = 0;
	//存储区不正确就不处理直接返回
	if( Flash_err_flag )
		return RET_SUCCEED;
	switch( step)
	{
		case 0:
			os_ret = Os_MutexLock( FS_Mutex, 0);
			if( os_ret != kStatus_OS_Success)
				return ERROR_T(ERR_FILESYS_BUSY);
			fd->reference_count --;
	
	
			//把写的最长的那个长度作为文件的长度
//			for( i = 0; i < Prio_task_end; i ++)
//			{
//				if( fd->wr_pstn[ i] > fd->wr_size)
//					fd->wr_size = fd->wr_pstn[ i];
//				
//			}
			
			fd->rd_pstn[ myid] = 0;
			fd->wr_pstn[ myid] = 0;
			
			
			
			if( fd->reference_count == 0)
			{
				step ++;
			}
			else {
				Os_MutexUnlock( FS_Mutex);
				return RET_SUCCEED;
			}
		case 1:
			ret = read_flash( FILE_INFO_SECTOR);
			if( ret == RET_SUCCEED)
				step ++;
			else
				return ret; 
		case 2:
			file_in_storage = ( file_info_t *)( Flash_buf + sizeof(sup_sector_head_t));
			for( i = 0; i < FILE_NUMBER_MAX; i ++)
			{
				if( strcmp(file_in_storage->name, fd->name) == 0x00 )
				{
//					file_in_storage->wr_bytes = fd->wr_size;
					Buf_chg_flag = 1;
					break;
				}
				file_in_storage ++;	
			}

			step = 0;
			elmt = list_get_elmt( &L_File_opened,fd->name);
			list_rem_next( &L_File_opened, elmt, &data);
			free( fd->area);
			
			free(fd);
			Os_MutexUnlock( FS_Mutex);
			return RET_SUCCEED;
	}
	
}


error_t fs_delete( file_Descriptor_t *fd)
{
	char myid = get_current_task_pid();
	void 							*data = NULL;
	error_t ret = 0;
	file_info_t	*file_in_storage;
	file_Descriptor_t *pfd;
	os_status_t os_ret;
	storage_area_t	*src_area,*dest_area;
	sup_sector_head_t	*sup_head;
	ListElmt			*ele;
	short i, j, k;
	static char step = 0;
	//存储区不正确就不处理直接返回
	if( Flash_err_flag )
		return RET_SUCCEED;
	switch(step)
	{
		case 0:
			//不能允许多个任务同时执行flash的读写操作，会引起混乱
			os_ret = Os_MutexLock( FS_Mutex, 0);
			if( os_ret != kStatus_OS_Success)
				return ERROR_T(ERR_FILESYS_BUSY);
			
			fd->reference_count --;
			if( fd->reference_count > 0)
				goto err1;
			
			
			ele = list_get_elmt( &L_File_opened,fd->name);		//先从已经打开的文件中查找是否已经被其他任务打开过
			if(  ele!= NULL)
			{
				list_rem_next( &L_File_opened, ele, &data);
			}
			
			
			step ++;
			
		case 1:
			ret = read_flash(FILE_INFO_SECTOR);
			if( ret == RET_SUCCEED)
				step ++;
			else
				return RET_PROCESSING;
		case 2:
			sup_head = ( sup_sector_head_t *)Flash_buf;
			file_in_storage = ( file_info_t *)( Flash_buf + sizeof(sup_sector_head_t));
			k = 0;
			for( i = 0; i < FILE_NUMBER_MAX; i ++)
			{
				if( strcmp(file_in_storage->name, fd->name) == 0x00 )
				{
					
					
					src_area = ( storage_area_t *)( Flash_buf + sizeof(sup_sector_head_t) + FILE_NUMBER_MAX * sizeof(file_info_t) );
					for( j = 0; j < STOREAGE_AREA_NUMBER_MAX; j ++)
					{
						if( src_area[j].file_id == file_in_storage->file_id)
						{
							memset( &src_area[j], 0xff, sizeof(storage_area_t));
							k ++;
							
						}
						if( k == file_in_storage->area_total)
							break;
						
						
					}
					
					Buf_chg_flag = 1;
					memset( file_in_storage, 0xff, sizeof(file_info_t));
					break;
				}
				
				file_in_storage ++;
				
				
			}
			sup_head->file_count --;
			if( fd->area != NULL)
				step ++;
			else 
			{
				free(fd);
				fd = NULL;
				step += 2;
				goto erase_a_chip;
			}
		case 3:
			ret = page_free( fd->area, fd->area_total);
			if( ret > RET_SUCCEED)
				return ret;
			free( fd->area);
			step ++;
			free(fd);
			fd = NULL;
		
			
erase_a_chip:			
		case 4:
			step = 0;
			Os_MutexUnlock( FS_Mutex);
			return RET_SUCCEED;
			
		
		
	}
	
	
	err1:
		step = 0;
		Os_MutexUnlock( FS_Mutex);
		return ERROR_T(ERR_FILE_OCCUPY);
	
}

error_t fs_flush( void)
{
	error_t ret;
	static char step = 0;
	os_status_t os_ret;
	//存储区不正确就不处理直接返回
	if( Flash_err_flag )
		return RET_SUCCEED;
	switch( step)
	{
		case 0:
			if( Buf_chg_flag == 0)
			{
				
				return RET_SUCCEED;
				
			}
			else 
				step ++;
		case 1:
			os_ret = Os_MutexLock( FS_Mutex, 0);
			if( os_ret != kStatus_OS_Success)
				return ERROR_T(ERR_FILESYS_BUSY);
			step ++;
		case 2:
			ret = flush_flash( Src_sector);
			if( ret == RET_SUCCEED)
			{
				Buf_chg_flag = 0;
				step = 0;
				Os_MutexUnlock( FS_Mutex);
				return RET_SUCCEED;
				
			}
			else
				return ret;
		
		
	}
	
//	if( Buf_chg_flag)
//	{
//		
//		
//		
//	}
//	else 
//	{
//		
//		
//		
//		Os_MutexUnlock( FS_Mutex);
//		return RET_SUCCEED;
//		
//	}
	
}



int filesys_dev_check(void)
{
	return w25q_TransferStatus();
	
}

static int modify_file_area( file_Descriptor_t *fd, area_t *area)
{
	static char step = 0;
	file_info_t	*file_in_storage;
	storage_area_t	*src_area,*dest_area;
	sup_sector_head_t	*sup_head;
	error_t ret;
	area_t		*tmp_area;
	short i,j;
	
	switch(step)
	{
		case 0:
			//文件描述符中的存储区间尾部加入新的存储区间
			if( fd->area[ fd->area_total -1].start_pg + fd->area[ fd->area_total -1].pg_number == area->start_pg)  //新增加的内存是连续的
			{
				 fd->area[ fd->area_total -1].pg_number += area->pg_number;
				
			}
			else 
			{
				fd->area_total ++;
				tmp_area = fd->area;
				fd->area = realloc( fd->area, file_in_storage->area_total * sizeof( area_t));
				if( fd->area == NULL)		//重新分配内存失败
				{
					fd->area = tmp_area;
					return ERROR_T(ERROR_MEM_UNAVAILABLE);
				}
				fd->area[ fd->area_total -1].start_pg = area->start_pg;
				fd->area[ fd->area_total -1].pg_number = area->pg_number;
			}
			step ++;
		case 1:
			ret = read_flash( FILE_INFO_SECTOR);
			if( ret == RET_SUCCEED)
				step ++;
			else
				return ret; 
		case 2:
			sup_head = (sup_sector_head_t *)Flash_buf;
			file_in_storage = ( file_info_t *)( Flash_buf + sizeof(sup_sector_head_t));
			src_area	= ( storage_area_t *)( Flash_buf + sizeof(sup_sector_head_t) + FILE_NUMBER_MAX * sizeof(file_info_t));
			for( i = 0 ; i < FILE_NUMBER_MAX; i++)
			{
				if( strcmp(file_in_storage->name, fd->name) == 0x00 )			
				{
					//如果新增的地址与上一个地址区间是连续的，那么在上一步就不会新增一个存储区间
					//所以这里通过存储区间数量来判断地址是不是连续的。
					if( file_in_storage->area_total < fd->area_total)   
					{						
						
						//地址不连续的话就新分配一个存放区间的内存
						for( j = 0; j < STOREAGE_AREA_NUMBER_MAX; j ++)
						{
							if( src_area[j].file_id == 0xff)		//未被使用
							{
								src_area[j].file_id = file_in_storage->file_id;
								src_area[j].seq = file_in_storage->area_total;
								file_in_storage->area_total ++;
								src_area[j].area.start_pg = area->start_pg;
								src_area[j].area.pg_number = area->pg_number;
								Buf_chg_flag = 1;
								break;
							}
						}
					}
					else 
					{
						//连续的地址只需要在前面的基础上增加数量就行了
						for( j = 0; j < STOREAGE_AREA_NUMBER_MAX; j ++)
						{
							if( src_area[j].file_id == file_in_storage->file_id)	
							{
								if( src_area[j].area.pg_number + src_area[j].area.start_pg ==area->start_pg)
								{
									src_area[j].area.pg_number += area->pg_number;
									Buf_chg_flag = 1;
								}
								
							}
							
						}
					}
					
					break;
				}
				
				file_in_storage ++;
			}
			
			step ++;
		
		
		

		
		case 3:
//			ret = w25q_Erase_Sector(FILE_INFO_SECTOR);
//			if( ret == RET_SUCCEED )
//			{
//				step += 1;
//				
//				
//			}
//			else
//				return ret;
//		
//		case 4:
//			ret = w25q_Write_Sector_Data( Flash_buf, FILE_INFO_SECTOR);
//			
//			if( ret == RET_SUCCEED )
//			{
//					step += 1;			
//			}
//			else if( ret < 0)
//			{
//				return RET_DELAY;
//				
//			}
//			else
//				return ret;	
//		case 5:
			step = 0;
			return RET_SUCCEED;
		
	}
	
	
}


static error_t read_flash( uint16_t sector)
{
	static char step = 0;
	error_t ret = 0;
	if( Src_sector == sector )		
	{
		//本次读取的扇区与上次读取的一致时候，可以不必再去读取
		//如果缓存修改标志置起，说明缓存中的内容比flash中的内容更新
		
		step = 0;
		return RET_SUCCEED;
		
		
	}
	
	//读取另外一个扇区，则根据缓存修改标志来决定是否将缓存内容写日flash
	switch( step)
	{
		case 0:
			if( Buf_chg_flag)
			{
				ret = flush_flash( Src_sector);
				if( ret == RET_SUCCEED)
				{
					Src_sector = INVALID_SECTOR;
					step ++;
				}
				else
					return ret;
				
			}
			else
				step ++;
		case 1:
			ret = flash_read_sector(Flash_buf,sector);
			if( ret == RET_SUCCEED)
			{
				Buf_chg_flag = 0;
				Src_sector = sector;
				step = 0;
				return RET_SUCCEED;
			}
			else
				return RET_PROCESSING;
		
	}
	
	
	
}

static error_t flush_flash( uint16_t sector)
{
	static char step = 0;
	error_t ret = 0;
	switch( step)
	{
		case 0:
			ret = flash_erase_sector(sector);
			if( ret == RET_SUCCEED )
			{
				step += 1;
				
				
			}
//			else if( ret < 0)
//			{
//				return RET_DELAY;
//				
//			}
			else
				return ret;
		case 1:
			ret = flash_write_sector( Flash_buf, sector);
			
			if( ret == RET_SUCCEED )
			{
					step = 0;			
					
					return RET_SUCCEED;
			}
//			else if( ret < 0)
//			{
//				return RET_DELAY;
//				
//			}
			else
				return ret;	
		
		
	}
	
	
	
}

static int page_malloc( area_t *area, int len)
{
	static char step = 0;
	static uint32_t i = 0;
	uint16_t j = 0, page_num,k;
	error_t ret = 0;
	
	
	switch( step)
	{
		case 0:
			Ptr_Flash_buf = Flash_buf;
			i = 0;
			MEM_sector = FLASH_USEE_SECTOR1;
			step += 1;
		case 1:
			ret = read_flash(MEM_sector);
			if( ret == RET_SUCCEED)
				step += 1;
			else
				return RET_PROCESSING;	
		case 2:
			//先查看flash是否具有可被分配的内存页
			//扇区0 1 2用于维护文件系统，不能作为普通内存使用，所以要跳过
			i = get_available_page( Ptr_Flash_buf, PAGE_AVAILBALE_OFFSET, ( SECTOR_SIZE * 8 - 1), len);
			if( i != FLASH_NULL_FLAG)	
			{
				step += 3;
				goto update_page_used;
				
			}
			else {
				//查询下半个内存中是否有可用页
				MEM_sector = FLASH_USEE_SECTOR2;
				
				step += 1;
				
				
			}
		case 3:
			
			ret = read_flash(MEM_sector);
			if( ret == RET_SUCCEED)
				step += 1;
			else
				return RET_PROCESSING;
		case 4:
			i = get_available_page( Ptr_Flash_buf, 0, ( SECTOR_SIZE * 8 - PAGE_REMIN_FOR_IMG), len);
			if( i != FLASH_NULL_FLAG)				//
			{
				i += SECTOR_SIZE * 8;
				step += 1;
				goto update_page_used;
				
			}
			else {
				
				step = 0;
				return ERROR_T(ERR_NO_FLASH_SPACE);
				
			}
		
update_page_used:
		//擦除是为了把0写成1，在标记已经被分配掉的页时是要把对应的bit从1->0
		//因此不会出现0->1的场景，为了节省操作就不进行擦除操作了。
		//只需要将被分配掉的页的标志清除即可
		case 5:
			area->start_pg = i;
			area->pg_number = len/PAGE_SIZE + 1;
			i %= SECTOR_SIZE * 8;  //消除扇区2的偏移
			for( j = 0; j <= len/PAGE_SIZE; j ++)
				clear_bit( Ptr_Flash_buf, i + j);
			step ++;
			WR_addr = 0;			//i所在的页
		case 6:
			
			//计算i这个起始位置在一个页中的起始位置
			j = i/8%PAGE_SIZE;
			k = len/PAGE_SIZE/8;  //计算要分配的长度所占用的字节长度
			
			page_num = (i + k) / PAGE_SIZE + 1;
			while(1)
			{
				ret = w25q_Write(Ptr_Flash_buf +( i/8/PAGE_SIZE + WR_addr)* PAGE_SIZE,  MEM_sector*SECTOR_SIZE + ( i/8/PAGE_SIZE + WR_addr) * PAGE_SIZE, PAGE_SIZE);
				if( ret == RET_SUCCEED )
				{
					WR_addr ++;
					if( WR_addr >= page_num) 
					{
						step = 0;	
						return RET_SUCCEED;
						
					}
					
					
					
				}
				else if( ret < 0)
				{
					return RET_DELAY;
					
				}
				else
					return ret;
			}
		
	}
	
	
	
	
}


//把属于同一个扇区的bit一次性一起设置起来，减少读写扇区的次数
static int page_free( area_t *area, int area_num)
{
	static char step = 0, boundary = 0;
	uint16_t i = 0;
	uint16_t j = 0, k = 0;
	error_t ret = 0;
	area_t	tmp_area ;
	
		
	switch( step)
	{
		case 0:
			Ptr_Flash_buf = Flash_buf;
		
		
			//整理存储区间，按所属的扇区来整理
			if( area_num > 1)
			{
				for( i = 0; i < area_num; i ++)
				{
					if( area[i].start_pg + area[i].pg_number > W25Q_flash.page_num)
						return -1;
					
					
					//记录flash内存页分配的页有两页，页1和页2
					//为了方便操作，按照页1，页2的顺序把属于页1内的区间放到前面，页2的区间放到后面
					//注：没有考虑区间跨越页1和页2的情况
					if( area[i].start_pg > SECTOR_PAGE_BIT)			
					{
						tmp_area.pg_number =  area[i].pg_number;
						tmp_area.start_pg =  area[i].start_pg;
						for( j = i; j < area_num; j ++)
						{
							if( area[j].start_pg < SECTOR_PAGE_BIT)
							{
								area[i].pg_number = area[j].pg_number;
								area[i].start_pg = area[j].start_pg;
								area[j].pg_number = tmp_area.pg_number;
								area[j].start_pg = tmp_area.start_pg;
								boundary = i;
								break;
							}
							
						}
					}
				}
				
				if( boundary == 0)			//全部的存储区间都是在扇区2中
					MEM_sector = FLASH_USEE_SECTOR2;
				else
					MEM_sector = FLASH_USEE_SECTOR1;
				
			}
			else 
			{
				if( area->start_pg + area->pg_number > W25Q_flash.page_num)
						return -1;
				
				if( area->start_pg > SECTOR_PAGE_BIT)
					MEM_sector = FLASH_USEE_SECTOR2;
				else
					MEM_sector = FLASH_USEE_SECTOR1;
			}
			
			step ++;
		case 1:
clear_bit_map:
			ret = read_flash(MEM_sector);
			if( ret == RET_SUCCEED)
				step += 1;
			else
				return ret;	
		case 2:
			if( MEM_sector == FLASH_USEE_SECTOR1)
			{
				for( k = 0; k <= boundary; k ++)
				{
					i = area[k].start_pg;
					for( j = 0; j < area[k].pg_number; j ++)
						set_bit( Ptr_Flash_buf, i + j);
					
					
				}
				
			}
			else 
			{
				for( k = boundary  + 1; k < area_num; k ++)
				{
					i = area[k].start_pg % SECTOR_PAGE_BIT;
					for( j = 0; j < area[k].pg_number; j ++)
						set_bit( Ptr_Flash_buf, i + j);
					
					
				}
				
			}
			Buf_chg_flag = 1;
			
			
			step ++;
		case 3:		
			if( MEM_sector == FLASH_USEE_SECTOR1 && boundary > 0 && boundary < ( area_num - 1))
			{
				step = 1;
				MEM_sector = FLASH_USEE_SECTOR2;
				goto clear_bit_map;
			}
			step = 0;	
			return RET_SUCCEED;
	}
	
	
	
	
}



static bool check_bit(uint8_t *data, int bit)
{
	int i, j ;
	i = bit/8;
	j = bit % 8;
	return ( data[i] & ( 1 << j));
	
	
}
static void clear_bit(uint8_t *data, int bit)
{
	int i, j ;
	i = bit/8;
	j = bit % 8;
	data[i] &= ~( 1 << j);
	
	
	
	
}

static void set_bit(uint8_t *data, int bit)
{
	int i, j ;
	i = bit/8;
	j = bit % 8;
	data[i] |=  1 << j;
	
	
	
	
}

static int get_available_page( uint8_t *data, int start, int end, int len)
{
	short i, j ;
	int pages = len / PAGE_SIZE + 1;
	
	for( i = start; i < end; i++)
	{
		
		if( check_bit( data, i)) 		
		{
			for( j = 0; j < pages; j ++)	//检查长度是否达标
			{
				if( !check_bit( data, i+j)) 
					break;
				
			}
			if( j == pages)
				return i;
			
		}
		
	}
	
	return FLASH_NULL_FLAG;
	
	
}














