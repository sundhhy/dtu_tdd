#ifndef __FILE_SYS_H__
#define __FILE_SYS_H__
#include "system_data.h"
//#include "lw_oopc.h"
#include "app.h"
#include "list.h"

#define FLASH_NULL_FLAG  0xffffffff
#define FILE_NUMBER_MAX								20				//最多可以创建的文件数量
#define STOREAGE_AREA_NUMBER_MAX			60				//
#define SUP_SECTOR_NUM					3
#define FILE_INFO_SECTOR				0
#define FLASH_USEE_SECTOR1			1
#define FLASH_USEE_SECTOR2			2
#define PAGE_AVAILBALE_OFFSET		( SUP_SECTOR_NUM * SECTOR_SIZE / PAGE_SIZE)
#define PAGE_REMIN_FOR_IMG			1024				//256k用于存储可执行镜像文件		
#define SECTOR_PAGE_BIT					32768			//8 * 4096

#define	INVALID_SECTOR					0xffff


#define flash_erase_sector 			w25q_Erase_Sector
#define	flash_write_sector			w25q_Write_Sector_Data
#define	flash_read_sector				w25q_Read_Sector_Data

//CMD


typedef enum {
	WR_SEEK_SET = 0,
	WR_SEEK_CUR = 1,
	WR_SEEK_END = 2,
	RD_SEEK_SET = 3,
	RD_SEEK_CUR = 4,
	RD_SEEK_END = 5,
	GET_WR_END = 6,
	GET_RD_END = 7,
}lseek_whence_t;
typedef enum {
	

	
	F_State_idle,
	F_State_Number,
	F_State_Err,
	
}task_flash_state;

typedef enum {
	ERROR_FILESYS_BEGIN = ERROR_BEGIN(MODULE_FILESYS),
	ERR_FLASH_UNAVAILABLE,
	ERR_FILE_UNAVAILABLE,
	ERR_FILE_EMPTY,
	ERR_FILE_FULL,
	ERR_FILESYS_BUSY,
	ERR_FILESYS_ERROR,
	ERR_FILE_ERROR,
	ERR_FILE_OCCUPY,
	ERR_OPEN_FILE_FAIL,
	ERR_CREATE_FILE_FAIL,
	ERR_NO_SUPSECTOR_SPACE,
	ERR_NO_FLASH_SPACE,
	
	INCREASE_PAGE,
	
}filesys_err_t;

typedef struct {
	short				file_count;				//指向第一个未使用的地址
	uint8_t				ver[6];
	
	
}sup_sector_head_t;

typedef struct {
	
	uint16_t 										start_pg;		//起始页号
	uint16_t 										pg_number;				//页的数量
	
}area_t;
typedef struct {
	uint8_t											file_id;
	uint8_t											seq;
	area_t											area;
	
}storage_area_t;




//文件在flash中的存储信息
typedef struct {
	char 												name[16];
	uint8_t											file_id;					//文件id用来联系文件与它的存储区间信息.
	uint8_t											area_total;
//	uint32_t										wr_bytes;							//当前文件被写入的字节数
}file_info_t;


//文件系统中操作文件时使用的描述符



typedef struct {
	char name[16];
	
	uint32_t		rd_pstn[Prio_task_end];								//文件的读写位置，以字节为单位
	uint32_t		wr_pstn[Prio_task_end];	
	
	uint32_t		wr_size;													//保存当前文件已经被写入的最长长度
	
	uint16_t		reference_count;	
	uint16_t		area_total;		
	
	area_t			*area;									//存储区间
	
}file_Descriptor_t;








int filesys_init(void);
int filesys_close(void);
int filesys_dev_check(void);
error_t fs_open(char *name, file_Descriptor_t **fd);
error_t fs_creator(char *name, file_Descriptor_t **fd, int len);
error_t fs_expansion(file_Descriptor_t *fd, int len);			//增加文件的容量
error_t fs_write( file_Descriptor_t *fd, uint8_t *data, int len);
error_t fs_read( file_Descriptor_t *fd, uint8_t *data, int len);
error_t fs_lseek( file_Descriptor_t *fd, int offset, int whence);
error_t fs_delete( file_Descriptor_t *fd);
error_t fs_close( file_Descriptor_t *fd);
error_t fs_flush( void);
error_t fs_format(void);
error_t wait_fs(void);
#endif
 