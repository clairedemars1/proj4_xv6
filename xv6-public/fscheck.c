//section: from mkfs.c
#include <stdio.h> // it's ok to use these sort of standard functions, since mkfs uses them too 
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>

#define stat xv6_stat  // avoid clash with host struct stat
#include "types.h"
#include "fs.h"
#include "stat.h"
#include "file_types.h"
#include "param.h"

#ifndef static_assert
#define static_assert(a, b) do { switch (0) case 0: case (a): ; } while (0)
#endif

#define NINODES 200


// Disk layout:
// [ boot block | sb block | log | inode blocks | free bit map | data blocks ]

int nbitmap = FSSIZE/(BSIZE*8) + 1;
int ninodeblocks = NINODES / IPB + 1;
int nlog = LOGSIZE;
int nmeta;    // Number of meta blocks (boot, sb, nlog, inode, bitmap)
int nblocks;  // Number of data blocks

int fsfd;
struct superblock sb;
char zeroes[BSIZE];
uint freeinode = 1;
uint freeblock;

//~ void balloc(int);
//~ void wsect(uint, void*);
//~ void winode(uint, struct dinode*);
void rinode(uint inum, struct dinode *ip);
void rsect(uint sec, void *buf);
//~ uint ialloc(ushort type);
//~ void iappend(uint inum, void *p, int n);

// end section

int inode_is_valid_or_unalloc(short type);
int datablock_address_is_valid(uint addr, struct superblock* sb);
int fsfd; // stupid global variable
struct superblock sb;
char temp_buf[BSIZE];

int
main(int argc, char *argv[]){
	
	// check args
	if(argc < 2){
		fprintf(stderr, "Usage: fscheck fs.img \n");
		exit(1);
	}
	
	// open file
    fsfd = open(argv[1], O_RDONLY);
	assert(fsfd != -1);
	rsect(1, (void*) &temp_buf); // 1 is block number 
	memmove( &sb, &temp_buf, sizeof(sb) ); // keeps rsect from failing (as opposed to rsect(1, (void*) &sb);
	uint block_num_of_first_inode = sb.inodestart;
	uint block_num_just_after_inodes = sb.bmapstart;
	uint i;
	
	struct dinode first_inode;
	rinode(1, &first_inode);
	if (first_inode.type != T_DIR){
		// Root directory exists, and it is inode number 1.
		printf("ERROR: root directory does not exist\n");
		exit(1);
	}
	
	// for each block of inodes
	for(i=block_num_of_first_inode; i< block_num_just_after_inodes; i++){ 
		char buf[BSIZE];
		rsect(i, (void*) &buf);
		struct dinode* inode_p = (struct dinode *) &buf;
		//~ for( ; inode_p < (inode_p + IPB); inode_p++){  // pointer arithmetic fails
		short counter;
		// for each inode
		for(counter=0; counter < IPB; counter++){ 
			short type = inode_p->type;
			if ( !inode_is_valid_or_unalloc(type) ){
				printf("ERROR: bad inode\n"); 
				exit(1);  
			} else if ( type != 0 ) { // inode is valid and allocated
				if( type == T_DIR ){
					struct dirent* dentry = (struct dirent* ) inode_p;
					int j;
					// do i have to go to each data block and go through all it's dirents?
				}
				
				// check data block addresses are valid
				uint* addrs = inode_p->addrs;
				int j;
				for (j=0; j < (NDIRECT + 1); j++){
					//~ printf("%d\t", addrs[j]);
					if ( !datablock_address_is_valid(addrs[j], &sb) ){
						printf("ERROR: bad address in inode");
						exit(1);
					}
				}
				uint* indirect_block = (uint*) addrs[NDIRECT]; // pointer to a pseudo-pointer
				int k;
				for(k=0; k < NINDIRECT; k++){
						//~ printf("%d\t", k);
						
						//~ printf("%d\t", indirect_block[k]); // why seg fault
						
					//~ if ( !datablock_address_is_valid( indirect_block[k], &sb) ){
						//~ printf("ERROR: bad address in inode");
						//~ exit(1);
					//~ }
				}
			}
			
			// increment pointer
			inode_p++;
		}	
	}

	//~ Each directory contains . and .. entries. ERROR: directory not
	//~ properly formatted.
	//~ Each .. entry in directory refers to the proper parent inode, and parent
	//~ inode points back to it. ERROR: parent directory mismatch.
	//~ For in-use inodes, each address in use is also marked in use in the
	//~ bitmap. ERROR: address used by inode but marked free in bitmap.
	//~ For blocks marked in-use in bitmap, actually is in-use in an inode or
	//~ indirect block somewhere. ERROR: bitmap marks block in use but it
	//~ is not in use.
	//~ For in-use inodes, any address in use is only used once. ERROR:
	//~ address used more than once.
	//~ For inodes marked used in inode table, must be referred to in at least
	//~ one directory. ERROR: inode marked use but not found in a
	//~ directory.
	//~ For inode numbers referred to in a valid directory, actually marked in
	//~ use in inode table. ERROR: inode referred to in directory but
	//~ marked free.
	//~ Reference counts (number of links) for regular files match the number
	//~ of times file is referred to in directories (i.e., hard links work correctly).
	//~ ERROR: bad reference count for file.
	//~ No extra links allowed for directories (each directory only appears in
	//~ one other directory). ERROR: directory appears more than once in
	//~ file system.
    
    close(fsfd);

	exit(0);
	return 0;

    //~ void* mmappedData = mmap(NULL, filesize, PROT_READ, MAP_PRIVATE | MAP_POPULATE, fd, 0);
    //~ assert(mmappedData != MAP_FAILED);
    //Write the mmapped data to stdout (= FD #1)
    //~ write(1, mmappedData, 1000); // prints something, which is a good sign
    
}

int datablock_address_is_valid(uint addr, struct superblock* sb){
	uint range_start = sb->bmapstart+1; // byte number (ie address)
	uint range_end = sb->size - 1;
				
	return addr == 0 || ( addr >= range_start && addr <= range_end ); 
}

int inode_is_valid_or_unalloc(short type){
	return (type == T_FILE || type == T_DIR || type == T_DEV || type == 0);
}
// sec=sector number, buf=to
void
rsect(uint sec, void *buf)
{
  if(lseek(fsfd, sec * BSIZE, 0) != sec * BSIZE){
    perror("lseek");
    exit(1);
  }
  if(read(fsfd, buf, BSIZE) != BSIZE){
    perror("read");
    exit(1);
  }
}

// global vars: sb
// inum = number of the inode, ip is pointer a struct dinode to put it in
void
rinode(uint inum, struct dinode *ip)
{
  char buf[BSIZE];
  uint bn;
  struct dinode *dip;

  bn = IBLOCK(inum, sb);
  printf("inode block num: %d\n", bn);
  rsect(bn, buf);
  dip = ((struct dinode*)buf) + (inum % IPB);
  *ip = *dip;
}
