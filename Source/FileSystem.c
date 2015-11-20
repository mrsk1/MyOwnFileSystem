# include <stdio.h>
# include <string.h>
# include "FileSystem.h"

void *fs;					// file system 
void *dbm;
void *ibm;
struct super_block *sb;		// super block 
struct inode *i_tab;		// table of inode structure
void *d_blks;


static void InitFS() 
{
	if (posix_memalign(&fs, ALIGN, MB))
    {
        perror("posix_memalign failed");
    }

	sb 	= (struct super_block *) fs; 
	dbm = (void *) sb + BLK_SIZE;
	ibm = dbm + BLK_SIZE;
	i_tab  = ibm + BLK_SIZE;
	d_blks = (void *) i_tab + (INODES * sizeof(struct inode));

    printf("sizeof(struct inode)  =    %d\n", (int) sizeof(struct inode));

    printf("fs  = 0x%08X\n", (unsigned long)fs);
    printf("sb  = 0x%08X\n", (unsigned int)sb);
    printf("dbm = 0x%08X\n", (unsigned int)dbm);
    printf("ibm = 0x%08X\n", (unsigned int)ibm);
    printf("i_tab =  0x%08X\n", (unsigned int)i_tab);
    printf("d_blks = 0x%08X\n", (unsigned int)d_blks);
	
	sb->total_inodes = INODES;		// Each inode represents one inode structure in inode table
	sb->total_datablocks = T_BLKS;	// We have 1MB of total memory and blk size is 1KB so will have 1024 blocks
	sb->free_inodes = INODES;
	sb->free_datablocks = T_BLKS - (3 + (INODES / (BLK_SIZE / sizeof(struct inode))));
	
	printf("sb->total_inodes = %d\n", sb->total_inodes);
    printf("sb->total_datablocks = %d\n", sb->total_datablocks);
    printf("sb->free_inodes	 = %d\n", sb->free_inodes);
    printf("sb->free_datablocks = %d\n", sb->free_datablocks);

	dbm = memset(dbm, 0, 128);
	
	printf ("dbm = %d\n",*(int*)dbm);

	ibm = memset(ibm, 0, 128);	

	printf ("ibm = %d\n",*(int*)ibm);
}
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
static int SetBit(void *block, int pos)
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
		return 0;
	return -1;
}
static int Filling_ITable(int index, char *file_name)
{
	struct inode *temp_inode;
	temp_inode = i_tab + index;
	temp_inode->i_no = index;
	strcpy(temp_inode->fname, file_name);
	printf("File name = %s\n", temp_inode->fname);
	
}
static int UpdateSuperBlock()
{
    sb->free_inodes = sb->free_inodes - 1;
}
static int CreateFile(char *file_name)
{
	int pos;
	pos = GetPosition(ibm);
	printf ("pos = %d\n", pos);
	if (SetBit(ibm, pos) == 0)
		printf ("bit is set successfuly\n");
	Filling_ITable(pos, file_name);
	UpdateSuperBlock();
}
static int GetInode(char *file_name, int *inode)
{
	int used_inodes;
	struct inode *temp_inode;
	temp_inode = i_tab;

	//Get the inode, search in inode table for the corresponding filen
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
	char buf[80];
	char c;
	int i;
	int pos;
	struct inode *temp_inode;
	void *temp_data;
	

	if (GetInode(file_name, &inode) == 0) {
		printf("File name found and inode = %d\n", inode);
	} else {
		printf ("Enter valid file_name\n");
		return -1;
	}
	for (i = 0;(c = getchar()) != EOF; i++) {
		buf[i] = c;
	}
	buf[i] = '\0';
	
	printf("buf = %s\n", buf);

	pos = GetPosition(dbm);	
	printf("ps = %d\n", pos);

	if (SetBit(dbm, pos) == 0)
		printf("data bit is set successfuly\n");
	/* Update inode struct */
		
	temp_inode = i_tab + inode;
	printf("temp_inode->i_no = %d\n", temp_inode->i_no);
	temp_inode->size = strlen(buf) + 1;
	temp_inode->blk_used = 1;
	temp_inode->blk[0] = pos;

		
	temp_data = d_blks + pos;
	memcpy(temp_data, buf, sizeof(buf));
	printf("%s\n", (char*)temp_data);

	
	
}

static int SuperBlockDetails()
{
	printf("sb->total_inodes = %d\n", sb->total_inodes);
	printf("sb->total_datablocks = %d\n", sb->total_datablocks);
	printf("sb->free_inodes  = %d\n", sb->free_inodes);
	printf("sb->free_datablocks = %d\n", sb->free_datablocks);

}
int main()
{
	int choice;
	char file_name[8];
	int count;
	InitFS();
	
	while(1){
		printf("Enter choice:");
		printf("1->Create\n");
		printf("2->Write\n");
		printf("3->Super\n");
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
			default:
				printf("Enter valid option\n");
				break;
		}
	}
	return 0;
}
