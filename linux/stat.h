#include "file_types.h"

//~ struct stat {
  //~ short type;  // Type of file
  //~ int dev;     // File system's disk device
  //~ uint ino;    // Inode number
  //~ short nlink; // Number of links to file
  //~ uint size;   // Size of file in bytes
//~ };


struct stat {
    short type;     // Type of file
    int dev;        // Device number
    uint ino;       // Inode number on device
    short nlink;    // Number of links to file
    uint size;      // Size of file in bytes
    uchar checksum; //checksum of file
};
