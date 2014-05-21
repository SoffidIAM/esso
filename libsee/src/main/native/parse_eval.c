/* (c) 2009 David Leonard. All rights reserved. */
#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <see/interpreter.h>
#include <see/try.h>
#include <see/mem.h>
#include <see/value.h>
#include <see/context.h>
#include <see/error.h>
#include <see/string.h>
#include <see/system.h>
#include <see/eval.h>
#include <see/intern.h>
#include <see/object.h>
#include <see/input.h>

#include "stringdefs.h"
#include "parse_node.h"
#include "parse_eval.h"
#include "dprint.h"
#include "function.h"
#include "enumerate.h"
#include "scope.h"
#include "nmath.h"
#include "compare.h"

/*
#include <see/cfunction.h>
#include "dtoa.h"
*/

#ifndef NDEBUG
extern int SEE_eval_debug;
#endif

/* Traces a statement-level event, or an eval() */
#define TRACE(loc, ctxt, event)                         \
    do {                                                \
        if (ctxt) {                                     \
            if (SEE_system.periodic)                    \
                (*SEE_system.periodic)((ctxt)->interpreter); \
            (ctxt)->interpreter->try_location = loc;    \
            trace_event(ctxt, event);                   \
        }                                               \
    } while (0)

/* Debugging to trace entering/leaving in EVAL() */
#ifndef NDEBUG
# if !HAVE___FUNCTION__
   /* Some trickery to stringize the __LINE__ macro */
#  define X_STR2(s) #s
#  define X_STR(s) X_STR2(s)
#  define __FUNCTION__   __FILE__ ":" X_STR(__LINE__)
# endif
# define EVAL_DEBUG_ENTER(node)                         \
        if (SEE_eval_debug)                             \
            dprintf("eval: %s enter %p\n",              \
                __FUNCTION__, node);
# define EVAL_DEBUG_LEAVE(node, ctxt, res)              \
        if (SEE_eval_debug && (ctxt)) {                 \
            dprintf("eval: %s leave %p -> %p = ",       \
                __FUNCTION__, node, (void *)(res));     \
            dprintv((ctxt)->interpreter, res);          \
            dprintf("\n");                              \
        }
#else /* NDEBUG */
# define EVAL_DEBUG_ENTER(node)
# define EVAL_DEBUG_LEAVE(node, ctxt, res)
#endif /* NDEBUG */

/*
 * Evaluator macro
 */
#define EVALFN(node) _SEE_nodeclass_eval[(node)->nodeclass]
# define EVAL(node, ctxt, res)                          \
    do {                                                \
        struct SEE_throw_location * _loc_save = NULL;   \
        EVAL_DEBUG_ENTER(node)                          \
        if (ctxt) {                                     \
          _loc_save = (ctxt)->interpreter->try_location;\
          (ctxt)->interpreter->try_location =           \
                &(node)->location;                      \
        }                                               \
        (*EVALFN(node))(node, ctxt, res);               \
        EVAL_DEBUG_LEAVE(node, ctxt, res)               \
    } while (0)
  /*
   * Note: there is no need to restore the _loc_save in
   * a try-finally block
   */

/*
 * There are only TWO fprocs used in ECMAScript node classes, so
 * don't waste space with a table. An if will do.
 */
#define FPROC(node, ctxt)                                       \
    do {                                                        \
        if ((node)->nodeclass == NODECLASS_FunctionDeclaration) \
            FunctionDeclaration_fproc(node, ctxt);              \
        else if ((node)->nodeclass == NODECLASS_SourceElements) \
            SourceElements_fproc(node, ctxt);                   \
    } while (0)


extern void (*_SEE_nodeclass_eval[])(struct node *, 
        struct SEE_context *, struct SEE_value *);

static void trace_event(struct SEE_context *ctxt, enum SEE_trace_event);
static struct SEE_traceback *traceback_enter(struct SEE_interpreter *interp, 
        struct SEE_object *callee, struct SEE_throw_location *loc, 
        int call_type);
static void traceback_leave(struct SEE_interpreter *interp, 
        struct SEE_traceback *old_tb);

static void GetValue(struct SEE_context *context, struct SEE_value *v, 
        struct SEE_value *res);
static void PutValue(struct SEE_context *context, struct SEE_value *v, 
        struct SEE_value *w);
static void Literal_eval(struct node *na, struct SEE_context *context, 
        struct SEE_value *res);
static void RegularExpressionLiteral_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void StringLiteral_eval(struct node *na, struct SEE_context *context, 
        struct SEE_value *res);
static void PrimaryExpression_this_eval(struct node *n, 
        struct SEE_context *context, struct SEE_value *res);
static void PrimaryExpression_ident_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void ArrayLiteral_eval(struct node *na, struct SEE_context *context, 
        struct SEE_value *res);
static void ObjectLiteral_eval(struct node *na, struct SEE_context *context, 
        struct SEE_value *res);
static void Arguments_eval(struct node *na, struct SEE_context *context, 
        struct SEE_value *res);
static void MemberExpression_new_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void MemberExpression_dot_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void MemberExpression_bracket_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void CallExpression_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void PostfixExpression_inc_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void PostfixExpression_dec_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void UnaryExpression_delete_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void UnaryExpression_void_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void UnaryExpression_typeof_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void UnaryExpression_preinc_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void UnaryExpression_predec_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void UnaryExpression_plus_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void UnaryExpression_minus_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void UnaryExpression_inv_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void UnaryExpression_not_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void MultiplicativeExpression_mul_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void MultiplicativeExpression_div_common(struct SEE_value *r2, 
        struct SEE_value *r4, struct SEE_context *context, 
	struct SEE_value *res);
static void MultiplicativeExpression_div_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void MultiplicativeExpression_mod_common(struct SEE_value *r2, 
        struct SEE_value *r4, struct SEE_context *context, 
	struct SEE_value *res);
static void MultiplicativeExpression_mul_common(struct SEE_value *r2, 
        struct SEE_value *r4, struct SEE_context *context, 
	struct SEE_value *res);
static void MultiplicativeExpression_mod_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void AdditiveExpression_add_common(struct SEE_value *r2, 
        struct SEE_value *r4, struct SEE_context *context,
	struct SEE_value *res);
static void AdditiveExpression_add_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void AdditiveExpression_sub_common(struct SEE_value *r2, 
        struct SEE_value *r4, struct SEE_context *context,
	struct SEE_value *res);
static void AdditiveExpression_sub_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void ShiftExpression_lshift_common(struct SEE_value *r2, 
        struct node *bn, struct SEE_context *context, struct SEE_value *res);
static void ShiftExpression_lshift_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void ShiftExpression_rshift_common(struct SEE_value *r2, 
        struct SEE_value *r4, struct SEE_context *context, 
	struct SEE_value *res);
static void ShiftExpression_rshift_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void ShiftExpression_urshift_common(struct SEE_value *r2, 
        struct SEE_value *r4, struct SEE_context *context,
	struct SEE_value *res);
static void ShiftExpression_urshift_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void RelationalExpression_lt_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void RelationalExpression_gt_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void RelationalExpression_le_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void RelationalExpression_ge_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void RelationalExpression_instanceof_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void RelationalExpression_in_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void EqualityExpression_eq_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void EqualityExpression_ne_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void EqualityExpression_seq_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void EqualityExpression_sne_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void BitwiseANDExpression_common(struct SEE_value *r2, 
        struct SEE_value *r4, struct SEE_context *context, 
	struct SEE_value *res);
static void BitwiseANDExpression_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void BitwiseXORExpression_common(struct SEE_value *r2, 
        struct SEE_value *r4, struct SEE_context *context, 
	struct SEE_value *res);
static void BitwiseXORExpression_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void BitwiseORExpression_common(struct SEE_value *r2, 
        struct SEE_value *r4, struct SEE_context *context,
	struct SEE_value *res);
static void BitwiseORExpression_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void LogicalANDExpression_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void LogicalORExpression_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void ConditionalExpression_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void AssignmentExpression_simple_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void AssignmentExpression_muleq_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void AssignmentExpression_diveq_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void AssignmentExpression_modeq_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void AssignmentExpression_addeq_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void AssignmentExpression_subeq_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void AssignmentExpression_lshifteq_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void AssignmentExpression_rshifteq_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void AssignmentExpression_urshifteq_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void AssignmentExpression_andeq_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void AssignmentExpression_xoreq_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void AssignmentExpression_oreq_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void Expression_comma_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void Block_empty_eval(struct node *n, struct SEE_context *context, 
        struct SEE_value *res);
static void StatementList_eval(struct node *na, struct SEE_context *context, 
        struct SEE_value *res);
static void VariableStatement_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void VariableDeclarationList_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void VariableDeclaration_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void EmptyStatement_eval(struct node *n, struct SEE_context *context, 
        struct SEE_value *res);
static void ExpressionStatement_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void IfStatement_eval(struct node *na, struct SEE_context *context, 
        struct SEE_value *res);
static void IterationStatement_dowhile_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void IterationStatement_while_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void IterationStatement_for_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void IterationStatement_forvar_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void IterationStatement_forin_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void IterationStatement_forvarin_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void ContinueStatement_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void BreakStatement_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void ReturnStatement_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void ReturnStatement_undef_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void WithStatement_eval(struct node *na, struct SEE_context *context, 
        struct SEE_value *res);
static void SwitchStatement_caseblock(struct SwitchStatement_node *n, 
        struct SEE_context *context, struct SEE_value *input, 
        struct SEE_value *res);
static void SwitchStatement_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void LabelledStatement_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void ThrowStatement_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static int TryStatement_catch(struct TryStatement_node *n, 
        struct SEE_context *context, struct SEE_value *C, 
        struct SEE_value *res, SEE_try_context_t *ctxt);
static void TryStatement_catch_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void TryStatement_finally_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void TryStatement_catchfinally_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void FunctionExpression_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void FunctionBody_eval(struct node *na, struct SEE_context *context, 
        struct SEE_value *res);
static void SourceElements_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void FunctionDeclaration_fproc(struct node *na, 
        struct SEE_context *context);
static void SourceElements_fproc(struct node *na, 
        struct SEE_context *context);
static void CallExpression_eval_common(struct SEE_context *, 
	struct SEE_throw_location *, struct SEE_value *, int, 
	struct SEE_value **, struct SEE_value *);
static void UnaryExpression_delete_eval_common(struct SEE_context *,
	struct SEE_value *, struct SEE_value *);
static void UnaryExpression_typeof_eval_common(struct SEE_context *,
	struct SEE_value *, struct SEE_value *);
static void UnaryExpression_inv_eval_common(struct SEE_context *,
	struct SEE_value *, struct SEE_value *);

/*
 * Generates a trace event, giving the host application an opportunity to
 * step or trace execution.
 */
static void
trace_event(ctxt, event)
	struct SEE_context *ctxt;
	enum SEE_trace_event event;
{
	if (ctxt->interpreter->trace)
	    (*ctxt->interpreter->trace)(ctxt->interpreter,
		ctxt->interpreter->try_location, ctxt, event);
}

/*
 * Pushes a new call context entry onto the traceback stack.
 * Returns the old traceback stack.
 */
