/*
 * Copyright(C) 2011-2014 Pedro H. Penna <pedrohenriquepenna@gmail.com>
 * 
 * This file is part of Nanvix.
 * 
 * Nanvix is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 * 
 * Nanvix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with Nanvix. If not, see <http://www.gnu.org/licenses/>.
 */

#include <sys/types.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "bitmap.h"
#include "minix.h"
#include "stat.h"
#include "util.h"

/**
 * @brief Mounted superblock.
 */
static struct d_superblock super;

/**
 * @brief Identifier of the file where the mounted Minix file system resides.
 */
static int fd = -1;

/**
 * @brief Inode map.
 */
static struct
{
	size_t size;      /**< Size of bitmap (in byte). */
	uint32_t *bitmap; /**< Bitmap.                   */
} imap;

/**
 * @brief Zone map.
 */
static struct
{
	size_t size;      /**< Size of bitmap (in byte). */
	uint32_t *bitmap; /**< Bitmap.                   */
} zmap;

/**
 * @brief Reads the superblock of a Minix file system.
 */
static void minix_super_read(void)
{	
	/* Read superblock. */
	slseek(fd, 1*BLOCK_SIZE, SEEK_SET);
	sread(fd, &super, sizeof(struct d_superblock));
	if (super.s_magic != SUPER_MAGIC)
		error("bad magic number");
	
	/* Read inode map. */
	imap.bitmap = smalloc(super.s_imap_nblocks*BLOCK_SIZE);
	imap.size = super.s_imap_nblocks*BLOCK_SIZE;
	sread(fd, imap.bitmap, super.s_imap_nblocks*BLOCK_SIZE);
	
	/* Read zone map. */
	zmap.bitmap = smalloc(super.s_bmap_nblocks*BLOCK_SIZE);
	zmap.size = super.s_bmap_nblocks*BLOCK_SIZE;
	sread(fd, zmap.bitmap, super.s_bmap_nblocks*BLOCK_SIZE);
}

/**
 * @brief Writes the superblock of a Minix file system.
 */
static void minix_super_write(void)
{
	/* Write superblock. */
	slseek(fd, 1*BLOCK_SIZE, SEEK_SET);
	swrite(fd, &super, sizeof(struct d_superblock));
	
	/* Read inode map. */
	imap.bitmap = smalloc(super.s_imap_nblocks*BLOCK_SIZE);
	imap.size = super.s_imap_nblocks*BLOCK_SIZE;
	swrite(fd, imap.bitmap, super.s_imap_nblocks*BLOCK_SIZE);
	
	/* Read zone map. */
	zmap.bitmap = smalloc(super.s_bmap_nblocks*BLOCK_SIZE);
	zmap.size = super.s_bmap_nblocks*BLOCK_SIZE;
	swrite(fd, zmap.bitmap, super.s_bmap_nblocks*BLOCK_SIZE);
	
	/* House keeping. */
	free(imap.bitmap);
	free(zmap.bitmap);
}

/**
 * @brief Mounts a Minix file system.
 * 
 * @param filename File where the Minix file system resides.
 */
void minix_mount(const char *filename)
{
	fd = sopen(filename, O_RDWR);
	minix_super_read();
}

/**
 * @brief Unmounts the currently mounted Minix file system.
 * 
 * @note The Minix file system must be mounted.
 */
void minix_umount(void)
{
	minix_super_write();
	sclose(fd);
}

/**
 * @brief Reads an inode from the currently mounted Minix file system.
 * 
 * @param num Number of the inode that shall be read.
 * 
 * @returns A pointer to the requested inode.
 * 
 * @note The Minix file system must be mounted.
 */
struct d_inode *minix_inode_read(uint16_t num)
{
	unsigned idx, off;         /* Inode number offset/index. */
	off_t offset;              /* Offset in the file system. */
	struct d_inode *ip;        /* Inode.                     */
	unsigned inodes_per_block; /* Inodes per block.          */
	
