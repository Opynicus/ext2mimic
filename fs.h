//
// Created by Opynicus on 2022/4/13.
//

#ifndef EXT2MIMIC_FS_H
#define EXT2MIMIC_FS_H

#include "params.h"
#include "image.h"
#include "superBlock.h"
#include <cstring>


class fs {
private:
    image img;
    superBlock super_block;
public:
    fs();
    int initFS();
};


#endif //EXT2MIMIC_FS_H
