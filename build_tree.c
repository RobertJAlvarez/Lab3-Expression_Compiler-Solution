#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "build_tree.h"

static void __error_no_memory(void) {
  fprintf(stderr, "No more memory available. Error: %s\n", strerror(errno));
}

static node_t *__new_node(nodetype_t type, int data) {
  node_t *p;

  if ((p = malloc(sizeof(node_t))) == NULL) return NULL;

  p->type = type;
  p->data = data;
  p->left = NULL;
  p->right = NULL;

  return p;
}

const operator_t optable[NUMOPTYPES] = {
    {UNARYOP, 7, RL, "-", ""},      {BINARYOP, 5, LR, "+", "add"},
    {BINARYOP, 5, LR, "-", "sub"},  {BINARYOP, 6, LR, "*", "mul"},
    {BINARYOP, 6, LR, "/", "div"},  {BINARYOP, 3, LR, "&", "and"},
    {BINARYOP, 1, LR, "|", "or"},   {BINARYOP, 2, LR, "^", "xor"},
    {UNARYOP, 6, RL, "~", ""},      {BINARYOP, 4, LR, "<<", "sll"},
    {BINARYOP, 4, LR, ">>", "srl"}, {UNARYOP, 0, LR, "(", ""},
    {UNARYOP, 0, LR, ")", ""}};

static int __read_variable(const char expr[], const size_t i,
                           nodestack_t *nstack) {
  void *p;
  int var;

  // push node on the nodestack
  if (nstack->top < MAXNODES) {
    var = (int)(expr[i] - 'a');
    assign_reg(var);

    if ((p = __new_node(VAR, var)) == NULL) {
      __error_no_memory();
      return 1;
    }
    nstack->nodes[nstack->top] = (node_t *)p;
    nstack->top++;
  } else {
    printf("Error: too many nodes\n");
    return 1;
  }

  return 0;
}

static int __read_digit(const char expr[], size_t *j, nodestack_t *nstack) {
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
  if (nstack->top < MAXNODES) {
    num = atoi(numstr);
    if ((p = __new_node(CONST, num)) == NULL) {
      __error_no_memory();
      return 1;
    }
    nstack->nodes[nstack->top] = (node_t *)p;
    nstack->top++;
  } else {
    printf("Error: too many nodes\n");
    return 1;
  }

  return 0;
}

static ops_t __get_operation(const char expr[], size_t *j) {
  ops_t op;
  size_t i = *j;

  switch (expr[i]) {
    case ('-'):
      if ((i == 0) || (!isalpha(expr[i - 1]) && !isdigit(expr[i - 1])))
        op = UMINUS;
      else
        op = SUB;
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
    case (')'):
      op = RPAREN;
      break;
    default:
      printf("Error: invalid character in expression: %c\n", expr[i]);
      op = ERROR_OP;
  }

  return op;
}

static int __make_nodes(operatorstack_t *opstack, nodestack_t *nstack,
                        const ops_t op) {
  node_t *t, *t1, *t2;

  while (opstack->top > 0 && opstack->ops[opstack->top - 1] != LPAREN &&
         ((optable[op].assoc == LR &&
           optable[opstack->ops[opstack->top - 1]].prec >= optable[op].prec) ||
          (optable[op].assoc == RL &&
           optable[opstack->ops[opstack->top - 1]].prec > optable[op].prec))) {
    // Pop the operator stack and create node for popped op
    t = __new_node(optable[opstack->ops[opstack->top - 1]].type,
                   opstack->ops[opstack->top - 1]);

    if (t == NULL) {
      __error_no_memory();
      return 1;
    }

    opstack->top--;
    // Pop the node stack
    if (t->type == UNARYOP) {
      t2 = nstack->nodes[nstack->top - 1];
      nstack->top--;
      t1 = NULL;
    } else {
      t1 = nstack->nodes[nstack->top - 1];
      nstack->top--;
      t2 = nstack->nodes[nstack->top - 1];
      nstack->top--;
    }
    // Add child/children and push new node on node stack
    t->left = t2;
    t->right = t1;
    if (nstack->top < MAXNODES) {
      nstack->nodes[nstack->top] = t;
      nstack->top++;
    } else {
      printf("Error: too many nodes\n");
      return 1;
    }
  }  // end while
  // Push op onto opstack
  if (opstack->top < MAXOPS) {
    opstack->ops[opstack->top] = ((int)op);
    opstack->top++;
  } else {
    printf("Error: too many operators\n");
    return 1;
  }

  return 0;
}

static int __pop_right_parenthesis(operatorstack_t *opstack,
                                   nodestack_t *nstack) {
  node_t *t, *t1, *t2;

  while (opstack->top > 0 && opstack->ops[opstack->top - 1] != LPAREN) {
    // Pop the operator stack and create node for popped op
    t = __new_node(optable[opstack->ops[opstack->top - 1]].type,
                   opstack->ops[opstack->top - 1]);

    if (t == NULL) {
      __error_no_memory();
      return 1;
    }

    opstack->top--;
    if (t->data == 12) return 1;

    // Pop the node stack
    if (t->type == UNARYOP) {
      t2 = nstack->nodes[nstack->top - 1];
      nstack->top--;
      t1 = NULL;
    } else {
      t1 = nstack->nodes[nstack->top - 1];
      nstack->top--;
      t2 = nstack->nodes[nstack->top - 1];
      nstack->top--;
    }
    // Add child/children and push new node on node stack
    t->left = t2;
    t->right = t1;
    if (nstack->top < MAXNODES) {
      nstack->nodes[nstack->top] = t;
      nstack->top++;
    } else {
      printf("Error: too many nodes\n");
      return 1;
    }
  }

  return 0;
}

node_t *build_tree(char exprin[]) {
  operatorstack_t opstack;
  nodestack_t nstack;
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
    if (expr[i] == '(') {
      // push LPAREN on the opstack
      if (opstack.top < MAXOPS) {
        opstack.ops[opstack.top] = LPAREN;
        opstack.top++;
      } else {
        printf("Error: too many operators\n");
        return (NULL);
      }
    } else if (isalpha(expr[i])) {
      if (__read_variable(expr, i, &nstack)) return NULL;
    } else if (isdigit(expr[i])) {
      if (__read_digit(expr, &i, &nstack)) return NULL;
    } else {
      // operator
      if ((op = __get_operation(expr, &i)) == ERROR_OP) return NULL;

      if (optable[op].prec > 0) {
        if (__make_nodes(&opstack, &nstack, op)) return NULL;
      } else if (op == RPAREN) {
        if (__pop_right_parenthesis(&opstack, &nstack)) return NULL;
        if (opstack.top > 0) opstack.top--;
      }
    }
    i++;
  }

  return nstack.nodes[nstack.top - 1];
}
