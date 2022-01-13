#!/bin/sh
cat <<__EOF__
%C - ${DESC:?}

Debug options:
    -d <mask>   - Enable DRM debug messages enable mask:
                  0x01 - DRM_UT_CORE messages.
                  0x02 - DRM_UT_DRIVER messages.
                  0x04 - DRM_UT_KMS messages.
                  0x08 - DRM_UT_PRIME messages.
                  0x10 - DRM_UT_ATOMIC messages.
                  0x20 - DRM_UT_INFO messages.
                  0x80 - DRM_UT_TRACE messages.
                  0xFF - all messages.
    -p          - Output messages via fprintf(stdout, ...);
    -s          - Output messages via slogf(...); Default mode.
    -t          - Output messages via trace_logf(...);
    -e          - Enable linux debugfs interface, by default disable it.
    -w <path>   - Sets the path to firmware folder. By default /lib/firmware is used.

Linux kernel modules options:
    -l                   - List all available options for modules (short format) and exit.
    -a                   - List all available options for modules (full format) and exit.
    -o [option]=[value]  - Sets module [option] to [value]. Can be repeated.
    -f [video_string]    - Analog of Linux kernel option "video=[video_string]". Can be
                           repeated multiple times. Video string has following format:

                           <conn>:<xres>x<yres>[M][R][-<bpp>][@<refresh>][i][m][eDd]
                           ---------------------------------------------------------
                           <conn>: Connector, e.g. DVI-I-1, eDP-1
                           <xres> x <yres>: resolution
                           M: compute a CVT mode
                           R: reduced blanking
                           -<bpp>: color depth
                           @<refresh>: refresh rate
                           i: interlaced (non-CVT mode)
                           m: margins
                           e: output forced to on
                           d: output forced to off
                           D: digital output forced to on (e.g. DVI-I connector)
Generic options:
    -v          - Print version information and exit.

Environment variables:
    DRM_CMDLINE - should consist of command line arguments, for example, set of
                  "-o variable=value". It is useful when drm-intel is invoked by
                  Screen directly and there is no more ways to pass specific
                  options for %C. DRM_CMDLINE is parsed before process arguments.

__EOF__
