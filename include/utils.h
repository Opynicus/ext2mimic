//
// Created by 23170 on 2022/5/24.
//

#ifndef EXT2MIMIC_UTILS_H
#define EXT2MIMIC_UTILS_H
#include "params.h"
#include <string>

string _getPasswd();
string _getRegular();
string _getLine(int maxNum, string(*func)());
string getPasswdConfirm(int maxNum);
string getRegular(int maxNum);
string getPasswd(int maxNum);
#endif //EXT2MIMIC_UTILS_H
