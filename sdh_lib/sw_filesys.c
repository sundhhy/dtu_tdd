/**
* @file 		sdh_filesys.c
* @brief		Ò»¸öÊÊÓÃÓëÂã»ú»îrtosµÄ¼òÒ×ÎÄ¼şÏµÍ³£¬ÎªÁË·½±ã´æ´¢Æ÷µÄ¹ÜÀí.
* @details		´æ´¢Æ÷µÄ±»·Ö³ÉÈı²¿·Ö£º ¹ÜÀíÇø£¬´æ´¢Çø£¬±£ÁôÇø.¹ÜÀíÇø±£´æÎÄ¼şÃèÊö½á¹¹£¬ÒÔ¼°ÄÚ´æÒ³µÄÊ¹ÓÃÇé¿ö¡£´æ´¢Çø´æ´¢Êı¾İ£¬±£ÁôÇøÊÇÎÄ¼şÏµÍ³²»»áÈ¥·ÃÎÊµÄ²¿·Ö¡£
*	²»Ö§³ÖÄ¿Â¼½á¹¹¡£ËùÓĞ´æ·ÅÔÚflashÖĞµÄÎÄ¼ş¶¼ÊÇÆ½¼¶µÄ¡£
*	ÉÈÇø0±£´æÎÄ¼şĞÅÏ¢¡£³ÆÎªÌØÊâÉÈÇø
*	½ô°¤×ÅµÄÉÈÇø1£¬»òÕß¸ü¶àµÄÉÈÇøÓÃÀ´¹ÜÀí´æ´¢Æ÷µÄÄÚ´æÒ³µÄÊ¹ÓÃ¡£Ò»¸öBit´ú±íÒ»¸öÒ³£¬1±íÊ¾¸ÃÒ³¿ÉÒÔ±»Ê¹ÓÃ£¬0±íÊ¾¸ÃÒ³ÒÑ¾­±»·ÖÅäµôÁË¡£
*	ÉÈÇø0µÄÎÄ¼şĞÅÏ¢ÓÉ3²¿·Ö×é³É£º
*	1¡¢ÉÈÇøÍ·ĞÅÏ¢£ºÒÑ¾­´´½¨µÄÎÄ¼şÊıÁ¿¼°°æ±¾ºÅ
*	2¡¢ÎÄ¼şĞÅÏ¢´æ´¢Çø
*	3¡¢ÎÄ¼ş´æ´¢Çø¼ä´æ´¢Çø
*	Ìá¹©»º´æ²Ù×÷»úÖÆ£¬½µµÍ´æ´¢Æ÷µÄ²Ù×÷´ÎÊı.
*	ÏûºÄ×ÊÔ´£º
*	µôµç´æ´¢Æ÷£º2 + n¸öÉÈÇø,Èç¹ûÄÚ´æÒ³¹ı¶àÎŞ·¨ÓÃÒ»¸öÉÈÇøÀ´±£´æ¾ÍÔõ½²ÄÚ´æ¹ÜÀíÉÈÇø
*	ÄÚ´æ£º		1¸öÉÈÇøµÄ´óĞ¡
* @author		author
* @date		date
* @version	A001
* @par Copyright (c): 
* 		XXX¹«Ë¾
* @par History:         
*	version: author, date, desc

*/

#include "list.h"
#include "sw_filesys.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "stdbool.h"
#include "sdhError.h"
#include "debug.h"


static List L_File_opened;
static uint8_t	*Flash_buf;
static storageInfo_t	StrgInfo;
static fs_area			Page_Zone;
static	uint8_t		Buf_chg_flag = 0;				//»º´æÄÚÈİ±»ĞŞ¸ÄµÄ±êÖ¾,»º´æ±»Ğ´ÈëflashÊ±±êÖ¾ÇåÁã
static uint16_t	Src_sector = INVALID_SECTOR;		//»º´æÖĞµÄÄÚÈİµÄÉÈÇø
static char Flash_err_flag = 0;

static	int FsErr = 0;

static bool check_bit(uint8_t *data, int bit);
static void clear_bit(uint8_t *data, int bit);
static void set_bit(uint8_t *data, int bit);
static void get_area( int pg_offset, int size, area_t *out_area);
static int page_malloc( area_t *area, int len);
static int page_free( area_t *area, int area_num);
static int read_sector( uint16_t sector);
static int read_page( uint8_t *rd_buf,  uint16_t page);
static int flush_flash( uint16_t sector);
static file_info_t	*searchfile( uint8_t* flash_data, char *name);
static int mach_file(const void *key, const void *data)
{
	char *name = ( char *)key;
	sdhFile	*fd = (sdhFile *)data;
	return strcmp( name, fd->name);
	
	
	
}

