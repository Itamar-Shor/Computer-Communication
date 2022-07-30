#pragma once

//https://algorithmtutor.com/Data-Structures/Tree/Binary-Heaps/

typedef struct heap_item {
	double virt_fin_time;
	void* queue;
}heap_item;

typedef struct heap {
	int size;
	int max_size;
	heap_item* bin_heap;

	//remove and retrun the min key element in the heap.
	//must be called with non-empty heap!
	heap_item (*extract_min)(struct heap* h);

	//insert a new item to the heap.
	void (*insert_item)(struct heap* h, void* queue, double virt_fin_time);

	//return the queue(=stream) of the min key element.
	//must be called with non-empty heap!
	void* (*get_min_key_queue)(struct heap* h);

	//retrun the min key element in the heap.
	//must be called with non-empty heap!
	double (*get_min_key)(struct heap* h);

}heap;

//return an empty heap.
heap* create_heap(int size);

//destroy the given heap (free everything).
void destroy_heap(heap* h);