/*!
 *  \file       FileSystem.c
 *  \brief      Program to Create a own filesystem simulator
 *  \author     several - andreas
 *
 *
 ******************************************************************************
 *  \par  History
 *  \n==== History ============================================================
 *  date        Author     version   action
 *  ---------------------------------------------------------------------------
 * 15-Dec-2015 Karthik M              Implementation of Deleting file
 *******************************************************************************/

# include <stdio.h>
# include <string.h>
# include <stdarg.h>
# include <stdint.h>
# include <stdlib.h>
# include "FileSystem.h"

void *fs;					// file system
void *dbm;
void *ibm;
struct super_block *sb;		// super block
struct inode *i_tab;		// table of inode structure
void *d_blks;
int update_sb = 0;

static int UpdateSuperBlock(int update_sb, ...);

/**
 * @fn      static void InitFS();
 * @brief   This function to Intialize the FileSystem
 * @params  <None>
 *
 */

static void InitFS()
{
   if (posix_memalign(&fs, ALIGN, MB)) {
      perror("posix_memalign failed");
   }

   sb	= (struct super_block *) fs;
   dbm = (void *) sb + BLK_SIZE;
   ibm = dbm + BLK_SIZE;
   i_tab  = ibm + BLK_SIZE;
   d_blks = (void *) i_tab + (INODES * sizeof(struct inode));

   //printf("sizeof(struct inode)  =    %d\n", (int) sizeof(struct inode));

   printf("fs  = 0x%08X\n", (unsigned int)(intptr_t)fs);
   printf("sb  = 0x%08X\n", (unsigned int)(intptr_t) sb);
   printf("dbm = 0x%08X\n", (unsigned int)(intptr_t) dbm);
   printf("ibm = 0x%08X\n", (unsigned int)(intptr_t) ibm);
   printf("i_tab =  0x%08X\n", (unsigned int)(intptr_t) i_tab);
   printf("d_blks = 0x%08X\n", (unsigned int)(intptr_t) d_blks);

   sb->total_inodes = INODES;		// Each inode represents one inode structure in inode table
   sb->total_datablocks = T_BLKS;	// We have 1MB of total memory and blk size is 1KB so will have 1024 blocks
   sb->free_inodes = INODES;
   sb->free_datablocks = T_BLKS - (3 + (INODES / (BLK_SIZE / sizeof(struct inode))));

   printf("sb->total_inodes = %d\n", sb->total_inodes);
   printf("sb->total_datablocks = %d\n", sb->total_datablocks);
   printf("sb->free_inodes	 = %d\n", sb->free_inodes);
   printf("sb->free_datablocks = %d\n", sb->free_datablocks);

   dbm = memset(dbm, 0, 128);
   //printf ("dbm = %d\n",*(int*)dbm);
   ibm = memset(ibm, 0, 128);
   //printf ("ibm = %d\n",*(int*)ibm);
}

/**
 * @fn     static int GetPosition(void *block);
 * @brief  Function to  Get the Position of the Free bit in GroupDescriptor
 * @param  block Pointer to the Block
 * @return position of free bit in the Block
 */

static int GetPosition(void *block)
{
   // Get the free bit and set it, get the position
   int *p;
   int pos = 0;
   p = (int*)block;
   while (pos <= 1023) {
      if (*p & (1 << pos))
         pos++;
      else
         return pos;
   }
}

/**
 * @fn     static RetVal SetBit(void *block, int pos)
 * @brief  Function to  set the Position of the Free bit in GroupDescriptor
 * @param  block Pointer to the Block
 * @param  pos  position of set bit
 * @return RET_OK on Success
 *         RET_ERR on Error
 */

static RetVal SetBit(void *block, int pos)
{
   int *p;
   p = (int*)block;
   int mov;
   int set_pos;

   mov = pos/32;
   set_pos = pos%32;
   p = p + mov;

   *p = *p | (1 << set_pos);
   if (*p & (1 << set_pos))
      return RET_OK;
   return RET_ERR;
}

/**
 * @fn     static RetVal ClearBit(void *block, int pos)
 * @brief  Function to  Clear the Position of the Free bit in GroupDescriptor
 * @param  block Pointer to the Block
 * @param  pos  position of Clear bit
 * @return RET_OK on Success
 *         RET_ERR on Error
 */

static RetVal ClearBit(void *block, int pos)
{
   int *p;
   p = (int*)block;
   int mov;
   int set_pos;

   mov = pos/32;
   set_pos = pos%32;
   p = p + mov;

   *p = *p & (~(1 << set_pos));
   if (!(*p & (1 << set_pos)))
      return RET_OK;
   return RET_ERR;
}

