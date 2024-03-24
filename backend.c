#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "build_tree.h"

int regtable[NUMREG];  // reg[i] contains current number of uses of register i
int vartable[NUMVAR];  // var[i] contains register to which var is assigned

void init_regtable(void) {
  for (int i = 0; i < NUMREG; i++) regtable[i] = 0;
}

void init_vartable(void) {
  for (int i = 0; i < NUMVAR; i++) vartable[i] = -1;
}

static int reuse_reg(int reg) {
  if (regtable[reg] == 1) return 1;
  if (regtable[reg] > 1) return 0;

  printf("Error: called reuse_reg on unused register\n");

  // shouldn't happen
  return -1;
}

int assign_reg(int var) {
  if ((var != -1) && (vartable[var] != -1)) {
    regtable[vartable[var]]++;  // variable is already assigned a register
    return (vartable[var]);
  }

  // Find unassigned register
  for (int i = 5; i < NUMREG; i++) {
    if (regtable[i] == 0) {
      regtable[i]++;
      if (var != -1) {
        vartable[var] = i;
      }
      return i;
    }
  }

  // out of registers
  return -1;
}

static int release_reg(int reg) {
  if (regtable[reg] > 0) {
    regtable[reg]--;
    return 0;
  }

  return -1;
}

void printregtable(void) {
  printf("register table -- number of uses of each register\n");
  for (int i = 0; i < NUMREG; i++)
    if (regtable[i]) printf("register: x%d, uses: %d\n", i, regtable[i]);
}

void printvartable(void) {
  printf("variable table -- register to which var is assigned\n");
  for (int i = 0; i < NUMVAR; i++)
    if (vartable[i] != -1)
      printf("variable: %c, register: x%d\n", 'a' + i, vartable[i]);
}

static int isPowerTwo(unsigned int n) {
  return (n & (n - 1)) == 0 ? log2(n) : 0;
}

node_t *generate_code(node_t *root) {
  node_t *left, *right;
  char instr[20];
  int destreg;

  if (root) {
    if (root->left) left = generate_code(root->left);
    if (root->right) right = generate_code(root->right);

    // if (root->type == REG) return (root);

    if (root->type == VAR) {
      root->type = REG;
      root->data = vartable[root->data];
    } else if (root->type == BINARYOP) {
      if ((left->type == REG) && (right->type == REG)) {
        if (reuse_reg(left->data) == 1) {
          destreg = left->data;
          release_reg(right->data);
        } else if (reuse_reg(right->data) == 1) {
          destreg = right->data;
          release_reg(left->data);
        } else {
          destreg = assign_reg(-1);
          if (destreg == -1) {
            printf("Error: out of registers\n");
            exit(-1);
          }
          release_reg(left->data);
          release_reg(right->data);
        }
        sprintf(instr, "%s  x%d, x%d, x%d", optable[root->data].instr, destreg,
                left->data, right->data);
        printf("%s\n", instr);
        free(left);
        free(right);
        root->type = REG;
        root->data = destreg;
      } else if ((left->type == REG) && (right->type == CONST)) {
        destreg = left->data;
        if (right->data < 2048) {
          if ((*optable[root->data].symbol == '*') &&
              (isPowerTwo(right->data) != 0)) {
            sprintf(instr, "slli  x%d, x%d, %d", destreg, left->data,
                    isPowerTwo(right->data));
            printf("%s\n", instr);
            free(left);
            free(right);
            root->type = REG;
            root->data = destreg;
          } else if ((*optable[root->data].symbol == '/') &&
                     (isPowerTwo(right->data) != 0)) {
            sprintf(instr, "srai  x%d, x%d, %d", destreg, left->data,
                    isPowerTwo(right->data));
            printf("%s\n", instr);
            free(left);
            free(right);
            root->type = REG;
            root->data = destreg;
          } else if ((*optable[root->data].symbol == '<') ||
                     (*optable[root->data].symbol == '>')) {
            sprintf(instr, "%si  x%d, x%d, %d", optable[root->data].instr,
                    destreg, left->data, right->data);
            printf("%s\n", instr);
            free(left);
            free(right);
            root->type = REG;
            root->data = destreg;
          } else {
            if ((*optable[root->data].symbol == '+') ||
                (*optable[root->data].symbol == '-')) {
              char sign = (*optable[root->data].symbol == '-') ? ('-') : ' ';
              sprintf(instr, "addi  x%d, x%d, %c%d", destreg, left->data, sign,
                      right->data);
              printf("%s\n", instr);
              root->type = REG;
              root->data = destreg;
            } else {
              destreg = assign_reg(-1);
              sprintf(instr, "addi  x%d, x%d, %d", destreg, left->data,
                      right->data);
              printf("%s\n", instr);
              right->type = REG;
              right->data = destreg;
              generate_code(root);
            }
          }
        } else {
          sprintf(instr, "lui  x%d, %d", destreg, right->data - 2047);
          printf("%s\n", instr);
          right->data = 2047;
          generate_code(root);
        }
      } else if ((left->type == CONST) && (right->type == REG)) {
        destreg = right->data;
        if (left->data < 2048) {
          if ((*optable[root->data].symbol == '*') &&
              (isPowerTwo(left->data) != 0)) {
            sprintf(instr, "slli  x%d, x%d, %d", destreg, right->data,
                    isPowerTwo(left->data));
            printf("%s\n", instr);
            free(left);
            free(right);
            root->type = REG;
            root->data = destreg;
          } else {
            destreg = assign_reg(-1);
            sprintf(instr, "addi  x%d, x0, %d", destreg, left->data);
            printf("%s\n", instr);
            left->type = REG;
            left->data = destreg;
            generate_code(root);
          }
        } else {
          sprintf(instr, "lui  x%d, %d", destreg, left->data - 2047);
          printf("%s\n", instr);
          left->data = 2047;
          generate_code(root);
        }
      } else if ((left->type == CONST) && (right->type == CONST)) {
        signed int a = left->data;
        signed int b = right->data;
        char c = *optable[root->data].symbol;
        signed int dest =
            (c == '+')
                ? (a + b)
                : ((c == '-')
                       ? (a - b)
                       : ((c == '*') ? (a * b) : ((c == '/') ? (a / b) : 0)));
        free(left);
        free(right);
        root->type = CONST;
        root->data = dest;
      }
    } else if (root->type == UNARYOP) {
      if (root->data == UMINUS) {
        if (left->type == REG) {
          if (reuse_reg(left->data)) {
            destreg = left->data;
          } else {
            destreg = assign_reg(-1);
            if (destreg == -1) {
              printf("Error: out of registers\n");
              exit(-1);
            }
            release_reg(left->data);
          }
          sprintf(instr, "sub  x%d, x0, x%d", destreg, left->data);
          printf("%s\n", instr);
          free(left);
          root->type = REG;
          root->data = destreg;
        }
      } else if (root->data == NOT) {
        if (left->type == REG) {
          if (reuse_reg(left->data)) {
            destreg = left->data;
          } else {
            destreg = assign_reg(-1);
            if (destreg == -1) {
              printf("Error: out of registers\n");
              exit(-1);
            }
            release_reg(left->data);
          }
          sprintf(instr, "xori  x%d, x%d, -1", destreg, left->data);
          printf("%s\n", instr);
          free(left);
          root->type = REG;
          root->data = destreg;
        } else if (left->type == CONST) {
          destreg = assign_reg(-1);
          sprintf(instr, "addi  x%d, x%d, -1", destreg, left->data);
          printf("%s\n", instr);
          free(left);
          root->type = REG;
          root->data = destreg;
        }
      }
    }
  }

  return root;
}
