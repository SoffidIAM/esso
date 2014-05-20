/* (c) 2009 David Leonard. All rights reserved. */
#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <see/mem.h>
#include <see/error.h>
#include <see/value.h>
#include <see/system.h>
#include <see/string.h>
#include <see/intern.h>
#include <see/try.h>

#include "dprint.h"
#include "stringdefs.h"
#include "function.h"
#include "parse.h"
#include "parse_node.h"
#include "parse_const.h"
#include "parse_codegen.h"
#include "code.h"
#include "nmath.h"              /* MAX() */

extern int SEE_parse_debug;

#define MAX3(a, b, c)    MAX(MAX(a, b), c)
#define MAX4(a, b, c, d) MAX(MAX(a, b), MAX(c, d))

#define PATCH_FIND_BREAK        0
#define PATCH_FIND_CONTINUE     1
#define CONTINUABLE 1

struct code_varscope {
	struct SEE_string *ident;
	unsigned int id;
	int in_scope;
};

struct code_context {
	struct SEE_code *code;

	/* A structure used to hold the break and continue patchables */
	struct patchables {
		SEE_code_patchable_t *cont_patch;
		unsigned int ncont_patch;
		struct SEE_growable gcont_patch;
		SEE_code_patchable_t *break_patch;
		unsigned int nbreak_patch;
		struct SEE_growable gbreak_patch;
		unsigned int target;
		struct patchables *prev;
		int continuable;
		unsigned int block_depth;
	} *patchables;

	/* The current block depth. Starts at zero. */
	unsigned int block_depth, max_block_depth;

	/* True when directly in the variables scope. This
	 * allows us to use VREF statements instead of LOOKUP.
	 * It goes false inside 'with' and 'catch' blocks.
	 * Individual vars can be descoped;
	 */
	int in_var_scope;

	/* True when we want to disable constant folding */
	int no_const;

	struct code_varscope *varscope;
	unsigned int          nvarscope;
	struct SEE_growable   gvarscope;
};

extern void (*_SEE_nodeclass_codegen[])(struct node *, 
        struct code_context *);
        /* unsigned int node.maxstack */
	/* Keeps track of the maximum stack space needed
	 * to run the code. */
	
        /* unsigned int node.is */
	/* Represents a union of the possible types that
	 * are left on top of the stack when code from
	 * an Expression node is run */
# define CG_TYPE_UNDEFINED	0x01
# define CG_TYPE_NULL		0x02
# define CG_TYPE_BOOLEAN	0x04
# define CG_TYPE_NUMBER		0x08
# define CG_TYPE_STRING		0x10
# define CG_TYPE_OBJECT		0x20
# define CG_TYPE_REFERENCE	0x40
# define CG_TYPE_PRIMITIVE	(CG_TYPE_UNDEFINED | \
				 CG_TYPE_NULL | \
				 CG_TYPE_BOOLEAN | \
				 CG_TYPE_NUMBER | \
				 CG_TYPE_STRING)
# define CG_TYPE_VALUE		(CG_TYPE_PRIMITIVE | CG_TYPE_OBJECT)
# define CG_IS_VALUE(n)	    (!((n)->is & CG_TYPE_REFERENCE))
# define CG_IS_PRIMITIVE(n) (!((n)->is & (CG_TYPE_REFERENCE|CG_TYPE_OBJECT)))
# define CG_IS_BOOLEAN(n)   ((n)->is == CG_TYPE_BOOLEAN)
# define CG_IS_NUMBER(n)    ((n)->is == CG_TYPE_NUMBER)
# define CG_IS_STRING(n)    ((n)->is == CG_TYPE_STRING)
# define CG_IS_OBJECT(n)    ((n)->is == CG_TYPE_OBJECT)

static void Arguments_codegen(struct node *na, struct code_context *cc);
static void push_patchables(struct code_context *cc, unsigned int target, 
	int cont);
static void pop_patchables(struct code_context *cc, 
	SEE_code_addr_t cont_addr, SEE_code_addr_t break_addr);
static struct patchables *patch_find(struct code_context *cc, 
	unsigned int target, int tok);
static void patch_add_continue(struct code_context *cc, struct patchables *p,
	SEE_code_patchable_t pa);
static void patch_add_break(struct code_context *cc, struct patchables *p,
	SEE_code_patchable_t pa);
static void cg_init(struct SEE_interpreter *, struct code_context *, int);
static void cg_const_codegen(struct node *node, struct code_context *cc);
static struct SEE_code *cg_fini(struct SEE_interpreter *interp,
	struct code_context *cc, unsigned int maxstack);
static void cg_block_enter(struct code_context *cc);
static void cg_block_leave(struct code_context *cc);
static unsigned int cg_block_current(struct code_context *cc);
static unsigned int cg_var_id(struct code_context *, struct SEE_string *);
static int cg_var_is_in_scope(struct code_context *, struct SEE_string *);
static void cg_var_set_scope(struct code_context *, struct SEE_string *, int);
static int cg_var_set_all_scope(struct code_context *, int);

# define CODEGENFN(node) _SEE_nodeclass_codegen[(node)->nodeclass]
# define CODEGEN(node)	do {				\
	if (!(cc)->no_const &&				\
	    ISCONST(node, (cc)->code->interpreter) &&	\
	    node->nodeclass != NODECLASS_Literal)	\
		cg_const_codegen(node, cc);		\
	else						\
	    (*CODEGENFN(node))(node, cc);	        \
    } while (0)

/* Call/construct operators */
# define _CG_OP1(name, n) \
    (*cc->code->code_class->gen_op1)(cc->code, SEE_CODE_##name, n)
# define CG_NEW(n)		_CG_OP1(NEW, n)
# define CG_CALL(n)		_CG_OP1(CALL, n)
# define CG_END(n)		_CG_OP1(END, n)
# define CG_VREF(n)		_CG_OP1(VREF, n)

/* Generic operators */
# define _CG_OP0(name) \
    (*cc->code->code_class->gen_op0)(cc->code, SEE_CODE_##name)
# define CG_NOP()		_CG_OP0(NOP)
# define CG_DUP()		_CG_OP0(DUP)
# define CG_POP()		_CG_OP0(POP)
# define CG_EXCH()		_CG_OP0(EXCH)
# define CG_ROLL3()		_CG_OP0(ROLL3)
# define CG_THROW()		_CG_OP0(THROW)
# define CG_SETC()		_CG_OP0(SETC)
# define CG_GETC()		_CG_OP0(GETC)
# define CG_THIS()		_CG_OP0(THIS)
# define CG_OBJECT()		_CG_OP0(OBJECT)
# define CG_ARRAY()		_CG_OP0(ARRAY)
# define CG_REGEXP()		_CG_OP0(REGEXP)
# define CG_REF()		_CG_OP0(REF)
# define CG_GETVALUE()		_CG_OP0(GETVALUE)
# define CG_LOOKUP()		_CG_OP0(LOOKUP)
# define CG_PUTVALUE()		_CG_OP0(PUTVALUE)
# define CG_DELETE()		_CG_OP0(DELETE)
# define CG_TYPEOF()		_CG_OP0(TYPEOF)
# define CG_TOOBJECT()		_CG_OP0(TOOBJECT)
# define CG_TONUMBER()		_CG_OP0(TONUMBER)
# define CG_TOBOOLEAN()		_CG_OP0(TOBOOLEAN)
# define CG_TOSTRING()		_CG_OP0(TOSTRING)
# define CG_TOPRIMITIVE()	_CG_OP0(TOPRIMITIVE)
# define CG_NEG()		_CG_OP0(NEG)
# define CG_INV()		_CG_OP0(INV)
# define CG_NOT()		_CG_OP0(NOT)
# define CG_MUL()		_CG_OP0(MUL)
# define CG_DIV()		_CG_OP0(DIV)
# define CG_MOD()		_CG_OP0(MOD)
# define CG_ADD()		_CG_OP0(ADD)
# define CG_SUB()		_CG_OP0(SUB)
# define CG_LSHIFT()		_CG_OP0(LSHIFT)
# define CG_RSHIFT()		_CG_OP0(RSHIFT)
# define CG_URSHIFT()		_CG_OP0(URSHIFT)
# define CG_LT()		_CG_OP0(LT)
# define CG_GT()		_CG_OP0(GT)
# define CG_LE()		_CG_OP0(LE)
# define CG_GE()		_CG_OP0(GE)
# define CG_INSTANCEOF()	_CG_OP0(INSTANCEOF)
# define CG_IN()		_CG_OP0(IN)
# define CG_EQ()		_CG_OP0(EQ)
# define CG_SEQ()		_CG_OP0(SEQ)
# define CG_BAND()		_CG_OP0(BAND)
# define CG_BXOR()		_CG_OP0(BXOR)
# define CG_BOR()		_CG_OP0(BOR)
# define CG_S_ENUM()		_CG_OP0(S_ENUM)
# define CG_S_WITH()		_CG_OP0(S_WITH)
# define CG_S_CATCH()		_CG_OP0(S_CATCH)
# define CG_ENDF()		_CG_OP0(ENDF)

/* Special PUTVALUE that takes attributes */
# define CG_PUTVALUEA(attr)	_CG_OP1(PUTVALUEA,attr)

/* Literals */
# define CG_LITERAL(vp) \
	(*cc->code->code_class->gen_literal)(cc->code, vp)

# define CG_UNDEFINED() do {		/* - | num */	\
	struct SEE_value _cgtmp;			\
	SEE_SET_UNDEFINED(&_cgtmp);			\
	CG_LITERAL(&_cgtmp);				\
  } while (0)

# define CG_STRING(str) do {		/* - | str */	\
	struct SEE_value _cgtmp;			\
	SEE_SET_STRING(&_cgtmp, str);\
	CG_LITERAL(&_cgtmp);				\
  } while (0)

# define CG_NUMBER(num) do {		/* - | num */	\
	struct SEE_value _cgtmp;			\
	SEE_SET_NUMBER(&_cgtmp, num);			\
	CG_LITERAL(&_cgtmp);				\
  } while (0)

# define CG_BOOLEAN(bool) do {		/* - | bool */	\
	struct SEE_value _cgtmp;			\
	SEE_SET_BOOLEAN(&_cgtmp, bool);			\
	CG_LITERAL(&_cgtmp);				\
  } while (0)
# define CG_TRUE()   CG_BOOLEAN(1)	/* - | true */
# define CG_FALSE()  CG_BOOLEAN(0)	/* - | false */

/* Function instance */
# define CG_FUNC(fn) \
	(*cc->code->code_class->gen_func)(cc->code, fn)	/* - | obj */

/* Record source location */
# define CG_LOC(loc) \
	(*cc->code->code_class->gen_loc)(cc->code, loc)

/* Branching and patching */
# define CG_HERE()						\
	(*cc->code->code_class->here)(cc->code)

/* Patch a previously saved address to point to CG_HERE() */
# define CG_LABEL(var)						\
	(*cc->code->code_class->patch)(cc->code, var, CG_HERE())

