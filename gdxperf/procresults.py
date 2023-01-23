import os.path

import yaml
import sys

import runbench


def main(args):
    with open('results.yml') as fp:
        try:
            res = yaml.safe_load(fp)
        except yaml.YAMLError as exc:
            print(exc)
            raise exc

    total_count = len(runbench.collect_gdx_filenames('.'))
    result_count = len(res)
    print(f'Collected results for {result_count}/{total_count} files ({round(result_count / total_count * 100.0, 2)}%)')

    max_rss, max_tot_time = None, None
    max_rss_fn, max_time_fn = '', ''

    fns, fn_to_rss_incr, fn_to_slowdown = [], {}, {}

    for fn, pair in res.items():
        if not pair['cxx']['max_rss']: continue
        fns.append(fn)
        rss_incr = max(1, pair['cxx']['max_rss'] / pair['p3']['max_rss'])
        fn_to_rss_incr[fn] = round((rss_incr-1.0)*100.0, 2)
        slowdown = max(1, pair['cxx']['total_secs'] / pair['p3']['total_secs'])
        fn_to_slowdown[fn] = round((slowdown-1.0)*100.0, 2)
        if not max_rss or rss_incr > max_rss:
            max_rss = rss_incr
            max_rss_fn = fn
        if not max_tot_time or slowdown > max_tot_time:
            max_tot_time = slowdown
            max_time_fn = fn

    print(f'Max time slowdown fn: {max_time_fn} and time: {round(max_tot_time, 2)}')
    print(f'Max RSS increase: {max_rss_fn} and RSS: {round(max_rss, 2)}')

    def rfill_blanks(s, dsize=30):
        l = len(s)
        return s if dsize <= l else s + (' ' * (dsize - l))

    toplist = []

    def res_table(title, headers, fn_to_res_mapping, fn_to_altres_mapping):
        print(f'\n{title}:')
        print(f'{rfill_blanks(headers[0])}{rfill_blanks(headers[1], 10)}{rfill_blanks(headers[2], 10)}{headers[3]}')
        for fn in sorted(fns, key=lambda fn: fn_to_res_mapping[fn], reverse=True)[:10]:
            print(f'{rfill_blanks(fn)}' +
                  f'{rfill_blanks(str(fn_to_res_mapping[fn])+"%", 10)}' +
                  f'{rfill_blanks(str(fn_to_altres_mapping[fn])+"%", 10)}' +
                  f'{round(res[fn]["cxx"]["total_secs"], 2)}')
            toplist.append(fn)

    res_table('Top 10 slowdown', ['GDX filename', 'slowdown', 'rss+', 'time'], fn_to_slowdown, fn_to_rss_incr)
    res_table('Top 10 RSS increase', ['GDX filename', 'rss+', 'slowdown', 'time'], fn_to_rss_incr, fn_to_slowdown)

    if not os.path.exists('toplist.txt'):  # do not overwrite
        with open('toplist.txt', 'w') as fp:
            fp.write('\n'.join(toplist))


if __name__ == '__main__':
    main(sys.argv)
