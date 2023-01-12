import re

pat = re.compile(r'^\s*(\d+\.\d+)\s+#\s+(\d+\.\d+)\s+#\s+(\d+\.\d+)\s+#(.+)$')


def collect_item_times(fn):
    expl_to_time = {}
    with open(fn) as fp:
        for line in fp.readlines():
            m = pat.match(line)
            if m:
                item_t, expl = float(m.group(3)), m.group(4)
                expl_to_time[expl] = item_t
    return expl_to_time


p3_times = collect_item_times('all_capi_p3.log')
cxx_times = collect_item_times('all_capi_cxx.log')

shared_expls = [k for k, v in p3_times.items() if k in cxx_times]


def slowdown(new_t, old_t):
    if old_t == 0:
        return new_t if new_t > 0 else 0
    sign = 1 if new_t > old_t else -1
    return sign * new_t / old_t


deltas = []
for expl in shared_expls:
    new_t, old_t = cxx_times[expl], p3_times[expl]
    deltas.append(dict(expl=expl, slowdown=slowdown(new_t, old_t), new_t=new_t, old_t=old_t))

for pair in list(sorted(deltas, key=lambda row: row['slowdown'], reverse=True))[:10]:
    print(pair)
