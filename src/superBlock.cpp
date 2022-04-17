//
// Created by Opynicus on 2022/4/12.
//

#include <iostream>
#include "../include/superBlock.h"

superBlock::superBlock() {
    inode_num = INODE_NUM;
    block_num = BLOCK_NUM;

    free_inode_num = INODE_NUM;
    free_block_num = BLOCK_NUM;

    inode_size = INODE_SIZE;
    block_size = BLOCK_SIZE;

    memset(free_block_stack,0,sizeof(free_block_stack));
    free_addr = DATA_BLOCK_START_ADDR;

    create_time = time(NULL);
    last_mount_time = 0;
    last_write_time = 0;

    superBlock_startAddr = SUPERBLOCK_START_ADDR;
    inode_bitmap_StartAddr = INODE_BITMAP_START_ADDR;
    block_bitmap_StartAddr = BLOCK_BITMAP_START_ADDR;
    inode_StartAddr = INODE_TABLE_START_ADDR;
    block_StartAddr = DATA_BLOCK_START_ADDR;

    ///cout<< "initial superBlock completed" << endl;
}

void superBlock::initFreeBlockStack(FILE* fw) {
    /*
     * 参考 https://blog.csdn.net/Ajay666/article/details/73569654
     */
    for(int i = BLOCK_NUM / MAX_FREE_BLOCKS - 1;i >= 0; i--) {	//一共BLOCK_NUM / MAX_FREE_BLOCKS，一组 128 个磁盘块 ，每组第一个磁盘块作为索引
        if(i == BLOCK_NUM / MAX_FREE_BLOCKS - 1) {
            free_block_stack[0] = -1;	//没有下一个空闲块了，标记为 -1
        }
        else {
            free_block_stack[0] = DATA_BLOCK_START_ADDR + (i + 1) * MAX_FREE_BLOCKS * BLOCK_SIZE;	//指向下一个空闲块
        }
        for(int j = 1;j < MAX_FREE_BLOCKS; j++) {
            free_block_stack[j] = DATA_BLOCK_START_ADDR + (i * MAX_FREE_BLOCKS + j) * BLOCK_SIZE;
        }
        //将第i组的所以地址信息存入data_blcok对应位置，前一个的结尾指向下一个的开头。
        fseek(fw,DATA_BLOCK_START_ADDR + i * MAX_FREE_BLOCKS * BLOCK_SIZE,SEEK_SET);
        fwrite(free_block_stack,sizeof(free_block_stack),1,fw);
    }
    fflush(fw);
}

void superBlock::writeSuperBlock2img(FILE *img) {
    fseek(img,SUPERBLOCK_START_ADDR,SEEK_SET);
    fwrite(this,sizeof(superBlock),1,img);
    fflush(img);
    //cout<< "SuperBlock写入到img文件完成" << endl;
}


void superBlock::printSuperBlockInfo() const {
    cout << "下面输出Block信息：" << endl;
    cout << "Inode总量为 " << inode_num << endl;
    cout << "Block总量为 " << block_num << endl;
    cout << "Inode剩余总量为 " << free_inode_num << endl;
    cout << "Block剩余总量为 " << free_block_num << endl;
    cout << "单个Block大小为 " << block_size << endl;
    cout << "单个Inode大小为 " << inode_size << endl;
    cout << "superBlock创建时间为 " << ctime(&create_time) << endl;
    cout << "superBlock上次写入时间为 " << ctime(&last_write_time) << endl;
    cout << "superBlock上次挂载时间为 " << ctime(&last_mount_time) << endl;
    cout << "superBlock开始所在位置为 " << superBlock_startAddr << endl;
    cout << "block位示图开始所在位置为 " << block_bitmap_StartAddr << endl;
    cout << "inode位示图开始所在位置为 " << inode_bitmap_StartAddr << endl;
    cout << "inode开始所在位置为 " << inode_StartAddr << endl;
    cout << "block开始所在位置为 " << block_StartAddr << endl;
    cout << endl;
}