int filesys_init(void)
{
	int		rese_pagenum;
	int		data_pagenum;			///ĞèÒª±»¹ÜÀíµÄÊı¾İÇøµÄÒ³Êı
	int		capacity_pagenum = 0;		///¿ÉÒÔÄÉÈë¹ÜÀíµÄÒ³Êı
	int		pageuseinfo_sector = 1;		///Êı¾İÒ³ÃæÊ¹ÓÃĞÅÏ¢µÄ´æ´¢ÉÈÇø

	if( STORAGE_INIT() != ERR_OK)
	{
		
		Flash_err_flag = 1;
		return ERR_FLASH_UNAVAILABLE;
		
	}
	else
		Flash_err_flag = 0;
	
	SYS_ARCH_INIT();
	STORAGE_INFO( &StrgInfo);
	StrgInfo.sector_size = StrgInfo.sector_pagenum * StrgInfo.page_size;
	StrgInfo.sector_number = StrgInfo.total_pagenum / StrgInfo.sector_pagenum;
	StrgInfo.block_size = StrgInfo.block_pagenum * StrgInfo.page_size;
	StrgInfo.block_number = StrgInfo.total_pagenum / StrgInfo.block_pagenum;
	Flash_buf = malloc( StrgInfo.sector_size);
	if( Flash_buf == NULL)
		return ERR_FLASH_UNAVAILABLE;
		
	//todo : ºó¼Ì¸Ä½ø¿ÉÒÔ¸ù¾İÎÄ¼şµÄ×î´óÊıÁ¿À´µ÷Õû
	Page_Zone.fileinfo_sector_begin = 0;
	Page_Zone.fileinfo_sector_end = 1;
	Page_Zone.pguseinfo_sector_begin = Page_Zone.fileinfo_sector_end;
	
	rese_pagenum = RESE_STOREAGE_SIZE_KB * 1024 / StrgInfo.page_size;
	
	
	///µÚ1¸öÉÈÇøÓÃÀ´´æ´¢ÎÄ¼şĞÅÏ
	data_pagenum = StrgInfo.total_pagenum - rese_pagenum - StrgInfo.sector_pagenum;		
	
	//ÕÒµ½¹ÜÀíÄÚ´æĞèÒª¼¸¸öÉÈÇø
	while(1)
	{
		capacity_pagenum = pageuseinfo_sector * StrgInfo.sector_pagenum * StrgInfo.page_size * 8;	//Ò»¸öÉÈÇøÄÜ¹»¹ÜÀíµÄÒ³ÃæÊıÁ¿		
		data_pagenum -= pageuseinfo_sector * StrgInfo.sector_pagenum;	//							///
		if( capacity_pagenum < data_pagenum)
			pageuseinfo_sector ++;
		else
			break;
	}
	
	Page_Zone.pguseinfo_sector_end =  Page_Zone.pguseinfo_sector_begin + pageuseinfo_sector;
	Page_Zone.data_sector_begin = Page_Zone.pguseinfo_sector_end;
	Page_Zone.data_sector_end = Page_Zone.data_sector_begin + (data_pagenum - rese_pagenum)/StrgInfo.sector_pagenum;
	list_init( &L_File_opened, free, mach_file);
	return ERR_OK;
	
}

int filesys_mount(void)
{
	int ret = 0;
	int suphead_page = Page_Zone.fileinfo_sector_begin * StrgInfo.sector_pagenum;
	sup_sector_head_t	*sup_head;
	
	ret = read_page(Flash_buf, suphead_page);
	if( ret != ERR_OK)
		return ERR_DRI_OPTFAIL;
	
	sup_head = ( sup_sector_head_t *)Flash_buf;
	
	if( strcmp( sup_head->ver, FILESYS_VER) != 0x00 )	//ÎÄ¼şÏµÍ³°æ±¾ºÅ²»Ò»ÖÂ
	{
		printf(" filesys ver = %s \n", sup_head->ver);
		return fs_format();

	}	

	return	ERR_OK;
	
}

int filesys_close(void)
{
	
	STORAGE_CLOSE();
	
	return ERR_OK;
	
}

