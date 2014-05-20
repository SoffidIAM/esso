/* (c) 2009 David Leonard. All rights reserved. */
#if HAVE_CONFIG_H
# include <config.h>
#endif

#if STDC_HEADERS
# include <stdio.h>
#endif

#include <see/string.h>
#include <see/value.h>
#include <see/interpreter.h>
#include <see/try.h>
#include <see/error.h>
#include <see/system.h>
#include <see/mem.h>

#include "function.h"
#include "stringdefs.h"
#include "parse_node.h"
#include "parse_print.h"

extern void (*_SEE_nodeclass_print[])(struct node *, 
        struct printer *);

struct printerclass {
	void	(*print_string)(struct printer *, struct SEE_string *);
	void	(*print_char)(struct printer *, int);
	void	(*print_newline)(struct printer *, int);
	void	(*print_node)(struct printer *, struct node *);
};

struct printer {
	struct printerclass *printerclass;
	struct SEE_interpreter *interpreter;
	int	indent;
	int	bol;
};

static void printer_init(struct printer *printer, 
        struct SEE_interpreter *interp, struct printerclass *printerclass);
static void printer_atbol(struct printer *printer);
static void printer_print_newline(struct printer *printer, int indent);
static void printer_print_node(struct printer *printer, struct node *n);
static void print_hex(struct printer *printer, int i);
static void stdio_print_string(struct printer *printer, 
        struct SEE_string *s);
static void stdio_print_char(struct printer *printer, int c);
static void stdio_print_node(struct printer *printer, struct node *n);
static struct printer *stdio_printer_new(struct SEE_interpreter *interp, 
        FILE *output);
static void string_print_string(struct printer *printer, 
        struct SEE_string *s);
static void string_print_char(struct printer *printer, int c);
static struct printer *string_printer_new(struct SEE_interpreter *interp, 
        struct SEE_string *string);

static void NumericLiteral_print(struct Literal_node *n,
        struct printer *printer);
static void Label_print(unsigned int target, struct printer *printer);

#define PRINTFN(n) _SEE_nodeclass_print[(n)->nodeclass]
#define PRINT(n)	 (*printer->printerclass->print_node)(printer, n)
#define PRINT_STRING(s)  (*printer->printerclass->print_string)(printer, s)
#define PRINT_CSTRING(s) (*printer->printerclass->print_cstring)(printer, s)
#define PRINT_CHAR(c)	 (*printer->printerclass->print_char)(printer, c)
#define PRINT_NEWLINE(i) (*printer->printerclass->print_newline)(printer, i)
#define PRINTP(n) do {					\
	     PRINT_CHAR('(');				\
	     PRINT(n);					\
	     PRINT_CHAR(')');				\
    } while (0)

void
_SEE_parser_print(struct printer *printer, struct node *node)
{
        PRINT(node);
}

struct printer *
_SEE_parser_print_stdio_new(struct SEE_interpreter *interp, FILE *file)
{
        return stdio_printer_new(interp, file);
}

struct printer *
_SEE_parser_print_string_new(struct SEE_interpreter *interp, 
        struct SEE_string *s)
{
        return string_printer_new(interp, s);
}


static void
Literal_print(na, printer)
	struct node *na; /* (struct Literal_node) */
	struct printer *printer;
{
	struct Literal_node *n = CAST_NODE(na, Literal);

	switch (SEE_VALUE_GET_TYPE(&n->value)) {
	case SEE_BOOLEAN:
		PRINT_STRING(n->value.u.boolean
			? STR(true)
			: STR(false));
		break;
	case SEE_NULL:
		PRINT_STRING(STR(null));
		break;
	case SEE_NUMBER:
		NumericLiteral_print(n, printer);
		break;
	default:
		PRINT_CHAR('?');
	}
	PRINT_CHAR(' ');
}

static void
NumericLiteral_print(n, printer)
	struct Literal_node *n;
	struct printer *printer;
{
	struct SEE_value numstr;

	SEE_ASSERT(printer->interpreter, 
	    SEE_VALUE_GET_TYPE(&n->value) == SEE_NUMBER);
	SEE_ToString(printer->interpreter, &n->value, &numstr);
	PRINT_STRING(numstr.u.string);
}

static void
StringLiteral_print(na, printer)
	struct node *na; /* (struct StringLiteral_node) */
	struct printer *printer;
{
	struct StringLiteral_node *n = CAST_NODE(na, StringLiteral);
	unsigned int i;

	PRINT_CHAR('"');
	for (i = 0; i < n->string->length; i ++) {
		SEE_char_t c = n->string->data[i];
		if (c == '\\' || c == '\"') {
			PRINT_CHAR('\\');
			PRINT_CHAR(c & 0x7f);
		} else if (c >= ' ' && c <= '~')
			PRINT_CHAR(c & 0x7f);
		else if (c < 0x100) {
			PRINT_CHAR('\\');
			PRINT_CHAR('x');
			PRINT_CHAR(SEE_hexstr_lowercase[ (c >> 4) & 0xf ]);
			PRINT_CHAR(SEE_hexstr_lowercase[ (c >> 0) & 0xf ]);
		} else {
			PRINT_CHAR('\\');
			PRINT_CHAR('u');
			PRINT_CHAR(SEE_hexstr_lowercase[ (c >>12) & 0xf ]);
			PRINT_CHAR(SEE_hexstr_lowercase[ (c >> 8) & 0xf ]);
			PRINT_CHAR(SEE_hexstr_lowercase[ (c >> 4) & 0xf ]);
			PRINT_CHAR(SEE_hexstr_lowercase[ (c >> 0) & 0xf ]);
		}
	}
	PRINT_CHAR('"');
	PRINT_CHAR(' ');
}

static void
RegularExpressionLiteral_print(na, printer)
	struct node *na; /* (struct RegularExpressionLiteral_node) */
	struct printer *printer;
{
	struct RegularExpressionLiteral_node *n = 
		CAST_NODE(na, RegularExpressionLiteral);
	PRINT_CHAR('/');
	PRINT_STRING(n->pattern.u.string);
	PRINT_CHAR('/');
	PRINT_STRING(n->flags.u.string);
	PRINT_CHAR(' ');
}

static void
PrimaryExpression_this_print(n, printer)
	struct printer *printer;
	struct node *n;
{
	PRINT_STRING(STR(this));
	PRINT_CHAR(' ');
}

static void
PrimaryExpression_ident_print(na, printer)
	struct node *na; /* (struct PrimaryExpression_ident_node) */
	struct printer *printer;
{
	struct PrimaryExpression_ident_node *n = 
		CAST_NODE(na, PrimaryExpression_ident);
	PRINT_STRING(n->string);
	PRINT_CHAR(' ');
}

