//
// Created by Opynicus on 2022-4-15.
//

#include <iostream>
#include "fs.h"

/*
 *  文件系统初始化，将已经初始化的SuperBlock与BitMap传入fs类
 *  初始状态时默认路径为root
 */

fs::fs(image &_image, superBlock* super_block, bitMap* bit_map) : img(_image), super_block(super_block), bit_map(bit_map) {
    nextGID = 0;
    nextUID = 0;
    is_login = false;
    strcpy(cur_user_name, "root");
    strcpy(cur_group_name, "root");
    cur_dir_addr = ROOT_DIR_ADDR;
    strcpy(cur_dir_name, "/");
}
/*
 *  格式化img
 */

bool fs::Format() {
    //BitMap信息写入image
    fseek(img.file_write, INODE_BITMAP_START_ADDR, SEEK_SET);
    fwrite(bit_map->inode_bitmap,sizeof(bit_map->inode_bitmap),1, img.file_write);

    fseek(img.file_write,BLOCK_BITMAP_START_ADDR,SEEK_SET);
    fwrite(bit_map->block_bitmap,sizeof(bit_map->block_bitmap),1, img.file_write);
    //成组链接
    super_block->initFreeBlockStack(img.file_write);
    //SuperBlock写入img
    super_block->writeSuperBlock2img(img.file_write);

    //创建根目录root
    inode root;

    //为根目录root分配inode
    int root_addr = iAlloc();

    //给这个inode分配block
    int block_addr = bAlloc();

    //在这个磁盘块里加入条目 "."，表示当前目录
    Dir dir[Dir_ITEM_NUM_PER_BLOCK] = {0};
    strcpy(dir[0].file_name,".");
    dir[0].inodeAddr = root_addr;

    //保存当前block位置
    fseek(img.file_write, block_addr, SEEK_SET);
    fwrite(dir, sizeof(dir),1, img.file_write);

    //定义root目录信息(inode)
    root.inode_id = 0;
    strcpy(root.user_name, cur_user_name);
    strcpy(root.group_name, cur_group_name);
    root.link_num = 1;	                                                                               //当前目录,"."
    memset(root.block_id0,-1,sizeof(root.block_id0));
    root.block_id0[0] = block_addr;                                                                    //根目录指向
    root.size = super_block->block_size;
    root.block_id1 = -1;	                                                                           //没使用一级间接块
    root.mode = MODE_DIR | DIR_DEF_PERMISSION;                                                         //初始化mode

    //保存root的inode信息
    fseek(img.file_write, root_addr,SEEK_SET);
    fwrite(&root, sizeof(inode), 1, img.file_write);
    fflush(img.file_write);

    //创建目录及配置文件
    mkdir(ROOT_DIR_ADDR,"home");	//用户目录
    cd(ROOT_DIR_ADDR,"home");

    mkdir(cur_dir_addr,"root");
    chmod(cur_dir_addr, "root", 0660);


    cd(cur_dir_addr,"..");
    mkdir(cur_dir_addr,"etc");	//配置文件目录
    cd(cur_dir_addr,"etc");

    char buf[1000] = {0};

    //增加条目，用户名：加密密码：用户ID：用户组ID
    sprintf(buf,"root:x:%d:%d\n", nextUID++, nextGID++);
    //创建用户信息文件
    create(cur_dir_addr, "user", buf);
    //增加条目，用户名：密码
    sprintf(buf,"root:root\n");
    //创建用户密码文件
    create(cur_dir_addr,"passwd",buf);
    //修改权限，禁止其它用户读取该文件
    chmod(cur_dir_addr,"passwd", 0660);

    //建立用户组，用户组名：口令（一般为空，这里没有使用）：组标识号：组内用户列表（用，分隔）
    sprintf(buf,"root::0:root\n");	//增加管理员用户组
    sprintf(buf+strlen(buf),"user::1:\n");	//增加普通用户组，组内用户列表为空
    create(cur_dir_addr,"group",buf);	//创建用户组信息文件

    cd(cur_dir_addr,"..");	//回到根目录
    return true;
}

