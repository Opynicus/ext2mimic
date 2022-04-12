//
// Created by Opynicus on 2022/4/12.
//

#ifndef EXT2MIMIC_PARAMS_H
#define EXT2MIMIC_PARAMS_H

#define INODE_NUM (1 << 16)                             // inode总数65536
#define SUPERBLOCK_SIZE 1024                            // superblock大小（byte）
#define INODE_SIZE 128                                  // inode大小（byte）
#define BLOCK_SIZE 4096                                 // block大小（byte）
#define BLOCK_NUM (SECTOR_NUM-1-2048-2-8)             // block个数（260085）

#define SUPERBLOCK_START_SECTOR 0                            // inode开始所在扇区
#define INODE_START_SECTOR 1                            // inode开始所在扇区
#define INODE_BITMAP_START_SECTOR (1 + INODE_NUM * INODE_SIZE / SECTOR_SIZE)                   // inode位示图开始所在扇区
#define BLOCK_BITMAP_START_SECTOR (INODE_BITMAP_START_SECTOR + INODE_NUM / (SECTOR_SIZE * 8) )     // block位示图开始所在扇区
#define BLOCK_START_SECTOR (BLOCK_BITMAP_START_SECTOR + SECTOR_NUM / (SECTOR_SIZE * 8))           // block开始所在扇区

#define IMAGE_FILE_NAME "disk.img"        //定义image映像文件
#define IMAGE_SIZE (1<<30)                           // 文件系统image总大小（1g = 2^30byte）
#define SECTOR_SIZE 4096                             // 文件系统image单个sector大小（byte）
#define SECTOR_NUM (IMAGE_SIZE / BLOCK_SIZE)   // sector个数（262144）


#endif //EXT2MIMIC_PARAMS_H
