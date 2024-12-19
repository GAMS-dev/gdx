import argparse
from jinja2 import Environment, FileSystemLoader
import yaml

import yaml2doxy


def maybe_deref(arg_name, arg_type, deref_char='*'):
    return (deref_char if arg_type == 'Oint' else '') + arg_name


def fp_decor(t, s):
    if s.strip().endswith('Proc_t'):
        return '::' + s
    elif s == 'DP':
        return f'(gdx::{t.rstrip()}) {s}'
    else:
        return s


custom_bodies = {
    'gdxUMUelGet':
        '''if(!UelMap)
   {
     int tmpUelMap;
     return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxUMUelGet(UelNr, Uel, tmpUelMap );
   }'''
}


def generate_c_wrapper(input, template_folder, template, output):
    with open(input) as fp:
        obj = yaml.load(fp, Loader=yaml.FullLoader)
    env = Environment(loader=FileSystemLoader(template_folder))
    cpp_wrap = template.endswith('gdxcppwrap.template.j2')
    env.globals['map_type'] = yaml2doxy.map_type_gen(obj['functionpointers'], for_cpp=cpp_wrap)
    env.globals['maybe_deref'] = maybe_deref
    env.globals['fp_decor'] = fp_decor
    template = env.get_template(template)
    with open(output, 'w') as fp:
        fp.write(template.render(obj=obj, properties=yaml2doxy.nice_properties(obj), custom_bodies=custom_bodies))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--input', type=str,
                    default='gdxapi.yaml', help='YAML API definition input filename')
    ap.add_argument('--output', type=str,
                    default='gdxcwrap.h', help='Output GDX C-API wrapper header filename')
    ap.add_argument('--template', type=str,
                    default='cwrap.template.j2', help='Name of Jinja2 template file')
    ap.add_argument('--template_folder', type=str,
                    default='templates', help='Name of the folder holding the Jinja2 template files')
    args = ap.parse_args()
    generate_c_wrapper(args.input, args.template_folder, args.template, args.output)


if __name__ == '__main__':
    main()