static void
ArrayLiteral_print(na, printer)
	struct node *na; /* (struct ArrayLiteral_node) */
	struct printer *printer;
{
	struct ArrayLiteral_node *n = CAST_NODE(na, ArrayLiteral);
	struct ArrayLiteral_element *element;
	int pos;

	PRINT_CHAR('[');
	PRINT_CHAR(' ');
	for (pos = 0, element = n->first; element; element = element->next) {
		while (pos < element->index) {
			PRINT_CHAR(',');
			PRINT_CHAR(' ');
			pos++;
		}
		PRINT(element->expr);
	}
	while (pos < n->length) {
		PRINT_CHAR(',');
		PRINT_CHAR(' ');
		pos++;
	}
	PRINT_CHAR(']');
}

static void
ObjectLiteral_print(na, printer)
	struct node *na; /* (struct ObjectLiteral_node) */
	struct printer *printer;
{
	struct ObjectLiteral_node *n = CAST_NODE(na, ObjectLiteral);
	struct ObjectLiteral_pair *pair;

	PRINT_CHAR('{');
	PRINT_CHAR(' ');
	for (pair = n->first; pair; pair = pair->next) {
		if (pair != n->first) {
			PRINT_CHAR(',');
			PRINT_CHAR(' ');
		}
		PRINT_STRING(pair->name);
		PRINT_CHAR(':');
		PRINT_CHAR(' ');
		PRINT(pair->value);
	}
	PRINT_CHAR('}');
}

static void
Arguments_print(na, printer)
	struct node *na; /* (struct Arguments_node) */
	struct printer *printer;
{
	struct Arguments_node *n = CAST_NODE(na, Arguments);
	struct Arguments_arg *arg;

	PRINT_CHAR('(');
	for (arg = n->first; arg; arg = arg->next) {
		if (arg != n->first) {
			PRINT_CHAR(',');
			PRINT_CHAR(' ');
		}
		PRINTP(arg->expr);
	}
	PRINT_CHAR(')');
}

static void
MemberExpression_new_print(na, printer)
	struct node *na; /* (struct MemberExpression_new_node) */
	struct printer *printer;
{
	struct MemberExpression_new_node *n = 
		CAST_NODE(na, MemberExpression_new);
	PRINT_STRING(STR(new));
	PRINT_CHAR(' ');
	PRINTP(n->mexp);
	if (n->args)
		PRINT((struct node *)n->args);
}

static void
MemberExpression_dot_print(na, printer)
	struct node *na; /* (struct MemberExpression_dot_node) */
	struct printer *printer;
{
	struct MemberExpression_dot_node *n = 
		CAST_NODE(na, MemberExpression_dot);
	PRINTP(n->mexp);
	PRINT_CHAR('.');
	PRINT_STRING(n->name);
	PRINT_CHAR(' ');
}

static void
MemberExpression_bracket_print(na, printer)
	struct node *na; /* (struct MemberExpression_bracket_node) */
	struct printer *printer;
{
	struct MemberExpression_bracket_node *n = 
		CAST_NODE(na, MemberExpression_bracket);
	PRINTP(n->mexp);
	PRINT_CHAR('[');
	PRINT(n->name);
	PRINT_CHAR(']');
}

static void
CallExpression_print(na, printer)
	struct node *na; /* (struct CallExpression_node) */
	struct printer *printer;
{
	struct CallExpression_node *n = CAST_NODE(na, CallExpression);
	PRINTP(n->exp);
	PRINT((struct node *)n->args);
}

static void
Unary_print(na, printer)
	struct node *na; /* (struct Unary_node) */
	struct printer *printer;
{
	struct Unary_node *n = CAST_NODE(na, Unary);
	PRINT(n->a);
}

static void
PostfixExpression_inc_print(na, printer)
	struct node *na; /* (struct Unary_node) */
	struct printer *printer;
{
	struct Unary_node *n = CAST_NODE(na, Unary);
	PRINTP(n->a);
	PRINT_CHAR('+');
	PRINT_CHAR('+');
	PRINT_CHAR(' ');
}

static void
PostfixExpression_dec_print(na, printer)
	struct node *na; /* (struct Unary_node) */
	struct printer *printer;
{
	struct Unary_node *n = CAST_NODE(na, Unary);
	PRINTP(n->a);
	PRINT_CHAR('-');
	PRINT_CHAR('-');
	PRINT_CHAR(' ');
}

static void
UnaryExpression_delete_print(na, printer)
	struct node *na; /* (struct Unary_node) */
	struct printer *printer;
{
	struct Unary_node *n = CAST_NODE(na, Unary);
	PRINT_STRING(STR(delete));
	PRINT_CHAR(' ');
	PRINTP(n->a);
}

static void
UnaryExpression_void_print(na, printer)
	struct node *na; /* (struct Unary_node) */
	struct printer *printer;
{
	struct Unary_node *n = CAST_NODE(na, Unary);
	PRINT_STRING(STR(void));
	PRINT_CHAR(' ');
	PRINTP(n->a);
}

static void
UnaryExpression_typeof_print(na, printer)
	struct node *na; /* (struct Unary_node) */
	struct printer *printer;
{
	struct Unary_node *n = CAST_NODE(na, Unary);
	PRINT_STRING(STR(typeof));
	PRINT_CHAR(' ');
	PRINTP(n->a);
}

static void
UnaryExpression_preinc_print(na, printer)
	struct node *na; /* (struct Unary_node) */
	struct printer *printer;
{
	struct Unary_node *n = CAST_NODE(na, Unary);
	PRINT_CHAR('+');
	PRINT_CHAR('+');
	PRINT_CHAR(' ');
	PRINTP(n->a);
}

static void
UnaryExpression_predec_print(na, printer)
	struct node *na; /* (struct Unary_node) */
	struct printer *printer;
{
	struct Unary_node *n = CAST_NODE(na, Unary);
	PRINT_CHAR('-');
	PRINT_CHAR('-');
	PRINT_CHAR(' ');
	PRINTP(n->a);
}

static void
UnaryExpression_plus_print(na, printer)
	struct node *na; /* (struct Unary_node) */
	struct printer *printer;
{
	struct Unary_node *n = CAST_NODE(na, Unary);
	PRINT_CHAR('+');
	PRINT_CHAR(' ');
	PRINTP(n->a);
}

static void
UnaryExpression_minus_print(na, printer)
	struct node *na; /* (struct Unary_node) */
	struct printer *printer;
{
	struct Unary_node *n = CAST_NODE(na, Unary);
	PRINT_CHAR('-');
	PRINT_CHAR(' ');
	PRINTP(n->a);
}

