#include "monitor/watchpoint.h"
#include "monitor/expr.h"

#define NR_WP 32

static WP wp_pool[NR_WP];
static WP *head, *free_;

void init_wp_pool() {
	int i;
	for(i = 0; i < NR_WP; i ++) {
		wp_pool[i].NO = i;
		wp_pool[i].next = &wp_pool[i + 1];
	}
	wp_pool[NR_WP - 1].next = NULL;

	head = NULL;
	free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */

WP* new_wp() {
	if(free_==NULL)
		assert(0);
	else {
		WP *p=NULL;

		p=free_->next;
		free_->next=head;
		head=free_;
		free_=p;
		p=NULL;
		return head;
	}
}

void free_wp(WP *p) {
	if(head==NULL)
		assert(0);
	else if(p==head) {
		head=head->next;
		p->next=free_;
		free_=p;
	}
	else {
		WP *pre=head;
		for(;pre!=NULL;pre=pre->next)
			if(pre->next==p)
				break;
		pre->next=p->next;
		p->next=free_;
		free_=p;
	}
}

bool find_to_del(int n) {
	WP *cur=head;
	bool rm=false;
	for(;cur!=NULL;cur=cur->next) {
		if(cur->NO==n) {
			free_wp(cur);
			rm=true;
			break;
		}
	}
	if(rm==true)
		return true;
	else return false;
}

void clear_wp(WP *p) {
	p->name[0]='\0';
	p->old_value=0;
	p->new_value=0;
}

bool check_w() {
	WP *p=head;
	bool success=true;
	bool flag=false;
	for(;p!=NULL;p=p->next) {
		p->old_value=p->new_value;
		p->new_value=expr(p->name,&success);
		if(p->old_value!=p->new_value) {
			printf("watchpoint%d:%s\nold value:%x\nnew value:%x\n",p->NO,p->name,p->old_value,p->new_value);
			flag=true;
		}
	}
	if(flag==true)
		return true;
	else return false;
}

void show_wp() {
	WP *q=head;
	if(q==NULL)
		printf("no watchpoint\n");
	else {
		for(;q!=NULL;q=q->next) {
			printf("watchpoint:%d\t%s\n",q->NO,q->name);
		}
	}
}
