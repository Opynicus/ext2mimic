//
// Created by Opynicus on 2022/4/13.
//

#include "fs.h"


string cur_user_name = "root";
string cur_group_name = "root";
 int Root_Dir_Addr;					//根目录inode地址
 int Cur_Dir_Addr;					//当前目录
 char Cur_Dir_Name[310];				//当前目录名
 char Cur_Host_Name[110];				//当前主机名
 char Cur_User_Name[110];				//当前登陆用户名
 char Cur_Group_Name[110];			//当前登陆用户组名
 char Cur_User_Dir_Name[310];			//当前登陆用户目录名
inode cur;
inode tmp;


fs::fs() {
    cout<< "开始初始化文件系统" << endl;
}

int fs::initFS() {
    //初始化image
    img.initImage();
    //初始化inode位图
    bit_map.initInodeBitmap(img.file_write);
    //初始化block位图
    bit_map.initBlockBitmap(img.file_write);
    //初始化磁盘块区，根据成组链接法组织
    super_block.initFreeBlockStack(img.file_write);
    //超级块写入到虚拟磁盘文件
    super_block.writeSuperBlock2img(img.file_write);

    //读取inode位图
    bit_map.readInodeBitmap(img.file_read);

    //读取block位图
    bit_map.readBlockBitmap(img.file_read);
//    //打印inode位图
//    bit_map.printInodeBitmap();
//    //打印block位图
//    bit_map.printBlockBitmap();
//
//    super_block.printSuperBlockInfo();

    int inoAddr = ialloc();
    int blockAddr = balloc();

    Dir dir[Dir_ITEM_NUM_PER_BLOCK];
    strcpy(dir[0].file_name,".");
    dir[0].inodeAddr = inoAddr;

    //写回磁盘块
    fseek(img.file_write,blockAddr,SEEK_SET);
    fwrite(dir,sizeof(dir),1,img.file_write);

    cur.initInode(cur_user_name, cur_group_name, blockAddr);

    fseek(img.file_write,inoAddr,SEEK_SET);
    fwrite(&cur,sizeof(inode),1,img.file_write);
    fflush(img.file_write);


    //创建目录及配置文件
    mkdir(Root_Dir_Addr,"home");	            //用户目录
    cd(Root_Dir_Addr,"home");
    mkdir(Cur_Dir_Addr,"root");

    cd(Cur_Dir_Addr,"..");
    mkdir(Cur_Dir_Addr,"etc");	        //配置文件目录
    cd(Cur_Dir_Addr,"etc");
}

int fs::ialloc() {
    if (super_block.getFreeInodeNum() == 0) {
        cout << "已无空闲Inode可供分配" <<endl;
        return -1;
    } else {
        int i;
        for (bool isUsed : bit_map.inode_bitmap) {          //在inode位图上找到空闲的位置
            if (!isUsed) {
                i = isUsed;
                break;
            }
        }
        super_block.setFreeInodeNum( super_block.getFreeInodeNum() - 1);
        super_block.writeOneBlock(img.file_write);              //superBlock被写入
        bit_map.occupyOneInode(img.file_write, i);       //更新inode位图

        return INODE_TABLE_START_ADDR + i * INODE_SIZE;             //返回Inode位置
    }
}

int fs::ifree(long addr) {
    if ( (addr - INODE_TABLE_START_ADDR) % INODE_SIZE != 0 ){
        printf("Warning: 需要inode节点起始位置\n");
        return -1;
    }
    inodeid_t ino = (addr - INODE_TABLE_START_ADDR) % INODE_SIZE;
    if (!bit_map.inode_bitmap[ino]) {
        cout << "Warning: 空闲节点无需释放" << endl;
        return -1;
    }

    inode inod;
    inod.clearInode(img.file_write, addr);
    super_block.setFreeInodeNum( super_block.getFreeInodeNum() + 1);
    super_block.freeOneBlock(img.file_write);
    bit_map.freeOneInode(img.file_write, ino);

    return 0;

}

