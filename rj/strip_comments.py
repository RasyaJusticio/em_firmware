import difflib
import re
import sys

KEEP = re.compile(r"SPDX-License-Identifier|Copyright|clang-format")


def strip_line_comments(text, line_ok):
    out, i, n, line = [], 0, len(text), 0
    while i < n:
        c = text[i]
        if c == "\n":
            line += 1
            out.append(c)
            i += 1
        elif c in "\"'":
            q, j = c, i + 1
            while j < n and text[j] != q:
                j += 2 if text[j] == "\\" else 1
            out.append(text[i : j + 1])
            i = j + 1
        elif text.startswith("//", i):
            j = text.find("\n", i)
            j = n if j < 0 else j
            if line_ok(line) and not KEEP.search(text[i:j]):
                i = j
            else:
                out.append(text[i:j])
                i = j
        elif text.startswith("/*", i):
            j = text.find("*/", i + 2)
            j = n if j < 0 else j + 2
            body = text[i:j]
            if line_ok(line) and not KEEP.search(body):
                line += body.count("\n")
                out.append("\n" * body.count("\n"))
            else:
                line += body.count("\n")
                out.append(body)
            i = j
        else:
            out.append(c)
            i += 1
    return "".join(out)


def tidy(orig_lines, text):
    lines = [l.rstrip() for l in text.split("\n")]
    res = []
    for k, l in enumerate(lines):
        if l == "" and res and res[-1] == "":
            continue
        res.append(l)
    while res and res[0] == "":
        res.pop(0)
    return "\n".join(res).rstrip("\n") + "\n"


def main():
    mode = sys.argv[1]
    if mode == "all":
        for p in sys.argv[2:]:
            s = open(p, encoding="utf-8", newline="").read().replace("\r\n", "\n")
            r = strip_line_comments(s, lambda _l: True)
            src_lines, new_lines = s.split("\n"), r.split("\n")
            keep = [nl for sl, nl in zip(src_lines, new_lines) if not (nl.strip() == "" and sl.strip() != "")]
            open(p, "w", encoding="utf-8", newline="\n").write(tidy(None, "\n".join(keep)))
            print("stripped", p)
    elif mode == "added":
        orig, p = sys.argv[2], sys.argv[3]
        o = open(orig, encoding="utf-8", newline="").read().replace("\r\n", "\n").split("\n")
        s = open(p, encoding="utf-8", newline="").read().replace("\r\n", "\n")
        cur = s.split("\n")
        added = set()
        sm = difflib.SequenceMatcher(a=o, b=cur, autojunk=False)
        for tag, a1, a2, b1, b2 in sm.get_opcodes():
            if tag in ("replace", "insert"):
                added.update(range(b1, b2))
        r = strip_line_comments(s, lambda l: l in added)
        new_lines = r.split("\n")
        keep = [nl for k, (sl, nl) in enumerate(zip(cur, new_lines)) if not (k in added and nl.strip() == "" and sl.strip() != "")]
        out = "\n".join(l.rstrip() if k in added else l for k, l in enumerate(keep))
        open(p, "w", encoding="utf-8", newline="").write(out)
        print("stripped added comments in", p)


main()
