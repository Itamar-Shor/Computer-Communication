#pragma once
#include "defines.h"
#include "queue.h"
#include "uthash.h"

//used as the key
typedef struct _local_link {
	char src_ip[IP_ADDR_MAX_LEN];
	short src_port;
	char dst_ip[IP_ADDR_MAX_LEN];
	short dst_port;
}local_link;

typedef struct hash_table_entry {
	local_link key;
	queue* data;
	UT_hash_handle hh;
}hash_table_entry;

//initialize an empty hash table
void create_hash_table();

//destroy the hash table (free evertything).
void destroy_hash_table();

//if exists, return the entry with the given key.
//else retrun NULL.
hash_table_entry* search_link(local_link* key);

//insert a new entry to the table.
//must be called only after search_link return NULL!
void insert_link(local_link* k, queue* d);

//deletes the entry with the given key from the table.
//must be called after search_link succeed!
void remove_link(local_link* k);