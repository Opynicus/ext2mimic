//
// Created by Opynicus on 2022/4/12.
//

#ifndef EXT2MIMIC_IMAGE_H
#define EXT2MIMIC_IMAGE_H

#include "params.h"

/*
 * image光盘映像文件数据结构定义
 */
class image {
 public:
  FILE *file_read;                            //读文件
  FILE *file_write;                           //写文件
  image();

  bool initImage();                           //初始化
  bool existImage();

 protected:
 private:
  string file_name;                           //文件名
};

#endif //EXT2MIMIC_IMAGE_H