/*
 *  cd命令，打开下一个目录项。
 *  params: 当前目录地址，待打开目录项
 *  访问权限不足，            return 1
 *  成功找到目录，cd命令成功， return 0
 *  没找到目录，              return -1
 */

int fs::cd(int parent_inode_addr, const char *name) {
    //取出当前目录的inode信息
    inode cur;
    fseek(img.file_read, parent_inode_addr, SEEK_SET);
    fread(&cur, sizeof(inode), 1, img.file_read);
    int filemode;   //判定文件属性

    int i = 0;
    while(i < BLOCK_NUM_PER_INODE) {    //在当前inode下的所有block中寻找
        //一个dir存32B，定义大小为Dir_ITEM_NUM_PER_BLOCK的数组可以对一整个Block操作 (32 * Dir_ITEM_NUM_PER_BLOCK = BlockSize)
        Dir dir[Dir_ITEM_NUM_PER_BLOCK] = {0};
        if(cur.block_id0[i / Dir_ITEM_NUM_PER_BLOCK] == -1) {   //直接块找到了为空的区域
            i += Dir_ITEM_NUM_PER_BLOCK;                        //查找下一个Dir
            continue;
        }
        //取出该组Dir块
        int token_block = cur.block_id0[i / Dir_ITEM_NUM_PER_BLOCK];
        fseek(img.file_read, token_block, SEEK_SET);
        fread(&dir, sizeof(dir), 1, img.file_read);

        //比较该组Dir块中的所有Dir
        for(int j = 0; j < Dir_ITEM_NUM_PER_BLOCK; j++) {
            if(strcmp(dir[j].file_name, name) == 0 ) {      //找到输入name对应的目录
                inode tmp;
                //取出该目录项的inode，判断该目录项是目录还是文件
                fseek(img.file_read, dir[j].inodeAddr, SEEK_SET);
                fread(&tmp, sizeof(inode),1,img.file_read);

                if( ( (tmp.mode >> 9) & 1 ) == 1 ) {     //文件为目录
                    if (!isPermit(cur)) {
                        cout << "权限不足" << endl;
                        return 1;
                    }
//                    if(strcmp(cur_user_name, tmp.user_name) == 0 )
//                        filemode = 6;
//                    else if(strcmp(cur_group_name, tmp.group_name) == 0)
//                        filemode = 3;
//                    else
//                        filemode = 0;
//
//                    if (strcmp(cur_user_name, "root") == 0) {/*root用户*/}
//                    else if( ((tmp.mode >> filemode >> 2) & 1) ==0  ) {
//                        cout << "权限不足" << endl;
//                        return 1;   //终止访问，返回1
//                    }
                    //找到该目录项，如果是目录，更换当前目录
                    //当前目录设置为找到的目录，首先修改对应的目录地址(id)
                    cur_dir_addr = dir[j].inodeAddr;
                    if(strcmp(dir[j].file_name, ".") == 0 ) {/*本目录，不动*/}
                    else if(strcmp(dir[j].file_name, "..") == 0) {
                        //上一级目录
                        int k;
                        for(k = strlen(cur_dir_name); k >= 0; k--) {
                            if(cur_dir_name[k] == '/') {
                                break;
                            }
                        }
                        cur_dir_name[k] = '\0';
                        if(strlen(cur_dir_name) == 0) {
                            //根目录
                            cur_dir_name[0] = '/';
                            cur_dir_name[1] = '\0';
                        }
                    }
                    else {  //是子目录，直接连接
                        if(cur_dir_name[strlen(cur_dir_name) - 1] != '/' ) {
                            strcat(cur_dir_name, "/");  //末尾加上'/'
                        }
                        strcat(cur_dir_name, dir[j].file_name);
                    }
                    return 0;   //cd成功，返回0
                }
            }
            i++;
        }
    }
    //没找到目录
    cout << "找不到该目录" << endl;
    return -1;


}

