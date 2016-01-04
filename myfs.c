/********************************************************
  File name: myfs.c
  Description: linked file system
  Author: Weifeng Li 984558 weifli@my.bridgeport.edu
  Version: 1.0
  Date: 11/29/2015
  History:
**********************************************************/
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>  
#include <time.h>

#define BLOCK_NUMBER 40
#define DATA_BLOCK_NUMBER 28
#define BLOCK_SIZE 256
#define FILENAME_LENGTH 20
#define FILE_NUMBER 28
#define FLAG_BLOCK_NUMBER 3
#define FCB_BLOCK_NUMBER 8



int isalive = 0;/*if the information of the file system is already in memory*/
char pfsname[FILENAME_LENGTH];

/*super block, store the information of the filesystem*/
typedef struct supb{
	int full;
    int size;
    int filenum;
    int freeblock;
    char name[FILENAME_LENGTH];
} sb_t;
/*FCB of each file stored in the filesystem*/
typedef struct fcb {
	char not_free;
	int start;
	int end;
	int size;
	long timep;
	char name[FILENAME_LENGTH];
} fcb_t;

/*Datablock delegate in the memory*/
// typedef struct datablock{
// 	int nextblock;
// 	void* data;
// } db_t;


//block is free: next[i] == -2
//block points to another block: next[i] == n
//block contains end of file: next[i] == -1
/* the in-memory delegate of filesystem(metadata)*/
typedef struct mpfs {
    sb_t sb[1];
	fcb_t fb[FILE_NUMBER];
	int next[DATA_BLOCK_NUMBER];
} pfs_t;

pfs_t pfs;

/*init file system*/
int init_pfs(FILE *pfile, const char* pfs);

/*load file system information to memory*/
int load_pfs(FILE *pfile);

/*Allocate a new 10 KByte "PFS" file
 * if it does not already exist. 
 * If it does exist, begin using it for further commands.
*/
int open_pfs(const char* pfs);

/*
 * Copy the Windows (or Unix/Linux) file "myfile" into your PFS file.
*/
int put(const char* myfile);

/*
 *Extract file "myfile" from your PFS file and
 * copy it to the current Windows (or Unix/Linux) directory.
*/
int get(const char* myfile);

/*
 *Delete "myfile" from your PFS file.
*/
int rm(const char* myfile);

/*
*List the files in your PFS file.
*/
int dir();

/*
*Append remarks to the directory entry for myfile in your PFS file.
*/
int putr(const char* myfile, char* remark);

/*
*Delete the PFSfile from the Windows file system.
*/
int kill();

/*
*Exit PFS.
*/
int quit();

/*check if file already exists in the file system*/
int file_exist(const char* myfile);


int main(){
    do{
    	printf("PFS>");
        char sbuf[1024];
        char *args[20] = { NULL };/*user input command and arguments*/
        fgets(sbuf, 1024, stdin);
        sbuf[strlen(sbuf) - 1] = 0;
        //printf("%s", sbuf);
         if(strcmp(sbuf, " ") == 0 || strcmp(sbuf, "\0") == 0 || strcmp(sbuf, "\n") == 0)continue;
        if(strcmp(sbuf, "quit") == 0) break;
        if(strcmp(sbuf, "dir") == 0) {
        	dir();
        }else{
        		char* p;
        		int i = 0;
        		p = strtok(sbuf, " ");
			while (p != NULL) {
				args[i] = p;/*split user's input and store it in args*/
				i++;
				p = strtok(NULL, " ");	
			}
			if(strcmp(args[0], "open") != 0 && strcmp(args[0], "put") != 0 && strcmp(args[0], "get") != 0){
				printf("Error command.\n");
				continue;
			}else{
				if(i != 2){
					printf("Error arguments.\n");
					continue;
			}
			}
			
			if(strcmp(args[0], "open") == 0){
				open_pfs(args[1]);
			}
			if(strcmp(args[0], "put") == 0){
				put(args[1]);
			}
			if(strcmp(args[0], "get") == 0){
				get(args[1]);
			}
        }
        
        
    }while(1);
	return 0;
}

