import itertools

from jinja2 import Environment, FileSystemLoader
import yaml

import re
import textwrap
import argparse

no_html_beautify = False


def contains_html(s):
    return any(a != 'p' for a, b in re.findall(r'<(\w+\s*)(/)?\s*>', s))


def no_paragraphs(s):
    return s.replace('<p>', '').replace('</p>', '')


text_wrapper = textwrap.TextWrapper(width=100, break_long_words=False, replace_whitespace=False)


def beautify_html(s):
    if s is None:
        return None
    if no_html_beautify or not contains_html(s):
        return wrap_text(s)
    import bs4
    raw_lines = str(bs4.BeautifulSoup(no_paragraphs(s), 'html.parser').prettify()).strip().split('\n')
    lines = []
    for line in raw_lines:
        num_spaces = sum(1 for _ in itertools.takewhile(str.isspace, line))
        for ix, narrow_line in enumerate(text_wrapper.fill(line).split('\n')):
            lines.append(' ' * num_spaces + narrow_line if ix > 0 else narrow_line)
    return '\n'.join(('    * ' if ix > 0 else '') + line for ix, line in enumerate(lines))


def parse_details(s):
    see_txt = None
    see_match = re.search(r'<p><b>See: </b>(.+)</p>', s)
    if see_match:
        see_txt = ', '.join(re.findall(r'#(gdx\w+)', see_match[1]))
        s = s.replace(see_match[0], '')
    marker_attention = '<font color="red"><b>Attention: </b></font>'
    marker_note = '<b>Note: </b>'
    expl_txt = s
    attention_txt = note_txt = None
    assert marker_attention not in s or marker_note not in s or s.index(marker_attention) > s.index(marker_note)
    if expl_txt.count(marker_attention) == 1:
        expl_txt, attention_txt = expl_txt.split(marker_attention)
    if expl_txt.count(marker_note) == 1:
        expl_txt, note_txt = expl_txt.split(marker_note)
    return beautify_html(expl_txt), see_txt, beautify_html(attention_txt), beautify_html(note_txt)


def wrap_text(s):
    lines = text_wrapper.fill(s.replace('<p>', '').replace('</p>', '')).replace('<br />', '<br />\n').split('\n')
    return '\n'.join('    *   ' + line if ix > 0 else line for ix, line in enumerate(lines))


def map_type_gen(func_ptrs, for_cpp=True):
    type_map = dict(css='const char *',
                    cSI='const char **',
                    oSS='char *',
                    int='int',
                    vII='int *',
                    Oint='int &' if for_cpp else 'int *',
                    oSI='char **',
                    cRV='const double *',
                    vRV='double *',
                    cII='const int *',
                    vSVA='double *',
                    cSVA='const double *',
                    ptr='void *',
                    D='double',
                    int64='int64_t' if for_cpp else 'INT64')

    func_ptr_types = {
        func_name: func_attrs['function'] + '_t'
        for func in func_ptrs
        for func_name, func_attrs in func.items()
    }

    def map_type(func_name, type_acro):
        if type_acro == 'FuncPtr':
            if func_name in func_ptr_types:
                return func_ptr_types[func_name] + ' '
            else:
                raise RuntimeError(f'Cannot find func ptr type for {func_name}')
        if type_acro in type_map:
            return type_map[type_acro] + (' ' if type_map[type_acro][-1] not in '*&' else '')
        else:
            raise RuntimeError(f'Cannot find type acro {type_acro} for function {func_name}')

    return map_type


def collect_groups(functions):
    groups = []
    gtf = {}
    for f in functions:
        for name, attrs in f.items():
            group = None if 'group' not in attrs else attrs['group']
            if group and group not in groups:
                groups.append(group)
            if group in gtf:
                gtf[group].append(f)
            else:
                gtf[group] = [f]
    groups = [None] + sorted(groups)
    return groups, gtf


def nice_property(prop):
    name = list(prop.keys())[0]
    res = dict(name=name)
    res.update(prop[name])
    return res


def nice_properties(obj):
    return [nice_property(p) for p in obj['properties']]


def generate_method_declarations(input_yaml_fn, template_folder, template_fn, output_header_fn):
    with open(input_yaml_fn) as fp:
        obj = yaml.load(fp, Loader=yaml.FullLoader)
    groups, gtf = collect_groups(obj['functions'])
    env = Environment(loader=FileSystemLoader(template_folder))
    for f in [beautify_html, parse_details, wrap_text, map_type_gen(obj['functionpointers'])]:
        env.globals[f.__name__] = f
    template = env.get_template(template_fn)
    with open(output_header_fn, 'w') as fp:
        fp.write(template.render(obj=obj, groups=groups, gtf=gtf, properties=nice_properties(obj)))


def main():
    p = argparse.ArgumentParser(prog='yaml2doxy',
                                description='Generate GDX header file gdx.hpp from GDX API definition YAML')
    p.add_argument('--input', type=str,
                   default='gdxapi.yaml', help='Filename of GDX API definition YAML file')
    p.add_argument('--template', type=str,
                   default='gdxheader.template.j2', help='Name of Jinja2 template file')
    p.add_argument('--template_folder', type=str,
                   default='templates', help='Name of the folder holding the Jinja2 template files')
    p.add_argument('--output', type=str,
                   default='gdx.hpp',
                   help='Name of C++ GDX header file to be generated')
    p.add_argument('--no_html_beautify', action='store_true')
    args = p.parse_args()
    if args.no_html_beautify:
        global no_html_beautify
        no_html_beautify = True
    generate_method_declarations(args.input,
                                 args.template_folder,
                                 args.template,
                                 args.output)


if __name__ == '__main__':
    main()
