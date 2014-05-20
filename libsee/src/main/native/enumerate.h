/* Copyright (c) 2003, David Leonard. All rights reserved. */

#ifndef _SEE_h_enumerate_
#define _SEE_h_enumerate_

struct SEE_interpreter;
struct SEE_string;
struct SEE_object;

struct SEE_string **SEE_enumerate(struct SEE_interpreter *i,
	struct SEE_object *o);
void SEE_enumerate_free(struct SEE_interpreter *i, struct SEE_string **props);

#endif /* _SEE_h_enumerate_ */