int init_pfs(FILE *pfile, const char* fname){
	char *buf;
	buf = (char*)malloc(BLOCK_SIZE);
	memset(buf, '\0', BLOCK_SIZE);
	for(int i = 0; i < BLOCK_NUMBER; i++) 
		fwrite(buf, BLOCK_SIZE, 1, pfile);

	//init in-memory object pfs
	pfs.sb[0].full = -1;
	pfs.sb[0].size = DATA_BLOCK_NUMBER;
	pfs.sb[0].filenum = 0;
	pfs.sb[0].freeblock = DATA_BLOCK_NUMBER;
	memset(pfs.sb[0].name, '\0', FILENAME_LENGTH);
	strcpy(pfs.sb[0].name, fname);
	fseek(pfile, 0, SEEK_SET);
	fwrite(pfs.sb, sizeof(pfs.sb[0]), 1, pfile);
	free(buf);
	for (int i = 0; i < sizeof(pfs.fb)/sizeof(pfs.fb[0]); i++) {
		pfs.fb[i].not_free = 0;
		memset(pfs.fb[i].name, '\0', FILENAME_LENGTH);
		pfs.fb[i].start = -1;
		pfs.fb[i].end = -1;
		pfs.fb[i].size = 0;
		pfs.fb[i].timep = 0;
	}
	for (int i = 0; i < DATA_BLOCK_NUMBER; i++) {
		pfs.next[i] = -2;
	}
	
	//allocate place for file descriptors
	buf = (char*)malloc(sizeof(fcb_t));
	memset(buf, '\0', sizeof(fcb_t));
	fseek(pfile, BLOCK_SIZE, SEEK_SET);
	for (int i = 0; i < FILE_NUMBER; i++) {
		fwrite(buf, sizeof(fcb_t), 1, pfile);
	}
	free(buf);
	//allocate place for precedence vector
	buf = (char*)malloc(sizeof(int));
	memset(buf, '\0', sizeof(int));
	for (int i = 0; i < DATA_BLOCK_NUMBER; i++) {
		fwrite(buf, sizeof(int), 1, pfile);
	}
	free(buf);
	fclose(pfile);
	return 0;
}
int load_pfs(FILE *pfile){
	fread(pfs.sb, BLOCK_SIZE, 1, pfile);
	//  printf("----in load_pfs()----\n");
// 	printf("-%d--%d--%d--%d--%s\n",pfs.sb[0].full,pfs.sb[0].size, pfs.sb[0].filenum, pfs.sb[0].freeblock, pfs.sb[0].name );
	if(pfs.sb[0].size != DATA_BLOCK_NUMBER ) return -1;
	fread(pfs.fb, sizeof(fcb_t), FILE_NUMBER, pfile);
	fseek(pfile,  (1 + FCB_BLOCK_NUMBER) * BLOCK_SIZE, SEEK_SET);
	fread(pfs.next, sizeof(int), DATA_BLOCK_NUMBER, pfile);
	for (int i = 0; i < DATA_BLOCK_NUMBER; i++)
		if (pfs.next[i] == 0)
			pfs.next[i] = -2;
	// for(int i = 0; i < FILE_NUMBER; i++){
// 		printf("-%d--%d--%d--%s\n",pfs.fb[i].not_free,pfs.fb[i].start, pfs.fb[i].end, pfs.fb[i].name);
// 	}
// 	for(int i = 0; i < DATA_BLOCK_NUMBER; i++){
// 		printf("--%d--\n",pfs.next[i]);
// 	}
	
	return 0;
}

