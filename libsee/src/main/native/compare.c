/* (c) 2009 David Leonard. All rights reserved. */
#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <see/interpreter.h>
#include <see/value.h>
#include <see/object.h>
#include <see/string.h>
#include <see/system.h>
#include <see/error.h>

#include "compare.h"

/*
 * 11.8.5 Abstract relational comparison function.
 */
void
_SEE_RelationalExpression_sub(interp, x, y, res)
        struct SEE_interpreter *interp;
        struct SEE_value *x, *y, *res;
{
        struct SEE_value r1, r2, r4, r5;
        struct SEE_value hint;
        int k;

        SEE_SET_OBJECT(&hint, interp->Number);

        SEE_ToPrimitive(interp, x, &hint, &r1);
        SEE_ToPrimitive(interp, y, &hint, &r2);
        if (!(SEE_VALUE_GET_TYPE(&r1) == SEE_STRING &&
              SEE_VALUE_GET_TYPE(&r2) == SEE_STRING))
        {
            SEE_ToNumber(interp, &r1, &r4);
            SEE_ToNumber(interp, &r2, &r5);
            if (SEE_NUMBER_ISNAN(&r4) || SEE_NUMBER_ISNAN(&r5))
                SEE_SET_UNDEFINED(res);
            else if (r4.u.number == r5.u.number)
                SEE_SET_BOOLEAN(res, 0);
            else if (SEE_NUMBER_ISPINF(&r4))
                SEE_SET_BOOLEAN(res, 0);
            else if (SEE_NUMBER_ISPINF(&r5))
                SEE_SET_BOOLEAN(res, 1);
            else if (SEE_NUMBER_ISNINF(&r5))
                SEE_SET_BOOLEAN(res, 0);
            else if (SEE_NUMBER_ISNINF(&r4))
                SEE_SET_BOOLEAN(res, 1);
            else
                SEE_SET_BOOLEAN(res, r4.u.number < r5.u.number);
        } else {
            for (k = 0;
                 k < r1.u.string->length && k < r2.u.string->length;
                 k++)
                if (r1.u.string->data[k] != r2.u.string->data[k])
                        break;
            if (k == r2.u.string->length)
                SEE_SET_BOOLEAN(res, 0);
            else if (k == r1.u.string->length)
                SEE_SET_BOOLEAN(res, 1);
            else
                SEE_SET_BOOLEAN(res, r1.u.string->data[k] <
                                 r2.u.string->data[k]);
        }
}

/*
 * 11.9.3 Abstract equality function.
 */
void
_SEE_EqualityExpression_eq(interp, x, y, res)
        struct SEE_interpreter *interp;
        struct SEE_value *x, *y, *res;
{
        struct SEE_value tmp;
        int xtype, ytype;

