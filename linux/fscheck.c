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
void update_datablock_bitmap_per_inode_pointers(
	uint datablock_num, struct dinode* inode_p, short* datablock_bitmap_per_inode_pointers);
void check_datablock_address(uint address, short* datablock_bitmap_per_inode_pointers );
void get_dirent(struct dinode* parent, struct dirent* out_dirent, uint dirent_num);

int fsfd; 
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
	
	// initialize superblock
	rsect(1, (void*) temp_buf);
	memmove( &sb, temp_buf, sizeof(sb) ); // keeps rsect from failing (as opposed to rsect(1, (void*) &sb);
	
	// setup comparison structures based on superblock
	short* datablock_bitmap_per_inode_pointers = (short*) malloc( sb.nblocks * sizeof(short) );
	memset(datablock_bitmap_per_inode_pointers, 0, sb.nblocks); // intialize to 0
	memset(datablock_bitmap_per_inode_pointers, 1, (sb.size - sb.nblocks) ); // metablocks are in use
	
	uint* inode_ref_counts_per_dirents = (uint*) malloc(sb.ninodes* sizeof(uint));
	memset(inode_ref_counts_per_dirents, 0, sb.ninodes);// initalize to 0
	
	
	// root directory 
	struct dinode first_inode;
	rinode(1, &first_inode);
	if (first_inode.type != T_DIR){
		printf("ERROR: root directory does not exist\n");
		exit(1);
	}
	
	// go through inodes
	int inum;
	for( inum=1; inum<sb.ninodes; inum++){ // start at 1 b/c inode 0 is not a thing
		struct dinode inode;
		rinode(inum, &inode);
		 
		// maybe exit early
		short type = inode.type;
		if ( !inode_is_valid_or_unalloc(type) ){
			printf("ERROR: bad inode\n"); 
			exit(1);  
		}
		if (type == 0 ){
			continue;
		}
		
		// for all allocated types: check datablocks
		// direct blocks
		uint* addrs = inode.addrs;
		int j;
		for (j=0; j < (NDIRECT + 1); j++){
			check_datablock_address( addrs[j], datablock_bitmap_per_inode_pointers );
		}
		// indirect blocks
		uint indirect_block_num = (uint) addrs[NDIRECT]; // block number, not pointer
		uint indir_block[BSIZE]; // char = 8 bits, uint = 4 bits
		rsect(indirect_block_num, indir_block);
		for(j=0; j < NINDIRECT; j++){
			check_datablock_address( indir_block[j], datablock_bitmap_per_inode_pointers );
		}
		
		// handle directories
		if( type == T_DIR ){
			
			char buf[BSIZE]; 			// could call get dirent 0, 1
			rsect(inode.addrs[0], buf);
			struct dirent* buf2 = (struct dirent*) buf;
			if ( strcmp(buf2[0].name, ".") || strcmp(buf2[1].name, "..") ){
				printf("ERROR: directory not properly formatted.\n");
			}
			
			uint inode_size = inode.size; // only look at as many dirents as size dictates
			uint num_of_dir_entries = inode_size / sizeof(struct dirent); // int division is fine, since you can't store part of a dirent
			uint dirent_num;
			for (dirent_num=0; dirent_num < num_of_dir_entries; dirent_num++){ // for each directory entry
				// get reference counts
				struct dirent entry;
				get_dirent(&inode, &entry, dirent_num);
				inode_ref_counts_per_dirents[ entry.inum ]++; // inode_ref_counts_per_dirents[0] gets a lot of increments, but there is no inode number 0 
				
				// if they are directories too, make sure their .. goes back to this dir
				struct dinode sub_inode; // inode pointed to by the directory in the current inode
				rinode(entry.inum, &sub_inode);
				if (sub_inode.type == T_DIR){
					struct dirent dot_dot_entry;
					get_dirent(&sub_inode, &dot_dot_entry, 1);
					if (dot_dot_entry.inum != inum){
						printf("ERROR: parent directory mismatch.\n");
						exit(1);
					}
				}
			}
		} // endif type == T_DIR
	} // end of going through inodes 1st time
	
	//~ printf("ref counts\n");
	//~ for (i=0; i<sb.ninodes; i++){
		//~ printf("%d\t", inode_ref_counts_per_dirents[i]);
	//~ }
	
	// go through inodes again
	// to use the inode ref counts we got last time 
	for( inum=1; inum<sb.ninodes; inum++){ // start at 1 b/c inode 0 is not a thing
		struct dinode inode;
		rinode(inum, &inode);
		if ( inode.type != 0 && inode_ref_counts_per_dirents[inum] == 0 ){
			printf("ERROR: inode marked use but not found in a directory.\n");
			exit(1);
		}
		if( inode_ref_counts_per_dirents[inum] != 0 && inode.type == 0 ){
			printf("ERROR: inode referred to in directory but marked free\n");
			exit(1);
		}
		if ( inode_ref_counts_per_dirents[inum] != inode.nlink && inum!=1){ // inum!=1 b/c the root node gets an incorrect ref count b/c its . and .. are stored in the same inode
			printf("%d, %d, %d\n", inode_ref_counts_per_dirents[inum], inode.nlink, inum );
			printf("ERROR: bad reference count for file.\n");
			exit(1);
		}
		if (inode.type == T_DIR && inode_ref_counts_per_dirents[inum] != 1 && inum!=1){ // inum!=1 for same reason as above
			printf("%d, %d, %d\n", inode_ref_counts_per_dirents[inum], inode.nlink, inum );
			printf("ERROR: directory appears more than once in file system.\n");
			exit(1);
		}
	}
	
	// todo: use datablock stuff to check xv6's bitmap
	//~ For in-use inodes, each address in use is also marked in use in the
	//~ bitmap. ERROR: address used by inode but marked free in bitmap.
	//~ For blocks marked in-use in bitmap, actually is in-use in an inode or
	//~ indirect block somewhere. ERROR: bitmap marks block in use but it
	//~ is not in use.
	
	//~ uint num_data_blocks = sb.nblocks;
	//~ int i;
	//~ for (i=0; i<num_data_blocks; i++){
		//~ 
	//~ }
    
    close(fsfd);
	exit(0);
	return 0;
}

// dirent num relative to the other directory entries in that directory
void get_dirent(struct dinode* parent, struct dirent* out_dirent, uint dirent_num){
	// go to that dirent
	uint num_entries_per_block = BSIZE/sizeof(struct dirent);
	uint addrs_index = dirent_num/(num_entries_per_block);
	char buf[BSIZE];
	
	rsect(parent->addrs[addrs_index], buf);
	
	uint offset = dirent_num % num_entries_per_block;
	struct dirent* buf2 = (struct dirent*) buf;
	*out_dirent = buf2[offset];
	//~ printf("%s %d; ", buf2[offset].name, buf2[offset].inum); // . .. README cat echo forktest grep init kill ln ls mkdir rm sh stressfs usertests wc zombie test  
}

void check_datablock_address(uint address, short* datablock_bitmap_per_inode_pointers ){
	if (address == 0) return; // 0 just means there is no datablock
	// check address in range
	if ( !datablock_address_is_valid(address, &sb) ){
		printf("ERROR: bad address in inode\n");
		exit(1);
	}
	
	// check address not used before (this part also stores info for bitmap checks)
	if (datablock_bitmap_per_inode_pointers[ address ]){ // if it's already set
		printf("%d\n", address);
		printf("ERROR: address used more than once.\n");
		exit(1);
	} else { // set it
		datablock_bitmap_per_inode_pointers[ address ]++;
	}
}

// "valid" in the sense of "in range"
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
