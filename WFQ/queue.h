#pragma once


typedef struct node {
	struct node* next;
	char* packet;
	double my_last_time;
	double pred_last_time;
}node;

typedef struct queue {
	node* front;
	node* back;
	int size;
	double weight;

	//insert an element to the back of the queue.
	void (*push_back)(struct queue* q, char* packet);

	//remove and return the first element in the queue.
	//must be called with non-empty queue!
	char* (*pop_front)(struct queue* q);

}queue;

//return an empty queue.
queue* create_queue();

//destroy the given queue (free everything).
void destroy_queue(queue* q);