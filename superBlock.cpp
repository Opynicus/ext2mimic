//
// Created by Opynicus on 2022/4/12.
//

#include <iostream>
#include "superBlock.h"

superBlock::superBlock() {
    this->inode_num = INODE_NUM;
    this->free_inode_num = INODE_NUM;
    this->block_num = BLOCK_NUM;
    this->free_block_num = BLOCK_NUM;

    this->inode_size = INODE_SIZE;
    this->block_size = BLOCK_SIZE;

    this->create_time = time(NULL);
    this->last_mount_time = 0;
    this->last_write_time = 0;

    this->superBlock_startAddr = SUPERBLOCK_START_SECTOR;
    this->inode_StartAddr = INODE_START_SECTOR;
    this->inode_bitmap_StartAddr = INODE_BITMAP_START_SECTOR;
    this->block_StartAddr = BLOCK_START_SECTOR;
    this->block_bitmap_StartAddr = BLOCK_BITMAP_START_SECTOR;

    cout<< "initial superBlock completed" << endl;
}

int superBlock::writeSuperBlock(superBlock* superblock, image* img) {
    superblock->last_write_time = time(NULL);
    return img->writeImage(0, superblock, sizeof(superblock));
}

superBlock *superBlock::readSuperBlock(image* img) {
    superBlock* sb = (superBlock *)malloc(sizeof(superBlock));
    if (img->readImage(0, sb, sizeof(superBlock)) != 0) {
        return NULL;
    }
    return sb;
}

unsigned int superBlock::readFreeBlock(image* img) {
    superBlock* sb = readSuperBlock(img);
    if(sb == nullptr)
        return -1;
    return sb->free_block_num;
}

unsigned int superBlock::readFreeInode(image *img) {
    superBlock *sb = readSuperBlock(img);
    if(sb == nullptr)
        return -1;
    return sb->free_inode_num;
}

int superBlock::writeFreeBlock(image *img, unsigned int new_free_block) {
    superBlock *sb = readSuperBlock(img);
    if(sb == nullptr)
        return -1;
    sb->free_block_num = new_free_block;
    return writeSuperBlock(sb, img);
}

int superBlock::writeFreeInode(image *img, unsigned int new_free_block) {
    superBlock *sb = readSuperBlock(img);
    if(sb == nullptr)
        return -1;
    sb->free_inode_num = new_free_block;
    return writeSuperBlock(sb, img);
}

void superBlock::debugInfo() {
    cout << "inode_num:                 " << inode_num << endl;
    cout << "free_inode_num:            " << free_inode_num << endl;
    cout << "block_num:                 " << block_num << endl;
    cout << "free_block_num:            " << free_block_num << endl;
    cout << "block_size:                " << block_size << endl;
    cout << "inode_size:                " << inode_size << endl;
    cout << "create_time:               " << create_time << endl;
    cout << "last_write_time:           " << last_write_time << endl;
    cout << "last_mount_time:           " << last_mount_time << endl;
    cout << "superBlock_startAddr:      " << superBlock_startAddr << endl;
    cout << "block_bitmap_StartAddr:    " << block_bitmap_StartAddr << endl;
    cout << "inode_bitmap_StartAddr:    " << inode_bitmap_StartAddr << endl;
    cout << "inode_StartAddr:           " << inode_StartAddr << endl;
    cout << "block_StartAddr:           " << block_StartAddr << endl;
}

void superBlock::setLastWriteTime() {
    this->last_write_time = time(NULL);
}

time_t superBlock::getLastWriteTime() {
    return this->last_write_time;
}