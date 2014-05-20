/*
 * Copyright (c) 2003
 *      David Leonard.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of David Leonard nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Combined parser and evaluator.
 *
 * This file contains two threads (storylines): the LL(2) recursive
 * descent parser thread, and the semantic functions thread. The parsing and 
 * semantics stages are grouped together by their common productions in the
 * grammar, to facilitate reference to the ECMA-262 standard.
 *
 * The input to the parser is an instance of the lexical analyser.
 * The output of the parser is an abstract syntax tree (AST). The input to
 * the evaluator is the AST, and the output of the evaluator is a SEE_value.
 *
 * For each production PROD in the grammar, the function PROD_parse() 
 * allocates and populates a 'node' structure, representing the root
 * of the syntax tree that represents the parsed production. Each node
 * holds a 'nodeclass' pointer to semantics information as well as
 * production-specific information. 
 *
 * Names of structures and functions have been chosen to correspond 
 * with the production names from the standard's grammar.
 *
 * The semantic functions in each node class are the following:
 *
 *  - PROD_eval() functions are called at runtime to implement
 *    the behaviour of the program. They "evaluate" the program.
 *
 *  - PROD_fproc() functions are called at execution time to generate
 *    the name/value bindings between container objects and included
 *    function objects. (It finds functions and assigns them to properties.)
 *    They provide a parallel, but independent, recusive semantic operation
 *    described in the standard as "process[ing] for function 
 *    declarations".
 *
 *  - PROD_print() functions are used to print the abstract syntax tree 
 *    to stdout during debugging.
 *
 * TODO This file is far too big; need to split it up.
 * TODO Compact/bytecode intermediate form
 *
 */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#if STDC_HEADERS
# include <stdio.h>
#endif

#if HAVE_STRING_H
# include <string.h>            /* memset */
#endif

#include <see/mem.h>
#include <see/try.h>
#include <see/string.h>
#include <see/interpreter.h>
#include <see/context.h>
#include <see/input.h>
#include <see/eval.h>
#include <see/error.h>
#include <see/system.h>
#include <see/intern.h>

#include "parse.h"
#include "lex.h"
#include "stringdefs.h"
#include "dprint.h"
#include "function.h"
#include "tokens.h"


#include "parse_node.h"
#include "parse_const.h"
#if WITH_PARSER_PRINT
# include "parse_print.h"
#endif
#if WITH_PARSER_CODEGEN
# include "parse_codegen.h"
#else
# include "parse_eval.h"
#endif

#ifndef NDEBUG
int SEE_parse_debug = 0;
int SEE_eval_debug = 0;
#endif

/*
 * A label is the identifier (string) before a statement that binds a 
 * "labelset" to the statement. Conversely, a labelset represents the 
 * set of labels associated with a statement. Labelsets are the targets 
 * of break and continue statements. If a 'break' or 'continue' statement 
 * is followed by an identifier, then the current label stack is searched 
 * for the corresponding labelset.
 *
 * For 'break' or 'continue' statements without a label, the label stack 
 * is searched for the internal names ".BREAK" or ".CONTINUE", respectively.
 * All 'iteration' statements are initialised with a labelset containing
 * one or both of .BREAK or .CONTINUE.
 *
 * A break to a labelset branches to the end of its statement. A continue
 * to a labelset branches to its statement's 'continue point', which is 
 * usually the body of the iteration.
 *
 * During any statement the labelset_current() method returns the currently
 * active labelset.
 * 
 * A labelset is given a unique target number only if and when it is 
 * referenced. In this way, the parser generates sequential branch targets 
 * IDs. Unused labelsets have an internal ID of -1.
 */

struct labelset {
	int		 continuable;	    /* can be target of continue */
	unsigned int	 target;	    /* unique id or -1 */
	struct labelset *next;		    /* list of all labelsets */
};

struct label {
	struct SEE_string *name;	    /* interned label name */
	struct labelset	*labelset;	    /* containing labelset */
	struct SEE_throw_location location; /* where the label is defined */
	struct label	*next;		    /* stack link of active labels */
};

#define UNGET_MAX 3
struct parser {
	struct SEE_interpreter *interpreter;
	struct lex 	 *lex;
	int		  unget, unget_end;
	struct SEE_value  unget_val[UNGET_MAX];
	int               unget_tok[UNGET_MAX];
	int               unget_lin[UNGET_MAX];
	SEE_boolean_t     unget_fnl[UNGET_MAX];
	int 		  noin;	  /* ignore 'in' in RelationalExpression */
	int		  is_lhs; /* derived LeftHandSideExpression */
	int		  funcdepth;
	struct var	**vars;		    /* list of declared variables */
	struct labelset	 *labelsets;	    /* list of all labelsets */
	struct label     *labels;	    /* stack of active labels */
	struct labelset	 *current_labelset; /* statement's labelset or NULL */
};


/*------------------------------------------------------------
 * function prototypes
 */

static struct node *new_node_internal(struct SEE_interpreter*interp, int sz, 
        enum nodeclass_enum nc, struct SEE_string* filename, int lineno,
	const char *dbg_nc);
static struct node *new_node(struct parser *parser, int sz, 
        enum nodeclass_enum nc, const char *dbg_nc);
static void parser_init(struct parser *parser, 
        struct SEE_interpreter *interp, struct lex *lex);
static unsigned int target_lookup(struct parser *parser, 
	struct SEE_string *name, int kind);
static int lookahead(struct parser *parser, int n);

static struct SEE_string *error_at(struct parser *parser, const char *fmt, 
        ...);
static struct node *Literal_parse(struct parser *parser);
static struct node *NumericLiteral_parse(struct parser *parser);
static struct node *StringLiteral_parse(struct parser *parser);
static struct node *RegularExpressionLiteral_parse(struct parser *parser);
static struct node *PrimaryExpression_parse(struct parser *parser);
static struct node *ArrayLiteral_parse(struct parser *parser);
static struct node *ObjectLiteral_parse(struct parser *parser);
static struct Arguments_node *Arguments_parse(struct parser *parser);
static struct node *MemberExpression_parse(struct parser *parser);
static struct node *LeftHandSideExpression_parse(struct parser *parser);
static struct node *PostfixExpression_parse(struct parser *parser);
static struct node *UnaryExpression_parse(struct parser *parser);
static struct node *MultiplicativeExpression_parse(struct parser *parser);
static struct node *AdditiveExpression_parse(struct parser *parser);
static struct node *ShiftExpression_parse(struct parser *parser);
static struct node *RelationalExpression_parse(struct parser *parser);
static struct node *EqualityExpression_parse(struct parser *parser);
static struct node *BitwiseANDExpression_parse(struct parser *parser);
static struct node *BitwiseXORExpression_parse(struct parser *parser);
static struct node *BitwiseORExpression_parse(struct parser *parser);
static struct node *LogicalANDExpression_parse(struct parser *parser);
static struct node *LogicalORExpression_parse(struct parser *parser);
static struct node *ConditionalExpression_parse(struct parser *parser);
static struct node *AssignmentExpression_parse(struct parser *parser);
static struct node *Expression_parse(struct parser *parser);
static struct node *Statement_parse(struct parser *parser);
static struct node *Block_parse(struct parser *parser);
static struct node *StatementList_parse(struct parser *parser);
static struct node *VariableStatement_parse(struct parser *parser);
static struct node *VariableDeclarationList_parse(struct parser *parser);
static struct node *VariableDeclaration_parse(struct parser *parser);
static struct node *EmptyStatement_parse(struct parser *parser);
static struct node *ExpressionStatement_parse(struct parser *parser);
static struct node *ExpressionStatement_make(struct SEE_interpreter *,
	struct node *);
static struct node *IfStatement_parse(struct parser *parser);
static struct node *IterationStatement_parse(struct parser *parser);
static struct node *ContinueStatement_parse(struct parser *parser);
static struct node *BreakStatement_parse(struct parser *parser);
static struct node *ReturnStatement_parse(struct parser *parser);
static struct node *WithStatement_parse(struct parser *parser);
static struct node *SwitchStatement_parse(struct parser *parser);
static struct node *LabelledStatement_parse(struct parser *parser);
static struct node *ThrowStatement_parse(struct parser *parser);
static struct node *TryStatement_parse(struct parser *parser);
static struct node *FunctionDeclaration_parse(struct parser *parser);
static struct node *FunctionExpression_parse(struct parser *parser);
static struct var *FormalParameterList_parse(struct parser *parser);
static struct node *FunctionBody_parse(struct parser *parser);
static struct node *FunctionBody_make(struct SEE_interpreter *, 
	struct node *, int);
static struct node *FunctionStatement_parse(struct parser *parser);
static struct function *Program_parse(struct parser *parser);
static struct node *SourceElements_make1(struct SEE_interpreter *,
	struct node *);
static struct node *SourceElements_parse(struct parser *parser);
static void eval_functionbody(void *, struct SEE_context *, struct SEE_value *);

static void *make_body(struct SEE_interpreter *, struct node *, int);

#define NO_CONST    1

/*------------------------------------------------------------
 * macros
 */

/*
 * Macros for accessing the tokeniser, with lookahead
 */
#define NEXT 						\
	(parser->unget != parser->unget_end		\
		? parser->unget_tok[parser->unget] 	\
		: parser->lex->next)
#define NEXT_VALUE					\
	(parser->unget != parser->unget_end		\
		? &parser->unget_val[parser->unget] 	\
		: &parser->lex->value)

#define NEXT_LINENO					\
	(parser->unget != parser->unget_end		\
		? parser->unget_lin[parser->unget] 	\
		: parser->lex->next_lineno)

#define NEXT_FILENAME					\
		  parser->lex->next_filename

#define NEXT_FOLLOWS_NL					\
	(parser->unget != parser->unget_end		\
		? parser->unget_fnl[parser->unget] 	\
		: parser->lex->next_follows_nl)

#define SKIP						\
    do {						\
	if (parser->unget == parser->unget_end)		\
		SEE_lex_next(parser->lex);		\
	else 						\
		parser->unget = 			\
		    (parser->unget + 1) % UNGET_MAX;	\
	SKIP_DEBUG					\
    } while (0)

#ifndef NDEBUG
#  define SKIP_DEBUG					\
    if (SEE_parse_debug)				\
      dprintf("SKIP: next = %s\n", SEE_tokenname(NEXT));
#else
#  define SKIP_DEBUG
#endif

/* Handy macros for describing syntax errors */
#define EXPECT(c) EXPECTX(c, SEE_tokenname(c))
#define EXPECTX(c, tokstr)				\
    do { 						\
	EXPECTX_NOSKIP(c, tokstr);			\
	SKIP;						\
    } while (0)
#define EXPECT_NOSKIP(c) EXPECTX_NOSKIP(c, SEE_tokenname(c))
#define EXPECTX_NOSKIP(c, tokstr)			\
    do { 						\
	if (NEXT != (c)) 				\
	    EXPECTED(tokstr);				\
    } while (0)
#define EXPECTED(tokstr)				\
    do { 						\
	    char nexttok[30];				\
	    SEE_tokenname_buf(NEXT, nexttok, 		\
		sizeof nexttok);			\
	    SEE_error_throw_string(			\
		parser->interpreter,			\
		parser->interpreter->SyntaxError,	\
		error_at(parser, 			\
		         "expected %s but got %s",	\
		         tokstr,			\
		         nexttok));			\
    } while (0)

#define EMPTY_LABEL		((struct SEE_string *)NULL)

/* 
 * Automatic semicolon insertion macros.
 *
 * Using these instead of NEXT/SKIP allows synthesis of
 * semicolons where they are permitted by the standard.
 */
#define NEXT_IS_SEMICOLON				\
	(NEXT == ';' || NEXT == '}' || NEXT_FOLLOWS_NL)
#define EXPECT_SEMICOLON				\
    do {						\
	if (NEXT == ';')				\
		SKIP;					\
	else if ((NEXT == '}' || NEXT_FOLLOWS_NL)) {	\
		/* automatic semicolon insertion */	\
	} else						\
		EXPECTX(';', "';', '}' or newline");	\
    } while (0)

/*
 * Macros for accessing the abstract syntax tree
 */

