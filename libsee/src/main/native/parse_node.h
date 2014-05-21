/* (c) 2009 David Leonard. All rights reserved. */

/* Requires: <see/try.h> <see/value.h> */

struct SEE_string;
struct var;                     /* function.h */
struct node;

/* Classes of nodes in an AST resulting from a parse */
enum nodeclass_enum {
        NODECLASS_None                               =  0,
	NODECLASS_Unary                              =  1,
	NODECLASS_Binary                             =  2,
	NODECLASS_Literal                            =  3,
	NODECLASS_StringLiteral                      =  4,
	NODECLASS_RegularExpressionLiteral           =  5,
	NODECLASS_PrimaryExpression_this             =  6,
	NODECLASS_PrimaryExpression_ident            =  7,
	NODECLASS_ArrayLiteral                       =  8,
	NODECLASS_ObjectLiteral                      =  9,
	NODECLASS_Arguments                          = 10,
	NODECLASS_MemberExpression_new               = 11,
	NODECLASS_MemberExpression_dot               = 12,
	NODECLASS_MemberExpression_bracket           = 13,
	NODECLASS_CallExpression                     = 14,
	NODECLASS_PostfixExpression_inc              = 15,
	NODECLASS_PostfixExpression_dec              = 16,
	NODECLASS_UnaryExpression_delete             = 17,
	NODECLASS_UnaryExpression_void               = 18,
	NODECLASS_UnaryExpression_typeof             = 19,
	NODECLASS_UnaryExpression_preinc             = 20,
	NODECLASS_UnaryExpression_predec             = 21,
	NODECLASS_UnaryExpression_plus               = 22,
	NODECLASS_UnaryExpression_minus              = 23,
	NODECLASS_UnaryExpression_inv                = 24,
	NODECLASS_UnaryExpression_not                = 25,
	NODECLASS_MultiplicativeExpression_mul       = 26,
	NODECLASS_MultiplicativeExpression_div       = 27,
	NODECLASS_MultiplicativeExpression_mod       = 28,
	NODECLASS_AdditiveExpression_add             = 29,
	NODECLASS_AdditiveExpression_sub             = 30,
	NODECLASS_ShiftExpression_lshift             = 31,
	NODECLASS_ShiftExpression_rshift             = 32,
	NODECLASS_ShiftExpression_urshift            = 33,
	NODECLASS_RelationalExpression_lt            = 34,
	NODECLASS_RelationalExpression_gt            = 35,
	NODECLASS_RelationalExpression_le            = 36,
	NODECLASS_RelationalExpression_ge            = 37,
	NODECLASS_RelationalExpression_instanceof    = 38,
	NODECLASS_RelationalExpression_in            = 39,
	NODECLASS_EqualityExpression_eq              = 40,
	NODECLASS_EqualityExpression_ne              = 41,
	NODECLASS_EqualityExpression_seq             = 42,
	NODECLASS_EqualityExpression_sne             = 43,
	NODECLASS_BitwiseANDExpression               = 44,
	NODECLASS_BitwiseXORExpression               = 45,
	NODECLASS_BitwiseORExpression                = 46,
	NODECLASS_LogicalANDExpression               = 47,
	NODECLASS_LogicalORExpression                = 48,
	NODECLASS_ConditionalExpression              = 49,
	NODECLASS_AssignmentExpression               = 50,
	NODECLASS_AssignmentExpression_simple        = 51,
	NODECLASS_AssignmentExpression_muleq         = 52,
	NODECLASS_AssignmentExpression_diveq         = 53,
	NODECLASS_AssignmentExpression_modeq         = 54,
	NODECLASS_AssignmentExpression_addeq         = 55,
	NODECLASS_AssignmentExpression_subeq         = 56,
	NODECLASS_AssignmentExpression_lshifteq      = 57,
	NODECLASS_AssignmentExpression_rshifteq      = 58,
	NODECLASS_AssignmentExpression_urshifteq     = 59,
	NODECLASS_AssignmentExpression_andeq         = 60,
	NODECLASS_AssignmentExpression_xoreq         = 61,
	NODECLASS_AssignmentExpression_oreq          = 62,
	NODECLASS_Expression_comma                   = 63,
	NODECLASS_Block_empty                        = 64,
	NODECLASS_StatementList                      = 65,
	NODECLASS_VariableStatement                  = 66,
	NODECLASS_VariableDeclarationList            = 67,
	NODECLASS_VariableDeclaration                = 68,
	NODECLASS_EmptyStatement                     = 69,
	NODECLASS_ExpressionStatement                = 70,
	NODECLASS_IfStatement                        = 71,
	NODECLASS_IterationStatement_dowhile         = 72,
	NODECLASS_IterationStatement_while           = 73,
	NODECLASS_IterationStatement_for             = 74,
	NODECLASS_IterationStatement_forvar          = 75,
	NODECLASS_IterationStatement_forin           = 76,
	NODECLASS_IterationStatement_forvarin        = 77,
	NODECLASS_ContinueStatement                  = 78,
	NODECLASS_BreakStatement                     = 79,
	NODECLASS_ReturnStatement                    = 80,
	NODECLASS_ReturnStatement_undef              = 81,
	NODECLASS_WithStatement                      = 82,
	NODECLASS_SwitchStatement                    = 83,
	NODECLASS_LabelledStatement                  = 84,
	NODECLASS_ThrowStatement                     = 85,
	NODECLASS_TryStatement                       = 86,
	NODECLASS_TryStatement_catch                 = 87,
	NODECLASS_TryStatement_finally               = 88,
	NODECLASS_TryStatement_catchfinally          = 89,
	NODECLASS_Function                           = 90,
	NODECLASS_FunctionDeclaration                = 91,
	NODECLASS_FunctionExpression                 = 92,
	NODECLASS_FunctionBody                       = 93,
	NODECLASS_SourceElements                     = 94
#define NODECLASS_MAX                                  95
};