static struct SEE_traceback *
traceback_enter(interp, callee, loc, call_type)
	struct SEE_interpreter *interp;
	struct SEE_object *callee;
	struct SEE_throw_location *loc;
	int call_type;
{
	struct SEE_traceback *old_tb, *tb;

	old_tb = interp->traceback;

	tb = SEE_NEW(interp, struct SEE_traceback);
	tb->call_location = loc;
	tb->callee = callee;
	tb->call_type = call_type;
	tb->prev = old_tb;
	interp->traceback = tb;

	return old_tb;
}

/*
 * Restores the traceback list before a call context was entered.
 */
static void
traceback_leave(interp, old_tb)
	struct SEE_interpreter *interp;
	struct SEE_traceback *old_tb;
{
	interp->traceback = old_tb;
}
/*------------------------------------------------------------
 * GetValue/SetValue
 */

/* 8.7.1 */
static void
GetValue(context, v, res)
	struct SEE_context *context;
	struct SEE_value *v;
	struct SEE_value *res;
{
	struct SEE_interpreter *interp = context->interpreter;

	if (SEE_VALUE_GET_TYPE(v) != SEE_REFERENCE) {
		if (v != res)
			SEE_VALUE_COPY(res, v);
		return;
	}
	if (v->u.reference.base == NULL)
		SEE_error_throw_string(interp, interp->ReferenceError, 
		    v->u.reference.property);
	else
		SEE_OBJECT_GET(interp, v->u.reference.base, 
		    v->u.reference.property, res);
}

/* 8.7.2 */
static void
PutValue(context, v, w)
	struct SEE_context *context;
	struct SEE_value *v;
	struct SEE_value *w;
{
	struct SEE_interpreter *interp = context->interpreter;
	struct SEE_object *target;

	if (SEE_VALUE_GET_TYPE(v) != SEE_REFERENCE)
		SEE_error_throw_string(interp, interp->ReferenceError,
		    STR(bad_lvalue));
	target = v->u.reference.base;
	if (target == NULL)
		target = interp->Global;
	SEE_OBJECT_PUT(interp, target, v->u.reference.property, w, 0);
}

/* 7.8 */
static void
Literal_eval(na, context, res)
	struct node *na; /* (struct Literal_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Literal_node *n = CAST_NODE(na, Literal);
	SEE_VALUE_COPY(res, &n->value);
}

/* 7.8.4 */
static void
StringLiteral_eval(na, context, res)
	struct node *na; /* (struct StringLiteral_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct StringLiteral_node *n = CAST_NODE(na, StringLiteral);
	SEE_SET_STRING(res, n->string);
}

/* 7.8.5 */
static void
RegularExpressionLiteral_eval(na, context, res)
	struct node *na; /* (struct RegularExpressionLiteral_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct RegularExpressionLiteral_node *n = 
		CAST_NODE(na, RegularExpressionLiteral);
	struct SEE_interpreter *interp = context->interpreter;
	struct SEE_traceback *tb;

        tb = traceback_enter(interp, interp->RegExp, &n->node.location,
		SEE_CALLTYPE_CONSTRUCT);
	TRACE(&na->location, context, SEE_TRACE_CALL);
	SEE_OBJECT_CONSTRUCT(interp, interp->RegExp, NULL, 
		2, n->argv, res);
	TRACE(&na->location, context, SEE_TRACE_RETURN);
        traceback_leave(interp, tb);
}

/* 11.1.1 */
static void
PrimaryExpression_this_eval(n, context, res)
	struct node *n;
	struct SEE_context *context;
	struct SEE_value *res;
{
	SEE_ASSERT(context->interpreter, context->thisobj != NULL);
	SEE_SET_OBJECT(res, context->thisobj);
}

/* 11.1.2 */
static void
PrimaryExpression_ident_eval(na, context, res)
	struct node *na; /* (struct PrimaryExpression_ident_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct PrimaryExpression_ident_node *n = 
		CAST_NODE(na, PrimaryExpression_ident);
	SEE_scope_lookup(context->interpreter, context->scope, n->string, res);
}

/* 11.1.4 */
static void
ArrayLiteral_eval(na, context, res)
	struct node *na; /* (struct ArrayLiteral_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct ArrayLiteral_node *n = CAST_NODE(na, ArrayLiteral);
	struct ArrayLiteral_element *element;
	struct SEE_value expv, elv;
	struct SEE_string *ind;
	struct SEE_interpreter *interp = context->interpreter;
	struct SEE_traceback *tb;

	ind = SEE_string_new(interp, 16);

        tb = traceback_enter(interp, interp->Array, &n->node.location,
		SEE_CALLTYPE_CONSTRUCT);
	TRACE(&na->location, context, SEE_TRACE_CALL);
	SEE_OBJECT_CONSTRUCT(interp, interp->Array, NULL, 
		0, NULL, res);
	TRACE(&na->location, context, SEE_TRACE_RETURN);
        traceback_leave(interp, tb);

	for (element = n->first; element; element = element->next) {
		EVAL(element->expr, context, &expv);
		GetValue(context, &expv, &elv);
		ind->length = 0;
		SEE_string_append_int(ind, element->index);
		SEE_OBJECT_PUT(interp, res->u.object, 
		    SEE_intern(interp, ind), &elv, 0);
	}
	SEE_SET_NUMBER(&elv, n->length);
	SEE_OBJECT_PUT(interp, res->u.object, STR(length), &elv, 0);
}

/* 11.1.5 */
static void
ObjectLiteral_eval(na, context, res)
	struct node *na; /* (struct ObjectLiteral_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct ObjectLiteral_node *n = CAST_NODE(na, ObjectLiteral);
	struct SEE_value valuev, v;
	struct SEE_object *o;
	struct ObjectLiteral_pair *pair;
	struct SEE_interpreter *interp = context->interpreter;

	o = SEE_Object_new(interp);
	for (pair = n->first; pair; pair = pair->next) {
		EVAL(pair->value, context, &valuev);
		GetValue(context, &valuev, &v);
		SEE_OBJECT_PUT(interp, o, pair->name, &v, 0);
	}
	SEE_SET_OBJECT(res, o);
}

/* 11.2.4 */
static void
Arguments_eval(na, context, res)
	struct node *na; /* (struct Arguments_node) */
	struct SEE_context *context;
	struct SEE_value *res;		/* Assumed pointer to array */
{
	struct Arguments_node *n = CAST_NODE(na, Arguments);
	struct Arguments_arg *arg;
	struct SEE_value v;

	for (arg = n->first; arg; arg = arg->next) {
		EVAL(arg->expr, context, &v);
		GetValue(context, &v, res);
		res++;
	}
}

/* 11.2.2 */
static void
MemberExpression_new_eval(na, context, res)
	struct node *na; /* (struct MemberExpression_new_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct MemberExpression_new_node *n = 
		CAST_NODE(na, MemberExpression_new);
	struct SEE_value r1, r2, *args, **argv;
	struct SEE_interpreter *interp = context->interpreter;
	int argc, i;
	struct SEE_traceback *tb;

	EVAL(n->mexp, context, &r1);
	GetValue(context, &r1, &r2);
	if (n->args) {
		argc = n->args->argc;
		args = SEE_ALLOCA(interp, struct SEE_value, argc);
		argv = SEE_ALLOCA(interp, struct SEE_value *, argc);
		Arguments_eval((struct node *)n->args, context, args);
		for (i = 0; i < argc; i++)
			argv[i] = &args[i];
	} else {
		argc = 0;
		argv = NULL;
	}
	if (SEE_VALUE_GET_TYPE(&r2) != SEE_OBJECT)
		SEE_error_throw_string(interp, interp->TypeError,
			STR(new_not_an_object));
	if (!SEE_OBJECT_HAS_CONSTRUCT(r2.u.object))
		SEE_error_throw_string(interp, interp->TypeError,
			STR(not_a_constructor));
        tb = traceback_enter(interp, r2.u.object, &n->node.location,
		SEE_CALLTYPE_CONSTRUCT);
	TRACE(&na->location, context, SEE_TRACE_CALL);
	SEE_OBJECT_CONSTRUCT(interp, r2.u.object, NULL, 
		argc, argv, res);
	TRACE(&na->location, context, SEE_TRACE_RETURN);
	traceback_leave(interp, tb);
}

/* 11.2.1 */
static void
MemberExpression_dot_eval(na, context, res)
	struct node *na; /* (struct MemberExpression_dot_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct MemberExpression_dot_node *n = 
		CAST_NODE(na, MemberExpression_dot);
	struct SEE_value r1, r2, r5;
	struct SEE_interpreter *interp = context->interpreter;

	EVAL(n->mexp, context, &r1);
	GetValue(context, &r1, &r2);
	SEE_ToObject(interp, &r2, &r5);
	_SEE_SET_REFERENCE(res, r5.u.object, n->name);
}

/* 11.2.1 */
static void
MemberExpression_bracket_eval(na, context, res)
	struct node *na; /* (struct MemberExpression_bracket_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct MemberExpression_bracket_node *n = 
		CAST_NODE(na, MemberExpression_bracket);
	struct SEE_value r1, r2, r3, r4, r5, r6;
	struct SEE_interpreter *interp = context->interpreter;

	EVAL(n->mexp, context, &r1);
	GetValue(context, &r1, &r2);
	EVAL(n->name, context, &r3);
	GetValue(context, &r3, &r4);
	SEE_ToObject(interp, &r2, &r5);
	SEE_ToString(interp, &r4, &r6);
	_SEE_SET_REFERENCE(res, r5.u.object, SEE_intern(interp, r6.u.string));
}

/* 11.2.3 */
static void
CallExpression_eval(na, context, res)
	struct node *na; /* (struct CallExpression_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct CallExpression_node *n = CAST_NODE(na, CallExpression);
	struct SEE_value r1, *args, **argv;
	int argc, i;

	EVAL(n->exp, context, &r1);
	argc = n->args->argc;
	if (argc) {
		args = SEE_ALLOCA(context->interpreter, 
			struct SEE_value, argc);
		argv = SEE_ALLOCA(context->interpreter, 
			struct SEE_value *, argc);
		Arguments_eval((struct node *)n->args, context, args);
		for (i = 0; i < argc; i++)
			argv[i] = &args[i];
	} else 
		argv = NULL;
	CallExpression_eval_common(context, &na->location, &r1, 
		argc, argv, res);
}

/* 11.2.3 */
static void
CallExpression_eval_common(context, loc, r1, argc, argv, res)
	struct SEE_context *context;
	struct SEE_throw_location *loc;
	struct SEE_value *r1;
	int argc;
	struct SEE_value **argv;
	struct SEE_value *res;
{
	struct SEE_interpreter *interp = context->interpreter;
	struct SEE_value r3;
	struct SEE_object *r6, *r7;
	struct SEE_traceback *tb;

