# pickle: A tiny TCL like interpreter

| Project   | Pickle: A tiny TCL like interpreter        |
| --------- | ------------------------------------------ |
| Author    | Salvator Sanfilippo (Original Interpreter) |
| Author    | Richard James Howe (Modifications Only)    |
| Copyright | 2007-2016 Salvatore Sanfilippo             |
| Copyright | 2018-2019 Richard James Howe               |
| License   | BSD                                        |
| Email     | howe.r.j.89@gmail.com                      |
| Website   | <https://github.com/howerj/pickle>         |


	   ▄███████▄  ▄█   ▄████████    ▄█   ▄█▄  ▄█          ▄████████
	  ███    ███ ███  ███    ███   ███ ▄███▀ ███         ███    ███
	  ███    ███ ███▌ ███    █▀    ███▐██▀   ███         ███    █▀
	  ███    ███ ███▌ ███         ▄█████▀    ███        ▄███▄▄▄
	▀█████████▀  ███▌ ███        ▀▀█████▄    ███       ▀▀███▀▀▀
	  ███        ███  ███    █▄    ███▐██▄   ███         ███    █▄
	  ███        ███  ███    ███   ███ ▀███▄ ███▌    ▄   ███    ███
	 ▄████▀      █▀   ████████▀    ███   ▀█▀ █████▄▄██   ██████████
				       ▀         ▀

This is a copy, and modification, of a small interpreter written by Antirez in
about 500 lines of C, this interpreter is for a small TCL like language. The
blog post describing this interpreter can be found at
<http://oldblog.antirez.com/post/picol.html>, along with the code itself at
<http://antirez.com/picol/picol.c.txt>. It does a surprising amount for such a
small amount of code.

## License

The files [pickle.c][] and [pickle.h][] are licensed under the [BSD License][],
to keep things consistent [block.c][] and [block.h][] (which contains a memory
pool allocator) is also licensed under the 2 clause [BSD License][].

## Building

To build you will need a [C][] compiler and [Make][].

Type 'make' to build the executable 'pickle' (or 'pickle.exe') on Windows. To
run type 'make run', which will drop you into a pickle shell. 'make test' will
run the built in unit tests and the unit tests in [test.tcl][].

## The Picol Language

The internals of the interpreter do not deviate from the original interpreter,
so the document on the [picol][] language still applies. The language is like a
simplified version of [TCL][], where everything is a command and the primary
data structure is the string.

Some programmers seem to an obsessive interest in their language of choice,
do not become one of those programmers. This language, like any other
language, will not solve all of your problems and may be entirely unsuitable
for the task you want to achieve. It is up to you to evaluate whether this
language, and implementation of it, is suitable.

Language and implementation advantages:

* Small and compact, easy to integrate into a wide variety of platforms and
programs. (4000 [LoC][] is the limit for the core library in [pickle.c][]).
* Fairly good at string handling
* Can be ported to a variety of platforms
* Customizable
* Suitable as a command language and shell

Disadvantages:

* Everything is a string (math operations and data structure manipulation
will be slow).
* This is a Do-It-Yourself solution, it may require that you modify the library
itself and will almost certainly require that you define your own new commands
in [C][].
* The language interpreter is not well tested and is likely to be insecure. If
you find a bug, please report it.
* Despite being a language designed to manipulate strings it lacks many
expected string operations, they have to be added by the user. The 'string'
command adds most of the functionality.
* Lacks Unicode/UTF-8 support.

Potential Improvements:

* Many, many more commands could be written in order to make this interpreter
more usable.
* The following small libraries can be used to either extend or modify the
interpreter to suite your purposes:
  - UTF-8: <https://www.cprogramming.com/tutorial/unicode.html>
  - Data Packing/Unpacking: <https://beej.us/guide/bgnet/html/multi/advanced.html#serialization>
  - Base-64: <https://stackoverflow.com/questions/342409/>
  - Line editing: <https://github.com/antirez/linenoise>
  - Fixed point (Q16.16, signed) library: <https://github.com/howerj/q>

### The language itself

Picol, and [TCL][], are dynamic languages with only one real data type, the
string. This might seem inefficient but it is fine for a glue language whose
main purpose is to bind lots of things written in C together. It is similar to
[lisp][] in ways, it is [homoiconic][], and is simple with very little in the
way of syntax.

The following table sums up the different language constructs:

	string  called if first argument
	{ }     quote, used to prevent evaluation
	[ ]     command substitution
	" "     string
	$var    variable lookup
	\c      escape a character
	#       comment
	;       terminates a command

A Picol program consists of a series of commands and arguments to those
commands. Before a command is evaluated, variables are looked up and strings
substituted.

You may have noticed that things such as 'if' or 'while', and even procedure
definition, are not part of the languages syntax. Instead, they are built in
commands and are called like any other command.

