/* Copyright (c) 2006, David Leonard. All rights reserved. */

#ifndef _SEE_h_printf_
#define _SEE_h_printf_

struct SEE_string;
struct SEE_interpreter;

void _SEE_vsprintf(struct SEE_interpreter *interp, struct SEE_string *,
			  const char *fmt, va_list ap);

#endif /* _SEE_h_printf_ */
