/* (c) 2009 David Leonard.  All rights reserved. */

struct node;
struct SEE_interpreter;

int _SEE_isconst(struct node *n, struct SEE_interpreter *interp);

#define ISCONST(n, interp) 				\
        ((n)->flags & NODE_FLAG_ISCONST_VALID           \
          ? (n)->flags & NODE_FLAG_ISCONST              \
          : _SEE_isconst(n, interp))

