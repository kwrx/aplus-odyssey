import re
import sys

source, output = sys.argv[1], sys.argv[2]

entries = []
lines = open(source, encoding="latin-1").read().splitlines()
i = 0
while i < len(lines):
    m = re.match(r"^(MSG_[A-Z0-9_]+)\s*\(", lines[i])
    if not m:
        i += 1
        continue
    text = []
    i += 1
    while i < len(lines) and not lines[i].startswith(";") and not re.match(r"^MSG_[A-Z0-9_]+\s*\(", lines[i]):
        line = lines[i]
        cont = line.endswith("\\")
        text.append(line[:-1] if cont else line)
        i += 1
        if not cont:
            break
    entries.append((m.group(1), "".join(text)))

def escape(s):
    return s.replace('"', '\\"')

with open(output, "w", encoding="utf-8") as f:
    f.write("#pragma once\n\n")
    for index, (name, _) in enumerate(entries):
        f.write("#define %s %d\n" % (name, index))
    f.write("\nstatic const char* const aplusCatalogStrings[] = {\n")
    for _, text in entries:
        f.write('    "%s",\n' % escape(text))
    f.write("};\n\n#define GSI(id) (aplusCatalogStrings[(id)])\n")
