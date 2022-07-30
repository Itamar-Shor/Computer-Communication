#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include "utils.h"
#include <io.h>
#include <fcntl.h>
#include <windows.h>

#define PORT_NUM_IDX 1
#define FILE_NAME_IDX 2

#pragma comment(lib, "Ws2_32.lib")

/*-------------------------------------------functions------------------------------------------------*/
void decode(char* data, int write_data_index, int write_data_offset);
void store_data(FILE* fd, char* data, int len);
void run(SOCKET receiver_socket, fd_set read_fds, FILE* fd);
char init(SOCKET* receiver_socket, fd_set* read_fds, struct sockaddr_in* remote_addr_receiver, FILE** fd, HANDLE* thread);
void ready2end(SOCKET* receiver_socket, FILE* fd, HANDLE* thread);
DWORD WINAPI  wait_for_stdin(void* param);
/*----------------------------------------------------------------------------------------------------*/

/*-----------------------------------------global fields----------------------------------------------*/
int nof_bytes_received = 0, nof_bytes_written = 0, nof_bad_frames = 0, nof_corrected_frames = 0;
char write_data[1100] = { 0 };
int port_num;
char* file_name;
int stop[1] = { 0 };
HANDLE ghMutex;
/*----------------------------------------------------------------------------------------------------*/

int main(int argc, char* argv[])
{
	FILE* fd = NULL;
	SOCKET receiver_socket;
	struct sockaddr_in remote_addr_receiver;
	fd_set read_fds;
	HANDLE thread;

	port_num = atoi(argv[PORT_NUM_IDX]);
	file_name = argv[FILE_NAME_IDX];

	/*init*/
	init_winsock;
	if (init(&receiver_socket, &read_fds, &remote_addr_receiver,&fd, &thread)){
		printf("[receiver]: init failes aborting...");
		exit(-1);
	}
	

	/*run*/
	run(receiver_socket, read_fds, fd);

	/*finish*/
	ready2end(&receiver_socket, fd, thread);
	return 0;
}


void decode(char* data, int write_data_index, int write_data_offset) {
	char mask, xor_bit_1 = 0, xor_bit_2 = 0, xor_bit_4 = 0, xor_bit_8 = 0;
	int idx, offset, i, bit_count = 0;
	char error_idx = 0;

	for (i = 1; i <= 15; i++) {
		idx = (i <= 8) ? 0 : 1;
		mask = 1 << ((i - 1) % 8);
		if ((i & 0x1) == 0x1) {
			xor_bit_1 ^= (((data[idx] & mask) >> ((i - 1) % 8)) & 0x1);
		}
		if ((i & 0x2) == 0x2) {
			xor_bit_2 ^= (((data[idx] & mask) >> ((i - 1) % 8)) & 0x1);
		}
		if ((i & 0x4) == 0x4) {
			xor_bit_4 ^= (((data[idx] & mask) >> ((i - 1) % 8)) & 0x1);
		}
		if ((i & 0x8) == 0x8) {
			xor_bit_8 ^= (((data[idx] & mask) >> ((i - 1) % 8)) & 0x1);
		}
	}
	error_idx = (xor_bit_8 << 3) | (xor_bit_4 << 2) | (xor_bit_2 << 1) | (xor_bit_1);
	if (error_idx != 0 && ((error_idx & (error_idx - 1)) != 0)){ // if the error is in data bit.
		nof_bad_frames++;
		nof_corrected_frames++;
		idx = (error_idx > 8) ? 1 : 0;
		mask = 1 << ((error_idx-1) % 8);
		data[idx] ^= mask;		// flip back the bit.
	}
	idx = 0;
	offset = 0;
	for (i = 1; i <= 8; i++) {
		if ((i & (i - 1)) != 0) { //check if is some power of 1
			place_bits(write_data, write_data_index, write_data_offset, data, (i-1) / 8, (i-1) % 8, 1);
			write_data_index += ((write_data_offset + 1) / 8);
			write_data_offset = (write_data_offset + 1) % 8;
			bit_count++;
		}
	}
	place_bits(write_data, write_data_index, write_data_offset, data, 1, 0, 11 - bit_count);  // on the second byte of res all the bits are data bits.
}

void store_data(FILE* fd, char* data, int len) {
	fwrite(data, sizeof(char), len, fd);
	nof_bytes_written += len;
}