# define _CG_OPA(name, patchp, addr)				\
    (*cc->code->code_class->gen_opa)(cc->code, SEE_CODE_##name, patchp, addr)

/* Backward (_b) and forward (_f) branching */
# define CG_B_ALWAYS_b(addr)		_CG_OPA(B_ALWAYS, 0, addr)
# define CG_B_TRUE_b(addr)		_CG_OPA(B_TRUE, 0, addr)
# define CG_B_ENUM_b(addr)		_CG_OPA(B_ENUM, 0, addr)
# define CG_S_TRYC_b(addr)		_CG_OPA(S_TRYC, 0, addr)
# define CG_S_TRYF_b(addr)		_CG_OPA(S_TRYF, 0, addr)

# define CG_B_ALWAYS_f(var)		_CG_OPA(B_ALWAYS, &(var), 0)
# define CG_B_TRUE_f(var)		_CG_OPA(B_TRUE, &(var), 0)
# define CG_B_ENUM_f(var)		_CG_OPA(B_ENUM, &(var), 0)
# define CG_S_TRYC_f(var)		_CG_OPA(S_TRYC, &(var), 0)
# define CG_S_TRYF_f(var)		_CG_OPA(S_TRYF, &(var), 0)

/* Execute program code */
# define CG_EXEC(co, ctxt, res)		(*(co)->code_class->exec)(co, ctxt, res)

/* Creates a new patchables for breaking/continuing */
static void
push_patchables(cc, target, continuable)
	struct code_context *cc;
	unsigned int target;
	int continuable;
{
	struct patchables *p;
	struct SEE_interpreter *interp = cc->code->interpreter;

	/* Initialise two empty lists of patchable locations */
	p = SEE_NEW(interp, struct patchables);
	SEE_GROW_INIT(interp, &p->gcont_patch, p->cont_patch,
	    p->ncont_patch);
	SEE_GROW_INIT(interp, &p->gbreak_patch, p->break_patch, 
	    p->nbreak_patch);
	p->target = target;
	p->continuable = continuable;
	p->block_depth = cc->block_depth;
	p->prev = cc->patchables;
	cc->patchables = p;
}

/* Pops a patchables, performing the previously pending patches */
static void
pop_patchables(cc, cont_addr, break_addr)
	struct code_context *cc;
	SEE_code_addr_t cont_addr;
	SEE_code_addr_t break_addr;
{
	struct patchables *p = cc->patchables;
	unsigned int i;

	/* Patch the continue locations with the break addresses */
	for (i = 0; i < p->ncont_patch; i++) {
#ifndef NDEBUG
	    if (SEE_parse_debug)
		dprintf("patching continue to 0x%x at 0x%x\n", 
		    cont_addr, p->cont_patch[i]);
#endif
	    (*cc->code->code_class->patch)(cc->code, p->cont_patch[i], 
		cont_addr);
	}

	/* Patch the break locations with the break address */
	for (i = 0; i < p->nbreak_patch; i++) {
#ifndef NDEBUG
	    if (SEE_parse_debug)
		dprintf("patching break to 0x%x at 0x%x\n", 
		    break_addr, p->break_patch[i]);
#endif
	    (*cc->code->code_class->patch)(cc->code, p->break_patch[i], 
		break_addr);
	}

	cc->patchables = p->prev;
}

/* Return the right patchables when breaking/continuing */
static struct patchables *
patch_find(cc, target, type)
	struct code_context *cc;
	unsigned int target;
	int type;    /* PATCH_FIND_BREAK or PATCH_FIND_CONTINUE */
{
	struct patchables *p;

	if (target == NO_TARGET && type == PATCH_FIND_CONTINUE) {
	    for (p = cc->patchables; p; p = p->prev)
		if (p->continuable)
		    return p;
	} else if (target == NO_TARGET)
	    return cc->patchables;
	else
	    for (p = cc->patchables; p; p = p->prev)
		if (p->target == target)
		    return p;
	SEE_ASSERT(cc->code->interpreter, !"lost patchable");
	/* UNREACHABLE */
	return NULL; 
}

/* Add a pending continue patch */
static void
patch_add_continue(cc, p, pa)
	struct code_context *cc;
	struct patchables *p;
	SEE_code_patchable_t pa;
{
	struct SEE_interpreter *interp = cc->code->interpreter;
	unsigned int n = p->ncont_patch;

	SEE_GROW_TO(interp, &p->gcont_patch, n + 1);
	p->cont_patch[n] = pa;
}

/* Add a pending break patch */
static void
patch_add_break(cc, p, pa)
	struct code_context *cc;
	struct patchables *p;
	SEE_code_patchable_t pa;
{
	struct SEE_interpreter *interp = cc->code->interpreter;
	unsigned int n = p->nbreak_patch;

	SEE_GROW_TO(interp, &p->gbreak_patch, n + 1);
	p->break_patch[n] = pa;
}

static void
cg_init(interp, cc, no_const)
	struct SEE_interpreter *interp;
	struct code_context *cc;
	int no_const;
{
	cc->code = (*SEE_system.code_alloc)(interp);
	cc->patchables = NULL;
	cc->block_depth = 0;
	cc->max_block_depth = 0;
	cc->in_var_scope = 1;
	cc->no_const = no_const;
	SEE_GROW_INIT(interp, &cc->gvarscope, cc->varscope, cc->nvarscope);
}

static struct SEE_code *
cg_fini(interp, cc, maxstack)
	struct SEE_interpreter *interp;
	struct code_context *cc;
	unsigned int maxstack;
{
	struct SEE_code *co = cc->code;

	SEE_ASSERT(interp, cc->block_depth == 0);
	SEE_ASSERT(interp, cc->in_var_scope);
	(*co->code_class->maxstack)(co, maxstack);
	(*co->code_class->maxblock)(co, cc->max_block_depth);
	(*co->code_class->close)(co);
	cc->code = NULL;
	return co;
}

/*
 * Evaluates a (constant) expression node, and then generates a LITERAL
 * instruction.
 */
static void
cg_const_codegen(node, cc)
	struct node *node;
	struct code_context *cc;
{
	struct SEE_value value;

	_SEE_const_evaluate(node, cc->code->interpreter, &value);
	CG_LITERAL(&value);
	switch (SEE_VALUE_GET_TYPE(&value)) {
	case SEE_UNDEFINED: node->is = CG_TYPE_UNDEFINED; break;
	case SEE_NULL:	    node->is = CG_TYPE_NULL;	  break;
	case SEE_BOOLEAN:   node->is = CG_TYPE_BOOLEAN;   break;
	case SEE_NUMBER:    node->is = CG_TYPE_NUMBER;    break;
	case SEE_STRING:    node->is = CG_TYPE_STRING;    break;
	case SEE_OBJECT:    node->is = CG_TYPE_OBJECT;    break;
	case SEE_REFERENCE: node->is = CG_TYPE_REFERENCE; break;
	default:	    node->is = 0;
	}
	node->maxstack = 1;
}

/* Called when entering a block. Increments the block depth */
static void
cg_block_enter(cc)
	struct code_context *cc;
{
	cc->block_depth++;
	if (cc->block_depth > cc->max_block_depth)
	    cc->max_block_depth = cc->block_depth;
}

/* Called when leaving a block. Restores the block depth */
static void
cg_block_leave(cc)
	struct code_context *cc;
{
	cc->block_depth--;
}

/* Returns the current block depth, suitable for CG_END() */
static unsigned int
cg_block_current(cc)
	struct code_context *cc;
{
	return cc->block_depth;
}

/* Returns the VREF ID of a identifier in the immediate variable scope */
static unsigned int
cg_var_id(cc, ident)
	struct code_context *cc;
	struct SEE_string *ident;
{
	unsigned int i;

	for (i = 0; i < cc->nvarscope; i++)
	    if (cc->varscope[i].ident == ident) {
#ifndef NDEBUG
		if (SEE_parse_debug) {
		    dprintf("cg_var_id(");
		    dprints(ident);
		    dprintf(") = %u\n", cc->varscope[i].id);
		}
#endif
		return cc->varscope[i].id;
	    }
	SEE_ASSERT(cc->code->interpreter, !"bad cg var identifier");
	return ~0; /* unreachable */
}

/* Returns true if the identifier is a variable in the immediate scope */
static int
cg_var_is_in_scope(cc, ident)
	struct code_context *cc;
	struct SEE_string *ident;
{
	unsigned int i;

	/* If in a 'with' block, then nothing is certain */
	if (cc->in_var_scope)
	    for (i = 0; i < cc->nvarscope; i++)
		if (cc->varscope[i].ident == ident) {
#ifndef NDEBUG
		    if (SEE_parse_debug) {
			dprintf("cg_var_is_in_scope(");
			dprints(ident);
			dprintf("): found, in_scope=%d\n",
			    cc->varscope[i].in_scope);
		    }
#endif		    
		    return cc->varscope[i].in_scope;
		}
#ifndef NDEBUG
	if (SEE_parse_debug) {
	    dprintf("cg_var_is_in_scope(");
	    dprints(ident);
	    dprintf("): not found\n");
	}
#endif		    
	return 0;
}

/* Sets the scope of a variable identifier */
static void
cg_var_set_scope(cc, ident, in_scope)
	struct code_context *cc;
	struct SEE_string *ident;
	int in_scope;
{
	unsigned int i;

	for (i = 0; i < cc->nvarscope; i++)
	    if (cc->varscope[i].ident == ident) {
#ifndef NDEBUG
		if (SEE_parse_debug) {
		    dprintf("cg_var_set_scope(");
		    dprints(ident);
		    dprintf(", %d): previously %d\n",
			in_scope, cc->varscope[i].in_scope);
		}
#endif		    
		cc->varscope[i].in_scope = in_scope;
		return;
	    }
	if (in_scope) {
	    SEE_GROW_TO(cc->code->interpreter, &cc->gvarscope,
			cc->nvarscope + 1);
	    cc->varscope[i].ident = ident;
	    cc->varscope[i].id = 
		(*cc->code->code_class->gen_var)(cc->code, ident);
	    cc->varscope[i].in_scope = 1;
#ifndef NDEBUG
	    if (SEE_parse_debug) {
		dprintf("cg_var_set_scope(");
		dprints(ident);
		dprintf(", %d): NEW (id %u)\n", in_scope, cc->varscope[i].id);
	    }
#endif
	}
}

/* Temporarily sets the scope visibility of all var idents. Returns old value.
 * This is used when entering a 'with' scope */
static int
cg_var_set_all_scope(cc, in_scope)
	struct code_context *cc;
	int in_scope;
{
	int old_scope = cc->in_var_scope;
	cc->in_var_scope = in_scope;
#ifndef NDEBUG
	if (SEE_parse_debug)
	    dprintf("cg_var_set_all_scope(%d) -> %d\n", in_scope, old_scope);
#endif
	return old_scope;
}

/* Returns a body suitable for use by eval_functionbody() */
void *
_SEE_codegen_make_body(interp, node, no_const)
	struct SEE_interpreter *interp;
	struct node *node;
	int no_const;
{
	struct code_context ccstorage, *cc;

	/* If there is no body, return NULL */
	if (_SEE_node_functionbody_isempty(interp, node))
	    return NULL;

	cc = &ccstorage;
	cg_init(interp, cc, no_const);
	CODEGEN(node);
	return cg_fini(interp, cc, node->maxstack);
}

/* 7.8 */
static void
Literal_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct Literal_node *n = CAST_NODE(na, Literal);

	CG_LITERAL(&n->value);		    /* val */
	if (SEE_VALUE_GET_TYPE(&n->value) == SEE_BOOLEAN)
	    n->node.is = CG_TYPE_BOOLEAN;
	else if (SEE_VALUE_GET_TYPE(&n->value) == SEE_NULL)
	    n->node.is = CG_TYPE_NULL;
	n->node.maxstack = 1;
}

/* 7.8.4 */
static void
StringLiteral_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct StringLiteral_node *n = CAST_NODE(na, StringLiteral);

	CG_STRING(n->string);		/* str */
	n->node.is = CG_TYPE_STRING;
	n->node.maxstack = 1;
}