static void
UnaryExpression_inv_print(na, printer)
	struct node *na; /* (struct Unary_node) */
	struct printer *printer;
{
	struct Unary_node *n = CAST_NODE(na, Unary);
	PRINT_CHAR('~');
	PRINT_CHAR(' ');
	PRINTP(n->a);
}

static void
UnaryExpression_not_print(na, printer)
	struct node *na; /* (struct Unary_node) */
	struct printer *printer;
{
	struct Unary_node *n = CAST_NODE(na, Unary);
	PRINT_CHAR('!');
	PRINT_CHAR(' ');
	PRINTP(n->a);
}

static void
Binary_print(na, printer)
	struct node *na; /* (struct Binary_node) */
	struct printer *printer;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	PRINT(n->a);
	PRINT(n->b);
}

static void
MultiplicativeExpression_mul_print(na, printer)
	struct node *na; /* (struct Binary_node) */
	struct printer *printer;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	PRINTP(n->a);
	PRINT_CHAR('*');
	PRINT_CHAR(' ');
	PRINTP(n->b);
}

static void
MultiplicativeExpression_div_print(na, printer)
	struct node *na; /* (struct Binary_node) */
	struct printer *printer;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	PRINTP(n->a);
	PRINT_CHAR('/');
	PRINT_CHAR(' ');
	PRINTP(n->b);
}

static void
MultiplicativeExpression_mod_print(na, printer)
	struct node *na; /* (struct Binary_node) */
	struct printer *printer;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	PRINTP(n->a);
	PRINT_CHAR('%');
	PRINT_CHAR(' ');
	PRINTP(n->b);
}

static void
AdditiveExpression_add_print(na, printer)
	struct node *na; /* (struct Binary_node) */
	struct printer *printer;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	PRINTP(n->a);
	PRINT_CHAR('+');
	PRINT_CHAR(' ');
	PRINTP(n->b);
}

static void
AdditiveExpression_sub_print(na, printer)
	struct node *na; /* (struct Binary_node) */
	struct printer *printer;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	PRINTP(n->a);
	PRINT_CHAR('-');
	PRINT_CHAR(' ');
	PRINTP(n->b);
}

static void
ShiftExpression_lshift_print(na, printer)
	struct node *na; /* (struct Binary_node) */
	struct printer *printer;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	PRINTP(n->a);
	PRINT_CHAR('<');
	PRINT_CHAR('<');
	PRINT_CHAR(' ');
	PRINTP(n->b);
}

static void
ShiftExpression_rshift_print(na, printer)
	struct node *na; /* (struct Binary_node) */
	struct printer *printer;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	PRINTP(n->a);
	PRINT_CHAR('>');
	PRINT_CHAR('>');
	PRINT_CHAR(' ');
	PRINTP(n->b);
}

static void
ShiftExpression_urshift_print(na, printer)
	struct node *na; /* (struct Binary_node) */
	struct printer *printer;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	PRINTP(n->a);
	PRINT_CHAR('>');
	PRINT_CHAR('>');
	PRINT_CHAR('>');
	PRINT_CHAR(' ');
	PRINTP(n->b);
}

static void
RelationalExpression_lt_print(na, printer)
	struct node *na; /* (struct Binary_node) */
	struct printer *printer;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	PRINTP(n->a);
	PRINT_CHAR('<');
	PRINT_CHAR(' ');
	PRINTP(n->b);
}

static void
RelationalExpression_gt_print(na, printer)
	struct node *na; /* (struct Binary_node) */
	struct printer *printer;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	PRINTP(n->a);
	PRINT_CHAR('>');
	PRINT_CHAR(' ');
	PRINTP(n->b);
}

static void
RelationalExpression_le_print(na, printer)
	struct node *na; /* (struct Binary_node) */
	struct printer *printer;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	PRINTP(n->a);
	PRINT_CHAR('<');
	PRINT_CHAR('=');
	PRINT_CHAR(' ');
	PRINTP(n->b);
}

static void
RelationalExpression_ge_print(na, printer)
	struct node *na; /* (struct Binary_node) */
	struct printer *printer;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	PRINTP(n->a);
	PRINT_CHAR('>');
	PRINT_CHAR('=');
	PRINT_CHAR(' ');
	PRINTP(n->b);
}

static void
RelationalExpression_instanceof_print(na, printer)
	struct node *na; /* (struct Binary_node) */
	struct printer *printer;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	PRINTP(n->a);
	PRINT_STRING(STR(instanceof));
	PRINT_CHAR(' ');
	PRINTP(n->b);
}

static void
RelationalExpression_in_print(na, printer)
	struct node *na; /* (struct Binary_node) */
	struct printer *printer;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	PRINTP(n->a);
	PRINT_STRING(STR(in));
	PRINT_CHAR(' ');
	PRINTP(n->b);
}

static void
EqualityExpression_eq_print(na, printer)
	struct node *na; /* (struct Binary_node) */
	struct printer *printer;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	PRINTP(n->a);
	PRINT_CHAR('=');
	PRINT_CHAR('=');
	PRINT_CHAR(' ');
	PRINTP(n->b);
}

static void
EqualityExpression_ne_print(na, printer)
	struct node *na; /* (struct Binary_node) */
	struct printer *printer;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	PRINTP(n->a);
	PRINT_CHAR('!');
	PRINT_CHAR('=');
	PRINT_CHAR(' ');
	PRINTP(n->b);
}

static void
EqualityExpression_seq_print(na, printer)
	struct node *na; /* (struct Binary_node) */
	struct printer *printer;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	PRINTP(n->a);
	PRINT_CHAR('=');
	PRINT_CHAR('=');
	PRINT_CHAR('=');
	PRINT_CHAR(' ');
	PRINTP(n->b);
}

static void
EqualityExpression_sne_print(na, printer)
	struct printer *printer;
	struct node *na; /* (struct Binary_node) */
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	PRINTP(n->a);
	PRINT_CHAR('!');
	PRINT_CHAR('=');
	PRINT_CHAR('=');
	PRINT_CHAR(' ');
	PRINTP(n->b);
}

static void
BitwiseANDExpression_print(na, printer)
	struct node *na; /* (struct Binary_node) */
	struct printer *printer;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	PRINTP(n->a);
	PRINT_CHAR('&');
	PRINT_CHAR(' ');
	PRINTP(n->b);
}

static void
BitwiseXORExpression_print(na, printer)
	struct node *na; /* (struct Binary_node) */
	struct printer *printer;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	PRINTP(n->a);
	PRINT_CHAR('^');
	PRINT_CHAR(' ');
	PRINTP(n->b);
}

static void
BitwiseORExpression_print(na, printer)
	struct node *na; /* (struct Binary_node) */
	struct printer *printer;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	PRINTP(n->a);
	PRINT_CHAR('|');
	PRINT_CHAR(' ');
	PRINTP(n->b);
}

