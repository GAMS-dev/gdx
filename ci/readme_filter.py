with open('README.md') as fp:
    lines = fp.readlines()
out_lines = []
in_ignore = False
for line in lines:
    if line.startswith('<!-- skip doxygen begin -->'):
        in_ignore = True
        continue
    if line.startswith('<!-- skip doxygen end -->'):
        in_ignore = False
        continue
    if not in_ignore:
        out_lines.append(line)
with open('README_filtered.md', 'w') as fp:
    fp.write(''.join(out_lines))