        if (SEE_VALUE_GET_TYPE(x) == SEE_VALUE_GET_TYPE(y))
            switch (SEE_VALUE_GET_TYPE(x)) {
            case SEE_UNDEFINED:
            case SEE_NULL:
                SEE_SET_BOOLEAN(res, 1);
                return;
            case SEE_NUMBER:
                if (SEE_NUMBER_ISNAN(x) || SEE_NUMBER_ISNAN(y))
                    SEE_SET_BOOLEAN(res, 0);
                else
                    SEE_SET_BOOLEAN(res, x->u.number == y->u.number);
                return;
            case SEE_STRING:
                SEE_SET_BOOLEAN(res, SEE_string_cmp(x->u.string,
                    y->u.string) == 0);
                return;
            case SEE_BOOLEAN:
                SEE_SET_BOOLEAN(res, !x->u.boolean == !y->u.boolean);
                return;
            case SEE_OBJECT:
                SEE_SET_BOOLEAN(res,
                        SEE_OBJECT_JOINED(x->u.object, y->u.object));
                return;
            default:
                SEE_ASSERT(interp, !"unexpected token");
            }
        xtype = SEE_VALUE_GET_TYPE(x);
        ytype = SEE_VALUE_GET_TYPE(y);
        if (xtype == SEE_NULL && ytype == SEE_UNDEFINED)
                SEE_SET_BOOLEAN(res, 1);
        else if (xtype == SEE_UNDEFINED && ytype == SEE_NULL)
                SEE_SET_BOOLEAN(res, 1);
        else if (xtype == SEE_NUMBER && ytype == SEE_STRING) {
                SEE_ToNumber(interp, y, &tmp);
                _SEE_EqualityExpression_eq(interp, x, &tmp, res);
        } else if (xtype == SEE_STRING && ytype == SEE_NUMBER) {
                SEE_ToNumber(interp, x, &tmp);
                _SEE_EqualityExpression_eq(interp, &tmp, y, res);
        } else if (xtype == SEE_BOOLEAN) {
                SEE_ToNumber(interp, x, &tmp);
                _SEE_EqualityExpression_eq(interp, &tmp, y, res);
        } else if (ytype == SEE_BOOLEAN) {
                SEE_ToNumber(interp, y, &tmp);
                _SEE_EqualityExpression_eq(interp, x, &tmp, res);
        } else if ((xtype == SEE_STRING || xtype == SEE_NUMBER) &&
                    ytype == SEE_OBJECT) {
                SEE_ToPrimitive(interp, y, x, &tmp);
                _SEE_EqualityExpression_eq(interp, x, &tmp, res);
        } else if ((ytype == SEE_STRING || ytype == SEE_NUMBER) &&
                    xtype == SEE_OBJECT) {
                SEE_ToPrimitive(interp, x, y, &tmp);
                _SEE_EqualityExpression_eq(interp, &tmp, y, res);
        } else
                SEE_SET_BOOLEAN(res, 0);
}

/*
 * 19.9.6 Strict equality function
 */
void
_SEE_EqualityExpression_seq(interp, x, y, res)
	struct SEE_interpreter *interp;
	struct SEE_value *x, *y, *res;
{
	if (SEE_VALUE_GET_TYPE(x) != SEE_VALUE_GET_TYPE(y))
	    SEE_SET_BOOLEAN(res, 0);
	else
	    switch (SEE_VALUE_GET_TYPE(x)) {
	    case SEE_UNDEFINED:
		SEE_SET_BOOLEAN(res, 1);
		break;
	    case SEE_NULL:
		SEE_SET_BOOLEAN(res, 1);
		break;
	    case SEE_NUMBER:
		if (SEE_NUMBER_ISNAN(x) || SEE_NUMBER_ISNAN(y))
			SEE_SET_BOOLEAN(res, 0);
		else
			SEE_SET_BOOLEAN(res, x->u.number == y->u.number);
		break;
	    case SEE_STRING:
		SEE_SET_BOOLEAN(res, SEE_string_cmp(x->u.string, 
		    y->u.string) == 0);
		break;
	    case SEE_BOOLEAN:
		SEE_SET_BOOLEAN(res, !x->u.boolean == !y->u.boolean);
		break;
	    case SEE_OBJECT:
		SEE_SET_BOOLEAN(res, 
			SEE_OBJECT_JOINED(x->u.object, y->u.object));
		break;
	    default:
		SEE_SET_BOOLEAN(res, 0);
	    }
}

/*
 * Compares two value using ECMAScript == and > operator semantics.
 * Returns  0 if x == y,
 *          1 if x > y or indeterminate,
 *         -1 otherwise.
 * This could be used as a better comparsion function for Array.sort().
 * Currently only used by RegExp.prototype.test()
 */
int
SEE_compare(interp, x, y)
        struct SEE_interpreter *interp;
        struct SEE_value *x, *y;
{
        struct SEE_value v;

        _SEE_EqualityExpression_eq(interp, x, y, &v);
        if (v.u.boolean)
                return 0;
        _SEE_RelationalExpression_sub(interp, x, y, &v);
        if (SEE_VALUE_GET_TYPE(&v) == SEE_UNDEFINED || !v.u.boolean)
                return 1;
        else
                return -1;
}