static int Filling_ITable(int index, char *file_name)
{
   struct inode *temp_inode;
   temp_inode = i_tab + index;
   temp_inode->i_no = index;
   strcpy(temp_inode->fname, file_name);
   printf("File name = %s\n", temp_inode->fname);

}
static int UpdateSuperBlock(int update_sb, ...)
{
   va_list ptr;
   int pos;


   if (update_sb == 0)
      sb->free_inodes = sb->free_inodes - 1;
   else if(update_sb == 1){
      va_start(ptr, update_sb);
      pos = va_arg(ptr, int);
      //printf("update_sb = %d pos = %d\n", update_sb, pos);
      sb->free_datablocks = sb->free_datablocks - pos;
   }else{
      va_start(ptr,update_sb);
      pos = va_arg(ptr, int);
      printf("update_sb = %d pos = %d\n", update_sb, pos);
      sb->free_datablocks = sb->free_datablocks + pos ;
   }

}
static int CreateFile(char *file_name)
{
   int pos;
   int update_sb = 0;
   pos = GetPosition(ibm);
   //printf ("pos = %d\n", pos);
   if (SetBit(ibm, pos) == 0)
      printf ("bit is set successfuly\n");
   Filling_ITable(pos, file_name);
   UpdateSuperBlock(update_sb, pos);
}
static int GetInode(char *file_name, int *inode)
{
   int used_inodes;
   struct inode *temp_inode;
   temp_inode = i_tab;

   //Get the inode, search in inode table for the corresponding filename
   used_inodes = INODES - sb->free_inodes;

   while (used_inodes--){
      if (strcmp(file_name, temp_inode->fname) == 0){
         *inode = temp_inode->i_no;
         return 0;
      }
      temp_inode++;
   }
   return -1;
}
static int WriteInToFile(char *file_name)
{
   int inode;
   char *buf;
   char c;
   int count;
   int pos;
   struct inode *temp_inode;
   void *temp_data;
   unsigned char update_sb;

   update_sb = 1;
   buf = NULL;


   if (GetInode(file_name, &inode) == 0) {
      printf("File name found and inode = %d\n", inode);
   } else {
      printf ("Enter valid file_name\n");
      return -1;
   }

   for (count = 0;(c = getchar()) != EOF; count++) {
      buf = (char*) realloc(buf, count + 1);
      buf[count] = c;
   }

   pos = GetPosition(dbm);
   if (SetBit(dbm, pos) == 0)
      printf("data bit is set successfuly\n");

   count = sizeof(buf) / BLK_SIZE;
   if (sizeof(buf) % BLK_SIZE != 0)
      count++;

   temp_data = d_blks + (pos * BLK_SIZE);
   memcpy(temp_data, buf, strlen(buf) + 1);

   /* Update inode struct */
   temp_inode = i_tab + inode;
   temp_inode->size = sizeof(buf);
   temp_inode->blk_used = count;

   for (c = 0;c < count; c++)
      temp_inode->blk[c] = pos + c;

   /* Updating super_block after allocating data */
   UpdateSuperBlock(update_sb, count);

}

static int SuperBlockDetails()
{
   printf("sb->total_inodes = %d\n", sb->total_inodes);
   printf("sb->total_datablocks = %d\n", sb->total_datablocks);
   printf("sb->free_inodes  = %d\n", sb->free_inodes);
   printf("sb->free_datablocks = %d\n", sb->free_datablocks);

}

static int ReadingBlockContents(char *file_name)
{
   int inode;
   void *Data;
   unsigned short Count;
   struct inode *temp_inode;


   if (GetInode(file_name, &inode) == 0) {
      printf("File name found and inode = %d\n", inode);
   } else {
      printf ("Enter valid file_name\n");
      return -1;
   }
   temp_inode = i_tab + inode;
   for (Count = 0; Count < temp_inode->blk_used; Count++){
      Data = d_blks + (temp_inode->blk[Count] * BLK_SIZE);
      write(1, Data,BLK_SIZE);
   }

}

static int printfile(int count)
{
   struct inode *temp_inode ;
   temp_inode = i_tab + count;
   printf("file name = %s\n",temp_inode->fname);
   printf("inode num = %d\n",temp_inode->i_no);
   printf("file size = %d\n",temp_inode->size);
   printf("blks used = %d\n",temp_inode->blk_used);
}

static int list()
{
   short int count;
   int *temp ;
   temp = (int *)ibm;
   for(count = 0; count < BLK_SIZE; count++){
      if (((count%32) == 0) && (count != 0))
         temp++;
      if ((*temp)&(1<<count)){
         printfile(count);
      }
   }

}

static int DeleteFile(char *filename)
{
   int inodenum;
   int i;
   struct inode *temp_inode;
   inodenum = 1;

   if (GetInode(filename,&inodenum) == -1){
      printf("Enter Vaild File name \n");
      return -1;
   }

   /** Get the Inode */
   temp_inode = i_tab + inodenum ;
  /** Clear All Data blocks bitmap */
   for (i = 0; i < temp_inode->blk_used ; i++)
      ClearBit(dbm, temp_inode->blk[i]);
   /** Clear inode bitmap */
   ClearBit(ibm, inodenum);
   UpdateSuperBlock(2,temp_inode->blk_used);
   memset(temp_inode,'\0',sizeof(struct inode));

}

int main()
{
   int choice;
   char file_name[8];
   int count;
   InitFS();

   while(1){
      printf("\n#################Enter choice:\n");
      printf("1->Create\n");
      printf("2->Write\n");
      printf("3->Super\n");
      printf("4->Display\n");
      printf("5->List Files\n");
      printf("6->Delete File\n");
      scanf("%d", &choice);

      switch (choice)
      {
         case 1:
            printf("Enter the file name:");
            scanf("%s",file_name);
            CreateFile(file_name);
            break;
         case 2:
            printf("To which file you want enter:");
            scanf("%s", file_name);
            WriteInToFile(file_name);
            break;
         case 3:
            SuperBlockDetails();
            break;
         case 4:
            printf("Enter the file_name:");
            scanf("%s", file_name);
            ReadingBlockContents(file_name);
            break;
         case 5:
            list();
            break;
         case 6:
            printf("Enter the file_name To Delete:");
            scanf("%s", file_name);
            DeleteFile(file_name);
            break;
         default:
            printf("Enter valid option\n");
            break;
      }
   }
   return 0;
}
