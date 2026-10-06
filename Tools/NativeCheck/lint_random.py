#!/usr/bin/env python3
"""
Determinism lint: fails on any statement that draws more than once from a random stream.

C++ leaves the evaluation order of function arguments and operands unspecified, so
FVector(Rng.FRand(), Rng.FRand(), Rng.FRand()) assigns the draws to different components depending on
the compiler (gcc and clang disagree). The static world is generated locally on every machine from a
replicated seed, so this would give players different worlds. Use one draw per statement or the
SpearfishRandom helpers (Rules/SpearfishRulesTypes.h). Usage: lint_random.py <module_dir>
"""
import os
import re
import sys

DRAW = re.compile(r'\b\w+\s*(?:\.|->)\s*(?:FRand|FRandRange|RandRange|RandHelper|VRand|VRandCone|GetUnitVector|RandBool|GetFraction|GetUnsignedInt)\s*\(')


def statements(text):
    """Yields (line, statement) split at ';', '{' and '}' outside parentheses."""
    depth, start = 0, 0
    for index, char in enumerate(text):
        if char == '(':
            depth += 1
        elif char == ')':
            depth -= 1
        elif char in ';{}' and depth == 0:
            yield text.count('\n', 0, start) + 1, text[start:index]
            start = index + 1


def main():
    module_dir = sys.argv[1]
    problems = []
    for root, _, files in os.walk(module_dir):
        for name in files:
            if not name.endswith(('.cpp', '.h')):
                continue
            path = os.path.join(root, name)
            text = re.sub(r'//[^\n]*', '', open(path, encoding='utf-8').read())
            text = re.sub(r'/\*.*?\*/', lambda m: '\n' * m.group(0).count('\n'), text, flags=re.S)
            for line, statement in statements(text):
                if len(DRAW.findall(statement)) > 1:
                    problems.append(f'{os.path.relpath(path, module_dir)}:{line}: several random draws in one statement: {" ".join(statement.split())[:120]}')
    for problem in problems:
        print('RANDOM:', problem)
    print(f'lint_random: {len(problems)} problem(s)')
    return 1 if problems else 0


if __name__ == '__main__':
    sys.exit(main())
