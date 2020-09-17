#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct inode_struct{
	int size;
	int num_of_blocks;
	int block_pointer;
	char type;
} inode_struct;

typedef struct root_dir{
	char *filenames[100];
	int filesizes[100];
	int fileinodes[100];
	int num_of_files;
} root_dir;

//disk size is 256KB
const int block_size =4096;
const int inode_blocks=80;
const int data_blocks = 56;
const int inode_start_address= 12288;
const int data_start_block = 8;
const int inode_bitmap_block = 1;
const int data_bitmap_block = 2;
const int inode_size= 256;
const int root_inode = 2;

char INODE_BITMAP[4096];			
char DATA_BITMAP[4096];

int currid;
FILE *disk[10];
char *FS_names[10];

//FILE *disk;


void open_all(){
	for (int i = 0; i < currid; ++i){
		disk[i]=fopen(FS_names[i],"r+b");
	}
}

void close_all(){
	for (int i = 0; i < currid; ++i){
		fclose(disk[i]);
	}
}

int readData(int fsid,int blockNum,void *block){
	fseek(disk[fsid],block_size*blockNum,SEEK_SET);
	int i=0;
	int c = fread(block,block_size,1,disk[fsid]);
	
    return 0;
}

int writeData(int fsid,int blockNum,void *block){
	int t = fseek(disk[fsid],block_size*blockNum,SEEK_SET);
	
	int c =fwrite(block,block_size,1,disk[fsid]);
	
	return 0;
}

inode_struct* create_inode(char type,int size){
	inode_struct *new = (inode_struct*)malloc(sizeof(inode_struct));
	new->size=size;
	new->num_of_blocks = size/block_size + ((size%block_size)!=0);
	new->type = type;
	return new;
}

int write_inode(int fsid,inode_struct *ptr,int inum){
	char *temp = (char*)malloc(sizeof(char)*inode_size);
	temp = (char*)ptr;
	int t = inode_start_address + (inum*inode_size);
	t = fseek(disk[fsid],t,SEEK_SET);

	if(t!=0) return 1;
	fwrite(temp,inode_size,1,disk[fsid]);
	return 0;
}

char* read_inode(int fsid,int inum){
	int t=inode_start_address+(inum*inode_size);
	t=fseek(disk[fsid],t,SEEK_SET);
	if(t!=0) return NULL;
	char *temp = (char*)malloc(sizeof(char)*inode_size);
	fread(temp,inode_size,1,disk[fsid]);
	return temp;
}


int give_dataBlock(int inum){
	int i=0;
	while(i < 56){
		if(DATA_BITMAP[i]=='0'){
			DATA_BITMAP[i]='1';
			return i;
			//ptrs[count++]=i;
		}
		i++;
	}
	INODE_BITMAP[inum] = '0';
	return -1;
}

int give_inodeBlock(){
	//readData(inode_bitmap_block,INODE_BITMAP);
	for (int i = 3; i < 80; ++i){
		if(INODE_BITMAP[i]=='0'){
			INODE_BITMAP[i]='1';
			return i;
		}
	}
	return -1;
}

root_dir* create_dir(){
	root_dir *new = (root_dir*)malloc(sizeof(root_dir));
	new->num_of_files =0;
	
	return new;
}