int fs::balloc() {
    //使用超级块中的空闲块堆栈
    //计算当前栈顶
    unsigned int top;	//栈顶指针
    if(super_block.getFreeBlockNum() == 0){	//剩余空闲块数为0
        cout << "已无空闲block可供分配" << endl;
        return -1;	//没有可分配的空闲块，返回-1
    }
    else{	//还有剩余块
        top = (super_block.getFreeBlockNum() - 1) % MAX_FREE_BLOCKS;
    }
    //将栈顶取出
    //如果已是栈底，将当前块号地址返回，即为栈底块号，并将栈底指向的新空闲块堆栈覆盖原来的栈
    int retAddr;

    if(top==0){
        retAddr = super_block.free_addr;
        super_block.free_addr = super_block.free_block_stack[0];	//取出下一个存有空闲块堆栈的空闲块的位置，更新空闲块堆栈指针

        //取出对应空闲块内容，覆盖原来的空闲块堆栈

        //取出下一个空闲块堆栈，覆盖原来的
        fseek(img.file_read,super_block.free_addr,SEEK_SET);
        fread(super_block.free_block_stack, sizeof(super_block.free_block_stack), 1, img.file_read);
        fflush(img.file_read);
        super_block.setFreeBlockNum(super_block.getFreeBlockNum() - 1);

    }
    else{	//如果不为栈底，则将栈顶指向的地址返回，栈顶指针-1.
        retAddr = super_block.free_block_stack[top];	//保存返回地址
        super_block.free_block_stack[top] = -1;	//清栈顶
        top--;		//栈顶指针-1
        super_block.setFreeBlockNum(super_block.getFreeBlockNum() - 1);

    }

    //更新超级块
    super_block.writeOneBlock(img.file_write);

    //更新block位图，(retAddr-Block_StartAddr)/BLOCK_SIZE为第几个空闲块
    bit_map.occupyOneBlock(img.file_write, (retAddr - DATA_BLOCK_START_ADDR) / BLOCK_SIZE);

    return retAddr;
}

int fs::bfree(long addr)	//磁盘块释放
{
    //该地址不是磁盘块的起始地址
    if( (addr - DATA_BLOCK_START_ADDR) % BLOCK_SIZE != 0 ){
       cout << "Warning: 需要data_block起始位置" << endl;
       return -1;
    }
    unsigned int bno = (addr - DATA_BLOCK_START_ADDR) % BLOCK_SIZE;	//inode节点号
    //该地址还未使用，不能释放空间
    if(!bit_map.block_bitmap[bno]) {
        cout << "Warning: block未使用" << endl;
        return -1;
    }

    //可以释放
    //计算当前栈顶
    unsigned int top;	//栈顶指针
    if(super_block.getFreeBlockNum() == BLOCK_NUM){	//没有非空闲的磁盘块
        cout << "Warning: 磁盘块全部空闲，无法释放" << endl;
        return -1;	//没有可分配的空闲块，返回-1
    }
    else{	//非满
        top = (super_block.getFreeBlockNum() - 1) % MAX_FREE_BLOCKS;

        //清空block内容
        super_block.clearOneBlock(img.file_write, addr);

        if(top == MAX_FREE_BLOCKS - 1) {	//该栈已满

            //该空闲块作为新的空闲块堆栈
            super_block.free_block_stack[0] = super_block.free_addr;	//新的空闲块堆栈第一个地址指向旧的空闲块堆栈指针
            for(int i = 1; i < MAX_FREE_BLOCKS; i++){
                super_block.free_block_stack[i] = -1;	//清空栈元素的其它地址
            }
            fseek(img.file_write,addr,SEEK_SET);
            fwrite(super_block.free_block_stack, sizeof(super_block.free_block_stack),1,img.file_write);	//填满这个磁盘块，256字节

        }
        else{	//栈还未满
            top++;	//栈顶指针+1
            super_block.free_block_stack[top] = addr;	//栈顶放上这个要释放的地址，作为新的空闲块
        }
    }


    //更新超级块
    super_block.setFreeBlockNum(super_block.getFreeBlockNum() + 1);
    super_block.writeOneBlock(img.file_write);


    //更新block位图
    bit_map.freeOneBlock(img.file_write, bno);

    return true;
}

