//
// Created by Opynicus on 2022/4/12.
//

#include <iostream>
#include <unistd.h>
#include "image.h"

image::image() {
    this->file_name = IMAGE_FILE_NAME;
    this->file_read = nullptr;
    this->file_write = nullptr;
}

bool image::initImage() {
    cout << "init image ......" << endl;
    if (!existImage()) {
        cout << "Warning: image file doesn't exist, try to create new one" << endl;
        file_write = fopen(IMAGE_FILE_NAME, "w");    //新建一个
        if (file_write == nullptr) {    //出错
            cout << "Fatal Error: can't create image file" << endl;
            exit(-1);
        }
        this->file_read = fopen(IMAGE_FILE_NAME, "r");
        cout << "create image !" << endl;
        return false;
    } else {
        this->file_read = fopen(IMAGE_FILE_NAME, "r");
        this->file_write = fopen(IMAGE_FILE_NAME, "r+");
        if (this->file_write == nullptr) {
            cout << "Error: open image failed" << endl;
            exit(-1);
        }
        cout << "load image completed !" << endl;
        return true;
    }
}


bool image::existImage() {
    if ((this->file_read = fopen(IMAGE_FILE_NAME, "rb")) == nullptr) {          //image光盘映像文件不存在
        return false;
    }
    return true;
}