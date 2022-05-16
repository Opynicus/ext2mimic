//
// Created by Opynicus on 2022/4/14.
//

#ifndef EXT2MIMIC_DIR_H
#define EXT2MIMIC_DIR_H

#include "params.h"

typedef struct Dir { //本质是地址位与名称的Hash Map
  int inodeAddr;
  char file_name[MAX_FILE_NAME];
} Dir;

#endif //EXT2MIMIC_DIR_H