#ifndef NDEBUG
#define NEW_NODE(t, nc)					\
	((t *)new_node(parser, sizeof (t), nc, #nc))
#define NEW_NODE_INTERNAL(i, t, nc)			\
	((t *)new_node_internal(i, sizeof (t), nc, STR(empty_string), 0, #nc))
#else
#define NEW_NODE(t, nc)					\
	((t *)new_node(parser, sizeof (t), nc, NULL))
#define NEW_NODE_INTERNAL(i, t, nc)			\
	((t *)new_node_internal(i, sizeof (t), nc, STR(empty_string), 0, NULL))
#endif

#ifndef NDEBUG
#define PARSE(prod)					\
    ((void)(SEE_parse_debug ? 				\
	dprintf("parse %s next=%s\n", #prod,		\
	    SEE_tokenname(NEXT)) : (void)0),		\
        prod##_parse(parser))
#else
#define PARSE(prod)					\
        prod##_parse(parser)
#endif

/* Generates a generic parse error */
#define ERROR						\
	SEE_error_throw_string(				\
	    parser->interpreter,			\
	    parser->interpreter->SyntaxError,		\
	    error_at(parser, "parse error before %s",	\
	    SEE_tokenname(NEXT)))

/* Generates a specific parse error */
#define ERRORm(m)					\
	SEE_error_throw_string(				\
	    parser->interpreter,			\
	    parser->interpreter->SyntaxError,		\
	    error_at(parser, "%s, near %s",		\
	    m, SEE_tokenname(NEXT)))


/* Codegen macros */


/*------------------------------------------------------------
 * Allocators and initialisers
 */

/*
 * Creates a new AST node, initialising it with the 
 * given node class nc, and recording the current
 * line number as reported by the parser.
 */
static struct node *
new_node_internal(interp, sz, nc, filename, lineno, dbg_nc)
	struct SEE_interpreter *interp;
	int sz;
	enum nodeclass_enum nc;
	struct SEE_string *filename;
	int lineno;
	const char *dbg_nc;
{
	struct node *n;

	n = (struct node *)SEE_malloc(interp, sz);
	n->nodeclass = nc;
	n->location.filename = filename;
	n->location.lineno = lineno;
	n->flags = 0;

        /* XXX codegen only */
	n->is = 0;
	n->maxstack = 0;

	return n;
}

static struct node *
new_node(parser, sz, nc, dbg_nc)
	struct parser *parser;
	int sz;
	enum nodeclass_enum nc;
	const char *dbg_nc;
{
	struct node *n;

	n = new_node_internal(parser->interpreter, sz, nc, 
	    NEXT_FILENAME, NEXT_LINENO, dbg_nc);
#ifndef NDEBUG
	if (SEE_parse_debug) 
		dprintf("parse: %p %s (next=%s)\n", 
			n, dbg_nc, SEE_tokenname(NEXT));
#endif
	return n;
}

/*
 * Initialises a parser state.
 */
static void
parser_init(parser, interp, lex)
	struct parser *parser;
	struct SEE_interpreter *interp;
	struct lex *lex;
{
	parser->interpreter = interp;
	parser->lex = lex;
	parser->unget = 0;
	parser->unget_end = 0;
	parser->noin = 0;
	parser->is_lhs = 0;
	parser->funcdepth = 0;
	parser->vars = NULL;
	parser->labelsets = NULL;
	parser->labels = NULL;
	parser->current_labelset = NULL;
}

/*------------------------------------------------------------
 * Labels
 */

/* Returns a labelset for the current statement, creating one if needed. */
static struct labelset *
labelset_current(parser)
	struct parser *parser;
{
	struct labelset *ls;

	if (!parser->current_labelset) {
	    ls = SEE_NEW(parser->interpreter, struct labelset);
	    if (parser->labelsets)
		ls->target = parser->labelsets->target + 1;
	    else
		ls->target = 1;
	    ls->next = parser->labelsets;
	    parser->labelsets = ls;
	    parser->current_labelset = ls;
#ifndef NDEBUG
	    if (SEE_parse_debug)
		dprintf("labelset_current(): new %p\n", 
		    parser->current_labelset);
#endif
	}
	return parser->current_labelset;
}

/*
 * Pushes a new label for the current labelset onto the label scope stack.
 * Checks for duplicate labels, which are not allowed, except for EMPTY_LABEL.
 */
static void
label_enter(parser, name)
	struct parser *parser;
	struct SEE_string *name;
{
	struct label *l;
	struct SEE_string *msg;
	struct SEE_throw_location location;

	location.lineno = NEXT_LINENO;
	location.filename = NEXT_FILENAME;

#ifndef NDEBUG
	if (SEE_parse_debug) {
	    dprintf("label_enter() [");
	    if (name == EMPTY_LABEL)
		dprintf("EMPTY_LABEL");
	    else
		dprints(name);
	    dprintf("]\n");
	}
#endif

	if (name != EMPTY_LABEL)
	    for (l = parser->labels; l; l = l->next)
		if (l->name == name) {
		    msg = SEE_location_string(parser->interpreter, &location);
		    SEE_string_append(msg, STR(duplicate_label));
		    SEE_string_append(msg, name);
		    SEE_string_addch(msg, '\'');
		    SEE_string_addch(msg, ';');
		    SEE_string_addch(msg, ' ');
		    SEE_string_append(msg, 
			SEE_location_string(parser->interpreter, 
			    &l->location));
		    SEE_string_append(msg, STR(previous_definition));
		    SEE_error_throw_string(parser->interpreter,
			    parser->interpreter->SyntaxError, msg);
		}


	l = SEE_NEW(parser->interpreter, struct label);
	l->name = name;
	l->labelset = labelset_current(parser);
	l->location.lineno = location.lineno;
	l->location.filename = location.filename;
	/* Push onto parser->labels */
	l->next = parser->labels;
	parser->labels = l;
}

/* Pops the last label pushed by label_enter(). */
static void
label_leave(parser)
	struct parser *parser;
{

	SEE_ASSERT(parser->interpreter, parser->labels != NULL);
#ifndef NDEBUG
	if (SEE_parse_debug) {
	    dprintf("label_leave() [");
	    if (parser->labels->name == EMPTY_LABEL)
		dprintf("EMPTY_LABEL");
	    else
		dprints(parser->labels->name);
	    dprintf("]\n");
	}
#endif
	parser->labels = parser->labels->next;
}

/*
 * Returns the target ID correspnding to the label, or raises a SyntaxError
 * if it isn't found.
 * Kind is a token indicating the kind of statement using the label 
 * (tBREAK or tCONTINUE), and a SyntaxError is thrown if the found labelset
 * is incompatible.
 * Always returns a valid target ID.
 */
static unsigned int
target_lookup(parser, label_name, kind)
	struct parser *parser;
	struct SEE_string *label_name;
	int kind;
{
	struct SEE_string *msg;
	struct label *l;

	SEE_ASSERT(parser->interpreter, kind == tBREAK || kind == tCONTINUE);

#ifndef NDEBUG
	if (SEE_parse_debug) {
	    dprintf("labelset_lookup_target: searching for '");
	    if (label_name == EMPTY_LABEL)
	        dprintf("EMPTY_LABEL");
	    else
	    	dprints(label_name);
	    dprintf("\n");
	}
#endif

	for (l = parser->labels; l; l = l->next)
	    if (l->name == label_name) {
	        if (kind == tCONTINUE && !l->labelset->continuable) {
		    if (label_name == EMPTY_LABEL)
		        continue;
		    msg = error_at(parser, "label '");
		    SEE_string_append(msg, label_name);
		    SEE_string_append(msg, 
			SEE_string_sprintf(parser->interpreter,
			"' not suitable for continue"));
		    SEE_error_throw_string(parser->interpreter,
			    parser->interpreter->SyntaxError, msg);
		}
		return l->labelset->target;
	    }

	if (label_name) {
	    msg = error_at(parser, "label '");
	    SEE_string_append(msg, label_name);
	    SEE_string_append(msg, 
		SEE_string_sprintf(parser->interpreter,
		"' not defined, or not reachable"));
	} else if (kind == tCONTINUE)
	    msg = error_at(parser,
		"continue statement not within a loop");
	else /* kind == tBREAK */
	    msg = error_at(parser,
		"break statement not within loop or switch");

	SEE_error_throw_string(parser->interpreter,
		parser->interpreter->SyntaxError, msg);
	/* NOTREACHED */
}


/*------------------------------------------------------------
 * Code generator helper functions
 */

static void *
make_body(interp, node, no_const)
	struct SEE_interpreter *interp;
	struct node *node;
	int no_const;
{
#if WITH_PARSER_CODEGEN
        return _SEE_codegen_make_body(interp, node, no_const);
#else
	return _SEE_eval_make_body(interp, node, no_const);
#endif
}


/*------------------------------------------------------------
 * LL(2) lookahead implementation
 */

/*
 * Returns the token that is n tokens ahead. (0 is the next token.)
 */
static int
lookahead(parser, n)
	struct parser *parser;
	int n;
{
	int token;
	SEE_ASSERT(parser->interpreter, n < (UNGET_MAX - 1));

	while ((UNGET_MAX + parser->unget_end - parser->unget) % UNGET_MAX < n)
	{
	    SEE_VALUE_COPY(&parser->unget_val[parser->unget_end], 
		&parser->lex->value);
	    parser->unget_tok[parser->unget_end] =
		parser->lex->next;
	    parser->unget_lin[parser->unget_end] =
		parser->lex->next_lineno;
	    parser->unget_fnl[parser->unget_end] = 
		parser->lex->next_follows_nl;
	    SEE_lex_next(parser->lex);
	    parser->unget_end = (parser->unget_end + 1) % UNGET_MAX;
	}
	if ((parser->unget + n) % UNGET_MAX == parser->unget_end)
		token = parser->lex->next;
	else
		token = parser->unget_tok[(parser->unget + n) % UNGET_MAX];

#ifndef NDEBUG
	if (SEE_parse_debug)
	    dprintf("lookahead(%d) -> %s\n", n, SEE_tokenname(token));
#endif

	return token;
}




/*------------------------------------------------------------
 * Error handling
 */

/*
 * Generates an error string prefixed with the filename and 
 * line number of the next token. e.g. "foo.js:23: blah blah".
 * This is useful for error messages.
 */
static struct SEE_string *
error_at(struct parser *parser, const char *fmt, ...)
{
	va_list ap;
	struct SEE_throw_location here;
	struct SEE_string *msg;
	struct SEE_interpreter *interp = parser->interpreter;

	here.lineno = NEXT_LINENO;
	here.filename = NEXT_FILENAME;

	va_start(ap, fmt);
	msg = SEE_string_vsprintf(interp, fmt, ap);
	va_end(ap);

	return SEE_string_concat(interp,
	    SEE_location_string(interp, &here), msg);
}

/*------------------------------------------------------------
 * Parser
 *
 * Each group of grammar productions is ordered:
 *   - production summary as a comment
 *   - node structure
 *   - evaluator function
 *   - function processor
 *   - node printer
 *   - recursive-descent parser
 */

/* -- 7.8
 *	Literal:
 *	 	NullLiteral
 *	 	BooleanLiteral
 *	 	NumericLiteral
 *	 	StringLiteral
 *
 *	NullLiteral:
 *		tNULL				-- 7.8.1
 *
 *	BooleanLiteral:
 *		tTRUE				-- 7.8.2
 *		tFALSE				-- 7.8.2
 */

static struct node *
Literal_parse(parser)
	struct parser *parser;
{
	struct Literal_node *n;

	/*
	 * Convert the next token into a regular expression
	 * if possible
	 */

	switch (NEXT) {
	case tNULL:
		n = NEW_NODE(struct Literal_node, NODECLASS_Literal);
		SEE_SET_NULL(&n->value);
		SKIP;
		return (struct node *)n;
	case tTRUE:
	case tFALSE:
		n = NEW_NODE(struct Literal_node,  NODECLASS_Literal);
		SEE_SET_BOOLEAN(&n->value, (NEXT == tTRUE));
		SKIP;
		return (struct node *)n;
	case tNUMBER:
		return PARSE(NumericLiteral);
	case tSTRING:
		return PARSE(StringLiteral);
	case tDIV:
	case tDIVEQ:
		SEE_lex_regex(parser->lex);
		return PARSE(RegularExpressionLiteral);
	default:
		EXPECTED("null, true, false, number, string, or regex");
	}
	/* NOTREACHED */
}

/*
 *	NumericLiteral:
 *		tNUMBER				-- 7.8.3
 */
static struct node *
NumericLiteral_parse(parser)
	struct parser *parser;
{
	struct Literal_node *n;

	EXPECT_NOSKIP(tNUMBER);
	n = NEW_NODE(struct Literal_node, NODECLASS_Literal);
	SEE_VALUE_COPY(&n->value, NEXT_VALUE);
	SKIP;
	return (struct node *)n;
}

/*
 *	StringLiteral:
 *		tSTRING				-- 7.8.4
 */
static struct node *
StringLiteral_parse(parser)
	struct parser *parser;
{
	struct StringLiteral_node *n;

	EXPECT_NOSKIP(tSTRING);
	n = NEW_NODE(struct StringLiteral_node, NODECLASS_StringLiteral);
	n->string = NEXT_VALUE->u.string;
	SKIP;
	return (struct node *)n;
}

/*
 *	RegularExpressionLiteral:
 *		tREGEX				-- 7.8.5
 */
static struct node *
RegularExpressionLiteral_parse(parser)
	struct parser *parser;
{
	struct RegularExpressionLiteral_node *n = NULL;
	struct SEE_string *s, *pattern, *flags;
	int p;

	if (NEXT == tREGEX)  {
	    /*
	     * Find the position after the regexp's closing '/'.
	     * i.e. the position of the regexp flags.
	     */
	    s = NEXT_VALUE->u.string;
	    for (p = s->length; p > 0; p--)
		    if (s->data[p-1] == '/')
			    break;
	    SEE_ASSERT(parser->interpreter, p > 1);

	    pattern = SEE_string_substr(parser->interpreter,
		s, 1, p - 2);
	    flags = SEE_string_substr(parser->interpreter,
		s, p, s->length - p);

	    n = NEW_NODE(struct RegularExpressionLiteral_node,
		NODECLASS_RegularExpressionLiteral);
	    SEE_SET_STRING(&n->pattern, pattern);
	    SEE_SET_STRING(&n->flags, flags);
	    n->argv[0] = &n->pattern;
	    n->argv[1] = &n->flags;

	}
	EXPECT(tREGEX);
	return (struct node *)n;
}

/*------------------------------------------------------------
 * -- 11.1
 *
 *	PrimaryExpression
 *	:	tTHIS				-- 11.1.1
 *	|	tIDENT				-- 11.1.2
 *	|	Literal				-- 11.1.3
 *	|	ArrayLiteral
 *	|	ObjectLiteral
 *	|	'(' Expression ')'		-- 11.1.6
 *	;
 */
static struct node *
PrimaryExpression_parse(parser)
	struct parser *parser;
{
	struct node *n;
	struct PrimaryExpression_ident_node *i;

	switch (NEXT) {
	case tTHIS:
		n = NEW_NODE(struct node, NODECLASS_PrimaryExpression_this);
		SKIP;
		return n;
	case tIDENT:
		i = NEW_NODE(struct PrimaryExpression_ident_node,
			NODECLASS_PrimaryExpression_ident);
		i->string = NEXT_VALUE->u.string;
		SKIP;
		return (struct node *)i;
	case '[':
		return PARSE(ArrayLiteral);
	case '{':
		return PARSE(ObjectLiteral);
	case '(':
		SKIP;
		n = PARSE(Expression);
		EXPECT(')');
		return n;
	default:
		return PARSE(Literal);
	}
}

/*
 *	ArrayLiteral				-- 11.1.4
 *	:	'[' ']'
 *	|	'[' Elision ']'
 *	|	'[' ElementList ']'
 *	|	'[' ElementList ',' ']'
 *	|	'[' ElementList ',' Elision ']'
 *	;
 *
 *	ElementList
 *	:	Elision AssignmentExpression
 *	|	AssignmentExpression
 *	|	ElementList ',' Elision AssignmentExpression
 *	|	ElementList ',' AssignmentExpression
 *	;
 *
 *	Elision
 *	:	','
 *	|	Elision ','
 *	;
 *
 * NB: I ignore the above elision nonsense and just build a list of
 * (index,expr) nodes with an overall length. It is equivalent 
 * to that in the standard.
 */
static struct node *
ArrayLiteral_parse(parser)
	struct parser *parser;
{
	struct ArrayLiteral_node *n;
	struct ArrayLiteral_element **elp;
	int index;

	n = NEW_NODE(struct ArrayLiteral_node,
	    NODECLASS_ArrayLiteral);
	elp = &n->first;

	EXPECT('[');
	index = 0;
	while (NEXT != ']')
		if (NEXT == ',') {
			index++;
			SKIP;
		} else {
			*elp = SEE_NEW(parser->interpreter,
			    struct ArrayLiteral_element);
			(*elp)->index = index;
			(*elp)->expr = PARSE(AssignmentExpression);
			elp = &(*elp)->next;
			index++;
			if (NEXT != ']')
				EXPECTX(',', "',' or ']'");
		}
	n->length = index;
	*elp = NULL;
	EXPECT(']');
	return (struct node *)n;
}

/*
 *	ObjectLiteral				-- 11.1.5
 *	:	'{' '}'
 *	|	'{' PropertyNameAndValueList '}'
 *	;
 *
 *	PropertyNameAndValueList
 *	:	PropertyName ':' AssignmentExpression
 *	|	PropertyNameAndValueList ',' PropertyName ':' 
 *							AssignmentExpression
 *	;
 *
 *	PropertyName
 *	:	tIDENT
 *	|	StringLiteral
 *	|	NumericLiteral
 *	;
 */
static struct node *
ObjectLiteral_parse(parser)
	struct parser *parser;
{
	struct ObjectLiteral_node *n;
	struct ObjectLiteral_pair **pairp;
	struct SEE_value sv;
	struct SEE_interpreter *interp = parser->interpreter;

	n = NEW_NODE(struct ObjectLiteral_node,
			NODECLASS_ObjectLiteral);
	pairp = &n->first;

	EXPECT('{');
	while (NEXT != '}') {
	    *pairp = SEE_NEW(interp, struct ObjectLiteral_pair);
	    switch (NEXT) {
	    case tIDENT:
	    case tSTRING:
		(*pairp)->name = SEE_intern(interp, NEXT_VALUE->u.string);
		SKIP;
		break;
	    case tNUMBER:
		SEE_ToString(parser->interpreter, NEXT_VALUE, &sv);
		(*pairp)->name = SEE_intern(interp, sv.u.string);
		SKIP;
		break;
	    default:
		EXPECTED("string, identifier or number");
	    }
	    EXPECT(':');
	    (*pairp)->value = PARSE(AssignmentExpression);
	    if (NEXT != '}') {
		    EXPECTX(',', "',' or '}'"); 
                    /*
                     * A comma before the closing brace is permitted
                     * by netscape, but not by JScript or ECMA-262-3.
                     */
                    if (!SEE_COMPAT_JS(parser->interpreter, >=, JS11))
                        if (NEXT == '}')
                            EXPECTED("string, identifier or number after ','");
	    }
	    pairp = &(*pairp)->next;
	}
	*pairp = NULL;
	EXPECT('}');
	return (struct node *)n;
}

/*
 *	-- 11.2
 *
 *	MemberExpression
 *	:	PrimaryExpression
 *	|	FunctionExpression				-- 11.2.5
 *	|	MemberExpression '[' Expression ']'		-- 11.2.1
 *	|	MemberExpression '.' tIDENT			-- 11.2.1
 *	|	tNEW MemberExpression Arguments			-- 11.2.2
 *	;
 *
 *	NewExpression
 *	:	MemberExpression
 *	|	tNEW NewExpression				-- 11.2.2
 *	;
 *
 *	CallExpression
 *	:	MemberExpression Arguments			-- 11.2.3
 *	|	CallExpression Arguments			-- 11.2.3
 *	|	CallExpression '[' Expression ']'		-- 11.2.1
 *	|	CallExpression '.' tIDENT			-- 11.2.1
 *	;
 *
 *	Arguments
 *	:	'(' ')'						-- 11.2.4
 *	|	'(' ArgumentList ')'				-- 11.2.4
 *	;
 *
 *	ArgumentList
 *	:	AssignmentExpression				-- 11.2.4
 *	|	ArgumentList ',' AssignmentExpression		-- 11.2.4
 *	;
 *
 *	LeftHandSideExpression
 *	:	NewExpression
 *	|	CallExpression
 *	;
 *
 * NOTE:  The standard grammar is complicated in order to resolve an 
 *        ambiguity in parsing 'new expr ( args )' as either
 *	  '(new  expr)(args)' or as 'new (expr(args))'. In fact, 'new'
 *	  is acting as both a unary and a binary operator. Yucky.
 *
 *	  Since recursive descent is single-token lookahead, we
 *	  can rewrite the above as the following equivalent grammar:
 *
 *	MemberExpression
 *	:	PrimaryExpression
 *	|	FunctionExpression		    -- lookahead == tFUNCTION
 *	|	MemberExpression '[' Expression ']'
 *	|	MemberExpression '.' tIDENT
 *	|	tNEW MemberExpression Arguments	    -- lookahead == tNEW
 *	|	tNEW MemberExpression 	            -- lookahead == tNEW
 *
 *	LeftHandSideExpression
 *	:	PrimaryExpression
 *	|	FunctionExpression		    -- lookahead == tFUNCTION
 *	|	LeftHandSideExpression '[' Expression ']'
 *	|	LeftHandSideExpression '.' tIDENT
 *	|	LeftHandSideExpression Arguments
 *	|	MemberExpression		    -- lookahead == tNEW
 *
 */


static struct Arguments_node *
Arguments_parse(parser)
	struct parser *parser;
{
	struct Arguments_node *n;
	struct Arguments_arg **argp;

	n = NEW_NODE(struct Arguments_node,
			NODECLASS_Arguments);
	argp = &n->first;
	n->argc = 0;

	EXPECT('(');
	while (NEXT != ')') {
		n->argc++;
		*argp = SEE_NEW(parser->interpreter, struct Arguments_arg);
		(*argp)->expr = PARSE(AssignmentExpression);
		argp = &(*argp)->next;
		if (NEXT != ')')
			EXPECTX(',', "',' or ')'");
	}
	*argp = NULL;
	EXPECT(')');
	return n;
}


/* 11.2.1 */
static struct node *
MemberExpression_parse(parser)
	struct parser *parser;
{
	struct node *n;
	struct MemberExpression_new_node *m;
	struct MemberExpression_dot_node *dn;
	struct MemberExpression_bracket_node *bn;

	switch (NEXT) {
        case tFUNCTION:
	    n = PARSE(FunctionExpression);
	    break;
        case tNEW:
	    m = NEW_NODE(struct MemberExpression_new_node,
	    	NODECLASS_MemberExpression_new);
	    SKIP;
	    m->mexp = PARSE(MemberExpression);
	    if (NEXT == '(')
		m->args = PARSE(Arguments);
	    else
		m->args = NULL;
	    n = (struct node *)m;
	    break;
	default:
	    n = PARSE(PrimaryExpression);
	}

	for (;;)
	    switch (NEXT) {
	    case '.':
		dn = NEW_NODE(struct MemberExpression_dot_node,
			NODECLASS_MemberExpression_dot);
		SKIP;
		if (NEXT == tIDENT) {
		    dn->mexp = n;
		    dn->name = NEXT_VALUE->u.string;
		    n = (struct node *)dn;
		}
	        EXPECT(tIDENT);
		break;
	    case '[':
		bn = NEW_NODE(struct MemberExpression_bracket_node,
			NODECLASS_MemberExpression_bracket);
		SKIP;
		bn->mexp = n;
		bn->name = PARSE(Expression);
		n = (struct node *)bn;
		EXPECT(']');
		break;
	    default:
		return n;
	    }
}

static struct node *
LeftHandSideExpression_parse(parser)
	struct parser *parser;
{
	struct node *n;
	struct CallExpression_node *cn;
	struct MemberExpression_dot_node *dn;
	struct MemberExpression_bracket_node *bn;

	switch (NEXT) {
        case tFUNCTION:
	    n = PARSE(FunctionExpression);	/* 11.2.5 */
	    break;
        case tNEW:
	    n = PARSE(MemberExpression);
	    break;
	default:
	    n = PARSE(PrimaryExpression);
	}

	for (;;)  {

#ifndef NDEBUG
	    if (SEE_parse_debug)
	        dprintf("LeftHandSideExpression: islhs = %d next is %s\n",
		    parser->is_lhs, SEE_tokenname(NEXT));
#endif

	    switch (NEXT) {
	    case '.':
	        dn = NEW_NODE(struct MemberExpression_dot_node,
		    NODECLASS_MemberExpression_dot);
		SKIP;
		if (NEXT == tIDENT) {
		    dn->mexp = n;
		    dn->name = NEXT_VALUE->u.string;
		    n = (struct node *)dn;
		}
	        EXPECT(tIDENT);
		break;
	    case '[':
		bn = NEW_NODE(struct MemberExpression_bracket_node,
			NODECLASS_MemberExpression_bracket);
		SKIP;
		bn->mexp = n;
		bn->name = PARSE(Expression);
		n = (struct node *)bn;
		EXPECT(']');
		break;
	    case '(':
		cn = NEW_NODE(struct CallExpression_node,
			NODECLASS_CallExpression);
		cn->exp = n;
		cn->args = PARSE(Arguments);
		n = (struct node *)cn;
		break;
	    default:
		/* Eventually we leave via this clause */
		parser->is_lhs = 1;
		return n;
	    }
	}
}

/*
 *	-- 11.3
 *
 *	PostfixExpression
 *	:	LeftHandSideExpression
 *	|	LeftHandSideExpression { NOLINETERM; } tPLUSPLUS    -- 11.3.1
 *	|	LeftHandSideExpression { NOLINETERM; } tMINUSMINUS  -- 11.3.2
 *	;
 */

static struct node *
PostfixExpression_parse(parser)
	struct parser *parser;
{
	struct node *n;
	struct Unary_node *pen;

	n = PARSE(LeftHandSideExpression);
	if (!NEXT_FOLLOWS_NL && 
	    (NEXT == tPLUSPLUS || NEXT == tMINUSMINUS))
	{
		pen = NEW_NODE(struct Unary_node,
			NEXT == tPLUSPLUS
			    ? NODECLASS_PostfixExpression_inc
			    : NODECLASS_PostfixExpression_dec);
		pen->a = n;
		n = (struct node *)pen;
		SKIP;
		parser->is_lhs = 0;
	}
	return n;
}

/*
 *	-- 11.4
 *
 *	UnaryExpression
 *	:	PostfixExpression
 *	|	tDELETE UnaryExpression				-- 11.4.1
 *	|	tVOID UnaryExpression				-- 11.4.2
 *	|	tTYPEOF UnaryExpression				-- 11.4.3
 *	|	tPLUSPLUS UnaryExpression			-- 11.4.4
 *	|	tMINUSMINUS UnaryExpression			-- 11.4.5
 *	|	'+' UnaryExpression				-- 11.4.6
 *	|	'-' UnaryExpression				-- 11.4.7
 *	|	'~' UnaryExpression				-- 11.4.8
 *	|	'!' UnaryExpression				-- 11.4.9
 *	;
 */
static struct node *
UnaryExpression_parse(parser)
	struct parser *parser;
{
	struct Unary_node *n;
	enum nodeclass_enum nc;

	switch (NEXT) {
	case tDELETE:
		nc = NODECLASS_UnaryExpression_delete;
		break;
	case tVOID:
		nc = NODECLASS_UnaryExpression_void;
		break;
	case tTYPEOF:
		nc = NODECLASS_UnaryExpression_typeof;
		break;
	case tPLUSPLUS:
		nc = NODECLASS_UnaryExpression_preinc;
		break;
	case tMINUSMINUS:
		nc = NODECLASS_UnaryExpression_predec;
		break;
	case '+':
		nc = NODECLASS_UnaryExpression_plus;
		break;
	case '-':
		nc = NODECLASS_UnaryExpression_minus;
		break;
	case '~':
		nc = NODECLASS_UnaryExpression_inv;
		break;
	case '!':
		nc = NODECLASS_UnaryExpression_not;
		break;
	default:
		return PARSE(PostfixExpression);
	}
	n = NEW_NODE(struct Unary_node, nc);
	SKIP;
	n->a = PARSE(UnaryExpression);
	parser->is_lhs = 0;
	return (struct node *)n;
}

/*
 *	-- 11.5
 *
 *	MultiplicativeExpression
 *	:	UnaryExpression
 *	|	MultiplicativeExpression '*' UnaryExpression	-- 11.5.1
 *	|	MultiplicativeExpression '/' UnaryExpression	-- 11.5.2
 *	|	MultiplicativeExpression '%' UnaryExpression	-- 11.5.3
 *	;
 */

static struct node *
MultiplicativeExpression_parse(parser)
	struct parser *parser;
{
	struct node *n;
	enum nodeclass_enum nc;
	struct Binary_node *m;

	n = PARSE(UnaryExpression);
	for (;;) {
	    /* Left-to-right associative */
	    switch (NEXT) {
	    case '*':
		nc = NODECLASS_MultiplicativeExpression_mul;
		break;
	    case '/':
		nc = NODECLASS_MultiplicativeExpression_div;
		break;
	    case '%':
		nc = NODECLASS_MultiplicativeExpression_mod;
		break;
	    default:
		return n;
	    }
	    SKIP;
	    m = NEW_NODE(struct Binary_node, nc);
	    m->a = n;
	    m->b = PARSE(UnaryExpression);
	    parser->is_lhs = 0;
	    n = (struct node *)m;
	}
}

/*
 *	-- 11.6
 *
 *	AdditiveExpression
 *	:	MultiplicativeExpression
 *	|	AdditiveExpression '+' MultiplicativeExpression	-- 11.6.1
 *	|	AdditiveExpression '-' MultiplicativeExpression	-- 11.6.2
 *	;
 */
static struct node *
AdditiveExpression_parse(parser)
	struct parser *parser;
{
	struct node *n;
	enum nodeclass_enum nc;
	struct Binary_node *m;

	n = PARSE(MultiplicativeExpression);
	for (;;) {
	    switch (NEXT) {
	    case '+':
		nc = NODECLASS_AdditiveExpression_add;
		break;
	    case '-':
		nc = NODECLASS_AdditiveExpression_sub;
		break;
	    default:
		return n;
	    }
	    parser->is_lhs = 0;
	    SKIP;
	    m = NEW_NODE(struct Binary_node, nc);
	    m->a = n;
	    m->b = PARSE(MultiplicativeExpression);
	    n = (struct node *)m;
	}
	return n;
}

/*
 *	-- 11.7
 *
 *	ShiftExpression
 *	:	AdditiveExpression
 *	|	ShiftExpression tLSHIFT AdditiveExpression	-- 11.7.1
 *	|	ShiftExpression tRSHIFT AdditiveExpression	-- 11.7.2
 *	|	ShiftExpression tURSHIFT AdditiveExpression	-- 11.7.3
 *	;
 */
static struct node *
ShiftExpression_parse(parser)
	struct parser *parser;
{
	struct node *n;
	enum nodeclass_enum nc;
	struct Binary_node *sn;

	n = PARSE(AdditiveExpression);
	for (;;) {
	    /* Left associative */
	    switch (NEXT) {
	    case tLSHIFT:
		nc = NODECLASS_ShiftExpression_lshift;
		break;
	    case tRSHIFT:
		nc = NODECLASS_ShiftExpression_rshift;
		break;
	    case tURSHIFT:
		nc = NODECLASS_ShiftExpression_urshift;
		break;
	    default:
		return n;
	    }
	    sn = NEW_NODE(struct Binary_node, nc);
	    SKIP;
	    sn->a = n;
	    sn->b = PARSE(AdditiveExpression);
	    parser->is_lhs = 0;
	    n = (struct node *)sn;
	}
}

/*
 *	-- 11.8
 *
 *	RelationalExpression
 *	:	ShiftExpression
 *	|	RelationalExpression '<' ShiftExpression	 -- 11.8.1
 *	|	RelationalExpression '>' ShiftExpression	 -- 11.8.2
 *	|	RelationalExpression tLE ShiftExpression	 -- 11.8.3
 *	|	RelationalExpression tGE ShiftExpression	 -- 11.8.4
 *	|	RelationalExpression tINSTANCEOF ShiftExpression -- 11.8.6
 *	|	RelationalExpression tIN ShiftExpression	 -- 11.8.7
 *	;
 *
 *	RelationalExpressionNoIn
 *	:	ShiftExpression
 *	|	RelationalExpressionNoIn '<' ShiftExpression	 -- 11.8.1
 *	|	RelationalExpressionNoIn '>' ShiftExpression	 -- 11.8.2
 *	|	RelationalExpressionNoIn tLE ShiftExpression	 -- 11.8.3
 *	|	RelationalExpressionNoIn tGE ShiftExpression	 -- 11.8.4
 *	|	RelationalExpressionNoIn tINSTANCEOF ShiftExpression -- 11.8.6
 *	;
 *
 * The *NoIn productions are implemented by the 'noin' boolean field
 * in the parser state.
 */

static struct node *
RelationalExpression_parse(parser)
	struct parser *parser;
{
	struct node *n;
	enum nodeclass_enum nc;
	struct Binary_node *rn;

	n = PARSE(ShiftExpression);
	for (;;) {
	    /* Left associative */
	    switch (NEXT) {
	    case '<':
		nc = NODECLASS_RelationalExpression_lt;
		break;
	    case '>':
		nc = NODECLASS_RelationalExpression_gt;
		break;
	    case tLE:
		nc = NODECLASS_RelationalExpression_le;
		break;
	    case tGE:
		nc = NODECLASS_RelationalExpression_ge;
		break;
	    case tINSTANCEOF:
		nc = NODECLASS_RelationalExpression_instanceof;
		break;
	    case tIN:
		if (!parser->noin) {
		    nc = NODECLASS_RelationalExpression_in;
		    break;
		} /* else Fallthrough */
	    default:
		return n;
	    }
	    rn = NEW_NODE(struct Binary_node, nc);
	    SKIP;
	    rn->a = n;
	    rn->b = PARSE(RelationalExpression);
	    parser->is_lhs = 0;
	    n = (struct node *)rn;
	}
}

/*
 *	-- 11.9
 *
 *	EqualityExpression
 *	:	RelationalExpression
 *	|	EqualityExpression tEQ RelationalExpression	-- 11.9.1
 *	|	EqualityExpression tNE RelationalExpression	-- 11.9.2
 *	|	EqualityExpression tSEQ RelationalExpression	-- 11.9.4
 *	|	EqualityExpression tSNE RelationalExpression	-- 11.9.5
 *	;
 *
 *	EqualityExpressionNoIn
 *	:	RelationalExpressionNoIn
 *	|	EqualityExpressionNoIn tEQ RelationalExpressionNoIn  -- 11.9.1
 *	|	EqualityExpressionNoIn tNE RelationalExpressionNoIn  -- 11.9.2
 *	|	EqualityExpressionNoIn tSEQ RelationalExpressionNoIn -- 11.9.4
 *	|	EqualityExpressionNoIn tSNE RelationalExpressionNoIn -- 11.9.5
 *	;
 */

static struct node *
EqualityExpression_parse(parser)
	struct parser *parser;
{
	struct node *n;
	enum nodeclass_enum nc;
	struct Binary_node *rn;

	n = PARSE(RelationalExpression);
	for (;;) {
	    /* Left associative */
	    switch (NEXT) {
	    case tEQ:
		nc = NODECLASS_EqualityExpression_eq;
		break;
	    case tNE:
		nc = NODECLASS_EqualityExpression_ne;
		break;
	    case tSEQ:
		nc = NODECLASS_EqualityExpression_seq;
		break;
	    case tSNE:
		nc = NODECLASS_EqualityExpression_sne;
		break;
	    default:
		return n;
	    }
	    rn = NEW_NODE(struct Binary_node, nc);
	    SKIP;
	    rn->a = n;
	    rn->b = PARSE(EqualityExpression);
	    parser->is_lhs = 0;
	    n = (struct node *)rn;
	}
}

/*
 *	-- 11.10
 *
 *	BitwiseANDExpression
 *	:	EqualityExpression
 *	|	BitwiseANDExpression '&' EqualityExpression
 *	;
 *
 *	BitwiseANDExpressionNoIn
 *	:	EqualityExpressionNoIn
 *	|	BitwiseANDExpressionNoIn '&' EqualityExpressionNoIn
 *	;
 */
static struct node *
BitwiseANDExpression_parse(parser)
	struct parser *parser;
{
	struct node *n;
	struct Binary_node *m;

	n = PARSE(EqualityExpression);
	if (NEXT != '&') 
		return n;
	m = NEW_NODE(struct Binary_node,
			NODECLASS_BitwiseANDExpression);
	SKIP;
	m->a = n;
	m->b = PARSE(BitwiseANDExpression);
	parser->is_lhs = 0;
	return (struct node *)m;
}

/*
 *	BitwiseXORExpression
 *	:	BitwiseANDExpression
 *	|	BitwiseXORExpression '^' BitwiseANDExpression
 *	;
 *
 *	BitwiseXORExpressionNoIn
 *	:	BitwiseANDExpressionNoIn
 *	|	BitwiseXORExpressionNoIn '^' BitwiseANDExpressionNoIn
 *	;
 */
static struct node *
BitwiseXORExpression_parse(parser)
	struct parser *parser;
{
	struct node *n;
	struct Binary_node *m;

	n = PARSE(BitwiseANDExpression);
	if (NEXT != '^') 
		return n;
	m = NEW_NODE(struct Binary_node,
			NODECLASS_BitwiseXORExpression);
	SKIP;
	m->a = n;
	m->b = PARSE(BitwiseXORExpression);
	parser->is_lhs = 0;
	return (struct node *)m;
}

/*
 *	BitwiseORExpression
 *	:	BitwiseXORExpression
 *	|	BitwiseORExpression '|' BitwiseXORExpression
 *	;
 *
 *	BitwiseORExpressionNoIn
 *	:	BitwiseXORExpressionNoIn
 *	|	BitwiseORExpressionNoIn '|' BitwiseXORExpressionNoIn
 *	;
 */
static struct node *
BitwiseORExpression_parse(parser)
	struct parser *parser;
{
	struct node *n;
	struct Binary_node *m;

	n = PARSE(BitwiseXORExpression);
	if (NEXT != '|') 
		return n;
	m = NEW_NODE(struct Binary_node,
			NODECLASS_BitwiseORExpression);
	SKIP;
	m->a = n;
	m->b = PARSE(BitwiseORExpression);
	parser->is_lhs = 0;
	return (struct node *)m;
}

/*
 *	-- 11.11
 *
 *	LogicalANDExpression
 *	:	BitwiseORExpression
 *	|	LogicalANDExpression tANDAND BitwiseORExpression
 *	;
 *
 *	LogicalANDExpressionNoIn
 *	:	BitwiseORExpressionNoIn
 *	|	LogicalANDExpressionNoIn tANDAND BitwiseORExpressionNoIn
 *	;
 */

/* Executes a known-constant subtree to yield its value. */
void
_SEE_const_evaluate(node, interp, res)
	struct node *node;
	struct SEE_interpreter *interp;
	struct SEE_value *res;
{
	void *body;
	struct SEE_context const_context;

#ifndef NDEBUG
	if (SEE_parse_debug) {
	    dprintf("const_evaluate: evaluating (");
#if WITH_PARSER_PRINT
            _SEE_parser_print(
                _SEE_parser_print_stdio_new(interp, stderr),
                node);
#else
	    dprintf("%p", node);
#endif
	    dprintf(")\n");
	}
#endif
	
	body = make_body(interp, 
	    FunctionBody_make(interp, 
	      SourceElements_make1(interp, 
	        ExpressionStatement_make(interp, node)), 1),
	    NO_CONST);

	/* A dummy context with the minimum we can get away with */
	memset(&const_context, 0, sizeof const_context);
	const_context.interpreter = interp;

	eval_functionbody(body, &const_context, res);

#ifndef NDEBUG
	if (SEE_parse_debug) {
	    dprintf("const_evaluate: result is ");
	    dprintv(interp, res);
	    dprintf("\n");
	}
#endif
}

static struct node *
LogicalANDExpression_parse(parser)
	struct parser *parser;
{
	struct node *n;
	struct Binary_node *m;

	n = PARSE(BitwiseORExpression);
	if (NEXT != tANDAND) 
		return n;
	m = NEW_NODE(struct Binary_node,
			NODECLASS_LogicalANDExpression);
	SKIP;
	m->a = n;
	m->b = PARSE(LogicalANDExpression);
	parser->is_lhs = 0;
	return (struct node *)m;
}

/*
 *	LogicalORExpression
 *	:	LogicalANDExpression
 *	|	LogicalORExpression tOROR LogicalANDExpression
 *	;
 *
 *	LogicalORExpressionNoIn
 *	:	LogicalANDExpressionNoIn
 *	|	LogicalORExpressionNoIn tOROR LogicalANDExpressionNoIn
 *	;
 */

static struct node *
LogicalORExpression_parse(parser)
	struct parser *parser;
{
	struct node *n;
	struct Binary_node *m;

	n = PARSE(LogicalANDExpression);
	if (NEXT != tOROR) 
		return n;
	m = NEW_NODE(struct Binary_node,
			NODECLASS_LogicalORExpression);
	SKIP;
	m->a = n;
	m->b = PARSE(LogicalORExpression);
	parser->is_lhs = 0;
	return (struct node *)m;
}

/*
 *	-- 11.12
 *
 *	ConditionalExpression
 *	:	LogicalORExpression
 *	|	LogicalORExpression '?' 
 *			AssignmentExpression ':' AssignmentExpression
 *	;
 *
 *	ConditionalExpressionNoIn
 *	:	LogicalORExpressionNoIn
 *	|	LogicalORExpressionNoIn '?' 
 *			AssignmentExpressionNoIn ':' AssignmentExpressionNoIn
 *	;
 */

static struct node *
ConditionalExpression_parse(parser)
	struct parser *parser;
{
	struct node *n;
	struct ConditionalExpression_node *m;

	n = PARSE(LogicalORExpression);
	if (NEXT != '?') 
		return n;
	m = NEW_NODE(struct ConditionalExpression_node,
			NODECLASS_ConditionalExpression);
	SKIP;
	m->a = n;
	m->b = PARSE(AssignmentExpression);
	EXPECT(':');
	m->c = PARSE(AssignmentExpression);
	parser->is_lhs = 0;
	return (struct node *)m;
}

/*
 *	-- 11.13
 *
 *	AssignmentExpression
 *	:	ConditionalExpression
 *	|	LeftHandSideExpression AssignmentOperator AssignmentExpression
 *	;
 *
 *	AssignmentExpressionNoIn
 *	:	ConditionalExpressionNoIn
 *	|	LeftHandSideExpression AssignmentOperator 
 *						AssignmentExpressionNoIn
 *	;
 *
 *	AssignmentOperator
 *	:	'='				-- 11.13.1
 *	|	tSTAREQ				-- 11.13.2
 *	|	tDIVEQ
 *	|	tMODEQ
 *	|	tPLUSEQ
 *	|	tMINUSEQ
 *	|	tLSHIFTEQ
 *	|	tRSHIFTEQ
 *	|	tURSHIFTEQ
 *	|	tANDEQ
 *	|	tXOREQ
 *	|	tOREQ
 *	;
 */
static struct node *
AssignmentExpression_parse(parser)
	struct parser *parser;
{
	struct node *n;
	enum nodeclass_enum nc;
	struct AssignmentExpression_node *an;

	/*
	 * If, while recursing we parse LeftHandSideExpression,
	 * then is_lhs will be set to 1. Otherwise, it is just a 
	 * ConditionalExpression, and we cannot derive the second
	 * production in this rule. So we just return.
	 */
	n = PARSE(ConditionalExpression);
	if (!parser->is_lhs)
		return n;

	switch (NEXT) {
	case '=':
		nc = NODECLASS_AssignmentExpression_simple;
		break;
	case tSTAREQ:
		nc = NODECLASS_AssignmentExpression_muleq;
		break;
	case tDIVEQ:
		nc = NODECLASS_AssignmentExpression_diveq;
		break;
	case tMODEQ:
		nc = NODECLASS_AssignmentExpression_modeq;
		break;
	case tPLUSEQ:
		nc = NODECLASS_AssignmentExpression_addeq;
		break;
	case tMINUSEQ:
		nc = NODECLASS_AssignmentExpression_subeq;
		break;
	case tLSHIFTEQ:
		nc = NODECLASS_AssignmentExpression_lshifteq;
		break;
	case tRSHIFTEQ:
		nc = NODECLASS_AssignmentExpression_rshifteq;
		break;
	case tURSHIFTEQ:
		nc = NODECLASS_AssignmentExpression_urshifteq;
		break;
	case tANDEQ:
		nc = NODECLASS_AssignmentExpression_andeq;
		break;
	case tXOREQ:
		nc = NODECLASS_AssignmentExpression_xoreq;
		break;
	case tOREQ:
		nc = NODECLASS_AssignmentExpression_oreq;
		break;
	default:
		return n;
	}
	an = NEW_NODE(struct AssignmentExpression_node, nc);
	an->lhs = n;
	SKIP;
	an->expr = PARSE(AssignmentExpression);
	parser->is_lhs = 0;
	return (struct node *)an;
}

/*
 *	-- 11.14
 *
 *	Expression
 *	:	AssignmentExpression
 *	|	Expression ',' AssignmentExpression
 *	;
 *
 *	ExpressionNoIn
 *	:	AssignmentExpressionNoIn
 *	|	ExpressionNoIn ',' AssignmentExpressionNoIn
 *	;
 *
 * Codgen notes:
 * All Expression nodes leave a val on the stack; i.e.   - | val
 *
 */
static struct node *
Expression_parse(parser)
	struct parser *parser;
{
	struct node *n;
	struct Binary_node *cn;

	n = PARSE(AssignmentExpression);
	if (NEXT != ',')
		return n;
	cn = NEW_NODE(struct Binary_node, NODECLASS_Expression_comma);
	SKIP;
	cn->a = n;
	cn->b = PARSE(Expression);
	parser->is_lhs = 0;
	return (struct node *)cn;
}

/*
 *
 * -- 12
 *
 *	Statement
 *	:	Block
 *	|	VariableStatement
 *	|	EmptyStatement
 *	|	ExpressionStatement
 *	|	IfStatement
 *	|	IterationStatement
 *	|	ContinueStatement
 *	|	BreakStatement
 *	|	ReturnStatement
 *	|	WithStatement
 *	|	LabelledStatement
 *	|	SwitchStatement
 *	|	ThrowStatement
 *	|	TryStatement
 *	;
 *
 * Codegen note:
 *   All statements expect to have a NORMAL completion value on the stack
 *   already. At the end of the statement, the value is replaced as needed.
 */

static struct node *
Statement_parse(parser)
	struct parser *parser;
{
	struct node *n;

	parser->current_labelset = NULL;

	switch (NEXT) {
	case '{':
		return PARSE(Block);
	case tVAR:
		return PARSE(VariableStatement);
	case ';':
		return PARSE(EmptyStatement);
	case tIF:
		return PARSE(IfStatement);
	case tDO:
	case tWHILE:
	case tFOR:
		n = PARSE(IterationStatement);
		return n;
	case tCONTINUE:
		return PARSE(ContinueStatement);
	case tBREAK:
		return PARSE(BreakStatement);
	case tRETURN:
		return PARSE(ReturnStatement);
	case tWITH:
		return PARSE(WithStatement);
	case tSWITCH:
		n = PARSE(SwitchStatement);
		return n;
	case tTHROW:
		return PARSE(ThrowStatement);
	case tTRY:
		return PARSE(TryStatement);
	case tFUNCTION:
		/* Conditional functions for JS1.5 compatibility */
		if (SEE_COMPAT_JS(parser->interpreter, >=, JS15) &&
		    lookahead(parser, 1) != '(')
			return PARSE(FunctionStatement);
		ERRORm("function keyword not allowed here");
	case tIDENT:
		if (lookahead(parser, 1) == ':')
			return PARSE(LabelledStatement);
		/* FALLTHROUGH */
	default:
		return PARSE(ExpressionStatement);
	}
}

/*
 *	-- 12.1
 *
 *	Block
 *	:	'{' '}'					-- 12.1
 *	|	'{' StatementList '}'			-- 12.1
 *	;
 */
static struct node *
Block_parse(parser)
	struct parser *parser;
{
	struct node *n;

	EXPECT('{');
	if (NEXT == '}')
		n = NEW_NODE(struct node, NODECLASS_Block_empty);
	else
		n = PARSE(StatementList);
	EXPECT('}');
	return n;
}

/*
 *	StatementList
 *	:	Statement				-- 12.1
 *	|	StatementList Statement			-- 12.1
 *	;
 */
static struct node *
StatementList_parse(parser)
	struct parser *parser;
{
	struct node *n;
	struct Binary_node *ln;

	n = PARSE(Statement);
	switch (NEXT) {
	case tFUNCTION:
	    if (SEE_COMPAT_JS(parser->interpreter, >=, JS15))
		break;
	    /* else fallthrough */
	case '}':
	case tEND:
	case tCASE:
	case tDEFAULT:
		return n;
	}
	ln = NEW_NODE(struct Binary_node, NODECLASS_StatementList);
	ln->a = n;
	ln->b = PARSE(StatementList);
	return (struct node *)ln;
}

/*
 *	-- 12.2
 *
 *	VariableStatement
 *	:	tVAR VariableDeclarationList ';'
 *	;
 *
 *	VariableDeclarationList
 *	:	VariableDeclaration
 *	|	VariableDeclarationList ',' VariableDeclaration
 *	;
 *
 *	VariableDeclarationListNoIn
 *	:	VariableDeclarationNoIn
 *	|	VariableDeclarationListNoIn ',' VariableDeclarationNoIn
 *	;
 *
 *	VariableDeclaration
 *	:	tIDENT
 *	|	tIDENT Initialiser
 *	;
 *
 *	VariableDeclarationNoIn
 *	:	tIDENT
 *	|	tIDENT InitialiserNoIn
 *	;
 *
 *	Initialiser
 *	:	'=' AssignmentExpression
 *	;
 *
 *	InitialiserNoIn
 *	:	'=' AssignmentExpressionNoIn
 *	;
 */
static struct node *
VariableStatement_parse(parser)
	struct parser *parser;
{
	struct Unary_node *n;

	n = NEW_NODE(struct Unary_node, NODECLASS_VariableStatement);
	EXPECT(tVAR);
	n->a = PARSE(VariableDeclarationList);
	EXPECT_SEMICOLON;
	return (struct node *)n;
}

static struct node *
VariableDeclarationList_parse(parser)
	struct parser *parser;
{
	struct node *n;
	struct Binary_node *ln;

	n = PARSE(VariableDeclaration);
	if (NEXT != ',') 
		return n;
	ln = NEW_NODE(struct Binary_node, NODECLASS_VariableDeclarationList);
	SKIP;
	/* NB: IterationStatement_parse() also constructs a VarDeclList */
	ln->a = n;
	ln->b = PARSE(VariableDeclarationList);
	return (struct node *)ln;
}




/*
 * Note: All declared vars end up attached to a function body's vars
 * list, and are set to undefined upon entry to that function.
 * See also:
 *	SEE_function_put_args()		- put args
 *	FunctionDeclaration_fproc()	- put func decls
 *	SourceElements_fproc()		- put vars
 */


static struct node *
VariableDeclaration_parse(parser)
	struct parser *parser;
{
	struct VariableDeclaration_node *v;

	v = NEW_NODE(struct VariableDeclaration_node, 
		NODECLASS_VariableDeclaration);
        v->var = SEE_NEW(parser->interpreter, struct var);
	if (NEXT == tIDENT)
		v->var->name = NEXT_VALUE->u.string;
	EXPECT(tIDENT);
	if (NEXT == '=') {
		SKIP;
		v->init = PARSE(AssignmentExpression);
	} else
		v->init = NULL;

	/* Record declared variables */
	if (parser->vars) {
		*parser->vars = v->var;
		parser->vars = &v->var->next;
	}

	return (struct node *)v;
}

/*
 *	-- 12.3
 *
 *	EmptyStatement
 *	:	';'
 *	;
 */
static struct node *
EmptyStatement_parse(parser)
	struct parser *parser;
{
	struct node *n;

	n = NEW_NODE(struct node, NODECLASS_EmptyStatement);
	EXPECT_SEMICOLON;
	return n;
}

/*
 *	-- 12.4
 *
 *	ExpressionStatement
 *	:	Expression ';'		-- lookahead != '{' or tFUNCTION
 *	;
 */
static struct node *
ExpressionStatement_make(interp, node)
	struct SEE_interpreter *interp;
	struct node *node;
{
	struct Unary_node *n;

	n = NEW_NODE_INTERNAL(interp, struct Unary_node, 
	    NODECLASS_ExpressionStatement);
	n->a = node;
	return (struct node *)n;
}

static struct node *
ExpressionStatement_parse(parser)
	struct parser *parser;
{
        struct Unary_node *n;

	n = NEW_NODE(struct Unary_node, NODECLASS_ExpressionStatement);
	n->a = PARSE(Expression);
	EXPECT_SEMICOLON;
	return (struct node *)n;
}


/*
 *	-- 12.5
 *
 *	IfStatement
 *	:	tIF '(' Expression ')' Statement tELSE Statement
 *	|	tIF '(' Expression ')' Statement
 *	;
 */
static struct node *
IfStatement_parse(parser)
	struct parser *parser;
{
	struct node *cond, *btrue, *bfalse;
	struct IfStatement_node *n;

	n = NEW_NODE(struct IfStatement_node, NODECLASS_IfStatement);
	EXPECT(tIF);
	EXPECT('(');
	cond = PARSE(Expression);
	EXPECT(')');
	btrue = PARSE(Statement);
	if (NEXT != tELSE)
		bfalse = NULL;
	else {
		SKIP; /* 'else' */
		bfalse = PARSE(Statement);
	}
	n->cond = cond;
	n->btrue = btrue;
	n->bfalse = bfalse;
	return (struct node *)n;
}

/*
 *	-- 12.6
 *	IterationStatement
 *	:	tDO Statement tWHILE '(' Expression ')' ';'	-- 12.6.1
 *	|	tWHILE '(' Expression ')' Statement		-- 12.6.2
 *	|	tFOR '(' ';' ';' ')' Statement
 *	|	tFOR '(' ExpressionNoIn ';' ';' ')' Statement
 *	|	tFOR '(' ';' Expression ';' ')' Statement
 *	|	tFOR '(' ExpressionNoIn ';' Expression ';' ')' Statement
 *	|	tFOR '(' ';' ';' Expression ')' Statement
 *	|	tFOR '(' ExpressionNoIn ';' ';' Expression ')' Statement
 *	|	tFOR '(' ';' Expression ';' Expression ')' Statement
 *	|	tFOR '(' ExpressionNoIn ';' Expression ';' Expression ')'
 *			Statement
 *	|	tFOR '(' tVAR VariableDeclarationListNoIn ';' ';' ')' Statement
 *	|	tFOR '(' tVAR VariableDeclarationListNoIn ';'  
 *			Expression ';' ')' Statement
 *	|	tFOR '(' tVAR VariableDeclarationListNoIn ';' ';' 
 *			Expression ')' Statement
 *	|	tFOR '(' tVAR VariableDeclarationListNoIn ';' Expression ';' 
 *			Expression ')' Statement
 *	|	tFOR '(' LeftHandSideExpression tIN Expression ')' Statement
 *	|	tFOR '(' tVAR VariableDeclarationNoIn tIN Expression ')' 
 *			Statement
 *	;
 */

/* Note: the VarDecls of n->init are exposed through parser->vars */

static struct node *
IterationStatement_parse(parser)
	struct parser *parser;
{
	struct IterationStatement_while_node *w;
	struct IterationStatement_for_node *fn;
	struct IterationStatement_forin_node *fin;
	struct node *n;
	struct labelset *labelset = labelset_current(parser);

	labelset->continuable = 1;
	label_enter(parser, EMPTY_LABEL);
	switch (NEXT) {
	case tDO:
		w = NEW_NODE(struct IterationStatement_while_node,
			NODECLASS_IterationStatement_dowhile);
		SKIP;
		w->target = labelset->target;
		w->body = PARSE(Statement);
		EXPECT(tWHILE);
		EXPECT('(');
		w->cond = PARSE(Expression);
		EXPECT(')');
		EXPECT_SEMICOLON;
		label_leave(parser);
		return (struct node *)w;
	case tWHILE:
		w = NEW_NODE(struct IterationStatement_while_node,
			NODECLASS_IterationStatement_while);
		SKIP;
		w->target = labelset->target;
		EXPECT('(');
		w->cond = PARSE(Expression);
		EXPECT(')');
		w->body = PARSE(Statement);
		label_leave(parser);
		return (struct node *)w;
	case tFOR:
		break;
	default:
		SEE_ASSERT(parser->interpreter, !"unexpected token");
	}

	SKIP;		/* tFOR */
	EXPECT('(');

	if (NEXT == tVAR) {			 /* "for ( var" */
	    SKIP;	/* tVAR */
	    parser->noin = 1;
	    n = PARSE(VariableDeclarationList);	/* NB adds to parser->vars */
	    parser->noin = 0;
	    if (NEXT == tIN && 
		  n->nodeclass == NODECLASS_VariableDeclaration)
	    {					/* "for ( var VarDecl in" */
		fin = NEW_NODE(struct IterationStatement_forin_node,
		    NODECLASS_IterationStatement_forvarin);
		fin->target = labelset->target;
		fin->lhs = n;
		SKIP;	/* tIN */
		fin->list = PARSE(Expression);
		EXPECT(')');
		fin->body = PARSE(Statement);
		label_leave(parser);
		return (struct node *)fin;
	    }

	    /* Accurately describe possible tokens at this stage */
	    EXPECTX(';', 
	       (n->nodeclass == NODECLASS_VariableDeclaration
		  ? "';' or 'in'"
		  : "';'"));
					    /* "for ( var VarDeclList ;" */
	    fn = NEW_NODE(struct IterationStatement_for_node,
		NODECLASS_IterationStatement_forvar);
	    fn->target = labelset->target;
	    fn->init = n;
	    if (NEXT != ';')
		fn->cond = PARSE(Expression);
	    else
		fn->cond = NULL;
	    EXPECT(';');
	    if (NEXT != ')')
		fn->incr = PARSE(Expression);
	    else
		fn->incr = NULL;
	    EXPECT(')');
	    fn->body = PARSE(Statement);
	    label_leave(parser);
	    return (struct node *)fn;
	}

	if (NEXT != ';') {
	    parser->noin = 1;
	    n = PARSE(Expression);
	    parser->noin = 0;
	    if (NEXT == tIN && parser->is_lhs) {   /* "for ( lhs in" */
		fin = NEW_NODE(struct IterationStatement_forin_node,
		    NODECLASS_IterationStatement_forin);
		fin->target = labelset->target;
		fin->lhs = n;
		SKIP;		/* tIN */
		fin->list = PARSE(Expression);
		EXPECT(')');
		fin->body = PARSE(Statement);
		label_leave(parser);
		return (struct node *)fin;
	    }
	} else
	    n = NULL;				/* "for ( ;" */

	fn = NEW_NODE(struct IterationStatement_for_node,
	    NODECLASS_IterationStatement_for);
	fn->target = labelset->target;
	fn->init = n;
	EXPECT(';');
	if (NEXT != ';')
	    fn->cond = PARSE(Expression);
	else
	    fn->cond = NULL;
	EXPECT(';');
	if (NEXT != ')')
	    fn->incr = PARSE(Expression);
	else
	    fn->incr = NULL;
	EXPECT(')');
	fn->body = PARSE(Statement);
	label_leave(parser);
	return (struct node *)fn;
}

/*
 *	-- 12.7
 *
 *	ContinueStatement
 *	:	tCONTINUE ';'
 *	|	tCONTINUE tIDENT ';'
 *	;
 */
static struct node *
ContinueStatement_parse(parser)
	struct parser *parser;
{
	struct ContinueStatement_node *cn;

	cn = NEW_NODE(struct ContinueStatement_node,
		NODECLASS_ContinueStatement);
	EXPECT(tCONTINUE);
	if (NEXT_IS_SEMICOLON)
	    cn->target = target_lookup(parser, EMPTY_LABEL, tCONTINUE);
	else {
	    if (NEXT == tIDENT)
		cn->target = target_lookup(parser, NEXT_VALUE->u.string, 
			tCONTINUE);
	    EXPECT(tIDENT);
	}
	EXPECT_SEMICOLON;
	return (struct node *)cn;
}

/*
 *	-- 12.8
 *
 *	BreakStatement
 *	:	tBREAK ';'
 *	|	tBREAK tIDENT ';'
 *	;
 */
static struct node *
BreakStatement_parse(parser)
	struct parser *parser;
{
	struct BreakStatement_node *cn;

	cn = NEW_NODE(struct BreakStatement_node,
		NODECLASS_BreakStatement);
	EXPECT(tBREAK);
	if (NEXT_IS_SEMICOLON)
	    cn->target = target_lookup(parser, EMPTY_LABEL, tBREAK);
	else {
	    if (NEXT == tIDENT)
		cn->target = target_lookup(parser, NEXT_VALUE->u.string, 
		    tBREAK);
	    EXPECT(tIDENT);
	}
	EXPECT_SEMICOLON;
	return (struct node *)cn;
}

/*
 *	-- 12.9
 *
 *	ReturnStatement
 *	:	tRETURN ';'
 *	|	tRETURN Expression ';'
 *	;
 */
static struct node *
ReturnStatement_parse(parser)
	struct parser *parser;
{
	struct ReturnStatement_node *rn;

	EXPECT(tRETURN);
	if (!parser->funcdepth)
		ERRORm("'return' not within a function");
	if (!NEXT_IS_SEMICOLON) {
            rn = NEW_NODE(struct ReturnStatement_node,
                        NODECLASS_ReturnStatement);
	    rn->expr = PARSE(Expression);
	} else
            rn = NEW_NODE(struct ReturnStatement_node,
                        NODECLASS_ReturnStatement_undef);
	EXPECT_SEMICOLON;
	return (struct node *)rn;
}

/*
 *	-- 12.10
 *
 *	WithStatement
 *	:	tWITH '(' Expression ')' Statement
 *	;
 */
static struct node *
WithStatement_parse(parser)
	struct parser *parser;
{
	struct Binary_node *n;

	n = NEW_NODE(struct Binary_node, NODECLASS_WithStatement);
	EXPECT(tWITH);
	EXPECT('(');
	n->a = PARSE(Expression);
	EXPECT(')');
	n->b = PARSE(Statement);
	return (struct node *)n;
}

/*
 *	-- 12.11
 *
 *	SwitchStatement
 *	:	tSWITCH '(' Expression ')' CaseBlock
 *	;
 *
 *	CaseBlock
 *	:	'{' '}'
 *	|	'{' CaseClauses '}'
 *	|	'{' DefaultClause '}'
 *	|	'{' CaseClauses DefaultClause '}'
 *	|	'{' DefaultClause '}'
 *	|	'{' CaseClauses DefaultClause CaseClauses '}'
 *	;
 *
 *	CaseClauses
 *	:	CaseClause
 *	|	CaseClauses CaseClause
 *	;
 *
 *	CaseClause
 *	:	tCASE Expression ':'
 *	|	tCASE Expression ':' StatementList
 *	;
 *
 *	DefaultClause
 *	:	tDEFAULT ':'
 *	|	tDEFAULT ':' StatementList
 *	;
 */
static struct node *
SwitchStatement_parse(parser)
	struct parser *parser;
{
	struct SwitchStatement_node *n;
	struct case_list **cp, *c;
	int next;
	struct labelset *labelset = labelset_current(parser);

	n = NEW_NODE(struct SwitchStatement_node,
		NODECLASS_SwitchStatement);
	n->target = labelset->target;
	label_enter(parser, EMPTY_LABEL);
	EXPECT(tSWITCH);
	EXPECT('(');
	n->cond = PARSE(Expression);
	EXPECT(')');
	EXPECT('{');
	cp = &n->cases;
	n->defcase = NULL;
	while (NEXT != '}') {
	    c = SEE_NEW(parser->interpreter, struct case_list);
	    *cp = c;
	    cp = &c->next;
	    switch (NEXT) {
	    case tCASE:
		SKIP;
		c->expr = PARSE(Expression);
		break;
	    case tDEFAULT:
		SKIP;
		c->expr = NULL;
		if (n->defcase)
		    ERRORm("duplicate 'default' clause");
		n->defcase = c;
		break;
	    default:
		EXPECTED("'}', 'case' or 'default'");
	    }
	    EXPECT(':');
	    next = NEXT;
	    if (next != '}' && next != tDEFAULT && next != tCASE)
		c->body = PARSE(StatementList);
	    else
		c->body = NULL;
	}
	*cp = NULL;
	EXPECT('}');
	label_leave(parser);
	return (struct node *)n;
}

/*
 *	-- 12.12
 *
 *	LabelledStatement
 *	:	tIDENT
 *	|	Statement
 *	;
 */
static struct node *
LabelledStatement_parse(parser)
	struct parser *parser;
{
	struct LabelledStatement_node *n;
	struct SEE_string *label;
	unsigned int label_count = 0;
	struct labelset *old_labelset = parser->current_labelset;

	n = NEW_NODE(struct LabelledStatement_node, 
			NODECLASS_LabelledStatement);
	
	parser->current_labelset = NULL;
	n->target = labelset_current(parser)->target;
	do {
		/* Lookahead is IDENT ':' */
		label = NEXT_VALUE->u.string;
		label_enter(parser, label);
		label_count++;
		EXPECT(tIDENT);
		EXPECT(':');
	} while (NEXT == tIDENT && lookahead(parser, 1) == ':');

	switch (NEXT) {
	case tDO:
	case tWHILE:
	case tFOR:
		n->unary.a = PARSE(IterationStatement);
		break;
	case tSWITCH:
		n->unary.a = PARSE(SwitchStatement);
		break;
	default:
		n->unary.a = PARSE(Statement);
	}

	while (label_count--)
		label_leave(parser);

	parser->current_labelset = old_labelset;
	return (struct node *)n;
}

/*
 *	-- 12.13
 *
 *	ThrowStatement
 *	:	tTHROW Expression ';'
 *	;
 */
static struct node *
ThrowStatement_parse(parser)
	struct parser *parser;
{
	struct Unary_node *n;

	n = NEW_NODE(struct Unary_node, NODECLASS_ThrowStatement);
	EXPECT(tTHROW);
	if (NEXT_FOLLOWS_NL)
		ERRORm("newline not allowed after 'throw'");
	n->a = PARSE(Expression);
	EXPECT_SEMICOLON;
	return (struct node *)n;
}

/*
 *	-- 12.14
 *
 *	TryStatement
 *	:	tTRY Block Catch
 *	|	tTRY Block Finally
 *	|	tTRY Block Catch Finally
 *	;
 *
 *	Catch
 *	:	tCATCH '(' tIDENT ')' Block
 *	;
 *
 *	Finally
 *	:	tFINALLY Block
 *	;
 */
static struct node *
TryStatement_parse(parser)
	struct parser *parser;
{
	struct TryStatement_node *n;
	enum nodeclass_enum nc;
        struct node *block, *bcatch, *bfinally;
        struct SEE_string *ident = NULL;

	EXPECT(tTRY);
	block = PARSE(Block);
	if (NEXT == tCATCH) {
	    SKIP;
	    EXPECT('(');
	    if (NEXT == tIDENT)
		    ident = NEXT_VALUE->u.string;
	    EXPECT(tIDENT);
	    EXPECT(')');
	    bcatch = PARSE(Block);
	} else
	    bcatch = NULL;

	if (NEXT == tFINALLY) {
	    SKIP;
	    bfinally = PARSE(Block);
	} else
	    bfinally = NULL;

	if (bcatch && bfinally)
		nc = NODECLASS_TryStatement_catchfinally;
	else if (bcatch)
		nc = NODECLASS_TryStatement_catch;
	else if (bfinally)
		nc = NODECLASS_TryStatement_finally;
	else
		ERRORm("expected 'catch' or 'finally'");

	n = NEW_NODE(struct TryStatement_node, nc);
        n->block = block;
        n->bcatch = bcatch;
        n->bfinally = bfinally;
        n->ident = ident;

	return (struct node *)n;
}

/*
 *	-- 13
 *
 *	FunctionDeclaration
 *	:	tFUNCTION tIDENT '( ')' '{' FunctionBody '}'
 *	|	tFUNCTION tIDENT '( FormalParameterList ')' 
 *			'{' FunctionBody '}'
 *	;
 *
 *	FunctionExpression
 *	:	tFUNCTION '( ')' '{' FunctionBody '}'
 *	|	tFUNCTION '( FormalParameterList ')' '{' FunctionBody '}'
 *	|	tFUNCTION tIDENT '( ')' '{' FunctionBody '}'
 *	|	tFUNCTION tIDENT '( FormalParameterList ')' 
 *			'{' FunctionBody '}'
 *	;
 *
 *	FormalParameterList
 *	:	tIDENT
 *	|	FormalParameterList ',' tIDENT
 *	;
 *
 *	FunctionBody
 *	:	SourceElements
 *	;
 *
 *  Note: FunctionDeclaration semantics are never called, but defined in
 *  the spec. (Spec bug?)
 */
static struct node *
FunctionDeclaration_parse(parser)
	struct parser *parser;
{
	struct Function_node *n;
	struct node *body;
	struct var *formal;
	struct SEE_string *name = NULL;

	n = NEW_NODE(struct Function_node, NODECLASS_FunctionDeclaration);
	EXPECT(tFUNCTION);

	if (NEXT == tIDENT)
		name = NEXT_VALUE->u.string;
	EXPECT(tIDENT);

	EXPECT('(');
	formal = PARSE(FormalParameterList);
	EXPECT(')');

	EXPECT('{');
	parser->funcdepth++;
	body = PARSE(FunctionBody);
	parser->funcdepth--;
	EXPECT('}');

	n->function = SEE_function_make(parser->interpreter, 
		name, formal, make_body(parser->interpreter, body, 0));

	return (struct node *)n;
}

static struct node *
FunctionExpression_parse(parser)
	struct parser *parser;
{
	struct Function_node *n;
	struct var *formal;
	int noin_save, is_lhs_save;
	struct SEE_string *name;
	struct node *body;

	/* Save parser state */
	noin_save = parser->noin;
	is_lhs_save = parser->is_lhs;
	parser->noin = 0;
	parser->is_lhs = 0;

	n = NEW_NODE(struct Function_node, NODECLASS_FunctionExpression);
	EXPECT(tFUNCTION);
	if (NEXT == tIDENT) {
		name = NEXT_VALUE->u.string;
		SKIP;
	} else
		name = NULL;
	EXPECT('(');
	formal = PARSE(FormalParameterList);
	EXPECT(')');

	EXPECT('{');
	parser->funcdepth++;
	body = PARSE(FunctionBody);
	parser->funcdepth--;
	EXPECT('}');

	n->function = SEE_function_make(parser->interpreter,
		name, formal, make_body(parser->interpreter, body, 0));

	/* Restore parser state */
	parser->noin = noin_save;
	parser->is_lhs = is_lhs_save;

	return (struct node *)n;
}

static struct var *
FormalParameterList_parse(parser)
	struct parser *parser;
{
	struct var **p;
	struct var *result;

	p = &result;

	if (NEXT == tIDENT) {
	    *p = SEE_NEW(parser->interpreter, struct var);
	    (*p)->name = NEXT_VALUE->u.string;
	    p = &(*p)->next;
	    SKIP;
	    while (NEXT == ',') {
		SKIP;
		if (NEXT == tIDENT) {
		    *p = SEE_NEW(parser->interpreter, struct var);
		    (*p)->name = NEXT_VALUE->u.string;
		    p = &(*p)->next;
		}
		EXPECT(tIDENT);
	    }
	}
	*p = NULL;
	return result;
}

static struct node *
FunctionBody_make(interp, source_elements, is_program)
	struct SEE_interpreter *interp;
	struct node *source_elements;
	int is_program;
{
	struct FunctionBody_node *n;

	n = NEW_NODE_INTERNAL(interp, struct FunctionBody_node, 
	    NODECLASS_FunctionBody);
	n->u.a = source_elements;
	n->is_program = is_program;
	return (struct node *)n;
}

static struct node *
FunctionBody_parse(parser)
	struct parser *parser;
{
        struct FunctionBody_node *n;

	n = NEW_NODE(struct FunctionBody_node, NODECLASS_FunctionBody);
	n->u.a = PARSE(SourceElements);
	n->is_program = 0;
	return (struct node *)n;
}

/*
 * JavaScript 1.5 function statements. (Not part of ECMA-262, which
 * treats functions as declarations.) The statement 
 * 'function foo (args) { body };' is treated syntactically
 * equivalent to 'foo = function foo (args) { body };' The Netscape
 * documentation calls these 'conditional functions', as their intent
 * is to be used like this:
 *    if (0) function foo() { abc };
 *    else   function foo() { xyz };
 */
static struct node *
FunctionStatement_parse(parser)
	struct parser *parser;
{
	struct Function_node *f;
	struct PrimaryExpression_ident_node *i;
	struct AssignmentExpression_node *an;
	struct Unary_node *e;

	f = (struct Function_node *)FunctionExpression_parse(parser);

	i = NEW_NODE(struct PrimaryExpression_ident_node,
		NODECLASS_PrimaryExpression_ident);
	i->string = f->function->name;

	an = NEW_NODE(struct AssignmentExpression_node, 
			NODECLASS_AssignmentExpression_simple);
	an->lhs = (struct node *)i;
	an->expr = (struct node *)f;

	e = NEW_NODE(struct Unary_node, NODECLASS_ExpressionStatement);
	e->a = (struct node *)an;

	return (struct node *)e;
}

/*
 *	-- 14
 *
 *	Program
 *	:	SourceElements
 *	;
 *
 *
 *	SourceElements
 *	:	SourceElement
 *	|	SourceElements SourceElement
 *	;
 *
 *
 *	SourceElement
 *	:	Statement
 *	|	FunctionDeclaration
 *	;
 */
static struct function *
Program_parse(parser)
	struct parser *parser;
{
	struct node *body;
	struct FunctionBody_node *f;

	/*
	 * NB: The semantics of Program are indistinguishable from that of
	 * a FunctionBody. Syntactically, the only difference is that
	 * Program must be followed by the tEND (end-of-input) token.
	 * Practically, a program does not have parameters nor a name,
	 * and its 'this' is always set to the Global object.
	 */
	body = PARSE(FunctionBody);
	if (NEXT == '}')
		ERRORm("unmatched '}'");
	if (NEXT == ')')
		ERRORm("unmatched ')'");
	if (NEXT == ']')
		ERRORm("unmatched ']'");
	if (NEXT != tEND)
		ERRORm("unexpected token");

	f = CAST_NODE(body, FunctionBody);
	f->is_program = 1;

	return SEE_function_make(parser->interpreter,
		NULL, NULL, make_body(parser->interpreter, body, 0));
}

/* Builds a simple SourceElements around a single statement */
static struct node *
SourceElements_make1(interp, statement)
	struct SEE_interpreter *interp;
	struct node *statement;
{
	struct SourceElements_node *ss;
	struct SourceElement *s;

	s = SEE_NEW(interp, struct SourceElement);
	s->node = statement;
	s->next = NULL;
	ss = NEW_NODE_INTERNAL(interp, struct SourceElements_node, 
		NODECLASS_SourceElements); 
	ss->statements = s;
	ss->functions = NULL;
	ss->vars = NULL;
	return (struct node *)ss;
}

static struct node *
SourceElements_parse(parser)
	struct parser *parser;
{
	struct SourceElements_node *se;
	struct SourceElement **s, **f;
	struct var **vars_save;

	se = NEW_NODE(struct SourceElements_node, NODECLASS_SourceElements); 
	s = &se->statements;
	f = &se->functions;

	/* Whenever a VarDecl parses, it will get added to se->vars! */
	vars_save = parser->vars;
	parser->vars = &se->vars;

	for (;;) 
	    switch (NEXT) {
	    case tFUNCTION:
		if (lookahead(parser, 1) != '(') {
		    *f = SEE_NEW(parser->interpreter, struct SourceElement);
		    (*f)->node = PARSE(FunctionDeclaration);
		    f = &(*f)->next;
#ifndef NDEBUG
		    if (SEE_parse_debug) 
		        dprintf("SourceElements_parse: got function\n");
#endif
		    break;
		}
		/* else it's a function expression */
	    /* The 'first's of Statement */
	    case tTHIS: case tIDENT: case tSTRING: case tNUMBER:
	    case tNULL: case tTRUE: case tFALSE:
	    case '(': case '[': case '{':
	    case tNEW: case tDELETE: case tVOID: case tTYPEOF:
	    case tPLUSPLUS: case tMINUSMINUS:
	    case '+': case '-': case '~': case '!': case ';':
	    case tVAR: case tIF: case tDO: case tWHILE: case tFOR:
	    case tCONTINUE: case tBREAK: case tRETURN:
	    case tWITH: case tSWITCH: case tTHROW: case tTRY:
	    case tDIV: case tDIVEQ: /* in lieu of tREGEX */
		*s = SEE_NEW(parser->interpreter, struct SourceElement);
		(*s)->node = PARSE(Statement);
		s = &(*s)->next;
#ifndef NDEBUG
		if (SEE_parse_debug)
		    dprintf("SourceElements_parse: got statement\n");
#endif
		break;
	    case tEND:
	    default:
#ifndef NDEBUG
		if (SEE_parse_debug)
		    dprintf("SourceElements_parse: got EOF/other (%d)\n", 
		    	NEXT);
#endif
		*s = NULL;
		*f = NULL;
		*parser->vars = NULL;
		parser->vars = vars_save;

		return (struct node *)se;
	    }
}

/*------------------------------------------------------------
 * Public API
 */

/*
 * Parses a function declaration in two parts and
 * return a function structure, in a similar way to
 * FunctionDeclaration_parse() when called with the
 * right input.
 */
struct function *
SEE_parse_function(interp, name, paraminp, bodyinp)
	struct SEE_interpreter *interp;
	struct SEE_string *name;
	struct SEE_input *paraminp, *bodyinp;
{
	struct lex lex;
	struct parser parservar, *parser = &parservar;
	struct var *formal;
	struct node *body;

	if (paraminp) {
		SEE_lex_init(&lex, SEE_input_lookahead(paraminp, 6));
		parser_init(parser, interp, &lex);
		formal = PARSE(FormalParameterList);	/* handles "" too */
		EXPECT_NOSKIP(tEND);			/* uses parser var */
	} else
		formal = NULL;

	if (bodyinp) 
		SEE_lex_init(&lex, SEE_input_lookahead(bodyinp, 6));
	else {
		/* Set the lexer to EOF quickly */
		lex.input = NULL;
		lex.next = tEND;
	}
	parser_init(parser, interp, &lex);
	parser->funcdepth++;
	body = PARSE(FunctionBody);
	parser->funcdepth--;
	EXPECT_NOSKIP(tEND);

	return SEE_function_make(interp, name, formal, 
		make_body(interp, body, 0));
}

/*
 * Parses a Program. 
 * Does not close the input, but may consume up to 6 characters.
 * lookahead. This is not usually a problem, because the input is
 * always read to EOF on normal completion.
 */
struct function *
SEE_parse_program(interp, inp)
	struct SEE_interpreter *interp;
	struct SEE_input *inp;
{
	struct lex lex;
	struct parser localparse, *parser = &localparse;
	struct function *f;

	SEE_lex_init(&lex, SEE_input_lookahead(inp, 6));
	parser_init(parser, interp, &lex);
	f = PARSE(Program);

#if !defined(NDEBUG) && WITH_PARSER_PRINT && !WITH_PARSER_CODEGEN
	if (SEE_parse_debug) {
	    dprintf("parse Program result:\n");
            _SEE_parser_print(
                _SEE_parser_print_stdio_new(interp, stderr),
                (struct node *)f->body);
	    dprintf("<end>\n");
	}
#endif

	return f;
}

/*
 * Evaluates the function body with the given execution context. 
 * Function body must not be NULL
 */
static void
eval_functionbody(body, context, res)
	void *body;
	struct SEE_context *context;
	struct SEE_value *res;
{
#if WITH_PARSER_CODEGEN
        _SEE_codegen_eval_functionbody(body, context, res);
#else
        _SEE_eval_eval_functionbody(body, context, res);
#endif
}

/* Evaluates the function body with the given execution context. */
void
SEE_eval_functionbody(f, context, res)
	struct function *f;
	struct SEE_context *context;
	struct SEE_value *res;
{
	if (f && f->body)
	    eval_functionbody(f->body, context, res);
	else
	    SEE_SET_UNDEFINED(res);
	SEE_ASSERT(context->interpreter,
	    SEE_VALUE_GET_TYPE(res) != SEE_COMPLETION);
	SEE_ASSERT(context->interpreter,
	    SEE_VALUE_GET_TYPE(res) != SEE_REFERENCE);
}


int
SEE_functionbody_isempty(interp, f)
	struct SEE_interpreter *interp;
	struct function *f;
{
#if WITH_PARSER_CODEGEN
        return _SEE_codegen_functionbody_isempty(interp, f);
#else
	return _SEE_eval_functionbody_isempty(interp, f);
#endif
}

/* Returns true if the function body is empty */
int
_SEE_node_functionbody_isempty(interp, node)
        struct SEE_interpreter *interp;
        struct node *node;
{
        struct FunctionBody_node *fb = CAST_NODE(node, FunctionBody);
        struct SourceElements_node *se = CAST_NODE(fb->u.a, SourceElements);
        return se->statements == NULL &&
               se->vars == NULL &&
               (!fb->is_program || se->functions == NULL);
}


/* Returns the function body as a string */
struct SEE_string *
SEE_functionbody_string(interp, f)
	struct SEE_interpreter *interp;
	struct function *f;
{
	struct SEE_string *s;

#if WITH_PARSER_PRINT && !WITH_PARSER_CODEGEN
        s = SEE_string_new(interp, 0);
        _SEE_parser_print(_SEE_parser_print_string_new(interp, s),
	        (struct node *)f->body);
#else
        s = SEE_string_sprintf(interp, "/*%p*/", f);
#endif
	return s;
}

/*------------------------------------------------------------
 * eval
 *  -- 15.1.2.1
 */

/*
 * 15.1.2.1 Global.eval()
 * 'Eval' is a special function (not a cfunction), because it accesses 
 * the execution context of the caller (which is not available to 
 * functions and methods invoked via SEE_OBJECT_CALL()).
 *
 * This normally only ever get called from CallExpression_eval().
 * A stub cfunction exists for Global.eval, but it is bypassed.
 */
void
_SEE_call_eval(context, thisobj, argc, argv, res)
	struct SEE_context *context;
	struct SEE_object *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct SEE_input *inp;
        SEE_try_context_t try_ctxt;
	struct SEE_interpreter *interp = context->interpreter;

	if (argc == 0) {
		SEE_SET_UNDEFINED(res);
		return;
	}
	if (SEE_VALUE_GET_TYPE(argv[0]) != SEE_STRING) {
		SEE_VALUE_COPY(res, argv[0]);
		return;
	}

        /* It is not defined what happens when eval is called with
         * more than one argument. */
        if (argc != 1)
                SEE_error_throw_string(interp, interp->EvalError, 
                    STR(bad_argc));
	inp = SEE_input_string(interp, argv[0]->u.string);
	inp->filename = STR(eval_input_name);
        SEE_TRY(interp, try_ctxt) {
            _SEE_eval_input(context, thisobj, inp, res);
        }
        /*finally*/ {
            SEE_INPUT_CLOSE(inp);
        }
        SEE_DEFAULT_CATCH(interp, try_ctxt);
}

void
_SEE_eval_input(context, thisobj, inp, res)
	struct SEE_context *context;
	struct SEE_object *thisobj;
	struct SEE_input *inp;
	struct SEE_value *res;  /* optional */
{
	struct function *f;
	struct SEE_context evalcontext;
	struct SEE_interpreter *interp = context->interpreter;
        struct SEE_value ignore;

	/* 10.2.2 */
	evalcontext.interpreter = interp;

        /* XXX should an eval really share the context's activation? */
	evalcontext.activation = context->activation;

	evalcontext.variable = context->variable;
	evalcontext.varattr = 0;
	evalcontext.thisobj = context->thisobj;
	evalcontext.scope = context->scope;

	if (SEE_COMPAT_JS(interp, >=, JS11)	/* EXT:23 */
	    && thisobj && thisobj != interp->Global) 
	{
		/*
		 * support eval() being called from something
		 * other than the global object, where the 'thisobj'
		 * becomes the scope chain and variable object
		 */
		evalcontext.thisobj = thisobj;
		evalcontext.variable = thisobj;
		evalcontext.scope = SEE_NEW(interp, struct SEE_scope);
		evalcontext.scope->next = context->scope;
		evalcontext.scope->obj = thisobj;
	}
	
	f = SEE_parse_program(interp, inp);

	/* Set formal params to undefined, if any exist -- redundant? */
	SEE_function_put_args(context, f, 0, NULL);

	/* Evaluate the statement */
	SEE_eval_functionbody(f, &evalcontext, res ? res : &ignore);
}

/* 
 * Evaluates an expression in the given context.
 * This is a helper function intended for external debuggers wanting 
 * to evaluate user expressions in a given context.
 */
void
SEE_context_eval(context, expr, res)
	struct SEE_context *context;
	struct SEE_string *expr;
	struct SEE_value *res;
{
	struct SEE_value s, *argv[1];

	argv[0] = &s;
	SEE_SET_STRING(argv[0], expr);
	_SEE_call_eval(context, context->thisobj, 1, argv, res);
}