/*
 *  mkdir命令，作用是创建子目录
 *  params: 当前目录地址，待建立目录名称
 *  超过长度         return -1
 *  权限不足         return 1
 *  成功             return 0
 *  目录已存在       return 2
 *  inode分配失败    return 3
 *  block分配失效    return 4
 *  无空闲目录项     return 5
 */

int fs::mkdir(int parent_inode_addr, char *name) {
    if(strlen(name) >= MAX_FILE_NAME) {
        cout << "超过最大目录长度" << endl;
        return -1;
    }
    Dir dir[Dir_ITEM_NUM_PER_BLOCK];    //取Dir组
    inode cur;                          //取当前目录inode信息
    fseek(img.file_read, parent_inode_addr, SEEK_SET);
    fread(&cur, sizeof(inode), 1, img.file_read);
    if (!isPermit(cur)) {
        cout << "权限不足" << endl;
        return 1;
    }

    int i = 0;
    int find_pos_i = -1, find_pos_j = -1;
    // BLOCK_NUM_PER_INODE -> 10 直接 * Dir_ITEM_NUM_PER_BLOCK = BLOCK_NUM_PER_INODE 个目录项
    while(i < BLOCK_NUM_PER_INODE) {
        int dir_in_block = i / Dir_ITEM_NUM_PER_BLOCK;
        if(cur.block_id0[dir_in_block] == -1) {
            i += Dir_ITEM_NUM_PER_BLOCK;
            continue;
        }

        fseek(img.file_read, cur.block_id0[dir_in_block], SEEK_SET);
        fread(dir, sizeof(dir), 1, img.file_read);
        fflush(img.file_read);

        for(int j = 0; j < Dir_ITEM_NUM_PER_BLOCK; j++) {
            if(strcmp(dir[j].file_name, name) == 0) {
                inode tmp;
                fseek(img.file_read, dir[j].inodeAddr, SEEK_SET);
                fread(&tmp, sizeof(inode), 1, img.file_read);
                if ((( tmp.mode >> 9) & 1 )) {
                    cout << "目录已存在" << endl;
                    return 2;
                }
            } else {
                if(strcmp(dir[j].file_name, "") == 0) {
                    if(find_pos_i == -1) {
                        find_pos_i = dir_in_block;
                        find_pos_j = j;
                    }
                }
            }
        }
        i++;
    }

    //找到空闲位置
    if(find_pos_i != -1) {
        //取出这个直接块，要加入的目录条目的位置
        fseek(img.file_read, cur.block_id0[find_pos_i], SEEK_SET);
        fread(dir, sizeof(dir), 1, img.file_read);
        fflush(img.file_read);

        //创建这个目录项
        strcpy(dir[find_pos_j].file_name, name);	//目录名

        //写入两条记录 "." ".."，分别指向当前inode节点地址，和父inode节点
        int cur_inode_addr = iAlloc();	//分配当前节点地址
        if(cur_inode_addr == -1){
            cout << "inode节点不够" << endl;
            return 3;
        }
        dir[find_pos_j].inodeAddr = cur_inode_addr; //给这个新的目录分配的inode地址

        //创建对应的新inode
        inode p;
        p.inode_id = static_cast<unsigned short>((cur_inode_addr - INODE_TABLE_START_ADDR) / super_block->inode_size);
        strcpy(p.user_name, cur_user_name);
        strcpy(p.group_name, cur_group_name);
        p.link_num = 2;	    //两个项，当前目录, "."和".."

        //分配当前inode的blocks，写入两条记录 "." 和 ".."
        int curblockAddr = bAlloc();
        if(curblockAddr == -1) {
            cout << "block分配失败" << endl;
            return 4;
        }
        Dir dir2[Dir_ITEM_NUM_PER_BLOCK] = {0};
        strcpy(dir2[0].file_name, ".");
        strcpy(dir2[1].file_name, "..");
        dir2[0].inodeAddr = cur_inode_addr;	            //当前目录inode地址
        dir2[1].inodeAddr = parent_inode_addr;	                //父目录inode地址

        //写入到当前目录的Block，一次写满一个Block
        fseek(img.file_write, curblockAddr,SEEK_SET);
        fwrite(dir2, sizeof(dir2), 1, img.file_write);

        //让新建的inode的第一个直接块指向被分配的Block地址
        memset(p.block_id0,-1,sizeof(p.block_id0));
        p.block_id0[0] = curblockAddr;

        p.size = super_block->block_size;
        p.block_id1 = -1;
        p.mode = MODE_DIR | DIR_DEF_PERMISSION;

        //保存inode至分配地址
        fseek(img.file_write, cur_inode_addr, SEEK_SET);
        fwrite(&p, sizeof(inode), 1, img.file_write);

        //将当前目录的Block内容写回原来位置
        fseek(img.file_write, cur.block_id0[find_pos_i],SEEK_SET);
        fwrite(dir, sizeof(dir), 1, img.file_write);

        //保存inode
        cur.link_num++;
        fseek(img.file_write, parent_inode_addr, SEEK_SET);
        fwrite(&cur,sizeof(inode), 1, img.file_write);
        fflush(img.file_write);
        return 0;
    }
    else{
        cout << "无空闲目录项" << endl;
        cout << "没找到空闲目录项,目录创建失败" << endl;
        return 5;
    }
}

