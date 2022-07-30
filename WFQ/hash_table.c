#define _CRT_SECURE_NO_WARNINGS
#include <stdlib.h>
#include <stdio.h>
#include "defines.h"
#include "queue.h"
#include <string.h>
#include "hash_table.h"

//https://troydhanson.github.io/uthash/userguide.html#_structure_keys

hash_table_entry* ht;

void create_hash_table() {
	ht = NULL;
}

void destroy_hash_table() {
	hash_table_entry *p, * tmp;
	HASH_ITER(hh, ht, p, tmp) {
		HASH_DEL(ht, p);
		destroy_queue(p->data);
		free(p);
	}
}

hash_table_entry* search_link(local_link* key) {
	hash_table_entry* p;
	hash_table_entry temp = { 0 };
	temp.key.dst_port = key->dst_port;
	temp.key.src_port = key->src_port;
	strncpy(temp.key.dst_ip, key->dst_ip, IP_ADDR_MAX_LEN);
	strncpy(temp.key.src_ip, key->src_ip, IP_ADDR_MAX_LEN);
	HASH_FIND(hh, ht, &temp.key, sizeof(local_link), p);
	return p;
}

void insert_link(local_link* k, queue* d) {
	hash_table_entry* new_entry = (hash_table_entry*)malloc(sizeof(hash_table_entry));
	if (!new_entry) {
		printf("error\n");
		return;
	}

	new_entry->data = d;
	strncpy(new_entry->key.dst_ip, k->dst_ip, IP_ADDR_MAX_LEN);
	strncpy(new_entry->key.src_ip, k->src_ip, IP_ADDR_MAX_LEN);
	new_entry->key.dst_port = k->dst_port;
	new_entry->key.src_port = k->src_port;
	HASH_ADD(hh, ht, key, sizeof(local_link), new_entry);
}

void remove_link(local_link* k) {
	hash_table_entry* temp = search_link(k);
	HASH_DEL(ht, search_link(k));
	destroy_queue(temp->data);
	free(temp);
}