int open_pfs(const char* fname){
	strcpy(pfsname, fname);
	FILE *pfile;
	pfile = fopen(fname, "r");
	if(pfile == NULL){
		pfile = fopen(fname, "w+");
		init_pfs(pfile, fname);
	}else{
		if (-1 == load_pfs(pfile)){
			printf("Please check the file system name.\n");
			return -1;
		}
	}
	isalive = 1;
	printf("The file system is ready.\n");
    fclose(pfile);
	return 0;
}

int file_exist(const char* myfile){
	for(int i = 0; i < FILE_NUMBER; i++){
		if(strcmp(pfs.fb[i].name, myfile) == 0)
			return i;
	}
	return -1;
}

int find_free_fcb_id() {
	int i = 0;
	for (i = 0; i < FILE_NUMBER; i++) {
		if (!pfs.fb[i].not_free)
			return i;
	}
	return 1002;
}


int find_free_block() {
	int i = 0;
	for (i = 0; i < DATA_BLOCK_NUMBER; i++) {
		if (pfs.next[i] == -2)
			return i;
	}
	return 1001;
}

fcb_t *get_fcb_by_id(int id) {
	return &pfs.fb[id];
}

int write_supb(sb_t *sb){
	FILE *pfile = fopen(pfsname, "r+");
	fwrite(sb, 1, sizeof(sb_t), pfile);
	fclose(pfile);
	return 0;
}
int write_fcb(fcb_t *fb, int fcbid) {
	FILE *pfile = fopen(pfsname, "r+");
	fseek(pfile, BLOCK_SIZE + fcbid * sizeof(fcb_t), SEEK_SET);
	fwrite(fb, 1, sizeof(fcb_t), pfile);
	fclose(pfile);
	return 0;
}

int write_freeblock_list(int id) {
	int* p;
	*p = pfs.next[id];
	FILE *pfile = fopen(pfsname, "r+");
	fseek(pfile, (1 + FCB_BLOCK_NUMBER) * BLOCK_SIZE + id * sizeof(int), SEEK_SET);
	fwrite(p, sizeof(int), 1, pfile);
	fclose(pfile);
	return 0;
}

int write_data(int blockid, int* next, void* buf, int size){
	printf("----%d====%d--\n", blockid, *next);
	if(size == 0) return 0;
	FILE *file = fopen(pfsname, "r+");
	fseek(file, (1 + 3 + 8) * BLOCK_SIZE + blockid * BLOCK_SIZE, SEEK_SET);
	fwrite(next, sizeof(int), 1, file);
	fwrite(buf, size, 1, file);
	fclose(file);
	return 0;
}

int read_data(int start, int* nextblockid, char** buf, int size){
	FILE *file = fopen(pfsname, "r+");
	fseek(file, (1 + 3 + 8) * BLOCK_SIZE + start * BLOCK_SIZE, SEEK_SET);
	fread(nextblockid, sizeof(int), 1, file);
	fread(*buf, size, 1, file);
	return 0;
}

