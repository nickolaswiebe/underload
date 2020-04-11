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

// node creation helper function
struct node *node_new(func *op,struct node *head,struct node *tail) {
	struct node *node = new();
	node->op = op;
	node->head = head;
	node->tail = tail;
	node->refs = 1; // wherever this node gets stored, it'll have 1 ref
	return node;
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
// faster way of popping from one stack and pushing to another
void move(struct node **dst,struct node **src) {
	struct node *top = *src; // unlink top stack node
	*src = (*src)->tail;
	
	top->tail = *dst; // relink it onto dst stack
	*dst = top;
}

// garbage collection
void node_free(struct node *node) {
	struct node *st = NULL;
	
	push(&st,node);
	while(st != NULL) {
		node = pop(&st);
		if(node == NULL || --node->refs > 0) continue;
		push(&st,node->head);
		push(&st,node->tail);
		delete(node);
	}
}
struct node *node_dup(struct node *node) {
	node->refs++; // 1 more variable references it
	return node; // sharing woo
}

// virtual machine state and operations
struct node *rs, *st, *ip; // internal state of the vm
// rs and st are the return and data stacks
// ip is the next instruction to be executed
// all op_ functions represent vm operations
void op_ret() {
	ip = pop(&rs);
}
void op_push() {
	push(&st,node_dup(ip->head));
	node_free(ip);
	ip = pop(&rs);
}
void op_node() {
	if(ip->tail->op != &op_ret)
		push(&rs,node_dup(ip->tail));
	
	struct node *tofree = ip; // gotta free this node after we change the ip
	ip = node_dup(ip->head); // dup it so the free below won't kill the current node
	node_free(tofree);
}
void op_run() {
	ip = pop(&st);
}
void op_dup() {
	push(&st,node_dup(st->head));
	ip = pop(&rs);
}
void op_drop() {
	node_free(pop(&st));
	ip = pop(&rs);
}
void op_swap() {
	struct node *t = st->head;
	st->head = st->tail->head;
	st->tail->head = t;
	ip = pop(&rs);
}
void op_cat() {
	struct node *b = pop(&st);
	st->head = node_new(&op_node,st->head,b);
	ip = pop(&rs);
}
void op_quote() {
	st->head = node_new(&op_push,st->head,NULL);
	ip = pop(&rs);
}

// output routine
// PRINT_CHAR('c',func) means that func should be printed as 'c'
#define PRINT_CHAR(C,F) else if(node->op == &op_##F) putchar(C);
void print(struct node *node) {
	 // print stack represents nodes we still need to print, and starts with the input node
	// same principle of taking a node tree apart as the main vm
	struct node *pst = NULL;
	push(&pst,node);
	
	while(pst != NULL) {
		node = pop(&pst);
		
		if(node == NULL) { // NULL represents the end of a push op
			putchar(')');
		} else if(node->op == &op_node) {
			push(&pst,node->tail);
			push(&pst,node->head);
		} else if(node->op == &op_push) {
			putchar('(');
			push(&pst,NULL); // come back and print a ')' later
			push(&pst,node->head);
		}
		PRINT_CHAR(':',dup)
		PRINT_CHAR('!',drop)
		PRINT_CHAR('~',swap)
		PRINT_CHAR('*',cat)
		PRINT_CHAR('a',quote)
		PRINT_CHAR('^',run)
	}
}

// vm control/cpu
void done() { // technically should be an op_ function, but it's kinds special
	// reverse the stack by pushing it onto a temp stack
	struct node *rev = NULL;
	while(st != NULL) // st == NULL when st is empty
		move(&rev,&st);
	
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
	ip = prog;
	
	for(;;) // don't worry about stopping the execution loop, done() does that!
		ip->op();
}

// parsing

// adds inner to the current layer, handling the edge case
#define PARSE_UPDATE() \
	if(layer == NULL) \
		layer = inner; \
	else \
		layer = node_new(&op_node,layer,inner);

// means that C should be parsed to a node containing func F
#define PARSE_CHAR(C,F) \
	case C: \
		inner = node_new(&op_##F,NULL,NULL); \
		PARSE_UPDATE(); \
	break;

struct node *parse() {
	struct node *st = NULL; // stack of unclosed () quotes
	struct node *layer = NULL; // the current 'layer' of the program being parsed,
	// everything in here has fixed depth in the syntax tree
	struct node *inner; // the new section just parsed, to be added to the current layer
	char c;
	
	for(;;)
		switch(c = getchar()) {
			// handle basic operations
			PARSE_CHAR(':',dup)
			PARSE_CHAR('!',drop)
			PARSE_CHAR('~',swap)
			PARSE_CHAR('*',cat)
			PARSE_CHAR('a',quote)
			PARSE_CHAR('^',run)
			
			case '(':
				push(&st,layer); // save state of layer
				layer = NULL; // start new layer
			break;
			case ')':
				// add return operation
				inner = node_new(&op_ret,NULL,NULL);
				PARSE_UPDATE();
				
				if(st == NULL) // stack shouldn't be empty, we're not done
					exit(-1);
				
				inner = layer; // 'return' from inner expression
				layer = pop(&st); // restore layer
				inner = node_new(&op_push,inner,NULL);
				PARSE_UPDATE();
			break;
			case EOF:
				// add return operation
				inner = node_new(&op_ret,NULL,NULL);
				PARSE_UPDATE();
				
				if(st != NULL) // stack should be empty after parsing is done
					exit(-1);
				
				return layer;
			break;
		}
}

int main() {
	vm(parse());
	
	return 0; // never reached
}