/* 7.8.5 */
static void
RegularExpressionLiteral_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct RegularExpressionLiteral_node *n = 
		CAST_NODE(na, RegularExpressionLiteral);

	SEE_ASSERT(cc->code->interpreter, 
	    SEE_VALUE_GET_TYPE(&n->pattern) == SEE_STRING);
	SEE_ASSERT(cc->code->interpreter, 
	    SEE_VALUE_GET_TYPE(&n->flags) == SEE_STRING);

	CG_REGEXP();			/* obj */
	CG_STRING(n->pattern.u.string);	/* obj str */
	CG_STRING(n->flags.u.string);	/* obj str str */
	CG_NEW(2);			/* obj */

	n->node.is = CG_TYPE_OBJECT;
	n->node.maxstack = 3;
}

/* 11.1.1 */
static void
PrimaryExpression_this_codegen(n, cc)
	struct node *n;
	struct code_context *cc;
{
	CG_THIS();		/* obj */

	n->is = CG_TYPE_OBJECT;
	n->maxstack = 1;
}

/* 11.1.2 */
static void
PrimaryExpression_ident_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct PrimaryExpression_ident_node *n = 
		CAST_NODE(na, PrimaryExpression_ident);

	if (cg_var_is_in_scope(cc, n->string)) 
	    CG_VREF(cg_var_id(cc, n->string));	/* ref */
	else {
	    CG_STRING(n->string);		/* str */
	    CG_LOOKUP();			/* ref */
	}

	n->node.is = CG_TYPE_REFERENCE;
	n->node.maxstack = 2;
}

/* 11.1.4 */
static void
ArrayLiteral_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct ArrayLiteral_node *n = CAST_NODE(na, ArrayLiteral);
	struct ArrayLiteral_element *element;
	struct SEE_string *ind;
	struct SEE_interpreter *interp = cc->code->interpreter;
	unsigned int maxstack = 0;

	ind = SEE_string_new(interp, 16);

	CG_ARRAY();			    /* Array */
	CG_NEW(0);			    /* a */

	for (element = n->first; element; element = element->next) {
		CG_DUP();		    /* a a */
		ind->length = 0;
		SEE_string_append_int(ind, element->index);
		CG_STRING(SEE_intern(interp, ind)); /* a a "element" */
		CG_REF();		    /* a a[element] */
		CODEGEN(element->expr);	    /* a a[element] ref */
		maxstack = MAX(maxstack, element->expr->maxstack);

		if (!CG_IS_VALUE(element->expr))
		    CG_GETVALUE();	    /* a a[element] val */
		CG_PUTVALUE();		    /* a */
	}
	
	CG_DUP();			    /* a a */
	CG_STRING(STR(length));		    /* a a "length" */
	CG_REF();			    /* a a.length */
	CG_NUMBER(n->length);		    /* a a.length num */ 
	CG_PUTVALUE();			    /* a */

	n->node.is = CG_TYPE_OBJECT;
	n->node.maxstack = MAX(3, 2 + maxstack);
}

/* 11.1.5 */
static void
ObjectLiteral_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct ObjectLiteral_node *n = CAST_NODE(na, ObjectLiteral);
	struct ObjectLiteral_pair *pair;
	unsigned int maxstack = 0;

	CG_OBJECT();			    /* Object */
	CG_NEW(0);			    /* o */
	for (pair = n->first; pair; pair = pair->next) {
		CG_DUP();		    /* o o */
		CG_STRING(pair->name);	    /* o o name */
		CG_REF();		    /* o o.name */
		CODEGEN(pair->value);	    /* o o.name ref */
		maxstack = MAX(maxstack, pair->value->maxstack);
		if (!CG_IS_VALUE(pair->value))
		    CG_GETVALUE();	    /* o o.name val */
		CG_PUTVALUE();		    /* o */
	}

	n->node.is = CG_TYPE_OBJECT;
	n->node.maxstack = MAX(maxstack + 2, 3);
}

/* 11.2.4 */
static void
Arguments_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct Arguments_node *n = CAST_NODE(na, Arguments);
	struct Arguments_arg *arg;
	unsigned int maxstack = 0;
	unsigned int onstack = 0;

	for (arg = n->first; arg; arg = arg->next) {
					    /* ... */
		CODEGEN(arg->expr);	    /* ... ref */
		maxstack = MAX(maxstack, onstack + arg->expr->maxstack);
		if (!CG_IS_VALUE(arg->expr))
		    CG_GETVALUE();	    /* ... val */
		onstack++;
	}
	n->node.maxstack = maxstack;
}

/* 11.2.2 */
static void
MemberExpression_new_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct MemberExpression_new_node *n = 
		CAST_NODE(na, MemberExpression_new);
	int argc;
	int maxstack = 0;

	CODEGEN(n->mexp);		/* ref */
	maxstack = n->mexp->maxstack;
	if (!CG_IS_VALUE(n->mexp))
	    CG_GETVALUE();		/* val */
	if (n->args) {
		Arguments_codegen((struct node *)n->args, cc);
					/* val arg1..argn */
		argc = n->args->argc;
		maxstack = MAX(maxstack, 1+((struct node *)n->args)->maxstack);
	} else
		argc = 0;
	CG_NEW(argc);			/* obj */

	/* Assume that 'new' always yields an object by s8.6.2 */
	n->node.is = CG_TYPE_OBJECT;	
	n->node.maxstack = maxstack;
}

/* 11.2.1 */
static void
MemberExpression_dot_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct MemberExpression_dot_node *n = 
		CAST_NODE(na, MemberExpression_dot);

	CODEGEN(n->mexp);	    /* ref */
	if (!CG_IS_VALUE(n->mexp))
	    CG_GETVALUE();	    /* val */
	if (!CG_IS_OBJECT(n->mexp))
	    CG_TOOBJECT();	    /* obj */
	CG_STRING(n->name);	    /* obj "name" */
	CG_REF();		    /* ref */

	n->node.is = CG_TYPE_REFERENCE;
	n->node.maxstack = MAX(2, n->mexp->maxstack);
}

/* 11.2.1 */
static void
MemberExpression_bracket_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct MemberExpression_bracket_node *n = 
		CAST_NODE(na, MemberExpression_bracket);

	CODEGEN(n->mexp);	    /* ref1 */
	if (!CG_IS_VALUE(n->mexp))
	    CG_GETVALUE();	    /* val1 */
	CODEGEN(n->name);	    /* val1 ref2 */
	if (!CG_IS_VALUE(n->name))
	    CG_GETVALUE();	    /* val1 val2 */
	/* Note: we have to fritz with EXCH to match
	 * the semantics of 11.2.1 */
	if (!CG_IS_OBJECT(n->mexp)) {
	    CG_EXCH();		    /* val2 val1 */
	    CG_TOOBJECT();	    /* val2 obj1 */
	    CG_EXCH();		    /* obj1 val2 */
	}
	if (!CG_IS_STRING(n->name))
	    CG_TOSTRING();	    /* obj1 str2 */
	CG_REF();		    /* ref */

	n->node.is = CG_TYPE_REFERENCE;
	n->node.maxstack = MAX(n->mexp->maxstack, 1 + n->name->maxstack);
}

/* 11.2.3 */
static void
CallExpression_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct CallExpression_node *n = CAST_NODE(na, CallExpression);

	CODEGEN(n->exp);		/* ref */
	Arguments_codegen((struct node *)n->args, cc);	/* ref arg1 .. argn */
	CG_CALL(n->args->argc);		/* val */

	/* Called functions only return values */
	n->node.is = CG_TYPE_VALUE;
	n->node.maxstack = MAX(n->exp->maxstack,
	    1 + ((struct node *)n->args)->maxstack);
}

/* 11.3.1 */
static void
PostfixExpression_inc_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct Unary_node *n = CAST_NODE(na, Unary);

	CODEGEN(n->a);		/* ref */
	CG_DUP();		/* ref ref */
	if (!CG_IS_VALUE(n->a))
	    CG_GETVALUE();	/* ref val */
	if (!CG_IS_NUMBER(n->a))
	    CG_TONUMBER();	/* ref num */
	CG_DUP();		/* ref num num */
	CG_ROLL3();		/* num ref num */
	CG_NUMBER(1);		/* num ref num   1 */
	CG_ADD();		/* num ref num+1 */
	CG_PUTVALUE();		/* num */

	n->node.is = CG_TYPE_NUMBER;
	n->node.maxstack = MAX(n->a->maxstack, 4);

	/*
	 * Peephole optimisation note:
	 *		ref num
	 *  DUP		ref num num
	 *  ROLL3       num ref num
	 *  LITERAL,?   num ref num ?
	 *  ADD|SUB     num ref ?
	 *  PUTVALUE    num
	 *  POP         -
	 *
	 * is equivalent to:
	 *              ref num
	 *  LITERAL,?   ref num ?
	 *  ADD|SUB     ref ?
	 *  PUTVALUE	-
	 */

	/*
	 * Peephole optimisation note:
	 *		ref num
	 *  DUP		ref num num
	 *  ROLL3       num ref num
	 *  LITERAL,?   num ref num ?
	 *  ADD|SUB     num ref ?
	 *  PUTVALUE    num
	 *  SETC        -
	 *
	 * is equivalent to:
	 *              ref num
	 *  LITERAL,?   ref num ?
	 *  ADD|SUB     ref ?
	 *  DUP		ref ? ?
	 *  SETC	ref ?
	 *  PUTVALUE	-
	 */

}

/* 11.3.2 */
static void
PostfixExpression_dec_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct Unary_node *n = CAST_NODE(na, Unary);

	CODEGEN(n->a);		/* aref */
	CG_DUP();		/* aref aref */
	if (!CG_IS_VALUE(n->a))
	    CG_GETVALUE();	/* aref aval */
	if (!CG_IS_NUMBER(n->a))
	    CG_TONUMBER();	/* aref anum */
	CG_DUP();		/* aref anum anum */
	CG_ROLL3();		/* anum aref anum */
	CG_NUMBER(1);		/* anum aref anum   1 */
	CG_SUB();		/* anum aref anum-1 */
	CG_PUTVALUE();		/* anum */

	n->node.is = CG_TYPE_NUMBER;
	n->node.maxstack = MAX(n->a->maxstack, 4);
}

/* 11.4.1 */
static void
UnaryExpression_delete_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct Unary_node *n = CAST_NODE(na, Unary);

	CODEGEN(n->a);	/* ref */
	CG_DELETE();	/* bool */

	n->node.is = CG_TYPE_BOOLEAN;
	n->node.maxstack = n->a->maxstack;
}

/* 11.4.2 */
static void
UnaryExpression_void_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct Unary_node *n = CAST_NODE(na, Unary);
	static const struct SEE_value cg_undefined = { SEE_UNDEFINED };

	CODEGEN(n->a);		    /* ref */
	if (!CG_IS_VALUE(n->a))
	    CG_GETVALUE();	    /* val */
	CG_POP();		    /* - */
	CG_LITERAL(&cg_undefined);  /* undef */

	n->node.is = CG_TYPE_UNDEFINED;
	n->node.maxstack = n->a->maxstack;
}

/* 11.4.3 */
static void
UnaryExpression_typeof_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct Unary_node *n = CAST_NODE(na, Unary);
	
	CODEGEN(n->a);	    /* ref */
	CG_TYPEOF();	    /* str */

	n->node.is = CG_TYPE_STRING;
	n->node.maxstack = n->a->maxstack;
}