Examples of commands:

	puts "Hello, World"
	"puts" "Hello, World"

	# prints "Hello, World"
	set cmd puts
	$cmd "Hello, World"

	# prints "Hello, World"
	set a pu
	set b ts
	$a$b "Hello, World"

	+ 2 2
	- 4 5
	if {bool 4} { puts "TRUE"}

	proc x {a b} { + $a $b }
	puts "x(3, 9) == [x 3 9]"

	# prints 4 to 10 inclusive
	set z 3
	while {< $z 10} { set z [+ $z 1]; puts $z }

To best understand the language, play around with it, and look at the source,
there really is not that much there.

### Internally Defined Commands

Picol defines the commands in this section internally, in a default build all
of the commands in this section will be available. There are some build options
to remove some commands (such as the string function, the math functions, and
the list functions).

The options passed to the command and type are indicated after the command, a
question mark suffix on an argument indicates an optional command, an ellipsis
indicates an optional series of arguments.

For some concrete examples of commands being run, see [unit.tcl][], which
contains [unit tests][] for the project.

* argv

'argv' is a variable, not a function, which should contain the arguments passed
to the pickle interpreter on the command line in a TCL list.

* set variable value?

Create a variable, or overwrite an existing variable, with a value. If only one
argument is given, it returns the value of that variable if it exists or an error
if it does not.

* if {condition} {true clause}  *OR*  if {condition} {true clause} else {false clause}

*if* is the command used to implement conditional execution of either one
clause, or one clause or (exclusive or) another clause. Like in every other
programming language ever (or more accurately the languages with more than one
user, the implementer).

* while {condition} {clause}

Keep executing the while clause whilst the condition is true (ie. is non-zero).

* break

Break out of a while loop. This will continue to break out of a things until
the return code is caught by a loop, or 'catch'.

* continue

Desist from executing the rest of the clause in a while loop, and go back to
testing the condition.

* proc identifier {argument list} {function body}

Create a new command with the name 'identifier', or function if you prefer,
with the arguments in 'argument list', and code to be executed in the 'function
body'. If the final command is not a 'return' then the result of the last
command is used.

* variadic identifier name {function body}

In [TCL][] syntax can is used for variadic functions, this is one area in which
pickle differs, instead a special type of procedure can be created with the
'variadic' command. When the variadic function is called the arguments passed
to it are concatenated together to form a list which is assigned to the
argument 'name' given.

For example:

	variadic example l { puts $l }
	example
	example a
	example a b
	example a b {c d}

Prints:

	""
	"a"
	"a b"
	"a b {c d}"

Respectively.

* return string? number?

Optionally return a string, optionally with an internal number that can affect
control flow.

* uplevel number strings...

Evaluate the 'strings...' in the scope indicated by 'number'. A special case
is '#0', which is the global context. The strings are concatenated together as
with if they have been run through the 'concat' command. A scope of 0 is the
current scope, of 1, the caller, of 2, the caller's caller, and so on. A '#'
prefix is meant to reverse the search and start from the global scope and work
down through the call stack, however only '#0' is supported.

* upvar number otherVar myVar

Form a link from myVar to otherVar in the scope specified by number. A
special case is '#0', which is the global context, see 'uplevel' for a
description of the scoping traversal rules implied by the number argument.

You may have noticed that 'upvar' and 'uplevel', which come from [TCL][], are
strange, very strange. No arguments from me.

* unset string

Unset a variable, removing it from the current scope.

* eval strings...

Concatenate a list of strings with a space in-between them, as with 'concat',
then evaluate the string, returning the result of the evaluation.

* apply {{arg-list} {body}} args

Applies an argument list to a function body, substituting the provided 
arguments into the variables.

Examples:

	# Returns 4
	apply {{x} {* $x $x}} 2
	# Returns 7
	apply {{x y} {+ $x $y}} 3 4

It essential allows for anonymous functions to be made.

* mathematical operations

The following mathematical operations are defined:

'+', '-', '\*', '/', 'mod', '&lt;', '&lt;=', '&gt;', '&gt;=', '==', '!=', 
'min', 'max', 'pow', and 'log'. It should be obvious what each one does. 
It should 
be noted that because all variables are stored internally as strings, 
mathematical operations are egregiously slow. Numbers are first converted to 
strings, the operation performed, then converted back to strings. There are 
also some bitwise operations; 'lshift', 'rshift', 'and', 'or', 'xor'.

There are also the following unary mathematical operators defined: 'not'
(logical negation), 'invert' (bitwise inversion), 'abs' (absolute value), 
'bool' (turn number into a boolean 0 or 1), 'negate' (negate a number).

Numbers conversion is strict, an invalid number will not be silently converted
into a zero, or a string containing a part of a number will not become that
number, for example: "0", "-1" and "12" are valid numbers, whilst; "0a", "x",
"--2", "22x" are not.

* catch expr varname

This allows arbitrary codes to be caught, 'catch' evaluates an expression and
puts the return code into 'varname', the string returned is the result of the
evaluation of 'expr'.

* command item number *OR* command

This function is used to inspect the currently defined commands in the system.

