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
//~ void rinode(uint inum, struct dinode *ip);
void rsect(uint sec, void *buf);
//~ uint ialloc(ushort type);
//~ void iappend(uint inum, void *p, int n);


// end section

//~ // section: for mmap, from https://techoverflow.net/2013/08/21/a-simple-mmap-readonly-example/
//~ #include <sys/mman.h>
//~ #include <sys/types.h>
//~ #include <fcntl.h>
//~ #include <unistd.h>
//~ #include <sys/stat.h>
//~ #include <assert.h>
//~ /**
 //~ * Get the size of a file.
 //~ * @return The filesize, or 0 if the file does not exist.
 //~ */
//~ size_t getFilesize(const char* filename) {
    //~ struct stat st;
    //~ if(stat(filename, &st) != 0) {
        //~ return 0;
    //~ }
    //~ return st.st_size;   
//~ }
//~ // end section

int fsfd; // stupid global variable

int
main(int argc, char *argv[]){
	
	// check args
	if(argc < 2){
		fprintf(stderr, "Usage: fscheck fs.img \n");
		exit(1);
	}
	
	// open file
    //~ size_t filesize = getFilesize(argv[1]);
    //~ int fd = open(argv[1], O_RDONLY, 0);
    //~ assert(fd != -1);
    fsfd = open(argv[1], O_RDONLY, 0666);
	assert(fsfd != -1);
	struct superblock sb;
	rsect(1, (void*) &sb); // f***ing globally accesses the fsfd
	printf("size: %d, nblocks %d\n", sb.size, sb.nblocks);
    
    //read file
    //~ void* mmappedData = mmap(NULL, filesize, PROT_READ, MAP_PRIVATE | MAP_POPULATE, fd, 0);
    //~ assert(mmappedData != MAP_FAILED);
    //Write the mmapped data to stdout (= FD #1)
    //~ write(1, mmappedData, 1000); // prints something, which is a good sign
    
    
	//~ Each inode is either unallocated or one of the valid types (T_FILE,
	//~ T_DIR, T_DEV). ERROR: bad inode.
	// find superblock
	// use superblock to find start of inodes
	// go number of inodes, for each checking if it is 0 or the appropriate type
	// get beginning address
	// go forward one block size
	
	//~ For in-use inodes, each address that is used by inode is valid (points to
	//~ a valid datablock address within the image). Note: must check indirect
	//~ blocks too, when they are in use. ERROR: bad address in inode.
	//~ Root directory exists, and it is inode number 1. ERROR MESSAGE:
	//~ root directory does not exist.
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
    
    // mmap stuff
    //Cleanup
    //~ int rc = munmap(mmappedData, filesize);
    //~ assert(rc == 0);
    //~ close(fd);
    
    close(fsfd);

	exit(0);
	return 0;
	
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