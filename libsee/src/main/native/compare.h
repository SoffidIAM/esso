/* Copyright (c) 2009, David Leonard. All rights reserved. */

struct SEE_interpreter;
struct SEE_value;

void _SEE_RelationalExpression_sub(struct SEE_interpreter *interp,
        struct SEE_value *x, struct SEE_value *y, struct SEE_value *res);
void _SEE_EqualityExpression_eq(struct SEE_interpreter *interp,
        struct SEE_value *x, struct SEE_value *y, struct SEE_value *res);
void _SEE_EqualityExpression_seq(struct SEE_interpreter *interp,
        struct SEE_value *x, struct SEE_value *y, struct SEE_value *res);
int SEE_compare(struct SEE_interpreter *i, struct SEE_value *x, 
	struct SEE_value *y);