	GetValue(context, r1, &r3);
	if (SEE_VALUE_GET_TYPE(&r3) == SEE_UNDEFINED)	/* nonstandard */
		SEE_error_throw_string(interp, interp->TypeError,
			STR(no_such_function));
	if (SEE_VALUE_GET_TYPE(&r3) != SEE_OBJECT)
		SEE_error_throw_string(interp, interp->TypeError,
			STR(not_a_function));
	if (!SEE_OBJECT_HAS_CALL(r3.u.object))
		SEE_error_throw_string(interp, interp->TypeError,
			STR(not_callable));
	if (SEE_VALUE_GET_TYPE(r1) == SEE_REFERENCE)
		r6 = r1->u.reference.base;
	else
		r6 = NULL;
	if (r6 != NULL && IS_ACTIVATION_OBJECT(r6))
		r7 = NULL;
	else
		r7 = r6;
        tb = traceback_enter(interp, r3.u.object, loc,
		SEE_CALLTYPE_CALL);
	TRACE(loc, context, SEE_TRACE_CALL);
	if (r3.u.object == interp->Global_eval) {
	    /* The special 'eval' function' */
	    _SEE_call_eval(context, r7, argc, argv, res);
	} else {
#ifndef NDEBUG
	    SEE_SET_STRING(res, STR(internal_error));
#endif
	    if (!r7)
		r7 = interp->Global;
	    SEE_OBJECT_CALL(interp, r3.u.object, r7, argc, argv, res);
	}
	TRACE(loc, context, SEE_TRACE_RETURN);
        traceback_leave(interp, tb);
}

/* 11.3.1 */
static void
PostfixExpression_inc_eval(na, context, res)
	struct node *na; /* (struct Unary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Unary_node *n = CAST_NODE(na, Unary);
	struct SEE_value r1, r2, r3;

	EVAL(n->a, context, &r1);
	GetValue(context, &r1, &r2);
	SEE_ToNumber(context->interpreter, &r2, res);
	SEE_SET_NUMBER(&r3, res->u.number + 1);
	PutValue(context, &r1, &r3);
}

/* 11.3.2 */
static void
PostfixExpression_dec_eval(na, context, res)
	struct node *na; /* (struct Unary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Unary_node *n = CAST_NODE(na, Unary);
	struct SEE_value r1, r2, r3;

	EVAL(n->a, context, &r1);
	GetValue(context, &r1, &r2);
	SEE_ToNumber(context->interpreter, &r2, res);
	SEE_SET_NUMBER(&r3, res->u.number - 1);
	PutValue(context, &r1, &r3);
}

/* 11.4.1 */
static void
UnaryExpression_delete_eval(na, context, res)
	struct node *na; /* (struct Unary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Unary_node *n = CAST_NODE(na, Unary);
	struct SEE_value r1;

	EVAL(n->a, context, &r1);
	UnaryExpression_delete_eval_common(context, &r1, res);
}

static void
UnaryExpression_delete_eval_common(context, r1, res)
	struct SEE_context *context;
	struct SEE_value *r1, *res;
{
	struct SEE_interpreter *interp = context->interpreter;

	if (SEE_VALUE_GET_TYPE(r1) != SEE_REFERENCE) {
		SEE_SET_BOOLEAN(res, 0);
		return;
	}
	/*
	 * spec bug: if the base is null, it isn't clear what is meant 
	 * to happen. We return true as if the fictitous property 
	 * owner existed.
	 */
	if (!r1->u.reference.base || 
	    SEE_OBJECT_DELETE(interp, r1->u.reference.base, 
	    		      r1->u.reference.property))
		SEE_SET_BOOLEAN(res, 1);
	else
		SEE_SET_BOOLEAN(res, 0);
}