If no arguments are given then the number of commands defined is returned. If
an item is given a number indicates which command that it applies to. Commands
are indexed by numbers. Defining new command may change the index of other
commands. Commands are either user defined or built in commands.

 - args: get a functions arguments (returns '{built-in pointer pointer}' for built in commands)
 - body: get a functions body (returns '{built-in pointer pointer}' for built in commands)
 - name: get a functions name

* join {list} String

Given a [TCL][] list, 'join' will flatten that list and return a string by
inserting a String in-between its elements. For example "join {a b c} ," yields
"a,b,c".

* conjoin String Arguments\*

'conjoin' works the same as 'join' except instead of a list it joins its
arguments, for example:

	join {a b c} ,
	conjoin , a b c

Are equivalent.

* for {start} {test} {next} {body}

Implements a for loop.

* rename function-name new-name

Rename a function to a new-name, this will fail if the function does not exist
or a function by the same name exists for the name we are trying to rename to.
A special case exists when the new-name is an empty string, the function gets
deleted.

* llength list

Get the length a list. A TCL list consists of a specially formatted string
argument, each element of that list is separated by either space or is a string
or quote. For example the following lists each contain three elements:

	"a b c"
	"a { b } c"
	"a \" b \" c"

The list is the basic higher level data structure in Pickle, and as you can
see, there is nothing special about them. They are just strings treated in a
special way. Processing these lists is incredibility inefficient as everything
is stored as a string - a list needs to be parsed before it can be manipulated
at all. This applies to all of the list functions. A more efficient, non-TCL
compatible, set of list functions could be designed, or the internals of the
library could be changed so they are more complex (which would help speeding
up the mathematical functions), but either option is undesirable for different
reasons.

* lindex list index

See 'llength'.

Index into a list, retrieving an element from that list. Indexing starts at
zero, the first element being the zeroth element.

* lrepeat number string

Repeat a string a number of times to form a list.

Examples:

	pickle> lrepeat 3 abc
	abc abc abc
	pickle> lrepeat 2 {x x}
	{x x} {x x}

* lset variable index value

Look up a variable containing a list and set the element specified by an index to
be equal to 'value'.

* linsert list index value

Insert a value into a list at a specified index, indices less than zero are
treated as zero and greater than the last element are appended to the end of
the list.

* lreplace list first last values...
* lsort opts... list

This command sorts a list, it uses [insertion sort][] internally and lacks
many of the options of the full command. It does implement the following
options:

  - '-increasing' (default)

Sort the list in increasing order.

  - '-decreasing'

Sort the list in decreasing order.

  - '-ascii' (default)

The list is a series of strings that should be stored in [ASCII][] order.

  - '-integer'

The list is a series of numbers that should be sorted numerically.

* lreverse list

Reverse the elements in a list.

* lsearch opts... list pattern

The search command attempts to find a pattern within a list and if found it
returns the index as which the pattern was found within the list, or '-1' if
it was not found.

  - '-not'

Invert the selection, matching patterns that *do not* match.

  - '-exact'

Pattern is an exact string to search for.

  - '-integer'

The pattern is a number to search for.

  - '-glob' (default)

This subcommand uses the same [regex][] syntax (and engine) as the 
'string match' subcommand, it is quite limited, and it is the default search
option.

  - '-inline'

Instead of returning the index, return the found element.

  - '-start index'

Start at the specified index instead of at zero.

* split string splitter

Split a string into a list, the value to split on is not a regular expression,
but a string literal. There is a special case where the value to split on is
the empty string, in this case it splits a string into a list of its 
constituent characters.

* lappend variable values...

Append values to a list, stored in a variable, the function returns the newly
created list.

* list args...

Turn arguments into a list, arguments with spaces in them are quoted, the
list command returns the concatenation of the escaped elements.

* concat args...

Trim arguments before concatenating them into a string.

* reg opts... regex string

'reg' implements a small regular expression engine that can be used to extract
matches from text. It has a few options that can be passed to it, and a few
virtues; lazy, greedy and possessive.

 - nocase

Ignore case when matching a string.

 - start index

Set the start of the string to match from, numbers less than zero are treated
as zero, and numbers greater than the length of the string are treated as
referring to the end of the string.

 - lazy

Match the shortest string possible.

 - greedy (default)

Match the longest string possible.

 - possessive

Match the longest string possible, with no backtracking. If backtracking is
necessary the match fails.

* unknown {list}

This command is *not* defined at startup, but can be defined by the user to
catch command-not-found exceptions.

When the interpreter encounters a command that has not been defined it attempts
to find the 'unknown' command and execute that. If it is not found, it performs
its default action, which is to throw an error and return an error string
indicating the command has not been found. If the 'unknown' command has been
found then it is executed with the command and its arguments being passed to
'unknown' as a list.

For example, defining:

	proc unknown {l} { system "$l" }

Would mean any command the interpreter does know know about will be executed by
the system shell, including its arguments.

#### String Operator

* string option arg *OR* string option arg arg *OR* string option arg arg arg

