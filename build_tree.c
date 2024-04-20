#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "build_tree.h"

struct {
  unsigned int top;
  int ops[MAXOPS];
} opstack;

struct {
  unsigned int top;
  node_t *nodes[MAXNODES];
} nstack;

const operator_t optable[NUMOPTYPES] = {
    {UNARYOP, 7, RL, "-", ""},      {BINARYOP, 5, LR, "+", "add"},
    {BINARYOP, 5, LR, "-", "sub"},  {BINARYOP, 6, LR, "*", "mul"},
    {BINARYOP, 6, LR, "/", "div"},  {BINARYOP, 3, LR, "&", "and"},
    {BINARYOP, 1, LR, "|", "or"},   {BINARYOP, 2, LR, "^", "xor"},
    {UNARYOP, 6, RL, "~", ""},      {BINARYOP, 4, LR, "<<", "sll"},
    {BINARYOP, 4, LR, ">>", "srl"}, {UNARYOP, 0, LR, "(", ""},
    {UNARYOP, 0, LR, ")", ""}};

static void __error_no_memory(void) {
  fprintf(stderr, "Error: No more memory available. Error: %s\n",
          strerror(errno));
}

static node_t *__new_node(const nodetype_t type, const int data) {
  node_t *p;

  if ((p = malloc(sizeof(node_t))) == NULL) return NULL;

  p->type = type;
  p->data = data;
  p->left = NULL;
  p->right = NULL;

  return p;
}

static int __read_variable(const char expr[], const size_t i) {
  void *p;
  int var;

  // push node on the nodestack
  if (nstack.top < MAXNODES) {
    var = (int)(expr[i] - 'a');
    assign_reg(var);

    if ((p = __new_node(VAR, var)) == NULL) {
      __error_no_memory();
      return 1;
    }
    nstack.nodes[nstack.top] = (node_t *)p;
    nstack.top++;
  } else {
    fprintf(stderr, "Error: too many nodes\n");
    return 1;
  }

  return 0;
}

static int __read_digit(const char expr[], size_t *j) {
  void *p;
  size_t i = *j;
  char numstr[MAXNUMLENGTH + 1];
  int numlength;
  int num;

  numstr[0] = expr[i];
  numlength = 1;
  while (isdigit(expr[i + 1]) && (numlength < MAXNUMLENGTH)) {
    i++;
    numstr[numlength] = expr[i];
    numlength++;
  }

  // Update index
  *j = i;

  // Make numstr null terminated to call atoi()
  numstr[numlength] = '\0';

  // push node on the nodestack
  if (nstack.top < MAXNODES) {
    num = atoi(numstr);
    if ((p = __new_node(CONST, num)) == NULL) {
      __error_no_memory();
      return 1;
    }
    nstack.nodes[nstack.top] = (node_t *)p;
    nstack.top++;
  } else {
    fprintf(stderr, "Error: too many nodes\n");
    return 1;
  }

  return 0;
}

static ops_t __get_operation(const char expr[], size_t *j) {
  ops_t op;
  size_t i = *j;

  switch (expr[i]) {
    case ('-'):
      if ((i == 0) || (expr[i - 1] == ')') || (isalpha(expr[i - 1])) ||
          (isdigit(expr[i - 1])))
        op = SUB;
      else
        op = UMINUS;
      break;
    case ('+'):
      op = ADD;
      break;
    case ('*'):
      op = MUL;
      break;
    case ('/'):
      op = DIV;
      break;
    case ('&'):
      op = AND;
      break;
    case ('|'):
      op = OR;
      break;
    case ('^'):
      op = XOR;
      break;
    case ('~'):
      op = NOT;
      break;
    case ('<'):
      if (expr[i + 1] == '<') {
        op = SLL;
        (*j)++;
        break;
      }
    case ('>'):
      if (expr[i + 1] == '>') {
        op = SRL;
        (*j)++;
        break;
      }
    case ('('):
      op = LPAREN;
      break;
    case (')'):
      op = RPAREN;
      break;
    default:
      fprintf(stderr, "Error: invalid character in expression: %c\n", expr[i]);
      op = ERROR_OP;
  }

  return op;
}