/* 11.4.2 */
static void
UnaryExpression_void_eval(na, context, res)
	struct node *na; /* (struct Unary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Unary_node *n = CAST_NODE(na, Unary);
	struct SEE_value r1, r2;

	EVAL(n->a, context, &r1);
	GetValue(context, &r1, &r2);
	SEE_SET_UNDEFINED(res);
}

/* 11.4.3 */
static void
UnaryExpression_typeof_eval(na, context, res)
	struct node *na; /* (struct Unary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Unary_node *n = CAST_NODE(na, Unary);
	struct SEE_value r1;

	EVAL(n->a, context, &r1);
	UnaryExpression_typeof_eval_common(context, &r1, res);
}

static void
UnaryExpression_typeof_eval_common(context, r1, res)
	struct SEE_context *context;
	struct SEE_value *r1, *res;
{
	struct SEE_value r4;
	struct SEE_string *s;

	if (SEE_VALUE_GET_TYPE(r1) == SEE_REFERENCE && 
	    r1->u.reference.base == NULL) 
	{
		SEE_SET_STRING(res, STR(undefined));
		return;
	}
	GetValue(context, r1, &r4);
	switch (SEE_VALUE_GET_TYPE(&r4)) {
	case SEE_UNDEFINED:	s = STR(undefined); break;
	case SEE_NULL:		s = STR(object); break;
	case SEE_BOOLEAN:	s = STR(boolean); break;
	case SEE_NUMBER:	s = STR(number); break;
	case SEE_STRING:	s = STR(string); break;
	case SEE_OBJECT:	s = SEE_OBJECT_HAS_CALL(r4.u.object)
				  ? STR(function)
				  : STR(object);
				break;
	default:		s = STR(unknown);
	}
	SEE_SET_STRING(res, s);
}

/* 11.4.4 */
static void
UnaryExpression_preinc_eval(na, context, res)
	struct node *na; /* (struct Unary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Unary_node *n = CAST_NODE(na, Unary);
	struct SEE_value r1, r2;

	EVAL(n->a, context, &r1);
	GetValue(context, &r1, &r2);
	SEE_ToNumber(context->interpreter, &r2, res);
	res->u.number++;
	PutValue(context, &r1, res);
}

/* 11.4.5 */
static void
UnaryExpression_predec_eval(na, context, res)
	struct node *na; /* (struct Unary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Unary_node *n = CAST_NODE(na, Unary);
	struct SEE_value r1, r2;

	EVAL(n->a, context, &r1);
	GetValue(context, &r1, &r2);
	SEE_ToNumber(context->interpreter, &r2, res);
	res->u.number--;
	PutValue(context, &r1, res);
}

/* 11.4.6 */
static void
UnaryExpression_plus_eval(na, context, res)
	struct node *na; /* (struct Unary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Unary_node *n = CAST_NODE(na, Unary);
	struct SEE_value r1, r2;

	EVAL(n->a, context, &r1);
	GetValue(context, &r1, &r2);
	SEE_ToNumber(context->interpreter, &r2, res);
}

/* 11.4.7 */
static void
UnaryExpression_minus_eval(na, context, res)
	struct node *na; /* (struct Unary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Unary_node *n = CAST_NODE(na, Unary);
	struct SEE_value r1, r2;

	EVAL(n->a, context, &r1);
	GetValue(context, &r1, &r2);
	SEE_ToNumber(context->interpreter, &r2, res);
	res->u.number = -(res->u.number);
}

/* 11.4.8 */
static void
UnaryExpression_inv_eval(na, context, res)
	struct node *na; /* (struct Unary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Unary_node *n = CAST_NODE(na, Unary);
	struct SEE_value r1, r2;

	EVAL(n->a, context, &r1);
	GetValue(context, &r1, &r2);
	UnaryExpression_inv_eval_common(context, &r2, res);
}


static void
UnaryExpression_inv_eval_common(context, r2, res)
	struct SEE_context *context;
	struct SEE_value *r2, *res;
{
	SEE_int32_t r3;

	r3 = SEE_ToInt32(context->interpreter, r2);
	SEE_SET_NUMBER(res, ~r3);
}

/* 11.4.9 */
static void
UnaryExpression_not_eval(na, context, res)
	struct node *na; /* (struct Unary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Unary_node *n = CAST_NODE(na, Unary);
	struct SEE_value r1, r2, r3;

	EVAL(n->a, context, &r1);
	GetValue(context, &r1, &r2);
	SEE_ToBoolean(context->interpreter, &r2, &r3);
	SEE_SET_BOOLEAN(res, !r3.u.boolean);
}

static void
MultiplicativeExpression_mul_common(r2, r4, context, res)
	struct SEE_value *r2, *r4, *res;
	struct SEE_context *context;
{
	struct SEE_value r5, r6;

	SEE_ToNumber(context->interpreter, r2, &r5);
	SEE_ToNumber(context->interpreter, r4, &r6);
	SEE_SET_NUMBER(res, r5.u.number * r6.u.number);
}


/* 11.5.1 */
static void
MultiplicativeExpression_mul_eval(na, context, res)
	struct node *na; /* (struct Binary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	struct SEE_value r1, r2, r3, r4;

	EVAL(n->a, context, &r1);
	GetValue(context, &r1, &r2);
	EVAL(n->b, context, &r3);
	GetValue(context, &r3, &r4);
	MultiplicativeExpression_mul_common(&r2, &r4, context, res);
}

static void
MultiplicativeExpression_div_common(r2, r4, context, res)
	struct SEE_value *r2, *r4, *res;
	struct SEE_context *context;
{
	struct SEE_value r5, r6;

	SEE_ToNumber(context->interpreter, r2, &r5);
	SEE_ToNumber(context->interpreter, r4, &r6);
	SEE_SET_NUMBER(res, r5.u.number / r6.u.number);
}

/* 11.5.2 */
static void
MultiplicativeExpression_div_eval(na, context, res)
	struct node *na; /* (struct Binary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	struct SEE_value r1, r2, r3, r4;

	EVAL(n->a, context, &r1);
	GetValue(context, &r1, &r2);
	EVAL(n->b, context, &r3);
	GetValue(context, &r3, &r4);
        MultiplicativeExpression_div_common(&r2, &r4, context, res);
}

static void
MultiplicativeExpression_mod_common(r2, r4, context, res)
	struct SEE_value *r2, *r4, *res;
	struct SEE_context *context;
{
	struct SEE_value r5, r6;

	SEE_ToNumber(context->interpreter, r2, &r5);
	SEE_ToNumber(context->interpreter, r4, &r6);
	SEE_SET_NUMBER(res, NUMBER_fmod(r5.u.number, r6.u.number));
}

/* 11.5.3 */
static void
MultiplicativeExpression_mod_eval(na, context, res)
	struct node *na; /* (struct Binary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	struct SEE_value r1, r2, r3, r4;

	EVAL(n->a, context, &r1);
	GetValue(context, &r1, &r2);
	EVAL(n->b, context, &r3);
	GetValue(context, &r3, &r4);
	MultiplicativeExpression_mod_common(&r2, &r4, context, res);
}

static void
AdditiveExpression_add_common(r2, r4, context, res)
	struct SEE_value *r2, *r4, *res;
	struct SEE_context *context;
{
	struct SEE_value r5, r6,
			 r8, r9, r12, r13;
	struct SEE_string *s;

	SEE_ToPrimitive(context->interpreter, r2, NULL, &r5);
	SEE_ToPrimitive(context->interpreter, r4, NULL, &r6);
	if (!(SEE_VALUE_GET_TYPE(&r5) == SEE_STRING || 
	      SEE_VALUE_GET_TYPE(&r6) == SEE_STRING)) 
	{
		SEE_ToNumber(context->interpreter, &r5, &r8);
		SEE_ToNumber(context->interpreter, &r6, &r9);
		SEE_SET_NUMBER(res, r8.u.number + r9.u.number);
	} else {
		SEE_ToString(context->interpreter, &r5, &r12);
		SEE_ToString(context->interpreter, &r6, &r13);
		s = SEE_string_concat(context->interpreter, 
			r12.u.string, r13.u.string);
		SEE_SET_STRING(res, s);
	}
}

/* 11.6.1 */
static void
AdditiveExpression_add_eval(na, context, res)
	struct node *na; /* (struct Binary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	struct SEE_value r1, r2, r3, r4;

	EVAL(n->a, context, &r1);
	GetValue(context, &r1, &r2);
	EVAL(n->b, context, &r3);
	GetValue(context, &r3, &r4);
	AdditiveExpression_add_common(&r2, &r4, context, res);
}

static void
AdditiveExpression_sub_common(r2, r4, context, res)
	struct SEE_value *r2, *r4, *res;
	struct SEE_context *context;
{
	struct SEE_value r5, r6;

	SEE_ToNumber(context->interpreter, r2, &r5);
	SEE_ToNumber(context->interpreter, r4, &r6);
	SEE_SET_NUMBER(res, r5.u.number - r6.u.number);
}

/* 11.6.2 */
static void
AdditiveExpression_sub_eval(na, context, res)
	struct node *na; /* (struct Binary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	struct SEE_value r1, r2, r3, r4;

	EVAL(n->a, context, &r1);
	GetValue(context, &r1, &r2);
	EVAL(n->b, context, &r3);
	GetValue(context, &r3, &r4);
	AdditiveExpression_sub_common(&r2, &r4, context, res);
}

static void
ShiftExpression_lshift_common(r2, bn, context, res)
	struct SEE_value *r2, *res;
	struct node *bn;
	struct SEE_context *context;
{
	struct SEE_value r3, r4;
	SEE_int32_t r5;
	SEE_uint32_t r6;

	EVAL(bn, context, &r3);
	GetValue(context, &r3, &r4);
	r5 = SEE_ToInt32(context->interpreter, r2);
	r6 = SEE_ToUint32(context->interpreter, &r4);
	SEE_SET_NUMBER(res, r5 << (r6 & 0x1f));
}

/* 11.7.1 */
static void
ShiftExpression_lshift_eval(na, context, res)
	struct node *na; /* (struct Binary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	struct SEE_value r1, r2;

	EVAL(n->a, context, &r1);
	GetValue(context, &r1, &r2);
	ShiftExpression_lshift_common(&r2, n->b, context, res);
}

static void
ShiftExpression_rshift_common(r2, r4, context, res)
	struct SEE_value *r2, *r4, *res;
	struct SEE_context *context;
{
	SEE_int32_t r5;
	SEE_uint32_t r6;

	r5 = SEE_ToInt32(context->interpreter, r2);
	r6 = SEE_ToUint32(context->interpreter, r4);
	SEE_SET_NUMBER(res, r5 >> (r6 & 0x1f));
}

/* 11.7.2 */
static void
ShiftExpression_rshift_eval(na, context, res)
	struct node *na; /* (struct Binary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	struct SEE_value r1, r2, r3, r4;

	EVAL(n->a, context, &r1);
	GetValue(context, &r1, &r2);
	EVAL(n->b, context, &r3);
	GetValue(context, &r3, &r4);
	ShiftExpression_rshift_common(&r2, &r4, context, res);
}

static void
ShiftExpression_urshift_common(r2, r4, context, res)
	struct SEE_value *r2, *r4, *res;
	struct SEE_context *context;
{
	SEE_uint32_t r5, r6;

	r5 = SEE_ToUint32(context->interpreter, r2);
	r6 = SEE_ToUint32(context->interpreter, r4);
	SEE_SET_NUMBER(res, r5 >> (r6 & 0x1f));
}

/* 11.7.3 */
static void
ShiftExpression_urshift_eval(na, context, res)
	struct node *na; /* (struct Binary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	struct SEE_value r1, r2, r3, r4;

	EVAL(n->a, context, &r1);
	GetValue(context, &r1, &r2);
	EVAL(n->b, context, &r3);
	GetValue(context, &r3, &r4);
	ShiftExpression_urshift_common(&r2, &r4, context, res);
}

/* 11.8.1 */
static void
RelationalExpression_lt_eval(na, context, res)
	struct node *na; /* (struct Binary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	struct SEE_value r1, r2, r3, r4;

	EVAL(n->a, context, &r1);
	GetValue(context, &r1, &r2);
	EVAL(n->b, context, &r3);
	GetValue(context, &r3, &r4);
	_SEE_RelationalExpression_sub(context->interpreter, &r2, &r4, res);
	if (SEE_VALUE_GET_TYPE(res) == SEE_UNDEFINED)
		SEE_SET_BOOLEAN(res, 0);
}

/* 11.8.2 */
static void
RelationalExpression_gt_eval(na, context, res)
	struct node *na; /* (struct Binary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	struct SEE_value r1, r2, r3, r4;

	EVAL(n->a, context, &r1);
	GetValue(context, &r1, &r2);
	EVAL(n->b, context, &r3);
	GetValue(context, &r3, &r4);
	_SEE_RelationalExpression_sub(context->interpreter, &r4, &r2, res);
	if (SEE_VALUE_GET_TYPE(res) == SEE_UNDEFINED)
		SEE_SET_BOOLEAN(res, 0);
}

/* 11.8.3 */
static void
RelationalExpression_le_eval(na, context, res)
	struct node *na; /* (struct Binary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	struct SEE_value r1, r2, r3, r4, r5;

	EVAL(n->a, context, &r1);
	GetValue(context, &r1, &r2);
	EVAL(n->b, context, &r3);
	GetValue(context, &r3, &r4);
	_SEE_RelationalExpression_sub(context->interpreter, &r4, &r2, &r5);
	if (SEE_VALUE_GET_TYPE(&r5) == SEE_UNDEFINED)
		SEE_SET_BOOLEAN(res, 0);
	else
		SEE_SET_BOOLEAN(res, !r5.u.boolean);
}

/* 11.8.4 */
static void
RelationalExpression_ge_eval(na, context, res)
	struct node *na; /* (struct Binary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	struct SEE_value r1, r2, r3, r4, r5;

	EVAL(n->a, context, &r1);
	GetValue(context, &r1, &r2);
	EVAL(n->b, context, &r3);
	GetValue(context, &r3, &r4);
	_SEE_RelationalExpression_sub(context->interpreter, &r2, &r4, &r5);
	if (SEE_VALUE_GET_TYPE(&r5) == SEE_UNDEFINED)
		SEE_SET_BOOLEAN(res, 0);
	else
		SEE_SET_BOOLEAN(res, !r5.u.boolean);
}

/* 11.8.6 */
static void
RelationalExpression_instanceof_eval(na, context, res)
	struct node *na; /* (struct Binary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	struct SEE_interpreter *interp = context->interpreter;
	struct SEE_value r1, r2, r3, r4;
	int r7;

	EVAL(n->a, context, &r1);
	GetValue(context, &r1, &r2);
	EVAL(n->b, context, &r3);
	GetValue(context, &r3, &r4);
	if (SEE_VALUE_GET_TYPE(&r4) != SEE_OBJECT)
		SEE_error_throw_string(interp, interp->TypeError,
		    STR(instanceof_not_object));
	r7 = SEE_object_instanceof(interp, &r2, r4.u.object);
	SEE_SET_BOOLEAN(res, r7);
}

/* 11.8.7 */
static void
RelationalExpression_in_eval(na, context, res)
	struct node *na; /* (struct Binary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	struct SEE_interpreter *interp = context->interpreter;
	struct SEE_value r1, r2, r3, r4, r6;
	int r7;

	EVAL(n->a, context, &r1);
	GetValue(context, &r1, &r2);
	EVAL(n->b, context, &r3);
	GetValue(context, &r3, &r4);
	if (SEE_VALUE_GET_TYPE(&r4) != SEE_OBJECT)
		SEE_error_throw_string(interp, interp->TypeError,
		    STR(in_not_object));
	SEE_ToString(interp, &r2, &r6);
	r7 = SEE_OBJECT_HASPROPERTY(interp, r4.u.object, 
		SEE_intern(interp, r6.u.string));
	SEE_SET_BOOLEAN(res, r7);
}

/* 11.9.1 */
static void
EqualityExpression_eq_eval(na, context, res)
	struct node *na; /* (struct Binary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	struct SEE_value r1, r2, r3, r4;

	EVAL(n->a, context, &r1);
	GetValue(context, &r1, &r2);
	EVAL(n->b, context, &r3);
	GetValue(context, &r3, &r4);
	_SEE_EqualityExpression_eq(context->interpreter, &r4, &r2, res);
}

/* 11.9.2 */
static void
EqualityExpression_ne_eval(na, context, res)
	struct node *na; /* (struct Binary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	struct SEE_value r1, r2, r3, r4, t;

	EVAL(n->a, context, &r1);
	GetValue(context, &r1, &r2);
	EVAL(n->b, context, &r3);
	GetValue(context, &r3, &r4);
	_SEE_EqualityExpression_eq(context->interpreter, &r4, &r2, &t);
	SEE_SET_BOOLEAN(res, !t.u.boolean);
}

/* 11.9.4 */
static void
EqualityExpression_seq_eval(na, context, res)
	struct node *na; /* (struct Binary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	struct SEE_value r1, r2, r3, r4;

	EVAL(n->a, context, &r1);
	GetValue(context, &r1, &r2);
	EVAL(n->b, context, &r3);
	GetValue(context, &r3, &r4);
	_SEE_EqualityExpression_seq(context->interpreter, &r4, &r2, res);
}

/* 11.9.5 */
static void
EqualityExpression_sne_eval(na, context, res)
	struct node *na; /* (struct Binary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	struct SEE_value r1, r2, r3, r4, r5;

	EVAL(n->a, context, &r1);
	GetValue(context, &r1, &r2);
	EVAL(n->b, context, &r3);
	GetValue(context, &r3, &r4);
	_SEE_EqualityExpression_seq(context->interpreter, &r4, &r2, &r5);
	SEE_SET_BOOLEAN(res, !r5.u.boolean);
}

static void
BitwiseANDExpression_common(r2, r4, context, res)
	struct SEE_value *r2, *r4, *res;
	struct SEE_context *context;
{
	SEE_int32_t r5, r6;

	r5 = SEE_ToInt32(context->interpreter, r2);
	r6 = SEE_ToInt32(context->interpreter, r4);
	SEE_SET_NUMBER(res, r5 & r6);
}


/* 11.10 */
static void
BitwiseANDExpression_eval(na, context, res)
	struct node *na; /* (struct Binary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	struct SEE_value r1, r2, r3, r4;

	EVAL(n->a, context, &r1);
	GetValue(context, &r1, &r2);
	EVAL(n->b, context, &r3);
	GetValue(context, &r3, &r4);
	BitwiseANDExpression_common(&r2, &r4, context, res);
}

static void
BitwiseXORExpression_common(r2, r4, context, res)
	struct SEE_value *r2, *r4, *res;
	struct SEE_context *context;
{
	SEE_int32_t r5, r6;

	r5 = SEE_ToInt32(context->interpreter, r2);
	r6 = SEE_ToInt32(context->interpreter, r4);
	SEE_SET_NUMBER(res, r5 ^ r6);
}

/* 11.10 */
static void
BitwiseXORExpression_eval(na, context, res)
	struct node *na; /* (struct Binary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	struct SEE_value r1, r2, r3, r4;

	EVAL(n->a, context, &r1);
	GetValue(context, &r1, &r2);
	EVAL(n->b, context, &r3);
	GetValue(context, &r3, &r4);
	BitwiseXORExpression_common(&r2, &r4, context, res);
}

static void
BitwiseORExpression_common(r2, r4, context, res)
	struct SEE_value *r2, *r4, *res;
	struct SEE_context *context;
{
	SEE_int32_t r5, r6;

	r5 = SEE_ToInt32(context->interpreter, r2);
	r6 = SEE_ToInt32(context->interpreter, r4);
	SEE_SET_NUMBER(res, r5 | r6);
}

/* 11.10 */
static void
BitwiseORExpression_eval(na, context, res)
	struct node *na; /* (struct Binary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	struct SEE_value r1, r2, r3, r4;

	EVAL(n->a, context, &r1);
	GetValue(context, &r1, &r2);
	EVAL(n->b, context, &r3);
	GetValue(context, &r3, &r4);
	BitwiseORExpression_common(&r2, &r4, context, res);
}

/* 11.11 */
static void
LogicalANDExpression_eval(na, context, res)
	struct node *na; /* (struct Binary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	struct SEE_value r1, r3, r5;

	EVAL(n->a, context, &r1);
	GetValue(context, &r1, res);
	SEE_ToBoolean(context->interpreter, res, &r3);
	if (!r3.u.boolean)
		return;
	EVAL(n->b, context, &r5);
	GetValue(context, &r5, res);
}

/* 11.11 */
static void
LogicalORExpression_eval(na, context, res)
	struct node *na; /* (struct Binary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	struct SEE_value r1, r3, r5;

	EVAL(n->a, context, &r1);
	GetValue(context, &r1, res);
	SEE_ToBoolean(context->interpreter, res, &r3);
	if (r3.u.boolean)
		return;
	EVAL(n->b, context, &r5);
	GetValue(context, &r5, res);
}

/* 11.12 */
static void
ConditionalExpression_eval(na, context, res)
	struct node *na; /* (struct ConditionalExpression_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct ConditionalExpression_node *n = 
		CAST_NODE(na, ConditionalExpression);
	struct SEE_value r1, r2, r3, t;

	EVAL(n->a, context, &r1);
	GetValue(context, &r1, &r2);
	SEE_ToBoolean(context->interpreter, &r2, &r3);
	if (r3.u.boolean)
		EVAL(n->b, context, &t);
	else
		EVAL(n->c, context, &t);
	GetValue(context, &t, res);
}

/* 11.13.1 */
static void
AssignmentExpression_simple_eval(na, context, res)
	struct node *na; /* (struct AssignmentExpression_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct AssignmentExpression_node *n = 
		CAST_NODE(na, AssignmentExpression);
	struct SEE_value r1, r2;

	EVAL(n->lhs, context, &r1);
	EVAL(n->expr, context, &r2);
	GetValue(context, &r2, res);
	PutValue(context, &r1, res);
}

/* 11.13.2 *= */
static void
AssignmentExpression_muleq_eval(na, context, res)
	struct node *na; /* (struct AssignmentExpression_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct AssignmentExpression_node *n = 
		CAST_NODE(na, AssignmentExpression);
	struct SEE_value r1, r2, r3, r4;

	EVAL(n->lhs, context, &r1);
	GetValue(context, &r1, &r2);
	EVAL(n->expr, context, &r3);
	GetValue(context, &r3, &r4);
	MultiplicativeExpression_mul_common(&r2, &r4, context, res);
	PutValue(context, &r1, res);
}

/* 11.13.2 /= */
static void
AssignmentExpression_diveq_eval(na, context, res)
	struct node *na; /* (struct AssignmentExpression_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct AssignmentExpression_node *n = 
		CAST_NODE(na, AssignmentExpression);
	struct SEE_value r1, r2, r3, r4;

	EVAL(n->lhs, context, &r1);
	GetValue(context, &r1, &r2);
	EVAL(n->expr, context, &r3);
	GetValue(context, &r3, &r4);
	MultiplicativeExpression_div_common(&r2, &r4, context, res);
	PutValue(context, &r1, res);
}

/* 11.13.2 %= */
static void
AssignmentExpression_modeq_eval(na, context, res)
	struct node *na; /* (struct AssignmentExpression_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct AssignmentExpression_node *n = 
		CAST_NODE(na, AssignmentExpression);
	struct SEE_value r1, r2, r3, r4;

	EVAL(n->lhs, context, &r1);
	GetValue(context, &r1, &r2);
	EVAL(n->expr, context, &r3);
	GetValue(context, &r3, &r4);
	MultiplicativeExpression_mod_common(&r2, &r4, context, res);
	PutValue(context, &r1, res);
}

/* 11.13.2 += */
static void
AssignmentExpression_addeq_eval(na, context, res)
	struct node *na; /* (struct AssignmentExpression_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct AssignmentExpression_node *n = 
		CAST_NODE(na, AssignmentExpression);
	struct SEE_value r1, r2, r3, r4;

	EVAL(n->lhs, context, &r1);
	GetValue(context, &r1, &r2);
	EVAL(n->expr, context, &r3);
	GetValue(context, &r3, &r4);
	AdditiveExpression_add_common(&r2, &r4, context, res);
	PutValue(context, &r1, res);
}

/* 11.13.2 -= */
static void
AssignmentExpression_subeq_eval(na, context, res)
	struct node *na; /* (struct AssignmentExpression_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct AssignmentExpression_node *n = 
		CAST_NODE(na, AssignmentExpression);
	struct SEE_value r1, r2, r3, r4;

	EVAL(n->lhs, context, &r1);
	GetValue(context, &r1, &r2);
	EVAL(n->expr, context, &r3);
	GetValue(context, &r3, &r4);
	AdditiveExpression_sub_common(&r2, &r4, context, res);
	PutValue(context, &r1, res);
}

/* 11.13.2 <<= */
static void
AssignmentExpression_lshifteq_eval(na, context, res)
	struct node *na; /* (struct AssignmentExpression_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct AssignmentExpression_node *n = 
		CAST_NODE(na, AssignmentExpression);
	struct SEE_value r1, r2;

	EVAL(n->lhs, context, &r1);
	GetValue(context, &r1, &r2);
	ShiftExpression_lshift_common(&r2, n->expr, context, res);
	PutValue(context, &r1, res);
}

/* 11.13.2 >>= */
static void
AssignmentExpression_rshifteq_eval(na, context, res)
	struct node *na; /* (struct AssignmentExpression_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct AssignmentExpression_node *n = 
		CAST_NODE(na, AssignmentExpression);
	struct SEE_value r1, r2, r3, r4;

	EVAL(n->lhs, context, &r1);
	GetValue(context, &r1, &r2);
	EVAL(n->expr, context, &r3);
	GetValue(context, &r3, &r4);
	ShiftExpression_rshift_common(&r2, &r4, context, res);
	PutValue(context, &r1, res);
}

/* 11.13.2 >>>= */
static void
AssignmentExpression_urshifteq_eval(na, context, res)
	struct node *na; /* (struct AssignmentExpression_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct AssignmentExpression_node *n = 
		CAST_NODE(na, AssignmentExpression);
	struct SEE_value r1, r2, r3, r4;

	EVAL(n->lhs, context, &r1);
	GetValue(context, &r1, &r2);
	EVAL(n->expr, context, &r3);
	GetValue(context, &r3, &r4);
	ShiftExpression_urshift_common(&r2, &r4, context, res);
	PutValue(context, &r1, res);
}

/* 11.13.2 &= */
static void
AssignmentExpression_andeq_eval(na, context, res)
	struct node *na; /* (struct AssignmentExpression_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct AssignmentExpression_node *n = 
		CAST_NODE(na, AssignmentExpression);
	struct SEE_value r1, r2, r3, r4;

	EVAL(n->lhs, context, &r1);
	GetValue(context, &r1, &r2);
	EVAL(n->expr, context, &r3);
	GetValue(context, &r3, &r4);
	BitwiseANDExpression_common(&r2, &r4, context, res);
	PutValue(context, &r1, res);
}

/* 11.13.2 ^= */
static void
AssignmentExpression_xoreq_eval(na, context, res)
	struct node *na; /* (struct AssignmentExpression_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct AssignmentExpression_node *n = 
		CAST_NODE(na, AssignmentExpression);
	struct SEE_value r1, r2, r3, r4;

	EVAL(n->lhs, context, &r1);
	GetValue(context, &r1, &r2);
	EVAL(n->expr, context, &r3);
	GetValue(context, &r3, &r4);
	BitwiseXORExpression_common(&r2, &r4, context, res);
	PutValue(context, &r1, res);
}

/* 11.13.2 |= */
static void
AssignmentExpression_oreq_eval(na, context, res)
	struct node *na; /* (struct AssignmentExpression_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct AssignmentExpression_node *n = 
		CAST_NODE(na, AssignmentExpression);
	struct SEE_value r1, r2, r3, r4;

	EVAL(n->lhs, context, &r1);
	GetValue(context, &r1, &r2);
	EVAL(n->expr, context, &r3);
	GetValue(context, &r3, &r4);
	BitwiseORExpression_common(&r2, &r4, context, res);
	PutValue(context, &r1, res);
}

/* 11.14 */
static void
Expression_comma_eval(na, context, res)
	struct node *na; /* (struct Binary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	struct SEE_value r1, r2, r3;

	EVAL(n->a, context, &r1);
	GetValue(context, &r1, &r2);
	EVAL(n->b, context, &r3);
	GetValue(context, &r3, res);
}

/* 12.1 */
static void
Block_empty_eval(n, context, res)
	struct node *n;
	struct SEE_context *context;
	struct SEE_value *res;
{
	_SEE_SET_COMPLETION(res, SEE_COMPLETION_NORMAL, NULL, NO_TARGET);
}

