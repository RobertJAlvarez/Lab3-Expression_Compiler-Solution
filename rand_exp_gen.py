import sys
from random import random, choice, randint
from typing import List

UNARIES = ["(~%s)", "(-%s)"]
BINARIES = [
    "(%s+%s)",
    "(%s-%s)",
    "(%s*%s)",
    "(%s/%s)",
    "(%s&%s)",
    "(%s|%s)",
    "(%s^%s)",
    "(%s<<%s)",
    "(%s>>%s)",
]

PROP_BINARY = 0.7


def generate_expressions(scope: List[str], num_exp: int, num_ops: int) -> List[str]:
    for _ in range(num_ops):
        if random() < PROP_BINARY:  # decide unary or binary operator
            scope.append(choice(BINARIES) % (choice(scope), choice(scope)))
        else:
            scope.append(choice(UNARIES) % choice(scope))
    # return most recent expressions
    return scope[-num_exp:]


if __name__ == "__main__":
    scope = (
        [c for c in "abcdefghij"]
        + [randint(0, 4095) for _ in range(5)]
        + ["(" + str(randint(-2048, -1)) + ")" for _ in range(5)]
    )

    N_EXPS = 15
    N_OPS = 70

    if len(sys.argv) > 1:
        N_EXPS = int(sys.argv[1])

    for expression in generate_expressions(scope, N_EXPS, N_OPS):
        print(expression)