//´ÓÎÄ¼ş¼ÇÂ¼ÉÈÇø0ÖĞÕÒµ½Ö¸¶¨Ãû×ÖµÄÎÄ¼ş¼ÇÂ¼ĞÅÏ¢
//0. ÉèÖÃºÃÆôÊ¾Ò³ºÍÆğÒ³»º´æÎ»ÖÃÎªÎÄ¼şÏµÍ³¹ÜÀíÄÚ´æµÄÆğÊ¼Î»ÖÃ
//1. ¶ÁÈ¡Ò³ÄÚÈİµ½»º´æ
//2. ´ÓÎÄ¼şÏµÍ³¹ÜÀí»º´æÆğÊ¼Î»ÖÃ¿ªÊ¼²éÕÒÎÄ¼ş,Èç¹ûÕÒµ½¾Í·µ»Ø½á¹û£¬Èç¹ûÎÄ¼şÏµÍ³ĞÅÏ¢²»¶Ô¾ÍÖ±½ÓÍË³öÎÄ¼şÏµÍ³´íÎó.
//3.Èç¹ûÕÒ²»µ½£¬¾Í¶ÁÈ¡Ò³Êı¼Ó1£¬Í¬Ê±Ò³»º´æµÄÎ»ÖÃÒ²Æ«ÒÆÒ»¸öÒ³µÄ³¤¶È
//4.ÖØ¸´²½Öè1£¬2£¬3£¬Ö±µ½ÕÒµ½ÎÄ¼ş»òÕßÎÄ¼şĞÅÏ¢ÉÈÇø0µÄÊı¾İ¶¼±»¶ÁÍêÁË¾Í·µ»ØÕÒ²»µ½ÎÄ¼ş
static sdhFile* rdFilearea_byfileinfo( file_info_t *file_info, int current_page);
sdhFile * fs_open(char *name)
{
	int 				ret = 0;
	file_info_t			*file_in_storage;
	sdhFile 			*pfd;
	sup_sector_head_t	*sup_head;
	ListElmt			*ele;
	short 				j;
	short 				step = 0;
	short 				fileinfo_page = Page_Zone.fileinfo_sector_begin * StrgInfo.sector_pagenum;
	short 				fileinfo_end_page = Page_Zone.fileinfo_sector_end * StrgInfo.sector_pagenum;
	uint8_t 			*p_rdpage_buf = Flash_buf;
	if( Flash_err_flag )
	{
		FsErr = ERR_FLASH_UNAVAILABLE;
		return (NULL);
	}
	Src_sector = INVALID_SECTOR;
	while(1)
	{
		
		switch( step)
		{
			case 0:
				ele = list_get_elmt( &L_File_opened,name);		//ÏÈ´ÓÒÑ¾­´ò¿ªµÄÎÄ¼şÖĞ²éÕÒÊÇ·ñÒÑ¾­±»ÆäËûÈÎÎñ´ò¿ª¹ı
				if(  ele!= NULL)
				{
					pfd = list_data(ele);
					pfd->reference_count ++;
					pfd->rd_pstn[SYS_GETTID()] = 0;
					pfd->wr_pstn[SYS_GETTID()] = 0;
					printf(" list_get_elmt = %p \n",pfd);
					return pfd;
					
				}
				step ++;
				break;
			case 1:
				ret = read_page( p_rdpage_buf, fileinfo_page);
//				printf(" read_page fileinfo_page = %d, p_rdpage_buf = %p \n",fileinfo_page, p_rdpage_buf);

				if( ret == ERR_OK)
					step ++;
				else
				{
					printf(" read_page FIALED %d \n", ret);
					FsErr =  ERR_DRI_OPTFAIL;
					return NULL;
				}
			case 2:
				sup_head = ( sup_sector_head_t *)Flash_buf;
				if( strcmp( sup_head->ver, FILESYS_VER) != 0x00 )	//ÎÄ¼şÏµÍ³°æ±¾ºÅ²»Ò»ÖÂ
				{
					FsErr =  ERR_FILESYS_ERROR;
					printf(" ERR_FILESYS_ERROR \n");
					return NULL;
					
				}	
				//²éÕÒÎÄ¼şÃû
				file_in_storage = searchfile( Flash_buf, name);

				
				if( file_in_storage )	
				{
					//²éÕÒÎÄ¼şÔÚ´æ´¢Æ÷ÖĞµÄ´æ´¢Çø¼ä
					pfd = rdFilearea_byfileinfo( file_in_storage, fileinfo_page);
					if( pfd)
					{
						strcpy( pfd->name, name);
						pfd->reference_count = 1;
						pfd->area_total = j;
						memset( pfd->rd_pstn, 0, sizeof( pfd->rd_pstn));
						memset( pfd->wr_pstn, 0, sizeof( pfd->wr_pstn));
						step = 0;
						list_ins_next( &L_File_opened, L_File_opened.tail, pfd);
						FsErr = ERR_OK;
						return pfd;
						
					}
					else
					{
						FsErr =  ERR_FILE_ERROR;
						printf(" file %s can\'t find sotre area \n", file_in_storage->name);
						return NULL;
					}
				
				}
				else if( fileinfo_page < fileinfo_end_page)
				{
					fileinfo_page ++;
					p_rdpage_buf += StrgInfo.page_size;
					step = 1;
					break;
					
				}
				else
				{
					
					FsErr =  ERR_NON_EXISTENT;
					return NULL;
				}
		}		//switch
		
	}		//while(1)
}
static sdhFile* rdFilearea_byfileinfo( file_info_t *file_info, int current_page)
{
	int 				ret = 0;
	sdhFile 			*pfd;
	storage_area_t		*src_area;
	area_t				*dest_area;	
	short 				j;
	short 				fileinfo_begin_page = Page_Zone.fileinfo_sector_begin * StrgInfo.sector_pagenum;
	short 				fileinfo_end_page = Page_Zone.fileinfo_sector_end * StrgInfo.sector_pagenum;
	short 				rd_page = current_page;
	short 				local = 0;
	short				end = 0;
	uint8_t 			*p_rdpage_buf = Flash_buf + ( rd_page - fileinfo_begin_page) * StrgInfo.page_size;
	

	src_area = ( storage_area_t *)( Flash_buf + sizeof(sup_sector_head_t) + FILE_NUMBER_MAX * sizeof(file_info_t));		
	//¼ì²éÎÄ¼ş´æ´¢Çø¼äĞÅÏ¢ÊÇ·ñÒÑ¾­¶Áµ½»º´æÖĞÁË
	local = sizeof(sup_sector_head_t) + FILE_NUMBER_MAX * sizeof(file_info_t);
	for( j = 0; j < file_info->area_total ; )
	{
		end = ( rd_page - fileinfo_begin_page + 1) * StrgInfo.page_size;
		if( src_area->file_id == file_info->file_id)		
		{
			j ++;
		}
		local += sizeof(file_info_t);
		src_area ++;
		if( local > end)
		{
			rd_page ++;					
			if( rd_page < fileinfo_end_page)
			{
				p_rdpage_buf += StrgInfo.page_size;
				ret = read_page( p_rdpage_buf, rd_page);

				if( ret != ERR_OK)
				{
					printf(" read_page FIALED %d \n", ret);
					FsErr =  ERR_DRI_OPTFAIL;
					return NULL;
				}
				
				
			}
			else
			{
				return NULL;
			}

		}	
		
	}
	//¶ÁÈ¡ÎÄ¼ş´æ´¢Çø
	src_area = ( storage_area_t *)( Flash_buf + sizeof(sup_sector_head_t) + FILE_NUMBER_MAX * sizeof(file_info_t));		
	
	pfd = (sdhFile *)malloc(sizeof( sdhFile));
	dest_area = malloc( file_info->area_total * sizeof( area_t));
	pfd->area = dest_area;
	//°ÑÎÄ¼şµÄ´æ´¢Çø¼ä¸³Öµ¸øÎÄ¼şÃèÊö·û
	for( j = 0; j < file_info->area_total ;)
	{
		if( src_area->file_id == file_info->file_id)		
		{
			
			pfd->area[src_area->seq].start_pg = src_area->area.start_pg;
			pfd->area[src_area->seq].pg_number = src_area->area.pg_number;
			j ++;
		}
		src_area ++;					
	}								
	return 	pfd;
}
int fs_get_error(void)
{
	
	return FsErr;
}
//¸ñÊ½»¯ÎÄ¼şÏµÍ³£¬É¾³ıËùÓĞµÄÄÚÈİ£¨±£ÁôÇø³ıÍâ£©
int fs_format(void)
{
	sup_sector_head_t	*sup_head;
	int ret;
	int erase_start = Page_Zone.fileinfo_sector_begin * StrgInfo.sector_size;
	int	erase_len = ( Page_Zone.pguseinfo_sector_end) * StrgInfo.sector_size;
//	int sector = 0;
	
	//²Á³ıÕû¸öÎÄ¼şÏµÍ³
//	while(1)
//	{
//		ret = flash_erase_sector(sector);
//		if( ret == ERR_OK)
//		{
//			if( 	sector < Page_Zone.pguseinfo_sector_end )
//			{
//				sector ++;
//				
//				
//			}
//			else
//				break;
//			
//		}
//		else
//			return ERR_DRI_OPTFAIL;
//	}

	//²Á³ıÎÄ¼ş¹ÜÀíÇøºÍÄÚ´æÒ³Ê¹ÓÃĞÅÏ¢ÉÈÇø
	ret = flash_erase( erase_start, erase_len);
	if( ret != ERR_OK)
		return ERR_DRI_OPTFAIL;
	//¶ÁÈ¡ÉÈÇøPage_Zone.fileinfo_sector_begin
	//ÒòÎª¸Õ²Á³ı¹ı£¬ËùÓĞµÄflashÄÚÈİ¶¼ÊÇ0xff£¬Ò²¾Í²»±ØÈ¥ÕæµÄ¶ÁÈ¡ÁË
	memset( Flash_buf, 0xff, SECTOR_SIZE);		
	//ÉèÖÃÉÏ´Î¶ÁÈ¡µÄÉÈÇøÎªPage_Zone.fileinfo_sector_begin
	Src_sector = Page_Zone.fileinfo_sector_begin;
	
	sup_head = ( sup_sector_head_t *)Flash_buf;
	
	sup_head->file_count = 0;
	
	strcpy( sup_head->ver, FILESYS_VER);
	Buf_chg_flag = 1;
	fs_flush();
	return ERR_OK;
	
}