static void
LogicalANDExpression_print(na, printer)
	struct node *na; /* (struct Binary_node) */
	struct printer *printer;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	PRINTP(n->a);
	PRINT_CHAR('&');
	PRINT_CHAR('&');
	PRINT_CHAR(' ');
	PRINTP(n->b);
}

static void
LogicalORExpression_print(na, printer)
	struct node *na; /* (struct Binary_node) */
	struct printer *printer;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	PRINTP(n->a);
	PRINT_CHAR('|');
	PRINT_CHAR('|');
	PRINT_CHAR(' ');
	PRINTP(n->b);
}

static void
ConditionalExpression_print(na, printer)
	struct node *na; /* (struct ConditionalExpression_node) */
	struct printer *printer;
{
	struct ConditionalExpression_node *n = 
		CAST_NODE(na, ConditionalExpression);
	PRINTP(n->a);
	PRINT_CHAR('?');
	PRINT_CHAR(' ');
	PRINTP(n->b);
	PRINT_CHAR(':');
	PRINT_CHAR(' ');
	PRINTP(n->c);
}

static void
AssignmentExpression_simple_print(na, printer)
	struct node *na; /* (struct AssignmentExpression_node) */
	struct printer *printer;
{
	struct AssignmentExpression_node *n = 
		CAST_NODE(na, AssignmentExpression);
	PRINTP(n->lhs);
	PRINT_CHAR('=');
	PRINT_CHAR(' ');
	PRINTP(n->expr);
}

static void
AssignmentExpression_muleq_print(na, printer)
	struct node *na; /* (struct AssignmentExpression_node) */
	struct printer *printer;
{
	struct AssignmentExpression_node *n = 
		CAST_NODE(na, AssignmentExpression);
	PRINTP(n->lhs);
	PRINT_CHAR('*');
	PRINT_CHAR('=');
	PRINT_CHAR(' ');
	PRINTP(n->expr);
}

static void
AssignmentExpression_diveq_print(na, printer)
	struct node *na; /* (struct AssignmentExpression_node) */
	struct printer *printer;
{
	struct AssignmentExpression_node *n = 
		CAST_NODE(na, AssignmentExpression);
	PRINTP(n->lhs);
	PRINT_CHAR('/');
	PRINT_CHAR('=');
	PRINT_CHAR(' ');
	PRINTP(n->expr);
}

static void
AssignmentExpression_modeq_print(na, printer)
	struct node *na; /* (struct AssignmentExpression_node) */
	struct printer *printer;
{
	struct AssignmentExpression_node *n = 
		CAST_NODE(na, AssignmentExpression);
	PRINTP(n->lhs);
	PRINT_CHAR('%');
	PRINT_CHAR('=');
	PRINT_CHAR(' ');
	PRINTP(n->expr);
}

static void
AssignmentExpression_addeq_print(na, printer)
	struct node *na; /* (struct AssignmentExpression_node) */
	struct printer *printer;
{
	struct AssignmentExpression_node *n = 
		CAST_NODE(na, AssignmentExpression);
	PRINTP(n->lhs);
	PRINT_CHAR('+');
	PRINT_CHAR('=');
	PRINT_CHAR(' ');
	PRINTP(n->expr);
}

static void
AssignmentExpression_subeq_print(na, printer)
	struct node *na; /* (struct AssignmentExpression_node) */
	struct printer *printer;
{
	struct AssignmentExpression_node *n = 
		CAST_NODE(na, AssignmentExpression);
	PRINTP(n->lhs);
	PRINT_CHAR('-');
	PRINT_CHAR('=');
	PRINT_CHAR(' ');
	PRINTP(n->expr);
}

static void
AssignmentExpression_lshifteq_print(na, printer)
	struct node *na; /* (struct AssignmentExpression_node) */
	struct printer *printer;
{
	struct AssignmentExpression_node *n = 
		CAST_NODE(na, AssignmentExpression);
	PRINTP(n->lhs);
	PRINT_CHAR('<');
	PRINT_CHAR('<');
	PRINT_CHAR('=');
	PRINT_CHAR(' ');
	PRINTP(n->expr);
}

static void
AssignmentExpression_rshifteq_print(na, printer)
	struct node *na; /* (struct AssignmentExpression_node) */
	struct printer *printer;
{
	struct AssignmentExpression_node *n = 
		CAST_NODE(na, AssignmentExpression);
	PRINTP(n->lhs);
	PRINT_CHAR('>');
	PRINT_CHAR('>');
	PRINT_CHAR('=');
	PRINT_CHAR(' ');
	PRINTP(n->expr);
}

static void
AssignmentExpression_urshifteq_print(na, printer)
	struct node *na; /* (struct AssignmentExpression_node) */
	struct printer *printer;
{
	struct AssignmentExpression_node *n = 
		CAST_NODE(na, AssignmentExpression);
	PRINTP(n->lhs);
	PRINT_CHAR('>');
	PRINT_CHAR('>');
	PRINT_CHAR('>');
	PRINT_CHAR('=');
	PRINT_CHAR(' ');
	PRINTP(n->expr);
}

static void
AssignmentExpression_andeq_print(na, printer)
	struct node *na; /* (struct AssignmentExpression_node) */
	struct printer *printer;
{
	struct AssignmentExpression_node *n = 
		CAST_NODE(na, AssignmentExpression);
	PRINTP(n->lhs);
	PRINT_CHAR('&');
	PRINT_CHAR('=');
	PRINT_CHAR(' ');
	PRINTP(n->expr);
}

static void
AssignmentExpression_xoreq_print(na, printer)
	struct node *na; /* (struct AssignmentExpression_node) */
	struct printer *printer;
{
	struct AssignmentExpression_node *n = 
		CAST_NODE(na, AssignmentExpression);
	PRINTP(n->lhs);
	PRINT_CHAR('^');
	PRINT_CHAR('=');
	PRINT_CHAR(' ');
	PRINTP(n->expr);
}

static void
AssignmentExpression_oreq_print(na, printer)
	struct node *na; /* (struct AssignmentExpression_node) */
	struct printer *printer;
{
	struct AssignmentExpression_node *n = 
		CAST_NODE(na, AssignmentExpression);
	PRINTP(n->lhs);
	PRINT_CHAR('|');
	PRINT_CHAR('=');
	PRINT_CHAR(' ');
	PRINTP(n->expr);
}

static void
Expression_comma_print(na, printer)
	struct node *na; /* (struct Binary_node) */
	struct printer *printer;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	PRINT(n->a);
	PRINT_CHAR(',');
	PRINT_CHAR(' ');
	PRINT(n->b);
}

