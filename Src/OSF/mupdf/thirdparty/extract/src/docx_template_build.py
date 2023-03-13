#! /usr/bin/env python3

'''
Creates C code for creating docx files using internal template docx content.

Args:

    -i <docx-path>
        Set template docx file to extract from.

    -o <out-path>
        Set name of output files.
        
        We write to <out-path>.c and <out-path>.h.
'''

import io
import os
import re
import sys
import textwrap


def system(command):
    '''
    Like os.system() but raises exception if command fails.
    '''
    e = os.system(command)
    if e:
        print(f'command failed: {command}')
        assert 0

def read(path):
    '''
    Returns contents of file. We assume it is utf-8.
    '''
    with open(path, 'rb') as f:
        raw = f.read()
        return raw.decode('utf-8')

def write(text, path):
    '''
    Writes text to file.
    '''
    parent = os.path.dirname(path)
    if parent:
        os.makedirs(parent, exist_ok=True)
    with open(path, 'w') as f:
        f.write(text)

def write_if_diff(text, path):
    try:
        old = read(path)
    except Exception:
        old = None
    if text != old:
        write(text, path)

def check_path_safe(path):
    '''
    Raises exception unless path consists only of characters and sequences that
    are known to be safe for shell commands.
    '''
    if '..' in path:
        raise Exception(f'Path is unsafe because contains "..": {path!r}')
    for c in path:
        if not c.isalnum() and c not in '/._-':
            #print(f'unsafe character {c} in: {path}') 
            raise Exception(f'Path is unsafe because contains "{c}": {path!r}')

def path_safe(path):
    '''
    Returns True if path is safe else False.
    '''
    try:
        check_path_safe(path)
    except Exception:
        return False
    else:
        return True

assert not path_safe('foo;rm -rf *')
assert not path_safe('..')
assert path_safe('foo/bar.x')


def main():

    path_in = None
    path_out = None
    args = iter(sys.argv[1:])
    while 1:
        try: arg = next(args)
        except StopIteration: break
        if arg == '-h' or arg == '--help':
            print(__doc__)
            return
        elif arg == '--docx-pretty':
            d = next(args)
            for dirpath, dirnames, filenames in os.walk(d):
                for filename in filenames:
                    if not filename.endswith('.xml'):
                        continue
                    path = os.path.join(dirpath, filename)
                    system(f'xmllint --format {path} > {path}-')
                    system(f'mv {path}- {path}')
        elif arg == '-i':
            path_in = next(args)
        elif arg == '-o':
            path_out = next(args)
        else:
            assert 0
    
    if not path_in:
        return
    
    if not path_in:
        raise Exception('Need to specify -i <docx-path>')
    if not path_out:
        raise Exception('Need to specify -o <out-path>')
    
    check_path_safe(path_in)
    check_path_safe(path_out)
    path_temp = f'{path_in}.dir'
    os.system(f'rm -r "{path_temp}" 2>/dev/null')
    system(f'unzip -q -d {path_temp} {path_in}')
    
    out_c = io.StringIO()
    out_c.write(f'/* THIS IS AUTO-GENERATED CODE, DO NOT EDIT. */\n')
    out_c.write(f'\n')
    out_c.write(f'#include "{os.path.basename(path_out)}.h"\n')
    out_c.write(f'\n')
    
    
    out_c.write('const docx_template_item_t docx_template_items[] =\n')
    out_c.write(f'{{\n')
    
    num_items = 0
    for dirpath, dirnames, filenames in os.walk(path_temp):
        dirnames.sort()
        
        if 0:
            # Write code to create directory item in zip. This isn't recognised by zipinfo, and doesn't
            # make Word like the file.
            #
            name = dirpath[ len(path_temp)+1: ]
            if name:
                if not name.endswith('/'):
                    name += '/'
                    out_c3.write(f'        if (extract_zip_write_file(zip, NULL, 0, "{name}")) goto end;\n')
        
        for filename in sorted(filenames):
            num_items += 1
            path = os.path.join(dirpath, filename)
            name = path[ len(path_temp)+1: ]
            text = read(os.path.join(dirpath, filename))
            #print(f'first line is: %r' % text.split("\n")[0])
            text = text.replace('"', '\\"')
            
            # Looks like template files use \r\n when we interpret them as
            # utf-8, so we preserve this in the generated strings.
            #
            text = text.replace('\r\n', '\\r\\n"\n                "')

            # Split on '<' to avoid overly-long lines, which break windows
            # compiler.
            #
            text = re.sub('([<][^/])', '"\n                "\\1', text)
            
            out_c.write(f'    {{\n')
            out_c.write(f'        "{name}",\n')
            out_c.write(f'        "{text}"\n')
            out_c.write(f'    }},\n')
            out_c.write(f'    \n')
    
    out_c.write(f'}};\n')
    out_c.write(f'\n')
    out_c.write(f'int docx_template_items_num = {num_items};\n')
    
    out_c = out_c.getvalue()
    write_if_diff(out_c, f'{path_out}.c')
    
    out_h = io.StringIO()
    out_h.write(f'#ifndef EXTRACT_DOCX_TEMPLATE_H\n')
    out_h.write(f'#define EXTRACT_DOCX_TEMPLATE_H\n')
    out_h.write(f'\n')
    out_h.write(f'/* THIS IS AUTO-GENERATED CODE, DO NOT EDIT. */\n')
    out_h.write(f'\n')
    out_h.write(f'\n')
    out_h.write(f'typedef struct\n')
    out_h.write(f'{{\n')
    out_h.write(f'    const char* name; /* Name of item in docx archive. */\n')
    out_h.write(f'    const char* text; /* Contents of item in docx archive. */\n')
    out_h.write(f'}} docx_template_item_t;\n')
    out_h.write(f'\n')
    out_h.write(f'extern const docx_template_item_t docx_template_items[];\n')
    out_h.write(f'extern int docx_template_items_num;\n')
    out_h.write(f'\n')
    out_h.write(f'\n')
    out_h.write(f'#endif\n')
    write_if_diff(out_h.getvalue(), f'{path_out}.h')
    #os.system(f'rm -r "{path_temp}"')
    
if __name__ == '__main__':
    main()