/* 12.1 */
static void
StatementList_eval(na, context, res)
	struct node *na; /* (struct Binary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	struct SEE_value *val;

	EVAL(n->a, context, res);
	if (res->u.completion.type == SEE_COMPLETION_NORMAL) {
		val = res->u.completion.value;
		EVAL(n->b, context, res);
		if (res->u.completion.value == NULL)
			res->u.completion.value = val;
		else 
			SEE_free(context->interpreter, (void **)&val);
	}
}

/* 12.2 */
static void
VariableStatement_eval(na, context, res)
	struct node *na; /* (struct Unary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Unary_node *n = CAST_NODE(na, Unary);
	struct SEE_value v;

	TRACE(&na->location, context, SEE_TRACE_STATEMENT);
	EVAL(n->a, context, &v);
	_SEE_SET_COMPLETION(res, SEE_COMPLETION_NORMAL, NULL, NO_TARGET);
}

/* 12.2 */
static void
VariableDeclarationList_eval(na, context, res)
	struct node *na; /* (struct Binary_node) */
	struct SEE_context *context;
	struct SEE_value *res;		/* unused */
{
	struct Binary_node *n = CAST_NODE(na, Binary);

	EVAL(n->a, context, res);
	EVAL(n->b, context, res);
}

/* 12.2 */
static void
VariableDeclaration_eval(na, context, res)
	struct node *na; /* (struct VariableDeclaration_node) */
	struct SEE_context *context;
	struct SEE_value *res;		/* unused */
{
	struct VariableDeclaration_node *n = 
		CAST_NODE(na, VariableDeclaration);
	struct SEE_value r1, r2, r3;

	if (n->init) {
		SEE_scope_lookup(context->interpreter, context->scope, 
			n->var->name, &r1);
		EVAL(n->init, context, &r2);
		GetValue(context, &r2, &r3);
		PutValue(context, &r1, &r3);
	}
}

