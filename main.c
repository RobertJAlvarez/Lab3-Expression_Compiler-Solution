#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "build_tree.h"

static int get_str(char s[], int lim) {
  int c, i;

  for (i = 0; i < lim - 1 && (c = getchar()) != EOF && c != '\n'; ++i) s[i] = c;
  s[i] = '\0';

  return i;
}

void postorder(node_t *root) {
  if (root) {
    postorder(root->left);
    postorder(root->right);
    if (root->type == UNARYOP || root->type == BINARYOP)
      printf("%s", optable[root->data].symbol);
    else if (root->type == VAR)
      printf("%c", 'a' + (char)root->data);
    else
      printf("%d", root->data);
  }
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

    postorder(root);

    printf("\n");
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
