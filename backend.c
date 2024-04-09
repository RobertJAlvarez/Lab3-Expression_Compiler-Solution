#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "build_tree.h"

#define SET_NODE(NODE, TYPE, DATA) \
  NODE->type = TYPE;               \
  NODE->data = DATA;

// reg[i] contains current number of uses of register i
int regtable[NUMREG];
// var[i] contains register to which var is assigned
int vartable[NUMVAR];

void init_regtable(void) {
  for (int i = 0; i < NUMREG; i++) regtable[i] = 0;
}

void init_vartable(void) {
  for (int i = 0; i < NUMVAR; i++) vartable[i] = -1;
}

static int __reuse_reg(int reg) {
  if (regtable[reg] == 1) return 1;
  if (regtable[reg] > 1) return 0;

  printf("Error: called __reuse_reg on unused register\n");

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

static int __release_reg(int reg) {
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

static int __isPowerTwo(int n) {
  return (n & (n - 1)) == 0 ? ((int)log2(n)) : 0;
}

static void __done_w_instr(int destreg, node_t *root, node_t *node1,
                           node_t *node2, int is_imm) {
  if (is_imm) {
    printf("%si x%d, x%d, %d\n", optable[root->data].instr, destreg,
           node1->data, node2->data);
  } else {
    printf("%s x%d, x%d, x%d\n", optable[root->data].instr, destreg,
           node1->data, node2->data);
  }

  free(node1);
  free(node2);
  SET_NODE(root, REG, destreg);
}

static void __reg_reg(node_t *root, node_t *left, node_t *right) {
  int destreg;

  if (__reuse_reg(left->data) == 1) {
    destreg = left->data;
    __release_reg(right->data);
  } else if (__reuse_reg(right->data) == 1) {
    destreg = right->data;
    __release_reg(left->data);
  } else {
    if ((destreg = assign_reg(-1)) == -1) {
      printf("Error: out of registers\n");
      exit(-1);
    }
    __release_reg(left->data);
    __release_reg(right->data);
  }

  __done_w_instr(destreg, root, left, right, /*is_imm=*/0);
}

static void __lui_data(node_t *node) {
  int destreg;
  int high = node->data >> 12;
  int low = node->data & 0xFFF;

  if ((destreg = assign_reg(-1)) == -1) {
    printf("Error: out of registers\n");
    exit(-1);
  }

  /* Positive case *
   * 1000 = 4096  -> li 4096
   * 17FF = 6143  -> li 4096, addi 2047
   * 1800 = 6144  -> li 8192, addi -2048
   * 1801 = 6145  -> li 8192, addi -2047
   * 2000 = 8192  -> li 8192

   * Negative case *
   * F000 = -4096 -> li -4096
   * E801 = -6143 -> li -4096, addi -2047
   * E800 = -6144 -> li -4096, addi -2048
   * E7FF         -> li -8192, addi 2047
   * E000 = -8192 -> li -8192
   */

  if (low >= 0x800) {
    printf("lui x%d, %d\n", destreg, high + 1);
    low -= 0x1000;
  } else {
    printf("lui x%d, %d\n", destreg, high);
  }
  printf("addi x%d, x%d, %d\n", destreg, destreg, low);

  /*
  if (node->data > 0) {
    if (low >= 0x800) {
      printf("lui x%d, %d\n", destreg, high + 1);
      if (low != 0) {
        printf("addi x%d, x%d, %d\n", destreg, destreg, low - 0x1000);
      }
    } else {
      printf("lui x%d, %d\n", destreg, high);
      if (low != 0) {
        printf("addi x%d, x%d, %d\n", destreg, destreg, low);
      }
    }
  } else {
    if (low >= 0x800) {
      printf("lui x%d, %d\n", destreg, high + 1);
      if (low != 0) {
        printf("addi x%d, x%d, %d\n", destreg, destreg, low - 0x1000);
      }
    } else {
      printf("lui x%d, %d\n", destreg, high);
      if (low != 0) {
        printf("addi x%d, x%d, %d\n", destreg, destreg, low);
      }
    }
  }
  */

  SET_NODE(node, REG, destreg);
}

static inline int __is_slli(node_t *root, node_t *node) {
  return (root->data == MUL) && (__isPowerTwo(node->data) != 0);
}

static inline int __is_srai(node_t *root, node_t *node) {
  return (root->data == DIV) && (__isPowerTwo(node->data) != 0);
}

static void __slli(int destreg, node_t *root, node_t *node1, node_t *node2) {
  printf("slli x%d, x%d, %d\n", destreg, node1->data,
         __isPowerTwo(node2->data));
  free(node1);
  free(node2);
  SET_NODE(root, REG, destreg);
}

static void __srai(int destreg, node_t *root, node_t *node1, node_t *node2) {
  printf("srai x%d, x%d, %d\n", destreg, node1->data,
         __isPowerTwo(node2->data));
  free(node1);
  free(node2);
  SET_NODE(root, REG, destreg);
}

static void __pre_gen_code(char instr[], int destreg, node_t *root,
                           node_t *node) {
  printf("%s\n", instr);
  SET_NODE(node, REG, destreg);
  generate_code(root);
}

static void __reg_const(node_t *root, node_t *left, node_t *right) {
  char instr[20];
  int destreg;

  if ((right->data > 0xFFF) || (right->data < -2048)) {
    __lui_data(right);
    generate_code(root);
    return;
  }

  // muli and divi are not valid instructions. Move constant value to a register
  if ((!__isPowerTwo(right->data)) &&
      ((root->data == MUL) || (root->data == DIV))) {
    // Get new register to store constant value
    if ((destreg = assign_reg(-1)) == -1) {
      printf("Error: out of registers\n");
      exit(-1);
    }
    sprintf(instr, "addi x%d, x0, %d", destreg, right->data);
    __pre_gen_code(instr, destreg, root, right);
    return;
  }

  // Get register to assign operation
  if (__reuse_reg(left->data) == 1) {
    destreg = left->data;
  } else {
    if ((destreg = assign_reg(-1)) == -1) {
      printf("Error: out of registers\n");
      exit(-1);
    }
  }

  if (__is_slli(root, right)) {
    __slli(destreg, root, left, right);
  } else if (__is_srai(root, right)) {
    __srai(destreg, root, left, right);
  } else if (root->data == SUB) {
    printf("addi x%d, x%d, -%d\n", destreg, left->data, right->data);
    SET_NODE(root, REG, destreg);
    free(left);
    free(right);
  } else {
    __done_w_instr(destreg, root, left, right, /*is_imm=*/1);
    SET_NODE(root, REG, destreg);
  }
}

static void __const_reg(node_t *root, node_t *left, node_t *right) {
  char instr[20];
  int destreg;

  if ((right->data > 0xFFF) || (right->data < -2048)) {
    __lui_data(left);
    generate_code(root);
    return;
  }

  if (__is_slli(root, left)) {
    // Get destination register
    if (__reuse_reg(right->data) == 1) {
      destreg = right->data;
    } else {
      if ((destreg = assign_reg(-1)) == -1) {
        printf("Error: out of registers\n");
        exit(-1);
      }
    }
    __slli(destreg, root, right, left);
  } else {
    if ((destreg = assign_reg(-1)) == -1) {
      printf("Error: out of registers\n");
      exit(-1);
    }
    sprintf(instr, "addi x%d, x0, %d", destreg, left->data);
    __pre_gen_code(instr, destreg, root, left);
  }
}

static void __const_const(node_t *root, node_t *left, node_t *right) {
  int a = left->data;
  int b = right->data;
  int data;

  free(left);
  free(right);

  switch (root->data) {
    case ADD:
      data = a + b;
      break;
    case SUB:
      data = a - b;
      break;
    case MUL:
      data = a * b;
      break;
    case DIV:
      data = a / b;
      break;
    case AND:
      data = a & b;
      break;
    case OR:
      data = a | b;
      break;
    case XOR:
      data = a ^ b;
      break;
    case SLL:
      data = a << b;
      break;
    case SRL:
      data = a >> b;
      break;
    default:
      data = 0;
  }

  SET_NODE(root, CONST, data);
}

static void __unary_op(node_t *root, node_t *left) {
  int destreg;

  if (root->data == UMINUS) {
    if (left->type == REG) {
      if (__reuse_reg(left->data)) {
        destreg = left->data;
      } else {
        if ((destreg = assign_reg(-1)) == -1) {
          printf("Error: out of registers\n");
          exit(-1);
        }
        __release_reg(left->data);
      }
      printf("sub x%d, x0, x%d\n", destreg, left->data);
      free(left);
      SET_NODE(root, REG, destreg);
    }
  } else if (root->data == NOT) {
    if (left->type == REG) {
      if (__reuse_reg(left->data)) {
        destreg = left->data;
      } else {
        if ((destreg = assign_reg(-1)) == -1) {
          printf("Error: out of registers\n");
          exit(-1);
        }
        __release_reg(left->data);
      }
      printf("xori x%d, x%d, -1\n", destreg, left->data);
      free(left);
      SET_NODE(root, REG, destreg);
    } else if (left->type == CONST) {
      SET_NODE(root, CONST, ~left->data);
      root->left = NULL;
      root->right = NULL;
      free(left);
    }
  }
}

node_t *generate_code(node_t *root) {
  node_t *left, *right;

  if (root) {
    if (root->left) left = generate_code(root->left);
    if (root->right) right = generate_code(root->right);

    // if (root->type == REG) return root;

    if (root->type == VAR) {
      SET_NODE(root, REG, vartable[root->data]);
    } else if (root->type == BINARYOP) {
      if ((left->type == REG) && (right->type == REG)) {
        __reg_reg(root, left, right);
      } else if ((left->type == REG) && (right->type == CONST)) {
        __reg_const(root, left, right);
      } else if ((left->type == CONST) && (right->type == REG)) {
        __const_reg(root, left, right);
      } else if ((left->type == CONST) && (right->type == CONST)) {
        __const_const(root, left, right);
      }
    } else if (root->type == UNARYOP) {
      __unary_op(root, left);
    }
  }

  return root;
}
