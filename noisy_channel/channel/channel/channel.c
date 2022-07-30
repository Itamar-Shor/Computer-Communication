#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include "utils.h"
#include <math.h>
#include <ws2tcpip.h>

#define PORT_NUM_IDX 1
#define IP_ADDR_IDX 2
#define RECEIVER_PORT_NUM_IDX 3
#define ERR_PROB_IDX 4
#define SEED_IDX 5

#pragma comment(lib, "Ws2_32.lib")

/*-------------------------------------------functions------------------------------------------------*/
int send_message(char* data, int len, double prob, SOCKET s, struct sockaddr* addr);
void run(SOCKET channel_socket, fd_set read_fds, struct sockaddr_in receiver_sockaddr, double error_prob);
char randomWithProb(double p);
void make_some_noise(char* data, double prob, int len);
char init(SOCKET* channel_socket, fd_set* read_fds, struct sockaddr_in* channel_addr, struct sockaddr_in* remote_addr_receiver);
void ready2end(SOCKET* channel_socket);
/*----------------------------------------------------------------------------------------------------*/

/*-----------------------------------------global fields----------------------------------------------*/
int channel_port, receiver_port, seed;
char* ip_addr;
int nof_bytes = 0, nof_bits_flipped = 0;
double error_prob;
/*----------------------------------------------------------------------------------------------------*/

int main(int argc, char* argv[])
{
	SOCKET cahnnel_socket;
	struct sockaddr_in remote_addr_receiver, channel_addr;
	fd_set read_fds;
	int count = 0, curr_count = 0;
	struct sockaddr sender_sockaddr = { 0 };

	ip_addr = argv[IP_ADDR_IDX];
	channel_port = atoi(argv[PORT_NUM_IDX]);
	receiver_port = atoi(argv[RECEIVER_PORT_NUM_IDX]);
	error_prob = ((double)atoi(argv[ERR_PROB_IDX])) * pow(2, -16);
	seed = atoi(argv[SEED_IDX]);

	/*init*/
	init_winsock;
	if (init(&cahnnel_socket, &read_fds ,&channel_addr, &remote_addr_receiver)) {
		printf("[channel]: init failes aborting...");
		exit(-1);
	}
	/*run*/
	run(cahnnel_socket, read_fds, remote_addr_receiver, error_prob);
	
	/*finish*/
	ready2end(&cahnnel_socket);
	return 0;
}

int send_message(char* data, int len, double prob, SOCKET s, struct sockaddr* addr) {
	make_some_noise(data, prob, len);
	if (sendall(s, data, &len, addr) < 0) { //some failure
		return -1;
	}
	return 0;
}

//https://stackoverflow.com/questions/16255812/random-function-which-generates-1-or-0-with-given-probability
char randomWithProb(double prob) {
	double rndDouble = (double)rand() / RAND_MAX;
	return rndDouble < prob;
}

void make_some_noise(char* data, double prob, int len) {
	char mask;
	for (int byte = 0; byte < len; byte++) {
		mask = 0;
		for (int offset = 0; offset < 8; offset++) {
			if (randomWithProb(prob)) { // flip
				mask |= (1 << offset);
				nof_bits_flipped++;
			}
			data[byte] ^= mask;
		}
	}
}

void run(SOCKET channel_socket , fd_set read_fds, struct sockaddr_in receiver_sockaddr, double error_prob) {
	int count = 0, curr_count = 0;
	struct sockaddr_in clients_addr, sender_addr;
	char packet[1500];
	int len;
	char sender_ip[INET_ADDRSTRLEN];

	while (1) {
		if (select(channel_socket, &read_fds, NULL, NULL, NULL) < 0) {
			printf("[channel]: error in select, %s\n", strerror(errno));
		}
		if (FD_ISSET(channel_socket, &read_fds)) {
			count = 0;
			len = sizeof(clients_addr);
			memset(packet, 0, 1500);

			count = recvfrom(channel_socket, packet, sizeof(packet), 0, (SOCKADDR*)&clients_addr, &len);
			if (clients_addr.sin_port == htons(receiver_port)) { // feedback from receiver
				if (sendall(channel_socket, packet, &count, (SOCKADDR*)&sender_addr) < 0) {
					printf("[channel]: error in send message to sender, %s\n", strerror(errno));
				}
				inet_ntop(AF_INET, &sender_addr.sin_addr, sender_ip, sizeof(sender_ip));
				fprintf(stderr, "\nsender: %s\nreceiver: %s\n%d bytes, flipped %d bits\n\n", sender_ip, ip_addr, nof_bytes, nof_bits_flipped);
				return;
			}
			else { // packets from sender
				sender_addr = clients_addr;
				nof_bytes += count;
				if (send_message(packet, count, error_prob, channel_socket, (SOCKADDR*)&receiver_sockaddr) < 0) {
					printf("[channel]: error in send message to receiver, %s\n", strerror(errno));
				}
			}
		}
	}
}

char init(SOCKET* channel_socket, fd_set* read_fds, struct sockaddr_in* channel_addr,  struct sockaddr_in* remote_addr_receiver) {

	int status = 0;
	create_socket(*channel_socket, "channel");

	remote_addr_receiver->sin_family = AF_INET;
	remote_addr_receiver->sin_addr.s_addr = inet_addr(ip_addr);
	remote_addr_receiver->sin_port = htons(receiver_port);

	channel_addr->sin_family = AF_INET;
	channel_addr->sin_addr.s_addr = INADDR_ANY;
	channel_addr->sin_port = htons(channel_port);

	//binding socket
	if (bind(*channel_socket, (SOCKADDR*)channel_addr, sizeof(*channel_addr)) < 0) {
		printf("[channel]: error in bind, %s\n", strerror(errno));
		status = 1;
	}

	FD_ZERO(read_fds);
	FD_SET(*channel_socket, read_fds);

	srand(seed);

	return status;
}

void ready2end(SOCKET* cahnnel_socket) {
	closesocket(*cahnnel_socket);
	return;
}