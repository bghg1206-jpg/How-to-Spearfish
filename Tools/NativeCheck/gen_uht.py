#!/usr/bin/env python3
"""
Generates stand-in *.generated.h files for native compilation of the game module (no UnrealHeaderTool).

For every header that includes "<Name>.generated.h" this writes <out>/<Name>.generated.h containing:
  - CURRENT_FILE_ID for that header
  - one FID_<Name>_<Line>_GENERATED_BODY macro per GENERATED_BODY() line, declaring what UHT would:
      * Super / ThisClass typedefs, StaticClass()/StaticStruct()
      * <Rpc>_Implementation (+ _Validate for WithValidation) for Server/Client/NetMulticast UFUNCTIONs
      * <Event>_Implementation for BlueprintNativeEvent UFUNCTIONs
It also validates a few UHT rules that commonly break real builds:
  - the generated.h include must be the last #include of the header
  - header base names must be unique inside the module
  - ReplicatedUsing=OnRep_X must name a UFUNCTION declared in the same header
  - Replicated/ReplicatedUsing properties need DOREPLIFETIME* in some .cpp
Usage: gen_uht.py <module_dir> <out_dir>
"""
import os
import re
import sys

CLASS_DECL = re.compile(
    r'^\s*(class|struct)\s+(?:alignas\(\w+\)\s+)?(?:[A-Z0-9_]+_API\s+)?(\w+)\s*(?:final\s*)?'
    r'(?::\s*(?:public|protected|private)\s+([\w:<>, ]+?))?\s*(?:\{.*)?$')


def strip_comments(text):
    """Removes // and /* */ comments (string/char literal aware), preserving line numbers."""
    out, index, length = [], 0, len(text)
    while index < length:
        char = text[index]
        nxt = text[index + 1] if index + 1 < length else ''
        if char == '/' and nxt == '/':
            end = text.find('\n', index)
            index = length if end == -1 else end
        elif char == '/' and nxt == '*':
            end = text.find('*/', index + 2)
            end = length if end == -1 else end + 2
            out.append('\n' * text.count('\n', index, end))
            index = end
        elif char in '"\'':
            quote, start = char, index
            index += 1
            while index < length and text[index] != quote:
                index += 2 if text[index] == '\\' else 1
            index += 1
            out.append(text[start:index])
        else:
            out.append(char)
            index += 1
    return ''.join(out)


def balanced_args(text, open_index):
    """Returns (inner_text, close_index) for the parenthesis at open_index."""
    depth = 0
    for index in range(open_index, len(text)):
        char = text[index]
        if char == '(':
            depth += 1
        elif char == ')':
            depth -= 1
            if depth == 0:
                return text[open_index + 1:index], index
    raise ValueError('unbalanced parentheses')


def split_params(params):
    parts, depth, current = [], 0, ''
    for char in params:
        if char in '(<[{':
            depth += 1
        elif char in ')>]}':
            depth -= 1
        if char == ',' and depth == 0:
            parts.append(current)
            current = ''
        else:
            current += char
    if current.strip():
        parts.append(current)
    return parts


def strip_defaults(params):
    cleaned = []
    for part in split_params(params):
        depth, cut = 0, None
        for index, char in enumerate(part):
            if char in '(<[{':
                depth += 1
            elif char in ')>]}':
                depth -= 1
            elif char == '=' and depth == 0:
                cut = index
                break
        part = part if cut is None else part[:cut]
        part = re.sub(r'UPARAM\s*\([^)]*\)', '', part)
        cleaned.append(part.strip())
    return ', '.join(p for p in cleaned if p)


def parse_ufunctions(body_text):
    """Yields (spec, return_type, name, params, is_const) for each UFUNCTION in body_text."""
    for match in re.finditer(r'UFUNCTION\s*\(', body_text):
        spec, close = balanced_args(body_text, match.end() - 1)
        rest = body_text[close + 1:]
        decl_end = rest.find(';')
        brace = rest.find('{')
        if brace != -1 and (decl_end == -1 or brace < decl_end):
            decl_end = brace
        decl = rest[:decl_end]
        paren = decl.find('(')
        if paren == -1:
            continue
        head = decl[:paren].strip()
        params, params_close = balanced_args(decl, paren)
        tail = decl[params_close + 1:]
        head = re.sub(r'\b(virtual|static|FORCEINLINE|inline)\b', '', head).strip()
        name_match = re.search(r'(\w+)\s*$', head)
        if not name_match:
            continue
        name = name_match.group(1)
        return_type = head[:name_match.start()].strip() or 'void'
        yield spec, return_type, name, strip_defaults(params), ('const' in tail)


def find_enclosing_class(lines, body_line_index):
    for index in range(body_line_index - 1, -1, -1):
        line = lines[index]
        if line.rstrip().endswith(';') and '{' not in line:
            continue
        match = CLASS_DECL.match(line)
        if match:
            keyword, name, base = match.group(1), match.group(2), match.group(3)
            base = base.split(',')[0].strip() if base else None
            macro = ''
            for back in range(index - 1, max(index - 6, -1), -1):
                stripped = lines[back].strip()
                if stripped.startswith(('UCLASS', 'USTRUCT', 'UINTERFACE')):
                    macro = stripped.split('(')[0]
                    break
            return keyword, name, base, macro
    return None