	/* Bad inode number. */
	if (num >= super.s_imap_nblocks*BLOCK_SIZE*8)
		error("bad inode number");
	
	ip = smalloc(sizeof(struct d_inode));
	
	/* Compute file offset. */
	inodes_per_block = BLOCK_SIZE/sizeof(struct d_inode);
	idx = num/inodes_per_block; 
	off = num%inodes_per_block;
	offset = (2 + super.s_imap_nblocks + super.s_bmap_nblocks + idx)*BLOCK_SIZE;
	offset += off*sizeof(struct d_inode);
	
	/* Read inode. */
	slseek(fd, offset, SEEK_SET);
	sread(fd, ip, sizeof(struct d_inode));
	
	return (ip);
}

/**
 * @brief Writes an inode to the currently mounted Minix file system.
 * 
 * @param num Number of the inode that shall be written.
 * @param ip  Inode that shall be written.
 * 
 * @note The Minix file system must be mounted.
 */
void minix_inode_write(uint16_t num, struct d_inode *ip)
{
	unsigned idx, off;         /* Inode number offset/index. */
	off_t offset;              /* Offset in the file system. */
	unsigned inodes_per_block; /* Inodes per block.          */
	
	/* Bad inode number. */
	if (num >= super.s_imap_nblocks*BLOCK_SIZE*8)
		error("bad inode number");
	
	/* Compute file offset. */
	inodes_per_block = BLOCK_SIZE/sizeof(struct d_inode);
	idx = num/inodes_per_block; 
	off = num%inodes_per_block;
	offset = (2 + super.s_imap_nblocks + super.s_bmap_nblocks + idx)*BLOCK_SIZE;
	offset += off*sizeof(struct d_inode);
	
	/* Read inode. */
	slseek(fd, offset, SEEK_SET);
	swrite(fd, ip, sizeof(struct d_inode));
	
	free(ip);
}

/**
 * @brief Allocates a disk block.
 * 
 * @returns The number of the allocated block.
 * 
 * @note The Minix file system must be mounted.
 */
static block_t minix_block_alloc(void)
{
	uint32_t bit;

	/* Allocate block. */
	bit = bitmap_first_free(zmap.bitmap, zmap.size);
	if (bit == BITMAP_FULL)
		error("block map overflow");
	bitmap_set(zmap.bitmap, bit);
	
	return (super.s_first_data_block + bit);
}

/**
 * @brief Allocates an inode.
 * 
 * @param mode Access mode.
 * @param uid  User ID.
 * @param gid  User group ID.
 * 
 * @returns The number of the allocated inode.
 * 
 * @note The Minix file system must be mounted.
 */
static uint16_t minix_inode_alloc(uint16_t mode, uint16_t uid, uint16_t gid)
{
	uint16_t num;       /* Inode number.                */
	uint32_t bit;       /* Bit number if the inode map. */
	struct d_inode *ip; /* New inode.                   */

	/* Allocate inode. */
	bit = bitmap_first_free(imap.bitmap, imap.size);
	if (bit == BITMAP_FULL)
		error("inode map overflow");
	bitmap_set(imap.bitmap, bit);
	num = 2 + super.s_imap_nblocks + super.s_bmap_nblocks + bit;
	
	/* Initialize inode. */
	ip = minix_inode_read(num);
	ip->i_mode = mode;
	ip->i_uid = uid;
	ip->i_size = 0;
	ip->i_time = 0;
	ip->i_gid = gid;
	ip->i_nlinks = 1;
	for (unsigned i; i < NR_ZONES; i++)
		ip->i_zones[i] = BLOCK_NULL;
	minix_inode_write(num, ip);

	return (num);
}

/**
 * @brief Maps a file byte offset in a block number.
 * 
 * @param ip     File to use
 * @param off    File byte offset.
 * @param create Create offset?
 * 
 * @returns The block number that is associated with the file byte offset.
 * 
 * @note @p ip must point to a valid inode.
 * @note The Minix file system must be mounted.
 */
