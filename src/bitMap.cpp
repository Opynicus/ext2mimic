//
// Created by Opynicus on 2022/4/15.
//

#include "../include/bitMap.h"

bitMap::bitMap() {
    memset(inode_bitmap,0,sizeof(inode_bitmap));
    memset(block_bitmap,0,sizeof(block_bitmap));
}