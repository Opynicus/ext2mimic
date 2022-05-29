//
// Created by Opynicus on 2022-4-15.
//

#include <iostream>
#include "../include/fs.h"

/*
 * 加载img文件
 */
void fs::loadFs() {
  //读取superblock
  if (img.file_read == nullptr)
    return;
  fseek(img.file_read, SUPERBLOCK_START_ADDR, SEEK_SET);
  fread(&super_block, sizeof(superBlock), 1, img.file_read);

  //super_block.printSuperBlockInfo();

  //读取inode位图
  fseek(img.file_read, INODE_BITMAP_START_ADDR, SEEK_SET);
  fread(bit_map.inode_bitmap, sizeof(bit_map.inode_bitmap), 1, img.file_read);

  //读取block位图
  fseek(img.file_read, BLOCK_BITMAP_START_ADDR, SEEK_SET);
  fread(bit_map.block_bitmap, sizeof(bit_map.block_bitmap), 1, img.file_read);
}

/*
 *  格式化img
 */
bool fs::format() {
  //BitMap信息写入image
  fseek(img.file_write, INODE_BITMAP_START_ADDR, SEEK_SET);
  fwrite(bit_map.inode_bitmap, sizeof(bit_map.inode_bitmap), 1, img.file_write);

  fseek(img.file_write, BLOCK_BITMAP_START_ADDR, SEEK_SET);
  fwrite(bit_map.block_bitmap, sizeof(bit_map.block_bitmap), 1, img.file_write);
  //成组链接
  super_block.initFreeBlockStack(img.file_write);
  //SuperBlock写入img
  super_block.writeSuperBlock2img(img.file_write);

  //创建根目录root
  inode root{};

  //为根目录root分配inode
  int root_addr = iAlloc();

  //给这个inode分配block
  int block_addr = bAlloc();

  //在这个磁盘块里加入条目 "."，表示当前目录
  Dir dir[Dir_ITEM_NUM_PER_BLOCK] = {0};
  strcpy(dir[0].file_name, ".");
  dir[0].inodeAddr = root_addr;

  //保存当前block位置
  fseek(img.file_write, block_addr, SEEK_SET);
  fwrite(dir, sizeof(dir), 1, img.file_write);

  //定义root目录信息(inode)
  root.inode_id = 0;
  strcpy(root.user_name, cur_user_name);
  strcpy(root.group_name, cur_group_name);
  root.link_num = 1;                                                                                   //当前目录, "."
  memset(root.block_id0, -1, sizeof(root.block_id0));
  root.block_id0[0] = block_addr;                                                                    //根目录指向
  root.size = super_block.block_size;
  root.block_id1 = -1;                                                                               //一级间接块
  root.mode = MODE_DIR | DIR_DEF_PERMISSION;                                                         //初始化mode

  //保存root的inode信息
  fseek(img.file_write, root_addr, SEEK_SET);
  fwrite(&root, sizeof(inode), 1, img.file_write);
  fflush(img.file_write);

  //创建目录及配置文件
  mkdir(ROOT_DIR_ADDR, "home");    //用户目录
  cd(ROOT_DIR_ADDR, "home");

  mkdir(cur_dir_addr, "root");
  chmod(cur_dir_addr, "root", 0660);

  cd(cur_dir_addr, "..");
  mkdir(cur_dir_addr, "etc");    //配置文件目录
  cd(cur_dir_addr, "etc");

  char buf[1000] = {0};

  //增加条目，用户名：加密密码：用户ID：用户组ID
  sprintf(buf, "root:x:%d:%d\n", nextUID++, nextGID++);
  //创建用户信息文件
  create(cur_dir_addr, "user", buf);
  //输入root密码
  cout << endl << "This is a brand new file system, please input root password." << endl;
  string root_map = "root:" + getPasswdConfirm(MAX_PASSWD_LEN) + "\n";
  //增加条目，用户名：密码
  sprintf(buf, root_map.c_str());
  //创建用户密码文件
  create(cur_dir_addr, "passwd", buf);
  //修改权限，禁止其它用户读取该文件
  chmod(cur_dir_addr, "passwd", 0660);

  //建立用户组，用户组名：口令（一般为空，这里没有使用）：组标识号：组内用户列表（用，分隔）
  sprintf(buf, "root::0:root\n");    //增加管理员用户组
  sprintf(buf + strlen(buf), "user::1:\n");    //增加普通用户组，组内用户列表为空
  create(cur_dir_addr, "group", buf);    //创建用户组信息文件

  cd(cur_dir_addr, "..");    //回到根目录
  return true;
}

/*
 *  文件系统初始化，将已经初始化的SuperBlock与BitMap传入fs类
 *  初始状态时默认路径为root
 */
fs::fs(image &_image, superBlock &super_block, bitMap &bit_map) : img(_image), super_block(super_block),
                                                                  bit_map(bit_map) {
  nextGID = 0;
  nextUID = 0;
  is_login = false;
  strcpy(cur_user_name, "root");
  strcpy(cur_group_name, "root");
  cur_dir_addr = ROOT_DIR_ADDR;
  strcpy(cur_dir_name, "/");
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
  inode cur{};
  fseek(img.file_read, parent_inode_addr, SEEK_SET);
  fread(&cur, sizeof(inode), 1, img.file_read);
  int filemode;   //判定文件属性

  int i = 0;
  while (i < BLOCK_NUM_PER_INODE) {    //在当前inode下的所有block中寻找
    Dir dir[Dir_ITEM_NUM_PER_BLOCK] = {0};
    if (cur.block_id0[i / Dir_ITEM_NUM_PER_BLOCK] == -1) {   //直接块找到了为空的区域
      i += Dir_ITEM_NUM_PER_BLOCK;                        //查找下一个Dir
      continue;
    }
    //取出该组Dir块
    int token_block = cur.block_id0[i / Dir_ITEM_NUM_PER_BLOCK];
    fseek(img.file_read, token_block, SEEK_SET);
    fread(&dir, sizeof(dir), 1, img.file_read);

    //比较该组Dir块中的所有Dir
    for (auto &j : dir) {
      if (strcmp(j.file_name, name) == 0) {      //找到输入name对应的目录
        inode tmp{};
        //取出该目录项的inode，判断该目录项是目录还是文件
        fseek(img.file_read, j.inodeAddr, SEEK_SET);
        fread(&tmp, sizeof(inode), 1, img.file_read);

        if (((tmp.mode >> 9) & 1) == 1) {     //文件为目录
          if (!isPermitRead(cur)) {
            cout << "WARNING: Permission denied(NO READ AUTHORITY)" << endl;
            return 1;
          }

          cur_dir_addr = j.inodeAddr;
          if (strcmp(j.file_name, ".") == 0) {/*本目录，不动*/}
          else if (strcmp(j.file_name, "..") == 0) {
            //上一级目录
            int k;
            for (k = strlen(cur_dir_name); k >= 0; k--) {
              if (cur_dir_name[k] == '/') {
                break;
              }
            }
            cur_dir_name[k] = '\0';
            if (strlen(cur_dir_name) == 0) {
              //根目录
              cur_dir_name[0] = '/';
              cur_dir_name[1] = '\0';
            }
          } else {  //是子目录，直接连接
            if (cur_dir_name[strlen(cur_dir_name) - 1] != '/') {
              strcat(cur_dir_name, "/");  //末尾加上'/'
            }
            strcat(cur_dir_name, j.file_name);
          }
          return 0;   //cd成功，返回0
        }
      }
      i++;
    }
  }
  //没找到目录
  cout << "can't find dir" << endl;
  return -1;
}

