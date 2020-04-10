## Underload Interpreters
Underload has been my favorite [esolang](https://esolangs.org/) ever since I first heard about it.  I've been mulling over how to implement a fast interpreter for a while, and now I've got something to show for it.  There are three interpreters, written in Haskell, Python and C, written in that order.

### Language
Underload is a pretty simple language, so I'm just going to summarize it here.  The only objects available in the language are programs, with each operation represented by a single character.  Brackets are used to quote a program.  The basic operations can be summarized as rewrite rules in a nice table:

|Before|After|
|---|---|
|`(X):`|`(X)(X)`|
|`(X)!`|` `|
|`(X)(Y)~`|`(Y)(X)`|
|`(X)a`|`((X))`|
|`(X)(Y)*`|`(XY)`|
|`(X)^`|`X`|

There's also a less functional operation `S` that simply prints it's argument, deleting it.  This is currently not supported for reasons explained below.

I feel that this is the simplest way to understand the language, but it can also be seen as a stack based language, which is exactly how the interpreters work internally.

That's about the minimum needed to understand the code, but you should see the [full Esolangs page](https://esolangs.org/wiki/Underload) if you want to get a good understanding of Underload besides it's interpretation.

### Output Handling
I've been billing the interpreters as full Underload interpreters this entire time, but that's actually not entirely true: the `S` operation has not been implemented at all.  The interpreters actually 'reduce' the input, in the Lambda Calculus sense, to a program with no operations at the top level.  The reason for this is both speed and complexity; supporting output of arbitrary terms would require keeping track of all the useless (computation-wise) characters in the program, either treating them as no-op instructions, or keeping track of strings and code separately at runtime.  Both would be quite expensive and complicated to implement, and did not contribute to Underload's unique computation style, so I feel the interpreters function just fine without them.

### Inner Workings
The interpreters work by manipulating the programs as lisp-style cons cells.  A cell/node/program can be defined easily in Haskell:

`data Prog = Dup | Drop | Swap | Cat | Quote | Run | Push Prog | Node Prog Prog | Ret`

The VM's state consists of three values: the return stack (rs), the data stack (st), and the instruction pointer (ip), the instruction pointer being a single node, and the stacks unsurprisingly being stacks of nodes.

Each step of the interpreter VM, the operation at the ip is executed, and the ip is loaded with the next instruction.  Most of the instructions are pretty self explanatory, but the two extra instructions `Node` and `Ret` need explaining.

`Node` essentially functions as a concatenation function; it executes its head, then its tail, pushing the tail to the rs temporarily until its head is done.  These are used to join individual operations into a program, and are created at runtime by the `*` operation.  `Ret` acts as the opposite, popping a node from the rs into the ip, and is inserted at the end of a run of instructions.

The interpreter proceeds in this way, decomposing the treelike structure of the program into smaller nodes on the rs, and building up new programs on the st, executing instructions as it reaches them.

### Development Process
The reason there are three different interpreters in three different languages essentially comes down to complexity.  Writing in a higher level language like haskell is easier for semi-functional programs like this, as evident by the relative size of the source code.  C offers more optimization opportunities, so I used Python as a sort of intermediate language for translating it into a more procedural style.  The C version was the end goal, so the Haskell and Python versions are unlikely to be updated.

### Tasks
*Done:*
* write Haskell program
* translate Haskell program to Python
* translate Python program to C
* rewrite C program to avoid large chains of if statements
* remove recursion from print
* remove recursion from parse

*TODO:*
* remove recursion from node_free; C doesn't have unlimited stack!
* somehow implement `S` operator, maybe
* become the fastest interpreter in the west
* rewrite in assembly?
