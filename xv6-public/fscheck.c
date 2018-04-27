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
void update_inode_ref_counts_per_dirents(uint dirent_num, struct dinode* inode_p, uint* inode_ref_counts_per_dirents);
	// based on the directory entries in the inodes

int fsfd; 
struct superblock sb;
char temp_buf[BSIZE];


int
main(int argc, char *argv[]){
	uint* inode_ref_counts_per_dirents;
	
	// check args
	if(argc < 2){
		fprintf(stderr, "Usage: fscheck fs.img \n");
		exit(1);
	}
	
	// open file
    fsfd = open(argv[1], O_RDONLY);
	assert(fsfd != -1);
	
	// initialize superblock
	rsect(1, (void*) temp_buf); // 1 is block number 
	memmove( &sb, temp_buf, sizeof(sb) ); // keeps rsect from failing (as opposed to rsect(1, (void*) &sb);
	//~ uint block_num_of_first_inode = sb.inodestart;
	//~ uint block_num_just_after_inodes = sb.bmapstart;
	
	// setup checking structures
	inode_ref_counts_per_dirents = (uint*) malloc(sb.ninodes* sizeof(uint));
	
	// root directory 
	struct dinode first_inode;
	rinode(1, &first_inode);
	if (first_inode.type != T_DIR){
		printf("ERROR: root directory does not exist\n");
		exit(1);
	}
	
	int i;
	for( i=1; i<sb.ninodes; i++){ // start at 1 b/c inode 0 is not a thing
		struct dinode inode;
		rinode(i, &inode);
		 
		short type = inode.type;
		if ( !inode_is_valid_or_unalloc(type) ){
			printf("ERROR: bad inode\n"); 
			exit(1);  
		} 
		if (type == 0 ){
			continue;
		}
		//~ switch(type){
			//~ case T_FILE:
				//~ printf("file\t");
				//~ break;
			//~ case T_DIR:
				//~ printf("dir\t");
				//~ break;
			//~ case T_DEV:
				//~ printf("dev\t");
				//~ break;
			//~ case 0:
				//~ printf("unall\t");
		//~ }
		
		// check data block addresses are valid
		// direct blocks
		uint* addrs = inode.addrs;
		int j;
		for (j=0; j < (NDIRECT + 1); j++){
			if ( !datablock_address_is_valid(addrs[j], &sb) ){
				printf("ERROR: bad address in inode\n");
				exit(1);
			}
			//~ printf("data block num %d\t", addrs[j]);
		}
		// indirect blocks
		uint indirect_block_num = (uint) addrs[NDIRECT]; // block number, not pointer
		uint indir_block[BSIZE]; // char = 8 bits, uint = 4 bits
		rsect(indirect_block_num, indir_block);
		for(j=0; j < NINDIRECT; j++){
			if ( !datablock_address_is_valid( indir_block[j], &sb) ){
				printf("ERROR: bad address in inode\n");
				exit(1);
			}
		}
		/// end section: data blocks
			
		if( type == T_DIR ){
			
			// only look at as many dirents as size dictates
			uint inode_size = inode.size;
			uint num_of_dir_entries = inode_size / sizeof(struct dirent); // int division is fine, since you can't store part of a dirent
			uint dirent_num;
			for (dirent_num=0; dirent_num < num_of_dir_entries; dirent_num++){
				update_inode_ref_counts_per_dirents(dirent_num, &inode, inode_ref_counts_per_dirents);
			}
		} // endif type == T_DIR
	}
	
	//~ printf("ref counts\n");
	//~ for (i=0; i<sb.ninodes; i++){
		//~ printf("%d\t", inode_ref_counts_per_dirents[i]);
	//~ }
	
	// go through inodes again
	// to use the inode ref counts we got last time 
	for( i=1; i<sb.ninodes; i++){ // start at 1 b/c inode 0 is not a thing
		struct dinode inode;
		rinode(i, &inode);
		if ( inode.type != 0 && inode_ref_counts_per_dirents[i] == 0 ){
			printf("ERROR: inode marked use but not found in a directory.\n");
			exit(1);
		}
		if( inode_ref_counts_per_dirents[i] != 0 && inode.type == 0 ){
			printf("ERROR: inode referred to in directory but marked free\n");
			exit(1);
		}
		if ( inode_ref_counts_per_dirents[i] != inode.nlink && i!=1){ // i!=1 b/c the root node gets an incorrect ref count b/c its . and .. are stored in the same inode
			printf("%d, %d, %d\n", inode_ref_counts_per_dirents[i], inode.nlink, i );
			printf("ERROR: bad reference count for file.\n");
			exit(1);
		}
		if (inode.type == T_DIR && inode_ref_counts_per_dirents[i] != 1 && i!=1){ // i!=1 for same reason as above
			printf("%d, %d, %d\n", inode_ref_counts_per_dirents[i], inode.nlink, i );
			printf("ERROR: directory appears more than once in file system.\n");
			exit(1);
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
    
    close(fsfd);

	exit(0);
	return 0;

    //~ void* mmappedData = mmap(NULL, filesize, PROT_READ, MAP_PRIVATE | MAP_POPULATE, fd, 0);
    //~ assert(mmappedData != MAP_FAILED);
    //Write the mmapped data to stdout (= FD #1)
    //~ write(1, mmappedData, 1000); // prints something, which is a good sign
    
}

void update_inode_ref_counts_per_dirents(uint dirent_num, struct dinode* inode_p, uint* inode_ref_counts_per_dirents){
	// go to that dirent
	uint num_entries_per_block = BSIZE/sizeof(struct dirent);
	uint addrs_index = dirent_num/(num_entries_per_block);
	char buf[BSIZE];
	
	rsect(inode_p->addrs[addrs_index], buf);
	
	uint offset = dirent_num % num_entries_per_block;
	struct dirent* buf2 = (struct dirent*) buf;
	//~ printf("%s %d; ", buf2[offset].name, buf2[offset].inum); // . .. README cat echo forktest grep init kill ln ls mkdir rm sh stressfs usertests wc zombie test  
	// see which inode num it is pointing to
	// ++ the ref count for that inode
	inode_ref_counts_per_dirents[ buf2[offset].inum ]++; // inode_ref_counts_per_dirents[0] gets a lot of increments, but there is no inode number 0 
}


// "address" in the sense of block number, as opposed to a byte number
int datablock_address_is_valid(uint addr, struct superblock* sb){
	uint range_start = sb->bmapstart+1; // not byte number, block number
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
  //~ printf("inode block num: %d\n", bn);
  rsect(bn, buf);
  dip = ((struct dinode*)buf) + (inum % IPB);
  *ip = *dip;
}