int fs::cda(int parent_inode_addr, const char *name) {
  int temp_cur_dir = cur_dir_addr;
  char temp_cur_dir_name[MAX_FILE_NAME];
  strcpy(temp_cur_dir_name, cur_dir_name);
  bool is_access = true;
  vector<string> split_dir = splitDir(name, cur_user_dir_name);
  int n = split_dir.size();
  string parsed_dir = parseDir(name, cur_user_dir_name);

  if (split_dir[0] == "/") {
    //绝对地址寻址
    cur_dir_addr = ROOT_DIR_ADDR;
    strcpy(cur_dir_name, "/");
    for (int i = 1; i < n; i++) {
      if (cd(cur_dir_addr, split_dir[i].c_str()) == 0) {
        continue;
      } else {
        is_access = false;
        break;
      }
    }
  } else {
    //相对地址寻址
    for (int i = 0; i < n; i++) {
      if (cd(cur_dir_addr, split_dir[i].c_str()) == 0) {
        continue;
      } else {
        is_access = false;
        break;
      }
    }
  }

  if (!is_access) {
    cur_dir_addr = temp_cur_dir;
    strcpy(cur_dir_name, temp_cur_dir_name);
    return 1;
  }

  return 0;
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
  if (strlen(name) >= MAX_FILE_NAME) {
    cout << "Exceeded max file name length" << endl;
    return -1;
  }
  Dir dir[Dir_ITEM_NUM_PER_BLOCK];    //取Dir组
  inode cur{};                          //取当前目录inode信息
  fseek(img.file_read, parent_inode_addr, SEEK_SET);
  fread(&cur, sizeof(inode), 1, img.file_read);
  if (!isPermitRead(cur)) {
    cout << "WARNING: Permission denied(NO READ AUTHORITY)" << endl;
    return 1;
  }

  int i = 0;
  int find_pos_i = -1, find_pos_j = -1;

  int first_empty_id0 = -1;//第一个找到的全空的id0
  bool is_first_empty_id0 = true;//用于first_empty_id0的寻找

  // BLOCK_NUM_PER_INODE . BLOCK_ID0_NUM * Dir_ITEM_NUM_PER_BLOCK = BLOCK_NUM_PER_INODE 个目录项
  while (i < BLOCK_NUM_PER_INODE) {
    int dir_in_block = i / Dir_ITEM_NUM_PER_BLOCK;
    if (cur.block_id0[dir_in_block] == -1) {
      if (is_first_empty_id0) {
        first_empty_id0 = i / Dir_ITEM_NUM_PER_BLOCK;
        is_first_empty_id0 = false;
      }
      i += Dir_ITEM_NUM_PER_BLOCK;
      continue;
    }

    fseek(img.file_read, cur.block_id0[dir_in_block], SEEK_SET);
    fread(dir, sizeof(dir), 1, img.file_read);
    fflush(img.file_read);

    for (int j = 0; j < Dir_ITEM_NUM_PER_BLOCK; j++) {
      if (strcmp(dir[j].file_name, name) == 0) {
        inode tmp{};
        fseek(img.file_read, dir[j].inodeAddr, SEEK_SET);
        fread(&tmp, sizeof(inode), 1, img.file_read);
        if (((tmp.mode >> 9) & 1)) {
          cout << "Dir already existed" << endl;
          return 2;
        }
      } else {
        if (strcmp(dir[j].file_name, "") == 0) {
          if (find_pos_i == -1) {
            find_pos_i = dir_in_block;
            find_pos_j = j;
          }
        }
      }
    }
    i++;
  }

  //找到空闲位置
  if (find_pos_i != -1) {
    //取出这个直接块，要加入的目录条目的位置
    fseek(img.file_read, cur.block_id0[find_pos_i], SEEK_SET);
    fread(dir, sizeof(dir), 1, img.file_read);
    fflush(img.file_read);

    //创建这个目录项
    strcpy(dir[find_pos_j].file_name, name);    //目录名

    //写入两条记录 "." ".."，分别指向当前inode节点地址，和父inode节点
    int cur_inode_addr = iAlloc();    //分配当前节点地址
    if (cur_inode_addr == -1) {
      cout << "ERROR: no more inodes" << endl;
      return 3;
    }
    dir[find_pos_j].inodeAddr = cur_inode_addr; //给这个新的目录分配的inode地址

    //创建对应的新inode
    inode p{};
    p.inode_id = static_cast<unsigned short>((cur_inode_addr - INODE_TABLE_START_ADDR) / super_block.inode_size);
    strcpy(p.user_name, cur_user_name);
    strcpy(p.group_name, cur_group_name);
    p.link_num = 2;        //两个项，当前目录, "."和".."

    //分配当前inode的block，写入两条记录 "." 和 ".."
    int cur_block_addr = bAlloc();
    if (cur_block_addr == -1) {
      cout << "ERROR: alloc block failed" << endl;
      return 4;
    }
    Dir dir2[Dir_ITEM_NUM_PER_BLOCK] = {0};
    strcpy(dir2[0].file_name, ".");
    strcpy(dir2[1].file_name, "..");
    dir2[0].inodeAddr = cur_inode_addr;                //当前目录inode地址
    dir2[1].inodeAddr = parent_inode_addr;            //父目录inode地址

    //写入到当前目录的Block，一次写满一个Block
    fseek(img.file_write, cur_block_addr, SEEK_SET);
    fwrite(dir2, sizeof(dir2), 1, img.file_write);

    //让新建的inode的第一个直接块指向被分配的Block地址
    memset(p.block_id0, -1, sizeof(p.block_id0));
    p.block_id0[0] = cur_block_addr;

    p.size = super_block.block_size;
    p.block_id1 = -1;
    p.mode = MODE_DIR | DIR_DEF_PERMISSION;
    p.create_time = time(nullptr);
    p.last_modified_time = time(nullptr);
    p.last_read_time = time(nullptr);

    //保存inode至分配地址
    fseek(img.file_write, cur_inode_addr, SEEK_SET);
    fwrite(&p, sizeof(inode), 1, img.file_write);

    //将当前目录的Block内容写回原来位置
    fseek(img.file_write, cur.block_id0[find_pos_i], SEEK_SET);
    fwrite(dir, sizeof(dir), 1, img.file_write);

    //保存inode
    cur.link_num++;
    fseek(img.file_write, parent_inode_addr, SEEK_SET);
    fwrite(&cur, sizeof(inode), 1, img.file_write);
    fflush(img.file_write);
    return 0;
  } else if (first_empty_id0 != -1) {
    //启用一个新的id0地址,并为止分配一个数据块来存储地址
    cur.block_id0[first_empty_id0] = bAlloc();

    Dir new_dir[Dir_ITEM_NUM_PER_BLOCK] = {0};
    strcpy(new_dir[0].file_name, name);
    int cur_inode_addr = iAlloc();
    if (cur_inode_addr == -1) {
      cout << "ERROR: alloc inode failed" << endl;
      return 2;
    }
    new_dir[0].inodeAddr = cur_inode_addr;

    inode p{};
    p.inode_id = static_cast<unsigned short>((cur_inode_addr - INODE_TABLE_START_ADDR) / super_block.inode_size);
    strcpy(p.user_name, cur_user_name);
    strcpy(p.group_name, cur_group_name);
    p.link_num = 2;        //两个项，当前目录, "."和".."

    //分配当前inode的block，写入两条记录 "." 和 ".."
    int cur_block_addr = bAlloc();
    if (cur_block_addr == -1) {
      cout << "ERROR: alloc block failed" << endl;
      return 4;
    }
    Dir dir2[Dir_ITEM_NUM_PER_BLOCK] = {0};
    strcpy(dir2[0].file_name, ".");
    strcpy(dir2[1].file_name, "..");
    dir2[0].inodeAddr = cur_inode_addr;                //当前目录inode地址
    dir2[1].inodeAddr = parent_inode_addr;            //父目录inode地址

    //写入到当前目录的Block，一次写满一个Block
    fseek(img.file_write, cur_block_addr, SEEK_SET);
    fwrite(dir2, sizeof(dir2), 1, img.file_write);

    //让新建的inode的第一个直接块指向被分配的Block地址
    memset(p.block_id0, -1, sizeof(p.block_id0));
    p.block_id0[0] = cur_block_addr;

    p.size = super_block.block_size;
    p.block_id1 = -1;
    p.mode = MODE_DIR | DIR_DEF_PERMISSION;
    p.create_time = time(nullptr);
    p.last_modified_time = time(nullptr);
    p.last_read_time = time(nullptr);

    //保存inode至分配地址
    fseek(img.file_write, cur_inode_addr, SEEK_SET);
    fwrite(&p, sizeof(inode), 1, img.file_write);

    //将当前目录的Block内容写回原来位置
    fseek(img.file_write, cur.block_id0[find_pos_i], SEEK_SET);
    fwrite(new_dir, sizeof(dir), 1, img.file_write);

    //保存inode
    cur.link_num++;
    fseek(img.file_write, parent_inode_addr, SEEK_SET);
    fwrite(&cur, sizeof(inode), 1, img.file_write);
    fflush(img.file_write);
    return 0;

  } else {
    cout << "ERROR: no free dir, mkdir failed" << endl;
    return 5;
  }
}

/*
 * inode分配
 * 无法分配 return -1
 * 否则返回分配地址
 */
int fs::iAlloc() {
  if (super_block.free_inode_num == 0) {
    cout << "WARNING: no free inodes can be alloc" << endl;
    return -1;
  } else {
    int pos = 0;
    for (int i = 0; i < super_block.inode_num; i++) {
      if (bit_map.inode_bitmap[i] == 0) {
        pos = i;
        break;
      }
    }
    super_block.free_inode_num--;
    fseek(img.file_write, SUPERBLOCK_START_ADDR, SEEK_SET);
    fwrite(&super_block, sizeof(super_block), 1, img.file_write);
    bit_map.inode_bitmap[pos] = true;
    fseek(img.file_write, INODE_BITMAP_START_ADDR + pos, SEEK_SET);
    fwrite(&bit_map.inode_bitmap[pos], sizeof(bool), 1, img.file_write);
    fflush(img.file_write);

    return INODE_TABLE_START_ADDR + pos * super_block.inode_size;
  }
}

/*
 * Block分配
 * 无法分配 return -1
 * 否则返回成组连接栈栈顶指向地址
 */