/* 11.4.4 */
static void
UnaryExpression_preinc_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct Unary_node *n = CAST_NODE(na, Unary);

	/* Note: Makes no sense to check n->a is already a value */
	CODEGEN(n->a);	/* aref */
	CG_DUP();	/* aref aref */
	CG_GETVALUE();	/* aref aval */
	CG_TONUMBER();	/* aref anum */
	CG_NUMBER(1);	/* aref anum 1 */
	CG_ADD();	/* aref anum+1 */
	CG_DUP();	/* aref anum+1 anum+1 */
	CG_ROLL3();	/* anum+1 aref anum+1 */
	CG_PUTVALUE();	/* anum+1 */

	n->node.is = CG_TYPE_NUMBER;
	n->node.maxstack = MAX(n->a->maxstack, 3);
}

/* 11.4.5 */
static void
UnaryExpression_predec_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct Unary_node *n = CAST_NODE(na, Unary);

	/* Note: Makes no sense to check n->a is already a value */
	CODEGEN(n->a);	/* aref */
	CG_DUP();	/* aref aref */
	CG_GETVALUE();	/* aref aval */
	CG_TONUMBER();	/* aref anum */
	CG_NUMBER(1);	/* aref anum 1 */
	CG_SUB();	/* aref anum-1 */
	CG_DUP();	/* aref anum-1 anum-1 */
	CG_ROLL3();   	/* anum-1 aref anum-1 */
	CG_PUTVALUE();	/* anum-1 */

	n->node.is = CG_TYPE_NUMBER;
	n->node.maxstack = MAX(n->a->maxstack, 3);
}

/* 11.4.6 */
static void
UnaryExpression_plus_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct Unary_node *n = CAST_NODE(na, Unary);

	CODEGEN(n->a);		/* aref */
	if (!CG_IS_VALUE(n->a))
	    CG_GETVALUE();	/* aval */
	if (!CG_IS_NUMBER(n->a))
	    CG_TONUMBER();	/* anum */

	n->node.is = CG_TYPE_NUMBER;
	n->node.maxstack = n->a->maxstack;
}

/* 11.4.7 */
static void
UnaryExpression_minus_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct Unary_node *n = CAST_NODE(na, Unary);

	CODEGEN(n->a);		/* aref */
	if (!CG_IS_VALUE(n->a))
	    CG_GETVALUE();	/* aval */
	if (!CG_IS_NUMBER(n->a))
	    CG_TONUMBER();	/* anum */
	CG_NEG();		/* -anum */

	n->node.is = CG_TYPE_NUMBER;
	n->node.maxstack = n->a->maxstack;
}

/* 11.4.8 */
static void
UnaryExpression_inv_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct Unary_node *n = CAST_NODE(na, Unary);

	CODEGEN(n->a);		/* aref */
	if (!CG_IS_VALUE(n->a))
	    CG_GETVALUE();	/* aval */
	CG_INV();		/* ~aval */

	n->node.is = CG_TYPE_NUMBER;
	n->node.maxstack = n->a->maxstack;
}

/* 11.4.9 */
static void
UnaryExpression_not_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct Unary_node *n = CAST_NODE(na, Unary);

	CODEGEN(n->a);		    /* aref */
	if (!CG_IS_VALUE(n->a))
	    CG_GETVALUE();	    /* aval */
	if (!CG_IS_BOOLEAN(n->a))
	    CG_TOBOOLEAN();	    /* abool */
	CG_NOT();		    /* !abool */

	n->node.is = CG_TYPE_BOOLEAN;
	n->node.maxstack = n->a->maxstack;
}

static void
Binary_common_codegen(n, cc)
	struct Binary_node *n;
	struct code_context *cc;
{
	CODEGEN(n->a);	    /* aref */
	if (!CG_IS_VALUE(n->a))
	    CG_GETVALUE();  /* aval */
	CODEGEN(n->b);	    /* aval bref */
	if (!CG_IS_VALUE(n->b))
	    CG_GETVALUE();  /* aval bval */
}

static void
MultiplicativeExpression_common_codegen(n, cc)
	struct Binary_node *n;
	struct code_context *cc;
{
	Binary_common_codegen(n, cc); /* val val */
	if (!CG_IS_NUMBER(n->a)) {
	    /* Exchanges needed to match spec semantics */
	    CG_EXCH();	    /* bval aval */
	    CG_TONUMBER();  /* bval anum */
	    CG_EXCH();	    /* anum bval */
	}
	if (!CG_IS_NUMBER(n->b))
	    CG_TONUMBER();  /* anum bnum */

	n->node.is = CG_TYPE_NUMBER;
	n->node.maxstack = MAX(n->a->maxstack, 1 + n->b->maxstack);
}

/* 11.5.1 */
static void
MultiplicativeExpression_mul_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct Binary_node *n = CAST_NODE(na, Binary);

	MultiplicativeExpression_common_codegen(n, cc); /* num num */
	CG_MUL();	    /* num */
}

/* 11.5.2 */
static void
MultiplicativeExpression_div_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct Binary_node *n = CAST_NODE(na, Binary);

	MultiplicativeExpression_common_codegen(n, cc); /* num num */
	CG_DIV();	    /* num */
}

/* 11.5.3 */
static void
MultiplicativeExpression_mod_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct Binary_node *n = CAST_NODE(na, Binary);

	MultiplicativeExpression_common_codegen(n, cc); /* num num */
	CG_MOD();	    /* num */
}

/* 11.6.1 */
static void
AdditiveExpression_add_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct Binary_node *n = CAST_NODE(na, Binary);

	Binary_common_codegen(n, cc); /* val val */
	if (!CG_IS_PRIMITIVE(n->a)) {
	    CG_EXCH();		/* bval aval */
	    CG_TOPRIMITIVE();	/* bval aprim */
	    CG_EXCH();		/* aprim bval */
	}
	if (!CG_IS_PRIMITIVE(n->b))
	    CG_TOPRIMITIVE();	/* aprim bprim */
	CG_ADD();		/* val */

	/* Carefully figure out if the result type can be restricted */
	if (CG_IS_STRING(n->a) || CG_IS_STRING(n->b))
	    n->node.is = CG_TYPE_STRING;
	else if (CG_IS_PRIMITIVE(n->a) && CG_IS_PRIMITIVE(n->b))
	    n->node.is = CG_TYPE_NUMBER;
	else
	    n->node.is = CG_TYPE_STRING | CG_TYPE_NUMBER;
	n->node.maxstack = MAX(n->a->maxstack, 1 + n->b->maxstack);
}

/* 11.6.2 */
static void
AdditiveExpression_sub_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct Binary_node *n = CAST_NODE(na, Binary);

	Binary_common_codegen(n, cc); /* aval bval */
	if (!CG_IS_NUMBER(n->a)) {
	    CG_EXCH();	    /* bval aval */
	    CG_TONUMBER();  /* bval anum */
	    CG_EXCH();	    /* anum bval */
	}
	if (!CG_IS_NUMBER(n->b))
	    CG_TONUMBER();  /* anum bnum */
	CG_SUB();	    /* num */

	n->node.is = CG_TYPE_NUMBER;
	n->node.maxstack = MAX(n->a->maxstack, 1 + n->b->maxstack);
}

/* 11.7.1 */
static void
ShiftExpression_lshift_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct Binary_node *n = CAST_NODE(na, Binary);

	Binary_common_codegen(n, cc); /* aval bval */
	CG_LSHIFT();		/* num */

	n->node.is = CG_TYPE_NUMBER;
	n->node.maxstack = MAX(n->a->maxstack, 1 + n->b->maxstack);
}

/* 11.7.2 */
static void
ShiftExpression_rshift_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct Binary_node *n = CAST_NODE(na, Binary);

	Binary_common_codegen(n, cc); /* aval bval */
	CG_RSHIFT();		/* num */

	n->node.is = CG_TYPE_NUMBER;
	n->node.maxstack = MAX(n->a->maxstack, 1 + n->b->maxstack);
}

/* 11.7.3 */
static void
ShiftExpression_urshift_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct Binary_node *n = CAST_NODE(na, Binary);

	Binary_common_codegen(n, cc); /* aval bval */
	CG_URSHIFT();		      /* num */

	n->node.is = CG_TYPE_NUMBER;
	n->node.maxstack = MAX(n->a->maxstack, 1 + n->b->maxstack);
}

/* 11.8.1 */
static void
RelationalExpression_lt_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct Binary_node *n = CAST_NODE(na, Binary);

	Binary_common_codegen(n, cc); /* aval bval */
	CG_LT();		      /* bool */

	n->node.is = CG_TYPE_BOOLEAN;
	n->node.maxstack = MAX(n->a->maxstack, 1 + n->b->maxstack);
}

/* 11.8.2 */
static void
RelationalExpression_gt_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct Binary_node *n = CAST_NODE(na, Binary);

	Binary_common_codegen(n, cc); /* aval bval */
	CG_GT();		      /* bool */

	n->node.is = CG_TYPE_BOOLEAN;
	n->node.maxstack = MAX(n->a->maxstack, 1 + n->b->maxstack);
}

/* 11.8.3 */
static void
RelationalExpression_le_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct Binary_node *n = CAST_NODE(na, Binary);

	Binary_common_codegen(n, cc); /* aval bval */
	CG_LE();		      /* bool */

	n->node.is = CG_TYPE_BOOLEAN;
	n->node.maxstack = MAX(n->a->maxstack, 1 + n->b->maxstack);
}

/* 11.8.4 */
static void
RelationalExpression_ge_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct Binary_node *n = CAST_NODE(na, Binary);

	Binary_common_codegen(n, cc); /* aval bval */
	CG_GE();		      /* bool */

	n->node.is = CG_TYPE_BOOLEAN;
	n->node.maxstack = MAX(n->a->maxstack, 1 + n->b->maxstack);
}

/* 11.8.6 */
static void
RelationalExpression_instanceof_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct Binary_node *n = CAST_NODE(na, Binary);

	Binary_common_codegen(n, cc); /* aval bval */
	CG_INSTANCEOF();	      /* bool */

	n->node.is = CG_TYPE_BOOLEAN;
	n->node.maxstack = MAX(n->a->maxstack, 1 + n->b->maxstack);
}

/* 11.8.7 */
static void
RelationalExpression_in_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct Binary_node *n = CAST_NODE(na, Binary);

	/* Binary_common_codegen(n, cc); */
	CODEGEN(n->a);			/* aref */
	if (!CG_IS_VALUE(n->a))
	    CG_GETVALUE();		/* aval */
	if (!CG_IS_STRING(n->a))
	    CG_TOSTRING();		/* astr */
	CODEGEN(n->b);			/* astr bref */
	if (!CG_IS_VALUE(n->b))
	    CG_GETVALUE();		/* aval bval */
	CG_IN();		        /* bool */

	n->node.is = CG_TYPE_BOOLEAN;
	n->node.maxstack = MAX(n->a->maxstack, 1 + n->b->maxstack);
}

/* 11.9.1 */
static void
EqualityExpression_eq_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct Binary_node *n = CAST_NODE(na, Binary);

	Binary_common_codegen(n, cc); /* aval bval */
	CG_EQ();		      /* bool */

	n->node.is = CG_TYPE_BOOLEAN;
	n->node.maxstack = MAX(n->a->maxstack, 1 + n->b->maxstack);
}

