//
// Created by Opynicus on 2022/4/13.
//

#ifndef EXT2MIMIC_INODE_H
#define EXT2MIMIC_INODE_H

#include <ctime>
#include "image.h"

class inode {
public:
    inodeid_t inode_id;                  // inode编号
    unsigned int mode;                      // r/w权限控制
    string user_name;                            // 所属用户ower
    string group_name;                           // 所属用户组group
    unsigned int size;                      // 文件或文件夹实际大小size
    unsigned int flag;                      // 文件标志位
    unsigned int link_num;                  // 链接数
    unsigned int block_id0[10];             // block直接指向，这12个指针保存了10 * BLOCK_SIZE = 12 * 512 = 6144B的数据
    unsigned int block_id1;                 // block间接指向，一个指针4Bytes，(512 / 4) * 512 = (128 * 512)B
//  int block_id2;                          // block双间接指向
    time_t create_time;                     // 创建或状态改变时间
    time_t last_read_time;                  // 上次读取时间
    time_t last_modified_time;              // 上次修改时间
//    int writeInode(unsigned int inode_id, image* img, inode* ind);
//    inode* readInode(unsigned int inode_id, image* img);
    void initInode(string cur_user_name, string cur_group_name, int addr);
    void setInodeSize(unsigned int size);
    void setInodeID(unsigned int size);
    void setLinkNum(unsigned int ln);
    void setBlockNum(unsigned int bn);
    void clearInode(FILE* fw, long addr);
    void debugInfo();
};


#endif //EXT2MIMIC_INODE_H