/*
 * inode分配
 * 无法分配 return -1
 * 否则返回分配地址
 */

int fs::iAlloc() {
    if(super_block->free_inode_num == 0) {
        cout << "没有空闲inode可以分配" << endl;
        return -1;
    } else {
        int pos = 0;
        for(int i = 0; i < super_block->inode_num; i++) {
            if(bit_map->inode_bitmap[i] == 0) {
                pos = i;
                break;
            }
        }
        super_block->free_inode_num--;
        fseek(img.file_write, SUPERBLOCK_START_ADDR, SEEK_SET);
        fwrite(super_block, sizeof(super_block), 1, img.file_write);
        bit_map->inode_bitmap[pos] = true;
        fseek(img.file_write, INODE_BITMAP_START_ADDR + pos, SEEK_SET);
        fwrite(&bit_map->inode_bitmap[pos], sizeof(bool), 1, img.file_write);
        fflush(img.file_write);

        return INODE_TABLE_START_ADDR + pos * super_block->inode_size;
    }
}

/*
 * Block分配
 * 无法分配 return -1
 * 否则返回成组连接栈栈顶指向地址
 */
int fs::bAlloc() {
    int top; //栈顶指针
    if (super_block->free_block_num==0) {
        cout << "已经没有空闲块分配" << endl;
        return -1;
    }
    else{
        //如果已经是最后一块，此时top应该为0
        top = (super_block->free_block_num-1) % MAX_FREE_BLOCKS;
    }
    //将栈顶取出
    //如果已是栈底，将当前块号地址返回，即为栈底块号，并将栈底指向的新空闲块堆栈覆盖原来的栈
    int alloc_addr;
    if(top == 0){
        alloc_addr = super_block->free_addr;
        super_block->free_addr = super_block->free_block_stack[0];	//取出下一个存有空闲块堆栈的空闲块的位置，更新空闲块堆栈指针

        //取出对应空闲块内容，覆盖原来的空闲块堆栈
        //取出下一个空闲块堆栈，覆盖原来的
        fseek(img.file_read, super_block->free_addr, SEEK_SET);
        fread(super_block->free_block_stack, sizeof(super_block->free_block_stack), 1, img.file_read);
        fflush(img.file_read);
        super_block->free_block_num--;

    }
    else{	//如果不为栈底，则将栈顶指向的地址返回，栈顶指针-1.
        alloc_addr = super_block->free_block_stack[top];	//保存返回地址
        super_block->free_block_stack[top] = -1;	//清栈顶
        top--;		//栈顶指针-1
        super_block->free_block_num--;	//空闲块数-1

    }
    //更新超级块
    fseek(img.file_write, SUPERBLOCK_START_ADDR, SEEK_SET);
    fwrite(super_block, sizeof(superBlock), 1, img.file_write);
    fflush(img.file_write);

    //更新block位图
    bit_map->block_bitmap[(alloc_addr - DATA_BLOCK_START_ADDR) / BLOCK_SIZE] = true;
    fseek(img.file_write, (alloc_addr - DATA_BLOCK_START_ADDR) / BLOCK_SIZE + BLOCK_BITMAP_START_ADDR, SEEK_SET);
    fwrite(&bit_map->block_bitmap[(alloc_addr - DATA_BLOCK_START_ADDR) / BLOCK_SIZE], sizeof(bool), 1, img.file_write);
    fflush(img.file_write);

    return alloc_addr;

}

