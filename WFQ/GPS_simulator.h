#pragma once

/*pick a packet to send*/

typedef struct GPS_SIMULATOR {
	int (*send_packet)(int curr_time);

	void (*receive_packet)(char* packet);

	int (*get_nof_packets)();
}GPS_SIMULATOR;


GPS_SIMULATOR* init_GPS_simulator(int nof_links);

void destroy_GPS_simulator(GPS_SIMULATOR* GPS_sim);