static void
Block_empty_print(n, printer)
	struct node *n;
	struct printer *printer;
{
	PRINT_CHAR('{');
	PRINT_CHAR('}');
}

static void
VariableStatement_print(na, printer)
	struct node *na; /* (struct Unary_node) */
	struct printer *printer;
{
	struct Unary_node *n = CAST_NODE(na, Unary);
	PRINT_STRING(STR(var));
	PRINT_CHAR(' ');
	PRINT(n->a);
	PRINT_CHAR(';');
	PRINT_NEWLINE(0);
}

static void
VariableDeclarationList_print(na, printer)
	struct node *na; /* (struct Binary_node) */
	struct printer *printer;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	PRINT(n->a);
	PRINT_CHAR(',');
	PRINT_CHAR(' ');
	PRINT(n->b);
}

static void
VariableDeclaration_print(na, printer)
	struct node *na; /* (struct VariableDeclaration_node) */
	struct printer *printer;
{
	struct VariableDeclaration_node *n = 
		CAST_NODE(na, VariableDeclaration);
	PRINT_STRING(n->var->name);
	PRINT_CHAR(' ');
	if (n->init) {
		PRINT_CHAR('=');
		PRINT_CHAR(' ');
		PRINT(n->init);
	}
}

static void
EmptyStatement_print(n, printer)
	struct node *n;
	struct printer *printer;
{
	PRINT_CHAR(';');
	PRINT_NEWLINE(0);
}

static void
ExpressionStatement_print(na, printer)
	struct node *na; /* (struct Unary_node) */
	struct printer *printer;
{
	struct Unary_node *n = CAST_NODE(na, Unary);
	PRINT(n->a);
	PRINT_CHAR(';');
	PRINT_NEWLINE(0);
}

static void
IfStatement_print(na, printer)
	struct node *na; /* (struct IfStatement_node) */
	struct printer *printer;
{
	struct IfStatement_node *n = CAST_NODE(na, IfStatement);
	PRINT_STRING(STR(if));
	PRINT_CHAR(' ');
	PRINT_CHAR('(');
	PRINT(n->cond);
	PRINT_CHAR(')');
	PRINT_CHAR('{');
	PRINT_NEWLINE(1);
	PRINT(n->btrue);
	PRINT_CHAR('}');
	PRINT_NEWLINE(-1);
	if (n->bfalse) {
	    PRINT_STRING(STR(else));
	    PRINT_CHAR('{');
	    PRINT_NEWLINE(1);
	    PRINT(n->bfalse);
	    PRINT_CHAR('}');
	    PRINT_NEWLINE(-1);
	}
}

static void
IterationStatement_dowhile_print(na, printer)
	struct node *na; /* (struct IterationStatement_while_node) */
	struct printer *printer;
{
	struct IterationStatement_while_node *n = 
		CAST_NODE(na, IterationStatement_while);

	PRINT_STRING(STR(do));
	PRINT_CHAR('{');
	PRINT_NEWLINE(1);
	PRINT(n->body);
	PRINT_CHAR('}');
	PRINT_NEWLINE(-1);
	PRINT_STRING(STR(while));
	PRINT_CHAR(' ');
	PRINT_CHAR('(');
	PRINT(n->cond);
	PRINT_CHAR(')');
	PRINT_CHAR(';');
	PRINT_NEWLINE(0);
}

static void
IterationStatement_while_print(na, printer)
	struct node *na; /* (struct IterationStatement_while_node) */
	struct printer *printer;
{
	struct IterationStatement_while_node *n = 
		CAST_NODE(na, IterationStatement_while);

	PRINT_STRING(STR(while));
	PRINT_CHAR(' ');
	PRINT_CHAR('(');
	PRINT(n->cond);
	PRINT_CHAR(')');
	PRINT_CHAR('{');
	PRINT_NEWLINE(1);
	PRINT(n->body);
	PRINT_CHAR('}');
	PRINT_NEWLINE(-1);
}

static void
IterationStatement_for_print(na, printer)
	struct node *na; /* (struct IterationStatement_for_node) */
	struct printer *printer;
{
	struct IterationStatement_for_node *n = 
		CAST_NODE(na, IterationStatement_for);

	PRINT_STRING(STR(for));
	PRINT_CHAR(' ');
	PRINT_CHAR('(');
	if (n->init) PRINT(n->init);
	PRINT_CHAR(';');
	PRINT_CHAR(' ');
	if (n->cond) PRINT(n->cond);
	PRINT_CHAR(';');
	PRINT_CHAR(' ');
	if (n->incr) PRINT(n->incr);
	PRINT_CHAR(')');
	PRINT_CHAR('{');
	PRINT_NEWLINE(1);
	PRINT(n->body);
	PRINT_CHAR('}');
	PRINT_NEWLINE(-1);
}

static void
IterationStatement_forvar_print(na, printer)
	struct node *na; /* (struct IterationStatement_for_node) */
	struct printer *printer;
{
	struct IterationStatement_for_node *n = 
		CAST_NODE(na, IterationStatement_for);
		
	PRINT_STRING(STR(for));
	PRINT_CHAR(' ');
	PRINT_CHAR('(');
	PRINT_STRING(STR(var));
	PRINT_CHAR(' ');
	PRINT(n->init);
	PRINT_CHAR(';');
	PRINT_CHAR(' ');
	if (n->cond) PRINT(n->cond);
	PRINT_CHAR(';');
	PRINT_CHAR(' ');
	if (n->incr) PRINT(n->incr);
	PRINT_CHAR(')');
	PRINT_CHAR('{');
	PRINT_NEWLINE(1);
	PRINT(n->body);
	PRINT_CHAR('}');
	PRINT_NEWLINE(-1);
}

static void
IterationStatement_forin_print(na, printer)
	struct node *na; /* (struct IterationStatement_forin_node) */
	struct printer *printer;
{
	struct IterationStatement_forin_node *n = 
		CAST_NODE(na, IterationStatement_forin);

	PRINT_STRING(STR(for));
	PRINT_CHAR(' ');
	PRINT_CHAR('(');
	PRINT(n->lhs);
	PRINT_STRING(STR(in));
	PRINT_CHAR(' ');
	PRINT(n->list);
	PRINT_CHAR(')');
	PRINT_CHAR('{');
	PRINT_NEWLINE(1);
	PRINT(n->body);
	PRINT_CHAR('}');
	PRINT_NEWLINE(-1);
}

static void
IterationStatement_forvarin_print(na, printer)
	struct node *na; /* (struct IterationStatement_forin_node) */
	struct printer *printer;
{
	struct IterationStatement_forin_node *n = 
		CAST_NODE(na, IterationStatement_forin);

