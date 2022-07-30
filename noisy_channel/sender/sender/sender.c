#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include "utils.h"
#include <math.h>

#define IP_ADDR_IDX 1
#define PORT_NUM_IDX 2
#define FILE_NAME_IDX 3

#pragma comment(lib, "Ws2_32.lib")

/*-------------------------------------------functions------------------------------------------------*/
void encode(char* data, int packet_idx, int packet_offset, char* curr_packet);
void run(FILE* fd, SOCKET send_socket, fd_set feedback_set, struct sockaddr_in remote_addr);
char handle_feedback(SOCKET s, fd_set feedback_set, struct timeval* timeout);
char init(SOCKET* send_socket, fd_set* feedback_set, struct sockaddr_in* remote_addr, FILE** fd);
void ready2end(SOCKET* send_socket, FILE* fd);
char send_data(char* buff, char* packet, int packet_size, int nof_messages, SOCKET sender_socket, struct sockaddr_in remote_addr);
/*----------------------------------------------------------------------------------------------------*/

/*-----------------------------------------global fields----------------------------------------------*/
char packet[1500];
char* ip_addr, *file_name;
int port_num;
/*----------------------------------------------------------------------------------------------------*/

int main(int argc, char* argv[]) 
{
	SOCKET sender_socket;
	struct sockaddr_in remote_addr_channel;
	fd_set feedback_set;
	FILE* fd;

	ip_addr = argv[IP_ADDR_IDX];
	port_num = atoi(argv[PORT_NUM_IDX]);
	file_name = argv[FILE_NAME_IDX];

	/*init*/
	init_winsock;
	if (init(&sender_socket, &feedback_set, &remote_addr_channel, &fd)) {
		printf("[sender]: init failes aborting...");
		exit(-1);
	}

	/*run*/
	run(fd, sender_socket, feedback_set, remote_addr_channel);
	
	/*finish*/
	ready2end(&sender_socket, fd);
	return 0;
}


void run(FILE* fd, SOCKET send_socket, fd_set feedback_set, struct sockaddr_in remote_addr)
{
	char buff[1100] = { 0 };
	int last_packet_size, last_nof_msgs;
	int curr_bytes_read;
	char* last_packet;

	//read 1100 bytes at a time
	while ((curr_bytes_read = fread(buff, sizeof(char), 1100, fd)) == 1100) {
		memset(packet, 0, 1500);
		send_data(buff, packet, sizeof(packet), 800, send_socket, remote_addr);
	}
	//send last packet (if exists)
	if (curr_bytes_read > 0) {
		last_nof_msgs = (curr_bytes_read * 8) / 11; //nof messages
		last_packet_size = (int)ceil(((double)last_nof_msgs * 15) / 8);
		last_packet = (char*)calloc(last_packet_size, sizeof(char));
		send_data(buff, last_packet, last_packet_size, last_nof_msgs, send_socket, remote_addr);
		free(last_packet);
	}
	handle_feedback(send_socket, feedback_set, NULL); //blocking
	return;
}

void encode(char* data, int packet_idx, int packet_offset, char* curr_packet)
{
	int  i, bit_count = 0;
	char mask, xor_bit_1 = 0, xor_bit_2 = 0, xor_bit_4 = 0, xor_bit_8 = 0;
	int idx;
	char res[2] = { 0 };

	for (i = 1; i <= 8; i++) {
		if ((i & (i - 1)) != 0) { //check if is some power of 2
			place_bits(res, (i - 1) / 8, (i - 1) % 8, data, bit_count / 8, bit_count % 8, 1);
			bit_count++;
		}
	}
	place_bits(res, 1, 0, data, bit_count / 8, bit_count % 8, 11 - bit_count);  // on the second byte of res all the bits are data bits.
	//calc check bits.
	for (i = 1; i <= 15; i++) {
		if ((i & (i - 1)) != 0) {
			idx = (i < 8) ? 0 : 1;
			mask = 1 << ((i - 1) % 8);
			if ((i & 0x1) == 0x1) {
				xor_bit_1 ^= (((res[idx] & mask) >> ((i - 1) % 8)) & 0x1);
			}
			if ((i & 0x2) == 0x2) {
				xor_bit_2 ^= (((res[idx] & mask) >> ((i - 1) % 8)) & 0x1);
			}
			if ((i & 0x4) == 0x4) {
				xor_bit_4 ^= (((res[idx] & mask) >> ((i - 1) % 8)) & 0x1);
			}
			if ((i & 0x8) == 0x8) {
				xor_bit_8 ^= (((res[idx] & mask) >> ((i - 1) % 8)) & 0x1);
			}
		}
	}
	res[0] |= ((xor_bit_1) | (xor_bit_2 << 1) | (xor_bit_4 << 3) | (xor_bit_8 << 7));
	place_bits(curr_packet, packet_idx, packet_offset, res, 0, 0, 15);
}