static block_t minix_block_map(struct d_inode *ip, off_t off, bool create)
{
	block_t phys;                            /* Phys. blk. #.   */
	block_t logic;                           /* Logic. blk. #.  */
	block_t buf[BLOCK_SIZE/sizeof(block_t)]; /* Working buffer. */
	
	logic = off/BLOCK_SIZE;
	
	/* File offset too big. */
	if ((uint32_t)off >= super.s_max_size)
		error("file too big");
	
	/* 
	 * Create blocks that are
	 * in a valid offset.
	 */
	if ((uint32_t)off < ip->i_size)
		create = true;
	
	/* Direct block. */
	if (logic < NR_ZONES_DIRECT)
	{
		/* Create direct block. */
		if (ip->i_zones[logic] == BLOCK_NULL && create)
		{
			phys = minix_block_alloc();
			ip->i_zones[logic] = phys;
		}
		
		return (ip->i_zones[logic]);
	}
	
	logic -= NR_ZONES_DIRECT;
	
	/* Single indirect block. */
	if (logic < NR_SINGLE)
	{
		/* Create single indirect block. */
		if (ip->i_zones[ZONE_SINGLE] == BLOCK_NULL && create)
		{
			phys = minix_block_alloc();
			ip->i_zones[ZONE_SINGLE] = phys;
		}
		
		/* We cannot go any further. */
		if ((phys = ip->i_zones[ZONE_SINGLE]) == BLOCK_NULL)
			error("invalid offset");
	
		off = phys*BLOCK_SIZE;
		slseek(fd, off, SEEK_SET);
		sread(fd, buf, BLOCK_SIZE);
		
		/* Create direct block. */
		if (buf[logic] == BLOCK_NULL && create)
		{
			phys = minix_block_alloc();
			buf[logic] = phys;
			slseek(fd, off, SEEK_SET);
			swrite(fd, buf, BLOCK_SIZE);
		}
		
		return (buf[logic]);
	}
	
	logic -= NR_SINGLE;
	
	/* Double indirect zone. */
	error("double indect zone");
	
	return (BLOCK_NULL);
}

/**
 * @brief Searches for a directory entry.
 * 
 * @param ip       Directory where the directory entry shall be searched. 
 * @param filename Name of the directory entry that shall be searched.
 * @param create   Create directory entry?
 * 
 * @returns The file offset where the directory entry is located, or -1 if the
 *          file does not exist.
 * 
 * @note @p ip must point to a valid inode
 * @note @p filename must point to a valid file name.
 * @note The Minix file system must be mounted.
 */
static off_t dirent_search(struct d_inode *ip, const char *filename,bool create)
{
	int i;             /* Working entry.               */
	off_t base, off;   /* Working file offsets.        */
	int entry;         /* Free entry.                  */
	block_t blk;       /* Working block.               */
	int nentries;      /* Number of directory entries. */
	struct d_dirent d; /* Working directory entry.     */
	
	nentries = ip->i_size/sizeof(struct d_dirent);
	
	/* Search for directory entry. */
	i = 0;
	entry = -1;
	blk = ip->i_zones[0];
	base = -1;
	while (i < nentries)
	{
		/* Skip invalid block. */
		if (blk == BLOCK_NULL)
		{
			i += BLOCK_SIZE/sizeof(struct d_dirent);
			blk = minix_block_map(ip, i*sizeof(struct d_dirent), false);
			continue;
		}
		
		/* Compute file offset. */
		if (base < 0)
		{
			off = 0;
			base = (super.s_first_data_block + blk)*BLOCK_SIZE;
			slseek(fd, base, SEEK_SET);
		}
		
		/* Get next block. */
		else if (off >= BLOCK_SIZE)
		{
			base = -1;
			blk = minix_block_map(ip, i*sizeof(struct d_dirent), false);
			continue;
		}
		
		sread(fd, &d, sizeof(struct d_dirent));
		
		/* Valid entry. */
		if (d.d_ino != INODE_NULL)
		{
			/* Found. */
			if (!strncmp(d.d_name, filename, MINIX_NAME_MAX))
			{
				/* Duplicate entry. */
				if (create)
					return (-1);
				
				return (base + off);
			}
		}
		
		/* Remember entry index. */
		else
			entry = i;
		
		i++;
		off += sizeof(struct d_dirent);
	}
	
	/* No entry found. */
	if (!create)
		return (-1);

	/* Expand directory. */
	if (entry < 0)
	{
		entry = nentries;
		blk = minix_block_map(ip, entry*sizeof(struct d_dirent), true);
		ip->i_size += sizeof(struct d_dirent);
		ip->i_time = 0;
	}
	
	else
		blk = minix_block_map(ip, entry*sizeof(struct d_dirent), false);
	
	/* Compute file offset. */
	off = (entry%(BLOCK_SIZE/sizeof(struct d_dirent)))*sizeof(struct d_dirent);
	base = blk*BLOCK_SIZE;
		
	return (base + off);
}

