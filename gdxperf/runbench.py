import os
import pickle
import re
import sys

SKIPLIST = [
    '5062.gdx',
    'data1000.gdx',
    'data10000.gdx',
    'Modellendogen0004.gdx', 'Modellendogen0005.gdx', 'Modellendogen0725.gdx', 'Modellendogen0726.gdx',
    'Modellendogen1093.gdx', 'Modellendogen1407.gdx',
    'output2.gdx', 'output_DisEmp.gdx',
    'preFix.gdx',
    'soln_Toroptimierung_2_p1.gdx', 'Toroptimierung_2_p.gdx', 'Toroptimierung_2_p - Copy.gdx',
    'z.gdx',
    'data_m5.gdx',
    'dbX2370.gdx', 'dbX2372.gdx',
    'gmsgrid.gdx',
    'testInstance1000Ite.gdx', 'testInstance100Ite.gdx', 'testInstance200Ite.gdx',
    'x0.gdx', 'x1.gdx', 'x170.gdx', 'x170postfix.gdx', 'x171.gdx',
    'test.gdx', 'gdxdump.gdx',
    'map.gdx', 'new4.gdx',
    'food_man.gdx',
    'testExcel.gdx',
    'ELMOD.gdx', 'JMM.gdx',
    'arauco.gdx',
    'modelo.gdx',
    'PERSEUS.gdx'
    'AssignmentBug.gdx', 'globiom.gdx',
    'indata.gdx', 'seders.gdx',
    'CHPSystemData2_Converted.gdx',
    'pegase.gdx',
    'getdata.gdx',
    'rank_out.gdx', 'rank_output.gdx',
    'bau_p.gdx'
    'validnlpecopts.gdx', 'xyz.gdx'
]

PATH_BASE = "/home/andre/dockerhome/distrib/"
P3_GAMS_PATH = PATH_BASE + "master/gms\\ test/"
CXX_GAMS_PATH = PATH_BASE + "integrate-gdxnative/gms\\ test/"

rss_pat = re.compile(r'^\s*highwater RSS:\s*\d+\s*\(\s*(\d+\.\d+)\s*(MB|GB)\)$')
tot_time_pat = re.compile(r'^--- Job gdxperf.gms Stop \d\d/\d\d/\d\d \d\d:\d\d:\d\d elapsed (\d+):(\d\d):(\d\d).(\d+)$')
step_time_pat = re.compile(r'^\s*(\d+\.\d+)\s+#\s+(\d+\.\d+)\s+#\s+(\d+\.\d+)\s+#(.+)$')

log_fn = 'log.txt'


def get_cmd(sysdir_path, fn):
    perf_args = f"--RUN single --SINGLERUN {fn} --RUNDEFAULT 0 --RUNCAPI 1"
    sf = f" | tee {log_fn}"
    return f"{sysdir_path}gams {sysdir_path}gdxperf.gms ProcTreeMemMonitor 1 {perf_args} {sf}"


def run_single(sysdir_path, fn):
    cmd = get_cmd(sysdir_path, fn)
    print(cmd)
    os.system(cmd)
    with open(log_fn) as fp:
        lines = fp.readlines()
    aggr_secs, max_rss = None, None
    step_times = []
    for line in lines:
        step_t_match = step_time_pat.match(line)
        if step_t_match:
            step_times.append(dict(t=float(step_t_match.group(3)),
                                   expl=step_t_match.group(4).split('#')[-1].strip()))
        tot_t_match = tot_time_pat.match(line)
        if tot_t_match:
            hours, minutes, secs, msecs = (int(tot_t_match.group(i)) for i in [1, 2, 3, 4])
            aggr_secs = msecs / 1000.0 + secs + minutes * 60.0 + hours * 3600.0
            continue
        rss_match = rss_pat.match(line)
        if rss_match:
            max_rss = float(rss_match.groups(1)[0])
            if rss_match.groups(2) == 'GB':
                max_rss *= 1000
            continue
        if aggr_secs and max_rss:
            break
    return dict(total_secs=aggr_secs,
                max_rss=max_rss,
                step_times=step_times)


def collect_gdx_filenames(root_dir):
    fns = []

    def recursive_helper(dir):
        for fn in os.listdir(dir):
            if fn.startswith('.'):
                continue
            elif os.path.isdir(fn):
                recursive_helper(fn)
            elif fn.endswith('.gdx') and fn.split('/')[-1] not in SKIPLIST:
                fns.append(dir + os.path.sep + fn)

    recursive_helper(root_dir)
    return fns


def collect_results():
    res = []
    for fn in collect_gdx_filenames('.'):
        p3_res, cxx_res = (run_single(sys_dir, fn) for sys_dir in [P3_GAMS_PATH, CXX_GAMS_PATH])
        res.append(dict(fn=fn, p3=p3_res, cxx=cxx_res))
    with open('results.pkl', 'w') as fp:
        pickle.dump(res, fp)


def main(args):
    collect_results()


if __name__ == '__main__':
    main(sys.argv)