/* 12.3 */
static void
EmptyStatement_eval(na, context, res)
	struct node *na;
	struct SEE_context *context;
	struct SEE_value *res;
{
	TRACE(&na->location, context, SEE_TRACE_STATEMENT);
	_SEE_SET_COMPLETION(res, SEE_COMPLETION_NORMAL, NULL, NO_TARGET);
}

/* 12.4 */
static void
ExpressionStatement_eval(na, context, res)
	struct node *na; /* (struct Unary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Unary_node *n = CAST_NODE(na, Unary);
	struct SEE_value r1;
	struct SEE_value *v = SEE_NEW(context->interpreter, struct SEE_value);

	TRACE(&na->location, context, SEE_TRACE_STATEMENT);
	EVAL(n->a, context, &r1);
	GetValue(context, &r1, v);
	_SEE_SET_COMPLETION(res, SEE_COMPLETION_NORMAL, v, NO_TARGET);
}

/* 12.5 */
static void
IfStatement_eval(na, context, res)
	struct node *na; /* (struct IfStatement_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct IfStatement_node *n = CAST_NODE(na, IfStatement);
	struct SEE_value r1, r2, r3;

	TRACE(&na->location, context, SEE_TRACE_STATEMENT);
	EVAL(n->cond, context, &r1);
	GetValue(context, &r1, &r2);
	SEE_ToBoolean(context->interpreter, &r2, &r3);
	if (r3.u.boolean)
		EVAL(n->btrue, context, res);
	else if (n->bfalse)
		EVAL(n->bfalse, context, res);
	else
		_SEE_SET_COMPLETION(res, SEE_COMPLETION_NORMAL, NULL, NO_TARGET);
}

/* 12.6.1 */
static void
IterationStatement_dowhile_eval(na, context, res)
	struct node *na; /* (struct IterationStatement_while_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct IterationStatement_while_node *n = 
		CAST_NODE(na, IterationStatement_while);
	struct SEE_value *v, r7, r8, r9;

	v = NULL;
 step2:	EVAL(n->body, context, res);
	if (res->u.completion.value)
	    v = res->u.completion.value;
	if (res->u.completion.type == SEE_COMPLETION_CONTINUE &&
	    n->target == res->u.completion.target)
	    goto step7;
	if (res->u.completion.type == SEE_COMPLETION_BREAK &&
	    n->target == res->u.completion.target)
	    goto step11;
	if (res->u.completion.type != SEE_COMPLETION_NORMAL)
	    goto out;
 step7: TRACE(&na->location, context, SEE_TRACE_STATEMENT);
 	EVAL(n->cond, context, &r7);
	GetValue(context, &r7, &r8);
	SEE_ToBoolean(context->interpreter, &r8, &r9);
	if (r9.u.boolean)
	    goto step2;
 step11:_SEE_SET_COMPLETION(res, SEE_COMPLETION_NORMAL, v, NO_TARGET);
 out:	;
}

/* 12.6.2 */
static void
IterationStatement_while_eval(na, context, res)
	struct node *na; /* (struct IterationStatement_while_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct IterationStatement_while_node *n = 
		CAST_NODE(na, IterationStatement_while);
	struct SEE_value *v, r2, r3, r4;

	v = NULL;
 step2: TRACE(&na->location, context, SEE_TRACE_STATEMENT);
 	EVAL(n->cond, context, &r2);
	GetValue(context, &r2, &r3);
	SEE_ToBoolean(context->interpreter, &r3, &r4);
	if (!r4.u.boolean) {
	    _SEE_SET_COMPLETION(res, SEE_COMPLETION_NORMAL, v, NO_TARGET);
	    return;
	}
	EVAL(n->body, context, res);
	if (res->u.completion.value)
		v = res->u.completion.value;
	if (res->u.completion.type == SEE_COMPLETION_CONTINUE &&
	    n->target == res->u.completion.target)
		goto step2;
	if (res->u.completion.type == SEE_COMPLETION_BREAK &&
	    n->target == res->u.completion.target)
	{
	    _SEE_SET_COMPLETION(res, SEE_COMPLETION_NORMAL, v, NO_TARGET);
	    return;
	}
	if (res->u.completion.type != SEE_COMPLETION_NORMAL)
	    return;
	goto step2;
}

/* 12.6 for(;;) */
static void
IterationStatement_for_eval(na, context, res)
	struct node *na; /* (struct IterationStatement_for_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct IterationStatement_for_node *n = 
		CAST_NODE(na, IterationStatement_for);
	struct SEE_value *v, r2, r3, r6, r7, r8, r16, r17;

	if (n->init) {
	    TRACE(&n->init->location, context, SEE_TRACE_STATEMENT);
	    EVAL(n->init, context, &r2);
	    GetValue(context, &r2, &r3);		/* r3 not used */
	}
	v = NULL;
 step5:	if (n->cond) {
	    TRACE(&n->cond->location, context, SEE_TRACE_STATEMENT);
	    EVAL(n->cond, context, &r6);
	    GetValue(context, &r6, &r7);
	    SEE_ToBoolean(context->interpreter, &r7, &r8);
	    if (!r8.u.boolean) goto step19;
	} else
	    TRACE(&na->location, context, SEE_TRACE_STATEMENT);
	EVAL(n->body, context, res);
	if (res->u.completion.value)
	    v = res->u.completion.value;
	if (res->u.completion.type == SEE_COMPLETION_BREAK &&
	    n->target == res->u.completion.target)
		goto step19;
	if (res->u.completion.type == SEE_COMPLETION_CONTINUE &&
	    n->target == res->u.completion.target)
		goto step15;
	if (res->u.completion.type != SEE_COMPLETION_NORMAL)
		return;
step15: if (n->incr) {
	    TRACE(&n->incr->location, context, SEE_TRACE_STATEMENT);
	    EVAL(n->incr, context, &r16);
	    GetValue(context, &r16, &r17);	/* r17 not used */
	}
	goto step5;
step19:	_SEE_SET_COMPLETION(res, SEE_COMPLETION_NORMAL, v, NO_TARGET);
}

/* 12.6 for(var;;) */
static void
IterationStatement_forvar_eval(na, context, res)
	struct node *na; /* (struct IterationStatement_for_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct IterationStatement_for_node *n = 
		CAST_NODE(na, IterationStatement_for);
	struct SEE_value *v, r1, r4, r5, r6, r14, r15;

	TRACE(&n->init->location, context, SEE_TRACE_STATEMENT);
	EVAL(n->init, context, &r1);
	v = NULL;
 step3: if (n->cond) {
	    TRACE(&n->cond->location, context, SEE_TRACE_STATEMENT);
	    EVAL(n->cond, context, &r4);
	    GetValue(context, &r4, &r5);
	    SEE_ToBoolean(context->interpreter, &r5, &r6);
	    if (!r6.u.boolean) goto step17; /* spec bug: says step 14 */
	} else
	    TRACE(&na->location, context, SEE_TRACE_STATEMENT);
	EVAL(n->body, context, res);
	if (res->u.completion.value)
	    v = res->u.completion.value;
	if (res->u.completion.type == SEE_COMPLETION_BREAK &&
	    n->target == res->u.completion.target)
		goto step17;
	if (res->u.completion.type == SEE_COMPLETION_CONTINUE &&
	    n->target == res->u.completion.target)
		goto step13;
	if (res->u.completion.type != SEE_COMPLETION_NORMAL)
		return;
step13: if (n->incr) {
	    TRACE(&n->incr->location, context, SEE_TRACE_STATEMENT);
	    EVAL(n->incr, context, &r14);
	    GetValue(context, &r14, &r15); 		/* value not used */
	}
	goto step3;
step17:	_SEE_SET_COMPLETION(res, SEE_COMPLETION_NORMAL, v, NO_TARGET);
}

/* 12.6 for(in) */
static void
IterationStatement_forin_eval(na, context, res)
	struct node *na; /* (struct IterationStatement_forin_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct IterationStatement_forin_node *n = 
		CAST_NODE(na, IterationStatement_forin);
	struct SEE_interpreter *interp = context->interpreter;
	struct SEE_value *v, r1, r2, r3, r5, r6;
	struct SEE_string **props0, **props;

        TRACE(&na->location, context, SEE_TRACE_STATEMENT);
	EVAL(n->list, context, &r1);
	GetValue(context, &r1, &r2);
	SEE_ToObject(interp, &r2, &r3);
	v = NULL;
	for (props0 = props = SEE_enumerate(interp, r3.u.object); 
	     *props; 
	     props++)
	{
	    if (!SEE_OBJECT_HASPROPERTY(interp, r3.u.object, *props))
		    continue;	/* property was deleted! */
	    SEE_SET_STRING(&r5, *props);
	    EVAL(n->lhs, context, &r6);
	    PutValue(context, &r6, &r5);
	    EVAL(n->body, context, res);
	    if (res->u.completion.value)
		v = res->u.completion.value;
	    if (res->u.completion.type == SEE_COMPLETION_BREAK &&
		n->target == res->u.completion.target)
		    break;
	    if (res->u.completion.type == SEE_COMPLETION_CONTINUE &&
		n->target == res->u.completion.target)
		    continue;
	    if (res->u.completion.type != SEE_COMPLETION_NORMAL)
		    return;
	}
	SEE_enumerate_free(interp, props0);
	_SEE_SET_COMPLETION(res, SEE_COMPLETION_NORMAL, v, NO_TARGET);
}

/* 12.6 for(var in) */
static void
IterationStatement_forvarin_eval(na, context, res)
	struct node *na; /* (struct IterationStatement_forin_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct IterationStatement_forin_node *n = 
		CAST_NODE(na, IterationStatement_forin);
	struct SEE_interpreter *interp = context->interpreter;
	struct SEE_value *v, r2, r3, r4, r6, r7;
	struct SEE_string **props0, **props;
	struct VariableDeclaration_node *lhs 
		= CAST_NODE(n->lhs, VariableDeclaration);

	TRACE(&na->location, context, SEE_TRACE_STATEMENT);
	EVAL(n->lhs, context, NULL);
	EVAL(n->list, context, &r2);
	GetValue(context, &r2, &r3);
	SEE_ToObject(interp, &r3, &r4);
	v = NULL;
	for (props0 = props = SEE_enumerate(interp, r4.u.object);
	     *props; 
	     props++)
	{
	    if (!SEE_OBJECT_HASPROPERTY(interp, r4.u.object, *props))
		    continue;	/* property was deleted! */
	    SEE_SET_STRING(&r6, *props);
	    /* spec bug: "see 0" in step 7 */
	    SEE_scope_lookup(context->interpreter, context->scope, 
	    	lhs->var->name, &r7);
	    PutValue(context, &r7, &r6);
	    EVAL(n->body, context, res);
	    if (res->u.completion.value)
		v = res->u.completion.value;
	    if (res->u.completion.type == SEE_COMPLETION_BREAK &&
		n->target == res->u.completion.target)
		    break;
	    if (res->u.completion.type == SEE_COMPLETION_CONTINUE &&
		n->target == res->u.completion.target)
		    continue;
	    if (res->u.completion.type != SEE_COMPLETION_NORMAL)
		    return;
	}
	SEE_enumerate_free(interp, props0);
	_SEE_SET_COMPLETION(res, SEE_COMPLETION_NORMAL, v, NO_TARGET);
}

