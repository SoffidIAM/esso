/* (c) 2009 David Leonard. All rights reserved. */

struct printer;
struct node;
struct SEE_interpreter;
struct SEE_string;

void _SEE_parser_print(struct printer *printer, struct node *node);

struct printer *_SEE_parser_print_stdio_new(struct SEE_interpreter *interp, 
        FILE *file);

struct printer *_SEE_parser_print_string_new(struct SEE_interpreter *interp, 
        struct SEE_string *s);

