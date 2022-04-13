//
// Created by Opynicus on 2022/4/12.
//

#ifndef EXT2MIMIC_PARAMS_H
#define EXT2MIMIC_PARAMS_H

#include <iostream>
using namespace std;

#define IMAGE_FILE_NAME "disk.img"

#define INODE_NUM 640                                   // inode总数640个
#define SUPERBLOCK_SIZE 1024                            // superblock大小（byte）
#define INODE_SIZE 128                                  // inode大小（byte）128B
#define BLOCK_SIZE 512                                 // block大小（byte), 512B
#define BLOCK_NUM 10240                                 // 块号数为10240,大小为 10240 * 4096B

#define MAX_FREE_BLOCKS 64                              // 空闲块堆栈大小，一个空闲堆栈最多能存多少个磁盘块地址

#define SUPERBLOCK_START_ADDR 0                         // 超级块 偏移地址,占一个磁盘块
#define BLOCK_BITMAP_START_ADDR (1*BLOCK_SIZE)          // block位图 偏移地址，占8个磁盘块，最多监控 512 * 8 个磁盘块的状态
#define INODE_BITMAP_START_ADDR (BLOCK_BITMAP_START_ADDR+ 8 * BLOCK_SIZE)    // inode位图 偏移地址，占2个磁盘块，最多监控 512 * 2 个inode的状态
#define INODE_TABLE_START_ADDR (INODE_BITMAP_START_ADDR+ 2 * BLOCK_SIZE)    // inode节点区 偏移地址，占 INODE_NUM/(BLOCK_SIZE/INODE_SIZE) 个磁盘块
#define DATA_BLOCK_START_ADDR (INODE_TABLE_START_ADDR + INODE_NUM/(BLOCK_SIZE/INODE_SIZE) * BLOCK_SIZE)//block数据区 偏移地址 ，占 INODE_NUM 个磁盘块

#define TOTALSIZE (DATA_BLOCK_START_ADDR + BLOCK_NUM * BLOCK_SIZE)       //总大小

extern bool inode_bitmap[INODE_NUM];				//inode位图
extern bool block_bitmap[BLOCK_NUM];				//磁盘块位图


#endif //EXT2MIMIC_PARAMS_H
