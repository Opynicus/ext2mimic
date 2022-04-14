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

void image::initImage() {
    cout << "init image ......" << endl;
    if (!existImage()) {
        cout << "Warning: image file doesn't exist, try to create new one" << endl;
        file_write = fopen(IMAGE_FILE_NAME, "wb");    //新建一个
        if (file_write == nullptr) {    //出错
            cout << "Fatal Error: can't create image file" << endl;
            exit(-1);
        }
        this->file_read = fopen(IMAGE_FILE_NAME, "rb");
        cout << "Done" << endl;
    } else {
        this->file_read = fopen(IMAGE_FILE_NAME, "rb");
        this->file_write = fopen(IMAGE_FILE_NAME, "rb+");
        if (this->file_write == nullptr) {
            cout << "Error: open VHD failed" << endl;
        }
    }
    cout << "initial completed !" << endl;
}

string image::getFileName() {
    return file_name;
}

void image::setFileName(string &file_name) {
    this->file_name = file_name;
}

FILE *image::get_file_write() {
    return file_write;
}

void image::set_file_write(FILE *file_write) {
    this->file_write = file_write;
}

bool image::existImage() {
    if ((this->file_read = fopen(IMAGE_FILE_NAME, "rb")) == nullptr) {          //image光盘映像文件不存在
        return false;
    }
    return true;
}