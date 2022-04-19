
#include "include/fs.h"

int main(int argc, char** argv) {
    //函数入口
    image img;
    bool imgExist = img.initImage();    //创建img文件
    superBlock super_block;
    bitMap bit_map;

    fs fileSystem(img, &super_block,&bit_map);
    //cout << "initial file system..." << endl;
    if (imgExist) {
        fileSystem.loadFs();
    } else {
        fileSystem.format();
    }
    //cout << "initial completed !" << endl << endl;
    while(!fileSystem.login()){/*待登录*/};
    system("cls");
    fs::fsInfo();
    while (fileSystem.is_login) {
        fileSystem.commandLinePrompt();
        char cmd[100];
        cin.getline(cmd, 100);
        fileSystem.commandLine(cmd);
    }
    return 0;
}
