//
// Created by Opynicus on 2022/4/14.
//

#ifndef EXT2MIMIC_BITMAP_H
#define EXT2MIMIC_BITMAP_H

#include "params.h"
#include "inode.h"

class bitMap {
public:
    bool inode_bitmap[INODE_NUM];				            //inode位图
    bool block_bitmap[BLOCK_NUM];				            //磁盘块位图
    void initInodeBitmap(FILE* fw);                         //初始化inode位图
    void initBlockBitmap(FILE* fw);                         //初始化block位图
    void readInodeBitmap(FILE* fr);                         //读取inode位图
    void readBlockBitmap(FILE* fr);                         //读取block位图
    void occupyOneInode(FILE *fw, unsigned int pos);
    void freeOneInode(FILE *fw, unsigned int pos);
    void occupyOneBlock(FILE *fw, unsigned int pos);
    void freeOneBlock(FILE *fw, unsigned int pos);

    void printInodeBitmap();                                //打印inode位图的信息
    void printBlockBitmap();                                //打印block位图的信息
};


#endif //EXT2MIMIC_BITMAP_H