/* 11.9.2 */
static void
EqualityExpression_ne_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct Binary_node *n = CAST_NODE(na, Binary);

	Binary_common_codegen(n, cc); /* aval bval */
	CG_EQ();		      /* bool */
	CG_NOT();		      /* bool */

	n->node.is = CG_TYPE_BOOLEAN;
	n->node.maxstack = MAX(n->a->maxstack, 1 + n->b->maxstack);
}

/* 11.9.4 */
static void
EqualityExpression_seq_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct Binary_node *n = CAST_NODE(na, Binary);

	Binary_common_codegen(n, cc); /* aval bval */
	CG_SEQ();		      /* bool */

	n->node.is = CG_TYPE_BOOLEAN;
	n->node.maxstack = MAX(n->a->maxstack, 1 + n->b->maxstack);
}

/* 11.9.5 */
static void
EqualityExpression_sne_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct Binary_node *n = CAST_NODE(na, Binary);

	Binary_common_codegen(n, cc); /* aval bval */
	CG_SEQ();		      /* bool */
	CG_NOT();		      /* bool */

	n->node.is = CG_TYPE_BOOLEAN;
	n->node.maxstack = MAX(n->a->maxstack, 1 + n->b->maxstack);
}

/* 11.10 */
static void
BitwiseANDExpression_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct Binary_node *n = CAST_NODE(na, Binary);

	Binary_common_codegen(n, cc); /* aval bval */
	CG_BAND();		      /* num */

	n->node.is = CG_TYPE_NUMBER;
	n->node.maxstack = MAX(n->a->maxstack, 1 + n->b->maxstack);
}

/* 11.10 */
static void
BitwiseXORExpression_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct Binary_node *n = CAST_NODE(na, Binary);

	Binary_common_codegen(n, cc); /* aval bval */
	CG_BXOR();		      /* num */

	n->node.is = CG_TYPE_NUMBER;
	n->node.maxstack = MAX(n->a->maxstack, 1 + n->b->maxstack);
}

/* 11.10 */
static void
BitwiseORExpression_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct Binary_node *n = CAST_NODE(na, Binary);

	Binary_common_codegen(n, cc); /* aval bval */
	CG_BOR();		      /* num */

	n->node.is = CG_TYPE_NUMBER;
	n->node.maxstack = MAX(n->a->maxstack, 1 + n->b->maxstack);
}

/* 11.11 */
static void
LogicalANDExpression_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	SEE_code_patchable_t L1, L2;

	CODEGEN(n->a);				/* ref */
	if (!CG_IS_VALUE(n->a))
	    CG_GETVALUE();			/* val */
	if (!CG_IS_BOOLEAN(n->a))
	    CG_TOBOOLEAN();			/* bool */
	CG_B_TRUE_f(L1);			/* -  (L1)*/
	CG_FALSE();				/* false */
	CG_B_ALWAYS_f(L2);			/* false (2) */

	CG_LABEL(L1);				/* 1: - */
	CODEGEN(n->b);				/* ref */
	if (!CG_IS_VALUE(n->b))
	    CG_GETVALUE();			/* val */
	if (!CG_IS_BOOLEAN(n->b))
	    CG_TOBOOLEAN();			/* bool */
	CG_LABEL(L2);				/* 2: bool */

	n->node.is = CG_TYPE_BOOLEAN;
	n->node.maxstack = MAX(n->a->maxstack, n->b->maxstack);
}

static void
LogicalORExpression_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	SEE_code_patchable_t L1, L2;

	CODEGEN(n->a);				/* ref */
	if (!CG_IS_VALUE(n->a))
	    CG_GETVALUE();			/* val */
	if (!CG_IS_BOOLEAN(n->a))
	    CG_TOBOOLEAN();			/* bool */
	CG_B_TRUE_f(L1);		 	/* -  (1)*/

	CODEGEN(n->b);				/* ref */
	if (!CG_IS_VALUE(n->b))
	    CG_GETVALUE();			/* val */
	if (!CG_IS_BOOLEAN(n->b))
	    CG_TOBOOLEAN();			/* bool */

	CG_B_ALWAYS_f(L2);

    CG_LABEL(L1);				/* 1: - */
	CG_TRUE();				/* true */
    CG_LABEL(L2);				/* 2: bool */

	n->node.is = CG_TYPE_BOOLEAN;
	n->node.maxstack = MAX(n->a->maxstack, n->b->maxstack);
}

/* 11.12 */
static void
ConditionalExpression_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct ConditionalExpression_node *n = 
		CAST_NODE(na, ConditionalExpression);
	SEE_code_patchable_t L1, L2;

	CODEGEN(n->a);				/*     ref      */
	if (!CG_IS_VALUE(n->a))
	    CG_GETVALUE();			/*     val      */
	if (!CG_IS_BOOLEAN(n->a))
	    CG_TOBOOLEAN();			/*     bool     */
	CG_B_TRUE_f(L1);			/*     -    (1) */

	/* The false branch */
	CODEGEN(n->c);				/*     ref      */
	if (!CG_IS_VALUE(n->c))
	    CG_GETVALUE();			/*     val      */
	CG_B_ALWAYS_f(L2);			/*     val  (2) */

	/* The true branch */
	CG_LABEL(L1);				/* 1:  -        */
	CODEGEN(n->b);				/*     ref      */
	if (!CG_IS_VALUE(n->b))
	    CG_GETVALUE();			/*     val      */

	CG_LABEL(L2);				/* 2:  val      */

	if (!CG_IS_VALUE(n->b) || !CG_IS_VALUE(n->c))
	    n->node.is = CG_TYPE_VALUE;
	else
	    n->node.is = n->b->is | n->c->is;
	n->node.maxstack = MAX3(n->a->maxstack, n->b->maxstack, n->c->maxstack);
}

static void
AssignmentExpression_common_codegen_pre(n, cc)	/* - | ref num num */
	struct AssignmentExpression_node *n;
	struct code_context *cc;
{
	CODEGEN(n->lhs);	/* ref */
	CG_DUP();		/* ref ref */
	CG_GETVALUE();		/* ref val */
	CG_TONUMBER();		/* ref num */
	CODEGEN(n->expr);	/* ref num ref */
	if (!CG_IS_VALUE(n->expr))
	    CG_GETVALUE();	/* ref num val */
	if (!CG_IS_NUMBER(n->expr))
	    CG_TONUMBER();	/* ref num num */
}

static void
AssignmentExpression_common_codegen_shiftpre(n, cc)	/* - | ref val val */
	struct AssignmentExpression_node *n;
	struct code_context *cc;
{
	CODEGEN(n->lhs);	/* ref */
	CG_DUP();		/* ref ref */
	CG_GETVALUE();		/* ref val */
	CODEGEN(n->expr);	/* ref num ref */
	if (!CG_IS_VALUE(n->expr))
	    CG_GETVALUE();	/* ref num val */
}

static void
AssignmentExpression_common_codegen_post(n, cc) /* ref val | val */
	struct AssignmentExpression_node *n;
	struct code_context *cc;
{
	CG_DUP();		/* ref val val */
	CG_ROLL3();   		/* val ref val */
	CG_PUTVALUE();		/* val */
	n->node.maxstack = MAX(n->lhs->maxstack, 2 + n->expr->maxstack);

	/* Peephole optimisation note:
	 *                  ref val
	 *   DUP	    ref val val
	 *   ROLL3   	    val ref val
	 *   PUTVALUE	    val
	 *   POP	    -
	 *
	 * is equivalent to:
	 *                  ref val
	 *   PUTVALUE       -
	 * 
	 * The backend could check for this when the 'POP' instruction
	 * is generated.
	 */

	/* Peephole optimisation note:
	 *                  ref val
	 *   DUP	    ref val val
	 *   ROLL3   	    val ref val
	 *   PUTVALUE	    val
	 *   SETC	    -
	 *
	 * is equivalent to:
	 *                  ref val
	 *   DUP	    ref val val
	 *   SETC	    ref val
	 *   PUTVALUE       -
	 * 
	 * The backend could check for this when the 'SETC' instruction
	 * is generated.
	 */

}

/* 11.13.2 */
static void
AssignmentExpression_simple_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct AssignmentExpression_node *n = 
		CAST_NODE(na, AssignmentExpression);

	CODEGEN(n->lhs);	/* ref */
	CODEGEN(n->expr);	/* ref ref */
	if (!CG_IS_VALUE(n->expr))
	    CG_GETVALUE();	/* ref val */
	AssignmentExpression_common_codegen_post(n, cc);/* val */
	n->node.is = !CG_IS_VALUE(n->expr) ?  CG_TYPE_VALUE : n->expr->is;
}

/* 11.13.2 */
static void
AssignmentExpression_muleq_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct AssignmentExpression_node *n = 
		CAST_NODE(na, AssignmentExpression);

	AssignmentExpression_common_codegen_pre(n, cc);	/* ref num num */
	CG_MUL();
	AssignmentExpression_common_codegen_post(n, cc);/* val */
	n->node.is = CG_TYPE_NUMBER;
}

/* 11.13.2 */
static void
AssignmentExpression_diveq_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct AssignmentExpression_node *n = 
		CAST_NODE(na, AssignmentExpression);

	AssignmentExpression_common_codegen_pre(n, cc);	/* ref num num */
	CG_DIV();					/* ref num */
	AssignmentExpression_common_codegen_post(n, cc);/* val */
	n->node.is = CG_TYPE_NUMBER;
}

/* 11.13.2 */
static void
AssignmentExpression_modeq_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct AssignmentExpression_node *n = 
		CAST_NODE(na, AssignmentExpression);

	AssignmentExpression_common_codegen_pre(n, cc);	/* ref num num */
	CG_MOD();					/* ref num */
	AssignmentExpression_common_codegen_post(n, cc);/* val */
	n->node.is = CG_TYPE_NUMBER;
}

/* 11.13.2 */
static void
AssignmentExpression_addeq_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct AssignmentExpression_node *n = 
		CAST_NODE(na, AssignmentExpression);

	CODEGEN(n->lhs);	/* ref1 */
	CG_DUP();		/* ref1 ref1 */
	CG_GETVALUE();		/* ref1 val1 */
	CODEGEN(n->expr);	/* ref1 val1 ref2 */
	if (!CG_IS_VALUE(n->expr))
	    CG_GETVALUE();	/* ref1 val1 val2 */
	CG_EXCH();		/* ref1 val2 val1 */
	CG_TOPRIMITIVE();	/* ref1 val2 prim1 */
	CG_EXCH();		/* ref1 prim1 val2 */
	if (!CG_IS_PRIMITIVE(n->expr))
	    CG_TOPRIMITIVE();	/* ref1 prim1 prim2 */
	CG_ADD();		/* ref1 prim3 */
	AssignmentExpression_common_codegen_post(n, cc);/* prim3 */

	if (CG_IS_STRING(n->expr))
	    n->node.is = CG_TYPE_STRING;
	else 
	    n->node.is = CG_TYPE_STRING | CG_TYPE_NUMBER;
}

/* 11.13.2 */
static void
AssignmentExpression_subeq_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct AssignmentExpression_node *n = 
		CAST_NODE(na, AssignmentExpression);

	AssignmentExpression_common_codegen_pre(n, cc);	/* ref num num */
	CG_SUB();
	AssignmentExpression_common_codegen_post(n, cc);/* val */
	n->node.is = CG_TYPE_NUMBER;
}

/* 11.13.2 */
static void
AssignmentExpression_lshifteq_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct AssignmentExpression_node *n = 
		CAST_NODE(na, AssignmentExpression);

	AssignmentExpression_common_codegen_shiftpre(n, cc);/* ref val val */
	CG_LSHIFT();					    /* ref num */
	AssignmentExpression_common_codegen_post(n, cc);    /* num */
	n->node.is = CG_TYPE_NUMBER;
}