The 'string' command in [TCL][] implements nearly every string command you
could possibly want, however this version of 'string' is more limited and
behaves differently in many circumstances. 'string' also pulls in more standard
C library functions from '[ctype.h][]' and '[string.h][]'.

Some of the commands that are implemented:

  - string match pattern String

This command is a primitive regular expression matcher, as available from
<http://c-faq.com/lib/regex.html>. What it lacks in functionality, safety and
usability, it makes up for by being only ten lines long (in the original). It
is meant more for wildcard expansion of file names (so '?' replaces the meaning
of '.' is most regular expression languages). '%' is used as an escape
character, which escapes the next character.

The following operations are supported: '\*' (match any string) and '?' (match
any character). By default all patterns are anchored to match the entire
string, but the usual behavior can be emulated by prefixing the suffixing the
pattern with '\*'.

  - string trimleft  String Class?

If 'class' is empty, a white-space class is used. 'trimleft' removes leading
characters in a Class from the given String.

  - string trimright String Class?

If 'class' is empty, a white-space class is used. 'trimleft' removes trailing
characters in a class from the given String.

  - string trim      String Class?

If 'class' is empty, a white-space class is used. 'trimleft' removes both
leading and trailing characters in a class from the given String.

  - string length  String

Get the length of String. This is a simple byte length excluding an ASCII NUL
terminator.

  - string tolower String

Convert an ASCII String to lower case.

  - string toupper String

Convert an ASCII String to upper case.

  - string reverse String

Reverse a string.

  - string equal   String1 String2

Compare two strings for equality. Returns '1' if equal, '0' if not equal. This
comparison is case sensitive.

  - string compare String1 String2

Compare two strings.

  - string index   String Index

Retrieve a character from a String at the specified Index. The index starts at
zero for the first character up to the last character. Indices past the last
character return the last character. Negative indexes starting counting from
the last character (the last character being -1) and count downward, negative
indexes that go before the first character return the first character.

  - string is Class String

