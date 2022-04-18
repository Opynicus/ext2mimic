//
// Created by Opynicus on 2022/4/17.
//

#include "../include/vimimic.h"

vimimic::vimimic() {
    //初始化vimimic的位置
    handle_out = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleScreenBufferInfo(handle_out, &screen_info);
    window_x = screen_info.srWindow.Right - screen_info.srWindow.Left + 1;
    window_y = screen_info.srWindow.Bottom - screen_info.srWindow.Top + 1;
    cur_x = screen_info.dwCursorPosition.X;
    cur_y = screen_info.dwCursorPosition.Y;
    last_x = screen_info.dwCursorPosition.X;
    last_y = screen_info.dwCursorPosition.Y;
    mode = 0;
}

bool vimimic::method(char *buf, int &cnt, int &max_len) {
    unsigned char c;
    if (mode == 0) {    //命令行模式
        c = getch();
        if (c == 'i' || c == 'a' || c == 'o') {    //插入模式
            if (c == 'a') {
                cur_x++;
                if (cur_x == window_x) {
                    cur_x = 0;
                    cur_y++;
                }
            } else if (c == 'o') {
                buf[cnt] = '\n';
                cur_y++;
                if (cur_x == window_x) {
                    cur_x = 0;
                    cur_y++;
                }
            }
            if (cur_y > window_y - 2 || cur_y % (window_y - 1) == window_y - 2) {
                //超过屏幕范围，翻页
                if (cur_y % (window_y - 1) == window_y - 2)
                    printf("\n");
                SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // 恢复原来的属性
                for (int i = 0; i < window_x - 1; i++)
                    printf(" ");
                redirectPos(handle_out, 0, cur_y + 1);
                printf(" - Insert mode - ");
                redirectPos(handle_out, 0, cur_y);
            } else {
                //显示 "插入模式"
                redirectPos(handle_out, 0, window_y - 1);
                SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // 恢复原来的属性
                for (int i = 0; i < window_x - 1; i++)
                    printf(" ");
                redirectPos(handle_out, 0, window_y - 1);
                printf(" - Insert mode - ");
                redirectPos(handle_out, cur_x, cur_y);
            }
            redirectPos(handle_out, cur_x, cur_y);
            mode = 1;
        }
        else if(c == 224) {	//判断左右按键
            c = getch();
            if(c == 75) {
                moveLeft(buf, cnt);
            }
            else if(c == 77) {
                moveRight(buf, cnt, max_len);
            }
            return true;
        }
        else if (c == 'x') {
            return delChar(buf, cnt);
        } else if (c == ':') {
            if(cur_y-window_y + 2 > 0)
                redirectPos(handle_out, 0, cur_y + 1);
            else
                redirectPos(handle_out, 0, window_y - 1);
            if(cur_y - window_y + 2 > 0)
                pos.X = 0, pos.Y = cur_y + 1;
            else
                pos.X = 0, pos.Y = window_y - 1;
            SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // 恢复原来的属性
            for(int i = 0;i < window_x - 1; i++)
                printf(" ");

            if(cur_y - window_y + 2 > 0)
                redirectPos(handle_out,0,cur_y + 1);
            else
                redirectPos(handle_out,0,window_y - 1);
            WORD att = BACKGROUND_RED | BACKGROUND_BLUE | FOREGROUND_INTENSITY; // 文本属性
            SetConsoleTextAttribute(handle_out, FOREGROUND_INTENSITY | BACKGROUND_RED | BACKGROUND_BLUE | FOREGROUND_GREEN  );	//设置文本颜色
            printf(":");

            char cur_c;
            int input_char_num = 1;	//输入的字符计数
            while((c = getch())) {
                if(c == '\r') {	//回车
                    break;
                }
                else if(c == '\b') {	//退格，从命令条删除一个字符
                    input_char_num--;
                    if(input_char_num == 0)
                        break;
                    printf("\b");
                    printf(" ");
                    printf("\b");
                    continue;
                }
                cur_c = c;
                printf("%c", cur_c);
                input_char_num++;
            }
            if(cur_c == 'q') { //退出
                buf[cnt] = '\0';
                SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // 恢复原来的属性
                system("cls");
                return false;
            }
            else {
                if(cur_y - window_y + 2 > 0)
                    redirectPos(handle_out,0, cur_y + 1);
                else
                    redirectPos(handle_out,0, window_y - 1);
                SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // 恢复原来的属性
                for(int i = 0;i < window_x - 1; i++)
                    printf(" ");

                if(cur_y - window_y + 2 > 0)
                    redirectPos(handle_out, 0, cur_y + 1);
                else
                    redirectPos(handle_out, 0, window_y - 1);
                SetConsoleTextAttribute(handle_out, FOREGROUND_INTENSITY | BACKGROUND_RED | BACKGROUND_BLUE | FOREGROUND_GREEN  );	//设置文本颜色
                printf(" Wrong Command");
                SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // 恢复原来的属性
                redirectPos(handle_out, cur_x, cur_y);
            }
        } else if(c == 27) {	//ESC，命令行模式，清状态条
            redirectPos(handle_out,0,window_y - 1);
            SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // 恢复原来的属性
            for(int i = 0;i < window_x - 1; i++)
                printf(" ");
            redirectPos(handle_out,cur_x,cur_y);
        }
    } else if(mode == 1) {	//插入模式

        redirectPos(handle_out, window_x / 4 * 3, window_y - 1);
        int i = window_x / 4 * 3;
        while(i < window_x - 1) {
            printf(" ");
            i++;
        }
        if(cur_y > window_y - 2)
            redirectPos(handle_out, window_x / 4 * 3, cur_y + 1);
        else
            redirectPos(handle_out, window_x/ 4 * 3, window_y - 1);
        printf("[column:%d, row:%d]",cur_x == -1 ? 0 : cur_x,cur_y);
        redirectPos(handle_out, cur_x, cur_y);

        c = getch();
        if(c == 27) {	// ESC，进入命令模式
            mode = 0;
            //清状态条
            redirectPos(handle_out, 0, window_y-1);
            SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // 恢复原来的属性
            redirectPos(handle_out, 0, window_y-1);
            for(int i = 0;i < window_x - 1;i++)
                printf(" ");
            redirectPos(handle_out, cur_x, cur_y);
            return true;
        } else if(c == '\b') {
            return delChar(buf, cnt);
        }
        else if(c == 224) {	//判断左右按键
            c = getch();
            if(c == 75) {
                moveLeft(buf, cnt);
            }
            else if(c == 77) {
                moveRight(buf, cnt, max_len);
            }
            return true;
        }
        if(c == '\r') {	//遇到回车
            printf("\n");
            cur_x = 0;
            cur_y++;

            if(cur_y > window_y - 2 || cur_y % (window_y - 1) == window_y - 2) {
                //超过这一屏，向下翻页
                if(cur_y % (window_y - 1) == window_y - 2)
                    printf("\n");
                SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // 恢复原来的属性
                for(int i = 0; i < window_x - 1;i++)
                    printf(" ");
                redirectPos(handle_out, 0, cur_y+1);
                printf(" - Insert mode - ");
                redirectPos(handle_out,0,cur_y);
            }
            buf[cnt++] = '\n';
            if(cnt>max_len)
                max_len = cnt;
            return true;
        }
        else{
            printf("%c", c);
        }
        //移动光标
        cur_x++;
        if(cur_x == window_x) {
            cur_x = 0;
            cur_y++;

            if(cur_y > window_y - 2 || cur_y % (window_y - 1) == window_y - 2) {
                //超过这一屏，向下翻页
                if(cur_y % (window_y - 1) == window_y - 2)
                    printf("\n");
                SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // 恢复原来的属性
                int i;
                for(i=0;i<window_x-1;i++)
                    printf(" ");
                redirectPos(handle_out,0,cur_y+1);
                printf(" - Insert mode - ");
                redirectPos(handle_out,0,cur_y);
            }

            buf[cnt++] = '\n';
            if(cnt > max_len)
                max_len = cnt;
            if(cur_y == window_y) {
                printf("\n");
            }
        }
        //记录字符
        buf[cnt++] = c;
        if(cnt > max_len)
            max_len = cnt;
    }
}

