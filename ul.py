DUP = 0
DROP = 1
SWAP = 2
CAT = 3
QUOTE = 4
RUN = 5
PUSH = 6
NODE = 7
RET = 8
class Prog:
	def __init__(this,t):
		this.t = t
	def __str__(this):
		return str(this)
def go(ip):
	st = []; push = st.append; pop = st.pop
	rs = []; rpush = rs.append; rpop = rs.pop
	while True:
		if ip.t == DUP:
			a = pop()
			push(a)
			push(a)
			ip = Prog(RET)
		elif ip.t == DROP:
			pop()
			ip = Prog(RET)
		elif ip.t == SWAP:
			a = pop()
			b = pop()
			push(a)
			push(b)
			ip = Prog(RET)
		elif ip.t == CAT:
			t = Prog(NODE)
			t.tail = pop()
			t.head = pop()
			push(t)
			ip = Prog(RET)
		elif ip.t == QUOTE:
			t = Prog(PUSH)
			t.head = pop()
			push(t)
			ip = Prog(RET)
		elif ip.t == RUN:
			ip = pop()
		elif ip.t == PUSH:
			push(ip.head)
			ip = Prog(RET)
		elif ip.t == NODE:
			if ip.tail.t == RET:
				ip = ip.head
			else:
				rpush(ip.tail)
				ip = ip.head
		elif ip.t == RET:
			if rs:
				ip = rpop()
			else:
				return st
def str(ip):
	if ip.t == DUP:
		return ":"
	elif ip.t == DROP:
		return "!"
	elif ip.t == SWAP:
		return "~"
	elif ip.t == CAT:
		return "*"
	elif ip.t == QUOTE:
		return "a"
	elif ip.t == RUN:
		return "^"
	elif ip.t == PUSH:
		return "(" + str(ip.head) + ")"
	elif ip.t == NODE:
		return str(ip.head) + str(ip.tail)
	elif ip.t == RET:
		return ""
def parse(str):
	if not str:
		return Prog(RET),""
	if str[0] == '(':
		inner,rest1 = parse(str[1:])
		after,rest2 = parse(rest1)
		t = Prog(NODE)
		t.head = Prog(PUSH)
		t.head.head = inner
		t.tail = after
		return t,rest2
	if str[0] == ')':
		return Prog(RET),str[1:]
	else:
		inner,rest1 = parse(str[1:])
		t = Prog(NODE)
		t.tail = inner
		if str[0] == ':':
			t.head = Prog(DUP)
		elif str[0] == '!':
			t.head = Prog(DROP)
		elif str[0] == '~':
			t.head = Prog(SWAP)
		elif str[0] == '*':
			t.head = Prog(CAT)
		elif str[0] == 'a':
			t.head = Prog(QUOTE)
		elif str[0] == '^':
			t.head = Prog(RUN)
		else:
			return inner,rest1
		return t,rest1
def run(prog):
	return reduce(lambda a,b:a+b,map(lambda s: "(" + str(s) + ")",reversed(go(parse(prog)[0]))))