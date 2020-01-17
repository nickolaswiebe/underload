#include <stdlib.h>
struct node {
	void(*func)(void);
	unsigned int refs;
	struct node *head, *tail;
};
struct node *nalloc() {
	return malloc(sizeof(struct node));
}
void nfree(struct node *node) {
	free(node);
}
struct node *nnode(struct node *head,struct node *tail) {
	struct node *ret = nalloc();
	ret->refs = 1;
	ret->head = head;
	ret->tail = tail;
	return ret;
}
struct node *ndup(struct node *node) {
	if(node->head != NULL)
		node->refs++;
	return node;
}
void ndrop(struct node *node) {
	if(node == NULL) return;
	if(node->head == NULL) return;
	if(--node->refs > 0) return;
	ndrop(node->head);
	ndrop(node->tail);
	nfree(node);
}
#define nhead(N) ((N)->head)
#define ntail(N) ((N)->tail)
#define nfunc(N) ((N)->func)
void push(struct node **st,struct node *val) {
	*st = nnode(val,*st);
}
struct node *pop(struct node **st) {
	struct node *ret = nhead(*st);
	struct node *new = ntail(*st);
	nfree(*st);
	*st = new;
	return ret;
}
struct node *a,*b,*ip,*st,*rs;
void dup() { a = pop(&st); push(&st,a); push(&st,ndup(a)); }
void drop() { ndrop(pop(&st)); }
void swap() { a = pop(&st); b = pop(&st); push(&st,a); push(&st,b); }
void cat() { b = pop(&st); a = pop(&st); push(&st,nnode(a,b)); }
void quote() { a = pop(&st); push(&st,nnode(a,NULL)); }
void run() { ip = pop(&st); }
void ret() { ip = pop(&rs); }
void done() { }
void vm() {
	//push(&st,odone);
	for(;;)
		if(ntail(ip) == NULL) {
			if(nhead(ip) == NULL) {
				(*nfunc(ip))();
			} else {
				push(&st,nhead(ip));
				ndrop(ip);
			}
		} else {
			push(&rs,ntail(ip));
			a = ip;
			ip = nhead(ip);
			ndrop(a);
		}
}
int main(){}