/******************************************
File	:debug.c 
Author	:linyang
Date	:11/21/2006
comment :用于调试代码
******************************************/

#include "debug.h"

void ASSERT(int f)
{
	if( f==1 )
	{
		return;
	}
	else
	{
// 		__asm{
// 			int 3;}
	}
}