int fs::mkdir(int parentAddr,char name[]) {
    if(strlen(name) >= MAX_FILE_NAME){
        cout <<"超过最大目录名长度" << endl;
        return -1;
    }

    Dir dir[Dir_ITEM_NUM_PER_BLOCK];	//临时目录清单

    //从这个地址取出inode
    fseek(img.file_read,parentAddr,SEEK_SET);
    fread(&cur,sizeof(inode),1,img.file_read);

    int i = 0;
    int cnt = cur.link_num + 1;	//目录项数
    int posi = -1, posj = -1;
    while(i < BLOCK_NUM_PER_INODE){
        //160个目录项之内，可以直接在直接块里找
        int dno = i / 16;	//在第几个直接块里

        if(cur.block_id0[dno] == -1){
            i += 16;
            continue;
        }
        //取出这个直接块，要加入的目录条目的位置
        fseek(img.file_read,cur.block_id0[dno],SEEK_SET);
        fread(dir,sizeof(dir),1,img.file_read);
        fflush(img.file_read);

        //输出该磁盘块中的所有目录项
        int j;
        for(j = 0; j < Dir_ITEM_NUM_PER_BLOCK; j++){
            if( strcmp(dir[j].file_name, name) == 0 ) {
                fseek(img.file_read, dir[j].inodeAddr,SEEK_SET);
                fread(&tmp,sizeof(inode),1,img.file_read);
                if( ((tmp.mode >> 9 ) & 1) == 1 ){	//不是目录
                    cout <<"目录已存在" << endl;
                    return -1;
                }
            }
            else if( strcmp(dir[j].file_name, "") == 0 ){
                //找到一个空闲记录，将新目录创建到这个位置
                //记录这个位置
                if(posi == -1){
                    posi = dno;
                    posj = j;
                }

            }
            i++;
        }

    }

    if(posi != -1) {	//找到这个空闲位置

        //取出这个直接块，要加入的目录条目的位置
        fseek(img.file_read,cur.block_id0[posi],SEEK_SET);
        fread(dir,sizeof(dir),1,img.file_read);
        fflush(img.file_read);

        //创建这个目录项
        strcpy(dir[posj].file_name,name);	//目录名
        //写入两条记录 "." ".."，分别指向当前inode节点地址，和父inode节点
        int chiinoAddr = ialloc();	//分配当前节点地址
        if(chiinoAddr == -1){
            cout << "inode分配失败" << endl;
            return false;
        }
        dir[posj].inodeAddr = chiinoAddr; //给这个新的目录分配的inode地址

        //设置新条目的inode
        inode p;
        p.inode_id = (chiinoAddr - INODE_TABLE_START_ADDR) / INODE_SIZE;
        p.create_time = time(NULL);
        p.last_read_time = time(NULL);
        p.last_modified_time = time(NULL);
        p.user_name = cur_user_name;
        p.group_name = cur_group_name;
        p.link_num = 2;	//两个项，当前目录,"."和".."

        //分配这个inode的磁盘块，在磁盘号中写入两条记录 . 和 ..
        int curblockAddr = balloc();
        if(curblockAddr == -1){
            cout <<"block分配失败" << endl;
            return -1;
        }
        Dir dir2[16];	//临时目录项列表 - 2
        strcpy(dir2[0].file_name,".");
        strcpy(dir2[1].file_name,"..");
        dir2[0].inodeAddr = chiinoAddr;	//当前目录inode地址
        dir2[1].inodeAddr = parentAddr;	//父目录inode地址

        //写入到当前目录的磁盘块
        fseek(img.file_write,curblockAddr,SEEK_SET);
        fwrite(dir2,sizeof(dir2),1,img.file_write);

        p.block_id0[0] = curblockAddr;
        for(int i = 1;i < 10; i++){
            p.block_id0[i] = -1;
        }
        p.size = BLOCK_SIZE;
        p.block_id1 = -1;	//没使用一级间接块
        p.mode = MODE_DIR | DIR_DEF_PERMISSION;

        //将inode写入到申请的inode地址
        fseek(img.file_write,chiinoAddr,SEEK_SET);
        fwrite(&p,sizeof(inode),1,img.file_write);

        //将当前目录的磁盘块写回
        fseek(img.file_write,cur.block_id0[posi],SEEK_SET);
        fwrite(dir,sizeof(dir),1,img.file_write);

        //写回inode
        cur.link_num++;
        fseek(img.file_write,parentAddr,SEEK_SET);
        fwrite(&cur,sizeof(inode),1,img.file_write);
        fflush(img.file_write);

        return true;
    }
    else{
        cout <<"没找到空闲目录项,目录创建失败" << endl;
        return false;
    }
}

