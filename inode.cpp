//
// Created by Opynicus on 2022/4/13.
//

#include <cstring>
#include <iostream>
#include "inode.h"
#include "params.h"

void inode::initInode(string cur_user_name, string cur_group_name) {
    inode_id = 0;
    create_time = time(NULL);
    last_read_time = time(NULL);
    last_modified_time = time(NULL);
    user_name = cur_user_name;
    group_name = cur_group_name;
    link_num = 1;                               //初始化时当前目录只有根目录
    //block_id0[0] =

}

void inode::setInodeID(unsigned int id) {
    this->inode_id = id;
}

void inode::setInodeSize(unsigned int size) {
    this->size = size;
}

void inode::setLinkNum(unsigned int ln) {
    this->link_num = ln;
}

void inode::debugInfo() {
    cout << "inode_id:                 " << inode_id << endl;
    cout << "mode:            " << mode << endl;
    cout << "user_name:                 " << user_name << endl;
    cout << "group_name:            " << group_name << endl;
    cout << "size:                " << size << endl;
    cout << "create_time:               " << create_time << endl;
    cout << "flag:           " << flag << endl;
    cout << "link_num:           " << link_num << endl;
    cout << "block_id0:      " << block_id0 << endl;
    cout << "block_id1:    " << block_id1 << endl;
    cout << "create_time:           " << create_time << endl;
    cout << "last_read_time:           " << last_read_time << endl;
    cout << "last_modified_time:           " << last_modified_time << endl;
}