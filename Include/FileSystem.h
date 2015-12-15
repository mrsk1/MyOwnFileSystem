#ifndef _FILESYSTEM_H
#define _FILESYSTEM_H

#define MB (1024 * 1024)
#define ALIGN 0x1000
#define BLK_SIZE 1024
#define INODES 1024
#define T_BLKS	1024

typedef unsigned int u_int;
typedef unsigned short u_short;
typedef unsigned char u_char;

/**
 * @brief Enum To Check with the Return State
 */
enum RetValue {
   RET_ERR = -1,                    //<   Error State
   RET_OK                           //<   Ok State
};
typedef enum RetValue RetVal;

struct super_block {
   u_int total_inodes;
   u_int total_datablocks;
   u_int free_inodes;
   u_int free_datablocks;
};

struct inode {
   u_int i_no:10;
   u_int size:12;
   u_int blk_used:3;
   u_short blk[5];
   u_char fname[8];
};

static void init_fs(void);

#endif

