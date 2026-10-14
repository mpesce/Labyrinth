"""Extract function definitions (in order) and their string literals from C/C++ sources.
Usage: python srcfuncs.py file.c [...]  -> prints  file\tname\tstrings(joined by |)
"""
import re, sys

def strip_comments(s):
    out = []
    i = 0
    n = len(s)
    while i < n:
        if s.startswith('/*', i):
            j = s.find('*/', i + 2)
            if j < 0: break
            out.append(' ' * 0 + '\n' * s.count('\n', i, j + 2))
            i = j + 2
        elif s.startswith('//', i):
            j = s.find('\n', i)
            if j < 0: break
            i = j
        elif s[i] == '"':
            j = i + 1
            while j < n and s[j] != '"':
                if s[j] == '\\': j += 1
                j += 1
            out.append(s[i:j + 1]); i = j + 1
        elif s[i] == "'":
            j = i + 1
            while j < n and s[j] != "'":
                if s[j] == '\\': j += 1
                j += 1
            out.append(s[i:j + 1]); i = j + 1
        else:
            out.append(s[i]); i += 1
    return ''.join(out)

KW = {'if', 'while', 'for', 'switch', 'return', 'sizeof', 'do', 'else', 'case'}

def functions(text):
    """Find top-level brace blocks preceded by a ')' -> function bodies."""
    text = strip_comments(text)
    # remove preprocessor lines
    text = re.sub(r'^\s*#.*?(?<!\\)$', '', text, flags=re.M)
    res = []
    depth = 0
    i = 0
    n = len(text)
    last_top = 0
    while i < n:
        c = text[i]
        if c == '"':
            j = i + 1
            while j < n and text[j] != '"':
                if text[j] == '\\': j += 1
                j += 1
            i = j + 1; continue
        if c == "'":
            j = i + 1
            while j < n and text[j] != "'":
                if text[j] == '\\': j += 1
                j += 1
            i = j + 1; continue
        if c == '{':
            if depth == 0:
                head = text[last_top:i]
                start = i
            depth += 1
        elif c == '}':
            depth -= 1
            if depth == 0:
                body = text[start:i + 1]
                h = head.strip()
                m = None
                if ')' in h:
                    # name is identifier before the first '(' of the declarator
                    hh = re.sub(r'\s+', ' ', h)
                    # handle K&R: take segment up to matching paren of the declarator
                    mm = re.search(r'([A-Za-z_][\w:~]*(?:\s*operator\s*\S+)?)\s*\(', hh)
                    if mm and mm.group(1) not in KW:
                        m = mm.group(1)
                if m and not re.search(r'\b(struct|class|union|enum)\b[^()]*$', h.split('(')[0] if '(' in h else h):
                    strs = re.findall(r'"((?:[^"\\]|\\.)*)"', body)
                    res.append((m, strs))
                last_top = i + 1
        elif c == ';' and depth == 0:
            last_top = i + 1
        i += 1
    return res

if __name__ == '__main__':
    for fn in sys.argv[1:]:
        for name, strs in functions(open(fn, encoding='latin1').read()):
            print('%s\t%s\t%s' % (fn, name, '|'.join(strs)))