int fs::bAlloc() {
  int top; //栈顶指针
  if (super_block.free_block_num == 0) {
    cout << "WARNING: no free blocks can be alloc" << endl;
    return -1;
  } else {
    //如果已经是最后一块，此时top应该为0
    top = (super_block.free_block_num - 1) % MAX_FREE_BLOCKS;
  }
  //将栈顶取出
  //如果已是栈底，将当前块号地址返回，即为栈底块号，并将栈底指向的新空闲块堆栈覆盖原来的栈
  int alloc_addr;
  if (top == 0) {
    alloc_addr = super_block.free_addr;
    super_block.free_addr = super_block.free_block_stack[0];    //取出下一个存有空闲块堆栈的空闲块的位置，更新空闲块堆栈指针

    //取出对应空闲块内容，覆盖原来的空闲块堆栈
    //取出下一个空闲块堆栈，覆盖原来的
    fseek(img.file_read, super_block.free_addr, SEEK_SET);
    fread(super_block.free_block_stack, sizeof(super_block.free_block_stack), 1, img.file_read);
    fflush(img.file_read);
    super_block.free_block_num--;

  } else {    //如果不为栈底，则将栈顶指向的地址返回，栈顶指针-1.
    alloc_addr = super_block.free_block_stack[top];    //保存返回地址
    super_block.free_block_stack[top] = -1;    //清栈顶
    top--;        //栈顶指针-1
    super_block.free_block_num--;    //空闲块数-1

  }
  //更新超级块
  fseek(img.file_write, SUPERBLOCK_START_ADDR, SEEK_SET);
  fwrite(&super_block, sizeof(superBlock), 1, img.file_write);
  fflush(img.file_write);

  //更新block位图
  bit_map.block_bitmap[(alloc_addr - DATA_BLOCK_START_ADDR) / BLOCK_SIZE] = true;
  fseek(img.file_write, (alloc_addr - DATA_BLOCK_START_ADDR) / BLOCK_SIZE + BLOCK_BITMAP_START_ADDR, SEEK_SET);
  fwrite(&bit_map.block_bitmap[(alloc_addr - DATA_BLOCK_START_ADDR) / BLOCK_SIZE], sizeof(bool), 1, img.file_write);
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
 * id0地址耗尽      return 4
 */
int fs::create(int parent_inode_addr, const char *name, char *file_content) {
  if (strlen(name) >= MAX_FILE_NAME) {
    cout << "Exceeded max file name length" << endl;
    return -1;
  }
  Dir dir[Dir_ITEM_NUM_PER_BLOCK];

  //从这个地址取出inode
  inode cur{};
  fseek(img.file_read, parent_inode_addr, SEEK_SET);
  fread(&cur, sizeof(inode), 1, img.file_read);

  int i = 0;
  int find_pos_i = -1, find_pos_j = -1;

  int first_empty_id0 = -1;//第一个找到的全空的id0
  bool is_first_empty_id0 = true;//用于first_empty_id0的寻找


  while (i < BLOCK_NUM_PER_INODE) {

    if (cur.block_id0[i / Dir_ITEM_NUM_PER_BLOCK] == -1) {
      if (is_first_empty_id0) {
        first_empty_id0 = i / Dir_ITEM_NUM_PER_BLOCK;
        is_first_empty_id0 = false;
      }
      i += Dir_ITEM_NUM_PER_BLOCK;
      continue;
    }
    fseek(img.file_read, cur.block_id0[i / Dir_ITEM_NUM_PER_BLOCK], SEEK_SET);
    fread(dir, sizeof(dir), 1, img.file_read);
    fflush(img.file_read);

    //输出该磁盘块中的所有目录项
    for (int j = 0; j < Dir_ITEM_NUM_PER_BLOCK; j++) {

      if (find_pos_i == -1 && strcmp(dir[j].file_name, "") == 0) {
        //找到一个空闲记录，将新文件创建到这个位置
        find_pos_i = i / Dir_ITEM_NUM_PER_BLOCK;
        find_pos_j = j;
//        break;
      } else if (strcmp(dir[j].file_name, name) == 0) {
        //重名，取出inode，判断是否是文件
        inode tmp{};
        fseek(img.file_read, dir[j].inodeAddr, SEEK_SET);
        fread(&tmp, sizeof(inode), 1, img.file_read);
        if (((tmp.mode >> 9) & 1) == 0) {    //是文件且重名，不能创建文件
          cout << "File already existed" << endl;
          file_content[0] = '\0';
          return 1;
        }
      }
      i++;
    }

  }
  if (find_pos_i != -1) {
    //取出之前那个空闲目录项对应的磁盘块
    fseek(img.file_read, cur.block_id0[find_pos_i], SEEK_SET);
    fread(dir, sizeof(dir), 1, img.file_read);
    fflush(img.file_read);

    //创建这个目录项
    strcpy(dir[find_pos_j].file_name, name);
    int cur_inode_addr = iAlloc();
    if (cur_inode_addr == -1) {
      cout << "ERROR: alloc inode failed" << endl;
      return 2;
    }
    dir[find_pos_j].inodeAddr = cur_inode_addr;

    //设置新条目的inode
    inode p{};
    p.inode_id = static_cast<unsigned short>((cur_inode_addr - INODE_TABLE_START_ADDR) / super_block.inode_size);
    strcpy(p.user_name, cur_user_name);
    strcpy(p.group_name, cur_group_name);
    p.link_num = 1;    //只有一个文件指向


    int k;

    //将buf内容存到磁盘块
    int file_size = static_cast<int>(strlen(file_content));
    for (k = 0; k < file_size; k += super_block.block_size) {
      int cur_block_Addr = bAlloc();
      if (cur_block_Addr == -1) {
        cout << "ERROR: alloc block failed" << endl;
        return 3;
      }
      p.block_id0[k / super_block.block_size] = cur_block_Addr;
      fseek(img.file_write, cur_block_Addr, SEEK_SET);
      fwrite(file_content + k, super_block.block_size, 1, img.file_write);
    }


    //对其他项赋值为-1
    for (k = file_size / super_block.block_size + 1; k < BLOCK_ID0_NUM; k++) {
      p.block_id0[k] = -1;
    }

    if (file_size == 0) {    //长度为0的话也分给它一个block
      int cur_block_Addr = bAlloc();
      if (cur_block_Addr == -1) {
        cout << "ERROR: alloc block failed" << endl;
        return 3;
      }
      p.block_id0[k / super_block.block_size] = cur_block_Addr;
      //写入到当前目录的磁盘块
      fseek(img.file_write, cur_block_Addr, SEEK_SET);
      fwrite(file_content, super_block.block_size, 1, img.file_write);

    }
    p.size = file_size;
    p.block_id1 = -1;
    p.mode = 0;
    p.mode = MODE_FILE | FILE_DEF_PERMISSION;
    p.create_time = time(nullptr);
    p.last_modified_time = time(nullptr);
    p.last_read_time = time(nullptr);

    //将inode写入到申分配地址
    fseek(img.file_write, cur_inode_addr, SEEK_SET);
    fwrite(&p, sizeof(inode), 1, img.file_write);

    //将当前目录的Block写回
    fseek(img.file_write, cur.block_id0[find_pos_i], SEEK_SET);
    fwrite(dir, sizeof(dir), 1, img.file_write);

    //保存inode
    cur.link_num++;
    fseek(img.file_write, parent_inode_addr, SEEK_SET);
    fwrite(&cur, sizeof(inode), 1, img.file_write);
    fflush(img.file_write);
    return 0;
  } else if (first_empty_id0 != -1) {
    //启用一个新的id0地址,并为止分配一个数据块来存储地址
    cur.block_id0[first_empty_id0] = bAlloc();

    //将当前的文件名以及为该文件分配的inode地址计入新id0地址的第一位
    Dir new_dir[Dir_ITEM_NUM_PER_BLOCK] = {0};
    strcpy(new_dir[0].file_name, name);
    int cur_inode_addr = iAlloc();
    if (cur_inode_addr == -1) {
      cout << "ERROR: alloc inode failed" << endl;
      return 2;
    }
    new_dir[0].inodeAddr = cur_inode_addr;

    //设置新条目的inode
    inode p{};
    p.inode_id = static_cast<unsigned short>((cur_inode_addr - INODE_TABLE_START_ADDR) / super_block.inode_size);
    strcpy(p.user_name, cur_user_name);
    strcpy(p.group_name, cur_group_name);
    p.link_num = 1;    //只有一个文件指向

    int k;

    //将buf内容存到磁盘块
    int file_size = static_cast<int>(strlen(file_content));
    for (k = 0; k < file_size; k += super_block.block_size) {
      int cur_block_Addr = bAlloc();
      if (cur_block_Addr == -1) {
        cout << "ERROR: alloc block failed" << endl;
        return 3;
      }
      p.block_id0[k / super_block.block_size] = cur_block_Addr;
      fseek(img.file_write, cur_block_Addr, SEEK_SET);
      fwrite(file_content + k, super_block.block_size, 1, img.file_write);
    }


    //对其他项赋值为-1
    for (k = file_size / super_block.block_size + 1; k < BLOCK_ID0_NUM; k++) {
      p.block_id0[k] = -1;
    }

    if (file_size == 0) {    //长度为0的话也分给它一个block
      int cur_block_Addr = bAlloc();
      if (cur_block_Addr == -1) {
        cout << "ERROR: alloc block failed" << endl;
        return 3;
      }
      p.block_id0[k / super_block.block_size] = cur_block_Addr;
      //写入到当前目录的磁盘块
      fseek(img.file_write, cur_block_Addr, SEEK_SET);
      fwrite(file_content, super_block.block_size, 1, img.file_write);

    }
    p.size = file_size;
    p.block_id1 = -1;
    p.mode = 0;
    p.mode = MODE_FILE | FILE_DEF_PERMISSION;
    p.create_time = time(nullptr);
    p.last_modified_time = time(nullptr);
    p.last_read_time = time(nullptr);


    //当前文件的inode写回
    fseek(img.file_write, cur_inode_addr, SEEK_SET);
    fwrite(&p, sizeof(inode), 1, img.file_write);

    //将当前目录的Block写回
    fseek(img.file_write, cur.block_id0[first_empty_id0], SEEK_SET);
    fwrite(new_dir, sizeof(dir), 1, img.file_write);

    //将该文件所在路径inode写回
    cur.link_num++;
    fseek(img.file_write, parent_inode_addr, SEEK_SET);
    fwrite(&cur, sizeof(inode), 1, img.file_write);
    fflush(img.file_write);
    return 0;

  } else {
    cout << "error: There is not enough id0 dir in this inode.";
    return 4;
  }
}

/*
 * chmod命令，切换权限
 */
void fs::chmod(int parent_inode_addr, const char *name, int mode) {

  if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
    cout << "usage: chmod [filename] [permissions] : Change the file permissions" << endl;
    return;
  }

  inode cur{}, res{};
  fseek(img.file_read, parent_inode_addr, SEEK_SET);
  fread(&cur, sizeof(inode), 1, img.file_read);
  int i = 0, j = 0;
  Dir dir[Dir_ITEM_NUM_PER_BLOCK] = {0};
  bool flag = false;      //纯粹为了跳出循环设置的flag是屑
  while (i < BLOCK_NUM_PER_INODE) {
    if (cur.block_id0[i / Dir_ITEM_NUM_PER_BLOCK] == -1) {
      i += Dir_ITEM_NUM_PER_BLOCK;
      continue;
    }
    //取出磁盘块
    fseek(img.file_read, cur.block_id0[i / Dir_ITEM_NUM_PER_BLOCK], SEEK_SET);
    fread(&dir, sizeof(dir), 1, img.file_read);
    fflush(img.file_read);

    //输出该磁盘块中的所有目录项
    for (j = 0; j < Dir_ITEM_NUM_PER_BLOCK; j++) {
      if (strcmp(dir[j].file_name, name) == 0) {    //找到该目录或者文件
        //取出对应的inode
        fseek(img.file_read, dir[j].inodeAddr, SEEK_SET);
        fread(&res, sizeof(inode), 1, img.file_read);
        fflush(img.file_read);
        flag = true;
      }
      if (flag) break;
    }
    if (flag) break;
    i++;
  }
  if (i >= BLOCK_NUM_PER_INODE) {
    cout << "File name don't exist" << endl;
    return;
  }
  if (strcmp(cur_user_name, res.user_name) != 0 &&
      strcmp(cur_user_name, "root") != 0) {   //切换不成功（对应用户与当前目录不匹配且当前不是root用户）
    cout << "";
    return;
  }

  //模式切换
  res.mode = static_cast<unsigned short>((res.mode >> 9 << 9) | mode);

  //修改对应目录inode信息并保存
  fseek(img.file_write, dir[j].inodeAddr, SEEK_SET);
  fwrite(&res, sizeof(inode), 1, img.file_write);
  fflush(img.file_write);
}

/*
 *  ls命令，显示该目录下所有文件夹与文件
 *  params: 当前目录地址
 */
