/* (c) 2009 David Leonard. All rights reserved. */

struct SEE_context;
struct SEE_value;
struct SEE_interpreter;
struct function;
struct node;

void _SEE_eval_eval_functionbody(void *body, struct SEE_context *context,
        struct SEE_value *res);
void *_SEE_eval_make_body(struct SEE_interpreter *interp,
        struct node *node, int no_const);
int _SEE_eval_functionbody_isempty(struct SEE_interpreter *interp,
        struct function *f);



