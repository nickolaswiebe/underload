#include <stdio.h>
#include <unistd.h>
typedef union node Node;
union node {
	type
	void(*func)();
	
};
struct node {
	unsigned int refs;
	struct node *head, *tail;
};
struct node *unused = NULL;
struct node *alloc() {
	if(unused == NULL)
		return sbrk(sizeof(struct node));
	struct node *ret = unused;
	unused = unused->tail;
	return ret;
}
void dalloc(struct node *node) {
	node->tail = unused;
	unused = node;
}
struct node *mknode(struct node *head,struct node *tail) {
	struct node *new = alloc();
	new->refs = 1;
	new->head = head;
	new->tail = tail;
	return new;
}
struct node *ndup(struct node *node) {
	node->refs++;
	return node;
}
void drop(struct node *node) {
	if(node == NULL)
		return;
	node->refs--;
	if(node->refs > 0)
		return;
	drop(node->head);
	drop(node->tail);
}
#define SYM(NAME) struct node s_##NAME; struct node *NAME = &s_##NAME;
SYM(DUP) SYM(DROP) SYM(SWAP) SYM(CAT) SYM(QUOTE) SYM(RUN) SYM(RET) SYM(OUT) SYM(NL)
void push(struct node **st,struct node *val) {
	*st = mknode(val,*st);
}
struct node *pop(struct node **st) {
	struct node *ret = (*st)->head;
	struct node *new = (*st)->tail;
	dalloc(*st);
	*st = new;
	return ret;
}
struct node *st = NULL, *rs = NULL;
void print(struct node *node) {
	if(node == DUP) {
		putchar(':');
	} else if(node == DROP) {
		putchar('!');
	} else if(node == SWAP) {
		putchar('~');
	} else if(node == CAT) {
		putchar('*');
	} else if(node == QUOTE) {
		putchar('a');
	} else if(node == RUN) {
		putchar('^');
	} else if(node == NL) {
		putchar('n');
	} else if(node == OUT) {
		putchar('S');
	} else if(node != NULL && node != RET) {
		if(node->tail == NULL) {
			putchar('(');
			print(node->head);
			putchar(')');
		} else {
			print(node->head);
			print(node->tail);
		}
	}
}
struct node *ip, *a, *b;
void go() {
	for(;;) {
		if(ip == DUP) {
			a = pop(&st);
			push(&st,a);
			push(&st,ndup(a));
			ip = RET;
		} else if(ip == DROP) {
			drop(pop(&st));
			ip = RET;
		} else if(ip == SWAP) {
			a = pop(&st);
			b = pop(&st);
			push(&st,a);
			push(&st,b);
			ip = RET;
		} else if(ip == CAT) {
			b = pop(&st);
			a = pop(&st);
			push(&st,mknode(a,b));
			ip = RET;
		} else if(ip == QUOTE) {
			a = pop(&st);
			push(&st,mknode(a,NULL));
			ip = RET;
		} else if(ip == RUN) {
			ip = pop(&st);
		} else if(ip == RET) {
			if(rs == NULL)
				return;
			ip = pop(&rs);
		} else {
			if(ip->tail == NULL) { // PUSH
				push(&st,ip->head);
				drop(ip);
				ip = RET;
			} else { // NODE
				a = ip;
				if(ip->tail != RET)
					push(&rs,ip->tail);
				ip = ip->head;
				drop(a);
			}
		}
	}
}
struct node *parse() {
	switch(getchar()) {
		case '(': ;
			struct node *inner = parse();
			return mknode(mknode(inner,NULL),parse());
		case ')': case EOF: return RET;
		case ':': return mknode(DUP,parse());
		case '!': return mknode(DROP,parse());
		case '~': return mknode(SWAP,parse());
		case '*': return mknode(CAT,parse());
		case 'a': return mknode(QUOTE,parse());
		case '^': return mknode(RUN,parse());
		default: return parse();
	}
}
int main() {
	ip = parse();
	go();
	while(st != NULL)
		push(&rs,pop(&st));
	while(rs != NULL) {
		putchar('(');
		print(pop(&rs));
		putchar(')');
	}
}