void fs::ls(int parent_inode_addr, bool isAll) {
  inode cur{};
  //取当前目录inode
  fseek(img.file_read, parent_inode_addr, SEEK_SET);
  fread(&cur, sizeof(inode), 1, img.file_read);
  fflush(img.file_read);


  //取出目录项数
  int cnt = cur.link_num;



  //判断文件模式。
  if (!isPermitRead(cur)) {
    cout << "WARNING: Permission denied(NO READ AUTHORITY)" << endl;
    return;
  }

  //依次取出磁盘块
  int i = 0;
  int num_per_line = 0;
  while (i < cnt && i < BLOCK_NUM_PER_INODE) {
    Dir dir[Dir_ITEM_NUM_PER_BLOCK] = {0};
    if (cur.block_id0[i / Dir_ITEM_NUM_PER_BLOCK] == -1) {
      i += Dir_ITEM_NUM_PER_BLOCK;
      continue;
    }

    fseek(img.file_read, cur.block_id0[i / Dir_ITEM_NUM_PER_BLOCK], SEEK_SET);
    fread(&dir, sizeof(dir), 1, img.file_read);
    fflush(img.file_read);

    for (int j = 0; j < Dir_ITEM_NUM_PER_BLOCK && i < cnt; j++) {

      inode tmp{};
      //取出该目录项的inode，判断该目录项是目录还是文件
      fseek(img.file_read, dir[j].inodeAddr, SEEK_SET);
      fread(&tmp, sizeof(inode), 1, img.file_read);
      fflush(img.file_read);

      if (strcmp(dir[j].file_name, "") == 0) {
        continue;
      }

      if (i > 2 && ((strcmp(dir[j].file_name, ".") == 0 || strcmp(dir[j].file_name, "..") == 0))) {
        continue;
      }
      //当没有使用ls -a时，每找到一个以"."开头的文件就需要跳过，并把i加1，防止多显示。
      if (!isAll && dir[j].file_name[0] == '.') {
        i++;
        continue;
      }
      //输出信息
      cout << dir[j].file_name << '\t';    //文件名
      num_per_line++;
      if (num_per_line % 5 == 0) {
        num_per_line = 0;
        cout << endl;
      }
      i++;
    }

  }
  cout << endl;
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
bool fs::isPermitRead(inode &cur) {
  int file_mode;
  if (strcmp(cur_user_name, cur.user_name) == 0 || strcmp(cur_user_name, "root") == 0)
    file_mode = 6;
  else if (strcmp(cur_group_name, cur.group_name) == 0)
    file_mode = 3;
  else
    file_mode = 0;

  if (((cur.mode >> file_mode >> 2) & 1) == 0) {
    return false;
  } else {
    return true;
  }
}

/*
 *  判断是否有对当前目录的写入权限
 *  有权限返回true，没有权限返回false
 *  目录写入权限：
*   root可以写入所有
*   owner 6
*   group 3
*   other 0
*/
bool fs::isPermitWrite(inode &cur) {
  int filemode;
  if (strcmp(cur_user_name, cur.user_name) == 0 || strcmp(cur_user_name, "root") == 0)
    filemode = 6;
  else if (strcmp(cur_user_name, cur.group_name) == 0)
    filemode = 3;
  else
    filemode = 0;

  if (((cur.mode >> filemode >> 1) & 1) == 0) {
    return false;
  }
  return true;
}

/*
 * 释放inode
 * params: 当前目录地址
 * inode起始位置错误 return -1
 * inode未使用      return 1
 * 成功释放         return 0
 */
int fs::freeInode(int parent_inode_addr) {
  int addr = INODE_TABLE_START_ADDR;
  if ((parent_inode_addr - addr) % super_block.inode_size != 0) {   //是否为inode节点起始位置
    cout << "ERROR: invalid inode addr" << endl;
    return -1;
  }

  auto inode_id = static_cast<int>((parent_inode_addr - addr) / super_block.inode_size);
  if (!bit_map.inode_bitmap[inode_id]) {  //inode未使用
    cout << "WARNING: unused inode " << inode_id << endl;
    return 1;
  }

  //覆盖inode节点
  inode cur = {0};
  fseek(img.file_write, parent_inode_addr, SEEK_SET);
  fwrite(&cur, sizeof(cur), 1, img.file_write);

  //更新SuperBlock
  super_block.free_inode_num++;
  fseek(img.file_write, SUPERBLOCK_START_ADDR, SEEK_SET);
  fwrite(&super_block, sizeof(super_block), 1, img.file_write);

  //更新inode位图
  bit_map.inode_bitmap[inode_id] = false;
  fseek(img.file_write, INODE_BITMAP_START_ADDR + inode_id, SEEK_SET);
  fwrite(&bit_map.inode_bitmap[inode_id], sizeof(bool), 1, img.file_write);

  fflush(img.file_write);

  return 0;
}

/*
 * 释放Block
 * params: 当前目录地址
 * Block起始位置错误    return -1
 * Block未使用         return 1
 * 成功释放            return 0
 * 没有可供释放的Block  return 2
 */
int fs::freeBlock(int parent_inode_addr) {
  if ((parent_inode_addr - DATA_BLOCK_START_ADDR) % super_block.block_size != 0) {
    cout << "ERROR: invalid block addr" << endl;
    return -1;
  }
  unsigned int block_id = static_cast<unsigned int>((parent_inode_addr - DATA_BLOCK_START_ADDR) /
      super_block.block_size);    //inode节点号
  //该地址还未使用，不能释放空间
  if (!bit_map.block_bitmap[block_id]) {
    cout << "WARNING: unused block" << endl;
    return 1;
  }

  if (super_block.free_block_num == super_block.block_num) {
    cout << "WARNING: full free blocks" << endl;
    return 2;
  } else {
    //覆盖目录对应Block
    char buffer[BLOCK_SIZE] = {0};
    fseek(img.file_write, parent_inode_addr, SEEK_SET);
    fwrite(buffer, sizeof(buffer), 1, img.file_write);
    //重新成组链接
    int top = (super_block.free_block_num - 1) % MAX_FREE_BLOCKS;

    if (top == MAX_FREE_BLOCKS - 1) {  //栈顶是当前组最后一个
      //成组链接
      super_block.free_block_stack[0] = super_block.free_addr;    //新的空闲块堆栈第一个地址指向旧的空闲块堆栈指针
      //清空元素
      for (int i = 1; i < MAX_FREE_BLOCKS; i++) {
        super_block.free_block_stack[i] = -1;
      }
      fseek(img.file_write, parent_inode_addr, SEEK_SET);
      fwrite(super_block.free_block_stack, sizeof(super_block.free_block_stack), 1, img.file_write);
    } else {  //不是最后一个，让下一个指向原目录
      top++;
      super_block.free_block_stack[top] = parent_inode_addr;
    }
  }

  //更新SuperBlock
  super_block.free_block_num++;
  fseek(img.file_write, SUPERBLOCK_START_ADDR, SEEK_SET);
  fwrite(&super_block, sizeof(super_block), 1, img.file_write);

  //更新block位图
  bit_map.block_bitmap[block_id] = false;
  fseek(img.file_write, block_id + BLOCK_BITMAP_START_ADDR, SEEK_SET);
  fwrite(&bit_map.block_bitmap[block_id], sizeof(bool), 1, img.file_write);
  fflush(img.file_write);

  return 0;
}

/*
 * touch创建文件命令
 */
void fs::touch(int parent_inode_addr, char name[], char buf[]) {
  Dir dir[Dir_ITEM_NUM_PER_BLOCK];
  inode cur{};
  fseek(img.file_read, parent_inode_addr, SEEK_SET);
  fread(&cur, sizeof(inode), 1, img.file_read);

  if (!isPermitRead(cur)) {
    cout << "WARNING: Permission denied(NO READ AUTHORITY)" << endl;
  }

  int i = 0;

  int first_empty_id0 = -1;//第一个找到的全空的id0
  bool is_first_empty_id0 = true;//用于first_empty_id0的寻找


  while (i < BLOCK_NUM_PER_INODE) {

    if (cur.block_id0[i / Dir_ITEM_NUM_PER_BLOCK] == -1) {
      if (is_first_empty_id0) {
        first_empty_id0 = i / Dir_ITEM_NUM_PER_BLOCK;
        is_first_empty_id0 = false;
      }
      i += Dir_ITEM_NUM_PER_BLOCK;
      continue;
    }
    fseek(img.file_read, cur.block_id0[i / Dir_ITEM_NUM_PER_BLOCK], SEEK_SET);
    fread(dir, sizeof(dir), 1, img.file_read);
    fflush(img.file_read);

    for (auto &j : dir) {
      //当前是否有重名
      if (strcmp(j.file_name, name) == 0) {
        inode tmp{};
        fseek(img.file_read, j.inodeAddr, SEEK_SET);
        fread(&tmp, sizeof(inode), 1, img.file_read);
        if (((tmp.mode >> 9) & 1) == 0) {
          cout << "File already existed" << endl;
          return;
        }
      }
      i++;
    }
  }


  //文件不存在，创建一个空文件
  if (isPermitWrite(cur)) {
    //可写。
    buf[0] = '\0';
    create(parent_inode_addr, name, buf);    //创建文件
  } else {
    cout << "WARNING: Permission denied(NO WRITE AUTHORITY)" << endl;
    return;
  }
}

/*
 * rm -rf，删除所有文件或目录
 * params: 当前目录节点
 */
void fs::rmrf(int parent_inode_addr) {
  inode cur{};
  fseek(img.file_read, parent_inode_addr, SEEK_SET);
  fread(&cur, sizeof(inode), 1, img.file_read);

  if (cur.link_num <= 2) {  //只有当前目录 或 当前目录+上一级目录
    freeBlock(cur.block_id0[0]);    //释放inode直接块指向的Block
    freeInode(parent_inode_addr);                     //释放Inode
    return;
  }

  //依次取出磁盘块
  int i = 0;
  while (i < BLOCK_NUM_PER_INODE) {
    Dir dir[Dir_ITEM_NUM_PER_BLOCK] = {0};

    if (cur.block_id0[i / Dir_ITEM_NUM_PER_BLOCK] == -1) {
      i += Dir_ITEM_NUM_PER_BLOCK;
      continue;
    }
    int cur_block_addr = cur.block_id0[i / Dir_ITEM_NUM_PER_BLOCK];
    fseek(img.file_read, cur_block_addr, SEEK_SET);
    fread(&dir, sizeof(dir), 1, img.file_read);

    //目录项递归删除
    bool isFree = false;
    for (auto &j : dir) {
      if (!(strcmp(j.file_name, ".") == 0 || strcmp(j.file_name, "..") == 0 || strcmp(j.file_name, "") == 0)) {
        isFree = true;
        rmrf(j.inodeAddr);
      }
      i++;
    }
    //Block已空，回收
    if (isFree)
      freeBlock(cur_block_addr);
  }
  //inode已空，回收
  freeInode(parent_inode_addr);
}

/*
 * rm命令，移除单个文件
 * params: 当前目录位置，待移除文件名
 * 名字太长     return -1
 * 权限不够    return 1
 * 成功       return 0
 * 不存在文件  return 2
 */
int fs::rm(int parent_inode_addr, char name[]) {
  if (strlen(name) >= MAX_FILE_NAME) {
    cout << "Exceeded max file name length" << endl;
    return -1;
  }
  inode cur{};
  fseek(img.file_read, parent_inode_addr, SEEK_SET);
  fread(&cur, sizeof(inode), 1, img.file_read);

  int cnt = cur.link_num;

  if (!isPermitWrite(cur)) {
    cout << "WARNING: Permission denied(NO WRITE AUTHORITY)" << endl;
    return 1;
  }

  //依次取出磁盘块
  int i = 0;
  while (i < BLOCK_NUM_PER_INODE) {    //小于BLOCK_NUM_PER_INODE
    Dir dir[Dir_ITEM_NUM_PER_BLOCK] = {0};

    if (cur.block_id0[i / Dir_ITEM_NUM_PER_BLOCK] == -1) {
      i += Dir_ITEM_NUM_PER_BLOCK;
      continue;
    }
    int cur_block_addr = cur.block_id0[i / Dir_ITEM_NUM_PER_BLOCK];
    fseek(img.file_read, cur_block_addr, SEEK_SET);
    fread(&dir, sizeof(dir), 1, img.file_read);

    for (auto &j : dir) {
      inode tmp{};
      //取出该目录项的inode，判断该目录项是目录还是文件
      fseek(img.file_read, j.inodeAddr, SEEK_SET);
      fread(&tmp, sizeof(inode), 1, img.file_read);
      if (strcmp(j.file_name, name) == 0) {
        if (((tmp.mode >> 9) & 1) == 1) {/*目录*/}
        else {
          for (int k : tmp.block_id0)
            if (k != -1)
              freeBlock(k);

          //释放inode
          freeInode(j.inodeAddr);

          //删除该目录条目，写回磁盘
          strcpy(j.file_name, "");
          j.inodeAddr = -1;
          fseek(img.file_write, cur_block_addr, SEEK_SET);
          fwrite(&dir, sizeof(dir), 1, img.file_write);
          cur.link_num--;
          fseek(img.file_write, parent_inode_addr, SEEK_SET);
          fwrite(&cur, sizeof(inode), 1, img.file_write);

          fflush(img.file_write);
          return 0;
        }
      }
      i++;
    }

  }
  cout << name << ": No such file" << endl;
  return 2;
}

/*
 * rmdir命令，移除子目录
 * params: 当前目录位置，待移除目录名
 * 名字太长     return -1
 * 选择删除当前目录或是上一级目录      return 1
 * 权限不够         return 2
 * 成功       return 0
 * 其他情况   return 3
 */
int fs::rmdir(int parent_inode_addr, char name[]) {
  if (strlen(name) >= MAX_FILE_NAME) {
    cout << "Exceeded max file name length" << endl;
    return -1;
  }
  if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
    cout << "WARNING: only subDir can be deleted" << endl;
    return 1;
  }

  //目录信息
  inode cur{};
  fseek(img.file_read, parent_inode_addr, SEEK_SET);
  fread(&cur, sizeof(inode), 1, img.file_read);

  //判断有无权限
  if (!isPermitWrite(cur)) {
    cout << "WARNING: Permission denied(NO WRITE AUTHORITY)" << endl;
    return 2;
  }
  //依次取出磁盘块
  int i = 0;
  while (i < BLOCK_NUM_PER_INODE) {    //小于BLOCK_NUM_PER_INODE
    Dir dir[Dir_ITEM_NUM_PER_BLOCK] = {0};

    if (cur.block_id0[i / Dir_ITEM_NUM_PER_BLOCK] == -1) {
      i += Dir_ITEM_NUM_PER_BLOCK;
      continue;
    }
    //取出磁盘块
    int cur_block_addr = cur.block_id0[i / Dir_ITEM_NUM_PER_BLOCK];
    fseek(img.file_read, cur_block_addr, SEEK_SET);
    fread(&dir, sizeof(dir), 1, img.file_read);

    //找到要删除的目录
    for (auto &j : dir) {
      inode tmp{};

      fseek(img.file_read, j.inodeAddr, SEEK_SET);
      fread(&tmp, sizeof(inode), 1, img.file_read);

      if (strcmp(j.file_name, name) == 0) {
        if (((tmp.mode >> 9) & 1) == 1) {
          //目录
          rmrf(j.inodeAddr);
          strcpy(j.file_name, "");
          j.inodeAddr = -1;
          fseek(img.file_write, cur_block_addr, SEEK_SET);
          fwrite(&dir, sizeof(dir), 1, img.file_write);
          cur.link_num = static_cast<unsigned short>(cur.link_num - 1);
          fseek(img.file_write, parent_inode_addr, SEEK_SET);
          fwrite(&cur, sizeof(inode), 1, img.file_write);
          fflush(img.file_write);
          return 0;
        }
      }
      i++;
    }
  }
  return 3;
}