def generate(header_path, module_dir, out_dir, errors, onreps):
    raw = open(header_path, encoding='utf-8').read()
    text = strip_comments(raw)
    lines = text.split('\n')
    base_name = os.path.splitext(os.path.basename(header_path))[0]
    rel = os.path.relpath(header_path, module_dir)

    include_lines = [i for i, l in enumerate(lines) if re.match(r'\s*#\s*include\s', l)]
    gen_lines = [i for i, l in enumerate(lines) if re.search(r'#\s*include\s+"([\w/]+)\.generated\.h"', l)]
    if not gen_lines:
        return None
    gen_name = re.search(r'"([\w/]+)\.generated\.h"', lines[gen_lines[0]]).group(1)
    if gen_name != base_name:
        errors.append(f'{rel}: includes {gen_name}.generated.h but header is {base_name}.h')
    if include_lines and include_lines[-1] != gen_lines[0]:
        errors.append(f'{rel}: {base_name}.generated.h must be the last #include')

    file_id = f'FID_{base_name}'
    out = [f'// Generated by Tools/NativeCheck/gen_uht.py for {rel}. Native check only.',
           '#undef CURRENT_FILE_ID', f'#define CURRENT_FILE_ID {file_id}']

    body_indices = [i for i, l in enumerate(lines) if re.search(r'\bGENERATED_(?:U|I)?(?:CLASS|STRUCT|INTERFACE)?_?BODY\s*\(', l)]
    for position, body_index in enumerate(body_indices):
        line_no = body_index + 1
        enclosing = find_enclosing_class(lines, body_index)
        if not enclosing:
            errors.append(f'{rel}:{line_no}: GENERATED_BODY outside of a class/struct')
            continue
        keyword, name, base, macro = enclosing
        end_index = body_indices[position + 1] if position + 1 < len(body_indices) else len(lines)
        body_text = '\n'.join(lines[body_index:end_index])

        decls = []
        for spec, return_type, func, params, is_const in parse_ufunctions(body_text):
            spec_words = set(re.findall(r'\b\w+\b', spec))
            const_suffix = ' const' if is_const else ''
            if spec_words & {'Server', 'Client', 'NetMulticast'}:
                decls.append(f'void {func}_Implementation({params});')
                if 'WithValidation' in spec_words:
                    decls.append(f'bool {func}_Validate({params});')
            elif 'BlueprintNativeEvent' in spec_words:
                decls.append(f'virtual {return_type} {func}_Implementation({params}){const_suffix};')

        if keyword == 'struct' and macro == 'USTRUCT':
            parts = ['public:']
            if base and base not in ('FTableRowBase_NoSuper',):
                parts.append(f'typedef {base} Super;')
            parts.append('static class UScriptStruct* StaticStruct();')
            parts += decls
        elif macro == 'UINTERFACE':
            parts = ['public:', f'typedef {base or "UInterface"} Super;', f'typedef {name} ThisClass;',
                     'static class UClass* StaticClass();'] + decls + ['private:']
        elif name.startswith('I') and not macro:
            parts = ['public:', f'typedef U{name[1:]} UClassType;', f'typedef {name} ThisClass;'] + decls + \
                    ['protected:', f'virtual ~{name}() {{}}', 'private:']
        else:
            parts = ['public:']
            if base:
                parts.append(f'typedef {base} Super;')
            parts += [f'typedef {name} ThisClass;', 'static class UClass* StaticClass();'] + decls + ['private:']
        out.append(f'#define {file_id}_{line_no}_GENERATED_BODY ' + ' '.join(parts))

    # ReplicatedUsing validation
    functions = set(re.findall(r'\bvoid\s+(OnRep_\w+)\s*\(', text))
    for match in re.finditer(r'ReplicatedUsing\s*=\s*(\w+)', text):
        handler = match.group(1)
        if handler not in functions:
            errors.append(f'{rel}: ReplicatedUsing={handler} has no matching void {handler}(...) declaration')
    for match in re.finditer(r'UPROPERTY\s*\(([^)]*\bReplicated(?:Using\s*=\s*\w+)?\b[^)]*)\)\s*([^;]+);', text):
        decl = match.group(2).strip()
        prop = re.search(r'(\w+)\s*(?:=.*)?$', decl)
        if prop:
            onreps.append((rel, prop.group(1)))
    return base_name, '\n'.join(out) + '\n'


def main():
    module_dir, out_dir = sys.argv[1], sys.argv[2]
    os.makedirs(out_dir, exist_ok=True)
    errors, seen, replicated = [], {}, []
    for root, _, files in os.walk(module_dir):
        for file in sorted(files):
            if not file.endswith('.h'):
                continue
            path = os.path.join(root, file)
            result = generate(path, module_dir, out_dir, errors, replicated)
            if not result:
                continue
            base_name, content = result
            if base_name in seen:
                errors.append(f'duplicate header name {base_name}.h: {seen[base_name]} and {path}')
            seen[base_name] = path
            with open(os.path.join(out_dir, base_name + '.generated.h'), 'w', encoding='utf-8') as handle:
                handle.write(content)

    cpp_text = ''
    for root, _, files in os.walk(module_dir):
        for file in files:
            if file.endswith('.cpp'):
                cpp_text += open(os.path.join(root, file), encoding='utf-8').read()
    for rel, prop in replicated:
        if not re.search(r'DOREPLIFETIME\w*\s*\(\s*\w+\s*,\s*' + re.escape(prop) + r'\b', cpp_text):
            errors.append(f'{rel}: replicated property {prop} has no DOREPLIFETIME entry')

    for error in errors:
        print('UHT-CHECK:', error)
    print(f'gen_uht: wrote {len(seen)} generated headers, {len(errors)} problem(s)')
    return 1 if errors else 0


if __name__ == '__main__':
    sys.exit(main())
