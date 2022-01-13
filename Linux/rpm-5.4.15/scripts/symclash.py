#!/usr/bin/python

import os
import glob
import sys
import re

# Maximum number of collisions to print
max_emit = 5

whitelist = [
    [ '/usr/lib/libncurses.so', '/usr/lib/libncursesw.so', '.*' ],
    [ '/usr/lib/libform.so', '/usr/lib/libformw.so', '.*' ],
    [ '/usr/lib/libpanel.so', '/usr/lib/libpanelw.so', '.*' ],
    [ '/usr/lib/libmenu.so', '/usr/lib/libmenuw.so', '.*' ],
    [ '/usr/lib/libpng[0-9]*.so', '/usr/lib/libpng.so', '.*' ],
    [ '/usr/lib/libhistory.so', '/usr/lib/libreadline.so', '.*' ],
    [ '/usr/lib/libhistory.so.[0-9]', '/usr/lib/libhistory.so.[0-9]', '.*' ],
    [ '/usr/lib/libreadline.so.[0-9]', '/usr/lib/libreadline.so.[0-9]', '.*' ],
    [ '/usr/lib/libXaw*.so', '/usr/lib/libXaw*.so', '.*' ],
    [ '/usr/lib/libfam.so', '/usr/lib/libgamin-[0-9].so', '.*' ], 
    [ '/usr/lib/libg2c.so', '/usr/lib/libgfortran.so', '.*' ],
    [ '/usr/lib/lib.*XvMC.*.so', '/usr/lib/lib.*XvMC.*.so', '.*' ],
    [ '/usr/lib/php/modules/.*.so', '/usr/lib/php/modules/.*.so', 'get_module' ],
    ]

ignore_list = [ 
    '/usr/lib/libartsdsp.so',
    '/usr/lib/libartsdsp_st.so',
    '/usr/lib/libesddsp.so',
    '/usr/lib/libmudflap.so',
    '/usr/lib/libmudflapth.so',
    ]
    
compiled_whitelist = []

for w in whitelist:
    cw = [re.compile(w[0]), re.compile(w[1]), re.compile(w[2])]
    compiled_whitelist.append(cw)

    cw = [re.compile(w[1]), re.compile(w[0]), re.compile(w[2])]
    compiled_whitelist.append(cw)

ignore_list_re = re.compile("|".join(ignore_list))

# list of file names to check
files = sys.argv[1:]

# mapping of symbol => [files which define it]
syms = {}

for f in files:
    if ignore_list_re.match(f):
        continue
    
    lines = os.popen("nm -D %s | sed -n '/ [A-TV-Z] [^_]/{/^0* A /d;s/.* //;p;}'" % f)
    for l in lines:
        l = l.strip()
        l = l.strip()
        if l in syms:
            syms[l].append(f)
        else:
            syms[l] = [f]

# mapping of "libraries containing clashes" => [symbols]
clashes = {}
# mapping of "library" => {otherlib => [symbols]}
clashes_bylib = {}

def filter(symbol, libraries):
    for w in compiled_whitelist:
        for l in libraries:
            if w[0].match(l):
                for l2 in libraries:
                    if l != l2 and w[1].match(l2):
                        libraries.remove(l2)

for s in syms.keys():
    filter(s, syms[s])
    if len(syms[s]) > 1:
        libs = " ".join(syms[s])
        if libs in clashes:
            clashes[libs].append(s)
        else:
            clashes[libs] = [s]
        # update the clashes-by-library index
        for l in syms[s]:
            if l in clashes_bylib:
                for l2 in syms[s]:
                    if l2 != l:
                        if l2 in clashes_bylib[l]:
                            clashes_bylib[l][l2].append(s)
                        else:
                            clashes_bylib[l][l2] = [s]
            else:
                clashes_bylib[l] = {}
                for l2 in syms[s]:
                    if l2 != l:
                        clashes_bylib[l][l2] = [s]

def list_clashes(syms):
    if len(syms) > max_emit:
        return syms[0] + " " + syms[1] + \
               " (...%d symbols omitted...) " % (len(syms) - max_emit) + syms[-1]
    else:
        return  " ".join(syms)
    
#for c in clashes.keys():
#    print "Symbol clashes between libraries %s:\n  => %s\n" % (c, list_clashes(clashes[c]))
#

print "\n\nClashes by library:"

for l in clashes_bylib.keys():
    print "\nClashes for %s:" % l
    for l2 in clashes_bylib[l]:
        print "  with %s => %s" % (l2, list_clashes(clashes_bylib[l][l2]))