/*
 * exit指令，退出重置信息
 */
void fs::exit() {
  memset(cur_user_name, 0, sizeof(cur_user_name));                //清空当前用户名
  memset(cur_user_dir_name, 0, sizeof(cur_user_dir_name));        //清空当前用户目录
  cur_dir_addr = ROOT_DIR_ADDR;                                                //当前用户目录地址设为根目录地址
  strcpy(cur_dir_name, "/");                                    //当前目录设为"/"
  is_login = false;
}

/*
 *  Your Fake CommandLine
 */
void fs::commandLine(char *cmd) {
  char argv1[ARGV_LEN];
  char argv2[ARGV_LEN];
  char argv3[ARGV_LEN];
  char buffer[100000];
  sscanf(cmd, "%s", argv1);
  if (strcmp(argv1, "ls") == 0) {
    sscanf(cmd, "%s%s", argv1, argv2);
    if (strcmp(argv2, "") == 0) {
      ls(cur_dir_addr, false);
      return;
    } else if (strstr(argv2, "-") != nullptr && strstr(argv2, "l") != nullptr && strstr(argv2, "a") == nullptr) {
      lsl(cur_dir_addr, false);
      return;
    } else if (strstr(argv2, "-") != nullptr && strstr(argv2, "a") != nullptr && strstr(argv2, "l") == nullptr) {
      ls(cur_dir_addr, true);
      return;
    } else if (strstr(argv2, "-") != nullptr && strstr(argv2, "l") != nullptr && strstr(argv2, "a") != nullptr) {
      lsl(cur_dir_addr, true);
      return;
    } else {
      cout << "\tls: invalid option -- '" << argv2 << "'" << endl;
      return;
    }
  } else if (strcmp(argv1, "cd") == 0) {
    sscanf(cmd, "%s%s", argv1, argv2);
    if (strcmp(argv2, "") == 0) {
      cout << "\n\tcd: missing operand\n\t Try 'cd [fileName]'" << endl;
      return;
    }
    cda(cur_dir_addr, argv2);
//    cd(cur_dir_addr, argv2);
  } else if (strcmp(argv1, "pwd") == 0) {
    pwd();
  } else if (strcmp(argv1, "mkdir") == 0) {
    sscanf(cmd, "%s%s", argv1, argv2);
    if (strcmp(argv2, "") == 0) {
      cout << "\n\tmkdir: missing operand\n\tTry 'mkdir [dirName]'" << endl;
      return;
    }
    mkdir(cur_dir_addr, argv2);
    chmod(cur_dir_addr, argv2, 0660);
  } else if (strcmp(argv1, "rmdir") == 0) {
    sscanf(cmd, "%s%s", argv1, argv2);
    if (strcmp(argv2, "") == 0) {
      cout << "\n\trmdir: missing operand\n\tTry 'rmdir [dirName]'" << endl;
      return;
    }
    rmdir(cur_dir_addr, argv2);
  } else if (strcmp(argv1, "stat") == 0) {
    sscanf(cmd, "%s%s", argv1, argv2);
    if (strcmp(argv2, "") == 0) {
      cout << "\n\tstat: missing operand\n\tTry 'stat [fileName]'" << endl;
      return;
    }
    stat(cur_dir_addr, argv2);
  } else if (strcmp(argv1, "vi") == 0 || strcmp(argv1, "vim") == 0 || strcmp(argv1, "nano") == 0) {
    sscanf(cmd, "%s%s", argv1, argv2);
    if (strcmp(argv2, "") == 0) {
      cout << "\n\tvi: missing operand\n\tTry 'vi [fileName]'" << endl;
      return;
    }
    fakeVi(cur_dir_addr, argv2, buffer);
  } else if (strcmp(argv1, "rename") == 0) {
    sscanf(cmd, "%s%s%s", argv1, argv2, argv3);
    if (strcmp(argv2, "") == 0 || strcmp(argv3, "") == 0) {
      cout << "\n\ttouch: missing operand\n\tTry 'touch [fileName | dirName]'" << endl;
      return;
    }
    rename(cur_dir_addr, argv2, argv3);    //读取内容到buf
  } else if (strcmp(argv1, "useradd") == 0) {
    argv2[0] = '\0';
    sscanf(cmd, "%s%s", argv1, argv2);
    if (strcmp(argv2, "") == 0) {
      cout << "\n\tuseradd: missing operand\n\tTry 'useradd 'user'" << endl;
      return;
    }
    useradd(argv2);
  } else if (strcmp(argv1, "userdel") == 0) {
    argv2[0] = '\0';
    sscanf(cmd, "%s%s", argv1, argv2);
    if (strcmp(argv2, "") == 0) {
      cout << "\n\tuserdel: missing operand\n\tTry 'userdel 'user'" << endl;
      return;
    }
    userdel(argv2);
  } else if (strcmp(argv1, "touch") == 0) {
    sscanf(cmd, "%s%s", argv1, argv2);
    if (strcmp(argv2, "") == 0) {
      cout << "\n\ttouch: missing operand\n\tTry 'touch [fileName]'" << endl;
      return;
    }
    touch(cur_dir_addr, argv2, buffer);    //读取内容到buf
  } else if (strcmp(argv1, "cat") == 0) {
    sscanf(cmd, "%s%s", argv1, argv2);
    if (strcmp(argv2, "") == 0) {
      cout << "\n\tcat: missing operand\n\tTry 'cat [fileName]'" << endl;
      return;
    }
    cat(cur_dir_addr, argv2);
  } else if (strcmp(argv1, "rm") == 0) {    //删除一个文件
    sscanf(cmd, "%s%s", argv1, argv2);
    if (strcmp(argv2, "") == 0) {
      cout << "\n\trm: missing operand\n\tTry 'rm [fileName]'" << endl;
      return;
    }
    rm(cur_dir_addr, argv2);
  } else if (strcmp(argv1, "help") == 0 || strcmp(argv1, "h") == 0) {
    help();
  } else if (strcmp(argv1, "exit") == 0) {
    exit();
  } else if (strcmp(argv1, "chmod") == 0) {
    argv2[0] = '\0';
    argv3[0] = '\0';
    sscanf(cmd, "%s%s%s", argv1, argv2, argv3);
    if (strlen(argv2) == 0 || strlen(argv3) == 0) {
      cout << "usage: chmod [filename] [permissions] : Change the file permissions" << endl;
    } else {
      int num = 0;
      for (int i = 0; argv3[i]; i++) {
        num = num * 8 + argv3[i] - '0';
      }
      chmod(cur_dir_addr, argv2, num);
    }
  } else {
    cout << "invalid command: " << cmd << endl;
  }
}

/*
 * 开头信息
 */
void fs::commandLinePrompt() {
  if (strcmp(cur_user_name, "root") == 0) {
    cout << "[root@Mimic]" << convertCurDir(cur_dir_name, cur_user_dir_name) << "# ";
  } else {
    cout << "[" << cur_user_name << "@Mimic]" << convertCurDir(cur_dir_name, cur_user_dir_name) << "$ ";
  }
}

void fs::fsInfo() {
  cout << "Commands are defined internally, type 'help' for details\n" << endl;
  cout << "Welcome to ext2mimic 11.45.14" << endl;
  cout << " * Documentation: no doc" << endl;
  cout << " * Management: no management" << endl;
  cout << " * Support: no info\n" << endl;
  time_t curTime = time(nullptr);
  cout << "System information as of " << ctime(&curTime);
}

void fs::help() {
  cout << "" << endl;
  cout << "rm [fileName] : Remove file" << endl;
  cout << "mkdir [dirName] : Create dir" << endl;
  cout << "rmdir [dirName] : Delete dir" << endl;
  cout << "cd [dirName] : Change current directory" << endl;
  cout << "cat [fileName] : Display file content" << endl;
  cout << "ls [-l]: List the file and directory (-l: display file type, permissions, owner, file size)" << endl;
  cout << "chmod [fileName] [permissions] : Change the file permissions" << endl;
  cout << "touch [fileName] : Create a new empty file" << endl;
  cout << "stat [fileName | dirName] : Display file or dir detailed information" << endl;
  cout << "rename [fileName | dirName] [fileName | dirName] : Rename a file or dir" << endl;
  cout << "vi/vim/nano [fileName] : Edit file with fake Vi" << endl;
  cout << "pwd : Display current dir" << endl;
  cout << "useradd : Add a user" << endl;
  cout << "userdel : Delete a user" << endl;
  cout << "exit : Exit the file system" << endl;
  cout << "" << endl;

}

/*
 * 仿vi编辑
 */
