/*
 * (c) 2009 David Leonard.  All rights reserved.
 */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <see/try.h>
#include <see/value.h>
#include <see/system.h>
#include <see/error.h>

#include "parse.h"
#include "parse_node.h"
#include "parse_const.h"

static int Always_isconst(struct node *na, struct SEE_interpreter *interp);
static int Arguments_isconst(struct node *na,
        struct SEE_interpreter *interp);
static int Unary_isconst(struct node *na, struct SEE_interpreter *interp);
static int Binary_isconst(struct node *na, struct SEE_interpreter *interp);
static int LogicalANDExpression_isconst(struct node *na,
        struct SEE_interpreter *interp);
static int LogicalORExpression_isconst(struct node *na,
        struct SEE_interpreter *interp);
static int ConditionalExpression_isconst(struct node *na,
        struct SEE_interpreter *interp);

/*------------------------------------------------------------
 * Constant subexpression reduction
 *
 *  A subtree is 'constant' iff it
 *	- has no side-effects; and
 *	- yields the same result independent of context
 *  It follows then that a constant subtree can be evaluated using 
 *  a NULL context. We can perform that eval, and then replace the
 *  subtree with a node that generates that expression statically.
 */

/* Always returns true to indicate this class of node is always constant. */
static int
Always_isconst(na, interp)
	struct node *na;
	struct SEE_interpreter *interp;
{
	return 1;
}

static int
Arguments_isconst(na, interp)
	struct node *na; /* (struct Arguments_node) */
	struct SEE_interpreter *interp;
{
	struct Arguments_node *n = CAST_NODE(na, Arguments);
	struct Arguments_arg *arg;

	for (arg = n->first; arg; arg = arg->next)
		if (!ISCONST(arg->expr, interp))
			return 0;
	return 1;
}

static int
Unary_isconst(na, interp)
	struct node *na; /* (struct Unary_node) */
	struct SEE_interpreter *interp;
{
	struct Unary_node *n = CAST_NODE(na, Unary);
	return ISCONST(n->a, interp);
}

static int
Binary_isconst(na, interp)
	struct node *na; /* (struct Binary_node) */
	struct SEE_interpreter *interp;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	return ISCONST(n->a, interp) && ISCONST(n->b, interp);
}

static int
LogicalANDExpression_isconst(na, interp)
	struct node *na; /* (struct Binary_node) */
	struct SEE_interpreter *interp;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	if (ISCONST(n->a, interp)) {
		struct SEE_value r1, r3;
		_SEE_const_evaluate(n->a, interp, &r1);
		SEE_ASSERT(interp, SEE_VALUE_GET_TYPE(&r1) != SEE_REFERENCE);
		SEE_ToBoolean(interp, &r1, &r3);
		return r3.u.boolean ? ISCONST(n->b, interp) : 1;
	} else
		return 0;
}

static int
LogicalORExpression_isconst(na, interp)
	struct node *na; /* (struct Binary_node) */
	struct SEE_interpreter *interp;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	if (ISCONST(n->a, interp)) {
		struct SEE_value r1, r3;
		_SEE_const_evaluate(n->a, interp, &r1);
		SEE_ASSERT(interp, SEE_VALUE_GET_TYPE(&r1) != SEE_REFERENCE);
		SEE_ToBoolean(interp, &r1, &r3);
		return r3.u.boolean ? 1: ISCONST(n->b, interp);
	} else
		return 0;
}

/* 11.12 */
static int
ConditionalExpression_isconst(na, interp)
	struct node *na; /* (struct ConditionalExpression_node) */
	struct SEE_interpreter *interp;
{
	struct ConditionalExpression_node *n = 
		CAST_NODE(na, ConditionalExpression);
	if (ISCONST(n->a, interp)) {
		struct SEE_value r1, r3;
		_SEE_const_evaluate(n->a, interp, &r1);
		SEE_ASSERT(interp, SEE_VALUE_GET_TYPE(&r1) != SEE_REFERENCE);
		SEE_ToBoolean(interp, &r1, &r3);
		return r3.u.boolean 
		    ? ISCONST(n->b, interp) 
		    : ISCONST(n->c, interp);
	} else
		return 0;
}

/*
 * isconst functions return true if the expression node will always evaluate
 * to the same value; that is, it is a constant expression
 */
