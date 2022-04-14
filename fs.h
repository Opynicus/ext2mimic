//
// Created by Opynicus on 2022/4/13.
//

#ifndef EXT2MIMIC_FS_H
#define EXT2MIMIC_FS_H

#include "params.h"
#include "image.h"
#include "superBlock.h"
#include "bitMap.h"
#include <cstring>


class fs {
private:
    image img;
    superBlock super_block;
    bitMap bit_map{};

public:
    fs();
    int initFS();
    int ialloc();                                       //分配inode，返回inode地址
    int ifree(long addr);                                       //释放inode
};


#endif //EXT2MIMIC_FS_H