void fs::fakeVi(int parent_inode_addr, char *name, char *buf) {
  if (strlen(name) >= MAX_FILE_NAME) {
    cout << "Exceeded max file name length" << endl;
    return;
  }

  //清空缓冲区
  memset(buf, 0, sizeof(buf));
  int maxlen = 0;    //到达过的最大长度

  //查找有无同名文件，有的话进入编辑模式，没有进入创建文件模式
  Dir dir[Dir_ITEM_NUM_PER_BLOCK];    //临时目录清单

  //从这个地址取出inode
  inode cur{}, fileInode{};
  fseek(img.file_read, parent_inode_addr, SEEK_SET);
  fread(&cur, sizeof(inode), 1, img.file_read);

  int i = 0, j;
  int dno;
  int fileInodeAddr = -1;    //文件的inode地址
  bool isExist = false;    //文件是否已存在
  while (i < BLOCK_NUM_PER_INODE) {
    dno = i / Dir_ITEM_NUM_PER_BLOCK;    //在第几个直接块里

    if (cur.block_id0[dno] == -1) {
      i += Dir_ITEM_NUM_PER_BLOCK;
      continue;
    }
    fseek(img.file_read, cur.block_id0[dno], SEEK_SET);
    fread(dir, sizeof(dir), 1, img.file_read);
    fflush(img.file_read);
    bool flag = false;
    //输出该磁盘块中的所有目录项
    for (j = 0; j < Dir_ITEM_NUM_PER_BLOCK; j++) {
      if (strcmp(dir[j].file_name, name) == 0) {
        //重名，取出inode，判断是否是文件
        fseek(img.file_read, dir[j].inodeAddr, SEEK_SET);
        fread(&fileInode, sizeof(inode), 1, img.file_read);
        if (((fileInode.mode >> 9) & 1) == 0) {    //是文件且重名，打开这个文件，并编辑
          flag = true;
          fileInodeAddr = dir[j].inodeAddr;
          isExist = true;
          break;
        }
      }
      i++;
    }
    if (flag) {
      i--;
      break;
    }
  }

  //初始化vi
  int cnt = 0;
  system("cls");    //清屏

  int window_x, window_y, cur_x, cur_y;

  HANDLE handle_out;                              //定义一个句柄
  CONSOLE_SCREEN_BUFFER_INFO screen_info;         //定义窗口缓冲区信息结构体
  COORD pos = {0, 0};                             //定义一个坐标结构体

  if (isExist) {    //文件已存在，进入编辑模式，先输出之前的文件内容

    //权限判断。判断文件是否可读
    if (!isPermitRead(fileInode)) {
      //不可读
      cout << "WARNING: Permission denied(NO READ AUTHORITY)" << endl;
      return;
    }

    int sumlen = fileInode.size;    //文件长度
    int getlen = 0;    //取出来的长度
    char fileContent[100000] = {0};
    for (unsigned int i : fileInode.block_id0) {
      if (i == -1) {
        continue;
      }
      //依次取出磁盘块的内容
      fseek(img.file_read, i, SEEK_SET);
      fread(fileContent, super_block.block_size, 1, img.file_read);    //读取出一个磁盘块大小的内容
      fflush(img.file_read);
      //输出字符串
      int curlen = 0;    //当前指针
      while (curlen < super_block.block_size) {
        if (getlen >= sumlen)    //全部输出完毕
          break;
        cout << fileContent[curlen];    //输出到屏幕
        buf[cnt++] = fileContent[curlen];    //输出到buf
        curlen++;
        getlen++;
      }
      if (getlen >= sumlen)
        break;
    }
    maxlen = sumlen;
  }

  //获得输出之后的光标位置
  handle_out = GetStdHandle(STD_OUTPUT_HANDLE);   //获得标准输出设备句柄
  GetConsoleScreenBufferInfo(handle_out, &screen_info);   //获取窗口信息
  window_x = screen_info.srWindow.Right - screen_info.srWindow.Left + 1;
  window_y = screen_info.srWindow.Bottom - screen_info.srWindow.Top + 1;
  cur_x = screen_info.dwCursorPosition.X;
  cur_y = screen_info.dwCursorPosition.Y;


  //进入vi
  //先用vi读取文件内容
  vimimic vi;
  char ori_buf[100000];
  memset(ori_buf, 0, sizeof(ori_buf));
  strcpy(ori_buf, buf);
  while (vi.method(buf, ori_buf, cnt, maxlen)) {/*编辑中，编辑完退出循环*/};
  fs::fsInfo();
  if (isExist) {
    //将buf内容写回文件的磁盘块
    if (isPermitWrite(fileInode)) {    //可写
      writeFile(fileInode, fileInodeAddr, buf);
    } else {    //不可写
      cout << "WARNING: Permission denied(NO WRITE AUTHORITY)" << endl;
    }

  } else {    //是创建文件模式
    if (isPermitWrite(cur)) {
      //可写。可以创建文件
      create(parent_inode_addr, name, buf);    //创建文件
    } else {
      cout << "WARNING: Permission denied(NO WRITE AUTHORITY)" << endl;
      return;
    }
  }
}

void fs::writeFile(inode fileInode, int fileInodeAddr, char *buf) {
  int read_len = strlen(buf);    //文件长度，单位为字节
  for (int k = 0; k < read_len; k += super_block.block_size) {    //最多10次，10个磁盘快，即最多5K
    //分配这个inode的磁盘块，从控制台读取内容
    int curblockAddr;
    if (fileInode.block_id0[k / super_block.block_size] == -1) {
      //缺少磁盘块，申请一个
      curblockAddr = bAlloc();
      if (curblockAddr == -1) {
        cout << "block alloc failed" << endl;
        return;
      }
      fileInode.block_id0[k / super_block.block_size] = curblockAddr;
    } else {
      curblockAddr = fileInode.block_id0[k / super_block.block_size];
    }
    //写入到当前目录的磁盘块
    fseek(img.file_write, curblockAddr, SEEK_SET);
    fwrite(buf + k * super_block.block_size, super_block.block_size, 1, img.file_write);
    fflush(img.file_write);
  }
  //更新该文件大小
  fileInode.size = read_len;
  fileInode.last_modified_time = time(nullptr);
  fileInode.last_read_time = time(nullptr);
  fseek(img.file_write, fileInodeAddr, SEEK_SET);
  fwrite(&fileInode, sizeof(inode), 1, img.file_write);
  fflush(img.file_write);
}

/*
 *  stat指令，查看指定文件或目录具体相关信息
 */
int fs::stat(int parent_inode_addr, char name[]) {
  if (strlen(name) >= MAX_FILE_NAME) {
    cout << "Exceeded max file name length" << endl;
    return -1;
  }
  inode cur{};
  fseek(img.file_read, parent_inode_addr, SEEK_SET);
  fread(&cur, sizeof(inode), 1, img.file_read);

  int cnt = cur.link_num;

  if (!isPermitRead(cur)) {
    cout << "WARNING: Permission denied(NO READ AUTHORITY)" << endl;
    return 1;
  }

  //依次取出磁盘块
  int i = 0;
  while (i < BLOCK_NUM_PER_INODE) {    //小于BLOCK_NUM_PER_INODE
    Dir dir[Dir_ITEM_NUM_PER_BLOCK] = {0};

    if (cur.block_id0[i / Dir_ITEM_NUM_PER_BLOCK] == -1) {
      i += Dir_ITEM_NUM_PER_BLOCK;
      continue;
    }

    fseek(img.file_read, cur.block_id0[i / Dir_ITEM_NUM_PER_BLOCK], SEEK_SET);
    fread(&dir, sizeof(dir), 1, img.file_read);

    for (auto &j : dir) {
      inode tmp{};
      //取出该目录项的inode，判断该目录项是目录还是文件
      fseek(img.file_read, j.inodeAddr, SEEK_SET);
      fread(&tmp, sizeof(inode), 1, img.file_read);
      if (strcmp(j.file_name, name) == 0) {
        cout << "  File: \"" << j.file_name << "\"" << endl;
        cout << "Size: " << tmp.size << "\t";
        cout << "Block: " << 1 << "\t";
        cout << "IO Block: " << BLOCK_SIZE << "\t";
        if (((tmp.mode >> 9) & 1) == 1) {
          cout << "directory file" << endl;
        } else {
          cout << "regular file" << endl;
        }
        cout << "Device: fd01h/64769d\t";
        cout << "Inode: " << tmp.inode_id << "\t";
        cout << "Access: (0";
        int permiss_index = 8, idx = 0, temp = 0;
        char auth[8];
        while (permiss_index >= 0) {
          if (((tmp.mode >> permiss_index) & 1) == 1) {
            if (permiss_index % 3 == 2) {
              temp += 4;
              auth[idx] = 'r';
            }
            if (permiss_index % 3 == 1) {
              temp += 2;
              auth[idx] = 'w';
            }
            if (permiss_index % 3 == 0) {
              temp += 1;
              auth[idx] = 'x';
            }
          } else {
            auth[idx] = '-';
          }
          idx++;
          permiss_index--;
          if (idx % 3 == 0) {
            cout << temp;
            temp = 0;
          }
        }
        cout << "/";
        if (((tmp.mode >> 9) & 1) == 1) {
          cout << "d";
        } else {
          cout << "-";
        }
        fflush(stdout);
        cout << auth;
        cout << ")\tUser: " << tmp.user_name << "\t";
        cout << "Group: " << tmp.group_name << endl;
        cout << "Access: " << ctime(&tmp.create_time);
        cout << "Modify: " << ctime(&tmp.last_modified_time);
        cout << "Change: " << ctime(&tmp.last_read_time);
        return 0;
      }
      i++;
    }
  }
  cout << name << ": No such file" << endl;
  return 2;
}

/*
 * rename命令，重命名目录或文件
 */
int fs::rename(int parent_inode_addr, char *ori_name, char *modify_name) {
  if (strlen(modify_name) >= MAX_FILE_NAME) {
    cout << "Exceeded max file name length" << endl;
    return -1;
  }
  inode cur{};
  fseek(img.file_read, parent_inode_addr, SEEK_SET);
  fread(&cur, sizeof(inode), 1, img.file_read);

  cur.last_read_time = time(nullptr);
  int cnt = cur.link_num;

  if (!isPermitWrite(cur)) {
    cout << "WARNING: Permission denied(NO WRITE AUTHORITY)" << endl;
    return 1;
  }

  //依次取出磁盘块
  int i = 0;
  while (i < BLOCK_NUM_PER_INODE) {    //小于BLOCK_NUM_PER_INODE
    Dir dir[Dir_ITEM_NUM_PER_BLOCK] = {0};

    if (cur.block_id0[i / Dir_ITEM_NUM_PER_BLOCK] == -1) {
      i += Dir_ITEM_NUM_PER_BLOCK;
      continue;
    }

    fseek(img.file_read, cur.block_id0[i / Dir_ITEM_NUM_PER_BLOCK], SEEK_SET);
    fread(&dir, sizeof(dir), 1, img.file_read);

    for (auto &j : dir) {
      inode tmp{};
      //取出该目录项的inode，判断该目录项是目录还是文件
      fseek(img.file_read, j.inodeAddr, SEEK_SET);
      fread(&tmp, sizeof(inode), 1, img.file_read);
      if (strcmp(j.file_name, ori_name) == 0) {
        cur.last_modified_time = time(nullptr);
        strcpy(j.file_name, modify_name);
        fseek(img.file_write, cur.block_id0[i / Dir_ITEM_NUM_PER_BLOCK], SEEK_SET);
        fwrite(&dir, sizeof(dir), 1, img.file_write);
        fseek(img.file_write, parent_inode_addr, SEEK_SET);
        fwrite(&cur, sizeof(inode), 1, img.file_write);
        fflush(img.file_write);
        return 0;
      }
      i++;
    }

  }
  cout << ori_name << ": No such file" << endl;
  return 2;
}

/*
 * 显示当前完整目录
 */
inline void fs::pwd() {
  cout << cur_dir_name << endl;
}

/*
 *  cat命令将文件中的内容显示出来
 */
void fs::cat(int parent_inode_addr, char *name) {
  char buf[10000] = {0};
  Dir dir[Dir_ITEM_NUM_PER_BLOCK] = {0};
  inode cur = {0};
  fseek(img.file_read, parent_inode_addr, SEEK_SET);
  fread(&cur, sizeof(inode), 1, img.file_read);

  int i = 0, j;
  bool isExist = false;
  bool flag = false;
  inode file_inode = {0};
  while (i < BLOCK_NUM_PER_INODE) {
    if (cur.block_id0[i / Dir_ITEM_NUM_PER_BLOCK] == -1) {
      i += Dir_ITEM_NUM_PER_BLOCK;
      continue;
    }
    fseek(img.file_read, cur.block_id0[i / Dir_ITEM_NUM_PER_BLOCK], SEEK_SET);
    fread(dir, sizeof(dir), 1, img.file_read);
    fflush(img.file_read);

    for (j = 0; j < Dir_ITEM_NUM_PER_BLOCK; j++) {
      if (strcmp(dir[j].file_name, name) == 0) {
        fseek(img.file_read, dir[j].inodeAddr, SEEK_SET);
        fread(&file_inode, sizeof(inode), 1, img.file_read);
        if (((file_inode.mode >> 9) & 1) == 0) {    //找到文件
          isExist = true;
          flag = true;
          break;
        }
      }
      i++;
    }
    if (flag)
      break;
  }
  int cnt = 0;
  if (isExist) {    //文件存在，输出内容
    if (!isPermitRead(file_inode)) {
      cout << "WARNING: Permission denied(NO READ AUTHORITY)" << endl;
      return;
    }

    i = 0;
    int file_len = file_inode.size;    //文件长度
    int read_len = 0;    //读出来的长度
    for (i = 0; i < BLOCK_ID0_NUM; i++) {
      char fileContent[1000] = {0};
      if (file_inode.block_id0[i] == -1) {
        continue;
      }
      //依次取出磁盘块的内容
      fseek(img.file_read, file_inode.block_id0[i], SEEK_SET);
      fread(fileContent, super_block.block_size, 1, img.file_read);
      fflush(img.file_read);
      //输出字符串
      int cur_len = 0;
      while (cur_len < super_block.block_size) {
        if (read_len >= file_len)    //全部输出完毕
          break;
        cout << fileContent[cur_len];    //命令行打印
        buf[cnt++] = fileContent[cur_len];    //存入buf
        cur_len++;
        read_len++;
      }
      if (read_len >= file_len)
        break;
    }
  } else {
    cout << "cat '" << name << "': No such a file" << endl;
  }
  cout << endl;
}

