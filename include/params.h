//
// Created by Opynicus on 2022/4/12.
//

#ifndef EXT2MIMIC_PARAMS_H
#define EXT2MIMIC_PARAMS_H

#include <iostream>
#include <ctime>
#include <cstring>
#include <fstream>
#include "windows.h"
#include "conio.h"
#include <vector>

using namespace std;

#define IMAGE_FILE_NAME "disk.img"
#define MAX_FILE_NAME 28

#define INODE_NUM 640                                   // inode总数640个
#define SUPERBLOCK_SIZE 1024                            // superblock大小（byte）
#define INODE_SIZE 128                                  // inode大小（byte）128B
#define BLOCK_SIZE 512                                 // block大小（byte), 512B
#define BLOCK_NUM 10240                                 // 块号数为8192,大小为 8192 * 512B

#define MAX_FREE_BLOCKS 64                              // 空闲块堆栈大小，一个空闲堆栈最多能存多少个磁盘块地址

#define BLOCK_NUM_PER_INODE (INODE_NUM/(BLOCK_SIZE/INODE_SIZE))  //每个INODE节点占的磁盘块数(160)

#define SUPERBLOCK_START_ADDR 0                         // 超级块 偏移地址,占一个磁盘块
#define INODE_BITMAP_START_ADDR (1 * BLOCK_SIZE)          // block位图 偏移地址，占20个磁盘块，最多监控 512 * 20 个磁盘块的状态
#define BLOCK_BITMAP_START_ADDR (INODE_BITMAP_START_ADDR + 2 * BLOCK_SIZE)    // inode位图 偏移地址，占2个磁盘块，最多监控 512 * 2 个inode的状态
#define INODE_TABLE_START_ADDR (INODE_BITMAP_START_ADDR + 20 * BLOCK_SIZE)    // inode节点区 偏移地址，占 BLOCK_NUM_PER_INODE 个磁盘块
#define DATA_BLOCK_START_ADDR (INODE_TABLE_START_ADDR + BLOCK_NUM_PER_INODE * BLOCK_SIZE)//block数据区 偏移地址 ，占 INODE_NUM 个磁盘块

#define Dir_ITEM_NUM_PER_BLOCK (BLOCK_SIZE / sizeof(unsigned int) / 8)       //16
#define BLOCK_ID0_NUM 10

#define TOTALSIZE (DATA_BLOCK_START_ADDR + BLOCK_NUM * BLOCK_SIZE)       //总大小

#define MODE_DIR	01000					//目录标识
#define MODE_FILE	00000					//文件标识
#define OWNER_R	4<<6						//本用户读权限
#define OWNER_W	2<<6						//本用户写权限
#define OWNER_X	1<<6						//本用户执行权限
#define GROUP_R	4<<3						//组用户读权限
#define GROUP_W	2<<3						//组用户写权限
#define GROUP_X	1<<3						//组用户执行权限
#define OTHERS_R	4						//其它用户读权限
#define OTHERS_W	2						//其它用户写权限
#define OTHERS_X	1						//其它用户执行权限
#define FILE_DEF_PERMISSION 0664			//文件默认权限
#define DIR_DEF_PERMISSION	0755			//目录默认权限

#define ROOT_DIR_ADDR INODE_TABLE_START_ADDR

#endif //EXT2MIMIC_PARAMS_H