int writeFile(int fsid,char *filename,void *block){	
	inode_struct *i = (inode_struct*)read_inode(fsid,root_inode);
	char data[block_size];
	readData(fsid,data_start_block+i->block_pointer,data);
	root_dir* rootdir = (root_dir*)data;
	char **this = rootdir->filenames;
	int j;
	for(j=0; j<rootdir->num_of_files; j++)
	{
		if(!strcmp(this[j],filename)){
			printf("\"%s\" file already exists.Please choose a different name.\n", this[j] );
			return 1;
		}
	}

	readData(fsid,inode_bitmap_block,INODE_BITMAP);
	readData(fsid,data_bitmap_block,DATA_BITMAP);
	char *arr = (char*) block;
	int id = give_inodeBlock();
	if(id==-1){
		printf("Cannot write more files.Not enough Inodes\n");
		return 1;
	}
	inode_struct *ptr = create_inode('f',strlen(arr));
		
	ptr->block_pointer = give_dataBlock(id);
	if(ptr->block_pointer==-1){
		printf("Cannot write file! Insufficient disk Space\n");
		return 1;
	}

	
	writeData(fsid,data_start_block+ptr->block_pointer,block);
	write_inode(fsid,ptr,id);

	inode_struct *root = (inode_struct*)read_inode(fsid,root_inode);
	char rootbuff[block_size];
	readData(fsid,data_start_block+root->block_pointer,rootbuff);
	rootdir =(root_dir*)rootbuff;
	// Updating the entry in Root directory
	rootdir->filenames[rootdir->num_of_files] = filename;
	rootdir->filesizes[rootdir->num_of_files] = ptr->size;
	rootdir->fileinodes[rootdir->num_of_files] = id;
	rootdir->num_of_files++;
	// Updating the size of Root inode
	root->size += ptr->size;
	writeData(fsid,data_start_block+root->block_pointer,(char*)rootdir);
	write_inode(fsid,root,root_inode);
	writeData(fsid,inode_bitmap_block,INODE_BITMAP);
	writeData(fsid,data_bitmap_block,DATA_BITMAP);
	printf("\n %s Successfully created and Data Written to it ",filename);
	return 0;
}

int readFile(int fsid,char *filename,void *block){

	inode_struct *root = (inode_struct*)read_inode(fsid,root_inode);
	char rootbuff[block_size];
	readData(fsid,data_start_block+root->block_pointer,rootbuff);
	root_dir *rootdir =(root_dir*)rootbuff;
	for (int i = 0; i < rootdir->num_of_files; ++i){
		if(!strcmp(rootdir->filenames[i],filename)){
			inode_struct *ptr=(inode_struct*)read_inode(fsid,rootdir->fileinodes[i]);
			
			readData(fsid,data_start_block+ptr->block_pointer,block);
			return 0;
		}
	}
	printf("Error reading file: \"%s\" file does not exist in specified file system\n",filename);
	return 1;
}

void createSFS(char *filename){

	for (int i = 0; i < currid; ++i){
		if(!strcmp(filename,FS_names[i])){
			printf(" \"%s\" is already a disk.\n", filename);
			return;
		}
	}
	FS_names[currid] = filename;
	disk[currid] = fopen(filename,"wb+");
	for (int i = 0; i < 56; ++i){
		DATA_BITMAP[i]='0';
	}
	for (int i = 0; i < 80; ++i){
		INODE_BITMAP[i]='0';
	}
	INODE_BITMAP[2]='1';
	
	root_dir *rootdir = create_dir();
	inode_struct *root = create_inode('d',sizeof(rootdir));
	root->block_pointer = give_dataBlock(root_inode);
	if(root->block_pointer==-1){
		printf("Error initializing disk %s\n",filename);
		return;
	}
	writeData(currid,data_start_block+root->block_pointer,(char*)rootdir);
	write_inode(currid,root,root_inode);
	writeData(currid,inode_bitmap_block,INODE_BITMAP);
	writeData(currid,data_bitmap_block,DATA_BITMAP);
	printf("\"%s\" initialized as file system with fsid %d\n",filename,currid );
	currid++;
	
}


void printFileList(int fsid){
	inode_struct *i = (inode_struct*)read_inode(fsid,root_inode);
	char data[block_size];
	readData(fsid,data_start_block+i->block_pointer,data);
	root_dir* rootdir = (root_dir*)data;
	char **this = rootdir->filenames;
	int j;
	for(j=0; j<rootdir->num_of_files; j++)
	{
		printf("\n %s ", this[j] );
	}
}


void printInodeBitmap(int fsid){
	int x=0;
	char imap[4096];
	readData(fsid,inode_bitmap_block,imap);
	printf("\n");
	for(x=0; x<80; x++)
	{
		printf("%c ", imap[x] );
	}
	printf("\n");
}