void run(SOCKET receiver_socket, fd_set read_fds, FILE* fd) {
	int count = 0, curr_count = 0, addr_len, select_status;
	char stdin_buff[4] = { 0 };
	char receive_buff[1500] = { 0 };
	char feedback_buff[12] = { 0 }; // 3 integers.
	char message[2] = { 0 };
	int write_data_index = 0, write_data_offset = 0;
	int receive_idx = 0, receive_offset = 0;
	struct sockaddr channel_addr;
	struct timeval timeout = { 0,0 };

	while (1) {
		select_status = select(receiver_socket, &read_fds, NULL, NULL, &timeout);
		if (select_status < 0) {
			perror("");
			printf("[receiver]: error in select, %s\n", strerror(errno));
			return;
		}
		WaitForSingleObject(ghMutex, INFINITE);
	
		if (*stop == 1) {
			curr_count = sizeof(feedback_buff);
			int2char(nof_bytes_received, feedback_buff);
			int2char(nof_bytes_written, feedback_buff + 4);
			int2char(nof_corrected_frames, feedback_buff + 8);
			if (sendall(receiver_socket, feedback_buff, &curr_count, &channel_addr) < 0) {
				printf("[receiver]: error in send message to sender, %s\n", strerror(errno));
			}
			fprintf(stderr, "\nreceived: %d bytes\nwrote: %d bytes\ndetected & corrected: %d errors\n\n", nof_bytes_received, nof_bytes_written, nof_corrected_frames);
			ReleaseMutex(ghMutex);
			return;
		}
		if (select_status == 0) {
			FD_SET(receiver_socket, &read_fds);
			ReleaseMutex(ghMutex);
			continue;
		}

		if (FD_ISSET(receiver_socket, &read_fds)) {
			count = 0;
			addr_len = sizeof(channel_addr);
			memset(receive_buff, 0, 1500);
			count = recvfrom(receiver_socket, receive_buff, sizeof(receive_buff), 0, &channel_addr, &addr_len);
			memset(write_data, 0, 1100);
			write_data_index = 0;
			write_data_offset = 0;
			nof_bytes_received += count;
			receive_idx = 0;
			receive_offset = 0;
			for (int i = 0; i < ((count*8)/15); i++) {
				memset(message, 0, 2);
				place_bits(message, 0, 0, receive_buff, receive_idx, receive_offset, 15);
				receive_idx = (receive_offset >= 1) ? receive_idx + 2 : receive_idx + 1;
				receive_offset = (receive_offset + 15) % 8;
				decode(message, write_data_index, write_data_offset);
				write_data_index = (write_data_offset >= 5) ? write_data_index + 2 : write_data_index + 1;
				write_data_offset = (write_data_offset + 11) % 8;
			}
			store_data(fd, write_data, (count/15)*11);
		}
		// free lock
		ReleaseMutex(ghMutex);
	}
	return;
}

char init(SOCKET* receiver_socket, fd_set* read_fds, struct sockaddr_in* remote_addr_receiver, FILE** fd, HANDLE* thread) {
	int status = 0;

	create_socket(*receiver_socket, "receiver");

	*fd = fopen(file_name, "wb");
	if (!(*fd)) {
		printf("[receiver]: cannot open file: %s\n", file_name);
		status = 1;
	}

	remote_addr_receiver->sin_family = AF_INET;
	remote_addr_receiver->sin_addr.s_addr = INADDR_ANY;
	remote_addr_receiver->sin_port = htons(port_num);


	//binding socket to channel port
	if (bind(*receiver_socket, (SOCKADDR*)remote_addr_receiver, sizeof(*remote_addr_receiver)) < 0) {
		printf("[receiver]: error in bind, %s\n", strerror(errno));
		status = 1;
	}

	ghMutex = CreateMutex(NULL, FALSE, NULL);
	if (ghMutex == NULL) {
		printf("[receiver]: mutex creation failed, %s\n", strerror(errno));
		status = 1;
	}
	*thread = CreateThread(NULL, 0, wait_for_stdin, NULL, 0, NULL);
	if (!thread) {
		printf("[receiver]: thread creation failed, %s\n", strerror(errno));
		status = 1;
	}

	FD_ZERO(read_fds);
	FD_SET(*receiver_socket, read_fds);
	return status;
}

void ready2end(SOCKET* receiver_socket, FILE* fd, HANDLE* thread) {
	closesocket(*receiver_socket);
	fclose(fd);
//	CloseHandle(*thread);
	CloseHandle(ghMutex);
	return;
}

DWORD WINAPI  wait_for_stdin(void* param) {

	char ch;
	int turn = 0;

	while (_read(_fileno(stdin), &ch, 1) > 0)
	{
		switch (turn) {
		case 0:
			if (ch == 'E') {
				turn = 1;
			}
			else turn = 0;
			break;
		case 1:
			if (ch == 'n') {
				turn = 2;
			}
			else turn = 0;
			break;
		case 2:
			if (ch == 'd') {
				turn = 3;
			}
			else turn = 0;
			break;
		}

		if (turn == 3) {
			break;
		}
	}
	WaitForSingleObject(ghMutex, INFINITE);
	*stop = 1;
	ReleaseMutex(ghMutex);
	return 0;
}