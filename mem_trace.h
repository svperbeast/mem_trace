#include "rbtree.h"

#define MAX_STACK	64
struct trace_record {
	struct rb_node node;
	unsigned long addr;
	unsigned long bt[MAX_STACK];
	unsigned long size;
};
