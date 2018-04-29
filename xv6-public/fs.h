// On-disk file system format.
// Both the kernel and user programs use this header file.


#define ROOTINO 1  // root i-number
#define BSIZE 512  // block size

// Disk layout:
// [ boot block | super block | log | inode blocks |
//                                          free bit map | data blocks]
//
// mkfs computes the super block and builds an initial file system. The
// super block describes the disk layout:
struct superblock {
  uint size;         // Size of file system image (blocks)
  uint nblocks;      // Number of data blocks
  uint ninodes;      // Number of inodes.
  uint nlog;         // Number of log blocks
  uint logstart;     // Block number of first log block
  uint inodestart;   // Block number of first inode block // LOOK HERE
  uint bmapstart;    // Block number of first free map block
};

#define NDIRECT 12 // me: i guess every file gets 12 direct data blocks no matter what
#define NINDIRECT (BSIZE / sizeof(uint)) // me: number of pointers we can fit in a block
#define MAXFILE (NDIRECT + NINDIRECT) // max number of blocks in a file // so i guess the file gets a 13th block of indirect pointers?

// On-disk inode structure
struct dinode { // me: d for disk
  short type;           // File type
  short major;          // Major device number (T_DEV only)
  short minor;          // Minor device number (T_DEV only)
  short nlink;          // Number of links to inode in file system
  uint size;            // Size of file (bytes)
  uint addrs[NDIRECT+1];   // Data block addresses // me: +1 for the block used for addresses to point to indirect blocks
};

// Inodes per block.
#define IPB           (BSIZE / sizeof(struct dinode))

// Block containing inode i // must pass it sb, the superblock
#define IBLOCK(i, sb)     ((i) / IPB + sb.inodestart)

// Bitmap bits per block
#define BPB           (BSIZE*8)

// Block of free map containing bit for block b
#define BBLOCK(b, sb) (b/BPB + sb.bmapstart)

// Directory is a file containing a sequence of dirent structures.
#define DIRSIZ 14

struct dirent { // directory entry
  ushort inum; // me: inode number for that file
  char name[DIRSIZ];
};

// assumes the address is a uint, ie 4 bytes
#define CLEAR_TOP_BYTE(address) (address & 0x00FFFFFF)