/* 11.13.2 */
static void
AssignmentExpression_rshifteq_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct AssignmentExpression_node *n = 
		CAST_NODE(na, AssignmentExpression);

	AssignmentExpression_common_codegen_shiftpre(n, cc);/* ref val val */
	CG_RSHIFT();					    /* ref num */
	AssignmentExpression_common_codegen_post(n, cc);    /* num */
	n->node.is = CG_TYPE_NUMBER;
}

/* 11.13.2 */
static void
AssignmentExpression_urshifteq_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct AssignmentExpression_node *n = 
		CAST_NODE(na, AssignmentExpression);

	AssignmentExpression_common_codegen_shiftpre(n, cc);/* ref val val */
	CG_URSHIFT();					    /* ref num */
	AssignmentExpression_common_codegen_post(n, cc);    /* num */
	n->node.is = CG_TYPE_NUMBER;
}

/* 11.13.2 */
static void
AssignmentExpression_andeq_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct AssignmentExpression_node *n = 
		CAST_NODE(na, AssignmentExpression);

	AssignmentExpression_common_codegen_pre(n, cc);	    /* ref num num */
	CG_BAND();					    /* ref num */
	AssignmentExpression_common_codegen_post(n, cc);    /* num */
	n->node.is = CG_TYPE_NUMBER;
}

/* 11.13.2 */
static void
AssignmentExpression_xoreq_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct AssignmentExpression_node *n = 
		CAST_NODE(na, AssignmentExpression);

	AssignmentExpression_common_codegen_pre(n, cc);	    /* ref num num */
	CG_BXOR();					    /* ref num */
	AssignmentExpression_common_codegen_post(n, cc);    /* num */
	n->node.is = CG_TYPE_NUMBER;
}

/* 11.13.2 */
static void
AssignmentExpression_oreq_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct AssignmentExpression_node *n = 
		CAST_NODE(na, AssignmentExpression);

	AssignmentExpression_common_codegen_pre(n, cc);	    /* ref num num */
	CG_BOR();					    /* ref num */
	AssignmentExpression_common_codegen_post(n, cc);    /* num */
	n->node.is = CG_TYPE_NUMBER;

}

/* 11.14 */
static void
Expression_comma_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct Binary_node *n = CAST_NODE(na, Binary);

	CODEGEN(n->a);		/* ref */
	if (!CG_IS_VALUE(n->a))
	    CG_GETVALUE();	/* val */
	CG_POP();		/* -   */
	CODEGEN(n->b);		/* ref */
	if (!CG_IS_VALUE(n->b))
	    CG_GETVALUE();	/* val */

	n->node.is = CG_IS_VALUE(n->b) ? n->b->is : CG_TYPE_VALUE;
	n->node.maxstack = MAX(n->a->maxstack, n->b->maxstack);
}

/* 12.1 */
static void
Block_empty_codegen(n, cc)
	struct node *n;
	struct code_context *cc;
{
						/* - | - */
	n->maxstack = 0;
}

/* 12.1 */
static void
StatementList_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct Binary_node *n = CAST_NODE(na, Binary);

	CODEGEN(n->a);				 /*    -      */
	CODEGEN(n->b);				 /*    -      */
	n->node.maxstack = MAX(n->a->maxstack, n->b->maxstack);
}

/* 12.2 */
static void
VariableStatement_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct Unary_node *n = CAST_NODE(na, Unary);

	/* Note: VariableDeclaration leaves nothing on the stack */
	CG_LOC(&na->location);
	CODEGEN(n->a);	    /* - */
	n->node.maxstack = n->a->maxstack;
}

/* 12.2 */
static void
VariableDeclarationList_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct Binary_node *n = CAST_NODE(na, Binary);

	CODEGEN(n->a);			/* - */
	CODEGEN(n->b);			/* - */
	n->node.maxstack = MAX(n->a->maxstack, n->b->maxstack);
}

/* 12.2 */
static void
VariableDeclaration_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct VariableDeclaration_node *n = 
		CAST_NODE(na, VariableDeclaration);
	if (n->init) {
		if (cg_var_is_in_scope(cc, n->var->name)) 
		    CG_VREF(cg_var_id(cc, n->var->name));    /* ref */
		else {
		    CG_STRING(n->var->name);		    /* str */
		    CG_LOOKUP();			    /* ref */
		}
		CODEGEN(n->init);			    /* ref ref */
		if (!CG_IS_VALUE(n->init))
		    CG_GETVALUE();			    /* ref val */
		CG_PUTVALUE();				    /* - */
	}
	n->node.maxstack = n->init ? 1 + n->init->maxstack : 0;
}

/* 12.3 */
static void
EmptyStatement_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	CG_LOC(&na->location);
	/* CG_NOP(); */		/* - */
	na->maxstack = 0;
}

/* 12.4 */
static void
ExpressionStatement_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct Unary_node *n = CAST_NODE(na, Unary);

	CG_LOC(&na->location);
	CODEGEN(n->a);			/* ref */
	if (!CG_IS_VALUE(n->a))
	    CG_GETVALUE();		/* val */
	CG_SETC();			/* -   */

	n->node.maxstack = n->a->maxstack;
}

/* 12.5 */
static void
IfStatement_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct IfStatement_node *n = CAST_NODE(na, IfStatement);
	SEE_code_patchable_t L1, L2;

	CG_LOC(&na->location);
	CODEGEN(n->cond);			/*     ref      */
	if (!CG_IS_VALUE(n->cond))
	    CG_GETVALUE();			/*     val      */
	if (!CG_IS_BOOLEAN(n->cond))
	    CG_TOBOOLEAN();			/*     bool     */
	CG_B_TRUE_f(L1);			/*     -   (L1) */
	if (n->bfalse)
	    CODEGEN(n->bfalse);			/*     -        */
	CG_B_ALWAYS_f(L2);			/*     -   (L2) */
    CG_LABEL(L1);				/* L1: -        */
	CODEGEN(n->btrue);			/*     -        */
    CG_LABEL(L2);				/* L2: -        */

	n->node.maxstack = MAX3(n->cond->maxstack,
	    n->btrue->maxstack, n->bfalse ? n->bfalse->maxstack : 0);
}

/* 12.6.1 */
static void
IterationStatement_dowhile_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct IterationStatement_while_node *n = 
		CAST_NODE(na, IterationStatement_while);
	SEE_code_addr_t L1, L2, L3;

	push_patchables(cc, n->target, CONTINUABLE);

    L1 = CG_HERE();
	CODEGEN(n->body);
    L2 = CG_HERE();			    /* continue point */
	CG_LOC(&na->location);
	CODEGEN(n->cond);
	if (!CG_IS_VALUE(n->cond))
	    CG_GETVALUE();
	CG_B_TRUE_b(L1);
    L3 = CG_HERE();			    /* break point */

	pop_patchables(cc, L2, L3);

	na->maxstack = MAX(n->cond->maxstack, n->body->maxstack);
}

/* 12.6.2 */
static void
IterationStatement_while_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct IterationStatement_while_node *n = 
		CAST_NODE(na, IterationStatement_while);
	SEE_code_patchable_t P1;
	SEE_code_addr_t L1, L2, L3;

	push_patchables(cc, n->target, CONTINUABLE);

	CG_B_ALWAYS_f(P1);
    L1 = CG_HERE();
	CODEGEN(n->body);
    CG_LABEL(P1);
    L2 = CG_HERE();			    /* continue point */
	CG_LOC(&na->location);
	CODEGEN(n->cond);
	if (!CG_IS_VALUE(n->cond))
	    CG_GETVALUE();
	CG_B_TRUE_b(L1);
    L3 = CG_HERE();			    /* break point */

	pop_patchables(cc, L2, L3);

	na->maxstack = MAX(n->cond->maxstack, n->body->maxstack);
}

/* 12.6.3 for (init; cond; incr) body */
static void
IterationStatement_for_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct IterationStatement_for_node *n = 
		CAST_NODE(na, IterationStatement_for);
	SEE_code_patchable_t P1;
	SEE_code_addr_t L1, L2, L3;

	push_patchables(cc, n->target, CONTINUABLE);

	if (n->init) {
		CG_LOC(&n->init->location);
		CODEGEN(n->init);
		if (!CG_IS_VALUE(n->init))
		    CG_GETVALUE();
		CG_POP();
	}
	CG_B_ALWAYS_f(P1);
    L1 = CG_HERE();
	CODEGEN(n->body);
    L2 = CG_HERE();			    /* continue point */
	if (n->incr) {
		CG_LOC(&n->incr->location);
		CODEGEN(n->incr);
		if (!CG_IS_VALUE(n->incr))
		    CG_GETVALUE();
		CG_POP();
	}
    CG_LABEL(P1);
	if (n->cond) {
	    CG_LOC(&n->cond->location);
	    CODEGEN(n->cond);
	    if (!CG_IS_VALUE(n->cond))
		CG_GETVALUE();
	    CG_B_TRUE_b(L1);
	} else {
	    CG_LOC(&na->location);
	    CG_B_ALWAYS_b(L1);
	}
    L3 = CG_HERE();			    /* break point */

	pop_patchables(cc, L2, L3);

	na->maxstack = MAX(
	    MAX(n->incr ? n->incr->maxstack : 0,
		n->init ? n->init->maxstack : 0),
	    MAX(n->cond ? n->cond->maxstack : 0,
		n->body->maxstack));
}

/* 12.6.3 for (var init; cond; incr) body */
static void
IterationStatement_forvar_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct IterationStatement_for_node *n = 
		CAST_NODE(na, IterationStatement_for);
	SEE_code_patchable_t P1;
	SEE_code_addr_t L1, L2, L3;

	push_patchables(cc, n->target, CONTINUABLE);

	CG_LOC(&n->init->location);
	CODEGEN(n->init);
	if (!CG_IS_VALUE(n->init))
	    CG_GETVALUE();
	CG_B_ALWAYS_f(P1);
    L1 = CG_HERE();
	CODEGEN(n->body);
    L2 = CG_HERE();			    /* continue point */
	if (n->incr) {
		CG_LOC(&n->incr->location);
		CODEGEN(n->incr);
		if (!CG_IS_VALUE(n->incr))
		    CG_GETVALUE();
		CG_POP();
	}
    CG_LABEL(P1);
	if (n->cond) {
	    CG_LOC(&n->cond->location);
	    CODEGEN(n->cond);
	    if (!CG_IS_VALUE(n->cond))
		CG_GETVALUE();
	    CG_B_TRUE_b(L1);
	} else {
	    CG_LOC(&na->location);
	    CG_B_ALWAYS_b(L1);
	}
    L3 = CG_HERE();			    /* break point */

	pop_patchables(cc, L2, L3);

	na->maxstack = MAX(
	    MAX(n->incr ? n->incr->maxstack : 0,
		n->init->maxstack),
	    MAX(n->cond ? n->cond->maxstack : 0,
		n->body->maxstack));
}