void printDataBitmap(int fsid){
	char dmap[4096];
	readData(fsid,data_bitmap_block,dmap);
	int x=0;
	printf("\n");
	for(x=0; x<data_blocks; x++)
	{
		printf("%c ", dmap[x] );
	}
	printf("\n");
}

int deleteFile(int fsid,char* filename){
	int inode_num;
	int block_num;
	int index;
	int file_size;
	int flag =0;
	readData(fsid,inode_bitmap_block,INODE_BITMAP);
        readData(fsid,data_bitmap_block,DATA_BITMAP);
	
	
	inode_struct *root = (inode_struct*)read_inode(fsid,root_inode);
        char rootbuff[block_size];                                                                                                                             
	 readData(fsid,data_start_block+root->block_pointer,rootbuff);
	root_dir* rootdir = (root_dir*)rootbuff;
        rootdir =(root_dir*)rootbuff;
	char **this = rootdir->filenames;
	// Finding the inode number of the file in the root_inode of the file system
	for(int i=0;i<rootdir->num_of_files;i++){
		if(strcmp(this[i],filename)==0){
			index=i;
			inode_num = rootdir->fileinodes[i];
			flag = 1;
			break;
		}
	}
	if(flag==0)
		return 0;
	
	inode_struct *file_inode = (inode_struct*)read_inode(fsid,inode_num);
	block_num = file_inode->block_pointer;
	file_size = file_inode->size;
	//Deleting the file from the root directory 
	for(int i=index;i<(rootdir->num_of_files-1);i++){
		strcpy(rootdir->filenames[i],rootdir->filenames[i+1]);
		rootdir->filesizes[i] = rootdir->filesizes[i+1];
		rootdir->fileinodes[i] = rootdir->fileinodes[i+1];
	}
	
	rootdir->num_of_files--;
	root->size-= file_size;
	
	//Deallocating Inode block of the file
	if(INODE_BITMAP[inode_num]=='1')
		INODE_BITMAP[inode_num]='0';
	//Deallocating Data Block of the file
	if(DATA_BITMAP[block_num]=='1')
		DATA_BITMAP[inode_num]='0';

	// Writing the modified Inode and Data blocks into the filesystem
	writeData(fsid,data_start_block+root->block_pointer,(char*)rootdir);
        write_inode(fsid,root,root_inode); 
	writeData(fsid,inode_bitmap_block,INODE_BITMAP);
	writeData(fsid,data_bitmap_block,DATA_BITMAP);                                                                                                                                                   
        
	return 1;
}



int main(){
	currid=0;
	char buff1[4096]="This is test data for file 1";
	char buff2[4096] = "This is test data for file 2";
	char read_data1[block_size];
	int success;

	createSFS("FileSystem_1");       //Creates a new File system with file system ID 0 
	printf("\n Creating a new File File1 and writing to it : ");
	writeFile(0,"File1",buff1);  //Creates a  new file named file1 in the fileSystem1 and writes into it the data in buffer 1 
 	
	readFile(0,"File1",read_data1);    //Reads data from file1 in the fileSystem1
	printf("\n Data Read from File1 : %s  ",read_data1);

	printf("\n Creating a File system with the same name as first :"); 
	createSFS("FileSystem_1");

	printf("\n Creating a file with the same name as first :");
	writeFile(0,"File1",buff1);

	// Deleting file1
	printf("\n Deleting File1"); 
	success = deleteFile(0,"File1");
	if(success = 1)
		printf("\n File1 successfully deleted");
	writeFile(0,"File1",buff1);
	printf("\n Creating a new File File2 and writing to it :");
	writeFile(0,"File2",buff2);

	printf("\n List of files in the FileSystem_1 ");
	printFileList(0);
	
	printf("\n Inode Bitmap of FileSystem_1 ");
	printInodeBitmap(0);

	printf("\n Data Bitmap of FileSystem_1 ");
	printDataBitmap(0);	
		
	close_all();
	return 0;
}