/* 12.7 */
static void
ContinueStatement_eval(na, context, res)
	struct node *na; /* (struct ContinueStatement_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct ContinueStatement_node *n = CAST_NODE(na, ContinueStatement);

	TRACE(&na->location, context, SEE_TRACE_STATEMENT);
	_SEE_SET_COMPLETION(res, SEE_COMPLETION_CONTINUE, NULL, n->target);
}

/* 12.8 */
static void
BreakStatement_eval(na, context, res)
	struct node *na; /* (struct BreakStatement_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct BreakStatement_node *n = CAST_NODE(na, BreakStatement);

	TRACE(&na->location, context, SEE_TRACE_STATEMENT);
	_SEE_SET_COMPLETION(res, SEE_COMPLETION_BREAK, NULL, n->target);
}

/* 12.9 */
static void
ReturnStatement_eval(na, context, res)
	struct node *na; /* (struct ReturnStatement_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct ReturnStatement_node *n = CAST_NODE(na, ReturnStatement);
	struct SEE_value r2, *v;

	TRACE(&na->location, context, SEE_TRACE_STATEMENT);
	EVAL(n->expr, context, &r2);
	v = SEE_NEW(context->interpreter, struct SEE_value);
	GetValue(context, &r2, v);
	_SEE_SET_COMPLETION(res, SEE_COMPLETION_RETURN, v, NO_TARGET);
}

/* 12.9 */
static void
ReturnStatement_undef_eval(na, context, res)
	struct node *na; /* (struct ReturnStatement_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	static struct SEE_value undef = { SEE_UNDEFINED };

	TRACE(&na->location, context, SEE_TRACE_STATEMENT);
	_SEE_SET_COMPLETION(res, SEE_COMPLETION_RETURN, &undef, NO_TARGET);
}

/* 12.10 */
static void
WithStatement_eval(na, context, res)
	struct node *na; /* (struct Binary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	SEE_try_context_t ctxt;
	struct SEE_value r1, r2, r3;
	struct SEE_scope *s;

	TRACE(&na->location, context, SEE_TRACE_STATEMENT);
	EVAL(n->a, context, &r1);
	GetValue(context, &r1, &r2);
	SEE_ToObject(context->interpreter, &r2, &r3);

	/* Insert r3 in front of current scope chain */
	s = SEE_NEW(context->interpreter, struct SEE_scope);
	s->obj = r3.u.object;
	s->next = context->scope;
	context->scope = s;
	SEE_TRY(context->interpreter, ctxt)
	    EVAL(n->b, context, res);
	context->scope = context->scope->next;
	SEE_DEFAULT_CATCH(context->interpreter, ctxt);
}

static void
SwitchStatement_caseblock(n, context, input, res)
	struct SwitchStatement_node *n;
	struct SEE_context *context;
	struct SEE_value *input, *res;
{
	struct case_list *c;
	struct SEE_value cc1, cc2, cc3;

	/*
	 * Note, this should be functionally equivalent
	 * to the standard. We search through the in-order
	 * case statements to find an expression that is
	 * strictly equal to 'input', and then run all
	 * the statements from there till we break or reach
	 * the end. If no expression matches, we start at the
	 * default case, if one exists.
	 */
	for (c = n->cases; c; c = c->next) {
	    if (!c->expr) continue;
	    EVAL(c->expr, context, &cc1);
	    GetValue(context, &cc1, &cc2);
	    _SEE_EqualityExpression_seq(context->interpreter, input, 
                        &cc2, &cc3);
	    if (cc3.u.boolean)
		break;
	}
	if (!c)
	    c = n->defcase;	/* can be NULL, meaning no default */
	_SEE_SET_COMPLETION(res, SEE_COMPLETION_NORMAL, NULL, NO_TARGET);
	for (; c; c = c->next) {
	    if (c->body)
		EVAL(c->body, context, res);
	    if (res->u.completion.type != SEE_COMPLETION_NORMAL)
		break;
	}
}

/* 12.11 */
static void
SwitchStatement_eval(na, context, res)
	struct node *na; /* (struct SwitchStatement_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct SwitchStatement_node *n = CAST_NODE(na, SwitchStatement);
	struct SEE_value *v, r1, r2;

	TRACE(&na->location, context, SEE_TRACE_STATEMENT);
	EVAL(n->cond, context, &r1);
	GetValue(context, &r1, &r2);
	SwitchStatement_caseblock(n, context, &r2, res);
	if (res->u.completion.type == SEE_COMPLETION_BREAK &&
	    n->target == res->u.completion.target)
	{
		v = res->u.completion.value;
		_SEE_SET_COMPLETION(res, SEE_COMPLETION_NORMAL, v, NO_TARGET);
	}
}

/* 12.12 */
static void
LabelledStatement_eval(na, context, res)
	struct node *na; /* (struct LabelledStatement_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct LabelledStatement_node *n = CAST_NODE(na, LabelledStatement);

	EVAL(n->unary.a, context, res);
	if (res->u.completion.type == SEE_COMPLETION_BREAK &&
		res->u.completion.target == n->target)
	{
	    res->u.completion.type = SEE_COMPLETION_NORMAL;
	    res->u.completion.target = NO_TARGET;
	}
}

/* 12.13 */
static void
ThrowStatement_eval(na, context, res)
	struct node *na; /* (struct Unary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Unary_node *n = CAST_NODE(na, Unary);
	struct SEE_value r1, r2;

	TRACE(&na->location, context, SEE_TRACE_STATEMENT);
	EVAL(n->a, context, &r1);
	GetValue(context, &r1, &r2);

	traceback_enter(context->interpreter, 0, &n->node.location, 
	    SEE_CALLTYPE_THROW);
	TRACE(&na->location, context, SEE_TRACE_THROW);
	SEE_THROW(context->interpreter, &r2);

	/* NOTREACHED */
}

/*
 * Helper function to evaluate the catch clause in a new scope.
 * Return true if an exception was caught while executing the
 * catch clause.
 */
static int
TryStatement_catch(n, context, C, res, ctxt)
	struct TryStatement_node *n;
	struct SEE_context *context;
	struct SEE_value *C, *res;
	SEE_try_context_t *ctxt;
{
	struct SEE_object *r2;
	struct SEE_scope *s;
	struct SEE_interpreter *interp = context->interpreter;

	r2 = SEE_Object_new(interp);
	SEE_OBJECT_PUT(interp, r2, n->ident, C, SEE_ATTR_DONTDELETE);
	s = SEE_NEW(interp, struct SEE_scope);
	s->obj = r2;
	s->next = context->scope;
	context->scope = s;
	SEE_TRY(interp, *ctxt)
	    EVAL(n->bcatch, context, res);
	context->scope = context->scope->next;
	return SEE_CAUGHT(*ctxt) != NULL;
}

/* 12.14 try catch */
static void
TryStatement_catch_eval(na, context, res)
	struct node *na; /* (struct TryStatement_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct TryStatement_node *n = CAST_NODE(na, TryStatement);
	SEE_try_context_t block_ctxt, catch_ctxt;

	TRACE(&na->location, context, SEE_TRACE_STATEMENT);
	SEE_TRY(context->interpreter, block_ctxt)
		EVAL(n->block, context, res);
	if (SEE_CAUGHT(block_ctxt))
		if (TryStatement_catch(n, context, SEE_CAUGHT(block_ctxt),
			res, &catch_ctxt)) 
		{
		    TRACE(&na->location, context, SEE_TRACE_THROW);
		    SEE_RETHROW(context->interpreter, catch_ctxt);
		} 
}

/* 12.14 try finally */
static void
TryStatement_finally_eval(na, context, res)
	struct node *na; /* (struct TryStatement_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct TryStatement_node *n = CAST_NODE(na, TryStatement);
	struct SEE_value r2;
	SEE_try_context_t ctxt;

	TRACE(&na->location, context, SEE_TRACE_STATEMENT);
	SEE_TRY(context->interpreter, ctxt)
	    EVAL(n->block, context, res);
	EVAL(n->bfinally, context, &r2);
	if (SEE_VALUE_GET_TYPE(&r2) == SEE_COMPLETION &&
		r2.u.completion.type != SEE_COMPLETION_NORMAL)
	    SEE_VALUE_COPY(res, &r2); 		/* break, return etc */
	else if (SEE_CAUGHT(ctxt)) {
	    TRACE(&na->location, context, SEE_TRACE_THROW);
	    SEE_RETHROW(context->interpreter, ctxt);
	}
}

/* 12.14 try catch finally */
static void
TryStatement_catchfinally_eval(na, context, res)
	struct node *na; /* (struct TryStatement_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct TryStatement_node *n = CAST_NODE(na, TryStatement);
	struct SEE_value r6;
	SEE_try_context_t block_ctxt, finally_ctxt, catch_ctxt, *C = NULL;
	struct SEE_interpreter *interp = context->interpreter;

	TRACE(&na->location, context, SEE_TRACE_STATEMENT);
	SEE_TRY(interp, block_ctxt)
/*1*/		EVAL(n->block, context, res);
/*3*/	if (SEE_CAUGHT(block_ctxt))  {
		C = &block_ctxt;
/*4*/		if (TryStatement_catch(n, context, SEE_CAUGHT(block_ctxt),
			res, &catch_ctxt))
/*5*/		    C = &catch_ctxt;
		else
		    C = NULL;
	}

	SEE_TRY(interp, finally_ctxt)
/*6*/		EVAL(n->bfinally, context, &r6);
	if (SEE_CAUGHT(finally_ctxt))
		C = &finally_ctxt;
	else if (SEE_VALUE_GET_TYPE(&r6) == SEE_COMPLETION &&
		    r6.u.completion.type != SEE_COMPLETION_NORMAL)
		SEE_VALUE_COPY(res, &r6);	/* break, return etc */

	if (C) {
		TRACE(&na->location, context, SEE_TRACE_THROW);
		SEE_RETHROW(interp, *C);
	}
}

#if 0
/* 13 */
static void
FunctionDeclaration_eval(na, context, res)
	struct node *na; /* (struct Function_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Function_node *n = CAST_NODE(na, Function);
	_SEE_SET_COMPLETION(res, SEE_COMPLETION_NORMAL, NULL, NO_TARGET); /* 14 */
}
#endif

static void
FunctionDeclaration_fproc(na, context)
	struct node *na; /* struct Function_node */
	struct SEE_context *context;
{
	struct Function_node *n = CAST_NODE(na, Function);
	struct SEE_object *funcobj;
	struct SEE_value   funcval;

	/* 10.1.3 */
	funcobj = SEE_function_inst_create(context->interpreter,
	    n->function, context->scope);
	SEE_SET_OBJECT(&funcval, funcobj);
	SEE_OBJECT_PUT(context->interpreter, context->variable,
	    n->function->name, &funcval, context->varattr);
}

