//
// Created by Opynicus on 2022/4/13.
//

#include <cstring>
#include <iostream>
#include "../include/inode.h"
#include "../include/params.h"

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