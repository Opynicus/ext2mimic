//
// Created by Opynicus on 2022/4/12.
//

#ifndef EXT2MIMIC_SUPERBLOCK_H
#define EXT2MIMIC_SUPERBLOCK_H
#include "params.h"
#include <time.h>
#include "image.h"



/*
 * superBlock结构定义
 */
class superBlock {
private:
    unsigned int inode_num;                 // inode总量，最大2^16 = 65535
    unsigned int free_inode_num;            // inode剩余数量，
    unsigned int block_num;                 // block总量，最大2^32 = 4294967296
    unsigned int free_block_num;            // block剩余数量

    unsigned int block_size;                // block大小
    unsigned int inode_size;                // inode大小

    time_t create_time;                     // 创建时间
    time_t last_write_time;                 // 上次写入时间
    time_t last_mount_time;                 // 上次挂载时间

    unsigned int superBlock_startAddr;      // superBlock开始所在扇区
    unsigned int block_bitmap_StartAddr;    // block位示图开始所在扇区
    unsigned int inode_bitmap_StartAddr;    // inode位示图开始所在扇区
    unsigned int inode_StartAddr;           // inode开始所在扇区
    unsigned int block_StartAddr;           // block开始所在扇区

public:
    superBlock();                                                           // 初始化
    int writeSuperBlock(superBlock* superblock, image* img);                // 写入superBlock
    superBlock* readSuperBlock(image* img);                                 // 读取superBlock
    unsigned int readFreeBlock(image* img);                                 // 读取空闲block
    unsigned int readFreeInode(image* img);                                 // 读取空闲inode
    int writeFreeInode(image *img, unsigned int new_free_block);            // 写入空inode
    int writeFreeBlock(image *img, unsigned int new_free_block);            // 写入空block
    void debugInfo();
    void setLastWriteTime();                                           //测试用的
    time_t getLastWriteTime();                                           //测试用的
};


#endif //EXT2MIMIC_SUPERBLOCK_H
