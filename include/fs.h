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
#include "vimimic.h"


class fs {
public:
    bool is_login;
    fs(image& img, superBlock* super_block, bitMap* bit_map);
    bool format();
    int mkdir(int parent_inode_addr, char name[]);
    int cd(int parent_inode_addr, const char name[]);
    void chmod(int parent_inode_addr, const char name[], int mode);
    void ls(int parent_inode_addr);
    int rmdir(int parent_inode_addr, char name[]);
    void rmrf(int parent_inode_addr);
    int rm(int parent_inode_addr, char name[]);
    void cat(int parent_inode_addr, char name[]);
    void touch(int parent_inode_addr,char name[],char buf[]);
    void exit();
    void commandLine(char cmd[]);
    void commandLinePrompt();
    static void fsInfo();
    static void help();
    void loadFs();
    void fakeVi(int parent_inode_addr,char name[],char buf[]);
    int stat(int parent_inode_addr, char name[]);
    int rename(int parent_inode_addr, char ori_name[], char modify_name[]);
    void useradd(char user_name[]);
    void userdel(char user_name[]);
    bool login();
    inline void pwd();
protected:
private:
    image& img;
    superBlock* super_block;
    bitMap* bit_map;
    int cur_dir_addr;                           //当前目录
    int nextUID;								//下一个要分配的用户标识号
    int nextGID;								//下一个要分配的用户组标识号
    char cur_user_name[MAX_FILE_NAME];          //当前登陆用户名
    char cur_group_name[MAX_FILE_NAME];         //当前用户组名
    char cur_dir_name[MAX_FILE_NAME];           //当前目录名
    char cur_user_dir_name[MAX_FILE_NAME];      //当前登陆用户目录名
    bool isPermitRead(inode &cur);
    bool isPermitWrite(inode &cur);
    void writefile(inode fileInode, int fileInodeAddr, char buf[]);
    int create(int parent_inode_addr, const char name[], char file_content[]);
    static void delUser(char buf[], char user_name[]);
    bool access(char user_name[], char passwd[]);
    int freeInode(int parent_inode_addr);
    int freeBlock(int parent_inode_addr);
    int iAlloc();
    int bAlloc();
};


#endif //EXT2MIMIC_FS_H
