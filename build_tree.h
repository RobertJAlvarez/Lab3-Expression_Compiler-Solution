#ifndef __BUILD_TREE_H__
#define __BUILD_TREE_H__

#define MAXOPS 100
#define MAXNODES 100
#define MAXNUMLENGTH 12
#define MAXEXPRLENGTH 100
#define LR 0
#define RL 1
#define NUMOPTYPES 13
#define NUMREG 32
#define NUMVAR 10

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
  RPAREN
} ops_t;

typedef struct {
  nodetype_t type;
  int prec;
  int assoc;
  char symbol[3];
  char instr[4];
} operator_t;

extern operator_t optable[];

typedef struct node {
  nodetype_t type;
  int data;
  struct node *left, *right;
} node_t;

// Back-end functions
void init_regtable(void);
void init_vartable(void);
int assign_reg(int var);
void printregtable(void);
void printvartable(void);
node_t *generate_code(node_t *);

// Build tree function
void postorder(node_t *root);

// Main function
node_t *build_tree(char exprin[]);

typedef struct {
  unsigned int top;
  int ops[MAXOPS];
} operatorstack_t;

typedef struct {
  unsigned int top;
  node_t *nodes[MAXNODES];
} nodestack_t;

#endif