/**
 * @brief Searches for a file in a directory.
 * 
 * @param dip       Directory where the file shall be searched.
 * @param filename File that shal be searched.
 * 
 * @returns The inode number of the requested file, or #INODE_NULL, if such
 *          files does not exist.
 * 
 * @note @p dip must point to a valid inode.
 * @note @p filename must point to a valid file name.
 * @note The Minix file system must be mounted.
 */
uint16_t dir_search(struct d_inode *dip, const char *filename)
{
	off_t off;         /* File offset where the entry is. */
	struct d_dirent d; /* Working directory entry.        */
	
	/* Not a directory. */
	if (!S_ISDIR(dip->i_mode))
		error("not a directory");
	
	/* Search directory entry. */
	off = dirent_search(dip, filename, false);
	if (off == -1)
		return (INODE_NULL);
	
	slseek(fd, off, SEEK_SET);
	sread(fd, &d, sizeof(struct d_dirent));
	
	return (d.d_ino);
}

/**
 * @brief Adds an entry in a directory.
 * 
 * @param dip       Directory where the entry should be added.
 * @param filename Name of the entry.
 * @param num      Inode number of the entry.
 * 
 * @note @p dip must point to a valid inode.
 * @note @p filename must point to a valid file name.
 * @note The Minix file system must be mounted.
 */
static void minix_dirent_add
(struct d_inode *dip, const char *filename, uint16_t num)
{
	off_t off;         /* File offset of the entry. */
	struct d_dirent d; /* Directory entry.          */
	
	/* Get free entry. */
	off = dirent_search(dip, filename, true);
	
	/* Read directory entry. */
	slseek(fd, off, SEEK_SET);
	sread(fd, &d, sizeof(struct d_dirent));
	
	/* Set attributes. */
	d.d_ino = num;
	strncpy(d.d_name, filename, MINIX_NAME_MAX);
	
	/* Write directory entry. */
	slseek(fd, off, SEEK_SET);
	swrite(fd, &d, sizeof(struct d_dirent));
}

/**
 * @brief Creates a directory.
 * 
 * @param dip      Directory where the new directory shall be created.
 * @param filename Name of the new directory.
 * 
 * @returns The inode number of the newly created directory.
 * 
 * @note @p dip must point to a valid inode.
 * @note @p filename must point to a valid file name.
 * @note The Minix file system must be mounted.
 */
uint16_t minix_mkdir(struct d_inode *dip, const char *filename)
{
	uint16_t num;
	
	/* Not a directory. */
	if (!S_ISDIR(dip->i_mode))
		error("not a directory");
	
	/* Allocate inode. */
	num = minix_inode_alloc(S_IFDIR, 0, 0);
	minix_dirent_add(dip, filename, num);
	
	return (num);
}