void fs::cd(int parentAddr, char *name) {
    //取出当前目录的inode
    fseek(img.file_read,parentAddr,SEEK_SET);
    fread(&cur,sizeof(inode),1,img.file_read);

    //依次取出inode对应的磁盘块，查找有没有名字为name的目录项
    int i = 0;
    //取出目录项数
    int cnt = cur.link_num;

    //判断文件模式。6为owner，3为group，0为other
    int filemode;
    if(cur_user_name == cur.user_name )
        filemode = 6;
    else if(cur_user_name == cur.group_name)
        filemode = 3;
    else
        filemode = 0;

    while(i < BLOCK_NUM_PER_INODE){
        Dir dir[Dir_ITEM_NUM_PER_BLOCK];
        if (cur.block_id0[i / 16] == -1) {
            i += 16;
            continue;
        }
        //取出磁盘块
        int parblockAddr = cur.block_id0[i/16];
        fseek(img.file_read,parblockAddr,SEEK_SET);
        fread(&dir,sizeof(dir),1,img.file_read);

        //输出该磁盘块中的所有目录项
        for(int j = 0; j < 16; j++) {
            if(strcmp(dir[j].file_name, name)==0){
                //取出该目录项的inode，判断该目录项是目录还是文件
                fseek(img.file_read ,dir[j].inodeAddr,SEEK_SET);
                fread(&tmp,sizeof(inode),1,img.file_read);

                if( ( (tmp.mode >> 9 ) & 1 ) == 1 ){
                    //找到该目录，判断是否具有进入权限
                    if( ((tmp.mode>>filemode>>0)&1)==0 && cur_user_name != "root" ){	//root用户所有目录都可以查看
                        //没有执行权限
                        cout <<"权限不足：无执行权限" << endl;
                        return ;
                    }

                    //找到该目录项，如果是目录，更换当前目录

                    Cur_Dir_Addr = dir[j].inodeAddr;
                    if( strcmp(dir[j].file_name,".")==0){
                        //本目录，不动
                    }
                    else if(strcmp(dir[j].file_name,"..")==0){
                        //上一次目录
                        int k;
                        for(k = strlen(Cur_Dir_Name); k >= 0; k--)
                            if(Cur_Dir_Name[k] == '/')
                                break;
                        Cur_Dir_Name[k] = '\0';
                        if(strlen(Cur_Dir_Name) == 0)
                            Cur_Dir_Name[0]='/', Cur_Dir_Name[1] = '\0';
                    }
                    else{
                        if(Cur_Dir_Name[strlen(Cur_Dir_Name)-1]!='/')
                            strcat(Cur_Dir_Name,"/");
                        strcat(Cur_Dir_Name, dir[j].file_name);
                    }

                    return ;
                }

            }

            i++;
        }

    }

    //没找到
    cout <<"没有该目录" << endl;
    return ;

}