char handle_feedback(SOCKET s, fd_set feedback_set,struct timeval* timeout){
	int nof_received = 0, nof_written = 0, nof_corrected = 0;
	int count = 0, curr_count = 0;
	char feedback_buff[12] = { 0 }; //3 integers.

	if (select(s, &feedback_set, NULL, NULL, timeout) < 0) {
		printf("[sender]: error in select %s\n", strerror(errno));
	}
	if (FD_ISSET(s, &feedback_set)) {
		count = recvfrom(s, feedback_buff, sizeof(feedback_buff), 0, NULL, NULL);
		if (count < 0) {
			printf("[sender]: error in recvfrom %s\n", strerror(errno));
		}
		char2int(&nof_received, (unsigned char*)feedback_buff);
		char2int(&nof_written, (unsigned char*)feedback_buff + 4);
		char2int(&nof_corrected, (unsigned char*)feedback_buff + 8);
		fprintf(stderr, "\nreceived: %d bytes\nwritten: %d bytes\ndetected & corrected %d errors\n\n", (unsigned int)nof_received, (unsigned int)nof_written, (unsigned int)nof_corrected);
		return 1;
	}
	//should never get here (if timeout == NULL)
	FD_SET(s, &feedback_set);
	return 0;
}

char send_data(char* buff, char* packet, int packet_size, int nof_messages, SOCKET sender_socket, struct sockaddr_in remote_addr) {
	int packet_idx = 0, packet_offset = 0;
	int idx = 0, offset = 0;
	char message[2] = { 0 };
//	memset(packet, 0, 1500);

	for (int i = 0; i < nof_messages; i++) {
		memset(message, 0, 2);
		place_bits(message, 0, 0, buff, idx, offset, 11);
		idx = (offset >= 5) ? idx + 2 : idx + 1;
		offset = (offset + 11) % 8;
		encode(message, packet_idx, packet_offset, packet);
		packet_idx = (packet_offset >= 1) ? packet_idx + 2 : packet_idx + 1;
		packet_offset = (packet_offset + 15) % 8;
	}
	if (sendall(sender_socket, packet, &packet_size, (SOCKADDR*)&remote_addr) < 0) {
		printf("[sender]: error in send message to channel, %s\n", strerror(errno));
		return 1;
	}
	return 0;
}


char init(SOCKET* send_socket, fd_set* feedback_set, struct sockaddr_in* remote_addr, FILE** fd){

	int status = 0;

	*fd = fopen(file_name, "rb");
	if (!(*fd)) {
		printf("[sender]: cannot open file: %s\n", file_name);
		status = 1;
	}

	create_socket(*send_socket, "sender");

	remote_addr->sin_family = AF_INET;
	remote_addr->sin_addr.s_addr = inet_addr(ip_addr);
	remote_addr->sin_port = htons(port_num);

	FD_ZERO(feedback_set);
	FD_SET(*send_socket, feedback_set);

	return status;
}

void ready2end( SOCKET* send_socket, FILE* fd) {
	closesocket(*send_socket);
	fclose(fd);
	return;
}