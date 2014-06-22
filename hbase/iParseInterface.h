#ifndef INTERFACE_H_H
#define INTERFACE_H_H


#include<stdio.h>
#include"str.h"
#include"parse.h"

#ifdef __cplusplus
extern "C"{
__declspec(dllexport)  RC parse(char* st,sqlstr* sqln);
};
#endif
#endif