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

#define INODE_NUM 640                                   //Inode总数640个
#define SUPERBLOCK_SIZE 1024                            //SuperBlock大小（byte）
#define INODE_SIZE 128                                  //Inode大小（byte）128B
#define BLOCK_SIZE 512                                 //Block大小（byte), 512B
#define BLOCK_NUM 10240                                 //块号数为8192,大小为 8192 * 512B

#define MAX_FREE_BLOCKS 64                              //空闲块堆栈大小，一个空闲堆栈最多能存多少个磁盘块地址

#define BLOCK_NUM_PER_INODE (INODE_NUM/(BLOCK_SIZE/INODE_SIZE))  //每个Inode节点占的磁盘块数(160)

#define SUPERBLOCK_START_ADDR 0                             //超级块 偏移地址,占一个磁盘块
#define INODE_BITMAP_START_ADDR (1 * BLOCK_SIZE)            //Block位图 偏移地址，占20个磁盘块，最多监控 512 * 20 个磁盘块的状态
#define BLOCK_BITMAP_START_ADDR (INODE_BITMAP_START_ADDR + 2 * BLOCK_SIZE)    //inode位图 偏移地址，占2个磁盘块，最多监控 512 * 2 个inode的状态
#define INODE_TABLE_START_ADDR (INODE_BITMAP_START_ADDR + 20 * BLOCK_SIZE)    //inode节点区 偏移地址，占 BLOCK_NUM_PER_INODE 个磁盘块
#define DATA_BLOCK_START_ADDR (INODE_TABLE_START_ADDR + BLOCK_NUM_PER_INODE * BLOCK_SIZE)//block数据区 偏移地址 ，占 INODE_NUM 个磁盘块

#define Dir_ITEM_NUM_PER_BLOCK (BLOCK_SIZE / sizeof(unsigned int) / 8)       //每个Block拥有的目录项数量
#define BLOCK_ID0_NUM 10                                                    //Inode直接块数量

#define MODE_DIR	01000					//目录标识
#define MODE_FILE	00000					//文件标识
#define FILE_DEF_PERMISSION 0664			//文件默认权限
#define DIR_DEF_PERMISSION	0755			//目录默认权限

#define ROOT_DIR_ADDR INODE_TABLE_START_ADDR
#endif //EXT2MIMIC_PARAMS_H
