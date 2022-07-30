#define _CRT_SECURE_NO_WARNINGS
#include "GPS_simulator.h"
#include "hash_table.h"
#include "min_heap.h"
#include "queue.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>

#define MAX(a,b) ((a) > (b)  ? (a) : (b) )

//state of the system so far
static double weights = 0;
static double next_vir_departure_time = 0;

static double last_real_time = 0;
static double round_t = 0;

static int nof_packets = 0;


/*all the packets that didn't finish sending (regards with virtual time)*/
heap* virt_departure_heap;

/*all the packets that didn't sent yet (regards with real time) */
heap* real_departure_heap;

double calc_round_time(int real_time) {
	if (virt_departure_heap->size == 0) return round_t; //if there is no packets waiting
	return round_t + (double)((real_time - last_real_time) / weights);
}

/*used when next event is departure, so the weights need to be updated*/
void update_round_time() {
	double x = (next_vir_departure_time - round_t)*weights;
	round_t = next_vir_departure_time;
	last_real_time += x;

	if (virt_departure_heap->size > 0) {
			heap_item item = virt_departure_heap->extract_min(virt_departure_heap);
			queue* q = (queue*)item.queue;	
			double old_weight = q->weight;
			free(q->pop_front(q));

			weights -= old_weight;
			if (q->size > 0) {
				weights += q->weight;
			}
			//update next_vir_departure_time
			if (virt_departure_heap->size > 0) {
				next_vir_departure_time = virt_departure_heap->get_min_key(virt_departure_heap);
			}
	}
}

//calculate virtual departure time according to the curr packet being proccesed and the current state of the system.
double calc_last_time(int real_time, double round_time, double last_time_in_q, int size, double weight) {
	double last_time = MAX(round_time, last_time_in_q) + (double)(size / weight);
	round_t = round_time;
	last_real_time = real_time;
	return last_time;
}

//proccess a new packet
void receive_packet(char* packet) {
	char* packet_args[MAX_ARGS];
	int nof_splits = split(packet, ' ', MAX_ARGS, packet_args);
	int arrival_time = atoi(packet_args[TIME]);

	double curr_round_time;
	local_link* link = (local_link*)malloc(sizeof(local_link));
	malloc_check("receive_packet", "link", link);
	link->dst_port = atoi(packet_args[DST_PORT]);
	link->src_port = atoi(packet_args[SRC_PORT]);
	strncpy(link->dst_ip, packet_args[DST_IP], IP_ADDR_MAX_LEN);
	strncpy(link->src_ip, packet_args[SRC_IP], IP_ADDR_MAX_LEN);

	hash_table_entry* stream;
	//if this is the first packet from that link
	if ((stream = search_link(link)) == NULL) {
		queue* q = create_queue();
		insert_link(link, q);
		stream = search_link(link);
	}
	char queue_was_empty = (stream->data->size == 0);
	stream->data->push_back(stream->data, packet);
	curr_round_time = calc_round_time(arrival_time);
	//next event = departure
	while ((next_vir_departure_time <= curr_round_time) && virt_departure_heap->size) {
		update_round_time();
		//recalc the round_t
		curr_round_time = calc_round_time(arrival_time);
	}
	stream->data->back->my_last_time = calc_last_time(	arrival_time, 
														curr_round_time, 
														stream->data->back->pred_last_time, 
														atoi(packet_args[LEN]), 
														stream->data->weight
													  );
	
	if (queue_was_empty) {
		weights += (stream->data->weight);
	}
	virt_departure_heap->insert_item(virt_departure_heap, stream->data, stream->data->back->my_last_time);
	char* packet2add = (char*)malloc(sizeof(char) * (strlen(stream->data->back->packet) + 1));
	malloc_check("receive_packet", "packet2add", packet2add);
	strcpy(packet2add, stream->data->back->packet);
	real_departure_heap->insert_item(real_departure_heap, packet2add, stream->data->back->my_last_time);
	//update next_vir_departure_time if needed
	next_vir_departure_time = virt_departure_heap->get_min_key(virt_departure_heap);

	free(link);
	for (int i = 0; i < nof_splits + 1; i++) {
		free(packet_args[i]);
	}
	nof_packets++;
}


int send_packet(int curr_time) {
	heap_item hi = real_departure_heap->extract_min(real_departure_heap);
	nof_packets--;
	char* packet = (char*)hi.queue;
	char* splitted[MAX_ARGS];
	int nof_splits = split(packet, ' ', MAX_ARGS, splitted);
	int len = atoi(splitted[LEN]);
	int departure_time = MAX(atoi(splitted[TIME]), curr_time);
	fprintf(stdout, "%d: %s\n", departure_time, packet);
	free(packet);
	for (int i = 0; i < nof_splits + 1; i++) {
		free(splitted[i]);
	}
	return departure_time + len;
}


int get_nof_packets() {
	return nof_packets;
}

GPS_SIMULATOR* init_GPS_simulator(int nof_links) {
	create_hash_table();
	virt_departure_heap = create_heap(nof_links);
	real_departure_heap = create_heap(nof_links);
	GPS_SIMULATOR* GPS_sim = (GPS_SIMULATOR*)malloc(sizeof(GPS_SIMULATOR));
	if (!GPS_sim) {
		return NULL;
	}
	GPS_sim->get_nof_packets = get_nof_packets;
	GPS_sim->receive_packet = receive_packet;
	GPS_sim->send_packet = send_packet;
	return GPS_sim;
}

void destroy_GPS_simulator(GPS_SIMULATOR* GPS_sim) {
	destroy_hash_table();
	destroy_heap(virt_departure_heap);
	destroy_heap(real_departure_heap);
	free(GPS_sim);
}