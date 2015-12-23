/*!
 *  \file       FileSystem.c
 *  \brief      Program to Create a own filesystem simulation
 *  \author     several - andreas
 *
 *
 ******************************************************************************
 *  \par  History
 *  \n==== History ============================================================
 *  date        Author     version   action
 *  ---------------------------------------------------------------------------
 * 21-Dec-2015 Karthik M              Commenting the Functions
 *******************************************************************************/

# include <stdio.h>
# include <string.h>
# include <stdarg.h>
# include <stdint.h>
# include <stdlib.h>
# include "FileSystem.h"


#define PRINT printf
#define NOPRINT(...)
void *fs;					// file system
void *dbm;
void *ibm;
struct super_block *sb;		// super block
struct inode *i_tab;		// table of inode structure
void *d_blks;
int UpdateSB = 0;

static int UpdateSuperBlock(int UpdateSB, ...);

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
      exit(EXIT_FAILURE);
   }

   sb	= (struct super_block *) fs;
   dbm = (void *) sb + BLK_SIZE;
   ibm = dbm + BLK_SIZE;
   i_tab  = ibm + BLK_SIZE;
   d_blks = (void *) i_tab + (INODES * sizeof(struct inode));


   NOPRINT("fs  = 0x%08X\n", (unsigned int)(intptr_t)fs);
   NOPRINT("sb  = 0x%08X\n", (unsigned int)(intptr_t) sb);
   NOPRINT("dbm = 0x%08X\n", (unsigned int)(intptr_t) dbm);
   NOPRINT("ibm = 0x%08X\n", (unsigned int)(intptr_t) ibm);
   NOPRINT("i_tab =  0x%08X\n", (unsigned int)(intptr_t) i_tab);
   NOPRINT("d_blks = 0x%08X\n", (unsigned int)(intptr_t) d_blks);
   /* Each inode represents one inode structure in inode table */
   sb->total_inodes = INODES;
   /* We have 1MB of total memory and blk size is 1KB so will have
      1024 blocks */
   sb->total_datablocks = T_BLKS;	   sb->free_inodes = INODES;
   sb->free_datablocks = T_BLKS -
      (3 + (INODES / (BLK_SIZE / sizeof(struct inode))));

   PRINT("sb->total_inodes = %d\n", sb->total_inodes);
   PRINT("sb->total_datablocks = %d\n", sb->total_datablocks);
   PRINT("sb->free_inodes	 = %d\n", sb->free_inodes);
   PRINT("sb->free_datablocks = %d\n", sb->free_datablocks);

   dbm = memset(dbm, 0, 128);
   ibm = memset(ibm, 0, 128);
}


/**
 * @fn     static int GetPosition(void *block);
 * @brief  Function to  Get the Position of the Free bit in GroupDescriptor
 * @param  Block Pointer to the Block
 * @return position of free bit in the Block
 */
static int GetPosition(void *Block)
{
   int *Ptr;
   int Pos = 0;

   Ptr = (int*)Block;
   while (Pos <= 1023) {
      if (*Ptr & (1 << Pos))
         Pos++;
      else
         return Pos;
   }
}

/**
 * @fn     static RetVal SetBit(void *block, int pos);
 * @brief  Function to  set the Position of the Free bit in GroupDescriptor
 * @param  Block Pointer to the Block
 * @param  Pos  position of set bit
 * @return RET_OK on Success
 *         RET_ERR on Error
 */
