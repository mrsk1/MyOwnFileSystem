/*!
 *  \file       FileSystem.h
 *  \brief      Header to Create a own filesystem simulation
 *  \author     several - andreas
 *
 *
 ******************************************************************************
 *  \par  History
 *  \n==== History ============================================================
 *  date        Author     version   action
 *  ---------------------------------------------------------------------------
 * 14-Jul-2016 Karthik M              Adding Debug  Macros
 *******************************************************************************/


#ifndef _FILESYSTEM_H
#define _FILESYSTEM_H

/* buffer MAX lengyh */
#define LINE_MAX 100
/* File name length */
#define FNAME_LEN 8
/* Mega Byte Value */
#define MB (1024 * 1024)
/* Alignment Range */
#define ALIGN 0x1000
/* Block Size in bytes */
#define BLK_SIZE 1024
/* Inode Count */
#define INODES 1024
/* Total Block Count */
#define T_BLKS	1024

#define PRINT printf
#define DB_PRINT printf
#define NOPRINT(...)

typedef unsigned int u_int;
typedef unsigned short u_short;
typedef unsigned char u_char;

/**
 * @brief Enum To Check with the Return State
 */
enum RetValue {
   RET_ERR = -1,                    //<   -1 Error State
   RET_OK                           //<    0 Ok State
};
typedef enum RetValue RetVal;

/**
 * @brief Struct to Maintain the SuperBlock Details
 */
struct super_block {
   u_int total_inodes;            //< Total num of Indoes in Filesystem
   u_int total_datablocks;        //< Total num of Datablocks in Filesystem
   u_int free_inodes;             //< Total num of Freeindoes in Filesystem
   u_int free_datablocks;         //< Total num of Freedatablocks inf Filesystem
};

/**
 * @brief Struct to Maintain the Inode Details
 */
struct inode {
   u_int i_no:10;                 //< Index of the Inode in Indoe Table
   u_int size:12;                 //< Size of the File
   u_int blk_used:3;              //< Number of Blks used for the File
   u_short blk[5];                //< Index of the Blks
   u_char fname[FNAME_LEN];               //< Filename  of the File
};

/* TODO:Need to Add the Fucntions with or without static ? */
#endif

