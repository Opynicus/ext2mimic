//
// Created by Opynicus on 2022/4/12.
//

#include <iostream>
#include "../image.h"
#include "../superBlock.h"

void debug(image* img, superBlock* sb) {
    cout << "image name:                " << img->getFileName() << endl;
    sb->debugInfo();
}

void test() {
    image img;
    img.initImage();        //初始化image
    superBlock superblock;
    img.writeImage(0, &superblock, sizeof(superBlock));     //第一次初始化superBlock后，须写入image中
    cout << "successfully write" << endl;
    if (superblock.readSuperBlock(&img) == NULL) {  //读superBlock失败
        cout << "Error: read failed" << endl;
    }
    debug(&img, &superblock);      //打印信息
    //修改时间
    cout << "It has been " << superblock.getLastWriteTime() << " s passed. I think?" << endl;
    superblock.setLastWriteTime();
    superblock.readSuperBlock(&img);
    _sleep(3000);
        cout << "It has been " << superblock.getLastWriteTime() << " s passed. I think?" << endl;


};

int main(int argc, char** argv) {
    //函数入口
    test();
}