//
// Created by Opynicus on 2022/4/14.
//

#include "bitMap.h"

void bitMap::initInodeBitmap(FILE* fw) {
    memset(inode_bitmap,0,sizeof(inode_bitmap));
    fseek(fw,INODE_BITMAP_START_ADDR,SEEK_SET);
    fwrite(inode_bitmap,sizeof(inode_bitmap),1,fw);
    cout<< "初始化inode位图完成" << endl;
    fflush(fw);

}

void bitMap::initBlockBitmap(FILE *fw) {
    memset(block_bitmap,0,sizeof(block_bitmap));
    fseek(fw,BLOCK_BITMAP_START_ADDR,SEEK_SET);
    fwrite(block_bitmap,sizeof(block_bitmap),1,fw);
    cout<< "初始化block位图完成" << endl;
    fflush(fw);
}

void bitMap::readInodeBitmap(FILE *fr) {
    fseek(fr,INODE_BITMAP_START_ADDR,SEEK_SET);
    fread(inode_bitmap,sizeof(inode_bitmap),1,fr);
    cout<< "读取inode位图完成" << endl;
    fflush(fr);

}

void bitMap::readBlockBitmap(FILE *fr) {
    fseek(fr,BLOCK_BITMAP_START_ADDR,SEEK_SET);
    fread(block_bitmap,sizeof(block_bitmap),1,fr);
    cout<< "读取block位图完成" << endl;
    fflush(fr);
}




void bitMap::printInodeBitmap() {
    cout << "Inode使用表如下：" << endl;
    int bitmap_each_line = 32;      //每行32个，一共20行
    for (int i = 0; i < INODE_NUM / bitmap_each_line; ++i) {
        for (int j = 0; j < bitmap_each_line; ++j) {
            if (inode_bitmap[i*bitmap_each_line+j]) {
                cout << "●";
            } else {
                cout << "○";
            }
        }
        cout << "        " << i << endl;
    }
    cout << endl;
}


void bitMap::printBlockBitmap() {
    int bitmap_each_line = 64;      //每行64个，一共160行
    int display_lines_nums_each_times = 20; //每次显示20行
    cout << "Block使用表如下（每次显示 " << display_lines_nums_each_times << " 行，敲下ENTER以继续，q + ENTER以退出）：" << endl;
    for (int i = 0; i < BLOCK_NUM / bitmap_each_line; ++i) {
        for (int j = 0; j < bitmap_each_line; ++j) {
            if (block_bitmap[i*bitmap_each_line+j]) {
                cout << "■";
            } else {
                cout << "□";
            }
        }
        cout << "        " << i << endl;
        if (i != 0 && i % 20 == 0) {
            fflush(stdin);
            char ch;
            while(ch = getchar())
            {
                if (ch == '\n') break;
                if (ch == 'q') {
                    cout << endl;
                    return;
                }
            }
            fflush(stdin);
        }
    }
    cout << endl;
}

void bitMap::occupyOneInode(FILE *fw, unsigned int pos) {
    inode_bitmap[pos] = true;
    fseek(fw,INODE_BITMAP_START_ADDR + pos,SEEK_SET);
    fwrite(&inode_bitmap[pos], sizeof(bool), 1, fw);
    fflush(fw);
}

void bitMap::freeOneInode(FILE *fw, unsigned int pos) {
    inode_bitmap[pos] = false;
    fseek(fw,INODE_BITMAP_START_ADDR + pos,SEEK_SET);
    fwrite(&inode_bitmap[pos], sizeof(bool), 1, fw);
    fflush(fw);
}


void bitMap::occupyOneBlock(FILE *fw, unsigned int pos) {
    block_bitmap[pos] = true;
    fseek(fw,BLOCK_BITMAP_START_ADDR + pos,SEEK_SET);
    fwrite(&block_bitmap[pos], sizeof(bool), 1, fw);
    fflush(fw);
}

void bitMap::freeOneBlock(FILE *fw, unsigned int pos) {
    block_bitmap[pos] = false;
    fseek(fw,BLOCK_BITMAP_START_ADDR + pos,SEEK_SET);
    fwrite(&block_bitmap[pos], sizeof(bool), 1, fw);
    fflush(fw);
}