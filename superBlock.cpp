//
// Created by Opynicus on 2022/4/12.
//

#include <iostream>
#include "superBlock.h"

superBlock::superBlock() {
    inode_num = INODE_NUM;
    block_num = BLOCK_NUM;

    free_inode_num = INODE_NUM;
    free_block_num = BLOCK_NUM;

    inode_size = INODE_SIZE;
    block_size = BLOCK_SIZE;

    create_time = time(nullptr);
    last_mount_time = 0;
    last_write_time = 0;

    superBlock_startAddr = SUPERBLOCK_START_ADDR;
    inode_bitmap_StartAddr = INODE_BITMAP_START_ADDR;
    block_bitmap_StartAddr = BLOCK_BITMAP_START_ADDR;
    inode_StartAddr = INODE_TABLE_START_ADDR;
    block_StartAddr = DATA_BLOCK_START_ADDR;

    cout<< "initial superBlock completed" << endl;
}