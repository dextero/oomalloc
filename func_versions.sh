#!/bin/bash

objdump -T /lib/$(uname -m)-linux-gnu/libc.so.6 | awk '
/ (m|c|re)alloc$| free$/ {
    syms_by_ver[$6] = syms_by_ver[$6] " " $7 ";"
}
END {
    for (ver in syms_by_ver) {
        print ver " {" syms_by_ver[ver] " };"
    }
}
'
