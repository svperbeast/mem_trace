#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>

#include "mem_trace.h"

static int trace_done = 0;
static char exec_path[PATH_MAX];

/* record tree */
struct rb_root rt = RB_ROOT;
static void mtr_store_alloc_info(unsigned long *bt, int n, unsigned long addr, 
				 size_t size);
static void mtr_remove(struct rb_root *root, unsigned long addr);
static void mtr_report(void);

/* original malloc, free function */
static void *(*malloc_org)(size_t size) = NULL;
static void (*free_org)(void *ptr) = NULL;

/* library initialization function */
void mem_trace_init(void) __attribute__((constructor));
void mem_trace_init(void)
{
	unsetenv("LD_PRELOAD");

	if (get_exec_path(exec_path, sizeof(exec_path)) < 0) {
		fprintf(stderr, "can not get executable path.\n");
		exit(1);
	}
	fprintf(stderr, "\n> executable: %s\n", exec_path);
	
	malloc_org = dlsym(RTLD_NEXT, "malloc");
	fprintf(stderr, "> malloc: using trace hooks..");
	if (malloc_org == NULL) {
		char *error = dlerror();
		if (error == NULL) {
			error = "malloc is NULL";
		}
		fprintf(stderr, "%s\n", error);
		exit(EXIT_FAILURE);
	}
	fprintf(stderr, "ok\n");

	free_org = dlsym(RTLD_NEXT, "free");
	fprintf(stderr, "> free: using trace hooks..");
	if (free_org == NULL) {
		char *error = dlerror();
		if (error == NULL) {
			error = "free is NULL";
		}
		fprintf(stderr, "%s\n", error);
		exit(EXIT_FAILURE);
	}
	fprintf(stderr, "ok\n");

	if (atexit(mtr_report) < 0) {
		fprintf(stderr, "atexit() failed.\n");
		exit(EXIT_FAILURE);
	}
	fprintf(stderr, "Reporting when program exit..\n");
	return;
}

#ifdef X86
void **get_ebp(int dummy)
{
	void **ebp = (void **)&dummy - 2;
	return ebp;
}
#endif

/* wrapper functions */
void *malloc(size_t size)
{
	int dummy; /* dummy must be here */
	int i = 0;
	void *p;
	unsigned long caller_addr = 0;
	void **ebp, **ret;
	unsigned long bt[MAX_STACK];
	
	if (trace_done) {
		p = malloc_org(size);
		return p;
	}

#ifdef X86
	ebp = get_ebp(dummy);
	ret = NULL;
	while (*ebp) {
		ret = ebp + 1;
		bt[i++] = (unsigned long)*ret;
		ebp = (void **)(*ebp);
	}
#elif SPARC
	asm volatile ("movl 8(%%fp), %0"
		      : "=r" (caller_addr)
		      :
		      );
	bt[i++] = caller_addr;
#elif PARISC
	asm volatile ("copy %%r2, %0"
		      : "=r" (caller_addr)
		      :
		      );
	bt[i++] = caller_addr;
#endif

	p = malloc_org(size);

	mtr_store_alloc_info(bt, i, (unsigned long)p, size);
	return p;
}

void free(void *ptr)
{
	free_org(ptr);
	
	if (!trace_done)
		mtr_remove(&rt, (unsigned long)ptr);
	return;
}

/* 
 * deals with trace record 
 * prefix: mtr (memory trace record)
 */
static struct trace_record *mtr_search(struct rb_root *root, unsigned long addr)
{
	struct rb_node *node = root->rb_node;

	while (node) {
		struct trace_record *rec = 
				container_of(node, struct trace_record, node);
		
		if (addr < rec->addr)
			node = node->rb_left;
		else if (addr > rec->addr)
			node = node->rb_right;
		else
			return rec;
	}
	return NULL;
}

static int mtr_insert(struct rb_root *root, struct trace_record *rec)
{
	struct rb_node **new = &(root->rb_node), *parent = NULL;

	/* figure out where to put new node */
	while (*new) {
		struct trace_record *this =
				container_of(*new, struct trace_record, node);

		parent = *new;
		if (rec->addr < this->addr)
			new = &((*new)->rb_left);
		else if (rec->addr > this->addr)
			new = &((*new)->rb_right);
		else
			return -1;
	}

	/* add new node and rebalance tree */
	rb_link_node(&(rec->node), parent, new);
	rb_insert_color(&(rec->node), root);
	return 0;
}

static void mtr_remove(struct rb_root *root, unsigned long addr)
{
	struct trace_record *rec;

	rec = mtr_search(root, addr);
	if (rec) {
		rb_erase(&(rec->node), root);
		free_org(rec);
	}
	return;
}

static void mtr_report(void)
{
	int i, j;
	FILE *fp;
	struct rb_node *node;
	struct trace_record *rec;
	Dl_info dlip;
	char exec_name[BUFSIZ];
	char *func, *coord;

	trace_done = 1;

	check_addr2line();

	fp = fopen("mem_trace.out", "w");
	if (fp == NULL) {
		fprintf(stderr, "can not open mem_trace.out\n");
		return;
	}

	i = 0;
	for (node = rb_first(&rt); node; node = rb_next(node)) {
		j = 0;
		rec = container_of(node, struct trace_record, node);
		dladdr((void *)rec->bt[j], &dlip);
		if (dlip.dli_sname == NULL) {
			get_coordinate(rec->bt[j], dlip.dli_fname,
				       &func, &coord);
			fprintf(fp, "[%p]\t%s (%s) %ld byte(s) lost.\n",
					(void *)rec->bt[j],
					func,
					coord,
					rec->size);
			free(func);
			free(coord);
		} else { 
			fprintf(fp, "[%p]\t%s %ld byte(s) lost.\n",
					(void *)rec->bt[j], 
					dlip.dli_sname,
					rec->size);
		}
		j++;
		while (rec->bt[j] != 0) {
			dladdr((void *)rec->bt[j], &dlip);
			if (dlip.dli_sname == NULL) {
				get_coordinate(rec->bt[j], dlip.dli_fname,
				       	       &func, &coord);
				fprintf(fp, "[%p]\t%s (%s)\n",
						(void *)rec->bt[j],
						func,
						coord);
				free(func);
				free(coord);
			} else {
				fprintf(fp, "[%p]\t%s\n", 
						(void *)rec->bt[j],
						dlip.dli_sname);
			}
			j++;
		}
		if (j > 1)
			fprintf(fp, "\n");
	}

	fclose(fp);
	return;
}

static void mtr_store_alloc_info(unsigned long *bt, int n, unsigned long addr, 
				 size_t size)
{
	int i;
	struct trace_record *rec;

	rec = malloc_org(sizeof(struct trace_record));
	if (rec == NULL) {
		fprintf(stderr, "malloc() failed. %s\n", strerror(errno));
		return;
	}
	memset(&(rec->bt[0]), 0, sizeof(unsigned long) * MAX_STACK);

	for (i = 0; i < n; i++)
		rec->bt[i] = bt[i];
	rec->addr = addr;
	rec->size = size;

	if (mtr_insert(&rt, rec) < 0) {
		fprintf(stderr, "mtr_insert() failed.\n");
	}
	return;
}
