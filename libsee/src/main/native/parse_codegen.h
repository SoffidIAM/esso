/* (c) 2009 David Leonard. All rights reserved. */

struct SEE_interpreter;
struct SEE_context;
struct SEE_value;
struct function;
struct node;

int _SEE_codegen_functionbody_isempty(struct SEE_interpreter *interp,
            struct function *f);
void * _SEE_codegen_make_body(struct SEE_interpreter *interp,
            struct node *node, int no_const);
void _SEE_codegen_eval_functionbody(void *body, struct SEE_context *context,
            struct SEE_value *res);
