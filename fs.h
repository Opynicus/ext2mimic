//
// Created by Opynicus on 2022/4/13.
//

#ifndef EXT2MIMIC_FS_H
#define EXT2MIMIC_FS_H

#include "params.h"
#include "image.h"
#include "superBlock.h"
#include "inode.h"
#include "Dir.h"
#include "bitMap.h"


class fs {
private:
    image& img;
    superBlock* super_block;
    bitMap* bit_map;
    int cur_dir_addr;                           //当前目录
    int nextUID;								//下一个要分配的用户标识号
    int nextGID;								//下一个要分配的用户组标识号
    char cur_user_name[MAX_FILE_NAME];          //当前登陆用户名
    char cur_group_name[MAX_FILE_NAME];          //当前用户组名
    char cur_dir_name[MAX_FILE_NAME];           //当前目录名
    char cur_user_dir_name[MAX_FILE_NAME];      //当前登陆用户目录名
    bool isPermit(inode &cur);
public:
    fs(image& img, superBlock* super_block, bitMap* bit_map);
    bool Format();
    int mkdir(int parent_inode_addr, char name[]);
    bool is_login;
    int cd(int parent_inode_addr, const char name[]);
    int iAlloc();
    int bAlloc();
    int create(int parent_inode_addr, const char name[], char file_content[]);
    void chmod(int parent_inode_addr, const char name[], int mode);
    void ls(int parent_inode_addr);
};


#endif //EXT2MIMIC_FS_H
