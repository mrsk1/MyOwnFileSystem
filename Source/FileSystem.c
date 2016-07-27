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
 * 14-Jul-2016 Karthik M              Fixing Issues Related to Inode Full 
 										input validation
 *******************************************************************************/

# include <stdio.h>
# include <string.h>
# include <stdarg.h>
# include <stdint.h>
# include <stdlib.h>
# include <unistd.h>
# include <ctype.h>
# include "FileSystem.h"


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
 * @fn     static int IsInodeFull();
 * @brief  Function to chech the Inode is full or not
 * @return 0 on inodes is not full
 1 on inodes are full
 */
static int IsInodeFull()
{
	if (0 ==  sb->free_inodes)
		return 1;
	else
		return 0;
}

/**
 * @fn     static int GetPosition(void *block);
 * @brief  Function to  Get the Position of the Free bit in GroupDescriptor
 * @param  Block Pointer to the Block
 * @return position of free bit in the Block
 */
static int GetPosition(void *Block)
{
	int *Ptr = NULL;
	int Pos = 0;
	unsigned int BitCount = 0;

	Ptr = (int*)Block;
	while (Pos < BLK_SIZE) {
		if ((Pos != 0) && (Pos % 32 == 0)){
			BitCount = Pos%32;
			Ptr++;
		}
		if (*Ptr & (1 << BitCount)){
			Pos++;
			BitCount++;
		}
		else{
			return Pos;
		}
	}

	return RET_ERR;
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
	int *Ptr = NULL;
	int Move = -1;
	int SetPos = -1;

	if (NULL == Block){
		PRINT("%s:Invalid Block  \n", __func__);
		return RET_ERR;
	}
	if (-1 >= Pos){
		PRINT("%s:Invalid Positoin %d \n", __func__, Pos);
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
	int mov = -1;
	int *Ptr = NULL;
	int SetPos = -1;

	if (NULL == Block){
		PRINT("%s:Invalid Block \n", __func__);
		return RET_ERR;
	}
	if (-1 >= Pos){
		PRINT("%s:Invalid Positoin %d \n", __func__, Pos);
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
	strcpy((char *)TempInode->fname, FileName);
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
	int Pos = -1;
	va_list Ptr;

	if (-1 >= UpdateSB){
		PRINT("%s: Invalid UpdateSb = %d \n", __func__, UpdateSB);
		return RET_ERR;
	}
	if (0 == UpdateSB)
		sb->free_inodes = sb->free_inodes - 1;
	else if (1 == UpdateSB){
		/** update the Data blocks are used for file  creation*/
		va_start(Ptr, UpdateSB);
		Pos = va_arg(Ptr, int);
		//PRINT("UpdateSB = %d Pos = %d\n", UpdateSB, Pos);
		sb->free_datablocks = sb->free_datablocks - Pos;
	}else{
		/** update the Data blocks are used for file deletion*/
		va_start(Ptr,UpdateSB);
		/** reusing pos as no of used blocks */
		Pos = va_arg(Ptr, int);
		PRINT("File Found ");
		sb->free_datablocks = sb->free_datablocks + Pos ;
		sb->free_inodes = sb->free_inodes + 1;
	}
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
	int UsedInodes = -1;
	struct inode *TempInode;

	if (NULL == FileName){
		PRINT("%s: Invalid Filename\n", __func__);
		return RET_ERR;
	}
	if ( -1 >= *InodeNum){
		PRINT("%s: Invalid InodeNum %d\n", __func__, *InodeNum);
		return RET_ERR;
	}
	TempInode = i_tab;
	/* Get the inode, search in inode table for the corresponding filename */
	UsedInodes = INODES - sb->free_inodes;

	while (UsedInodes--){
		if (strcmp(FileName, (const char *)TempInode->fname) == 0){
			*InodeNum = TempInode->i_no;
			return RET_OK;
		}
		TempInode++;
	}
	return RET_ERR;
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
	int Pos = -1;
	int UpdateSB = 0;
	int InodeNum = 0;

	if (NULL == FileName){
		PRINT("%s Invalid FileName \n", __func__);
		return RET_ERR;
	}
	if (GetInode(FileName, &InodeNum) == 0) {
		PRINT("File Already Exists!!!!!\n");
		return RET_ERR;
	}
	if (IsInodeFull()){
		PRINT("Inodes are Full can't Create files anymore !!!\n");
		return RET_ERR;
	}
	Pos = GetPosition(ibm);
	PRINT("Pos = %d\n", Pos);
	if (Pos == BLK_SIZE){
		PRINT(" Inode is FULL !!! \n");
		return RET_ERR;
	}
	if (SetBit(ibm, Pos) == 0)
		PRINT("setbit success \n");
	FillingITable(Pos, FileName);
	UpdateSuperBlock(UpdateSB, Pos);
	PRINT("file Create Successfully \n");
	return RET_OK;
}

static RetVal WriteInToFile(char *FileName)
{
		char c;
		int Pos = -1;
		int i = -1;
		char Buf[BLK_SIZE] = {0};
		int Count = -1;
		int InodeNum = 0;
		void *TempData;
		unsigned char UpdateSB;
		short FillDataBlocks = 0;
		struct inode *TempInode;
		int SizeLeft = 0;

		UpdateSB = 1;
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

		/** find the inode */
		TempInode = i_tab + InodeNum;
		/** if inode already present update the datablocks used */
		if ( TempInode->size != 0){
				FillDataBlocks = TempInode->blk_used -1 ;
				if ( (TempInode -> size)% BLK_SIZE != 0){
						/** size is not aligned with the block size */
						SizeLeft = BLK_SIZE -(TempInode->size % BLK_SIZE) ;
						DB_PRINT("file size = %d\n",TempInode->size);
						DB_PRINT("file remaining size in datablocks = %d\n",SizeLeft);
				}
				DB_PRINT("file has already have some data \n");
		}
		/* TODO: Storing into file is not proper */
		for (Count = 0;(c = getchar()) != EOF; Count++) {
				/** TODO: update blk_used datablocks Index */
				if ( (0 != SizeLeft)&& (Count == (SizeLeft-1)) ){
						if (FillDataBlocks == 5){
								DB_PRINT(" Data block limits is over can't fill more data \n");
								return RET_ERR;
						}
						DB_PRINT("\n\n ***data Exceeds Remaining Datablocks DATABLOCK ** \n\n");
						TempInode->size += SizeLeft ;
						TempData = d_blks + (TempInode->blk[FillDataBlocks] * BLK_SIZE);
						DB_PRINT("\n ***data blk index = %d \n",TempInode->blk[FillDataBlocks]);
						memcpy(TempData +( TempInode ->size % BLK_SIZE), Buf, Count);
						/** update the datablocks count */
						Count = 0;
						memset(Buf,'\0',BLK_SIZE);
						FillDataBlocks++;
						SizeLeft = 0;

				}
				else if( Count == BLK_SIZE -1){
						/** Keep track of total datablocks count */
						if (FillDataBlocks == 5){
								DB_PRINT(" Data block limits is over can't fill more data \n");
								return RET_ERR;
						}
						DB_PRINT("\n\n ***data Exceeds one block Creating a another DATABLOCK ** \n\n");
						Pos = GetPosition(dbm);
						DB_PRINT(" pos of free data block insdie block full = %d \n",Pos);
						if (SetBit(dbm, Pos) == 0)
								PRINT("data writen into file successfuly\n");
						/** copy the Data blocks contents */
						TempData = d_blks + (Pos * BLK_SIZE);
						memcpy(TempData, Buf, Count);
						/** update the inode */
						TempInode->size += BLK_SIZE ;
						TempInode->blk_used = FillDataBlocks +1;
						TempInode->blk[FillDataBlocks] = Pos;
						/** update the datablocks count */
						Count = 0;
						memset(Buf,'\0',BLK_SIZE);
						FillDataBlocks++;
				}
				Buf[Count] = c;
		}
		if(c == EOF){
				Buf[Count] = '\0';
		}
		if ((c == EOF)&&(1 == Count)){ /** TODO: Count increses to 1 while Enter EOF ?? */
				PRINT("Warning: File Content Can't Be empty!!!\n");
				PRINT("No Content Written into file: %s\n",FileName);
				return RET_ERR;
		}
		if (FillDataBlocks == 5){
				DB_PRINT(" Data block limits is over can't fill more data \n");
				return RET_ERR;
		}
		/** TODO: handle the Full data blocks */
		if (0 != SizeLeft ){
				/** if the data is less than leftsize and file already has some data*/

				DB_PRINT("\n\n ***Remaining Datablocks is not Full writeing minimal data into file** \n\n");
				DB_PRINT("\n Count = %d \n",Count);
				DB_PRINT("\n index = %d \n",TempInode->blk[FillDataBlocks]);
				TempData = d_blks + (TempInode->blk[FillDataBlocks] * BLK_SIZE);
				memcpy(TempData +( TempInode ->size % BLK_SIZE), Buf, Count);
				TempInode->size += Count ;
				/** update the datablocks count */
				Count = 0;
				memset(Buf,'\0',BLK_SIZE);
//				FillDataBlocks++;
				SizeLeft = 0;

		}
		else {
				Pos = GetPosition(dbm);
				DB_PRINT(" pos of free data block  = %d \n",Pos);
				if (SetBit(dbm, Pos) == 0)
						PRINT("data writen into file successfuly\n");
				TempData = d_blks + (Pos * BLK_SIZE);
				memcpy(TempData, Buf, Count);
				TempInode->size += Count;
				TempInode->blk_used = FillDataBlocks +1;
				TempInode->blk[FillDataBlocks] = Pos;
		}
		DB_PRINT("fillblocks in updatesB =%d \n",FillDataBlocks +1 );
		UpdateSuperBlock(UpdateSB, FillDataBlocks + 1);
#define test
#ifdef test
		for (i = 0; i< FillDataBlocks; i++)
#endif
#if 0
				Count = sizeof(Buf) / BLK_SIZE;
		if (sizeof(Buf) % BLK_SIZE != 0)
				Count++;

		TempData = d_blks + (Pos * BLK_SIZE);
		memcpy(TempData, Buf, strlen(Buf) + 1);

		/* Update inode struct */
		TempInode = i_tab + InodeNum;
		TempInode->size = strlen(Buf) + 1 ;
		TempInode->blk_used = Count;

		for (i = 0;i < Count; i++)
				TempInode->blk[i] = Pos + i;
		/* Updating super_block after allocating data */
		UpdateSuperBlock(UpdateSB, Count);
		free(Buf);
#endif
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
	int InodeNum = 0;
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
	if (TempInode->blk_used == 0)
	{
		PRINT("File is EMPTY!!!!\n");
		return RET_OK;
	}
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
	PRINT("%s\n",TempInode->fname);
#if 0
	PRINT("file name = %s\n",TempInode->fname);
	PRINT("inode num = %d\n",TempInode->i_no);
	PRINT("file size = %d\n",TempInode->size);
	PRINT("blks used = %d\n",TempInode->blk_used);
#endif
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
	short int Count = -1;
	unsigned char foundfile = 0;

	Temp = (int *)ibm;
	for(Count = 0; Count < BLK_SIZE; Count++){
		if (((Count%32) == 0) && (Count != 0))
			Temp++;
		if ((*Temp)&(1<<Count)){
			DisplayFileDetails(Count);
			foundfile = 1;
		}
	}
	if (!foundfile){
		PRINT("No file to Display!!!\n");
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
	int i = -1;
	int InodeNum = -1;
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
 * @fn     static RetVal GetNumber(int *value)
 * @brief  Function to Validate Input option
 * @param  value  Return the  number 
 * @return RET_OK on Success
 *         RET_ERR on Error
 */
static RetVal GetNumber(int * Value)
{
	short count = 0;                                                               
	short num = 0;                                                                 
	char data[LINE_MAX] = {0};

	if(fgets(data, LINE_MAX, stdin) == 0){                                           
		PRINT("\nNo input found !!!\n");                                       
		return RET_ERR;
	}                                                                           

	data[strlen(data)-1] = '\0';                                                   
	count = strlen(data);                                                        

	unsigned short  nonum = 0;                                                     
	for( num = 0 ; num < count ; num++){                                           
		if ((isalpha(data[num]) != 0) || (iscntrl(data[num]) != 0)){               
			nonum = 1;                                                             
			break;                                                                 
		}                                                                          
	}                                                                              
	if (nonum)                                                                     
		return RET_ERR;                                                                 
	*Value = atoi(data);
	return RET_OK;      
}

/**
 * @fn     static RetVal GetFileName(char *File)
 * @brief  Function to Get the Valid file name from user
 * @param  File  Return the Valid filename
 * @return RET_OK on Success
 *         RET_ERR on Error
 */

static RetVal GetFileName(char *File)
{
	short count = 0;                                                               
	short num = 0;                                                                 
	char data[LINE_MAX] = {0};
	unsigned short  nofile = 0;                                                     

	if(fgets(data, LINE_MAX, stdin) == 0){                                           
		PRINT("\nNo input found !!!\n");                                       
		return RET_ERR;
	}                                                                           

	data[strlen(data)-1] = '\0';                                                   
	count = strlen(data);                                                        
	/** TODO:validate for File size */
	for( num = 0 ; num < count ; num++){                                           
		if ((iscntrl(data[num]) != 0)){               
			nofile = 1;                                                             
			break;                                                                 
		}                                                                          
	}                                                                              
	if (nofile)                                                                     
		return RET_ERR;                                                                 
	memcpy(File,data, count+1);
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
	int Choice  = -1;
	char FileName[8] = {0};

	InitFS();
	while(1){
		PRINT("\n#################\n");
		PRINT("1->Create\n");
		PRINT("2->Write\n");
		PRINT("3->Super\n");
		PRINT("4->Display\n");
		PRINT("5->List Files\n");
		PRINT("6->Delete File\n");
		PRINT("7->Exit Application\n");
		PRINT("###################\n");
		PRINT("Enter the choice:  ");

		if (GetNumber(&Choice) == RET_ERR){
			PRINT("\nPlease Enter Number\n");                                       
			continue ;
		}

		//#define DEBUG
		switch (Choice)
		{
			case 1:
#ifdef DEBUG
				printf(" 1 -> auto create 2 -> manual create \n");
				int mynum = 0;
				GetNumber(&mynum);
				if ( 1 == mynum){
					/** Testing the files Should Create more than 1024 */
					int temp = 0;
					char tempfile[8] = {0};
					for (temp= 0 ; temp < 1026 ; temp++){
						sprintf(tempfile,"%d",temp);
						PRINT(" file = %s \n",tempfile);
						CreateFile(tempfile);
					}
				}else {
					PRINT("Enter the File Name:");
					GetFileName(FileName);
					CreateFile(FileName);
				}
#else
				PRINT("Enter the File Name:");
				if (GetFileName(FileName) == RET_ERR){
					PRINT(" Invalid File Name \n");
					continue ;
				}
				CreateFile(FileName);
#endif
				break;

			case 2:
				PRINT("To which file you want enter:");
				if (GetFileName(FileName) == RET_ERR){
					PRINT(" Invalid File Name \n");
					continue ;
				}
				WriteInToFile(FileName);
				break;
			case 3:
				DisplaySBDetails();
				break;
			case 4:
				PRINT("Enter the File Name:");
				if (GetFileName(FileName) == RET_ERR){
					PRINT(" Invalid File Name \n");
					continue ;
				}
				PrintFileContents(FileName);
				break;
			case 5:
				ListFiles();
				break;
			case 6:
				PRINT("Enter the File Name To Delete:");
				if (GetFileName(FileName) == RET_ERR){
					PRINT(" Invalid File Name \n");
					continue ;
				}
				if (DeleteFile(FileName) == RET_OK)
					PRINT("File deleted Succesfully \n");
				break;
			case 7:
				free(fs);
				exit(0);
			default:
				PRINT("\nEnter valid option\n");
				break;
		}
	}
	return 0;
}
