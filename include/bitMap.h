//
// Created by Opynicus on 2022/4/15.
//

#ifndef EXT2MIMIC_BITMAP_H
#define EXT2MIMIC_BITMAP_H

#endif //EXT2MIMIC_BITMAP_H

#include "params.h"

class bitMap {
 public:
  bool inode_bitmap[INODE_NUM];
  bool block_bitmap[BLOCK_NUM];

  bitMap();
};