/*
 * create内部创建文件，外部无法访问
 * params: 当前目录，文件名，文件内容
 * 超过最大文件名      return -1
 * 成功               return 0
 * 文件已经存在       return 1
 * inode分配失败    return 2
 * block分配失败    return 3
 */

int fs::create(int parent_inode_addr, const char *name, char *file_content) {
    if(strlen(name) >= MAX_FILE_NAME){
        cout << "超过最大文件名" << endl;
        return -1;
    }
    Dir dir[Dir_ITEM_NUM_PER_BLOCK];

    //从这个地址取出inode
    inode cur;
    fseek(img.file_read, parent_inode_addr, SEEK_SET);
    fread(&cur, sizeof(inode), 1, img.file_read);

    int i = 0;
    int find_pos_i = -1, find_pos_j = -1;
    while(i < BLOCK_NUM_PER_INODE) {

        if(cur.block_id0[i / Dir_ITEM_NUM_PER_BLOCK] == -1) {
            i += Dir_ITEM_NUM_PER_BLOCK;
            continue;
        }
        fseek(img.file_read, cur.block_id0[i / Dir_ITEM_NUM_PER_BLOCK], SEEK_SET);
        fread(dir, sizeof(dir), 1, img.file_read);
        fflush(img.file_read);

        //输出该磁盘块中的所有目录项
        for(int j = 0; j < Dir_ITEM_NUM_PER_BLOCK; j++) {

            if( find_pos_i == -1 && strcmp(dir[j].file_name, "") == 0 ) {
                //找到一个空闲记录，将新文件创建到这个位置
                find_pos_i = i / Dir_ITEM_NUM_PER_BLOCK;
                find_pos_j = j;
            }
            else if(strcmp(dir[j].file_name, name) == 0 ) {
                //重名，取出inode，判断是否是文件
                inode tmp;
                fseek(img.file_read, dir[j].inodeAddr, SEEK_SET);
                fread(&tmp, sizeof(inode), 1, img.file_read);
                if( ((tmp.mode>>9) &1)==0 ){	//是文件且重名，不能创建文件
                    cout << "文件已经存在" << endl;
                    file_content[0] = '\0';
                    return 1;
                }
            }
            i++;
        }

    }
    if(find_pos_i != -1) {
        //取出之前那个空闲目录项对应的磁盘块
        fseek(img.file_read, cur.block_id0[find_pos_i], SEEK_SET);
        fread(dir, sizeof(dir), 1, img.file_read);
        fflush(img.file_read);

        //创建这个目录项
        strcpy(dir[find_pos_j].file_name, name);
        int cur_inode_addr = iAlloc();
        if(cur_inode_addr == -1){
            cout << "INode分配失败" << endl;
            return 2;
        }
        dir[find_pos_j].inodeAddr = cur_inode_addr;

        //设置新条目的inode
        inode p;
        p.inode_id = static_cast<unsigned short>((cur_inode_addr - INODE_TABLE_START_ADDR) / super_block->inode_size);
        strcpy(p.user_name, cur_user_name);
        strcpy(p.group_name, cur_group_name);
        p.link_num = 1;	//只有一个文件指向


        int k;

        //将buf内容存到磁盘块
        int file_size = static_cast<int>(strlen(file_content));
        for(int k = 0; k < file_size; k += super_block->block_size) {
            int cur_block_Addr = bAlloc();
            if(cur_block_Addr == -1){
                cout << "Block分配失败" << endl;
                return 3;
            }
            p.block_id0[k / super_block->block_size] = cur_block_Addr;
            fseek(img.file_write, cur_block_Addr,SEEK_SET);
            fwrite(file_content + k, super_block->block_size, 1, img.file_write);
        }


        //对其他项赋值为-1
        for(k= file_size / super_block->block_size + 1; k < 10; k++) {
            p.block_id0[k] = -1;
        }


        if( file_size == 0) {	//长度为0的话也分给它一个block
            int cur_block_Addr = bAlloc();
            if(cur_block_Addr == -1) {
                cout << "Block分配失败" << endl;
                return 3;
            }
            p.block_id0[k / super_block->block_size] = cur_block_Addr;
            //写入到当前目录的磁盘块
            fseek(img.file_write, cur_block_Addr, SEEK_SET);
            fwrite(file_content, super_block->block_size, 1, img.file_write);

        }
        p.size = file_size;
        p.block_id1 = -1;
        p.mode = 0;
        p.mode = MODE_FILE | FILE_DEF_PERMISSION;

        //将inode写入到申分配地址
        fseek(img.file_write, cur_inode_addr, SEEK_SET);
        fwrite(&p, sizeof(inode), 1, img.file_write);

        //将当前目录的Block写回
        fseek(img.file_write, cur.block_id0[find_pos_i], SEEK_SET);
        fwrite(dir, sizeof(dir), 1, img.file_write);

        //保存inode
        cur.link_num++;
        fseek(img.file_write, parent_inode_addr, SEEK_SET);
        fwrite(&cur,sizeof(inode), 1, img.file_write);
        fflush(img.file_write);
        return 0;
    }
    else
        return 4;
}

