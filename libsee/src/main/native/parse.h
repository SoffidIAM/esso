/* Copyright (c) 2003, David Leonard. All rights reserved. */

#ifndef _SEE_h_parse_
#define _SEE_h_parse_

struct SEE_string;
struct SEE_value;
struct SEE_interpreter;
struct SEE_context;
struct SEE_input;
struct function;

struct function *SEE_parse_function(struct SEE_interpreter *i,
	struct SEE_string *name, struct SEE_input *param_input, 
	struct SEE_input *body_input);
void  SEE_eval_functionbody(struct function *f,
	struct SEE_context *context, struct SEE_value *res);
struct function *SEE_parse_program(struct SEE_interpreter *i, 
	struct SEE_input *input);

void  SEE_functionbody_print(struct SEE_interpreter *i, struct function *f);
struct SEE_string *SEE_functionbody_string(struct SEE_interpreter *i, 
	struct function *f);
int SEE_functionbody_isempty(struct SEE_interpreter *i, struct function *f);

void _SEE_call_eval(struct SEE_context *context, 
        struct SEE_object *thisobj, int argc, struct SEE_value **argv, 
        struct SEE_value *res);

void _SEE_eval_input(struct SEE_context *context, 
        struct SEE_object *thisobj, struct SEE_input *inp,
        struct SEE_value *res);

#endif /* _SEE_h_parse_ */
