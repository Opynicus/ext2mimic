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
#include "utils.h"

class fs {
 public:
  bool is_login;  //是否登录

  //文件系统初始化
  fs(image &img, superBlock &super_block, bitMap &bit_map);

  //文件系统格式化
  bool format();

  //mkdir命令，在当前目录下创建目录
  int mkdir(int parent_inode_addr, char name[]);

  //cd命令，在当前目录下进入子目录
  int cd(int parent_inode_addr, const char name[]);

  //chmod命令，修改当前目录权限
  void chmod(int parent_inode_addr, const char name[], int mode);

  //ls命令，显示该目录下所有文件及目录
  void ls(int parent_inode_addr);

  //ls -l命令，显示该目录下所有文件及目录具体信息
  void lsl(int parent_inode_addr);

  //rmdir命令，移除文件夹命令
  int rmdir(int parent_inode_addr, char name[]);

  //rm -rf命令，删除该目录下所有文件及目录
  void rmrf(int parent_inode_addr);

  //rm命令，移除单个文件
  int rm(int parent_inode_addr, char name[]);

  //cat命令，将文件中的内容在Terminal打印
  void cat(int parent_inode_addr, char name[]);

  //touch命令，在当前目录下创建一个文件
  void touch(int parent_inode_addr, char name[], char buf[]);

  //退出文件系统
  void exit();

  //文件系统命令行
  void commandLine(char cmd[]);

  //文件系统信息头
  void commandLinePrompt();

  //提示信息
  static void fsInfo();

  //帮助信息
  static void help();

  //下次进入文件系统载入信息
  void loadFs();

  //vi命令，仿Vi编辑文件
  void fakeVi(int parent_inode_addr, char name[], char buf[]);

  //stat命令，显示目录或文件具体信息
  int stat(int parent_inode_addr, char name[]);

  //rename命令，重命名文件或目录
  int rename(int parent_inode_addr, char ori_name[], char modify_name[]);

  //useradd命令，添加用户(root用户专用命令)
  void useradd(char user_name[]);

  //userdel命令，删除用户(root用户专用命令)
  void userdel(char user_name[]);

  //登陆文件系统
  bool login();

  //pwd命令，显示当前目录绝对路径
  inline void pwd();

 protected:
 private:
  image &img;
  superBlock &super_block;
  bitMap &bit_map;
  int cur_dir_addr;                           //当前目录
  int nextUID;                                //下一个要分配的用户标识号
  int nextGID;                                //下一个要分配的用户组标识号
  char cur_user_name[MAX_FILE_NAME];          //当前登陆用户名
  char cur_group_name[MAX_FILE_NAME];         //当前用户组名
  char cur_dir_name[MAX_FILE_NAME];           //当前目录名
  char cur_user_dir_name[MAX_FILE_NAME];      //当前登陆用户目录名
  //可读权限判断
  bool isPermitRead(inode &cur);

  //可写权限判断
  bool isPermitWrite(inode &cur);

  //写入文件
  void writeFile(inode fileInode, int fileInodeAddr, char buf[]);

  //创建文件
  int create(int parent_inode_addr, const char name[], char file_content[]);

  //删除用户
  static void delUser(char buf[], char user_name[]);

  //登录检测
  bool access(const char user_name[], const char passwd[]);

  //释放Inode
  int freeInode(int parent_inode_addr);

  //释放Block
  int freeBlock(int parent_inode_addr);

  //Inode分配
  int iAlloc();

  //Block分配
  int bAlloc();
};

#endif //EXT2MIMIC_FS_H
