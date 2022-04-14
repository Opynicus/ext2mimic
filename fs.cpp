//
// Created by Opynicus on 2022/4/13.
//

#include "fs.h"

fs::fs() {
    cout<< "开始初始化文件系统" << endl;
}

int fs::initFS() {
    //初始化image
    img.initImage();
    //初始化inode位图
    bit_map.initInodeBitmap(img.file_write);
    //初始化block位图
    bit_map.initBlockBitmap(img.file_write);
    //初始化磁盘块区，根据成组链接法组织
    super_block.initFreeBlockStack(img.file_write);
    //超级块写入到虚拟磁盘文件
    super_block.writeSuperBlock2img(img.file_write);

    //读取inode位图
    bit_map.readInodeBitmap(img.file_read);

    //读取block位图
    bit_map.readBlockBitmap(img.file_read);
//    //打印inode位图
//    bit_map.printInodeBitmap();
//    //打印block位图
//    bit_map.printBlockBitmap();
//
//    super_block.printSuperBlockInfo();

}

int fs::ialloc() {
    if (super_block.getFreeInodeNum() == 0) {
        cout << "已无空闲Inode可供分配" <<endl;
        return -1;
    } else {
        int i;
        for (bool isUsed : bit_map.inode_bitmap) {   //在inode位图上找到空闲的位置
            if (!isUsed) {
                i = isUsed;
                break;
            }
        }
        super_block.writeOneBlock(img.file_write);  //superBlock被写入,inode_num--
        bit_map.occupyOneInode(img.file_write, i);       //更新inode位图

        return INODE_TABLE_START_ADDR + i * INODE_SIZE; //返回Inode位置
    }
}

int fs::ifree(long addr) {
    if ( (addr - INODE_TABLE_START_ADDR) % INODE_SIZE != 0 ){
        printf("Warning: 需要inode节点起始位置\n");
        return -1;
    }
    inodeid_t ino = (addr - INODE_TABLE_START_ADDR) % INODE_SIZE;
    if (!bit_map.inode_bitmap[ino]) {
        cout << "Warning: 空闲节点无需释放" << endl;
        return -1;
    }

    bit_map.clearInodeBitmap(img.file_write, addr);
    super_block.freeOneBlock(img.file_write);

    bit_map.freeOneInode(img.file_write, ino);

    return 0;

}
