/* (c) 2009 David Leonard. All rights reserved. */
#if HAVE_CONFIG_H
# include <config.h>
#endif

#if HAVE_STDC_HEADERS
# include <stdlib.h>
#endif

#include <see/try.h>
#include <see/value.h>

#include "dprint.h"
#include "parse_node.h"

extern enum nodeclass_enum _SEE_nodeclass_superclass[];

/*
 * Checks that the node pointer na has node class nc in its class chain.
 * Used by the CAST_NODE() macro to runtime check casts.
 */
struct node *
_SEE_cast_node(na, nc, cname, file, line)
	struct node *na;
	enum nodeclass_enum nc;
	const char *cname;
	const char *file;
	int line;
{
	if (na) {
		enum nodeclass_enum nac = na->nodeclass;
		while (nac != NODECLASS_None && nac != nc)
		    nac = _SEE_nodeclass_superclass[nac];
		if (!nac) {
		    dprintf(
                        "%s:%d: internal error: cast to %s failed [vers %s]\n",
			file, line, cname, PACKAGE_VERSION);
		    abort();
		}
	}
	return na;
}

enum nodeclass_enum _SEE_nodeclass_superclass[NODECLASS_MAX] = { 0 
    ,NODECLASS_None                         /*Unary*/
    ,NODECLASS_None                         /*Binary*/
    ,NODECLASS_None                         /*Literal*/
    ,NODECLASS_None                         /*StringLiteral*/
    ,NODECLASS_None                         /*RegularExpressionLiteral*/
    ,NODECLASS_None                         /*PrimaryExpression_this*/
    ,NODECLASS_None                         /*PrimaryExpression_ident*/
    ,NODECLASS_None                         /*ArrayLiteral*/
    ,NODECLASS_None                         /*ObjectLiteral*/
    ,NODECLASS_None                         /*Arguments*/
    ,NODECLASS_None                         /*MemberExpression_new*/
    ,NODECLASS_None                         /*MemberExpression_dot*/
    ,NODECLASS_None                         /*MemberExpression_bracket*/
    ,NODECLASS_None                         /*CallExpression*/
    ,NODECLASS_Unary                        /*PostfixExpression_inc*/
    ,NODECLASS_Unary                        /*PostfixExpression_dec*/
    ,NODECLASS_Unary                        /*UnaryExpression_delete*/
    ,NODECLASS_Unary                        /*UnaryExpression_void*/
    ,NODECLASS_Unary                        /*UnaryExpression_typeof*/
    ,NODECLASS_Unary                        /*UnaryExpression_preinc*/
    ,NODECLASS_Unary                        /*UnaryExpression_predec*/
    ,NODECLASS_Unary                        /*UnaryExpression_plus*/
    ,NODECLASS_Unary                        /*UnaryExpression_minus*/
    ,NODECLASS_Unary                        /*UnaryExpression_inv*/
    ,NODECLASS_Unary                        /*UnaryExpression_not*/
    ,NODECLASS_Binary                       /*MultiplicativeExpression_mul*/
    ,NODECLASS_Binary                       /*MultiplicativeExpression_div*/
    ,NODECLASS_Binary                       /*MultiplicativeExpression_mod*/
    ,NODECLASS_Binary                       /*AdditiveExpression_add*/
    ,NODECLASS_Binary                       /*AdditiveExpression_sub*/
    ,NODECLASS_Binary                       /*ShiftExpression_lshift*/
    ,NODECLASS_Binary                       /*ShiftExpression_rshift*/
    ,NODECLASS_Binary                       /*ShiftExpression_urshift*/
    ,NODECLASS_Binary                       /*RelationalExpression_lt*/
    ,NODECLASS_Binary                       /*RelationalExpression_gt*/
    ,NODECLASS_Binary                       /*RelationalExpression_le*/
    ,NODECLASS_Binary                       /*RelationalExpression_ge*/
    ,NODECLASS_Binary                       /*RelationalExpression_instanceof*/
    ,NODECLASS_Binary                       /*RelationalExpression_in*/
    ,NODECLASS_Binary                       /*EqualityExpression_eq*/
    ,NODECLASS_Binary                       /*EqualityExpression_ne*/
    ,NODECLASS_Binary                       /*EqualityExpression_seq*/
    ,NODECLASS_Binary                       /*EqualityExpression_sne*/
    ,NODECLASS_Binary                       /*BitwiseANDExpression*/
    ,NODECLASS_Binary                       /*BitwiseXORExpression*/
    ,NODECLASS_Binary                       /*BitwiseORExpression*/
    ,NODECLASS_Binary                       /*LogicalANDExpression*/
    ,NODECLASS_Binary                       /*LogicalORExpression*/
    ,NODECLASS_None                         /*ConditionalExpression*/
    ,NODECLASS_None                         /*AssignmentExpression*/
    ,NODECLASS_AssignmentExpression         /*AssignmentExpression_simple*/
    ,NODECLASS_AssignmentExpression         /*AssignmentExpression_muleq*/
    ,NODECLASS_AssignmentExpression         /*AssignmentExpression_diveq*/
    ,NODECLASS_AssignmentExpression         /*AssignmentExpression_modeq*/
    ,NODECLASS_AssignmentExpression         /*AssignmentExpression_addeq*/
    ,NODECLASS_AssignmentExpression         /*AssignmentExpression_subeq*/
    ,NODECLASS_AssignmentExpression         /*AssignmentExpression_lshifteq*/
    ,NODECLASS_AssignmentExpression         /*AssignmentExpression_rshifteq*/
    ,NODECLASS_AssignmentExpression         /*AssignmentExpression_urshifteq*/
    ,NODECLASS_AssignmentExpression         /*AssignmentExpression_andeq*/
    ,NODECLASS_AssignmentExpression         /*AssignmentExpression_xoreq*/
    ,NODECLASS_AssignmentExpression         /*AssignmentExpression_oreq*/
    ,NODECLASS_Binary                       /*Expression_comma*/
    ,NODECLASS_None                         /*Block_empty*/
    ,NODECLASS_Binary                       /*StatementList*/
    ,NODECLASS_Unary                        /*VariableStatement*/
    ,NODECLASS_Binary                       /*VariableDeclarationList*/
    ,NODECLASS_None                         /*VariableDeclaration*/
    ,NODECLASS_None                         /*EmptyStatement*/
    ,NODECLASS_Unary                        /*ExpressionStatement*/
    ,NODECLASS_None                         /*IfStatement*/
    ,NODECLASS_IterationStatement_while     /*IterationStatement_dowhile*/
    ,NODECLASS_None                         /*IterationStatement_while*/
    ,NODECLASS_None                         /*IterationStatement_for*/
    ,NODECLASS_IterationStatement_for       /*IterationStatement_forvar*/
    ,NODECLASS_IterationStatement_forin     /*IterationStatement_forin*/
    ,NODECLASS_IterationStatement_forin     /*IterationStatement_forvarin*/
    ,NODECLASS_None                         /*ContinueStatement*/
    ,NODECLASS_None                         /*BreakStatement*/
    ,NODECLASS_None                         /*ReturnStatement*/
    ,NODECLASS_None                         /*ReturnStatement_undef*/
    ,NODECLASS_Binary                       /*WithStatement*/
    ,NODECLASS_None                         /*SwitchStatement*/
    ,NODECLASS_Unary                        /*LabelledStatement*/
    ,NODECLASS_Unary                        /*ThrowStatement*/
    ,NODECLASS_None                         /*TryStatement*/
    ,NODECLASS_TryStatement                 /*TryStatement_catch*/
    ,NODECLASS_TryStatement                 /*TryStatement_finally*/
    ,NODECLASS_TryStatement                 /*TryStatement_catchfinally*/
    ,NODECLASS_None                         /*Function*/
    ,NODECLASS_Function                     /*FunctionDeclaration*/
    ,NODECLASS_Function                     /*FunctionExpression*/
    ,NODECLASS_Unary                        /*FunctionBody*/
    ,NODECLASS_None                         /*SourceElements*/
};
