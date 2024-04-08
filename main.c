#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "build_tree.h"

static int get_str(char s[], int lim) {
  int c, i;

  for (i = 0; i < lim - 1 && (c = getchar()) != EOF && c != '\n'; ++i)
    s[i] = ((char)c);
  s[i] = '\0';

  return i;
}

static void __postorder(const node_t *root) {
  if (root) {
    __postorder(root->left);
    __postorder(root->right);
    if (root->type == UNARYOP || root->type == BINARYOP)
      printf("%s", optable[root->data].symbol);
    else if (root->type == VAR)
      printf("%c", 'a' + (char)root->data);
    else
      printf("%d", root->data);
  }
}

static void __print_2D_util(const node_t *node, const int space) {
  // Base case
  if (node == NULL) return;

  // Process right child first
  __print_2D_util(node->right, space + 1);

  // Print current node
  for (int i = 0; i < space; i++) printf("   ");
  switch (node->type) {
    case VAR:
      printf("%c\n", 'a' + node->data);
      break;
    case REG:
      break;
    case CONST:
      printf("%d\n", node->data);
      break;
    case UNARYOP:
    case BINARYOP:
      printf("%s\n", optable[node->data].symbol);
      break;
    default:
      printf("ERROR: Invalid node type\n");
  }

  // Process left child
  __print_2D_util(node->left, space + 1);
}

int main(void) {
  char expr1[MAXEXPRLENGTH];
  node_t *root;

  while (get_str(expr1, MAXEXPRLENGTH)) {
    printf("Expression to parse: %s\n", expr1);

    root = build_tree(expr1);

    printvartable();
    printregtable();

    printf("Postfix: ");
    __postorder(root);
    printf("\n");

    // printf("AST:\n");
    //__print_2D_util(root, 0);

    root = generate_code(root);

    if (root->type == REG)
      printf("root: x%d\n", root->data);
    else if (root->type == CONST)
      printf("root: %d\n", root->data);

    printregtable();
    printf("\n");
  }

  return 0;
}
