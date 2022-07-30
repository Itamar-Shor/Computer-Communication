#include "min_heap.h"
#include <stdio.h>
#include <stdlib.h>
#include "defines.h"


#define parent(i) (((i)-1) / 2)
#define left_child(i) (2*(i)+1)
#define right_child(i) (2*(i)+2)
#define MIN(a,b) ((a) < (b)  ? (a) : (b) )


void swap(heap_item* x, heap_item* y) {
	heap_item temp = *x;
	*x = *y;
	*y = temp;
}

void destroy_heap(heap* h) {
	free(h->bin_heap);
	free(h);
}

void min_heapify(heap* h, int idx) {
	// find left child node
	int left = left_child(idx);

	// find right child node
	int right = right_child(idx);

	// find the smallest among 3 nodes
	int smallest = idx;

	// check if the left node is larger than the current node
	if (left <= h->size && h->bin_heap[left].virt_fin_time < h->bin_heap[smallest].virt_fin_time) {
		smallest = left;
	}

	// check if the right node is larger than the current node
	if (right <= h->size && h->bin_heap[right].virt_fin_time < h->bin_heap[smallest].virt_fin_time) {
		smallest = right;
	}

	// swap the smallest node with the current node 
	// and repeat this process until the current node is smaller than 
	// the right and the left node
	if (smallest != idx) {
		swap((h->bin_heap) + idx, (h->bin_heap) + smallest);
		min_heapify(h, smallest);
	}

}

heap_item extract_min(heap* h) {
	heap_item min_item = *(h->bin_heap);

	// replace the first item with the last item
	*(h->bin_heap) = (h->bin_heap)[h->size - 1];
	h->size = h->size - 1;

	// maintain the heap property by heapifying the 
	// first item
	min_heapify(h, 0);
	return min_item;
}

double get_min_key(heap* h) {
	return h->bin_heap[0].virt_fin_time;
}

void* get_min_key_queue(heap* h) {
	return h->bin_heap[0].queue;
}

void insert_item(heap* h, void* queue, double virt_fin_time) {
	if (h->size == h->max_size) {
		return;
	}
	// first insert the time at the last position of the array 
	// and move it up
	(h->bin_heap)[h->size].queue = queue;
	(h->bin_heap)[h->size].virt_fin_time = virt_fin_time;
	h->size = h->size + 1;

	// move up until the heap property satisfies
	int i = h->size - 1;
	while (i != 0 && (h->bin_heap)[parent(i)].virt_fin_time > (h->bin_heap)[i].virt_fin_time) {
		swap((h->bin_heap) + parent(i), (h->bin_heap) + i);
		i = parent(i);
	}
	return;
}

heap* create_heap(int size) {
	heap* h = malloc(sizeof(heap));
	malloc_check("create_heap", "h", h);
	h->bin_heap = malloc(sizeof(heap_item) * size);
	malloc_check("create_heap", "h->bin_heap", h->bin_heap);
	h->size = 0;
	h->max_size = size;
	h->insert_item = insert_item;
	h->extract_min = extract_min;
	h->get_min_key = get_min_key;
	return h;
}