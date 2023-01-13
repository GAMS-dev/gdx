import yaml

with open('results.yml') as fp:
    try:
        res = yaml.safe_load(fp)
    except yaml.YAMLError as exc:
        print(exc)
        raise exc

max_rss, max_tot_time = None, None
max_rss_fn, max_time_fn = '', ''

fns, fn_to_rss_incr, fn_to_slowdown = [], {}, {}

for fn, pair in res.items():
    if not pair['cxx']['max_rss']: continue
    fns.append(fn)
    rss_incr = max(1, pair['cxx']['max_rss']/pair['p3']['max_rss'])
    fn_to_rss_incr[fn] = rss_incr
    slowdown = max(1, pair['cxx']['total_secs']/pair['p3']['total_secs'])
    fn_to_slowdown[fn] = slowdown
    if not max_rss or rss_incr > max_rss:
        max_rss = rss_incr
        max_rss_fn = fn
    if not max_tot_time or slowdown > max_tot_time:
        max_tot_time = slowdown
        max_time_fn = fn

print(f'Max time slowdown fn: {max_time_fn} and time: {max_tot_time}')
print(f'Max RSS increase: {max_rss_fn} and RSS: {max_rss}')

print('\nTop 10 slowdown:')
for fn in sorted(fns, key=lambda fn: fn_to_slowdown[fn], reverse=True)[:10]:
    print(f'{fn} with slowdown {fn_to_slowdown[fn]} and cxx time {res[fn]["cxx"]["total_secs"]}')

print('\nTop 10 VSS increase:')
for fn in sorted(fns, key=lambda fn: fn_to_rss_incr[fn], reverse=True)[:10]:
    print(f'{fn} with increase {fn_to_rss_incr[fn]} and cxx time {res[fn]["cxx"]["total_secs"]}')