	PRINT_STRING(STR(for));
	PRINT_CHAR(' ');
	PRINT_CHAR('(');
	PRINT_STRING(STR(var));
	PRINT(n->lhs);
	PRINT_STRING(STR(in));
	PRINT_CHAR(' ');
	PRINT(n->list);
	PRINT_CHAR(')');
	PRINT_CHAR('{');
	PRINT_NEWLINE(1);
	PRINT(n->body);
	PRINT_CHAR('}');
	PRINT_NEWLINE(-1);
}

static void
ContinueStatement_print(na, printer)
	struct node *na; /* (struct ContinueStatement_node) */
	struct printer *printer;
{
	struct ContinueStatement_node *n = CAST_NODE(na, ContinueStatement);

	PRINT_STRING(STR(continue));
	PRINT_CHAR(' ');
	Label_print(n->target, printer);
	PRINT_CHAR(';');
	PRINT_NEWLINE(0);
}

static void
BreakStatement_print(na, printer)
	struct node *na; /* (struct BreakStatement_node) */
	struct printer *printer;
{
	struct BreakStatement_node *n = CAST_NODE(na, BreakStatement);

	PRINT_STRING(STR(break));
	PRINT_CHAR(' ');
	Label_print(n->target, printer);
	PRINT_CHAR(';');
	PRINT_NEWLINE(0);
}

static void
ReturnStatement_print(na, printer)
	struct node *na; /* (struct ReturnStatement_node) */
	struct printer *printer;
{
	struct ReturnStatement_node *n = CAST_NODE(na, ReturnStatement);
	PRINT_STRING(STR(return));
	PRINT_CHAR(' ');
	PRINT(n->expr);
	PRINT_CHAR(';');
	PRINT_NEWLINE(0);
}

static void
ReturnStatement_undef_print(na, printer)
	struct node *na; /* (struct ReturnStatement_node) */
	struct printer *printer;
{
	PRINT_STRING(STR(return));
	PRINT_CHAR(';');
	PRINT_NEWLINE(0);
}

static void
WithStatement_print(na, printer)
	struct node *na; /* (struct Binary_node) */
	struct printer *printer;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	PRINT_STRING(STR(with));
	PRINT_CHAR(' ');
	PRINT_CHAR('(');
	PRINT(n->a);
	PRINT_CHAR(')');
	PRINT_CHAR('{');
	PRINT_NEWLINE(1);
	PRINT(n->b);
	PRINT_CHAR('}');
	PRINT_NEWLINE(-1);
}

static void
SwitchStatement_print(na, printer)
	struct node *na; /* (struct SwitchStatement_node) */
	struct printer *printer;
{
	struct SwitchStatement_node *n = CAST_NODE(na, SwitchStatement);
	struct case_list *c;

	PRINT_STRING(STR(switch));
	PRINT_CHAR(' ');
	PRINT_CHAR('(');
	PRINT(n->cond);
	PRINT_CHAR(')');
	PRINT_CHAR(' ');
	PRINT_CHAR('{');
	PRINT_NEWLINE(1);

	for (c = n->cases; c; c = c->next) {
		if (c == n->defcase) {
			PRINT_STRING(STR(default));
			PRINT_CHAR(':');
			PRINT_NEWLINE(0);
		}
		if (c->expr) {
			PRINT_STRING(STR(case));
			PRINT_CHAR(' ');
			PRINT(c->expr);
			PRINT_CHAR(':');
			PRINT_NEWLINE(0);
		}
		if (c->body) {
			PRINT_NEWLINE(1);
			PRINT(c->body);
			PRINT_NEWLINE(-1);
		}
	}

	PRINT_CHAR('}');
	PRINT_NEWLINE(-1);
	PRINT_NEWLINE(0);
}

static void
Label_print(target, printer)
	unsigned int target;
	struct printer *printer;
{
	PRINT_CHAR('L');
	print_hex(printer, target);
}

static void
LabelledStatement_print(na, printer)
	struct node *na; /* (struct Unary_node) */
	struct printer *printer;
{
	struct LabelledStatement_node *n = CAST_NODE(na, LabelledStatement);

	Label_print(n->target, printer);
	PRINT_CHAR(':');
	PRINT(n->unary.a);
}

static void
ThrowStatement_print(na, printer)
	struct node *na; /* (struct Unary_node) */
	struct printer *printer;
{
	struct Unary_node *n = CAST_NODE(na, Unary);
	PRINT_STRING(STR(throw));
	PRINT_CHAR(' ');
	PRINT(n->a);
	PRINT_CHAR(';');
	PRINT_NEWLINE(0);
}

static void
TryStatement_catch_print(na, printer)
	struct node *na; /* (struct TryStatement_node) */
	struct printer *printer;
{
	struct TryStatement_node *n = CAST_NODE(na, TryStatement);
	PRINT_STRING(STR(try));
	PRINT_NEWLINE(1);
	PRINT(n->block);
	PRINT_NEWLINE(-1);
	PRINT_STRING(STR(catch));
	PRINT_CHAR(' ');
	PRINT_CHAR('(');
	PRINT_STRING(n->ident);
	PRINT_CHAR(')');
	PRINT_CHAR('{');
	PRINT_NEWLINE(1);
	PRINT(n->bcatch);
	PRINT_CHAR('}');
	PRINT_NEWLINE(-1);
}

static void
TryStatement_finally_print(na, printer)
	struct node *na; /* (struct TryStatement_node) */
	struct printer *printer;
{
	struct TryStatement_node *n = CAST_NODE(na, TryStatement);
	PRINT_STRING(STR(try));
	PRINT_CHAR('{');
	PRINT_NEWLINE(1);
	PRINT(n->block);
	PRINT_CHAR('}');
	PRINT_NEWLINE(-1);
	PRINT_STRING(STR(finally));
	PRINT_CHAR('{');
	PRINT_NEWLINE(1);
	PRINT(n->bfinally);
	PRINT_CHAR('}');
	PRINT_NEWLINE(-1);
}

static void
TryStatement_catchfinally_print(na, printer)
	struct node *na; /* (struct TryStatement_node) */
	struct printer *printer;
{
	struct TryStatement_node *n = CAST_NODE(na, TryStatement);
	PRINT_STRING(STR(try));
	PRINT_CHAR('{');
	PRINT_NEWLINE(1);
	PRINT(n->block);
	PRINT_CHAR('}');
	PRINT_NEWLINE(-1);
	PRINT_STRING(STR(catch));
	PRINT_CHAR(' ');
	PRINT_CHAR('(');
	PRINT_STRING(n->ident);
	PRINT_CHAR(')');
	PRINT_CHAR('{');
	PRINT_NEWLINE(1);
	PRINT(n->bcatch);
	PRINT_CHAR('}');
	PRINT_NEWLINE(-1);
	PRINT_STRING(STR(finally));
	PRINT_CHAR('{');
	PRINT_NEWLINE(1);
	PRINT(n->bfinally);
	PRINT_CHAR('}');
	PRINT_NEWLINE(-1);
}

