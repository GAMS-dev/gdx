import re

pat = re.compile(r'^\s*(\d+\.\d+)\s+(\d+\.\d+)\s+(\d+\.\d+)\s+\#(.+)$')


def collect_item_times(fn):
    expl_to_time = {}
    with open(fn) as fp:
        for line in fp.readlines():
            m = pat.match(line)
            if m:
                item_t, expl = float(m.group(3)), m.group(4)
                expl_to_time[expl] = item_t
    return expl_to_time


p3_times = collect_item_times('srcSuiteP3.log')
cxx_times = collect_item_times('srcSuiteCxx.log')

shared_expls = [k for k, v in p3_times.items() if k in cxx_times]

deltas = []
for expl in shared_expls:
    deltas.append((expl, cxx_times[expl]-p3_times[expl]))

for pair in list(sorted(deltas, key=lambda pair: pair[1], reverse=True))[:10]:
    print(pair)
