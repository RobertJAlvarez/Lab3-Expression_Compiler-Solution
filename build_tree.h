#ifndef __BUILD_TREE_H__
#define __BUILD_TREE_H__

typedef enum { VAR, REG, CONST, UNARYOP, BINARYOP } nodetype_t;

typedef enum {
  UMINUS,
  ADD,
  SUB,
  MUL,
  DIV,
  AND,
  OR,
  XOR,
  NOT,
  SLL,
  SRL,
  LPAREN,
  RPAREN,
  ERROR_OP
} ops_t;

typedef struct node {
  nodetype_t type;
  int data;
  struct node *left, *right;
} node_t;

// Share functions in backend.c
void init_regtable(void);
void init_vartable(void);
int assign_reg(int var);
void printregtable(void);
void printvartable(void);

// Back-end function
node_t *generate_code(node_t *);

// Build-tree function
node_t *build_tree(const char exprin[]);

#endif