/*
 * useradd指令，添加用户
 */
void fs::useradd(char user_name[]) {
  if (strcmp(cur_user_name, "root") != 0) {
    cout << "Warning: Only root user can add user" << endl;
    return;
  }
  cout << "new password: ";
  //用户密码
//  char password[MAX_PASSWD_LEN] = {0};
//  fflush(stdin);
//  cin.getline(password, MAX_PASSWD_LEN);
//
//  cout << "retype new password: ";
//  //确认密码
//  char refirm_password[MAX_PASSWD_LEN] = {0};
//  fflush(stdin);
//  cin.getline(refirm_password, MAX_PASSWD_LEN);
//  if (strcmp(password, refirm_password) != 0) {
//    cout << "Different passwords ! Register failed" << endl;
//    return;
//  }
  string password = getPasswdConfirm(MAX_PASSWD_LEN);
  Dir dir[Dir_ITEM_NUM_PER_BLOCK];
  int user_inode_Addr = -1;    //用户文件inode地址
  int passwd_inode_Addr = -1;    //用户密码文件inode地址
  int group_inode_Addr = -1;    //用户组文件inode地址
  inode user_inode{};        //用户文件的inode
  inode passwd_inode{};        //用户密码文件的inode
  inode group_inode{};        //用户组文件inode

  //原来的目录
  char tmp_cur_user_name[100];
  char tmp_cur_user_name_next[100];
  char tmp_cur_user_dir_name[100];
  int tmp_cur_dir_addr;
  char tmp_cur_dir_name[100];
  char tmp_cur_group_name[100];

  inode cur_dir_inode{};    //当前目录的inode
  int i, j;
  strcpy(tmp_cur_user_name, cur_user_name);
  strcpy(tmp_cur_user_dir_name, cur_user_dir_name);
  tmp_cur_dir_addr = cur_dir_addr;
  strcpy(tmp_cur_dir_name, cur_dir_name);
  //返回根目录
  memset(cur_user_name, 0, sizeof(cur_user_name));
  memset(cur_user_dir_name, 0, sizeof(cur_user_dir_name));
  cur_dir_addr = ROOT_DIR_ADDR;
  strcpy(cur_dir_name, "/");

  cd(cur_dir_addr, "home");

  strcpy(tmp_cur_user_name_next, cur_user_name);
  strcpy(tmp_cur_group_name, cur_group_name);

  strcpy(cur_user_name, user_name);
  strcpy(cur_group_name, "user");

  if (mkdir(cur_dir_addr, user_name)) {

    strcpy(cur_user_name, tmp_cur_user_name);
    strcpy(cur_group_name, tmp_cur_group_name);
    strcpy(cur_user_dir_name, tmp_cur_user_dir_name);
    cur_dir_addr = tmp_cur_dir_addr;
    strcpy(cur_dir_name, tmp_cur_dir_name);
    cout << "Error: Register failed" << endl;
    return;
  }

  chmod(cur_dir_addr, user_name, 0600);
  strcpy(cur_user_name, tmp_cur_user_name_next);
  strcpy(cur_group_name, tmp_cur_group_name);

  memset(cur_user_name, 0, sizeof(cur_user_name));
  memset(cur_user_dir_name, 0, sizeof(cur_user_dir_name));
  cur_dir_addr = ROOT_DIR_ADDR;
  strcpy(cur_dir_name, "/");

  cd(cur_dir_addr, "etc");

  fseek(img.file_read, cur_dir_addr, SEEK_SET);
  fread(&cur_dir_inode, sizeof(inode), 1, img.file_read);

  for (int i : cur_dir_inode.block_id0) {
    if (i == -1) {
      continue;
    }
    fseek(img.file_read, i, SEEK_SET);
    fread(&dir, sizeof(dir), 1, img.file_read);

    for (auto &j : dir) {
      if (strcmp(j.file_name, "user") == 0 || strcmp(j.file_name, "passwd") == 0 ||
          strcmp(j.file_name, "group") == 0) {
        inode tmp{};

        fseek(img.file_read, j.inodeAddr, SEEK_SET);
        fread(&tmp, sizeof(inode), 1, img.file_read);

        if (((tmp.mode >> 9) & 1) == 0) {

          if (strcmp(j.file_name, "user") == 0) {
            user_inode_Addr = j.inodeAddr;
            user_inode = tmp;
          } else if (strcmp(j.file_name, "passwd") == 0) {
            passwd_inode_Addr = j.inodeAddr;
            passwd_inode = tmp;
          } else if (strcmp(j.file_name, "group") == 0) {
            group_inode_Addr = j.inodeAddr;
            group_inode = tmp;
          }
        }
      }
    }
    if (user_inode_Addr != -1 && passwd_inode_Addr != -1)
      break;
  }

  char file_content[100000];
  char buffer[600];

  int cnt = 0;
  for (int i = 0; i < user_inode.size; i++) {
    if (i % super_block.block_size == 0) {
      //换新的磁盘块
      fseek(img.file_read, user_inode.block_id0[i / super_block.block_size], SEEK_SET);
      fread(&buffer, super_block.block_size, 1, img.file_read);
      cnt = 0;
    }
    file_content[i] = buffer[cnt++];
  }
  buffer[user_inode.size] = '\0';

  if (strstr(buffer, user_name) != nullptr) {
    cout << "Warning: User already existed" << endl;
    strcpy(cur_user_name, tmp_cur_user_name);
    strcpy(cur_user_dir_name, tmp_cur_user_dir_name);
    cur_dir_addr = tmp_cur_dir_addr;
    strcpy(cur_dir_name, tmp_cur_dir_name);
    return;
  }

  // 1 . 普通用户组 buffer + strlen(buffer) . 定位到末尾
  sprintf(buffer + strlen(buffer), "%s:x:%d:%d\n", user_name, nextUID++, 1);
  user_inode.size = strlen(buffer);
  writeFile(user_inode, user_inode_Addr, buffer);

  cnt = 0;
  for (int i = 0; i < passwd_inode.size; i++) {
    if (i % super_block.block_size == 0) {    //超出了这个磁盘块
      //换新的磁盘块
      fseek(img.file_read, passwd_inode.block_id0[i / super_block.block_size], SEEK_SET);
      fread(&buffer, super_block.block_size, 1, img.file_read);
      cnt = 0;
    }
    file_content[i] = buffer[cnt++];
  }
  buffer[passwd_inode.size] = '\0';

  sprintf(buffer + strlen(buffer), "%s:%s\n", user_name, password.c_str());
  passwd_inode.size = strlen(buffer);
  writeFile(passwd_inode, passwd_inode_Addr, buffer);


  //取出group文件内容
  cnt = 0;
  for (int i = 0; i < group_inode.size; i++) {
    if (i % super_block.block_size == 0) {    //超出了这个磁盘块
      //换新的磁盘块
      fseek(img.file_read, group_inode.block_id0[i / super_block.block_size], SEEK_SET);
      fread(&buffer, super_block.block_size, 1, img.file_read);
      cnt = 0;
    }
    file_content[i] = buffer[cnt++];
  }
  buffer[group_inode.size] = '\0';

  if (buffer[strlen(buffer) - 2] == ':') {                    //当前组内无用户
    sprintf(buffer + strlen(buffer) - 1, "%s\n", user_name);
  } else {
    sprintf(buffer + strlen(buffer) - 1, ",%s\n", user_name);
  }
  group_inode.size = strlen(buffer);
  writeFile(group_inode, group_inode_Addr, buffer);

  //恢复现场，回到原来的目录
  strcpy(cur_user_name, tmp_cur_user_name);
  strcpy(cur_user_dir_name, tmp_cur_user_dir_name);
  cur_dir_addr = tmp_cur_dir_addr;
  strcpy(cur_dir_name, tmp_cur_dir_name);
  cout << "Register success" << endl;
}

/*
 * userdel指令，删除用户
 */