struct node {
        enum nodeclass_enum nodeclass;
        struct SEE_throw_location location;     /* source location */
        int flags;
#define NODE_FLAG_ISCONST               0x0000001
#define NODE_FLAG_ISCONST_VALID         0x0000002

        /* XXX For codegen */
        unsigned int maxstack;
        unsigned int is;
};
 
struct Literal_node {
	struct node node;
	struct SEE_value value;
};

struct StringLiteral_node {
	struct node node;
	struct SEE_string *string;
};

struct RegularExpressionLiteral_node {
	struct node node;
	struct SEE_value pattern;
	struct SEE_value flags;
	struct SEE_value *argv[2];
};

struct PrimaryExpression_ident_node {
	struct node node;
	struct SEE_string *string;
};

struct ArrayLiteral_node {
	struct node node;
	int length;
	struct ArrayLiteral_element {
		int index;
		struct node *expr;
		struct ArrayLiteral_element *next;
	} *first;
};

struct ObjectLiteral_node {
	struct node node;
	struct ObjectLiteral_pair {
		struct node *value;
		struct ObjectLiteral_pair *next;
		struct SEE_string *name;
	} *first;
};

struct Arguments_node {				/* declare for early use */
	struct node node;
	int	argc;
	struct Arguments_arg {
		struct node *expr;
		struct Arguments_arg *next;
	} *first;
};

struct MemberExpression_new_node {
	struct node node;
	struct node *mexp;
	struct Arguments_node *args;
};

struct MemberExpression_dot_node {
	struct node node;
	struct node *mexp;
	struct SEE_string *name;
};

struct MemberExpression_bracket_node {
	struct node node;
	struct node *mexp, *name;
};

struct CallExpression_node {
	struct node node;
	struct node *exp;
	struct Arguments_node *args;
};

struct Unary_node {
	struct node node;
	struct node *a;
};

struct Binary_node {
	struct node node;
	struct node *a, *b;
};

struct ConditionalExpression_node {
	struct node node;
	struct node *a, *b, *c;
};

struct AssignmentExpression_node {
	struct node node;
	struct node *lhs, *expr;
};

struct VariableDeclaration_node {
	struct node node;
	struct var *var;
	struct node *init;
};

struct IfStatement_node {
	struct node node;
	struct node *cond, *btrue, *bfalse;
};

#define NO_TARGET       0

struct IterationStatement_while_node {
	struct node  node;
	unsigned int target;
	struct node *cond, *body;
};

struct IterationStatement_for_node {
	struct node node;
	unsigned int target;
	struct node *init, *cond, *incr, *body;
};

struct IterationStatement_forin_node {
	struct node node;
	unsigned int target;
	struct node *lhs, *list, *body;
};

struct ContinueStatement_node {
	struct node node;
	unsigned int target;
};

struct BreakStatement_node {
	struct node node;
	unsigned int target;
};

struct ReturnStatement_node {
	struct node node;
	struct node *expr;
};

struct SwitchStatement_node {
	struct node node;
	unsigned int target;
	struct node *cond;
	struct case_list {
		struct node *expr;	/* NULL for default case */
		struct node *body;
		struct case_list *next;
	} *cases, *defcase;
};

struct LabelledStatement_node {
	struct Unary_node unary;
	unsigned int target;
};

struct TryStatement_node {
	struct node node;
	struct node *block, *bcatch, *bfinally;
	struct SEE_string *ident;
};

struct Function_node {
	struct node node;
	struct function *function;
};

struct FunctionBody_node {
	struct Unary_node u;
	int is_program;
};

struct SourceElements_node {
	struct node node;
	struct SourceElement {
		struct node *node;
		struct SourceElement *next;
	} *statements, *functions;
	struct var *vars;
};

/*
 * Convenience function for a checked cast of a node to a given type.
 * Checking is disabled with NDEBUG.
 */
#ifdef NDEBUG
#define CAST_NODE(na, cls)      ((struct cls##_node *)(na))
#else
#define CAST_NODE(na, cls)     ((struct cls##_node *)_SEE_cast_node(na, \
                                NODECLASS_##cls, #cls, __FILE__, __LINE__))
#endif
struct node *_SEE_cast_node(struct node *, enum nodeclass_enum,
                            const char *, const char *, int);

/** Evaluates a constant node using minimal context. */
void _SEE_const_evaluate(struct node *, struct SEE_interpreter *, 
        struct SEE_value *res);
int _SEE_node_functionbody_isempty(struct SEE_interpreter *interp,
        struct node *node);