/* 12.6.3 for (lhs in list) body */
static void
IterationStatement_forin_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct IterationStatement_forin_node *n = 
		CAST_NODE(na, IterationStatement_forin);
	SEE_code_patchable_t P1;
	SEE_code_addr_t L1, L2, L3;

	CG_LOC(&na->location);
	CODEGEN(n->list);		/* ref */
	if (!CG_IS_VALUE(n->list))
	    CG_GETVALUE();		/* val */
	if (!CG_IS_OBJECT(n->list))
	    CG_TOOBJECT();		/* obj */

	CG_S_ENUM();			/* -  */
	cg_block_enter(cc);

	push_patchables(cc, n->target, CONTINUABLE);

	CG_B_ALWAYS_f(P1);

    L1 = CG_HERE();
	CODEGEN(n->lhs);		/* str ref */
	CG_EXCH();			/* ref str */
	CG_PUTVALUE();			/* - */

	CODEGEN(n->body);

    L2 = CG_HERE();			/* continue point */
    CG_LABEL(P1);
	CG_B_ENUM_b(L1);

    L3 = CG_HERE();			/* break point */
	pop_patchables(cc, L2, L3);

	CG_END(cg_block_current(cc));
	cg_block_leave(cc);

	na->maxstack = MAX4(
	    2,
	    n->list->maxstack,
	    1 + n->lhs->maxstack,
	    n->body->maxstack);
}

/* 12.6.3 for (var lhs in list) body */
static void
IterationStatement_forvarin_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct IterationStatement_forin_node *n = 
		CAST_NODE(na, IterationStatement_forin);
	struct VariableDeclaration_node *lhs 
		= CAST_NODE(n->lhs, VariableDeclaration);
	SEE_code_patchable_t P1;
	SEE_code_addr_t L1, L2, L3;

	CG_LOC(&na->location);
	CODEGEN(n->lhs);		/* - */
	CODEGEN(n->list);		/* ref */
	if (!CG_IS_VALUE(n->list))
	    CG_GETVALUE();		/* val */
	if (!CG_IS_OBJECT(n->list))
	    CG_TOOBJECT();		/* obj */

	CG_S_ENUM();			/* -  */
	cg_block_enter(cc);

	push_patchables(cc, n->target, CONTINUABLE);

	CG_B_ALWAYS_f(P1);

    L1 = CG_HERE();
	if (cg_var_is_in_scope(cc, lhs->var->name)) 
	    CG_VREF(cg_var_id(cc, lhs->var->name));    /* ref */
	else {
	    CG_STRING(lhs->var->name);		    /* str */
	    CG_LOOKUP();			    /* ref */
	}
	CG_EXCH();			/* ref str */
	CG_PUTVALUE();			/* - */

	CODEGEN(n->body);

    L2 = CG_HERE();			/* continue point */
    CG_LABEL(P1);
	CG_B_ENUM_b(L1);

    L3 = CG_HERE();			/* break point */
	pop_patchables(cc, L2, L3);
	CG_END(cg_block_current(cc));
	cg_block_leave(cc);

	na->maxstack = MAX4(
	    2,
	    n->list->maxstack,
	    1 + n->lhs->maxstack,
	    n->body->maxstack);
}

/* 12.7 */
static void
ContinueStatement_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct ContinueStatement_node *n = CAST_NODE(na, ContinueStatement);
	struct patchables *patchables;
	SEE_code_patchable_t pa;

	patchables = patch_find(cc, n->target, PATCH_FIND_CONTINUE);
	
	CG_LOC(&na->location);

	/* Generate an END instruction if we are continuing to an outer block */
	if (patchables->block_depth < cc->block_depth)
	    CG_END(patchables->block_depth+1);

	CG_B_ALWAYS_f(pa);
	patch_add_continue(cc, patchables, pa);

	n->node.maxstack = 0;
}

/* 12.8 */
static void
BreakStatement_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct BreakStatement_node *n = CAST_NODE(na, BreakStatement);
	struct patchables *patchables;
	SEE_code_patchable_t pa;

	patchables = patch_find(cc, n->target, PATCH_FIND_BREAK);

	CG_LOC(&na->location);

	/* Generate an END instruction if we are breaking to an outer block */
	if (patchables->block_depth < cc->block_depth)
	    CG_END(patchables->block_depth+1);

	CG_B_ALWAYS_f(pa);
	patch_add_break(cc, patchables, pa);

	n->node.maxstack = 0;
}

/* 12.9 */
static void
ReturnStatement_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct ReturnStatement_node *n = CAST_NODE(na, ReturnStatement);

	CG_LOC(&na->location);
	CODEGEN(n->expr);			/* ref */
	if (!CG_IS_VALUE(n->expr))
	    CG_GETVALUE();			/* val */
	CG_SETC();				/* - */
	CG_END(0);				/* (halt) */

	n->node.maxstack = n->expr->maxstack;
}

/* 12.9 */
static void
ReturnStatement_undef_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	CG_LOC(&na->location);
	CG_UNDEFINED();			    /* undef */
	CG_SETC();			    /* - */
	CG_END(0);			    /* (halt) */

	na->maxstack = 1;
}

/* 12.10 */
static void
WithStatement_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	int old_var_scope;

	CG_LOC(&na->location);
	CODEGEN(n->a);			/* ref */
	if (!CG_IS_VALUE(n->a))	
	    CG_GETVALUE();		/* val */
	if (!CG_IS_OBJECT(n->a))
	    CG_TOOBJECT();		/* obj */

	CG_S_WITH();			/* - */
	cg_block_enter(cc);
	old_var_scope = cg_var_set_all_scope(cc, 0);

	CODEGEN(n->b);

	CG_END(cg_block_current(cc));
	cg_block_leave(cc);
	cg_var_set_all_scope(cc, old_var_scope);

	na->maxstack = MAX(n->a->maxstack, n->b->maxstack);
}

/* 12.11 */
static void
SwitchStatement_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct SwitchStatement_node *n = CAST_NODE(na, SwitchStatement);
	struct case_list *c;
	int ncases, i;
	SEE_code_patchable_t *case_patches, default_patch;
	unsigned int expr_maxstack = 0, body_maxstack = 0;

	for (ncases = 0, c = n->cases; c; c = c->next)
	    if (c->expr)
		ncases++;
	case_patches = SEE_ALLOCA(cc->code->interpeter, 
	    SEE_code_patchable_t, ncases);

	CG_LOC(&na->location);
	CODEGEN(n->cond);		/* ref */
	if (!CG_IS_VALUE(n->cond))
	    CG_GETVALUE();		/* val */

	for (i = 0, c = n->cases; c; c = c->next)
	    if (c->expr) {
		CG_DUP();		/* val val */
		CODEGEN(c->expr);	/* val val ref */
		expr_maxstack = MAX(expr_maxstack, 2 + c->expr->maxstack);
		if (!CG_IS_VALUE(c->expr))
		    CG_GETVALUE();	/* val val val */
		CG_SEQ();		/* val bool */
		CG_B_TRUE_f(case_patches[i]);	/* val */
		i++;
	    }
	CG_B_ALWAYS_f(default_patch);

	push_patchables(cc, n->target, !CONTINUABLE);

	for (i = 0, c = n->cases; c; c = c->next) {
	    if (!c->expr)
		CG_LABEL(default_patch);
	    else {
		CG_LABEL(case_patches[i]);
		i++;
	    }
	    if (c->body) {
		CODEGEN(c->body);	/* val */
		body_maxstack = MAX(body_maxstack, 1 + c->body->maxstack);
	    }
	}

	/* If there was no default body, patch through to the end */
	if (!n->defcase)
	    CG_LABEL(default_patch);
	
	pop_patchables(cc, 0, CG_HERE());   /* All breaks lead here */
	CG_POP();

	na->maxstack = MAX3(n->cond->maxstack, expr_maxstack, body_maxstack);
}

/* 12.12 */
static void
LabelledStatement_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct LabelledStatement_node *n = CAST_NODE(na, LabelledStatement);
	SEE_code_addr_t L1;

	push_patchables(cc, n->target, !CONTINUABLE);
	CODEGEN(n->unary.a);
    L1 = CG_HERE();
	pop_patchables(cc, NULL, L1);
	na->maxstack = n->unary.a->maxstack;
}

/* 12.13 */
static void
ThrowStatement_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct Unary_node *n = CAST_NODE(na, Unary);

	CG_LOC(&na->location);
	CODEGEN(n->a);		/* ref */
	if (!CG_IS_VALUE(n->a))
	    CG_GETVALUE();	/* val */
	CG_THROW();		/* - */

	na->maxstack = n->a->maxstack;
}

/* 12.14 try catch */
static void
TryStatement_catch_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct TryStatement_node *n = CAST_NODE(na, TryStatement);
	SEE_code_patchable_t L1, L2;
	int in_scope;

	CG_LOC(&na->location);
	CG_STRING(n->ident);	    /* str */
	CG_S_TRYC_f(L1);	    /* - */
	cg_block_enter(cc);
	CODEGEN(n->block);	    /* - */
	CG_B_ALWAYS_f(L2);
    CG_LABEL(L1);
        /* If the catch variable was declared var then temporarily remove it
         * from the scope. */
	in_scope = cg_var_is_in_scope(cc, n->ident);
	if (in_scope)
	    cg_var_set_scope(cc, n->ident, 0);
	CG_S_CATCH();	            /* - */
	CODEGEN(n->bcatch);	    /* - */
	if (in_scope)
	    cg_var_set_scope(cc, n->ident, 1);
    CG_LABEL(L2);
	CG_END(cg_block_current(cc));
	cg_block_leave(cc);

	na->maxstack = MAX3(1, n->block->maxstack, n->bcatch->maxstack);

}

/* 12.14 try finally */
static void
TryStatement_finally_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct TryStatement_node *n = CAST_NODE(na, TryStatement);
	SEE_code_patchable_t L1, L2;

	CG_LOC(&na->location);
	CG_S_TRYF_f(L1);	    /* - */
	cg_block_enter(cc);
	CODEGEN(n->block);	    /* - */
	CG_B_ALWAYS_f(L2);
    CG_LABEL(L1);
	CG_GETC();		    /* val */
        if (cc->max_block_depth > cg_block_current(cc))
            CG_END(cg_block_current(cc)+1); 
	CODEGEN(n->bfinally);	    /* val */
	CG_SETC();		    /* - */
        CG_ENDF();
    CG_LABEL(L2);
	CG_END(cg_block_current(cc));
	cg_block_leave(cc);

	na->maxstack = MAX3(1, n->block->maxstack, 1 + n->bfinally->maxstack);

}

/* 12.14 try catch finally */
static void
TryStatement_catchfinally_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct TryStatement_node *n = CAST_NODE(na, TryStatement);
	SEE_code_patchable_t L1, L2, L3a, L3b;
	int in_scope;

	CG_LOC(&na->location);
	CG_S_TRYF_f(L1);	    /* - */
	cg_block_enter(cc);
	CG_STRING(n->ident);	    /* str */
	CG_S_TRYC_f(L2);	    /* - */
	cg_block_enter(cc);
	CODEGEN(n->block);	    /* - */
	CG_B_ALWAYS_f(L3a);
    CG_LABEL(L2);
	in_scope = cg_var_is_in_scope(cc, n->ident);
	if (in_scope)
	    cg_var_set_scope(cc, n->ident, 0);
	CG_S_CATCH();	            /* - */
	CODEGEN(n->bcatch);	    /* - */
	if (in_scope)
	    cg_var_set_scope(cc, n->ident, 1);
	CG_B_ALWAYS_f(L3b);
	cg_block_leave(cc);
    CG_LABEL(L1);
	CG_GETC();		    /* val */
        if (cc->max_block_depth > cg_block_current(cc))
            CG_END(cg_block_current(cc) + 1); 
	CODEGEN(n->bfinally);	    /* val */
	CG_SETC();		    /* - */
        CG_ENDF();
    CG_LABEL(L3a);
    CG_LABEL(L3b);
	CG_END(cg_block_current(cc));
	cg_block_leave(cc);

	na->maxstack = MAX4(1, n->block->maxstack, 
		n->bcatch->maxstack, 1 + n->bfinally->maxstack);


	/*
	 * Peephole optimizer note:
	 * Sometimes two SETCs will be generated in a row. This
	 * would be slightly faster if the first SETC were converted
	 * into a POP
	 */

}