void fs::userdel(char *user_name) {
  if (strcmp(user_name, "root") == 0) {
    cout << "Warning: root can't be deleted" << endl;
  }

  if (strcmp(cur_user_name, "root") != 0) {
    cout << "Warning: Only root user dan delete user" << endl;
    return;
  }

  Dir dir[Dir_ITEM_NUM_PER_BLOCK];
  int user_inode_Addr = -1;    //用户文件inode地址
  int passwd_inode_Addr = -1;    //用户密码文件inode地址
  int group_inode_Addr = -1;    //用户组文件inode地址
  inode user_inode{};        //用户文件的inode
  inode passwd_inode{};        //用户密码文件的inode
  inode group_inode{};        //用户组文件inode

  //原目录
  char tmp_cur_user_name[100];
  char tmp_cur_user_dir_name[100];
  int tmp_cur_dir_addr;
  char tmp_cur_dir_name[100];
  inode cur_dir_inode{};    //当前目录
  int i, j;

  strcpy(tmp_cur_user_name, cur_user_name);
  strcpy(tmp_cur_user_dir_name, cur_user_dir_name);
  tmp_cur_dir_addr = cur_dir_addr;
  strcpy(tmp_cur_dir_name, cur_dir_name);

  memset(cur_user_name, 0, sizeof(cur_user_name));
  memset(cur_user_dir_name, 0, sizeof(cur_user_dir_name));
  cur_dir_addr = ROOT_DIR_ADDR;
  strcpy(cur_dir_name, "/");

  //进入用户目录
  cd(cur_dir_addr, "etc");

  fseek(img.file_read, cur_dir_addr, SEEK_SET);
  fread(&cur_dir_inode, sizeof(inode), 1, img.file_read);

  for (unsigned int i : cur_dir_inode.block_id0) {
    if (i == -1) {
      continue;
    }
    fseek(img.file_read, i, SEEK_SET);
    fread(&dir, sizeof(dir), 1, img.file_read);

    for (auto &j : dir) {
      if (strcmp(j.file_name, "user") == 0 || strcmp(j.file_name, "passwd") == 0 ||
          strcmp(j.file_name, "group") == 0) {
        inode tmp{};
        fseek(img.file_read, j.inodeAddr, SEEK_SET);
        fread(&tmp, sizeof(inode), 1, img.file_read);

        //文件名标识别符号判断
        if (((tmp.mode >> 9) & 1) == 0) {
          if (strcmp(j.file_name, "user") == 0) {
            user_inode_Addr = j.inodeAddr;
            user_inode = tmp;
          } else if (strcmp(j.file_name, "passwd") == 0) {
            passwd_inode_Addr = j.inodeAddr;
            passwd_inode = tmp;
          } else if (strcmp(j.file_name, "group") == 0) {
            group_inode_Addr = j.inodeAddr;
            group_inode = tmp;
          }
        }
      }
    }
    if (user_inode_Addr != -1 && passwd_inode_Addr != -1)
      break;
  }

  char file_content[100000];
  char buffer[600];
  int cnt = 0;
  for (int i = 0; i < user_inode.size; i++) {
    if (i % super_block.block_size == 0) {
      //换新的磁盘块
      fseek(img.file_read, user_inode.block_id0[i / super_block.block_size], SEEK_SET);
      fread(&buffer, super_block.block_size, 1, img.file_read);
      cnt = 0;
    }
    file_content[i] = buffer[cnt++];
  }
  buffer[user_inode.size] = '\0';

  if (strstr(buffer, user_name) == nullptr) {
    cout << "Error: User don't exist" << endl;
    strcpy(cur_user_name, tmp_cur_user_name);
    strcpy(cur_user_dir_name, tmp_cur_user_dir_name);
    cur_dir_addr = tmp_cur_dir_addr;
    strcpy(cur_dir_name, tmp_cur_dir_name);
    return;
  }

  delUser(buffer, user_name);
  user_inode.size = strlen(buffer);
  writeFile(user_inode, passwd_inode_Addr, buffer);

  cnt = 0;
  for (int i = 0; i < passwd_inode.size; i++) {
    if (i % super_block.block_size == 0) {
      fseek(img.file_read, passwd_inode.block_id0[i / super_block.block_size], SEEK_SET);
      fread(&buffer, super_block.block_size, 1, img.file_read);
      cnt = 0;
    }
    file_content[i] = buffer[cnt++];
  }
  buffer[passwd_inode.size] = '\0';
  delUser(buffer, user_name);
  passwd_inode.size = strlen(buffer);
  writeFile(passwd_inode, passwd_inode_Addr, buffer);

  //取出group文件内容
  cnt = 0;
  for (int i = 0; i < group_inode.size; i++) {
    if (i % super_block.block_size == 0) {    //超出了这个磁盘块
      //换新的磁盘块
      fseek(img.file_read, group_inode.block_id0[i / super_block.block_size], SEEK_SET);
      fread(&buffer, super_block.block_size, 1, img.file_read);
      cnt = 0;
    }
    file_content[i] = buffer[cnt++];
  }
  buffer[group_inode.size] = '\0';
  delUser(buffer, user_name);
  group_inode.size = strlen(buffer);
  writeFile(group_inode, group_inode_Addr, buffer);

  char c, user_to_delete[110] = {0}, cmp_Cur_Dir_Name[311];
  strcpy(user_to_delete, "/home/");
  strcat(user_to_delete, user_name);
  strcat(user_to_delete, "/");

  strcpy(cmp_Cur_Dir_Name, tmp_cur_dir_name);
  strcat(cmp_Cur_Dir_Name, "/");

  for (i = 0, j = 0; j < 3; i++) {
    c = cmp_Cur_Dir_Name[i];
    if (c != user_to_delete[i])
      break;
    else if (c == '/')
      j++;
  }

  if (j != 3) {
    //恢复现场，回到原来的目录
    strcpy(cur_user_name, "root");
    strcpy(cur_user_dir_name, "root");
    cur_dir_addr = ROOT_DIR_ADDR;
    strcpy(cur_dir_name, "/");
    cd(cur_dir_addr, "home");
    rmdir(cur_dir_addr, user_name);

    strcpy(cur_user_name, tmp_cur_user_name);
    strcpy(cur_user_dir_name, tmp_cur_user_dir_name);
    cur_dir_addr = tmp_cur_dir_addr;
    strcpy(cur_dir_name, tmp_cur_dir_name);
  } else {
    // 在root用户进入a用户目录，删掉a用户，则回到根目录
    strcpy(cur_user_name, "root");
    strcpy(cur_user_dir_name, "root");
    cur_dir_addr = ROOT_DIR_ADDR;    //当前用户目录地址设为根目录地址
    strcpy(cur_dir_name, "/");        //当前目录设为"/"

    // 删除username目录
    cd(cur_dir_addr, "home");
    rmdir(cur_dir_addr, user_name);
    // 回到/home/root
    cd(cur_dir_addr, "..");
  }
  cout << "Delete user success" << endl;
}

void fs::delUser(char *buf, char *user_name) {
  char *ptr = strstr(buf, user_name);
  *ptr = '\0';
  while ((*ptr) != '\n') {
    ptr++;
  }
  //ptr + 1 . 下一个user的开始位置;
  ptr++;
  strcat(buf, ptr);
}

bool fs::login() {
//  char user_name[MAX_USER_NAME] = {0};
//  char passwd[MAX_PASSWD_LEN] = {0};

  string user_name, passwd;

  cout << "Please login" << endl;
  cout << "username: ";
//  fflush(stdin);
//  cin.getline(user_name, MAX_USER_NAME);
  user_name = getRegular(MAX_USER_NAME);
  cout << "password: ";
//  fflush(stdin);
//  cin.getline(passwd, MAX_PASSWD_LEN);
  passwd = getPasswd(MAX_PASSWD_LEN);
  fflush(stdout);
  fflush(stdin);
  if (access(user_name.c_str(), passwd.c_str())) {
    is_login = true;
    return true;
  } else {
    is_login = false;
    return false;
  }
}

bool fs::access(const char *user_name, const char *passwd) {
  int user_inode_addr = -1;
  int passwd_inode_addr = -1;
  inode user_inode{};
  inode passwd_inode{};
  inode cur_dir_inode{};
  Dir dir[Dir_ITEM_NUM_PER_BLOCK];
  //找到user文件和passwd文件的inode地址
  cd(cur_dir_addr, "etc");
  fseek(img.file_read, cur_dir_addr, SEEK_SET);
  fread(&cur_dir_inode, sizeof(inode), 1, img.file_read);
  for (int i : cur_dir_inode.block_id0) {
    if (i == -1) {
      continue;
    }
    fseek(img.file_read, i, SEEK_SET);
    fread(&dir, sizeof(dir), 1, img.file_read);
    for (auto &j : dir) {
      if (strcmp(j.file_name, "user") == 0 || strcmp(dir->file_name, "passwd") != 0) {
        inode tmp{};
        fseek(img.file_read, j.inodeAddr, SEEK_SET);
        fread(&tmp, sizeof(inode), 1, img.file_read);
        if (((tmp.mode >> 9) & 1) == 0) {
          //文件
          if (strcmp(j.file_name, "user") == 0) {
            user_inode_addr = j.inodeAddr;
            user_inode = tmp;
          } else if (strcmp(j.file_name, "passwd") == 0) {
            passwd_inode_addr = j.inodeAddr;
            passwd_inode = tmp;
          }
        }
      }
    }
    if (user_inode_addr != -1 && passwd_inode_addr != -1) {
      break;
    }
  }

  char user[100000];
  char buffer[600];
  int block_cnt = 0;
  int i;
  for (i = 0; i < user_inode.size; i++) {
    if (i % super_block.block_size == 0) {
      //需要用新的磁盘块
      fseek(img.file_read, user_inode.block_id0[i / super_block.block_size], SEEK_SET);
      fread(buffer, super_block.block_size, 1, img.file_read);
      block_cnt = 0;
    }
    user[i] = buffer[block_cnt++];
  }
  user[i] = '\0';
  char user_name_j[MAX_USER_NAME + 2];
  sprintf(user_name_j, "%s:x", user_name);
  if (strstr(user, user_name_j) == nullptr) {
    cout << "Warning: User doesn't exist" << endl;
    cd(cur_dir_addr, "..");
    return false;
  }

  block_cnt = 0;
  for (i = 0; i < passwd_inode.size; i++) {
    if (i % super_block.block_size == 0) {
      //需要用新的磁盘块
      fseek(img.file_read, passwd_inode.block_id0[i / (super_block.block_size)], SEEK_SET);
      fread(buffer, super_block.block_size, 1, img.file_read);
      block_cnt = 0;
    }
    user[i] = buffer[block_cnt++];
  }
  user[i] = '\0';
  char *p;
  if ((p = strstr(user, user_name)) == nullptr) {
    cout << "User doesn't exist in passwd file" << endl;
    cd(cur_dir_addr, "..");
    return false;
  }

  while ((*p) != ':') {
    p++;
  }
  p++;
  block_cnt = 0;
  while ((*p) != '\n') {
    buffer[block_cnt++] = *p;
    p++;
  }
  buffer[block_cnt] = '\0';
  //密码
  if (strcmp(buffer, passwd) == 0) {    //正确
    strcpy(cur_user_name, user_name);
    if (strcmp(user_name, "root") == 0)
      strcpy(cur_group_name, "root");    //当前登陆用户组名
    else
      strcpy(cur_group_name, "user");    //当前登陆用户组名
    cd(cur_dir_addr, "..");
    cd(cur_dir_addr, "home");
    cd(cur_dir_addr, user_name);    //进入到用户目录
    strcpy(cur_user_dir_name, cur_dir_name);    //复制当前登陆用户目录名
    return true;
  } else {
    cout << "Wrong Password !" << endl;
    cd(cur_dir_addr, "..");    //回到根目录
    return false;
  }
}

void fs::lsl(int parent_inode_addr, bool isAll) {
  inode cur{};
  //取当前目录inode
  fseek(img.file_read, parent_inode_addr, SEEK_SET);
  fread(&cur, sizeof(inode), 1, img.file_read);
  fflush(img.file_read);


  //取出目录项数
  int cnt = cur.link_num;


  //判断文件模式。
  if (!isPermitRead(cur)) {
    cout << "WARNING: Permission denied(NO READ AUTHORITY)" << endl;
    return;
  }

  //依次取出磁盘块
  int i = 0;
  while (i < cnt && i < BLOCK_NUM_PER_INODE) {
    Dir dir[Dir_ITEM_NUM_PER_BLOCK] = {0};
    if (cur.block_id0[i / Dir_ITEM_NUM_PER_BLOCK] == -1) {
      i += Dir_ITEM_NUM_PER_BLOCK;
      continue;
    }

    fseek(img.file_read, cur.block_id0[i / Dir_ITEM_NUM_PER_BLOCK], SEEK_SET);
    fread(&dir, sizeof(dir), 1, img.file_read);
    fflush(img.file_read);

    for (int j = 0; j < Dir_ITEM_NUM_PER_BLOCK && i < cnt; j++) {

      inode tmp{};
      //取出该目录项的inode，判断该目录项是目录还是文件
      fseek(img.file_read, dir[j].inodeAddr, SEEK_SET);
      fread(&tmp, sizeof(inode), 1, img.file_read);
      fflush(img.file_read);

      if (strcmp(dir[j].file_name, "") == 0) {
        continue;
      }

      if (i > 2 && ((strcmp(dir[j].file_name, ".") == 0 || strcmp(dir[j].file_name, "..") == 0))) {
        continue;
      }
      //当没有使用ls -a时，每找到一个以"."开头的文件就需要跳过，并把i加1，防止多显示。
      if (!isAll && dir[j].file_name[0] == '.') {
        i++;
        continue;
      }
      //输出信息
      if (((tmp.mode >> 9) & 1) == 1) {
        cout << "d";
      } else {
        cout << "-";
      }

      int permiss_index = 8;
      while (permiss_index >= 0) {
        if (((tmp.mode >> permiss_index) & 1) == 1) {
          if (permiss_index % 3 == 2) cout << "r";
          if (permiss_index % 3 == 1) cout << "w";
          if (permiss_index % 3 == 0) cout << "x";
        } else {
          cout << "-";
        }
        permiss_index--;
      }
      cout << "\t";
      cout << tmp.link_num << "\t";    //该文件链接
      cout << tmp.user_name << "\t";    //文件所属用户名
      cout << tmp.group_name << "\t";    //文件所属用户名
      cout << tmp.size << " B\t";    //文件大小
      cout << dir[j].file_name << endl;    //文件名
      i++;
    }

  }
}