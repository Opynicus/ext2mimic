//
// Created by Opynicus on 2022/4/14.
//

#ifndef EXT2MIMIC_DIR_H
#define EXT2MIMIC_DIR_H

#include "params.h"

class Dir {
public:
    int inodeAddr;
    char file_name[MAX_FILE_NAME];
};


#endif //EXT2MIMIC_DIR_H
