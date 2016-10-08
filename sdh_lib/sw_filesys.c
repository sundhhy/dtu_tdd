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
#include "list.h"
#include "sw_filesys.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "stdbool.h"
#include "sdhError.h"


static List L_File_opened;
uint8_t	Flash_buf[SECTOR_SIZE];
uint8_t	*Ptr_Flash_buf;		//用于读取扇区1或2的内存使用情况


static	uint8_t		Buf_chg_flag = 0;				//缓存内容被修改的标志,缓存被写入flash时标志清零
static uint16_t	Src_sector = INVALID_SECTOR;		//缓存中的内容的扇区
static char Flash_err_flag = 0;

static bool check_bit(uint8_t *data, int bit);
static void clear_bit(uint8_t *data, int bit);
static void set_bit(uint8_t *data, int bit);
static int get_available_page( uint8_t *data, int start, int end, int len);
static int page_malloc( area_t *area, int len);
static int page_free( area_t *area, int area_num);
static int read_flash( uint16_t sector);
static int flush_flash( uint16_t sector);

static int mach_file(const void *key, const void *data)
{
	char *name = ( char *)key;
	file_Descriptor_t	*fd = (file_Descriptor_t *)data;
	return strcmp( name, fd->name);
	
	
	
}

int filesys_init(void)
{
	
	if( STORAGE_INIT != ERR_OK)
	{
		
		Flash_err_flag = 1;
		return ERR_FLASH_UNAVAILABLE;
		
	}
	else
		Flash_err_flag = 0;
	SYS_ARCH_INIT;
	
	list_init( &L_File_opened, free, mach_file);
	return ERR_OK;
	
}

int filesys_close(void)
{
	
	STORAGE_CLOSE;
	
	return ERR_OK;
	
}

//从文件记录扇区0中找到指定名字的文件记录信息
int fs_open(char *name, file_Descriptor_t **fd)
{
	int ret = 0;
	file_info_t	*file_in_storage;
	file_Descriptor_t *pfd;
	storage_area_t	*src_area;
	area_t					*dest_area;	
	sup_sector_head_t	*sup_head;
	ListElmt			*ele;
	short 				i,j, k;
	short step = 0;
	if( Flash_err_flag )
		return (ERR_FLASH_UNAVAILABLE);
	while(1)
	{
		switch( step)
		{
			case 0:
				SYS_ARCH_PROTECT;
				ele = list_get_elmt( &L_File_opened,name);		//先从已经打开的文件中查找是否已经被其他任务打开过
				if(  ele!= NULL)
				{
					pfd = list_data(ele);
					pfd->reference_count ++;
					*fd = pfd;
					pfd->rd_pstn[SYS_GETTID] = 0;
					pfd->wr_pstn[SYS_GETTID] = 0;
					SYS_ARCH_UNPROTECT;
					return ERR_OK;
					
				}
				step ++;
				break;
			case 1:
				ret = read_flash(FILE_INFO_SECTOR);
				if( ret == ERR_OK)
					step ++;
				else
					return ERR_DRI_OPTFAIL;
			case 2:
				sup_head = ( sup_sector_head_t *)Flash_buf;
				if( sup_head->ver[0] != 'V')			//第一次上电，擦除整块flash并写入头信息
				{
					SYS_ARCH_UNPROTECT;
					return (ERR_FILESYS_ERROR);
					
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
									pfd->reference_count = 1;
									strcpy( pfd->name, name);
									pfd->area = NULL;
									*fd = pfd;
								
									step = 0;
									SYS_ARCH_UNPROTECT;
									return (ERR_FILE_ERROR);
								
								}
							
							}
							src_area ++;						
						}				
						pfd->area_total = j ;			//把实际找到的区间数量赋值给文件描述符
						file_in_storage->area_total = j;			//更新实际的区间数量
						strcpy( pfd->name, name);
						pfd->reference_count = 1;
						memset( pfd->rd_pstn, 0, sizeof( pfd->rd_pstn));
						memset( pfd->wr_pstn, 0, sizeof( pfd->wr_pstn));
						*fd = pfd;
						step = 0;
						list_ins_next( &L_File_opened, L_File_opened.tail, pfd);
						SYS_ARCH_UNPROTECT;
						return ERR_OK;
					}
					file_in_storage ++;
				}		//for
				*fd = NULL;
				SYS_ARCH_UNPROTECT;
				return (ERR_OPEN_FILE_FAIL);
		}		//switch
		
	}		//while(1)

}

