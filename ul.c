#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#define dup dup_lol
typedef void func();
struct node {
	func *op;
	unsigned int refs;
	struct node *head, *tail;
};
// fast brk allocator
#define BRK_SIZE (4096)
struct node *brk_alloc() {
	static int pos = 0; // 1 AFTER position to be returned
	static struct node *buf;
	if(pos == 0) {
		buf = sbrk(BRK_SIZE * sizeof(struct node));
		pos = BRK_SIZE;
	}
	return &buf[--pos];
}
// memory manager
struct node *free_list = NULL;
struct node *new() {
	if(free_list == NULL)
		return brk_alloc();
	struct node *ret = free_list;
	free_list = free_list->head;
	return ret;
}
void delete(struct node *node) {
	node->head = free_list;
	free_list = node;
}
// garbage collector
struct node *node_new(func *op,struct node *head,struct node *tail) {
	struct node *node = new();
	node->op = op;
	node->head = head;
	node->tail = tail;
	node->refs = 1;
	return node;
}
void node_free(struct node *node) {
	if(node == NULL || --node->refs > 0)
		return;
	node_free(node->head);
	node_free(node->tail);
	delete(node);
}
struct node *node_dup(struct node *node) {
	node->refs++;
	return node;
}
// generic stack operations
void push(struct node **st,struct node *item) {
	*st = node_new(NULL,item,*st);
}
struct node *pop(struct node **st) {
	struct node *ret = (*st)->head;
	struct node *old = *st;
	*st = (*st)->tail;
	delete(old);
	return ret;
}
// virtual machine
struct node *rs, *st, *ip;
void op_ret() {
}
void op_push() {
	push(&st,node_dup(ip->head));
	node_free(ip);
}
void op_node() {
	push(&rs,node_dup(ip->tail));
	push(&rs,node_dup(ip->head));
	node_free(ip);
}
void op_run() {
	push(&rs,pop(&st));
}
void op_dup() {
	struct node *t = pop(&st);
	push(&st,t);
	push(&st,node_dup(t));
}
void op_drop() {
	node_free(pop(&st));
}
void op_swap() {
	struct node *b = pop(&st);
	struct node *a = pop(&st);
	push(&st,b);
	push(&st,a);
}
void op_cat() {
	struct node *b = pop(&st);
	struct node *a = pop(&st);
	push(&st,node_new(&op_node,a,b));
}
void op_quote() {
	push(&st,node_new(&op_push,pop(&st),NULL));
}
#define PRINT_CHAR(C,F) else if(node->op == &op_##F) putchar(C);
void print(struct node *node) {
	if(node->op == &op_node) {
		print(node->head);
		print(node->tail);
	} else if(node->op == &op_push) {
		putchar('(');
		print(node->head);
		putchar(')');
	}
	PRINT_CHAR(':',dup)
	PRINT_CHAR('!',drop)
	PRINT_CHAR('~',swap)
	PRINT_CHAR('*',cat)
	PRINT_CHAR('a',quote)
	PRINT_CHAR('^',run)
}

void done() {
	while(st != NULL)
		push(&rs,pop(&st));
	while(rs != NULL) {
		putchar('(');
		print(pop(&rs));
		putchar(')');
	}
	exit(0);
}
void vm(struct node *prog) {
	rs = st = NULL;
	push(&rs,node_new(&done,NULL,NULL));
	push(&rs,prog);
	for(;;) {
		ip = pop(&rs);
		ip->op();
	}
}
#define PARSE_CHAR(C,F) case C: return node_new(&op_node,node_new(&op_##F,NULL,NULL),parse());
struct node *parse() {
	switch(getchar()) {
		case '(': ;
			struct node *inner = parse();
			return node_new(&op_node,node_new(&op_push,inner,NULL),parse());
		case ')':
		case EOF:
			return node_new(&op_ret,NULL,NULL);
		PARSE_CHAR(':',dup)
		PARSE_CHAR('!',drop)
		PARSE_CHAR('~',swap)
		PARSE_CHAR('*',cat)
		PARSE_CHAR('a',quote)
		PARSE_CHAR('^',run)
		default:
			return parse();
	}
}
int main() {
	vm(parse());
	return 0;
}
