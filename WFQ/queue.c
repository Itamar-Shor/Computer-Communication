#define _CRT_SECURE_NO_WARNINGS
#include "queue.h"
#include <stdio.h>
#include "defines.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>

// A utility function to create a new linked list node.
node* new_node(char* p)
{
	node* temp = (node*)malloc(sizeof(node));
	malloc_check("new_node", "temp", temp);
	size_t len = strlen(p);
	temp->packet = (char*)malloc(sizeof(char) * (len +1));
	malloc_check("new_node", "temp->packet", temp->packet);
	strcpy(temp->packet, p);
	if (temp->packet[len - 1] == '\n') temp->packet[len - 1] = '\0';
	temp->next = NULL;
	temp->my_last_time = -1;
	temp->pred_last_time = -1;
	return temp;
}


// The function to add a key k to q
void push_back(queue* q, char* packet)
{
	// Create a new LL node
	node* temp = new_node(packet);
	// If queue is empty, then new node is front and back both
	if (q->size == 0) {
		char* weight = extract_field(packet, WEIGHT);
		q->size = 1;
		q->front = q->back = temp;
		q->weight = (weight != NULL) ? strtod(weight, NULL) : q->weight;
		free(weight);
		return;
	}

	// Add the new node at the end of queue and change back
	temp->pred_last_time = q->back->my_last_time;
	q->back->next = temp;
	q->back = temp;
	q->size = q->size + 1;
}

// Function to remove a key from given queue q
char* pop_front(queue* q)
{
	// If queue is empty, return NULL.
	if (q->front == NULL)
		return NULL;

	// Store previous front and move front one node ahead
	node* temp = q->front;
	char* deleted_packet;
	q->front = q->front->next;
	// If front becomes NULL, then change back also as NULL
	if (q->front == NULL)
		q->back = NULL;
	else {
		char* weight = extract_field(q->front->packet, WEIGHT);
		q->weight = (weight != NULL) ? strtod(weight, NULL) : q->weight;
		free(weight);
	}

	q->size = q->size - 1;
	deleted_packet = temp->packet;
	free(temp);
	return deleted_packet;
}

void destroy_queue(queue* q) {
	for (int i = 0; i < q->size; i++) {
		free(q->pop_front(q));
	}
	free(q);
}

// A utility function to create an empty queue
queue* create_queue()
{
	queue* q = (queue*)malloc(sizeof(queue));
	malloc_check("create_queue", "q", q);
	q->front = q->back = NULL;
	q->size = 0;
	q->weight = 1;
	q->pop_front = pop_front;
	q->push_back = push_back;
	return q;
}