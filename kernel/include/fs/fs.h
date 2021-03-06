#ifndef INCLUDE_FS_INODE_H
#define INCLUDE_FS_INODE_H

#include <stdatomic.h>
#include <stdint.h>
#include <stddef.h>
#include <time.h>
#include <lib/rbtree.h>
#include <sched/spinlock.h>
#include <fs/direntry.h>
#include <uapi/fcntl.h>

#define NAME_MAX	256

#define ITYPE_MASK	7
#define ITYPE_DIR	1
#define ITYPE_FILE	2
#define ITYPE_CHAR	3

#define NROF_SYSOPEN_FLAGS		5

#define RAMFS_PRESENT	1 //always set if inode belongs to ramfs
#define RAMFS_INITRD	2 //set if file comes from initrd
#define RAMFS_DEVFILE	3

#define FILE_FLAG_PIPE	1 //same bit as SYSOPEN_FLAG_CREATE

typedef int64_t ssize_t;

struct Inode;
struct DirEntry;
struct DevFileOps;

struct File {
	spinlock_t lock;
	atomic_uint refCount;
	int flags;

	struct Inode *inode;
	uint64_t offset;
};

struct SuperBlock {
	unsigned int fsID;
	unsigned int curInodeID;
};

struct InodeAttributes {
	uint32_t ownerID;
	uint32_t groupID;
	uint16_t accessPermissions;

	time_t creationTime;
	time_t modificationTime;
	time_t accessTime;
};

struct Inode {
	//struct RbNode rbHeader; //value = (fsIndex << 32) | inodeIndex
	uint32_t inodeID;

	unsigned int type;
	atomic_int refCount;
	unsigned int nrofLinks;
	
	unsigned int ramfs;

	uint64_t fileSize; //can also be number of dents

	struct SuperBlock *superBlock;

	spinlock_t lock; //used for both the inode and cachedData
	struct InodeAttributes attr;

	bool cacheDirty;
	void *cachedData;
	size_t cachedDataSize;

	struct DevFileOps *ops; //used for device files only
};

extern struct Inode *rootDir;

static inline bool isDir(struct Inode *inode) {
	return (inode->type & ITYPE_MASK) == ITYPE_DIR;
}

/*
Mounts the root directory
*/
int mountRoot(struct Inode *rootInode);

/*
Parses a file/directory path and returns the inode or NULL if not found
*/
struct Inode *getInodeFromPath(struct Inode *cwd, const char *path);

/*
Parses a file/directory path and returns the inode of the base directory and an index to the file name
Useful for fsCreate/fsLink.
*/
struct Inode *getBaseDirFromPath(struct Inode *cwd, int *fileNameIndex, const char *path);

/*
Opens a file
*/
int fsOpen(struct File *f);

/*
Closes a file
*/
void fsClose(struct File *file);

/*
Creates a file or directory
*/
int fsCreate(struct File *output, struct Inode *dir, const char *name, uint32_t type);

/*
Creates a directory entry for a specified inode
*/
int fsLink(struct Inode *dir, struct Inode *inode, const char *name);

/*
Removes a directory entry (and the inode if nrofLinks == 0)
*/
int fsUnlink(struct Inode *dir, const char *name);

/*
Reads from a file, returns nrof bytes read
*/
ssize_t fsRead(struct File *file, void *buffer, size_t bufSize);

/*
Writes to a file
*/
int fsWrite(struct File *file, const void *buffer, size_t bufSize);

/*
Sets the current offset of a file stream
*/
int fsSeek(struct File *file, int64_t offset, int whence);

/*
Gets directory entries
*/
int fsGetDent(struct Inode *dir, struct GetDent *buf, unsigned int index);

/*
Closes all file descriptors with the PROCFILE_CLOEXEC flag set
*/
int fsCloseOnExec(void);

//System calls

int sysWrite(int fd, const void *buffer, size_t size);
int sysRead(int fd, void *buffer, size_t size);
int sysIoctl(int fd, unsigned long request, ...);
int sysOpen(const char *fileName, unsigned int flags);
int sysClose(int fd);

#endif