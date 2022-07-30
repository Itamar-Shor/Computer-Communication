#pragma once


#define TIME		0
#define SRC_IP		1
#define SRC_PORT	2
#define DST_IP		3
#define DST_PORT	4
#define LEN			5
#define WEIGHT		6
#define MAX_ARGS	7

#define MAX_ROW_LEN 500
#define MAX_LINKS   9999
#define IP_ADDR_MAX_LEN  16


#define malloc_check(func, name, ptr)\
	do{\
		if(!(ptr)){\
			printf("[%s]: malloc failed on - %s\n", func, name);\
			exit(-1);\
		}\
	} while(0)