//ÎÄ¼şµÄ´æ´¢ĞÅÏ¢½á¹¹Ìå¶¼ÊÇ4×Ö½Ú¶ÔÆëµÄ£¬ËùÒÔ²»±Ø¿¼ÂÇ×Ö½Ú¶ÔÆëÎÊÌâ
//Ç°ÖÃÌõ¼ş:ÔÚ´´½¨ÎÄ¼şµÄÊ±ºò,ÌØÊâ¿éÊÇ²»ÄÜ¹»´æÔÚ¿Õ¶´µÄ,ËùÒÔÔÚÉ¾³ı²Ù×÷µÄÊ±ºò,ÒªÈ¥ÕûÀíÌØÊâÉÈÇøÖĞµÄÄÚ´æ
//³¤¶ÈÊÇ¿ÉÑ¡²ÎÊı

/**
 * @brief ´´½¨ÎÄ¼ş.
 *
 * @details ÎÄ¼şµÄ´óĞ¡µÄ·¶Î§ÊÇ4k - .
 * 
 * @param[in]	inArgName input argument description.
 * @param[out]	outArgName output argument description. 
 * @retval	OK	³É¹¦
 * @retval	ERROR	´íÎó 
 * @par ±êÊ¶·û
 * 		±£Áô
 * @par ÆäËü
 * 		ÎŞ
 * @par ĞŞ¸ÄÈÕÖ¾
 * 		XXXÓÚ201X-XX-XX´´½¨
 */