//格式化文件系统，删除所有的内容（保留区除外）
int fs_format(void)
{
	sup_sector_head_t	*sup_head;
	int ret;
	int wr_addr = 0;
	
	//擦除整个文件系统
	while(1)
	{
		ret = w25q_Erase_block(wr_addr);
		if( ret == ERR_OK)
		{
			if( 	wr_addr < W25Q_flash.block_num - PAGE_REMIN_FOR_IMG/BLOCK_HAS_SECTORS/SECTOR_HAS_PAGES )
			{
				wr_addr ++;
				
				
			}
			else
				break;
			
		}
		else
			return ERR_DRI_OPTFAIL;
		
		
	}
	//读取扇区FILE_INFO_SECTOR
	//因为刚擦除过，所有的flash内容都是0xff，也就不必去真的读取了
	memset( Flash_buf, 0xff, SECTOR_SIZE);		
	//设置上次读取的扇区为FILE_INFO_SECTOR
	Src_sector = FILE_INFO_SECTOR;
	
	sup_head = ( sup_sector_head_t *)Flash_buf;
	
	sup_head->file_count = 0;
	memcpy( sup_head->ver, FILESYS_VER, 6);
	Buf_chg_flag = 1;
	
	return ERR_OK;
	
}


//文件的存储信息结构体都是4字节对齐的，所以不必考虑字节对齐问题
//前置条件:在创建文件的时候,特殊块是不能够存在空洞的,所以在删除操作的时候,要去整理特殊扇区中的内存
//长度是可选参数
int fs_creator(char *name, file_Descriptor_t **fd, int len)
{	
	int i = 0;
	int ret = 0;
	area_t		*tmp_area;
	sup_sector_head_t	*sup_head;
	file_info_t	*file_in_storage;
	file_info_t	*creator_file;
	storage_area_t	*target_area;
	file_Descriptor_t *p_fd;
	
	//存储区不正确就不处理直接返回
	if( Flash_err_flag )
		return ERR_FLASH_UNAVAILABLE;
	if (len == 0)
		len = 1;
	tmp_area = malloc( sizeof( storage_area_t));
	if( tmp_area == NULL)
	{
		ret = ERR_MEM_UNAVAILABLE;
		goto err1;
	}
	ret = page_malloc( tmp_area, len);
	if( ret < 0) {
		
		ret =  ERR_NO_FLASH_SPACE;
		goto err2;
	}
	
	ret = read_flash(FILE_INFO_SECTOR);
	if( ret != ERR_OK)
	{
		goto err2;
	}
	
	sup_head = (sup_sector_head_t *)Flash_buf;
	creator_file = ( file_info_t *)( Flash_buf+ sizeof( sup_sector_head_t));

	if( sup_head->file_count > FILE_NUMBER_MAX)
	{
		ret =  ERR_FILESYS_OVER_FILENUM;
		goto err2;
	}
	
	//检查文件是否已经存在
	file_in_storage = ( file_info_t *)( Flash_buf + sizeof(sup_sector_head_t));
	for( i = 0; i < FILE_NUMBER_MAX; i ++)
	{
		
		if( strcmp(file_in_storage->name, name) == 0x00 )	
		{	
			ret =  ERR_FILESYS_FILE_EXIST;
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
	{
		ret = ERR_NO_SUPSECTOR_SPACE;
		goto err2;	
	}
	target_area = ( storage_area_t *)( Flash_buf+ sizeof( sup_sector_head_t) + FILE_NUMBER_MAX * sizeof(file_info_t));
		
	for( i = 0; i < STOREAGE_AREA_NUMBER_MAX; i ++)
	{
		if( target_area[i].file_id == 0xff)
		{
			target_area[i].file_id = sup_head->file_count;
			target_area[i].seq = 0;
			target_area[i].area.start_pg = tmp_area->start_pg;
			target_area[i].area.pg_number = tmp_area->pg_number;
			break;
		}
		
		
	}
	if( i == STOREAGE_AREA_NUMBER_MAX)
	{
		ret = ERR_NO_SUPSECTOR_SPACE;
		goto err2;	
	}
			
	strcpy( creator_file->name, name);
	creator_file->file_id = sup_head->file_count;
	creator_file->area_total = 1;
	sup_head->file_count ++;
		
	Buf_chg_flag = 1;
		
	p_fd = malloc( sizeof( file_Descriptor_t));
	if( p_fd == NULL) {
		ret = ERR_MEM_UNAVAILABLE;
		goto err2;	
	}
		
	strcpy( p_fd->name, name);
	memset( p_fd->rd_pstn, 0 , sizeof( p_fd->rd_pstn));
	memset( p_fd->wr_pstn, 0 , sizeof( p_fd->wr_pstn));
	p_fd->wr_size = 0;
	p_fd->reference_count = 1;
	p_fd->area = malloc( sizeof( area_t));
		
	//装载存储区的地址区间到文件描述符中的存储区间链表中去
	p_fd->area->start_pg = tmp_area->start_pg;
	p_fd->area->pg_number = tmp_area->pg_number;
	p_fd->area_total = 1;
	*fd = p_fd;			
	list_ins_next( &L_File_opened, L_File_opened.tail, p_fd);
	free( tmp_area);
	return ERR_OK;
	

err2:
	page_free( tmp_area, 1);
err1:
	free( tmp_area);
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
int fs_write( file_Descriptor_t *fd, uint8_t *data, int len)
{
	
	
	int 			ret;
	int 				i, limit;
	int  				myid = SYS_GETTID;
	area_t		*wr_area;
	uint16_t	wr_page = 0, wr_sector = 0;

	while( 1)
	{
		
		wr_area = locate_page( fd, fd->wr_pstn[myid], &wr_page);
		if( wr_area == NULL)
		{
			return (ERR_FILE_FULL);
		}
		wr_sector = 	wr_page/SECTOR_HAS_PAGES;
		
		if( wr_sector < SUP_SECTOR_NUM)
		{
			return (ERR_FILE_ERROR);
			
		}
		ret = read_flash(wr_sector);
		if( ret != ERR_OK)
			return ret;
		
	
		i = ( wr_page % SECTOR_HAS_PAGES)*PAGE_SIZE + fd->wr_pstn[myid] % PAGE_SIZE;
		if( wr_area->pg_number + wr_area->start_pg > ( wr_sector + 1 ) *  SECTOR_HAS_PAGES )				//文件的结束位置超出本扇区，以扇区的大小为上限
				limit  = SECTOR_SIZE;
		else		//文件的结束位置在本扇区内部，以文件结束位置在本扇区中的相对位置为上限
				limit = ( wr_area->start_pg +  wr_area->pg_number - wr_sector *  SECTOR_HAS_PAGES) * PAGE_SIZE;
		while( len)
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
			len --;
			data++;
			
		}
		fd->wr_size =  fd->wr_pstn[myid];
		if( len == 0)			
		{
			break;
		}
	}
	
	return ERR_OK;
	
}

int fs_read( file_Descriptor_t *fd, uint8_t *data, int len)
{
	
	int 			ret;
	int 				i, limit;
	int  				myid = SYS_GETTID;
	area_t			*rd_area;
	uint16_t	rd_page = 0, rd_sector = 0;

	
	while(1)
	{
		rd_area = locate_page( fd, fd->rd_pstn[myid], &rd_page);
		if( rd_area == NULL)
		{
			return (ERR_FILE_EMPTY);
		}
		
		rd_sector = 	rd_page/SECTOR_HAS_PAGES;
		ret = read_flash(rd_sector);
		if( ret != ERR_OK)
			return ret;
		
		//读写的时候，要考虑文件的结尾不在本扇区的情况
		i = ( rd_page % SECTOR_HAS_PAGES)*PAGE_SIZE + fd->rd_pstn[myid] % PAGE_SIZE;
			
		if( rd_area->start_pg +  rd_area->pg_number > ( rd_sector + 1) *  SECTOR_HAS_PAGES)	//结尾位于本扇区外
			limit = SECTOR_SIZE;
		else	//结尾位于本扇区内
			limit = ( rd_area->start_pg +  rd_area->pg_number - rd_sector *  SECTOR_HAS_PAGES) * PAGE_SIZE;
			
		while( len)
		{
			if( i >= SECTOR_SIZE || i > limit)		 //超出了当前的扇区或者区间范围
			{
				break;
			}
			*data++ = Flash_buf[i];
			i ++;
			fd->rd_pstn[myid] ++;
			len --;
				
		}
			
		if( len == 0)			//还有数据没读完
		{
			
			break;
			
		}
			
	}
	
	return ERR_OK;
		
	
}

int fs_lseek( file_Descriptor_t *fd, int offset, int whence)
{
	char myid = SYS_GETTID;

	switch( whence)
	{
		case WR_SEEK_SET:
			fd->wr_pstn[ myid] = offset;
			break;
		case WR_SEEK_CUR:
			fd->wr_pstn[ myid] += offset;
			break;
		
		
		case WR_SEEK_END:			//连续5个0xff作为结尾

		
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
	
	return ERR_OK;
	
	
}

int fs_close( file_Descriptor_t *fd)
{
	char myid = SYS_GETTID;
	void 							*data = NULL;
	ListElmt           *elmt;
	int i, ret;
	file_info_t	*file_in_storage;

	fd->reference_count --;			
	fd->rd_pstn[ myid] = 0;
	fd->wr_pstn[ myid] = 0;
	if( fd->reference_count > 0)
		return ERR_OK;
	ret = read_flash( FILE_INFO_SECTOR);
	if( ret != ERR_OK)
		return ret;
	
	file_in_storage = ( file_info_t *)( Flash_buf + sizeof(sup_sector_head_t));
	for( i = 0; i < FILE_NUMBER_MAX; i ++)
	{
		if( strcmp(file_in_storage->name, fd->name) == 0x00 )
		{
			Buf_chg_flag = 1;
			break;
		}
		file_in_storage ++;	
	}
	elmt = list_get_elmt( &L_File_opened,fd->name);
	list_rem_next( &L_File_opened, elmt, &data);
	free( fd->area);
	
	free(fd);
	return ERR_OK;
}


int fs_delete( file_Descriptor_t *fd)
{
	int ret = 0;
	file_info_t	*file_in_storage;
	storage_area_t	*src_area;
	sup_sector_head_t	*sup_head;
	short i, j, k;

	
	fd->reference_count --;
	if( fd->reference_count > 0)
		return ERR_FILE_OCCUPY;
	ret = read_flash(FILE_INFO_SECTOR);
	if( ret != ERR_OK)
		return ret;
	sup_head = ( sup_sector_head_t *)Flash_buf;
	file_in_storage = ( file_info_t *)( Flash_buf + sizeof(sup_sector_head_t));
	k = 0;
	
	//存储器的管理区找到该文件的管理内容然后删除掉
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
	{
		page_free( fd->area, fd->area_total);
		free( fd->area);
	}
	
	free(fd);
	fd = NULL;
			
	return ERR_OK;
	
}

int fs_flush( void)
{
	int ret;
	//存储区不正确就不处理直接返回
	if( Flash_err_flag )
		return ERR_FLASH_UNAVAILABLE;

		
	if( Buf_chg_flag == 0)
		return ERR_OK;
	ret = flush_flash( Src_sector);
	if( ret == ERR_OK)
	{
		Buf_chg_flag = 0;
		
		return ERR_OK;
		
	}
	return ret;
		
	
	
}





static int read_flash( uint16_t sector)
{
	int ret = 0;
	if( Src_sector == sector )		
	{
		//本次读取的扇区与上次读取的一致时候，可以不必再去读取
		//如果缓存修改标志置起，说明缓存中的内容比flash中的内容更新
		
		return ERR_OK;
	}
	
	//读取另外一个扇区，则根据缓存修改标志来决定是否将缓存内容写日flash

	if( Buf_chg_flag)
	{
		ret = flush_flash( Src_sector);
		if( ret != ERR_OK)
		{
			return ret;
			
		}
//		Src_sector = INVALID_SECTOR;
	}
	SYS_ARCH_PROTECT;
	ret = flash_read_sector(Flash_buf,sector);
	SYS_ARCH_UNPROTECT;
	if( ret == ERR_OK)
	{
		Buf_chg_flag = 0;
		Src_sector = sector;
		return ERR_OK;
	}
		
	return ret;

}

static int flush_flash( uint16_t sector)
{
	int ret = 0;
	
	SYS_ARCH_PROTECT;
	ret = flash_erase_sector(sector);
	
	if( ret != ERR_OK )
	{
		SYS_ARCH_UNPROTECT;
		return ret;
	}

	ret =  flash_write_sector( Flash_buf, sector);
	SYS_ARCH_UNPROTECT;
	return ret;
}

static int page_malloc( area_t *area, int len)
{
	static uint32_t i = 0;
	uint16_t 	j = 0, page_num,k;
	uint16_t	mem_manger_sector = 0;
	short		wr_addr = 0;
	int ret = 0;
	
	
	

	Ptr_Flash_buf = Flash_buf;
	i = 0;
	mem_manger_sector = FLASH_USEE_SECTOR1;
	while(1)
	{
		
		ret = read_flash(mem_manger_sector);
		if( ret != ERR_OK)
			return ret;
		//先查看flash是否具有可被分配的内存页
		//扇区0 1 2用于维护文件系统，不能作为普通内存使用，所以要跳过
		i = get_available_page( Ptr_Flash_buf, PAGE_AVAILBALE_OFFSET, ( SECTOR_SIZE * 8 - 1), len);
		if( i != FLASH_NULL_FLAG)	
		{
			if( mem_manger_sector == FLASH_USEE_SECTOR2)
				i += SECTOR_SIZE * 8;
			break;
		}
		//在第二个内存管理扇区也找不到可用内存页，说明内存被用尽
		if( mem_manger_sector == FLASH_USEE_SECTOR2)
			return ERR_NO_FLASH_SPACE;
		
		mem_manger_sector = FLASH_USEE_SECTOR2;
		
	}
			
		
	//擦除是为了把0写成1，在标记已经被分配掉的页时是要把对应的bit从1->0
	//因此不会出现0->1的场景，为了节省操作就不进行擦除操作了。
	//只需要将被分配掉的页的标志清除即可
	
	area->start_pg = i;
	area->pg_number = len/PAGE_SIZE + 1;
	i %= SECTOR_SIZE * 8;  //消除扇区2的偏移
	for( j = 0; j <= len/PAGE_SIZE; j ++)
		clear_bit( Ptr_Flash_buf, i + j);
	
	//计算i这个起始位置在一个页中的起始位置
	j = i/8%PAGE_SIZE;
	k = len/PAGE_SIZE/8;  //计算要分配的长度所占用的字节长度
	
	page_num = (i + k) / PAGE_SIZE + 1;
	while(1)
	{
		ret = flash_write(Ptr_Flash_buf + \
		( i/8/PAGE_SIZE + wr_addr)* PAGE_SIZE,  mem_manger_sector*SECTOR_SIZE + ( i/8/PAGE_SIZE + wr_addr) * PAGE_SIZE, PAGE_SIZE);
		if( ret == ERR_OK )
		{
			wr_addr ++;
			if( wr_addr >= page_num) 
			{
				return wr_addr;
				
			}
			
			
			
		}
		else
			return ret;
	}
}


//把属于同一个扇区的bit一次性一起设置起来，减少读写扇区的次数
static int page_free( area_t *area, int area_num)
{
	char  boundary = 0;
	uint16_t i = 0;
	uint16_t j = 0, k = 0;
	uint16_t	mem_manger_sector = 0;

	int ret = 0;
	area_t	tmp_area ;
	
		

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
			mem_manger_sector = FLASH_USEE_SECTOR2;
		else
			mem_manger_sector = FLASH_USEE_SECTOR1;
		
	}
	else 
	{
		if( area->start_pg + area->pg_number > W25Q_flash.page_num)
				return -1;
		
		if( area->start_pg > SECTOR_PAGE_BIT)
			mem_manger_sector = FLASH_USEE_SECTOR2;
		else
			mem_manger_sector = FLASH_USEE_SECTOR1;
	}
			
	while(1)
	{
		ret = read_flash(mem_manger_sector);
		if( ret != ERR_OK)
			return ret;	
		
		if( mem_manger_sector == FLASH_USEE_SECTOR1)
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
		
		if( mem_manger_sector == FLASH_USEE_SECTOR1 && boundary > 0 && boundary < ( area_num - 1))
		{
			mem_manger_sector = FLASH_USEE_SECTOR2;
		}
		else
		{
			
			return ERR_OK;
		}
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














