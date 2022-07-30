#pragma once

#include "winsock2.h"
#include <stdio.h>
#include <stdlib.h>

#define init_winsock	WSADATA wsaData;\
						int iResult= WSAStartup(MAKEWORD(2,2), &wsaData);\
						if (iResult != NO_ERROR)\
							printf("Error at WSAStartup()\n")

#define create_socket(s, src) do{\
									if (((s) = socket(AF_INET, SOCK_DGRAM, 0)) <= 0 ) {\
									printf("[%s]: error in socket creation, %s\n",src, strerror(errno));\
									status = 1;\
									}\
								}while(0)


/*struct sockaddr_in {
	short sin_family; // Address family, AF_INET (IPv4)
	u_short sin_port; // Port number
	struct in_addr sin_addr; // Internet address
	char sin_zero[8]; // For same size as struct sockaddr
};*/

/*copies 'nof_bits' from 'src' (from 'src_idx', 'src_offset') to 'trg' (from 'trg_idx', 'trg_offset')*/
void place_bits(char* trg, int trg_idx, int trg_offset, char* src, int src_idx, int src_offset, int nof_bits);

void set_mask(char* mask, int nof_1s);

void printBits(int const size, void const* const ptr);

int sendall(SOCKET s, char* buf, int* len, struct sockaddr* addr);

void int2char(int n, char* c);

void char2int(int* n, unsigned char* c);