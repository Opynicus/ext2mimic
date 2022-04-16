
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
        cout << "初始化完成..." << endl;
        fileSystem.is_login = true;
        while (fileSystem.is_login) {
            fileSystem.commandLinePrompt();
            char cmd[100];
            cin.getline(cmd, 100);
            fileSystem.commandLine(cmd);
        }

}