/*
 * chmod命令，切换权限
 */

void fs::chmod(int parent_inode_addr, const char *name, int mode) {

    if(strcmp(name,".") == 0 || strcmp(name,"..") == 0) {
        cout << "usage: chmod [filename] [permissions] : Change the file permissions" << endl;
        return ;
    }

    inode cur, res;
    fseek(img.file_read, parent_inode_addr, SEEK_SET);
    fread(&cur,sizeof(inode), 1, img.file_read);
    int i = 0, j = 0;
    Dir dir[Dir_ITEM_NUM_PER_BLOCK] = {0};
    bool flag = false;      //纯粹为了跳出循环设置的flag是屑
    while(i < BLOCK_NUM_PER_INODE) {
        if(cur.block_id0[i / Dir_ITEM_NUM_PER_BLOCK] == -1){
            i += Dir_ITEM_NUM_PER_BLOCK;
            continue;
        }
        //取出磁盘块
        fseek(img.file_read, cur.block_id0[i / Dir_ITEM_NUM_PER_BLOCK], SEEK_SET);
        fread(&dir, sizeof(dir), 1, img.file_read);
        fflush(img.file_read);

        //输出该磁盘块中的所有目录项
        for(j = 0; j < Dir_ITEM_NUM_PER_BLOCK; j++) {
            if( strcmp(dir[j].file_name, name) == 0 ) {	//找到该目录或者文件
                //取出对应的inode
                fseek(img.file_read, dir[j].inodeAddr, SEEK_SET);
                fread(&res, sizeof(inode),1,img.file_read);
                fflush(img.file_read);
                flag = true;
            }
            if(flag) break;
        }
        if(flag) break;
        i++;
    }
    if(i >= BLOCK_NUM_PER_INODE) {
        cout << "当前文件名不存在" << endl;
        return ;
    }
    if(strcmp(cur_user_name, res.user_name) != 0 && strcmp(cur_user_name, "root") != 0) {   //切换不成功（对应用户与当前目录不匹配且当前不是root用户）
        cout << "" << endl;
        return ;
    }

    //模式切换
    res.mode  = static_cast<unsigned short>((res.mode >> 9 << 9) | mode);

    //修改对应目录inode信息并保存
    fseek(img.file_write, dir[j].inodeAddr,SEEK_SET);
    fwrite(&res, sizeof(inode), 1, img.file_write);
    fflush(img.file_write);
}