static void
Function_print(na, printer)
	struct node *na; /* (struct Function_node) */
	struct printer *printer;
{
	struct Function_node *n = CAST_NODE(na, Function);
	int i;

	PRINT_STRING(STR(function));
	PRINT_CHAR(' ');
	if (n->function->name) {
	    PRINT_STRING(n->function->name);
	    PRINT_CHAR(' ');
	}
	PRINT_CHAR('(');
	for (i = 0; i < n->function->nparams; i++) {
	    if (i) {
		PRINT_CHAR(',');
		PRINT_CHAR(' ');
	    }
	    PRINT_STRING(n->function->params[i]);
	}
	PRINT_CHAR(')');
	PRINT_CHAR(' ');
	PRINT_CHAR('{');
	PRINT_NEWLINE(1);
	PRINT((struct node *)n->function->body);
	PRINT_NEWLINE(-1);
	PRINT_CHAR('}');
	PRINT_NEWLINE(0);
}

static void
SourceElements_print(na, printer)
	struct node *na; /* (struct SourceElements_node) */
	struct printer *printer;
{
	struct SourceElements_node *n = CAST_NODE(na, SourceElements);
	struct var *v;
	struct SourceElement *e;
        SEE_char_t c;

	if (n->vars) {
	    PRINT_CHAR('/');
	    PRINT_CHAR('*');
	    PRINT_CHAR(' ');
	    PRINT_STRING(STR(var));
	    c = ' ';
	    for (v = n->vars; v; v = v->next)  {
		PRINT_CHAR(c); c = ',';
		PRINT_STRING(v->name);
	    }
	    PRINT_CHAR(';');
	    PRINT_CHAR(' ');
	    PRINT_CHAR('*');
	    PRINT_CHAR('/');
	    PRINT_NEWLINE(0);
	}

	for (e = n->functions; e; e = e->next)
		PRINT(e->node);

	PRINT_NEWLINE(0);

	for (e = n->statements; e; e = e->next)
		PRINT(e->node);
}

/*------------------------------------------------------------
 * Printer common code
 */

static void
printer_init(printer, interp, printerclass)
	struct printer *printer;
	struct SEE_interpreter *interp;
	struct printerclass *printerclass;
{
	printer->printerclass = printerclass;
	printer->interpreter = interp;
	printer->indent = 0;
	printer->bol = 0;
}

/* Called when the printer is at the beginning of a line. */
static void
printer_atbol(printer)
	struct printer *printer;
{
	int i;

	printer->bol = 0;		/* prevent recursion */
	PRINT_CHAR('\n');
	for (i = 0; i < printer->indent; i++) {
		PRINT_CHAR(' ');
		PRINT_CHAR(' ');
	}
}

static void
printer_print_newline(printer, indent)
	struct printer *printer;
	int indent;
{
	printer->bol = 1;
	printer->indent += indent;
}

static void
printer_print_node(printer, n)
	struct printer *printer;
	struct node *n;
{
	(*PRINTFN(n))(n, printer);
}

static void
print_hex(printer, i)
	struct printer *printer;
	int i;
{
	if (i >= 16) print_hex(printer, i >> 4);
	PRINT_CHAR(SEE_hexstr_lowercase[i & 0xf]);
}

/*------------------------------------------------------------
 * Stdio printer - Prints each node in an AST to a stdio file.
 * So we can reconstruct parsed programs and print them to the screen.
 */

struct stdio_printer {
	struct printer printer;
	FILE *output;
};

static void
stdio_print_string(printer, s)
	struct printer *printer;
	struct SEE_string *s;
{
	struct stdio_printer *sp = (struct stdio_printer *)printer;

	if (printer->bol) printer_atbol(printer);
	SEE_string_fputs(s, sp->output);
}

static void
stdio_print_char(printer, c)
	struct printer *printer;
	int c;		/* SEE_char_t promoted to int */
{
	struct stdio_printer *sp = (struct stdio_printer *)printer;

	if (printer->bol) printer_atbol(printer);
	fputc(c & 0x7f, sp->output);
}

static void
stdio_print_node(printer, n)
	struct printer *printer;
	struct node *n;
{
	struct stdio_printer *sp = (struct stdio_printer *)printer;

/*	fprintf(sp->output, "(%d: ", n->location.lineno);  */
	(PRINTFN(n))(n, printer);
/*	fprintf(sp->output, ")");  */
	fflush(sp->output);
}

static struct printerclass stdio_printerclass = {
	stdio_print_string,
	stdio_print_char,
	printer_print_newline,
	stdio_print_node, /* printer_print_node */
};

static struct printer *
stdio_printer_new(interp, output)
	struct SEE_interpreter *interp;
	FILE *output;
{
	struct stdio_printer *sp;

	sp = SEE_NEW(interp, struct stdio_printer);
	printer_init(&sp->printer, interp, &stdio_printerclass);
	sp->output = output;
	return (struct printer *)sp;
}

/*------------------------------------------------------------
 * String printer
 * Used to reconstruct parsed programs and save them in a string.
 */

struct string_printer {
	struct printer printer;
	struct SEE_string *string;
};

static void
string_print_string(printer, s)
	struct printer *printer;
	struct SEE_string *s;
{
	struct string_printer *sp = (struct string_printer *)printer;

	if (printer->bol) printer_atbol(printer);
	SEE_string_append(sp->string, s);
}

static void
string_print_char(printer, c)
	struct printer *printer;
	int c;		/* SEE_char_t promoted to int */
{
	struct string_printer *sp = (struct string_printer *)printer;

	if (printer->bol) printer_atbol(printer);
	SEE_string_addch(sp->string, c);
}

static struct printerclass string_printerclass = {
	string_print_string,
	string_print_char,
	printer_print_newline,
	printer_print_node
};

static struct printer *
string_printer_new(interp, string)
	struct SEE_interpreter *interp;
	struct SEE_string *string;
{
	struct string_printer *sp;

	sp = SEE_NEW(interp, struct string_printer);
	printer_init(&sp->printer, interp, &string_printerclass);
	sp->string = string;
	return (struct printer *)sp;
}