static RetVal SetBit(void *Block, int Pos)
{
   int *Ptr;
   int Move;
   int SetPos;

   if (NULL == Block){
      printf("%s:Invalid Block  \n", __func__);
      return RET_ERR;
   }
   if (-1 >= Pos){
      printf("%s:Invalid Positoin %d \n", __func__, Pos);
      return RET_ERR;
   }

   Ptr = (int*)Block;
   Move = Pos/32;
   SetPos = Pos%32;
   Ptr = Ptr + Move;

   *Ptr = *Ptr | (1 << SetPos);
   if (*Ptr & (1 << SetPos))
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
static RetVal ClearBit(void *Block, int Pos)
{
   int mov;
   int *Ptr;
   int SetPos;

   if (NULL == Block){
      printf("%s:Invalid Block \n", __func__);
      return RET_ERR;
   }
   if (-1 >= Pos){
      printf("%s:Invalid Positoin %d \n", __func__, Pos);
      return RET_ERR;
   }
   Ptr = (int*)Block;
   mov = Pos/32;
   SetPos = Pos%32;
   Ptr = Ptr + mov;

   *Ptr = *Ptr & (~(1 << SetPos));
   if (!(*Ptr & (1 << SetPos)))
      return RET_OK;
   return RET_ERR;
}

/**
 * @fn     static int FillingITable(int Index, char *FileName);
 * @brief  Function to  Fill the Inode with Filename
 * @param  Index Index of the Inode Table.
 * @param  FileName Filename to be filled in the inode structure.
 * @return RET_OK on Success
 *         RET_ERR on Error
 */
static RetVal FillingITable(int Index, char *FileName)
{
   struct inode *TempInode;

   if ( RET_ERR == Index){
      PRINT("%s Invalid index %d \n", __func__, Index);
      return Index;
   }
   if ( NULL == FileName){
      PRINT("%s Invalid FileName \n", __func__);
      return RET_ERR;
   }

   TempInode = i_tab + Index;
   TempInode->i_no = Index;
   strcpy(TempInode->fname, FileName);
   NOPRINT("File name = %s\n", TempInode->fname);
   return RET_OK;

}

/**
 * @fn     static int UpdateSuperBlock(int UpdateSB, ...);
 * @brief  Function to  Update the SuperBlock structure.
 * @param  UpdateSB  Value to Determine which operation needs to be performed
 * @param  vararg  paramerts to update the Super Block
 * @return RET_OK on Success
 *         RET_ERR on Error
 */
static RetVal UpdateSuperBlock(int UpdateSB, ...)
{
   int Pos;
   va_list Ptr;

   if (-1 >= UpdateSB){
      PRINT("%s: Invalid UpdateSb = %d \n", __func__, UpdateSB);
      return RET_ERR;
   }
   if (0 == UpdateSB)
      sb->free_inodes = sb->free_inodes - 1;
   else if (1 == UpdateSB){
      va_start(Ptr, UpdateSB);
      Pos = va_arg(Ptr, int);
      //PRINT("UpdateSB = %d Pos = %d\n", UpdateSB, Pos);
      sb->free_datablocks = sb->free_datablocks - Pos;
   }else{
      va_start(Ptr,UpdateSB);
      Pos = va_arg(Ptr, int);
      PRINT("UpdateSB = %d Pos = %d\n", UpdateSB, Pos);
      sb->free_datablocks = sb->free_datablocks + Pos ;
   }
   return RET_OK;
}

/**
 * @fn     static int CreateFile(char *FileName);
 * @brief  Function to Create a file in Filesystem.
 * @param  FileName   Name of the file to be created.
 * @return RET_OK on Success
 *         RET_ERR on Error
 */
static RetVal CreateFile(char *FileName)
{
   int Pos;
   int UpdateSB = 0;

   if (NULL == FileName){
      PRINT("%s Invalid FileName \n", __func__);
      return RET_ERR;
   }
   Pos = GetPosition(ibm);
   NOPRINT("Pos = %d\n", Pos);
   if (SetBit(ibm, Pos) == 0)
      PRINT("bit is set successfuly\n");
   FillingITable(Pos, FileName);
   UpdateSuperBlock(UpdateSB, Pos);
   return RET_OK;
}

/**
 * @fn     static int GetInode(char *FileName, int *Inode);
 * @brief  Function to Get the Inode number based on the Filename
 * @param  FileName   Name of the File to be searched in Inode
 * @param  Inodenum   int ptr to update the Inode Number
 * @return RET_OK on Success
 *         RET_ERR on Error
 */
static RetVal GetInode(char *FileName, int *InodeNum)
{
   int UsedInodes;
   struct inode *TempInode;

   if (NULL == FileName){
      PRINT("%s: Invalid Filename\n", __func__);
      return RET_ERR;
   }
   if ( -1 >= *InodeNum){
      PRINT("%s: Invalid InodNum %d\n", __func__, *InodeNum);
      return RET_ERR;
   }
   TempInode = i_tab;
   /* Get the inode, search in inode table for the corresponding filename */
   UsedInodes = INODES - sb->free_inodes;

   while (UsedInodes--){
      if (strcmp(FileName, TempInode->fname) == 0){
         *InodeNum = TempInode->i_no;
         return RET_OK;
      }
      TempInode++;
   }
   return RET_ERR;
}

/**
 * @fn     static RetVal WriteInToFile(char *FileName)
 * @brief  Function to Write the Data info Filename
 * @param  FileName   Name of the File to Written the File Content
 * @return RET_OK on Success
 *         RET_ERR on Error
 */
static RetVal WriteInToFile(char *FileName)
{
   char c;
   int Pos;
   char *Buf;
   int Count;
   int InodeNum;
   void *TempData;
   unsigned char UpdateSB;
   struct inode *TempInode;

   UpdateSB = 1;
   Buf = NULL;
   if (NULL == FileName){
      PRINT("%s: Invalid Filename \n", __func__);
      return RET_ERR;
   }
   if (GetInode(FileName, &InodeNum) == 0) {
      PRINT("File name found and Inode = %d\n", InodeNum);
   } else {
      PRINT("Enter valid FileName\n");
      return RET_ERR;
   }

   for (Count = 0;(c = getchar()) != EOF; Count++) {
      Buf = (char*) realloc(Buf, Count + 1);
      Buf[Count] = c;
   }

   Pos = GetPosition(dbm);
   if (SetBit(dbm, Pos) == 0)
      PRINT("data bit is set successfuly\n");

   Count = sizeof(Buf) / BLK_SIZE;
   if (sizeof(Buf) % BLK_SIZE != 0)
      Count++;

   TempData = d_blks + (Pos * BLK_SIZE);
   memcpy(TempData, Buf, strlen(Buf) + 1);

   /* Update inode struct */
   TempInode = i_tab + InodeNum;
   TempInode->size = strlen(Buf) + 1 ;
   TempInode->blk_used = Count;

   for (c = 0;c < Count; c++)
      TempInode->blk[c] = Pos + c;
   /* Updating super_block after allocating data */
   UpdateSuperBlock(UpdateSB, Count);
   return RET_OK;
}

/**
 * @fn      static void InitFS();
 * @brief   This function to Intialize the FileSystem
 * @params  <None>
 *
 */
static void DisplaySBDetails()
{
   PRINT("sb->total_inodes = %d\n", sb->total_inodes);
   PRINT("sb->total_datablocks = %d\n", sb->total_datablocks);
   PRINT("sb->free_inodes  = %d\n", sb->free_inodes);
   PRINT("sb->free_datablocks = %d\n", sb->free_datablocks);
}

/**
 * @fn     static int PrintFileContents(char *FileName);
 * @brief  Function to Read the File Content
 * @param  FileName   Name of the File to read File Content
 * @return RET_OK on Success
 *         RET_ERR on Error
 */
static RetVal PrintFileContents(char *FileName)
{
   void *Data;
   int InodeNum;
   unsigned short Count;
   struct inode *TempInode;

   if (NULL == FileName){
      PRINT("%s:Invalid FileName \n", __func__);
      return RET_ERR;
   }
   if (GetInode(FileName, &InodeNum) == RET_ERR) {
      PRINT("FileName Not Found !!!\n");
      return RET_ERR;
   }
   PRINT("File name found and InodeNum = %d\n", InodeNum);
   TempInode = i_tab + InodeNum;
   for (Count = 0; Count < TempInode->blk_used; Count++){
      Data = d_blks + (TempInode->blk[Count] * BLK_SIZE);
      write(1, Data,BLK_SIZE);
   }
   return RET_OK;
}

/**
 * @fn     static int DisplayFileDetails(int Count);
 * @brief  Function to Display file Details
 * @param  Count  index for the file
 * @return RET_OK on Success
 *         RET_ERR on Error
 */
static int DisplayFileDetails(int Count)
{
   struct inode *TempInode ;

   if ( -1 >= Count){
      PRINT("%s: Invalid Count %d \n", __func__, Count);
      return RET_ERR;
   }
   TempInode = i_tab + Count;
   PRINT("file name = %s\n",TempInode->fname);
   PRINT("inode num = %d\n",TempInode->i_no);
   PRINT("file size = %d\n",TempInode->size);
   PRINT("blks used = %d\n",TempInode->blk_used);
   return RET_OK;
}

/**
 * @fn      static void ListFiles()
 * @brief   This function to Intialize the FileSystem
 * @params  <None>
 *
 */
static void ListFiles()
{
   int *Temp ;
   short int Count;

   Temp = (int *)ibm;
   for(Count = 0; Count < BLK_SIZE; Count++){
      if (((Count%32) == 0) && (Count != 0))
         Temp++;
      if ((*Temp)&(1<<Count)){
         DisplayFileDetails(Count);
      }
   }

}

/**
 * @fn     static int DeleteFile(char *FileName);
 * @brief  Function to Delete the File Contents
 * @param  FileName  Filename to be deleted
 * @return RET_OK on Success
 *         RET_ERR on Error
 */
static RetVal DeleteFile(char *FileName)
{
   int i;
   int InodeNum;
   struct inode *TempInode;

   InodeNum = 1;
   if ( NULL == FileName){
      PRINT("%s: Invalid FileName \n", __func__);
      return RET_ERR;
   }
   if (GetInode(FileName,&InodeNum) == RET_ERR){
      PRINT("Enter Vaild File name \n");
      return RET_ERR;
   }

   /* Get the Inode */
   TempInode = i_tab + InodeNum ;
   /* Clear All Data blocks bitmap */
   for (i = 0; i < TempInode->blk_used ; i++)
      ClearBit(dbm, TempInode->blk[i]);
   /* Clear inode bitmap */
   ClearBit(ibm, InodeNum);
   UpdateSuperBlock(2,TempInode->blk_used);
   memset(TempInode,'\0',sizeof(struct inode));
   return RET_OK;
}

/**
 * @fn     int main();
 * @brief  Entery Point of Filesystem simulator
 * @param  <none>
 * @return RET_OK on Success
 *         RET_ERR on Error
 */
int main()
{
   int Choice;
   char FileName[8];
   int count;

   InitFS();
   while(1){
      PRINT("\n#################Enter choice:\n");
      PRINT("1->Create\n");
      PRINT("2->Write\n");
      PRINT("3->Super\n");
      PRINT("4->Display\n");
      PRINT("5->List Files\n");
      PRINT("6->Delete File\n");
      scanf("%d", &Choice);

      switch (Choice)
      {
         case 1:
            PRINT("Enter the File Name:");
            scanf("%s",FileName);
            CreateFile(FileName);
            break;
         case 2:
            PRINT("To which file you want enter:");
            scanf("%s", FileName);
            WriteInToFile(FileName);
            break;
         case 3:
            DisplaySBDetails();
            break;
         case 4:
            PRINT("Enter the File Name:");
            scanf("%s", FileName);
            PrintFileContents(FileName);
            break;
         case 5:
            ListFiles();
            break;
         case 6:
            PRINT("Enter the File Name To Delete:");
            scanf("%s", FileName);
            DeleteFile(FileName);
            break;
         default:
            PRINT("Enter valid option\n");
            break;
      }
   }
   return 0;
}
