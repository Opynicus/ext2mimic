//
// Created by Opynicus on 2022/4/13.
//

#include "fs.h"

bool inode_bitmap[INODE_NUM];				//inode位图
bool block_bitmap[BLOCK_NUM];				//磁盘块位图

fs::fs() {
    cout<< "开始初始化文件系统" << endl;
}

int fs::initFS() {
    //初始化inode位图
    memset(inode_bitmap,0,sizeof(inode_bitmap));
    fseek(img.file_write,INODE_BITMAP_START_ADDR,SEEK_SET);
    fwrite(inode_bitmap,sizeof(inode_bitmap),1,img.file_write);
    cout<< "初始化inode位图完成" << endl;

    //初始化block位图
    memset(block_bitmap,0,sizeof(block_bitmap));
    fseek(img.file_write,BLOCK_BITMAP_START_ADDR,SEEK_SET);
    fwrite(block_bitmap,sizeof(block_bitmap),1,img.file_write);
    cout<< "初始化block位图完成" << endl;

    /*
     * 参考 https://blog.csdn.net/Ajay666/article/details/73569654，初始化磁盘块区，根据成组链接法组织
     */
    for(int i = BLOCK_NUM / MAX_FREE_BLOCKS - 1;i >= 0; i--) {	//一共INODE_NUM/MAX_FREE_BLOCKS，一组FREESTACKNUM（128）个磁盘块 ，第一个磁盘块作为索引
        if(i == BLOCK_NUM / MAX_FREE_BLOCKS - 1) {
            super_block.free_block_stack[0] = -1;	//没有下一个空闲块了
        }
        else {
            int fuck = DATA_BLOCK_START_ADDR + (i + 1) * MAX_FREE_BLOCKS * BLOCK_SIZE;
            super_block.free_block_stack[0] = DATA_BLOCK_START_ADDR + (i + 1) * MAX_FREE_BLOCKS * BLOCK_SIZE;	//指向下一个空闲块
        }
        for(int j = 1;j < MAX_FREE_BLOCKS; j++) {
            int shit = DATA_BLOCK_START_ADDR + (i * MAX_FREE_BLOCKS + j) * BLOCK_SIZE;
            super_block.free_block_stack[j] = DATA_BLOCK_START_ADDR + (i * MAX_FREE_BLOCKS + j) * BLOCK_SIZE;
        }
        fseek(img.file_write,DATA_BLOCK_START_ADDR + i * MAX_FREE_BLOCKS * BLOCK_SIZE,SEEK_SET);
        fwrite(super_block.free_block_stack,sizeof(super_block.free_block_stack),1,img.file_write);	//填满这个磁盘块，4096字节
    }

    //超级块写入到虚拟磁盘文件
    fseek(img.file_write,SUPERBLOCK_START_ADDR,SEEK_SET);
    fwrite(&super_block,sizeof(superBlock),1,img.file_write);

    fflush(img.file_write);
    cout<< "超级块写入到img文件完成" << endl;

    //读取inode位图
    fseek(img.file_read,INODE_BITMAP_START_ADDR,SEEK_SET);
    fread(inode_bitmap,sizeof(inode_bitmap),1,img.file_read);
    cout<< "读取inode位图完成" << endl;

    //读取block位图
    fseek(img.file_read,BLOCK_BITMAP_START_ADDR,SEEK_SET);
    fread(block_bitmap,sizeof(block_bitmap),1,img.file_read);

    fflush(img.file_read);
    cout<< "读取block位图完成" << endl;

}

