#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include "common.h"

typedef struct watchpoint {
	int NO;
	struct watchpoint *next;

	/* TODO: Add more members if necessary */
	char name[128];
	uint32_t old_value;
	uint32_t new_value;

} WP;

WP* new_wp();
void free_wp(WP* pre);
void clear_wp(WP *p);
bool check_w();
void show_wp();
bool find_to_del(int n);
#endif