'is' determines whether a given String belongs to a Class. Most class tests
accept a zero length string as matching that class with a few exceptions. Most
class tests test that a class contains only certain characters (such as 'alpha'
which checks that a string only contains the characters 'a-z' and 'A-Z', or
'digit', which checks that a string only contains the characters '0-9'. Other
class tests test that a string matches a specific format, such as 'integer'
(which does not accept a zero length string), it excepts the string to contain
a decimal number with an optional '+' or '-' prefix.

Class can be:

    - [alnum][]
    - [alpha][]
    - [digit][]
    - [graph][]
    - [lower][]
    - [print][]
    - [punct][]
    - [space][]
    - [upper][]
    - [xdigit][]
    - ascii
    - [control][]
    - integer

Any other Class is invalid. Most classes are based on a C function (or macro)
available in the [ctype.h][] header.

  - string repeat String Count

Repeat a String 'Count' many times. 'Count' must be positive, inclusive of
zero.

  - string first Needle Haystack StartIndex?

Find a Needle in a Haystack, optionally starting from 'StartIndex'. The index
into the string where the first character of found of Needle in Haystack is
returned if the string has been found, negative one if it has not been found.

  - string ordinal String

Convert the first character in a string to a number that represents that
character.

  - string char Number

Convert a number to its character representation.

  - string hex2dec HexString

Convert a lower or uppercase hexadecimal number to its decimal representation.

  - string dec2hex Number

Convert a decimal number to its lowercase hexadecimal representation.

  - string hash String

Hash a string returning the hash of that string as a number.

  - string range String Index1 Index2

Create a sub-string from Index1 to Index2 from a String. If Index1 is greater
than Index2 an empty string is returned. If Index1 is less than zero, it is set
to zero, if Index2 is greater than the index of the last character, it is set
to the index of the last character. Indexing starts at zero and goes up to one
less than the strings length (or zero of empty string), which is the index of
the last character. The characters from Index1 to Index2 inclusive form the
sub-string.

 - string tr d set string *OR* string tr r set1 set2 string

Much like the Unix utility 'tr', this performs various translations given a set
(or two sets of characters). 'tr' can delete characters in the set of
characters in 'set' from 'string' if the option provided to it is 'd', or it
can perform a translation from one set to another if the 'r' specifier is
given. If the second set is larger than the first for the 'r' command the last
character applies to the rest of the characters in 'set2'.

Both 'r' and 'd' options can both have the additional specifier 'c', which
compliments the given 'set' or characters.

'r' can also have the 's' specifier, which will squeeze repeated characters in
the set

Example:

	proc lowercase {x} {
		string tr r abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ $x
	}

Which creates a function with the same functionality as 'string lowercase $x'.

 - string replace old-string first last new-string

This subcommands replaces a substring starting at 'first' and ending at 'last'.
The 'new-string' replaces the removed section of the 'old-string'.

* eq string string

Returns '0' is two strings are not equal and '1' if they are. Unlike '==' this
acts on the entire string.

* ne string string

Returns '1' is two strings are not equal and '0' if they are. Unlike '!=' this
acts on the entire string.

* incr variable number?

Increment a variable by 1, or by an optional value. 'incr' returns the
incremented variable. 'incr' being implemented in C is usually a lot more
efficient then defining 'incr' in TCL, like so:

	proc incr {x} { upvar 1 $x i; set i [+ $i 1] }

And it is used often in looping constructs.

### Extension Commands

[main.c][] extends the interpreter with some commands that make the language
usable for demonstration purposes by including routines for Input and Output,
Time, Retrieving Environment Variables, and executing commands with the Systems
Shell.

The following commands are defined:

* puts string

Write a string, followed by a newline, to the standard output stream,
[stdout][]. If an EOF is encountered, a return code of '1' occurs and the
string 'EOF' is returned.

* error string

Write an error message to the standard error stream, followed by a newline and
returns '1' for the return code.

* gets

Get a string from the standard input stream, [stdin][]. This returns a string
including a newline.

* system string?

Execute a command with the systems command interpreter, whatever that is. An
any decent Unixen this will be 'sh'. On any indecent Windows platform this will
be 'cmd.exe'. On MS-DOS this will be 'COMMAND.COM' (lol).

* exit number?

Exit from the interpreter, the number argument is optional and is coerced into
a number to be used as the exit code. This calls the C function 'exit', so
commands registered with 'atexit' will be called.

* quit number?

*quit* is a synonym for *exit*. It performs exactly the same function and is
only there because it is common to type either *quit* or *exit* in order to
leave an interpreter (I find it absurd that if you type 'exit' or 'quit'
into [python][] it knows exactly what you intended to do then does not do it.
Instead it taunts you with the correct answer).

* getenv string

Retrieve the contents of the environment variable named in string. This will
return the empty string on failure to locate the variable.

* random number?

Use whatever random number generator is available on your system to return a
random number. It should not be relied upon to produce good quality random
numbers. An optional number argument can be used to seed the pseudo random number
generator.

* clock format?

This command calls the [C][] function [strftime][] on GMT to create a new
string containing time information, or not, depending on what you have put in
the format string. The full details of the format string will not be specified
here, but here are a few examples:

	clock "Date/Time: %c"

Returns the String:

	"Date/Time: Sat Oct  6 11:32:59 2018"

When no argument is given the time since start of program execution is given.
On some systems this is the CPU time and not the total time that the program
has been executed.

* raise number

Raise a signal, what this will do depends on your system, but it will most
likely kill the process. If it does not, it returns an integer indicating the
result of calling "raise()".

* signal number action *OR* signal

This is used to both set a signal handler and to query whether a signal has
been raised. If no arguments are given then the signal fired, or zero if
no signals have been detected, is returned. A subsequent call with zero
arguments will return zero (if a signal has not be fired since). Signal events
are not queued and can be lost.

If two arguments are given, the first is treated as a signal number and
the second is the action to perform.  There are three possible actions;
"ignore", "default" and "catch". A caught signal will set a variable
which can be queried by calling signal with no arguments, as already described.
An "ignored" signal will be ignored and a signal with "default" behavior will
use the systems defaults. On successful installation of the handler, a "1" is
returned, and on failure "0".


* source file.tcl

Execute a file off disk, 'file.tcl' is the file to execute. This executes the
file in the current interpreter context and is *not* a safe operation.

* info item

Retrieve information about the system. Information items include:

 - level, call stack level
 - line, current line number
 - heap, information about the heap, if available, see 'heap' command.

But may include other information.

* heap item number *OR* heap item *OR* heap

The heap command is used to enquire about the status of the heap. Using the
command does change the thing it is measuring, however physics has the same
problem and physicists are doing pretty well. The command only works when the
custom allocator is used, as it interrogates it for the statistics it has
captured.

 - "freed": Number of calls to 'free'
 - "allocs": Number of calls to 'allocate'
 - "reallocs": Number of calls to 'realloc'
 - "active": Current active byte count
 - "max": Maximum number of bytes in use
 - "total": Total bytes request
 - "blocks": Total bytes given
 - "arenas": Number of arenas

These options require an argument; a number which species which allocation
arena to query for information.

 - "arena-size": Number of blocks
 - "arena-block": Size of a block
 - "arena-used": Number of blocks currently in use

* getch

Read a single character from standard in, returning the result as a number (you
can use "string char" to convert this to a character). -1 is returned on EOF.

* putch number

Write a character, represented numerically, to standard output. The original
character is returned if there is no error in doing this.

* fopen file-name mode

This 'fopen' opens 'file-name' in 'mode', like the C library command 'fopen'.
This 'fopen' command does not return a file handle however, it instead
registers a function which contains a file handle. The function itself has
subcommands which can operate on that hidden file handle.

To use this command:

	set fh [fopen file.txt rb]   # Open a new file
	$fh -seek 123 start          # Seek to a position in the file relative
	                             # to 'start' or 'current' or 'end'
	$fh                          # Get file position
	$fh -close                   # Close file, remove command
	$fh -error                   # Get error status of file
	$fh -eof                     # Get End Of File status of file
	$fh -getc                    # Write a character to a file
	$fh -gets                    # Get a line from a file
	$fh -rewind                  # Rewind the file stream
	$fh -putc c                  # Write a single character to file
	$fh -puts "string"           # Write a string to a file

As soon as '-close' is used on the returned function, it is removed and cannot
be used again. Subsequent uses cause errors. A similar system could be used
to implement a more efficient list data structure, whereby a small closure
is generated that can manipulate the list, instead of the current list
functions.

* frename src dst

This renames a file on disk, from 'src' to 'dst'. A special case exists if
'dst' is the empty string (which is usually an invalid file name), if 'dst' is
an empty file then the file is *deleted*.

* stdin

A file handle, as if returned by 'fopen', that reads from the standard input
steam. This file has been opened for reading.

* stdout

A file handle, as if returned by 'fopen', that reads from the standard output
steam. This file has been opened for writing.

* stdout

A file handle, as if returned by 'fopen', that reads from the standard error
output steam. This file has been opened for writing.

* errno *OR* errno -string *OR* errno -string number *OR* errno -set number

This command can be used to manipulated the 'errno' variable, which may get
clobbered by the internals of the interpreter.

Calling just 'errno' returns the current error number as a number. With the
optional '-string' command it returns the string corresponding to the current
error number. To set the current error number use 'errno -set number'. To get
the error string corresponding to an arbitrary error number, use 'errno -string
number'.

## Compile Time Options

I am not a big fan of using the [C Preprocessor][] to define a myriad of
compile time options. It leads to messy and unreadable code.

That said the following compile time options are available:

* NDEBUG

If defined this will disable assertions. It will also disable unit tests
functions from being compiled. Assertions are used heavily to check that the
library is being used correctly and to check the libraries internals, this
applies both to the block allocation routines and pickle itself.

There are other compile time options within [pickle.c][] that control;
maximum string length and whether to use one, whether to provide the default
allocator or not, whether certain functions are to be made available to the
interpreter or not (such as the command 'string', the mathematical operators
and the list functions), and whether strict numeric conversion is used. 
These options are semi-internal, they are subject to change and removal, you
should use the source to determine what they are and be aware that they may
change across releases.

Inevitably when an interpreter is made for a new language,
[readline][] (or [linenoise][]) integration is a build option, usually because
the author is tired of pressing the up arrow key and seeing '^\[\[A'. Naturally
this increases the complexity of the build system, adds more options, and adds
more code. Instead you can use [rlwrap][], or an alternative, as a wrapper
around your program.

## Custom Allocator / C Library usage

To aid in porting the system to embedded platforms, [pickle.c][] contains no
Input and Output functions (they are added in by registering commands in
[main.c][]). [pickle.c][] does include [stdio.h][], but only to access
[vsnprintf][]. The big problem with porting a string heavy language to an
embedded platform, unlike a language like [FORTH][], is memory allocation. It
is unavoidable that some kind of dynamic memory allocation is required. For
this purpose it is possible to provide your own allocator to the pickle
library. If an allocator is not provided, malloc will be used, you can remove
this from the initialization function in to stop your build system pulling in
your platforms allocator.

The block allocation library provided in [block.c][] can be optionally
used, but unlike [malloc][] will require tweaking to suite your purposes. The
maximum block size available to the allocator will also determine the maximum
string size that can be used by pickle.

Apart from [vsnprintf][], the other functions pulled in from the C
library are quite easy to implement. They include (but are not necessarily
limited to); strlen, memcpy, memchr, memset and abort.

## C API

The language can be extended by defining new commands in [C][] and registering
those commands with the *pickle\_register\_command* function. The internal
structures used are mostly opaque and can be interacted with from within the
language. As stated a custom allocator can be used and a block allocator is
provided, it is possible to do quite a bit with this scripting language whilst
only allocating about 32KiB of memory total on a 64-bit machine (for example
all of the unit tests and example programs run within that amount).

The C API is small and regular. All of the functions exported return the same
error codes and implementing an interpreter loop is trivial.

The language can be extended with new functions written in C, each function
accepts an integer length, and an array of pointers to ASCIIZ strings - much
like the 'main' function in C.

User defined commands can be registered with the 'pickle\_register\_command'
function. With in the user defined callbacks the 'pickle\_set\_result' family of
functions can be used. The callbacks passed to 'pickle\_register\_command' look
like this:

	typedef int (*pickle_command_func_t)(pickle_t *i, int argc, char **argv, void *privdata);

The callbacks accept a pointer to an instance of the pickle interpreter, and
a list of strings (in 'argc' and 'argv'). Arbitrary data may be passed to the
custom callback when the command is registered.

The function returns one of the following status codes:

	PICKLE_ERROR    = -1 (Throw an error until caught)
	PICKLE_OK       =  0 (Signal success, continue on execution)
	PICKLE_RETURN   =  1 (Return out of a function)
	PICKLE_BREAK    =  2 (Break out of a while loop)
	PICKLE_CONTINUE =  3 (Immediately proceed to next iteration of while loop)

These error codes can affect the flow control within the interpreter. The
actual return value of the callback is set with 'pickle\_set\_result' functions.

Some functions are define purely for convenience and are not strictly
necessary, such as 'pickle\_set\_result\_error' and 'pickle\_set\_result\_error\_arity',
these return 'PICKLE\_ERROR' always, the latter function deals specifically with
arity errors within functions, formatting the return buffer an number of
arguments related error message. Likewise, the functions that set the return
value to a number and not a string are wrappers around calls to [vsnprintf][]
and 'pickle\_set\_result\_string'. The 'get' and 'set' functions return 'PICKLE\_OK'
on success, and 'PICKLE\_ERROR' on failure. A 'get' fails if the variable does
not exists, a 'set' on a variable that does not exist creates that variable
(which may failure, returning 'PICKLE\_ERROR').

Variables can be set either within or outside of the user defined callbacks
with the 'pickle\_set\_variable' family of functions.

The pickle library does not come with many built in functions, and comes with
no Input/Output functions (even those available in the C standard library) to
make porting to non-hosted environments easier. The example test driver program
does add functions available in the standard library.

The following is the source code for a simple interpreter loop that reads a
line and then evaluates it:

	#include "pickle.h"
	#include <stdio.h>
	#include <string.h>
	
	int main(void) {
		pickle_t *p = NULL;
		if (pickle_new(&p, NULL) < 0)
			return -1;
		const char *prompt = "> ";
		fputs(prompt, stdout);
		fflush(stdout);
		for (char buf[80] = { 0 }; fgets(buf, sizeof buf, stdin); memset(buf, 0, sizeof buf)) {
			const char *r = NULL;
			const int er = pickle_eval(p, buf);
			pickle_get_result_string(p, &r);
			fprintf(stdout, "[%d]: %s\\n%s", er, r, prompt);
			fflush(stdout);
		}
		return pickle_delete(p);
	}

Also present is a custom prompt.

It should be obvious that the interface presented is not efficient for many
uses, treating everything as a string has a cost. It is however simple and
sufficient for many tasks.

While API presented in 'pickle.h' is small there are a few areas of
complication. They are: The memory allocation API, registering a command, the
getopt function and the unit tests. The most involved is the memory allocation
API and there is not too much to it, you do not even need to use it and can
pass a NULL to 'pickle\_new' if for the allocator argument if you want to use
the built in malloc/realloc/free based allocator (provided the library was
built with support for it).

It may not be obvious from the API how to go about designing functions to
integrate with the interpreter. The C API is deliberately kept as simple as
possible, more could be exported but there is a trade-off in doing so; it
places more of a burden on backwards compatibility, limits the development
of the library internals and makes the library more difficult to use. It is
always possible to hack your own personal copy of the library to suite your
purpose, the library is small enough that this should be possible.

The Pickle interpreter has no way of registering different types with it, the
string is king. As such, it is not immediately clear what the best way of
adding functionality that requires manipulating non-string data (such as
file handles or pointers to binary blobs) is. There are several ways of doing
this:

1. Convert the pointer to a string and add functions which deal with this string.
2. Put data into the private data field
3. Create a function which registers another function that contains private data.

Option '1' may seem natural, but it is much more error prone. It is possible
to pass the wrong string around and cause the program to crash. Option '2' is
limiting, the C portion of the program is entirely in control of what resources
get added, and only one handle to a resource can be controlled. Option '2' is
a good option for certain cases.

Option '3' is the most general and allows an arbitrary resource to be managed
by the interpreter. The idea is to create a function that acquires the resource
to be managed and registers a new function in the pickle global function
namespace with the resource in the private data field of the newly registered
function. The newly created function, a limited form of a closure, can then
perform operations on the handle. It can also cleanup the resource by release
the object in its private data field, and then deleting itself with the
 'pickle\_rename\_command' function. An example of this is the 'fopen' command,
it returns a closure which contains a file handle.

An example of using the 'fopen' command and the returned function from within
the pickle interpeter is:

	set fh [fopen file.txt rb]
	set line [$fh -gets]
	$fh -close

And an example of how this might be implemented in C is:


	int pickleCommandFile(pickle_t *i, int argc, char **argv, void *pd) {
		FILE *fh = (FILE*)pd;
		if (!strcmp(argv[1], "-close")) { /* delete self */
			fclose(fh);                                /* free handle */
			return pickle_rename_command(argv[0], ""); /* delete self */
		}
		if (!strcmp(argv[1], "-gets")) {
			char buf[512];
			fgets(buf, sizeof buf, fh);
			return pickle_set_result_string(i, buf);
		}
		return pickle_set_result_error(i, "invalid option");
	}

	int pickleCommandFopen(pickle_t *i, int argc, char **argv, void *pd) {
		char name[64];
		FILE *fh = fopen(argv[1], argv[2]);
		sprintf(name, "%p", fh); /* unique name */
		pickle_register_command(i, name, pickleCommandFile, fh);
		return pickle_set_result_string(i, name);
	}

The code illustrates the point, but lacks the assertions, error checking,
and functionality of the real 'fopen' command. The 'pickleCommandFopen' should
be registered with 'pickle\_register\_command', the 'pickleCommandFile' is not
as 'pickleCommandFopen' does the registering when needed.

It should be possible to implement the commands 'update', 'after' and 'vwait',
extending the interpreter with task management like behavior without any changes
to the API. It should be possible to implement most commands, although it might
be awkward to do so. Cleanup is still a problem.

## Style Guide

Style/coding guide and notes, for the file [pickle.c][]:

- 'pickle\_' and snake\_case is used for exported functions/variables/types
- 'picol'  and camelCase  is used for internal functions/variables/types,
with a few exceptions, such as 'advance' and 'compare', which are internal
functions whose names are deliberately kept short.
- Use asserts wherever you can for as many preconditions, postconditions
and invariants that you can think of.
- Make sure you make your functions static unless they are meant to
be exported. You can use 'objdump -t | awk '$2 ~ /[Gg]/' to find
all global functions. 'objdump -t | grep '\\\*UND\\\*' can be used to
check that you are not pulling in functions from the C library you
do not intend as well.
- The core project is written strictly in C99 and uses only things
that can be found in the standard library, and only things that are
easy to implement on a microcontroller.
- Error messages should begin with the string 'Invalid', even it is
does not make the best grammatical sense, this is so error messages can
be grepped for easily.

The callbacks all have their 'argv' argument defined as 'char\*',
as they do not modify their arguments. However adding this in just adds
a lot of noise to the function definitions. Also see
<http://c-faq.com/ansi/constmismatch.html>.

## Interpreter Limitations

Known limitations of the interpreter include:

* Recursion Depth - 128, set via a compile time option.
* Maximum size of file - 2GiB

[block.h]: block.h
[main.c]: main.c
[picol.c]: picol.c
[unit.tcl]: unit.tcl
[picol]: http://oldblog.antirez.com/post/picol.html
[TCL]: https://en.wikipedia.org/wiki/Tcl
[stdio.h]: http://www.cplusplus.com/reference/cstdio/
[snprintf]: http://www.cplusplus.com/reference/cstdio/snprintf/
[vsnprintf]: http://www.cplusplus.com/reference/cstdio/vsnprintf/
[FORTH]: https://en.wikipedia.org/wiki/Forth_(programming_language)
[C]: https://en.wikipedia.org/wiki/C_%28programming_language%29
[Make]: https://en.wikipedia.org/wiki/Make_(software)
[MIT License]: https://en.wikipedia.org/wiki/MIT_License
[BSD License]: https://en.wikipedia.org/wiki/BSD_licenses
[malloc]: https://en.wikipedia.org/wiki/C_dynamic_memory_allocation
[C Preprocessor]: https://en.wikipedia.org/wiki/C_preprocessor
[python]: https://www.python.org/
[stdout]: http://www.cplusplus.com/reference/cstdio/stdout/
[stdin]: http://www.cplusplus.com/reference/cstdio/stdin/
[strftime]: http://www.cplusplus.com/reference/ctime/strftime/
[readline]: https://tiswww.case.edu/php/chet/readline/rltop.html
[linenoise]: https://github.com/antirez/linenoise
[rlwrap]: https://linux.die.net/man/1/rlwrap
[lisp]: https://en.wikipedia.org/wiki/Lisp_(programming_language)
[homoiconic]: https://en.wikipedia.org/wiki/Homoiconicity
[loc]: https://en.wikipedia.org/wiki/Source_lines_of_code
[pickle-all]: https://github.com/howerj/pickle-all
[insertion sort]: https://en.wikipedia.org/wiki/Insertion_sort
[regex]: http://c-faq.com/lib/regex.html
[ASCII]: https://en.wikipedia.org/wiki/ASCII
[unit tests]: https://en.wikipedia.org/wiki/Unit_testing
[ctype.h]: http://www.cplusplus.com/reference/cctype/
[string.h]: http://www.cplusplus.com/reference/cstring/
[alnum]: http://www.cplusplus.com/reference/cctype/isalnum/
[alpha]: http://www.cplusplus.com/reference/cctype/isalpha/
[digit]: http://www.cplusplus.com/reference/cctype/isdigit/
[graph]: http://www.cplusplus.com/reference/cctype/isgraph/
[lower]: http://www.cplusplus.com/reference/cctype/islower/
[print]: http://www.cplusplus.com/reference/cctype/isprint/
[punct]: http://www.cplusplus.com/reference/cctype/ispunct/
[space]: http://www.cplusplus.com/reference/cctype/isspace/
[upper]: http://www.cplusplus.com/reference/cctype/isupper/
[xdigit]: http://www.cplusplus.com/reference/cctype/isxdigit/
[control]: http://www.cplusplus.com/reference/cctype/iscntrl/
[cdb]: https://github.com/howerj/cdb