#if 0
/* 13 */
static void
FunctionDeclaration_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct Function_node *n = CAST_NODE(na, Function);
	/* TBD - never actually called? */
}
#endif

/* 13 */
static void
FunctionExpression_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct Function_node *n = CAST_NODE(na, Function);
	int in_scope;

	if (n->function->name == NULL) {
	    CG_FUNC(n->function);	    /* obj */

	    na->maxstack = 1;
	} else {
	    /*
	     * The following creates a new mini-scope with S.WITH.
	     * The scope is inherited by the FUNC instruction. This is so
	     * the function can call itself recursively by name
	     */
	    CG_OBJECT();		    /* obj */
	    CG_DUP();			    /* obj obj */
	    CG_S_WITH();		    /* obj */
	    cg_block_enter(cc);
	    in_scope = cg_var_is_in_scope(cc, n->function->name);
	    if (in_scope)
		    cg_var_set_scope(cc, n->function->name, 0);
	    CG_STRING(n->function->name);   /* obj str */
	    CG_REF();			    /* ref */
	    CG_FUNC(n->function);	    /* ref obj */
	    CG_END(cg_block_current(cc));  
	    cg_block_leave(cc);
	    if (in_scope)
		    cg_var_set_scope(cc, n->function->name, 1);
	    CG_DUP();			    /* ref obj obj */
	    CG_ROLL3();			    /* obj ref obj */
	    CG_PUTVALUEA(SEE_ATTR_DONTDELETE | SEE_ATTR_READONLY); /* obj */

	    na->maxstack = 3;
	}
}

static void
FunctionBody_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct FunctionBody_node *n = CAST_NODE(na, FunctionBody);

	/* Note that SourceElements_codegen includes the fproc action */
	CODEGEN(n->u.a);	/* - */

	/* Non-programs convert 'normal' completion to return undefined */
	if (!n->is_program) {
	    CG_UNDEFINED();	/* undef */
	    CG_SETC();		/* - */
	}
	CG_END(0);		/* explicit return */

	na->maxstack = MAX(n->is_program ? 0 : 1, n->u.a->maxstack);
}

/* 14 */
static void
SourceElements_codegen(na, cc)
	struct node *na;
	struct code_context *cc;
{
	struct SourceElements_node *n = CAST_NODE(na, SourceElements);
	unsigned int maxstack = 0;
	struct SourceElement *e;
	struct var *v;
	struct Function_node *fn;

	/* SourceElements fproc:
	 * - create function closures of the current scope
	 * - initialise 'var's.
	 */

	for (e = n->functions; e; e = e->next) {
	    fn = CAST_NODE(e->node, Function);
	    cg_var_set_scope(cc, fn->function->name, 1);
	    CG_VREF(cg_var_id(cc, fn->function->name)); /* ref */
	    CG_FUNC(fn->function);		        /* ref obj */
	    CG_PUTVALUE();			        /* - */
	    maxstack = MAX(maxstack, 2);
	}

	for (v = n->vars; v; v = v->next) {
	    cg_var_set_scope(cc, v->name, 1);
	    maxstack = MAX(maxstack, 1);
	}

	/* SourceElements eval:
	 * - execute each statement
	 */
	for (e = n->statements; e; e = e->next) {
	    CODEGEN(e->node);
	    maxstack = MAX(maxstack, e->node->maxstack);
	}

	na->maxstack = maxstack;
}


/* Returns true if the codgen function body is empty */
int
_SEE_codegen_functionbody_isempty(interp, f)
	struct SEE_interpreter *interp;
	struct function *f;
{
	return f->body == NULL;
}

void 
_SEE_codegen_eval_functionbody(body, context, res)
        void *body;
        struct SEE_context *context;
        struct SEE_value *res;
{
        CG_EXEC((struct SEE_code *)body, context, res);
}

void (*_SEE_nodeclass_codegen[NODECLASS_MAX])(struct node *, 
        struct code_context *) = { 0
    ,0                                      /*Unary*/
    ,0                                      /*Binary*/
    ,Literal_codegen                        /*Literal*/
    ,StringLiteral_codegen                  /*StringLiteral*/
    ,RegularExpressionLiteral_codegen       /*RegularExpressionLiteral*/
    ,PrimaryExpression_this_codegen         /*PrimaryExpression_this*/
    ,PrimaryExpression_ident_codegen        /*PrimaryExpression_ident*/
    ,ArrayLiteral_codegen                   /*ArrayLiteral*/
    ,ObjectLiteral_codegen                  /*ObjectLiteral*/
    ,Arguments_codegen                      /*Arguments*/
    ,MemberExpression_new_codegen           /*MemberExpression_new*/
    ,MemberExpression_dot_codegen           /*MemberExpression_dot*/
    ,MemberExpression_bracket_codegen       /*MemberExpression_bracket*/
    ,CallExpression_codegen                 /*CallExpression*/
    ,PostfixExpression_inc_codegen          /*PostfixExpression_inc*/
    ,PostfixExpression_dec_codegen          /*PostfixExpression_dec*/
    ,UnaryExpression_delete_codegen         /*UnaryExpression_delete*/
    ,UnaryExpression_void_codegen           /*UnaryExpression_void*/
    ,UnaryExpression_typeof_codegen         /*UnaryExpression_typeof*/
    ,UnaryExpression_preinc_codegen         /*UnaryExpression_preinc*/
    ,UnaryExpression_predec_codegen         /*UnaryExpression_predec*/
    ,UnaryExpression_plus_codegen           /*UnaryExpression_plus*/
    ,UnaryExpression_minus_codegen          /*UnaryExpression_minus*/
    ,UnaryExpression_inv_codegen            /*UnaryExpression_inv*/
    ,UnaryExpression_not_codegen            /*UnaryExpression_not*/
    ,MultiplicativeExpression_mul_codegen   /*MultiplicativeExpression_mul*/
    ,MultiplicativeExpression_div_codegen   /*MultiplicativeExpression_div*/
    ,MultiplicativeExpression_mod_codegen   /*MultiplicativeExpression_mod*/
    ,AdditiveExpression_add_codegen         /*AdditiveExpression_add*/
    ,AdditiveExpression_sub_codegen         /*AdditiveExpression_sub*/
    ,ShiftExpression_lshift_codegen         /*ShiftExpression_lshift*/
    ,ShiftExpression_rshift_codegen         /*ShiftExpression_rshift*/
    ,ShiftExpression_urshift_codegen        /*ShiftExpression_urshift*/
    ,RelationalExpression_lt_codegen        /*RelationalExpression_lt*/
    ,RelationalExpression_gt_codegen        /*RelationalExpression_gt*/
    ,RelationalExpression_le_codegen        /*RelationalExpression_le*/
    ,RelationalExpression_ge_codegen        /*RelationalExpression_ge*/
    ,RelationalExpression_instanceof_codegen/*RelationalExpression_instanceof*/
    ,RelationalExpression_in_codegen        /*RelationalExpression_in*/
    ,EqualityExpression_eq_codegen          /*EqualityExpression_eq*/
    ,EqualityExpression_ne_codegen          /*EqualityExpression_ne*/
    ,EqualityExpression_seq_codegen         /*EqualityExpression_seq*/
    ,EqualityExpression_sne_codegen         /*EqualityExpression_sne*/
    ,BitwiseANDExpression_codegen           /*BitwiseANDExpression*/
    ,BitwiseXORExpression_codegen           /*BitwiseXORExpression*/
    ,BitwiseORExpression_codegen            /*BitwiseORExpression*/
    ,LogicalANDExpression_codegen           /*LogicalANDExpression*/
    ,LogicalORExpression_codegen            /*LogicalORExpression*/
    ,ConditionalExpression_codegen          /*ConditionalExpression*/
    ,0                                      /*AssignmentExpression*/
    ,AssignmentExpression_simple_codegen    /*AssignmentExpression_simple*/
    ,AssignmentExpression_muleq_codegen     /*AssignmentExpression_muleq*/
    ,AssignmentExpression_diveq_codegen     /*AssignmentExpression_diveq*/
    ,AssignmentExpression_modeq_codegen     /*AssignmentExpression_modeq*/
    ,AssignmentExpression_addeq_codegen     /*AssignmentExpression_addeq*/
    ,AssignmentExpression_subeq_codegen     /*AssignmentExpression_subeq*/
    ,AssignmentExpression_lshifteq_codegen  /*AssignmentExpression_lshifteq*/
    ,AssignmentExpression_rshifteq_codegen  /*AssignmentExpression_rshifteq*/
    ,AssignmentExpression_urshifteq_codegen /*AssignmentExpression_urshifteq*/
    ,AssignmentExpression_andeq_codegen     /*AssignmentExpression_andeq*/
    ,AssignmentExpression_xoreq_codegen     /*AssignmentExpression_xoreq*/
    ,AssignmentExpression_oreq_codegen      /*AssignmentExpression_oreq*/
    ,Expression_comma_codegen               /*Expression_comma*/
    ,Block_empty_codegen                    /*Block_empty*/
    ,StatementList_codegen                  /*StatementList*/
    ,VariableStatement_codegen              /*VariableStatement*/
    ,VariableDeclarationList_codegen        /*VariableDeclarationList*/
    ,VariableDeclaration_codegen            /*VariableDeclaration*/
    ,EmptyStatement_codegen                 /*EmptyStatement*/
    ,ExpressionStatement_codegen            /*ExpressionStatement*/
    ,IfStatement_codegen                    /*IfStatement*/
    ,IterationStatement_dowhile_codegen     /*IterationStatement_dowhile*/
    ,IterationStatement_while_codegen       /*IterationStatement_while*/
    ,IterationStatement_for_codegen         /*IterationStatement_for*/
    ,IterationStatement_forvar_codegen      /*IterationStatement_forvar*/
    ,IterationStatement_forin_codegen       /*IterationStatement_forin*/
    ,IterationStatement_forvarin_codegen    /*IterationStatement_forvarin*/
    ,ContinueStatement_codegen              /*ContinueStatement*/
    ,BreakStatement_codegen                 /*BreakStatement*/
    ,ReturnStatement_codegen                /*ReturnStatement*/
    ,ReturnStatement_undef_codegen          /*ReturnStatement_undef*/
    ,WithStatement_codegen                  /*WithStatement*/
    ,SwitchStatement_codegen                /*SwitchStatement*/
    ,LabelledStatement_codegen              /*LabelledStatement*/
    ,ThrowStatement_codegen                 /*ThrowStatement*/
    ,0                                      /*TryStatement*/
    ,TryStatement_catch_codegen             /*TryStatement_catch*/
    ,TryStatement_finally_codegen           /*TryStatement_finally*/
    ,TryStatement_catchfinally_codegen      /*TryStatement_catchfinally*/
    ,0                                      /*Function*/
    ,0                                      /*FunctionDeclaration*/
    ,FunctionExpression_codegen             /*FunctionExpression*/
    ,FunctionBody_codegen                   /*FunctionBody*/
    ,SourceElements_codegen                 /*SourceElements*/
};
