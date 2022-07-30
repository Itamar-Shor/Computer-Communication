#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include "utils.h"
#include "defines.h"
#include "GPS_simulator.h"



int main(int argc, char* argv[])
{
	GPS_SIMULATOR* GPS_sim = init_GPS_simulator(MAX_LINKS);
	if (!GPS_sim) {
		printf("error\n");
		exit(-1);
	}

	char line[MAX_ROW_LEN];
	int next_departure_time;
	char* arrival_time;

	next_departure_time = 0;
	while (fgets(line, MAX_ROW_LEN, stdin) != NULL) {
		arrival_time = extract_field(line, TIME);
		if (atoi(arrival_time) <= next_departure_time) {
			GPS_sim->receive_packet(line);
		}
		else {
			//we can send a new packet from the ones that arrived untill now...
			while (GPS_sim->get_nof_packets() && next_departure_time < atoi(arrival_time)) {
				next_departure_time = GPS_sim->send_packet(next_departure_time);
			}
			GPS_sim->receive_packet(line);
		}
		free(arrival_time);
	}
	//here we don't expect any new packets
	while (GPS_sim->get_nof_packets()) {
		next_departure_time = GPS_sim->send_packet(next_departure_time);
	}
	destroy_GPS_simulator(GPS_sim);
	return 0;
}