/*
 *  ls命令，显示该目录下所有文件夹与文件
 *  params: 当前目录地址
 */

void fs::ls(int parent_inode_addr) {
    inode cur;
    //取当前目录inode
    fseek(img.file_read, parent_inode_addr, SEEK_SET);
    fread(&cur,sizeof(inode), 1, img.file_read);
    fflush(img.file_read);


    //取出目录项数
    int cnt = cur.link_num;

//    int file_mode;
//    if(strcmp(cur_user_name, cur.user_name)==0 || strcmp(cur_user_name, "root") == 0)
//        file_mode = 6;
//    else if(strcmp(cur_group_name, cur.group_name)==0)
//        file_mode = 3;
//    else
//        file_mode = 0;
//
//
//    if (strcmp(cur_user_name, "root") == 0) {
//
//    } else if( ((cur.mode >> file_mode >> 2) & 1) ==0  ){
//        cout << "权限不足" << endl;
//        return ;
//    }

    //判断文件模式。
    if (!isPermit(cur)) {
        cout << "权限不足" << endl;
        return ;
    }

    //依次取出磁盘块
    int i = 0;
    while(i < cnt && i<BLOCK_NUM_PER_INODE) {
        Dir dir[Dir_ITEM_NUM_PER_BLOCK] = {0};
        if(cur.block_id0[i/Dir_ITEM_NUM_PER_BLOCK] == -1) {
            i += Dir_ITEM_NUM_PER_BLOCK;
            continue;
        }

        fseek(img.file_read,cur.block_id0[i / Dir_ITEM_NUM_PER_BLOCK], SEEK_SET);
        fread(&dir, sizeof(dir), 1, img.file_read);
        fflush(img.file_read);

        for(int j = 0; j < Dir_ITEM_NUM_PER_BLOCK && i < cnt; j++) {

            inode tmp;
            //取出该目录项的inode，判断该目录项是目录还是文件
            fseek(img.file_read, dir[j].inodeAddr, SEEK_SET);
            fread(&tmp, sizeof(inode),1,img.file_read);
            fflush(img.file_read);

            if( strcmp(dir[j].file_name, "")==0 ){
                continue;
            }

            if(i > 2 && (strcmp(dir[j].file_name, ".")==0 || strcmp(dir[j].file_name, "..")== 0) ) {
                continue;
            }

            //输出信息
            if( ( (tmp.mode>>9) & 1 ) == 1 ){
                cout << "d";
            }
            else{
                cout << "-";
            }


            int permiss_index = 8;
            while(permiss_index >= 0){
                if( ((tmp.mode >> permiss_index) & 1) == 1){
                    if(permiss_index % 3 == 2)	cout << "r";
                    if(permiss_index % 3 == 1)	cout << "w";
                    if(permiss_index % 3 == 0)	cout << "x";
                }
                else{
                    cout << "-";
                }
                permiss_index--;
            }
            cout << "\t";
            cout << tmp.link_num << "\t";	//该文件链接
            cout << tmp.user_name << "\t";	//文件所属用户名
            cout << tmp.group_name << "\t";	//文件所属用户名
            cout << tmp.size << " B\t";	//文件大小
            cout << dir[j].file_name;	//文件名
            cout << endl;
            i++;
        }

    }
}

/*
 *  判断是否有对当前目录的访问权限
 *  有权限返回true，没有权限返回false
 *  目录访问权限：
*   root可以访问所有
*   owner 6
*   group 3
*   other 0
*/

bool fs::isPermit(inode &cur) {
    int file_mode;
    if(strcmp(cur_user_name, cur.user_name)==0 || strcmp(cur_user_name, "root") == 0)
        file_mode = 6;
    else if(strcmp(cur_group_name, cur.group_name)==0)
        file_mode = 3;
    else
        file_mode = 0;

    if( ((cur.mode >> file_mode >> 2) & 1) == 0) {
        return false;
    } else {
        return true;
    }
}