/*
 * 改坐标
 */
void vimimic::redirectPos(HANDLE hout, int x, int y) {
    COORD pos;
    pos.X = x;
    pos.Y = y;
    SetConsoleCursorPosition(hout, pos);
}

/*
 * 光标左移
 */
void vimimic::moveLeft(char* buf, int &cnt) {  //左移
    if(cnt != 0) {
        cnt--;
        cur_x--;
        if(buf[cnt] == '\n') {
            //上一个字符是回车
            if(cur_y != 0)
                cur_y--;
            int k;
            cur_x = 0;
            for(k = cnt - 1; buf[k] != '\n' && k >= 0; k--)
                cur_x++;
        }
        redirectPos(handle_out, cur_x, cur_y);
    }
}

/*
 * 光标右移
 */
void vimimic::moveRight(char *buf, int &cnt, int &max_len) { //右移
    cnt++;
    if(cnt > max_len)
        max_len = cnt;
    cur_x++;
    if(cur_x == window_x) {
        cur_x = 0;
        cur_y++;
        if(cur_y > window_y-2 || cur_y % (window_y - 1) == window_y - 2) {
            //超过这一屏，向下翻页
            if(cur_y % (window_y - 1) == window_y - 2)
                printf("\n");
            SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // 恢复原来的属性
            for(int i = 0;i < window_x - 1; i++)
                printf(" ");
        }
    }
    redirectPos(handle_out, cur_x, cur_y);
}

bool vimimic::delChar(char *buf, int &cnt) {
    if(cnt == 0)	//已经退到最开始位置
        return true;
    printf("\b");
    printf(" ");
    printf("\b");
    cur_x--;
    cnt--;	//删除字符
    if(buf[cnt] == '\n') {
        //要删除的这个字符是回车，光标回到上一行
        if(cur_y != 0)
            cur_y--;
        cur_x = 0;
        for(int k = cnt - 1;buf[k] != '\n' && k >= 0; k--)
            cur_x++;
        redirectPos(handle_out, cur_x, cur_y);
        printf(" ");
        redirectPos(handle_out, cur_x, cur_y);
        if(cur_y - window_y + 2 >= 0) {	//翻页时
            redirectPos(handle_out, cur_x, 0);
            redirectPos(handle_out, cur_x, cur_y+1);
            SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // 恢复原来的属性
            for(int i = 0;i < window_x - 1; i++)
                printf(" ");
            redirectPos(handle_out, 0,cur_y + 1);
        }
        redirectPos(handle_out, cur_x, cur_y);
    }
    else
        buf[cnt] = ' ';
    return true;
}