void (*_SEE_nodeclass_print[NODECLASS_MAX])(struct node *, 
        struct printer *) = { 0
    ,Unary_print                            /*Unary*/
    ,Binary_print                           /*Binary*/
    ,Literal_print                          /*Literal*/
    ,StringLiteral_print                    /*StringLiteral*/
    ,RegularExpressionLiteral_print         /*RegularExpressionLiteral*/
    ,PrimaryExpression_this_print           /*PrimaryExpression_this*/
    ,PrimaryExpression_ident_print          /*PrimaryExpression_ident*/
    ,ArrayLiteral_print                     /*ArrayLiteral*/
    ,ObjectLiteral_print                    /*ObjectLiteral*/
    ,Arguments_print                        /*Arguments*/
    ,MemberExpression_new_print             /*MemberExpression_new*/
    ,MemberExpression_dot_print             /*MemberExpression_dot*/
    ,MemberExpression_bracket_print         /*MemberExpression_bracket*/
    ,CallExpression_print                   /*CallExpression*/
    ,PostfixExpression_inc_print            /*PostfixExpression_inc*/
    ,PostfixExpression_dec_print            /*PostfixExpression_dec*/
    ,UnaryExpression_delete_print           /*UnaryExpression_delete*/
    ,UnaryExpression_void_print             /*UnaryExpression_void*/
    ,UnaryExpression_typeof_print           /*UnaryExpression_typeof*/
    ,UnaryExpression_preinc_print           /*UnaryExpression_preinc*/
    ,UnaryExpression_predec_print           /*UnaryExpression_predec*/
    ,UnaryExpression_plus_print             /*UnaryExpression_plus*/
    ,UnaryExpression_minus_print            /*UnaryExpression_minus*/
    ,UnaryExpression_inv_print              /*UnaryExpression_inv*/
    ,UnaryExpression_not_print              /*UnaryExpression_not*/
    ,MultiplicativeExpression_mul_print     /*MultiplicativeExpression_mul*/
    ,MultiplicativeExpression_div_print     /*MultiplicativeExpression_div*/
    ,MultiplicativeExpression_mod_print     /*MultiplicativeExpression_mod*/
    ,AdditiveExpression_add_print           /*AdditiveExpression_add*/
    ,AdditiveExpression_sub_print           /*AdditiveExpression_sub*/
    ,ShiftExpression_lshift_print           /*ShiftExpression_lshift*/
    ,ShiftExpression_rshift_print           /*ShiftExpression_rshift*/
    ,ShiftExpression_urshift_print          /*ShiftExpression_urshift*/
    ,RelationalExpression_lt_print          /*RelationalExpression_lt*/
    ,RelationalExpression_gt_print          /*RelationalExpression_gt*/
    ,RelationalExpression_le_print          /*RelationalExpression_le*/
    ,RelationalExpression_ge_print          /*RelationalExpression_ge*/
    ,RelationalExpression_instanceof_print  /*RelationalExpression_instanceof*/
    ,RelationalExpression_in_print          /*RelationalExpression_in*/
    ,EqualityExpression_eq_print            /*EqualityExpression_eq*/
    ,EqualityExpression_ne_print            /*EqualityExpression_ne*/
    ,EqualityExpression_seq_print           /*EqualityExpression_seq*/
    ,EqualityExpression_sne_print           /*EqualityExpression_sne*/
    ,BitwiseANDExpression_print             /*BitwiseANDExpression*/
    ,BitwiseXORExpression_print             /*BitwiseXORExpression*/
    ,BitwiseORExpression_print              /*BitwiseORExpression*/
    ,LogicalANDExpression_print             /*LogicalANDExpression*/
    ,LogicalORExpression_print              /*LogicalORExpression*/
    ,ConditionalExpression_print            /*ConditionalExpression*/
    ,0                                      /*AssignmentExpression*/
    ,AssignmentExpression_simple_print      /*AssignmentExpression_simple*/
    ,AssignmentExpression_muleq_print       /*AssignmentExpression_muleq*/
    ,AssignmentExpression_diveq_print       /*AssignmentExpression_diveq*/
    ,AssignmentExpression_modeq_print       /*AssignmentExpression_modeq*/
    ,AssignmentExpression_addeq_print       /*AssignmentExpression_addeq*/
    ,AssignmentExpression_subeq_print       /*AssignmentExpression_subeq*/
    ,AssignmentExpression_lshifteq_print    /*AssignmentExpression_lshifteq*/
    ,AssignmentExpression_rshifteq_print    /*AssignmentExpression_rshifteq*/
    ,AssignmentExpression_urshifteq_print   /*AssignmentExpression_urshifteq*/
    ,AssignmentExpression_andeq_print       /*AssignmentExpression_andeq*/
    ,AssignmentExpression_xoreq_print       /*AssignmentExpression_xoreq*/
    ,AssignmentExpression_oreq_print        /*AssignmentExpression_oreq*/
    ,Expression_comma_print                 /*Expression_comma*/
    ,Block_empty_print                      /*Block_empty*/
    ,Binary_print                           /*StatementList*/
    ,VariableStatement_print                /*VariableStatement*/
    ,VariableDeclarationList_print          /*VariableDeclarationList*/
    ,VariableDeclaration_print              /*VariableDeclaration*/
    ,EmptyStatement_print                   /*EmptyStatement*/
    ,ExpressionStatement_print              /*ExpressionStatement*/
    ,IfStatement_print                      /*IfStatement*/
    ,IterationStatement_dowhile_print       /*IterationStatement_dowhile*/
    ,IterationStatement_while_print         /*IterationStatement_while*/
    ,IterationStatement_for_print           /*IterationStatement_for*/
    ,IterationStatement_forvar_print        /*IterationStatement_forvar*/
    ,IterationStatement_forin_print         /*IterationStatement_forin*/
    ,IterationStatement_forvarin_print      /*IterationStatement_forvarin*/
    ,ContinueStatement_print                /*ContinueStatement*/
    ,BreakStatement_print                   /*BreakStatement*/
    ,ReturnStatement_print                  /*ReturnStatement*/
    ,ReturnStatement_undef_print            /*ReturnStatement_undef*/
    ,WithStatement_print                    /*WithStatement*/
    ,SwitchStatement_print                  /*SwitchStatement*/
    ,LabelledStatement_print                /*LabelledStatement*/
    ,ThrowStatement_print                   /*ThrowStatement*/
    ,0                                      /*TryStatement*/
    ,TryStatement_catch_print               /*TryStatement_catch*/
    ,TryStatement_finally_print             /*TryStatement_finally*/
    ,TryStatement_catchfinally_print        /*TryStatement_catchfinally*/
    ,Function_print                         /*Function*/
    ,Function_print                         /*FunctionDeclaration*/
    ,Function_print                         /*FunctionExpression*/
    ,Unary_print                            /*FunctionBody*/
    ,SourceElements_print                   /*SourceElements*/
};
