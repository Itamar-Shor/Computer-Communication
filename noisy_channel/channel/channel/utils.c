#include "utils.h"

void printBits(int const size, void const* const ptr)
{
	unsigned char* b = (unsigned char*)ptr;
	unsigned char byte;
	int i, j;

	for (i = size - 1; i >= 0; i--) {
		for (j = 7; j >= 0; j--) {
			byte = (b[i] >> j) & 1;
			printf("%u", byte);
		}
	}
	printf("\n");
}


void set_mask(char* mask, int nof_1s) {
	if (nof_1s == 0) {
		mask[0] = 0;
		return;
	}
	mask[0] = 1;
	nof_1s--;
	while (nof_1s) {
		mask[0] = (mask[0] << 1) | 1;
		nof_1s--;
	}
}


/*copies 'nof_bits' from 'src' (from 'src_idx', 'src_offset') to 'trg' (from 'trg_idx', 'trg_offset')*/
void place_bits(char* trg, int trg_idx, int trg_offset, char* src, int src_idx, int src_offset, int nof_bits) {
	if (nof_bits <= 0) return;

	char curr_bits_src, temp_mask;
	int curr_nof_bits_src = min(min(8 - src_offset, nof_bits), 8 - trg_offset);
	set_mask(&temp_mask, curr_nof_bits_src);
	curr_bits_src = (src[src_idx] >> src_offset) & temp_mask;

	trg[trg_idx] |= (curr_bits_src << trg_offset);
	//update fields
	if (curr_nof_bits_src == nof_bits) return;

	nof_bits -= curr_nof_bits_src;

	if (curr_nof_bits_src == (8 - src_offset)) {
		if ((trg_offset + curr_nof_bits_src) < 8) {
			trg_offset += curr_nof_bits_src;
		}
		else {
			trg_offset = 0;
			trg_idx++;
		}
		src_offset = 0;
		src_idx++;
	}
	else { // curr_nof_bits_src == (8 - trg_offset)
		if ((src_offset + curr_nof_bits_src) < 8) {
			src_offset += (8 - trg_offset);
		}
		else {
			src_offset = 0;
			src_idx++;
		}
		trg_offset = 0;
		trg_idx++;
	}
	place_bits(trg, trg_idx, trg_offset, src, src_idx, src_offset, nof_bits);
}

//taken from rec_2
int sendall(SOCKET s, char* buf, int* len, struct sockaddr *addr) {

	int total = 0; /* how many bytes we've sent */
	int bytesleft = *len; /* how many we have left to send */
	int n = -2;

	while (total < *len) {
		n = sendto(s, buf+ total, bytesleft, 0, addr, sizeof(*addr));
		if (n == -1) { break; }
		total += n;
		bytesleft -= n;
	}
	*len = total; /* return number actually sent here */
	return n == -1 ? -1 : 0; /*-1 on failure, 0 on success */
}

void int2char(int n, char* c) {
	int mask = 0xFF;
	for (int i = 0; i < 4; i++) {
		c[i] = (char)((n & mask) >> (i*8));
		mask = mask << 8;
	}
}

void char2int(int* n, unsigned char* c) {
	for (int i = 0; i < 4; i++) {
		*n |= ((int)(c[i]) << (8*i));
	}
}