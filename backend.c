#include <stdio.h>
#include <stdlib.h>

#include "build_tree.h"
#include "helper.h"  // position_of_set_bit()

#define NUMREG 32
#define NUMVAR 10

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

static int __reuse_reg(const int reg) {
  if (regtable[reg] == 1) return 1;
  if (regtable[reg] > 1) return 0;

  fprintf(stderr, "Error: called __reuse_reg on unused register\n");

  // shouldn't happen
  return -1;
}

static int __release_reg(const int reg) {
  if (regtable[reg] > 0) {
    regtable[reg]--;
    return 0;
  }

  return -1;
}

int assign_reg(const int var) {
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

static int __get_new_reg(void) {
  int destreg;

  if ((destreg = assign_reg(-1)) == -1) {
    fprintf(stderr, "Error: out of registers\n");
    exit(-1);
  }

  return destreg;
}

static int __get_destreg(const int reg1, const int reg2) {
  int destreg;

  if (reg1 && __reuse_reg(reg1) == 1) {
    destreg = reg1;
    if (reg2) __release_reg(reg2);
  } else if (reg2 && __reuse_reg(reg2) == 1) {
    destreg = reg2;
    __release_reg(reg1);
  } else {
    destreg = __get_new_reg();
    __release_reg(reg1);
    if (reg2) __release_reg(reg2);
  }

  return destreg;
}

static const char *__get_instr(const ops_t op) {
  const static struct {
    ops_t op;
    const char *instr;
  } conversion[] = {
      {ADD, "add"}, {SUB, "sub"}, {MUL, "mul"}, {DIV, "div"}, {AND, "and"},
      {OR, "or"},   {XOR, "xor"}, {SLL, "sll"}, {SRL, "srl"},
  };

  for (size_t i = 0; i < sizeof(conversion) / sizeof(conversion[0]); i++) {
    if (op == conversion[i].op) return conversion[i].instr;
  }

  return "";
}

static void __reg_reg(node_t *root, const int l_data, const int r_data) {
  int destreg = __get_destreg(l_data, r_data);

  printf("%s x%d, x%d, x%d\n", __get_instr((ops_t)root->data), destreg, l_data,
         r_data);

  SET_NODE(root, REG, destreg);
}

static inline int __is_slli(const node_t *root, const int node_data) {
  return (root->data == MUL) && (position_of_set_bit(node_data) != -1);
}

static inline int __is_srai(const node_t *root, const int node_data) {
  return (root->data == DIV) && (position_of_set_bit(node_data) != -1);
}

static int __lui_data(const int imm) {
  const int destreg = __get_new_reg();
  const int high = imm >> 12;
  int low = imm & 0xFFF;

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
   * E7FF = -6145 -> li -8192, addi 2047
   * E000 = -8192 -> li -8192
   */

  if (low >= 0x800) {
    printf("lui x%d, %d\n", destreg, high + 1);
    low -= 0x1000;
  } else {
    printf("lui x%d, %d\n", destreg, high);
  }
  if (low != 0) printf("addi x%d, x%d, %d\n", destreg, destreg, low);

  return destreg;
}

static void __reg_const(node_t *root, const int l_data, const int r_data) {
  int destreg;

  if ((r_data > 0xFFF) || (r_data < -2048)) {
    __reg_reg(root, l_data, __lui_data(r_data));
    return;
  }

  // muli and divi are not valid instructions. Move constant value to a register
  if (((root->data == MUL) || (root->data == DIV)) &&
      (position_of_set_bit(r_data) == -1)) {
    destreg = __get_new_reg();

    printf("addi x%d, x0, %d\n", destreg, r_data);

    __reg_reg(root, l_data, destreg);

    return;
  }

  // Get register to assign operation
  destreg = __get_destreg(l_data, 0);

  if (__is_slli(root, r_data)) {
    printf("slli x%d, x%d, %d\n", destreg, l_data, position_of_set_bit(r_data));
  } else if (__is_srai(root, r_data)) {
    printf("srai x%d, x%d, %d\n", destreg, l_data, position_of_set_bit(r_data));
  } else if ((root->data == SUB) && (r_data <= 2048)) {
    printf("addi x%d, x%d, -%d\n", destreg, l_data, r_data);
  } else {
    printf("%si x%d, x%d, %d\n", __get_instr((ops_t)root->data), destreg,
           l_data, r_data);
  }

  SET_NODE(root, REG, destreg);
}

static void __const_reg(node_t *root, const int l_data, const int r_data) {
  int destreg;

  if ((l_data > 0xFFF) || (l_data < -2048)) {
    __reg_reg(root, __lui_data(l_data), r_data);
    return;
  }

  if ((root->data == ADD) || (root->data == AND) || (root->data == OR) ||
      (root->data == XOR) || (__is_slli(root, l_data))) {
    // If operator commute and there is an I-type instruction
    __reg_const(root, r_data, l_data);
  } else {
    destreg = __get_new_reg();

    printf("addi x%d, x0, %d\n", destreg, l_data);

    __reg_reg(root, destreg, r_data);
  }
}

static void __const_const(node_t *root, const int a, const int b) {
  int data;

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

static void __unary_reg(node_t *root, const int l_data) {
  int destreg = __get_destreg(l_data, 0);

  if (root->data == UMINUS) {
    printf("sub x%d, x0, x%d\n", destreg, l_data);
  } else if (root->data == NOT) {
    printf("xori x%d, x%d, -1\n", destreg, l_data);
  }

  SET_NODE(root, REG, destreg);
}

static void __unary_const(node_t *root, const int l_data) {
  if (root->data == UMINUS) {
    SET_NODE(root, CONST, -l_data);
  } else if (root->data == NOT) {
    SET_NODE(root, CONST, ~l_data);
  }
}

node_t *generate_code(node_t *root) {
  node_t *left, *right;

  if ((root == NULL) || (root->type == REG)) return root;

  if (root->type == VAR) {
    SET_NODE(root, REG, vartable[root->data]);
    return root;
  }

  left = generate_code(root->left);
  right = generate_code(root->right);

  if (root->type == BINARYOP) {
    if ((left->type == REG) && (right->type == REG)) {
      __reg_reg(root, left->data, right->data);
    } else if ((left->type == REG) && (right->type == CONST)) {
      __reg_const(root, left->data, right->data);
    } else if ((left->type == CONST) && (right->type == REG)) {
      __const_reg(root, left->data, right->data);
    } else if ((left->type == CONST) && (right->type == CONST)) {
      __const_const(root, left->data, right->data);
    } else {
      fprintf(stderr,
              "Invalid type for right or left node. right->type = %d, "
              "left->type = %d\n",
              right->type, left->type);
    }
    root->right = NULL;
    free(right);
  } else if (root->type == UNARYOP) {
    if (left->type == REG) {
      __unary_reg(root, left->data);
    } else if (left->type == CONST) {
      __unary_const(root, left->data);
    }
  }

  root->left = NULL;
  free(left);

  return root;
}
