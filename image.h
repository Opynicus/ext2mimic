//
// Created by Opynicus on 2022/4/12.
//

#ifndef EXT2MIMIC_IMAGE_H
#define EXT2MIMIC_IMAGE_H

#include <string>
#include <fstream>

#define IMAGEFILENAME "data.img"        //定义image映像文件

using namespace std;

//extern char* imgPath;

/*
 * image光盘映像文件数据结构定义
 */
class image {
private:
    string file_name;                           //文件名
    FILE* file_read;                            //读文件
    FILE* file_write;                           //写文件
public:
    image();
    void initImage();                           //初始化
    string setFileName(string &file_name);      //设置文件名
    string getFileName();                       //获得文件名
    FILE *get_file_write();                     //文件写入指针
    FILE *set_file_write(FILE *file_write);     //设置文件写入指针
    bool existImage();

};


#endif //EXT2MIMIC_IMAGE_H