static int __pop_stack_add_children(node_t *t) {
  node_t *t1, *t2;

  // Pop the node stack
  opstack.top--;

  if (t->type == UNARYOP) {
    t2 = nstack.nodes[nstack.top - 1];
    nstack.top--;
    t1 = NULL;
  } else {
    t1 = nstack.nodes[nstack.top - 1];
    nstack.top--;
    t2 = nstack.nodes[nstack.top - 1];
    nstack.top--;
  }

  // Add child/children and push new node on node stack
  t->left = t2;
  t->right = t1;
  if (nstack.top < MAXNODES) {
    nstack.nodes[nstack.top] = t;
    nstack.top++;
  } else {
    fprintf(stderr, "Error: too many nodes\n");
    return 1;
  }

  return 0;
}

static int __make_nodes(const ops_t op) {
  node_t *t;

  while (opstack.top > 0 &&  // More things to pop
         opstack.ops[opstack.top - 1] !=
             LPAREN &&  // We are not in open parenthesis
         ((optable[op].assoc == LR &&
           optable[opstack.ops[opstack.top - 1]].prec >= optable[op].prec) ||
          (optable[op].assoc == RL &&
           optable[opstack.ops[opstack.top - 1]].prec > optable[op].prec))) {
    // Pop the operator stack and create node for popped op
    t = __new_node(optable[opstack.ops[opstack.top - 1]].type,
                   opstack.ops[opstack.top - 1]);

    if (t == NULL) {
      __error_no_memory();
      return 1;
    }

    if (__pop_stack_add_children(t)) return 1;
  }

  // Push op onto opstack
  if (opstack.top < MAXOPS) {
    opstack.ops[opstack.top] = ((int)op);
    opstack.top++;
  } else {
    fprintf(stderr, "Error: too many operators\n");
    return 1;
  }

  return 0;
}

static int __push_left_parenthesis(void) {
  // push LPAREN on the opstack
  if (opstack.top < MAXOPS) {
    opstack.ops[opstack.top] = LPAREN;
    opstack.top++;
  } else {
    fprintf(stderr, "Error: too many operators\n");
    return 1;
  }
  return 0;
}

/* pop operator and create nodes until left parenthesis. */
static int __pop_right_parenthesis(void) {
  node_t *t;

  while (opstack.top > 0 && opstack.ops[opstack.top - 1] != LPAREN) {
    // Pop the operator stack and create node for popped op
    t = __new_node(optable[opstack.ops[opstack.top - 1]].type,
                   opstack.ops[opstack.top - 1]);

    if (t == NULL) {
      __error_no_memory();
      return 1;
    }

    if (t->data == 12) return 1;

    if (__pop_stack_add_children(t)) return 1;
  }

  if (opstack.top > 0) opstack.top--;

  return 0;
}

node_t *build_tree(const char exprin[]) {
  ops_t op;

  size_t length, i;
  char expr[MAXEXPRLENGTH + 2];

  init_vartable();
  init_regtable();
  expr[0] = '(';

  for (i = 0; i < strlen(exprin); i++) expr[i + 1] = exprin[i];
  expr[i + 1] = ')';
  expr[i + 2] = '\0';

  opstack.top = 0;
  nstack.top = 0;

  length = strlen(expr);
  i = 0;

  while (i < length) {
    if (isalpha(expr[i])) {
      // Read variable
      if (__read_variable(expr, i)) return NULL;
    } else if (isdigit(expr[i])) {
      // Read digit
      if (__read_digit(expr, &i)) return NULL;
    } else {
      // Operator
      if ((op = __get_operation(expr, &i)) == ERROR_OP) return NULL;

      // If we find an operator (not a parenthesis)
      if (optable[op].prec > 0) {
        if (__make_nodes(op)) return NULL;
      } else if (op == LPAREN) {
        if (__push_left_parenthesis()) return NULL;
      } else if (op == RPAREN) {
        if (__pop_right_parenthesis()) return NULL;
      }
    }
    i++;
  }

  return nstack.nodes[nstack.top - 1];
}