sdhFile * fs_creator(char *name,  int len)
{	
	int i = 0;
	int ret = 0;
	int end = 0;
	area_t		*tmp_area;
	sup_sector_head_t	*sup_head;
	file_info_t	*file_in_storage;
	file_info_t	*creator_file;
	storage_area_t	*target_area;
	sdhFile *p_fd;
	
	//´æ´¢Çø²»ÕıÈ·¾Í²»´¦ÀíÖ±½Ó·µ»Ø
	if( Flash_err_flag )
		return NULL;
	
	if( len <= 0)
	{
		FsErr = ERR_BAD_PARAMETER;
		goto err1;
	}
	
	tmp_area = malloc( sizeof( storage_area_t));
	if( tmp_area == NULL)
	{
		FsErr = ERR_MEM_UNAVAILABLE;
		goto err1;
	}
	ret = page_malloc( tmp_area, len);
	if( ret < 0) {
		
		FsErr =  ERR_NO_FLASH_SPACE;
		goto err2;
	}
	
	ret = read_sector( Page_Zone.fileinfo_sector_begin);

	if( ret != ERR_OK)
	{
		FsErr = ERR_STORAGE_FAIL;
		goto err2;
	}
	
	sup_head = (sup_sector_head_t *)Flash_buf;
	creator_file = ( file_info_t *)( Flash_buf+ sizeof( sup_sector_head_t));

	if( sup_head->file_count > FILE_NUMBER_MAX)
	{
		FsErr =  ERR_FILESYS_OVER_FILENUM;
		goto err2;
	}
	
	//¼ì²éÎÄ¼şÊÇ·ñÒÑ¾­´æÔÚ
	file_in_storage = searchfile( Flash_buf, name);

	if( file_in_storage )	
	{	
		printf(" file %s EXISTS!\n", name);
		FsErr =  ERR_FILESYS_FILE_EXIST;
		goto err2;
	}
	//ÕÒµ½µÚÒ»¸öÃ»ÓĞ±»Ê¹ÓÃµÄÎÄ¼şĞÅÏ¢´æ´¢Çø
	i = 0;
	for( i = 0; i < FILE_NUMBER_MAX; i ++)
	{
		if( creator_file->file_id == 0xff)
			break;
		creator_file ++;
		
	}
	if ( i == 	FILE_NUMBER_MAX)
	{
		FsErr = ERR_NO_SUPSECTOR_SPACE;
		goto err2;	
	}
	target_area = ( storage_area_t *)( Flash_buf+ sizeof( sup_sector_head_t) + FILE_NUMBER_MAX * sizeof(file_info_t));
	end = sizeof( sup_sector_head_t) + FILE_NUMBER_MAX * sizeof(file_info_t);
	i = 0;
	while(1)
	{
		if( target_area[i].file_id == 0xff)
		{
			target_area[i].file_id = sup_head->file_count;
			target_area[i].seq = 0;
			target_area[i].area.start_pg = tmp_area->start_pg;
			target_area[i].area.pg_number = tmp_area->pg_number;
			break;
		}
		i ++;
		end += sizeof(file_info_t);
		if( end > StrgInfo.sector_size)
		{
			FsErr = ERR_NO_SUPSECTOR_SPACE;
			goto err2;	
			
		}
		
		
		
	}

			
	strcpy( creator_file->name, name);
	creator_file->file_id = sup_head->file_count;
	creator_file->area_total = 1;
	sup_head->file_count ++;
		
	Buf_chg_flag = 1;
		
	p_fd = malloc( sizeof( sdhFile));
	if( p_fd == NULL) {
		FsErr = ERR_MEM_UNAVAILABLE;
		goto err2;	
	}
		
	strcpy( p_fd->name, name);
	memset( p_fd->rd_pstn, 0 , sizeof( p_fd->rd_pstn));
	memset( p_fd->wr_pstn, 0 , sizeof( p_fd->wr_pstn));
	p_fd->wr_size = 0;
	p_fd->reference_count = 1;
	p_fd->area = malloc( sizeof( area_t));
		
	//×°ÔØ´æ´¢ÇøµÄµØÖ·Çø¼äµ½ÎÄ¼şÃèÊö·ûÖĞµÄ´æ´¢Çø¼äÁ´±íÖĞÈ¥
	p_fd->area->start_pg = tmp_area->start_pg;
	p_fd->area->pg_number = tmp_area->pg_number;
	p_fd->area_total = 1;
	list_ins_next( &L_File_opened, L_File_opened.tail, p_fd);
	free( tmp_area);
	return p_fd;
	

err2:
	page_free( tmp_area, 1);
err1:
	free( tmp_area);
	return NULL;

}

//¶¨Î»µ±Ç°Î»ÖÃÔÚ´æ´¢Çø¼äµÄÆğÊ¼Ò³,²¢·µ»ØÇø¼ä
static area_t *locate_page( sdhFile *fd, uint32_t pstn, uint16_t *pg)
{
	short i = 0;
	uint32_t offset = pstn;
	int	lct_page = 0;

	
	
	for( i = 0; i < fd->area_total; i ++)
	{
		//Î»ÖÃÎ»ÓÚ±¾Çø¼äÄÚ²¿
		if( fd->area[i].pg_number *StrgInfo.page_size > offset)
		{
			
			lct_page = fd->area[i].start_pg + offset/StrgInfo.page_size ;
			*pg = lct_page;
			return &fd->area[i];
		}
		//²»Öª±¾Çø¼ä·¶Î§ÄÚ£¬È¥ÏÂÒ»¸öÇø¼ä²éÕÒ
		offset -= fd->area[i].pg_number * StrgInfo.page_size;
		
		
	}
	

	
	return NULL;		//ÎŞ·¨¶¨Î»£¬ÕâÖÖÇé¿ö³öÏÖµÄ»°£¬Ó¦¸ÃĞÂ·ÖÅäflash¿Õ¼ä
	
	
}

