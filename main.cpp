
#include "fs.h"

int main(int argc, char** argv) {
    //函数入口
    image img;
    img.initImage();    //创建img文件
    superBlock super_block;
    bitMap bit_map;

    fs fileSystem(img, &super_block,&bit_map);
        cout << "初始化文件系统..." << endl;
        fileSystem.Format();
}