static int (*_SEE_nodeclass_isconst[NODECLASS_MAX])(struct node *, 
        struct SEE_interpreter *) = { 0
    ,Unary_isconst                          /*Unary*/
    ,Binary_isconst                         /*Binary*/
    ,Always_isconst                         /*Literal*/
    ,Always_isconst                         /*StringLiteral*/
    ,0                                      /*RegularExpressionLiteral*/
    ,0                                      /*PrimaryExpression_this*/
    ,0                                      /*PrimaryExpression_ident*/
    ,0                                      /*ArrayLiteral*/
    ,0                                      /*ObjectLiteral*/
    ,Arguments_isconst                      /*Arguments*/
    ,0                                      /*MemberExpression_new*/
    ,0                                      /*MemberExpression_dot*/
    ,0                                      /*MemberExpression_bracket*/
    ,0                                      /*CallExpression*/
    ,0                                      /*PostfixExpression_inc*/
    ,0                                      /*PostfixExpression_dec*/
    ,Unary_isconst                          /*UnaryExpression_delete*/
    ,Unary_isconst                          /*UnaryExpression_void*/
    ,Unary_isconst                          /*UnaryExpression_typeof*/
    ,0                                      /*UnaryExpression_preinc*/
    ,0                                      /*UnaryExpression_predec*/
    ,Unary_isconst                          /*UnaryExpression_plus*/
    ,Unary_isconst                          /*UnaryExpression_minus*/
    ,Unary_isconst                          /*UnaryExpression_inv*/
    ,Unary_isconst                          /*UnaryExpression_not*/
    ,Binary_isconst                         /*MultiplicativeExpression_mul*/
    ,Binary_isconst                         /*MultiplicativeExpression_div*/
    ,Binary_isconst                         /*MultiplicativeExpression_mod*/
    ,Binary_isconst                         /*AdditiveExpression_add*/
    ,Binary_isconst                         /*AdditiveExpression_sub*/
    ,Binary_isconst                         /*ShiftExpression_lshift*/
    ,Binary_isconst                         /*ShiftExpression_rshift*/
    ,Binary_isconst                         /*ShiftExpression_urshift*/
    ,Binary_isconst                         /*RelationalExpression_lt*/
    ,Binary_isconst                         /*RelationalExpression_gt*/
    ,Binary_isconst                         /*RelationalExpression_le*/
    ,Binary_isconst                         /*RelationalExpression_ge*/
    ,Binary_isconst                         /*RelationalExpression_instanceof*/
    ,Binary_isconst                         /*RelationalExpression_in*/
    ,Binary_isconst                         /*EqualityExpression_eq*/
    ,Binary_isconst                         /*EqualityExpression_ne*/
    ,Binary_isconst                         /*EqualityExpression_seq*/
    ,Binary_isconst                         /*EqualityExpression_sne*/
    ,Binary_isconst                         /*BitwiseANDExpression*/
    ,Binary_isconst                         /*BitwiseXORExpression*/
    ,Binary_isconst                         /*BitwiseORExpression*/
    ,LogicalANDExpression_isconst           /*LogicalANDExpression*/
    ,LogicalORExpression_isconst            /*LogicalORExpression*/
    ,ConditionalExpression_isconst          /*ConditionalExpression*/
    ,0                                      /*AssignmentExpression*/
    ,0                                      /*AssignmentExpression_simple*/
    ,0                                      /*AssignmentExpression_muleq*/
    ,0                                      /*AssignmentExpression_diveq*/
    ,0                                      /*AssignmentExpression_modeq*/
    ,0                                      /*AssignmentExpression_addeq*/
    ,0                                      /*AssignmentExpression_subeq*/
    ,0                                      /*AssignmentExpression_lshifteq*/
    ,0                                      /*AssignmentExpression_rshifteq*/
    ,0                                      /*AssignmentExpression_urshifteq*/
    ,0                                      /*AssignmentExpression_andeq*/
    ,0                                      /*AssignmentExpression_xoreq*/
    ,0                                      /*AssignmentExpression_oreq*/
    ,Binary_isconst                         /*Expression_comma*/
    ,0                                      /*Block_empty*/
    ,0                                      /*StatementList*/
    ,0                                      /*VariableStatement*/
    ,0                                      /*VariableDeclarationList*/
    ,0                                      /*VariableDeclaration*/
    ,0                                      /*EmptyStatement*/
    ,0                                      /*ExpressionStatement*/
    ,0                                      /*IfStatement*/
    ,0                                      /*IterationStatement_dowhile*/
    ,0                                      /*IterationStatement_while*/
    ,0                                      /*IterationStatement_for*/
    ,0                                      /*IterationStatement_forvar*/
    ,0                                      /*IterationStatement_forin*/
    ,0                                      /*IterationStatement_forvarin*/
    ,0                                      /*ContinueStatement*/
    ,0                                      /*BreakStatement*/
    ,0                                      /*ReturnStatement*/
    ,0                                      /*ReturnStatement_undef*/
    ,0                                      /*WithStatement*/
    ,0                                      /*SwitchStatement*/
    ,0                                      /*LabelledStatement*/
    ,0                                      /*ThrowStatement*/
    ,0                                      /*TryStatement*/
    ,0                                      /*TryStatement_catch*/
    ,0                                      /*TryStatement_finally*/
    ,0                                      /*TryStatement_catchfinally*/
    ,0                                      /*Function*/
    ,0                                      /*FunctionDeclaration*/
    ,0                                      /*FunctionExpression*/
    ,0                                      /*FunctionBody*/
    ,0                                      /*SourceElements*/
};

#define ISCONSTFN(n)    _SEE_nodeclass_isconst[(n)->nodeclass]

/* Updates a node's ISCONST_VALID and ISCONST flags. Returns true if ISCONST */
int 
_SEE_isconst(struct node *n, struct SEE_interpreter *interp) {
    int result;

    if (ISCONSTFN(n) && (*ISCONSTFN(n))(n, interp))
        result = NODE_FLAG_ISCONST;
    else
        result = 0;
    n->flags = (n->flags & ~(NODE_FLAG_ISCONST_VALID|NODE_FLAG_ISCONST)) |
        result | NODE_FLAG_ISCONST_VALID;
    return result;
}