int put(const char* myfile){
	if(isalive == 0) {
		printf("Please open file system first.\n");
		return -1;
	}
	if(file_exist(myfile) != -1){
		printf("file exists\n");
		return -1;
	}
	char *buf;
	buf = (char*)malloc(BLOCK_SIZE - sizeof(int));
	FILE *file;
	file = fopen(myfile, "r");
	int size = 0;
	if(file == NULL){
		printf("File %s does not exist.\n", myfile);
		return -1;
	}
	int fcb_id = find_free_fcb_id();
	fcb_t* fcb = get_fcb_by_id(fcb_id);
	pfs.fb[fcb_id].not_free = '1';
	strcpy(pfs.fb[fcb_id].name, myfile);
	 time_t long_time;
	 time( &long_time ); 
	pfs.fb[fcb_id].timep = long_time;
	int i = 0;
	while (!feof(file)) {
        int res = fread(buf, (BLOCK_SIZE - sizeof(int)), 1, file);
        int blockid = find_free_block();
		pfs.next[blockid] = -1;
		int nextblockid = -1;
        if(i == 0){
        	pfs.fb[fcb_id].start = blockid;
        }
        if(feof(file)){
        	size = ftell(file);
        	//write_data(blockid, &nextblockid, buf, BLOCK_SIZE - sizeof(int));
        	pfs.fb[fcb_id].end = blockid;
        	pfs.fb[fcb_id].size = size;
        }else{
            nextblockid = find_free_block();
        	if(nextblockid == 1001) {
        		printf("The file system is full.\n");
        		pfs.sb[0].full = 1;
        		return -1;
        	}
        	//write_data(blockid, &nextblockid, buf, BLOCK_SIZE - sizeof(int));
        	
        }
        //printf("int put(), nextblockid = %d/n", nextblockid);
        write_data(blockid, &nextblockid, buf, BLOCK_SIZE - sizeof(int));
        pfs.sb[0].freeblock--;
        free(buf);
        buf = (char*)malloc(BLOCK_SIZE - sizeof(int));
        memset(buf, '\0', BLOCK_SIZE - sizeof(int));
        write_freeblock_list(blockid);
        i++;
    }
    pfs.sb[0].filenum++;
//     printf("----in put()----\n");
//     printf("-%d--%d--%d--%d--%s\n",pfs.sb[0].full,pfs.sb[0].size, pfs.sb[0].filenum, pfs.sb[0].freeblock, pfs.sb[0].name );
//     for(int i = 0; i < FILE_NUMBER; i++){
// 		printf("-%d--%d--%d--%s\n",pfs.fb[i].not_free,pfs.fb[i].start, pfs.fb[i].end, pfs.fb[i].name);
// 	}
    write_fcb(&(pfs.fb[fcb_id]), fcb_id);
    write_supb(&(pfs.sb[0]));
    
    fclose(file);
	return 0;
}

int get(const char* myfile){
	if(isalive == 0) {
		printf("Please open file system first.\n");
		return -1;
	}
	int fcbid = file_exist(myfile);
	if(fcbid == -1){
		printf("file not exists.\n");
		return -1;
	}
	int start = pfs.fb[fcbid].start;
	int end = pfs.fb[fcbid].end;
	int size = pfs.fb[fcbid].size;
	int nextblockid = -1;
	int* p = &nextblockid;
	char *buf;
	buf = (char*)malloc(BLOCK_SIZE - sizeof(int));
	memset(buf, '\0', BLOCK_SIZE - sizeof(int));
	FILE *file;
	file = fopen(myfile, "w+");
	
	int j = size / (BLOCK_SIZE - sizeof(int));
	int left = size % (BLOCK_SIZE - sizeof(int));
	int readsize = BLOCK_SIZE - sizeof(int);
	do{
		if(j == 0){
			readsize = left;
			free(buf);
			buf = (char*)malloc(readsize);
			memset(buf, '\0', readsize);
		}
		read_data(start, p, &buf, readsize);
		start = *p;
		fwrite(buf, readsize, 1, file);
		free(buf);
		buf = (char*)malloc(readsize);
		memset(buf, '\0', readsize);
		j--;
		
	}while(start != -1);
	free(buf);
	fclose(file);
	return 0;

}
int gettime(char** stime, time_t long_time){
	struct tm *newtime;
    time( &long_time );   // - get time in long format
    //struct tm* pTmNow = localtime(&t);
    struct tm* pTmNow = localtime(&long_time);
 	*stime = asctime(pTmNow);
    //printf( " %s\n", stime);
    return 0;
}
int dir(){
	if(isalive == 0) {
		printf("Please open file system first.\n");
		return -1;
	}
	for(int i = 0; i < FILE_NUMBER; i++){
		char *stime;
		stime = (char*)malloc(20);
		memset(stime, '\0', 20);
		gettime(&stime, pfs.fb[i].timep);
		if(pfs.fb[i].not_free){
		    printf("%s\t%d bytes\t%s\n", pfs.fb[i].name, pfs.fb[i].size, stime);
		}
		//free(stime);
	}
	return 0;
}