//
int fs_write( sdhFile *fd, uint8_t *data, int len)
{
	
	
	int 			ret;
	int 				i, limit;
	int  				myid = SYS_GETTID();
	area_t		*wr_area;
	uint16_t	wr_page = 0, wr_sector = 0;

	while( 1)
	{
		
		wr_area = locate_page( fd, fd->wr_pstn[myid], &wr_page);
		if( wr_area == NULL)
		{
			return (ERR_FILE_FULL);
		}
		wr_sector = 	wr_page/StrgInfo.sector_pagenum;
		
		if( wr_sector < Page_Zone.data_sector_begin)
		{
			return (ERR_FILE_ERROR);
			
		}
		ret = read_sector(wr_sector);
		if( ret != ERR_OK)
			return ret;
		
	
		i = ( wr_page % StrgInfo.sector_pagenum)*StrgInfo.page_size + fd->wr_pstn[myid] % StrgInfo.page_size;
		if( wr_area->pg_number + wr_area->start_pg > ( wr_sector + 1 ) *  StrgInfo.sector_pagenum )				//ÎÄ¼şµÄ½áÊøÎ»ÖÃ³¬³ö±¾ÉÈÇø£¬ÒÔÉÈÇøµÄ´óĞ¡ÎªÉÏÏŞ
				limit  = StrgInfo.sector_size;
		else		//ÎÄ¼şµÄ½áÊøÎ»ÖÃÔÚ±¾ÉÈÇøÄÚ²¿£¬ÒÔÎÄ¼ş½áÊøÎ»ÖÃÔÚ±¾ÉÈÇøÖĞµÄÏà¶ÔÎ»ÖÃÎªÉÏÏŞ
				limit = ( wr_area->start_pg +  wr_area->pg_number - wr_sector *  StrgInfo.sector_pagenum) * StrgInfo.page_size;
		while( len)
		{
			if( i >= StrgInfo.sector_size || i > limit)		 //³¬³öÁËµ±Ç°µÄÉÈÇø»òÕßÇø¼ä·¶Î§
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

int fs_read( sdhFile *fd, uint8_t *data, int len)
{
	
	int 			ret;
	int 				i, limit;
	int  				myid = SYS_GETTID();
	area_t			*rd_area;
	uint16_t	rd_page = 0, rd_sector = 0;

	
	while(1)
	{
		rd_area = locate_page( fd, fd->rd_pstn[myid], &rd_page);
		if( rd_area == NULL)
		{
			return (ERR_FILE_EMPTY);
		}
		
		rd_sector = 	rd_page/StrgInfo.sector_pagenum;
		ret = read_sector(rd_sector);
		if( ret != ERR_OK)
			return ret;
		
		//¶ÁĞ´µÄÊ±ºò£¬Òª¿¼ÂÇÎÄ¼şµÄ½áÎ²²»ÔÚ±¾ÉÈÇøµÄÇé¿ö
		i = ( rd_page % StrgInfo.sector_pagenum)*StrgInfo.page_size + fd->rd_pstn[myid] % StrgInfo.page_size;
			
		if( rd_area->start_pg +  rd_area->pg_number > ( rd_sector + 1) *  StrgInfo.sector_pagenum)	//½áÎ²Î»ÓÚ±¾ÉÈÇøÍâ
			limit = StrgInfo.sector_size;
		else	//½áÎ²Î»ÓÚ±¾ÉÈÇøÄÚ
			limit = ( rd_area->start_pg +  rd_area->pg_number - rd_sector *  StrgInfo.sector_pagenum) * StrgInfo.page_size;
			
		while( len)
		{
			if( i >= StrgInfo.sector_size || i > limit)		 //³¬³öÁËµ±Ç°µÄÉÈÇø»òÕßÇø¼ä·¶Î§
			{
				break;
			}
			*data++ = Flash_buf[i];
			i ++;
			fd->rd_pstn[myid] ++;
			len --;
				
		}
			
		if( len == 0)			//»¹ÓĞÊı¾İÃ»¶ÁÍê
		{
			
			break;
			
		}
			
	}
	
	return ERR_OK;
		
	
}

int fs_lseek( sdhFile *fd, int offset, int whence)
{
	char myid = SYS_GETTID();

	switch( whence)
	{
		case WR_SEEK_SET:
			fd->wr_pstn[ myid] = offset;
			break;
		case WR_SEEK_CUR:
			fd->wr_pstn[ myid] += offset;
			break;
		
		
		case WR_SEEK_END:			//Á¬Ğø5¸ö0xff×÷Îª½áÎ²

		
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

int fs_close( sdhFile *fd)
{
	char myid = SYS_GETTID();
	void 							*data = NULL;
	ListElmt           *elmt;
	int  ret;
	file_info_t	*file_in_storage;

	fd->reference_count --;			
	fd->rd_pstn[ myid] = 0;
	fd->wr_pstn[ myid] = 0;
	if( fd->reference_count > 0)
		return ERR_OK;
	ret = read_sector( Page_Zone.fileinfo_sector_begin);
	if( ret != ERR_OK)
		return ret;
	
	file_in_storage = searchfile( Flash_buf, fd->name);

	if( file_in_storage)
	{
		Buf_chg_flag = 1;		//ÎÄ¼ş±»¹Ø±Õ£¬ÄÇÃ´¾ÍÒª¾¡¿ìÇøË¢ĞÂflash»º´æÁË
	}
	
	elmt = list_get_elmt( &L_File_opened,fd->name);
	list_rem_next( &L_File_opened, elmt, &data);
	free( fd->area);
	
	free(fd);
	return ERR_OK;
}
///·µ»ØÎÄ¼şÕ¼ÓÃµÄ´æ´¢Çø¿Õ¼ä
int fs_du( sdhFile *fd)
{
	int i = 0;
	int size = 0;
	for( i = 0; i < fd->area_total; i ++)
	{
		size += fd->area[i].pg_number * StrgInfo.page_size;
	}
	return size;
	
}

int fs_delete( sdhFile *fd)
{
	int ret = 0;
	file_info_t	*file_in_storage;
	storage_area_t	*src_area;
	sup_sector_head_t	*sup_head;
	short  j, k;
	short end = 0;

	
	fd->reference_count --;
	if( fd->reference_count > 0)
		return ERR_FILE_OCCUPY;
	ret = read_sector(Page_Zone.fileinfo_sector_begin);
	if( ret != ERR_OK)
		return ret;
	sup_head = ( sup_sector_head_t *)Flash_buf;
	k = 0;
	file_in_storage = searchfile( Flash_buf, fd->name);
	//´æ´¢Æ÷µÄ¹ÜÀíÇøÕÒµ½¸ÃÎÄ¼şµÄ¹ÜÀíÄÚÈİÈ»ºóÉ¾³ıµô
	
	if( file_in_storage )
	{
		
		
		src_area = ( storage_area_t *)( Flash_buf + sizeof(sup_sector_head_t) + FILE_NUMBER_MAX * sizeof(file_info_t) );
		end = sizeof(sup_sector_head_t) + FILE_NUMBER_MAX * sizeof(file_info_t);
		j = 0;
		while(1)
		{
			if( src_area[j].file_id == file_in_storage->file_id)
			{
				memset( &src_area[j], 0xff, sizeof(storage_area_t));
				k ++;
				
			}
			if( k == file_in_storage->area_total)
				break;
			end += sizeof(file_info_t);
			if( end > StrgInfo.sector_size)
				break;
			
		}
		
		Buf_chg_flag = 1;
		memset( file_in_storage, 0xff, sizeof(file_info_t));
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
	//´æ´¢Çø²»ÕıÈ·¾Í²»´¦ÀíÖ±½Ó·µ»Ø
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





static int read_page( uint8_t *rd_buf,  uint16_t page)
{
	return flash_read_page(rd_buf,page);
	
}


static int read_sector( uint16_t sector)
{
	int ret = 0;
	if( Src_sector == sector )		
	{
		//±¾´Î¶ÁÈ¡µÄÉÈÇøÓëÉÏ´Î¶ÁÈ¡µÄÒ»ÖÂÊ±ºò£¬¿ÉÒÔ²»±ØÔÙÈ¥¶ÁÈ¡
		//Èç¹û»º´æĞŞ¸Ä±êÖ¾ÖÃÆğ£¬ËµÃ÷»º´æÖĞµÄÄÚÈİ±ÈflashÖĞµÄÄÚÈİ¸üĞÂ
		
		return ERR_OK;
	}
	
	//¶ÁÈ¡ÁíÍâÒ»¸öÉÈÇø£¬Ôò¸ù¾İ»º´æĞŞ¸Ä±êÖ¾À´¾ö¶¨ÊÇ·ñ½«»º´æÄÚÈİĞ´ÈÕflash

	if( Buf_chg_flag)
	{
		ret = flush_flash( Src_sector);
		if( ret != ERR_OK)
		{
			return ret;
			
		}
//		Src_sector = INVALID_SECTOR;
	}
	SYS_ARCH_PROTECT();
	ret = flash_read_sector(Flash_buf,sector);
	SYS_ARCH_UNPROTECT();
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
	
	SYS_ARCH_PROTECT();
	ret = flash_erase_sector(sector);
	
	if( ret != ERR_OK )
	{
		SYS_ARCH_UNPROTECT();
		return ret;
	}

	ret =  flash_write_sector( Flash_buf, sector);
	SYS_ARCH_UNPROTECT();
	return ret;
}

static file_info_t	*searchfile( uint8_t	*flash_data, char *name)
{
	int i = 0;
	file_info_t	*file_in_storage;
	file_in_storage = ( file_info_t *)( flash_data + sizeof(sup_sector_head_t));
	
	//´æ´¢Æ÷µÄ¹ÜÀíÇøÕÒµ½¸ÃÎÄ¼şµÄ¹ÜÀíÄÚÈİÈ»ºóÉ¾³ıµô
	for( i = 0; i < FILE_NUMBER_MAX; i ++)
	{
		if( strcmp(file_in_storage->name, name) == 0x00 )
		{
							
			return file_in_storage;
		}
		
		file_in_storage ++;
		
		
	}
	return NULL;
	
}

//·µ»ØµÄ¿Õ¼äÊ±µ±Ç°ÄÜ¹»ÕÒµ½µÄ×îºÏÊÊµÄ¿Õ¼ä£¬²»Ò»¶¨ÄÜ¹»Âú×ãÇëÇóµÄ¿Õ¼ä£¬µ÷ÓÃÕßÀ´¼ì²é½á¹û²¢¾ö¶¨ÊÇ·ñÔÙ´Îµ÷ÓÃÀ´²¹×ãÊ£ÓàµÄ¿Õ¼ä
static int page_malloc( area_t *area, int size)
{
	uint16_t	mem_manger_sector = 0;
	uint16_t	offset_page ;
	uint16_t	end_page = 0;
	uint16_t j = 0;
	int ret = 0;
	
	
	mem_manger_sector = Page_Zone.pguseinfo_sector_begin;
	offset_page = Page_Zone.data_sector_begin * StrgInfo.sector_pagenum;
	while(1)
	{
		
		ret = read_sector(mem_manger_sector);
		if( ret != ERR_OK)
			return ret;
		//´ÓÊı¾İ´æ´¢ÇøµÄ¿ÕÏĞÒ³ÖĞÕÒµ½ÄÜ¹»·ûºÏ³¤¶ÈÒªÇóµÄÆğÊ¼Ò³µØÖ·
		
		get_area( offset_page, size, area);
		if( area->start_pg > 0)	
		{
			
			
			
			end_page = area->start_pg + area->pg_number;
			for( j = area->start_pg; j < end_page; j ++)
				clear_bit( Flash_buf, j);
			Buf_chg_flag = 1;
			
			//todo:Ò»´Î·ÖÅäÎŞ·¨Âú×ã×ã¹»µÄ´óĞ¡µÄ´¦Àí
			if( area->pg_number * 	StrgInfo.page_size < size)
			{
				;
			}
			area->start_pg += ( mem_manger_sector - Page_Zone.pguseinfo_sector_begin) * StrgInfo.sector_size * 8;
			return area->pg_number;
			
		}
		
		
		
		mem_manger_sector ++;
		offset_page = 0;
		if( mem_manger_sector == Page_Zone.pguseinfo_sector_end)
			return ERR_NO_FLASH_SPACE;
		
		
	}
			
		
}


//ÎÄ¼şµÄÄÚ´æÒªÔÚÍ¬Ò»¸öÉÈÇø¹ÜÀíµÄÄÚ´æÒ³ÇøÖĞ²ÅÄÜÊ¹ÓÃ¸Ä³ÌĞò
static int page_free( area_t *area, int area_num)
{
	
	uint16_t i = 0;
	uint16_t j = 0, k = 0;
	uint16_t	mem_manger_sector = 0;
	uint16_t	sector_bit = StrgInfo.sector_size * 8;
	uint16_t	sector_offset = 0;
	int ret = 0;
	
		


	while( area_num)
	{


		if( area->start_pg + area->pg_number > StrgInfo.total_pagenum)
				return -1;
		
		sector_offset = area->start_pg / sector_bit;
		mem_manger_sector = Page_Zone.pguseinfo_sector_begin + sector_offset;	
		ret = read_sector(mem_manger_sector);
		if( ret != ERR_OK)
			return ret;	
		
		
		i = area[k].start_pg % ( sector_offset * sector_bit);
		for( j = 0; j < area[k].pg_number; j ++)
			set_bit( Flash_buf, i + j);
		
		Buf_chg_flag = 1;
		
		area_num --;
		if( area_num)
			area ++;
	}
	
		
	return ERR_OK;
		
	
			
			
			
			
			
	
	
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

///¾¡¿ÉÄÜµÄ·µ»Ø×ã¹»´óµÄ¿Õ¼ä
static void get_area( int pg_offset, int size, area_t *out_area)
{
	short i, j ;
	int pages = 0;
	area_t	max_area = {0,0};
	
	size --;
	pages = size / StrgInfo.page_size + 1;

	
	for( i = pg_offset; i < StrgInfo.sector_size * 8; i++)
	{
		
		if( check_bit( Flash_buf, i)) 		
		{
			
			
			for( j = 0; j < pages; j ++)	//¼ì²é³¤¶ÈÊÇ·ñ´ï±ê
			{
				if( !check_bit( Flash_buf, i+j)) 
					break;
				
			}
			if( j == pages)
			{
				out_area->start_pg = i;
				out_area->pg_number = pages;
				return ;
			}
			
			if(max_area.pg_number < j  )
			{
				max_area.start_pg = i;
				max_area.pg_number = j ;
			}
			i+= j;		///Ìø¹ıÕâ¸öÁ¬ĞøµÄÇø¼ä£¬Ñ°ÕÒÏÂÒ»¸öÇø¼ä
			
		}
		
	}
	
	out_area->start_pg = max_area.start_pg;
	out_area->pg_number = max_area.pg_number;;
	
	
}

/**
 * @brief ÎÄ¼şÏµÍ³µÄ²âÊÔ³ÌĞò.
 *
 * @details ÓÃ¾¡¿ÉÄÜµÄÈ«¸²¸Ç´úÂëÂ·¾¶µÄÔ­ÔòÀ´±àĞ´Õâ¸ö²âÊÔ³ÌĞò.
 * 
 * @param[in]	inArgName input argument description.
 * @param[out]	outArgName output argument description. 
 * @retval	OK	??
 * @retval	ERROR	?? 
 */
#define TEST_FILENAME "Max_file1.test"
int fs_test(void)
{
	int ret = 0;
	uint32_t	filesize = 4096;
	sdhFile	*ftest;
	uint8_t data;
	int i = 0;
	
	DPRINTF(" init filesystem successed! \n");
//	filesize = StrgInfo.sector_size *(Page_Zone.data_sector_end - Page_Zone.data_sector_begin);
	ftest = fs_open(TEST_FILENAME);
	if( fs_get_error() == ERR_FILESYS_ERROR)
	{
		ret = fs_format();
		if( ret != ERR_OK)
		{
			DPRINTF(" format filesystem fail, return val %d \n", ret);
			return ERR_FAIL;
			
		}
		DPRINTF(" format filesystem successed! \n");
	}

	if( fs_get_error() != ERR_OK)
	{
		DPRINTF("try create a max file, size %d KB,  ", filesize/1024);
		ftest = fs_creator(TEST_FILENAME,  filesize);
		if( ftest == NULL)
		{
			DPRINTF(" failed !\n");
			return ERR_FAIL;
			
		}
		filesize = fs_du(ftest);
		DPRINTF("created %dkB succeed! \n", filesize );
	}
	filesize = fs_du(ftest);
	for( i = 0; i < filesize; i ++)
	{
		data = i;
		if( fs_write( ftest, &data, 1) != ERR_OK)
		{
			DPRINTF(" fs_write failed at the %d times\n", i);
			return ERR_FAIL;
		}
		
	}
	DPRINTF(" fs_write %dKB data succeed\n", i/1024);
	
	for( i = 0; i < filesize; i ++)
	{
		if( fs_read( ftest, &data, 1) != ERR_OK)
		{
			DPRINTF(" fs_read failed at the %d times\n", i);
			return ERR_FAIL;
		}
		if( data != ( i & 0xff))
		{
			DPRINTF(" read data is  %d  != %d, err\n", data, i&0xff);
			return ERR_FAIL;
		}
		
	}
	DPRINTF(" fs_read %d data succeed\n", i/1024);
	return ERR_OK;
	
}











