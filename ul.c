#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

typedef void func();
struct node {
	func *op;
	unsigned int refs;
	struct node *head, *tail;
};

// fast brk allocator
#define BRK_SIZE (4096)
struct node *brk_alloc() {
	static int pos = 0; // 1 AFTER position to be returned, runs down to zero
	static struct node *buf;
	if(pos == 0) { // no more nodes in buffer
		buf = sbrk(BRK_SIZE * sizeof(struct node));
		pos = BRK_SIZE;
	}
	return &buf[--pos];
}

// memory manager
// instead of freeing an unused node's memory, recycle it in a linked list
struct node *free_list = NULL;
struct node *new() {
	if(free_list == NULL) // only allocate when no old nodes are available
		return brk_alloc();
	
	// take new node from free list
	struct node *ret = free_list;
	free_list = free_list->head;
	return ret;
}
void delete(struct node *node) {
	// put it in the free list
	node->head = free_list;
	free_list = node;
}

// garbage collector
struct node *node_new(func *op,struct node *head,struct node *tail) {
	struct node *node = new();
	node->op = op;
	node->head = head;
	node->tail = tail;
	node->refs = 1; // wherever this node gets stored, it'll have 1 ref
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
	node->refs++; // 1 more variable references it
	return node; // sharing woo
}

// generic stack operations
// takes double pointer to stack, so it can modify it "in place"
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

// virtual machine state and operations
struct node *rs, *st, *ip; // internal state of the vm
// rs and st are the return and data stacks
// ip is the next instruction to be executed
// all op_ functions represent vm operations
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
	struct node *t = pop(&st); // these two lines just get the top of the stack
	push(&st,t);
	push(&st,node_dup(t)); // this one actually does the work
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
	struct node *b = pop(&st); // have to write these as variables to get a sequence point
	struct node *a = pop(&st);
	push(&st,node_new(&op_node,a,b));
}
void op_quote() {
	push(&st,node_new(&op_push,pop(&st),NULL));
}

// output routine
// PRINT_CHAR('c',func) means that func should be printed as 'c'
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

// vm control/cpu
void done() { // technically should be an op_ function, but it's kinds special
	// reverse the stack by pushing it onto a temp stack
	struct node *rev = NULL;
	while(st != NULL) // st == NULL when st is empty
		push(&rev,pop(&st));
	
	// print each stack item with it's implicit brackets
	while(rev != NULL) {
		putchar('(');
		print(pop(&rev));
		putchar(')');
	}
	
	// if we don't manually exit the program here, it'll try to pop from the
	// empty return stack and segfault
	exit(0);
}
void vm(struct node *prog) { // the vm entry point
	rs = st = NULL; // ensure stacks are empty
	
	// special done() func on rs represents end of program
	push(&rs,node_new(&done,NULL,NULL));
	push(&rs,prog);
	
	for(;;) { // don't worry about stopping the execution loop, done() does that!
		ip = pop(&rs);
		ip->op();
	}
}

// parsing
// PARSE_CHAR('c',func) means that 'c' should be parsed to a node containing func
#define PARSE_CHAR(C,F) \
	case C: \
		return node_new(&op_node,node_new(&op_##F,NULL,NULL),parse());
struct node *parse() {
	switch(getchar()) {
		case '(': ; // use a blank ; to shut up the compiler
			struct node *inner = parse(); // ensure order of evaluation
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
	
	return 0; // never reached
}