/* 13 */
static void
FunctionExpression_eval(na, context, res)
	struct node *na; /* (struct Function_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Function_node *n = CAST_NODE(na, Function);
	struct SEE_object *funcobj = NULL, *obj;
	struct SEE_value   v;
	struct SEE_scope  *scope;
	SEE_try_context_t  ctxt;
	struct SEE_interpreter *interp = context->interpreter;

	if (n->function->name == NULL) {
	    funcobj = SEE_function_inst_create(interp,
	        n->function, context->scope);
            SEE_SET_OBJECT(res, funcobj);
	} else {
	    /*
	     * Construct a single scope step that lets the
	     * function call itself recursively
	     */
	    obj = SEE_Object_new(interp);

	    scope = SEE_NEW(interp, struct SEE_scope);
	    scope->obj = obj;
	    scope->next = context->scope;
	    context->scope = scope;

	    /* Be careful to restore the scope on any exception! */
	    SEE_TRY(interp, ctxt) {
	        funcobj = SEE_function_inst_create(interp,
	            n->function, context->scope);
	        SEE_SET_OBJECT(&v, funcobj);
	        SEE_OBJECT_PUT(interp, obj, n->function->name, &v,
		    SEE_ATTR_DONTDELETE | SEE_ATTR_READONLY);
                SEE_SET_OBJECT(res, funcobj);
	    }
	    context->scope = context->scope->next;
	    SEE_DEFAULT_CATCH(interp, ctxt);	/* re-throw any exception */
	}
}

/* 13 */
static void
FunctionBody_eval(na, context, res)
	struct node *na; /* (struct Unary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct FunctionBody_node *n = CAST_NODE(na, FunctionBody);
	struct SEE_value v;

	FPROC(n->u.a, context);
	EVAL(n->u.a, context, &v);

	SEE_ASSERT(context->interpreter,
	    SEE_VALUE_GET_TYPE(&v) == SEE_COMPLETION);
	SEE_ASSERT(context->interpreter,
	    v.u.completion.type == SEE_COMPLETION_NORMAL ||
	    v.u.completion.type == SEE_COMPLETION_RETURN);

	/* Functions convert 'normal' completion to 'return undefined',
	 * while Programs return the value from their last 'normal' 
	 * completion. */
	if ((!n->is_program && v.u.completion.type == SEE_COMPLETION_NORMAL) ||
		v.u.completion.value == NULL)
	    SEE_SET_UNDEFINED(res);
	else
	    SEE_VALUE_COPY(res, v.u.completion.value);
}

/* 14 */
static void
SourceElements_eval(na, context, res)
	struct node *na; /* (struct SourceElements_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct SourceElements_node *n = CAST_NODE(na, SourceElements);
	struct SourceElement *e;

	/*
	 * NB: strictly, this should 'evaluate' the
	 * FunctionDeclarations, but they only yield <NORMAL, NULL, NULL>
	 * so, we don't. We just run the non-functiondecl statements
	 * instead. It has the same result.
	 */
	_SEE_SET_COMPLETION(res, SEE_COMPLETION_NORMAL, NULL, NO_TARGET);
	for (e = n->statements; e; e = e->next) {
		EVAL(e->node, context, res);
		if (res->u.completion.type != SEE_COMPLETION_NORMAL)
			break;
	}
}

static void
SourceElements_fproc(na, context)
	struct node *na; /* struct SourceElements_node */
	struct SEE_context *context;
{
	struct SourceElements_node *n = CAST_NODE(na, SourceElements);
	struct SourceElement *e;
	struct var *v;
	struct SEE_value undefv;

	for (e = n->functions; e; e = e->next)
		FPROC(e->node, context);

	/*
	 * spec bug(?): although not mentioned in the spec, this
	 * is the place to set the declared variables
	 * to undefined. (10.1.3). 
	 * (I say 'spec bug' because there is partial overlap
	 * between 10.1.3 and the semantics of 13.)
	 */
	SEE_SET_UNDEFINED(&undefv);
	for (v = n->vars; v; v = v->next)
	    if (!SEE_OBJECT_HASPROPERTY(context->interpreter,
		context->variable, v->name))
	            SEE_OBJECT_PUT(context->interpreter, context->variable, 
			v->name, &undefv, context->varattr);
}

void
_SEE_eval_eval_functionbody(body, context, res)
        void *body;
        struct SEE_context *context;
        struct SEE_value *res;
{
        EVAL((struct node *)body, context, res);
}

void *
_SEE_eval_make_body(interp, node, no_const)
        struct SEE_interpreter *interp;
        struct node *node;
        int no_const;
{
        return node;
}

/* Returns true if the FunctionBody is empty. */
int
_SEE_eval_functionbody_isempty(interp, f)
        struct SEE_interpreter *interp;
        struct function *f;
{
        return _SEE_node_functionbody_isempty(interp, (struct node *)f->body);
}

/*
 * Table of evaluators used when executable ASTs are enabled
 */
void (*_SEE_nodeclass_eval[NODECLASS_MAX])(struct node *, 
        struct SEE_context *, struct SEE_value *) = { 0
    ,0                                      /*Unary*/
    ,0                                      /*Binary*/
    ,Literal_eval                           /*Literal*/
    ,StringLiteral_eval                     /*StringLiteral*/
    ,RegularExpressionLiteral_eval          /*RegularExpressionLiteral*/
    ,PrimaryExpression_this_eval            /*PrimaryExpression_this*/
    ,PrimaryExpression_ident_eval           /*PrimaryExpression_ident*/
    ,ArrayLiteral_eval                      /*ArrayLiteral*/
    ,ObjectLiteral_eval                     /*ObjectLiteral*/
    ,Arguments_eval                         /*Arguments*/
    ,MemberExpression_new_eval              /*MemberExpression_new*/
    ,MemberExpression_dot_eval              /*MemberExpression_dot*/
    ,MemberExpression_bracket_eval          /*MemberExpression_bracket*/
    ,CallExpression_eval                    /*CallExpression*/
    ,PostfixExpression_inc_eval             /*PostfixExpression_inc*/
    ,PostfixExpression_dec_eval             /*PostfixExpression_dec*/
    ,UnaryExpression_delete_eval            /*UnaryExpression_delete*/
    ,UnaryExpression_void_eval              /*UnaryExpression_void*/
    ,UnaryExpression_typeof_eval            /*UnaryExpression_typeof*/
    ,UnaryExpression_preinc_eval            /*UnaryExpression_preinc*/
    ,UnaryExpression_predec_eval            /*UnaryExpression_predec*/
    ,UnaryExpression_plus_eval              /*UnaryExpression_plus*/
    ,UnaryExpression_minus_eval             /*UnaryExpression_minus*/
    ,UnaryExpression_inv_eval               /*UnaryExpression_inv*/
    ,UnaryExpression_not_eval               /*UnaryExpression_not*/
    ,MultiplicativeExpression_mul_eval      /*MultiplicativeExpression_mul*/
    ,MultiplicativeExpression_div_eval      /*MultiplicativeExpression_div*/
    ,MultiplicativeExpression_mod_eval      /*MultiplicativeExpression_mod*/
    ,AdditiveExpression_add_eval            /*AdditiveExpression_add*/
    ,AdditiveExpression_sub_eval            /*AdditiveExpression_sub*/
    ,ShiftExpression_lshift_eval            /*ShiftExpression_lshift*/
    ,ShiftExpression_rshift_eval            /*ShiftExpression_rshift*/
    ,ShiftExpression_urshift_eval           /*ShiftExpression_urshift*/
    ,RelationalExpression_lt_eval           /*RelationalExpression_lt*/
    ,RelationalExpression_gt_eval           /*RelationalExpression_gt*/
    ,RelationalExpression_le_eval           /*RelationalExpression_le*/
    ,RelationalExpression_ge_eval           /*RelationalExpression_ge*/
    ,RelationalExpression_instanceof_eval   /*RelationalExpression_instanceof*/
    ,RelationalExpression_in_eval           /*RelationalExpression_in*/
    ,EqualityExpression_eq_eval             /*EqualityExpression_eq*/
    ,EqualityExpression_ne_eval             /*EqualityExpression_ne*/
    ,EqualityExpression_seq_eval            /*EqualityExpression_seq*/
    ,EqualityExpression_sne_eval            /*EqualityExpression_sne*/
    ,BitwiseANDExpression_eval              /*BitwiseANDExpression*/
    ,BitwiseXORExpression_eval              /*BitwiseXORExpression*/
    ,BitwiseORExpression_eval               /*BitwiseORExpression*/
    ,LogicalANDExpression_eval              /*LogicalANDExpression*/
    ,LogicalORExpression_eval               /*LogicalORExpression*/
    ,ConditionalExpression_eval             /*ConditionalExpression*/
    ,0                                      /*AssignmentExpression*/
    ,AssignmentExpression_simple_eval       /*AssignmentExpression_simple*/
    ,AssignmentExpression_muleq_eval        /*AssignmentExpression_muleq*/
    ,AssignmentExpression_diveq_eval        /*AssignmentExpression_diveq*/
    ,AssignmentExpression_modeq_eval        /*AssignmentExpression_modeq*/
    ,AssignmentExpression_addeq_eval        /*AssignmentExpression_addeq*/
    ,AssignmentExpression_subeq_eval        /*AssignmentExpression_subeq*/
    ,AssignmentExpression_lshifteq_eval     /*AssignmentExpression_lshifteq*/
    ,AssignmentExpression_rshifteq_eval     /*AssignmentExpression_rshifteq*/
    ,AssignmentExpression_urshifteq_eval    /*AssignmentExpression_urshifteq*/
    ,AssignmentExpression_andeq_eval        /*AssignmentExpression_andeq*/
    ,AssignmentExpression_xoreq_eval        /*AssignmentExpression_xoreq*/
    ,AssignmentExpression_oreq_eval         /*AssignmentExpression_oreq*/
    ,Expression_comma_eval                  /*Expression_comma*/
    ,Block_empty_eval                       /*Block_empty*/
    ,StatementList_eval                     /*StatementList*/
    ,VariableStatement_eval                 /*VariableStatement*/
    ,VariableDeclarationList_eval           /*VariableDeclarationList*/
    ,VariableDeclaration_eval               /*VariableDeclaration*/
    ,EmptyStatement_eval                    /*EmptyStatement*/
    ,ExpressionStatement_eval               /*ExpressionStatement*/
    ,IfStatement_eval                       /*IfStatement*/
    ,IterationStatement_dowhile_eval        /*IterationStatement_dowhile*/
    ,IterationStatement_while_eval          /*IterationStatement_while*/
    ,IterationStatement_for_eval            /*IterationStatement_for*/
    ,IterationStatement_forvar_eval         /*IterationStatement_forvar*/
    ,IterationStatement_forin_eval          /*IterationStatement_forin*/
    ,IterationStatement_forvarin_eval       /*IterationStatement_forvarin*/
    ,ContinueStatement_eval                 /*ContinueStatement*/
    ,BreakStatement_eval                    /*BreakStatement*/
    ,ReturnStatement_eval                   /*ReturnStatement*/
    ,ReturnStatement_undef_eval             /*ReturnStatement_undef*/
    ,WithStatement_eval                     /*WithStatement*/
    ,SwitchStatement_eval                   /*SwitchStatement*/
    ,LabelledStatement_eval                 /*LabelledStatement*/
    ,ThrowStatement_eval                    /*ThrowStatement*/
    ,0                                      /*TryStatement*/
    ,TryStatement_catch_eval                /*TryStatement_catch*/
    ,TryStatement_finally_eval              /*TryStatement_finally*/
    ,TryStatement_catchfinally_eval         /*TryStatement_catchfinally*/
    ,0                                      /*Function*/
    ,0 /* FunctionDeclaration_eval */       /*FunctionDeclaration*/
    ,FunctionExpression_eval                /*FunctionExpression*/
    ,FunctionBody_eval                      /*FunctionBody*/
    ,SourceElements_eval                    /*SourceElements*/
};
