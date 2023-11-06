import argparse
import re
import json


def parse(fn):
    with open(fn) as fp:
        contents = fp.read()
    res = dict(memoryMB=dict())
    m = re.findall(r'(\d+\.\d+)\sMB\)', contents)
    for ix, mem_type in enumerate(['RSS', 'VSS']):
        res['memoryMB'][mem_type] = float(m[ix])
    m = re.search(r'elapsed\s(\d+):(\d+):(\d+\.\d+)', contents)
    hours, minutes, seconds = (float(m.group(i)) for i in range(1, 4))
    res['runtimeSecs'] = hours * 3600.0 + minutes * 60.0 + seconds
    return res


def main():
    ap = argparse.ArgumentParser('perflogparse',
                                 description='Parse and compare logs from apilib/gdxperf from GAMS 43 and master')
    ap.add_argument('--delphi_log', type=str,
                    default='delphigdx.log', help='Log from GAMS 43 gdxperf with procTreeMemMonitor=1')
    ap.add_argument('--cpp_log', type=str,
                    default='cppgdx.log', help='Log from GAMS master gdxperf with procTreeMemMonitor=1')
    args = ap.parse_args()
    lang_fn = [('Delphi', args.delphi_log), ('C++', args.cpp_log)]
    languages = [lang for lang, fn in lang_fn]
    res = {lang: parse(fn) for lang, fn in lang_fn}
    assert(res['C++']['runtimeSecs'] <= res['Delphi']['runtimeSecs'])
    slowest_runtime = max(res[lang]["runtimeSecs"] for lang in languages)
    fastest_runtime = min(res[lang]["runtimeSecs"] for lang in languages)
    res['slowdown'] = f'{round((slowest_runtime-fastest_runtime)/fastest_runtime*100.0, 2)}%'
    print(json.dumps(res, sort_keys=True, indent=4))


if __name__ == '__main__':
    main()
