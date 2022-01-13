dnl ##
dnl ##  acinclude.m4 -- manually provided local Autoconf macros
dnl ##

dnl ##
dnl ##  NAME:
dnl ##    AC_MSG_TITLE -- Display a configuration title
dnl ##
dnl ##  USAGE:
dnl ##    AC_MSG_TITLE(<name>, <version>)
dnl ##

AC_DEFUN([AC_MSG_TITLE],[
    _AS_ECHO([Configuring $1, Version $2])
])

dnl ##
dnl ##  NAME:
dnl ##    AC_MSG_HEADER -- Display a configuration header
dnl ##
dnl ##  USAGE:
dnl ##    AC_MSG_HEADER(<text>)
dnl ##

AC_DEFUN([AC_MSG_HEADER],[
    _AS_ECHO([])
    _AS_ECHO([=== $1 ===])
])

dnl ##
dnl ##  NAME:
dnl ##    AC_MSG_VERBOSE -- Display a message under --verbose
dnl ##
dnl ##  USAGE:
dnl ##    AC_MSG_VERBOSE(<text>)
dnl ##

AC_DEFUN([AC_MSG_VERBOSE], [
    if test ".$verbose" = .yes; then
        _AS_ECHO([$1])
    fi
])

dnl ##
dnl ##  NAME:
dnl ##    RPM_CHECK_LIB -- Check for third-party libraries
dnl ##
dnl ##  COPYRIGHT
dnl ##    Copyright (c) 2007 Ralf S. Engelschall <rse@engelschall.com>
dnl ##
dnl ##  DESCRIPTION:
dnl ##    This is a rather complex Autoconf macro for sophisticated
dnl ##    checking the availability of third-party libraries and
dnl ##    extending the build environment for correctly building
dnl ##    against it.
dnl ##
dnl ##    It especially supports the following particular features:
dnl ##    - is aware of old-style [lib]<libname>-config style scripts
dnl ##    - is aware of new-style pkg-config(1) [lib]<libname>.pc configuration files
dnl ##    - searches under standard sub-directories "include", "lib", etc.
dnl ##    - searches under arbitrary sub-areas of a tree like ".libs", etc.
dnl ##    - searches in standard system locations (implicitly)
dnl ##    - supports searching for function in multiple libraries
dnl ##    - supports searching for multiple headers
dnl ##    - supports multiple search locations (fallbacks!)
dnl ##
dnl ##  USAGE:
dnl ##  - configure.in:
dnl ##    RPM_CHECK_LIB(
dnl ##        <lib-real-name>,        -- [$1] e.g. GNU bzip2
dnl ##        <lib-tag-name>,         -- [$2] e.g. bzip2
dnl ##        <lib-link-name>,        -- [$3] e.g. bz2
dnl ##        <lib-function-name>,    -- [$4] e.g. BZ2_bzlibVersion
dnl ##        <lib-header-filename>,  -- [$5] e.g. bzlib.h
dnl ##        <with-arg-default>[,    -- [$6] e.g. yes,external:internal:none
dnl ##        <internal-subdir>[,     -- [$7] e.g. lib/bzip2:include:src
dnl ##        <action-success>[,      -- [$8] e.g. AC_DEFINE(USE_BZIP2, 1, [...])
dnl ##        <action-failure>        -- [$9] e.g. AC_MSG_ERROR([...])
dnl ##    ]]])
dnl ##
dnl ##  - Makefile.in:
dnl ##    top_srcdir                 = @top_srcdir@
dnl ##    srcdir                     = @srcdir@
dnl ##    WITH_<LIB-TAG-NAME>        = @WITH_<LIB-TAG-NAME>@
dnl ##    WITH_<LIB-TAG-NAME>_SUBDIR = @WITH_<LIB-TAG-NAME>_SUBDIR@
dnl ##    CPPFLAGS                   = @CPPFLAGS@
dnl ##    CFLAGS                     = @CFLAGS@
dnl ##    LDFLAGS                    = @LDFLAGS@
dnl ##    LIBS                       = @LIBS@
dnl ##
dnl ##  - CLI:
dnl ##    $ ./configure \
dnl ##      --with-<lib-tag-name>[=<with-arg>]
dnl ##      [...]
dnl ##
dnl ##  SYNTAX:
dnl ##    <with-arg>           ::= <with-arg-mode> |   <with-arg-location>
dnl ##    <with-arg-default>   ::= <with-arg-mode> "," <with-arg-location>
dnl ##    <with-arg-mode>      ::= "yes" | "no"
dnl ##    <with-arg-location>  ::= <with-arg-location> ":" <with-arg-location>
dnl ##                           | <directory-path>
dnl ##                           | "system"
dnl ##                           | "external"
dnl ##                           | "internal"
dnl ##                           | "none"
dnl ##    <directory-path>     ::= [...] /* valid arg for test(1) option "-d" */
dnl ##
dnl ##

dnl # public API macro
AC_DEFUN([RPM_CHECK_LIB], [
    dnl ##
    dnl ## PROLOG
    dnl ##

    dnl # parse <with-arg-default> into default enable mode and default locations
    m4_define([__rcl_default_enable], [m4_substr([$6], 0, m4_index([$6], [,]))])
    m4_define([__rcl_default_locations], [m4_substr([$6], m4_eval(m4_index([$6], [,]) + 1))])

    dnl # provide defaults
    if test ".${with_$2+set}" != .set; then
        dnl # initialize to default enable mode
        with_$2="__rcl_default_enable"
        AC_MSG_VERBOSE([++ assuming --with-$2=$with_$2])
    fi
    if test ".${with_$2}" = .yes; then
        dnl # map simple "--with-foo=yes" to an enabled default location path
        with_$2="__rcl_default_locations"
        AC_MSG_VERBOSE([++ mapping --with-$2=yes to --with-$2="$with_$2"])
    fi

    dnl ##
    dnl ## HANDLING
    dnl ##

    __rcl_result_hint=""
    __rcl_location_$2=""
    __rcl_location_last=""
    m4_if([$7],,, [
        WITH_[]m4_translit([$2],[a-z],[A-Z])[]_SUBDIR=""
        WITH_[]m4_translit([$2],[a-z],[A-Z])[]_CPPFLAGS=""
        WITH_[]m4_translit([$2],[a-z],[A-Z])[]_LDFLAGS=""
        WITH_[]m4_translit([$2],[a-z],[A-Z])[]_LIBS=""
    ])
    AC_ARG_WITH($2,
        AS_HELP_STRING([--with-$2=ARG], [build with $1 library (__rcl_default_enable) (location path: "__rcl_default_locations")]), [dnl
        if test ".${with_$2}" != .no; then
            dnl # iterate over location path specification for searching purposes
            __rcl_IFS="${IFS}"; IFS=":"
            for __rcl_location in ${with_$2}; do
                IFS="${__rcl_IFS}"
                __rcl_location_last="${__rcl_location}"
                AC_MSG_VERBOSE([++ searching location: ${__rcl_location}])
                if test ".${__rcl_location}" = .none; then
                    dnl # no operation in loop, ignore failure later, too.
                    AC_MSG_VERBOSE([-- no operation])
                m4_if([$7],,, [ elif test ".${__rcl_location}" = .internal; then
                    dnl # optional support for <internal-subdir> feature
                    m4_define([__rcl_subdir],
                              [m4_if(m4_index([$7], [:]), -1, [$7],
                                     m4_substr([$7], 0, m4_index([$7], [:])))])
                    if test -d ${srcdir}/__rcl_subdir; then
                        AC_MSG_VERBOSE([-- activating local sub-directory: __rcl_subdir])
                        if test -f ${srcdir}/__rcl_subdir/configure; then
                            AC_CONFIG_SUBDIRS(__rcl_subdir)
                        fi
                        dnl # NOTICE: an internal copy of the third-party library is a tricky thing
                        dnl # because for the following two major reasons we cannot just extend
                        dnl # CPPFLAGS, LDFLAGS and LIBS in this case:
                        dnl # 1. at _this_ "configure" time at least the library file (libfoo.a)
                        dnl #    is still not existing, so extending LIBS with "-lfoo" would cause
                        dnl #    following Autoconf checks to fail.
                        dnl # 2. even deferring the extension of LIBS doesn't work, because although
                        dnl #    this works around the problem under (1), it will fail if more than
                        dnl #    one internal third-party library exists: LIBS would contains "-lfoo
                        dnl #    -lbar" and if build "foo", "bar" usually still isn't built (or vice
                        dnl #    versa). Hence, the linking of programs (tools, tests, etc) in "foo"
                        dnl #    would fail.
                        dnl # 3. information in at least LDFLAGS and LIBS is usually passed-through
                        dnl #    to applications via xxx-config(1) scripts or pkg-config(1)
                        dnl #    specifications. As the path to the internal copy is usually just a
                        dnl #    temporary path, this will break there, too.
                        dnl # So, internal copies of third-party libraries inherently have to be
                        dnl # handled explicitly by the build environment and for this we can only
                        dnl # provide the necessary information in dedicated per-library variables.
                        WITH_]m4_translit([$2],[a-z],[A-Z])[_SUBDIR="__rcl_subdir"
                        __rcl_location_$2=internal
                        AC_MSG_VERBOSE([++ post-adjustments for --with-$2 (${__rcl_location_$2})])
                        __rcl_dirs_inc=`echo '$7' | sed -e 's/^[[^:]]*://' -e 's/:[[^:]]*[$]//'`
                        __rcl_dirs_lib=`echo '$7' | sed -e 's/^[[^:]]*:[[^:]]*://'`
                        __rcl_srcdir="\[$](top_srcdir)/\[$](WITH_[]m4_translit([$2],[a-z],[A-Z])[]_SUBDIR)"
                        __rcl_builddir="\[$](top_builddir)/\[$](WITH_[]m4_translit([$2],[a-z],[A-Z])[]_SUBDIR)"
                        __rcl_firstlib="m4_if(m4_index([$3], [ ]), -1, [$3], m4_substr([$3], 0, m4_index([$3], [ ])))"
                        if test ".${__rcl_dirs_inc}" != ".$7"; then
                            __rcl_IFS="${IFS}"; IFS=","
                            for __rcl_dir in ${__rcl_dirs_inc}; do
                                IFS="${__rcl_IFS}"
                                test ".${__rcl_dir}" = . && continue
                                AC_MSG_VERBOSE([-- extending WITH_[]m4_translit([$2],[a-z],[A-Z])[]_CPPFLAGS: -I${__rcl_srcdir}/${__rcl_dir}])
                                WITH_[]m4_translit([$2],[a-z],[A-Z])[]_CPPFLAGS="${WITH_[]m4_translit([$2],[a-z],[A-Z])[]_CPPFLAGS} -I${__rcl_srcdir}/${__rcl_dir}"
                                AC_MSG_VERBOSE([-- extending WITH_[]m4_translit([$2],[a-z],[A-Z])[]_CPPFLAGS: -I${__rcl_builddir}/${__rcl_dir}])
                                WITH_[]m4_translit([$2],[a-z],[A-Z])[]_CPPFLAGS="${WITH_[]m4_translit([$2],[a-z],[A-Z])[]_CPPFLAGS} -I${__rcl_builddir}/${__rcl_dir}"
                            done
                            IFS="${__rcl_IFS}"
                        fi
                        AC_MSG_VERBOSE([-- extending WITH_[]m4_translit([$2],[a-z],[A-Z])[]_CPPFLAGS: -I${__rcl_srcdir}])
                        WITH_[]m4_translit([$2],[a-z],[A-Z])[]_CPPFLAGS="${WITH_[]m4_translit([$2],[a-z],[A-Z])[]_CPPFLAGS} -I${__rcl_srcdir}"
                        AC_MSG_VERBOSE([-- extending WITH_[]m4_translit([$2],[a-z],[A-Z])[]_CPPFLAGS: -I${__rcl_builddir}])
                        WITH_[]m4_translit([$2],[a-z],[A-Z])[]_CPPFLAGS="${WITH_[]m4_translit([$2],[a-z],[A-Z])[]_CPPFLAGS} -I${__rcl_builddir}"
                        if test ".${__rcl_dirs_lib}" != ".$7"; then
                            __rcl_IFS="${IFS}"; IFS=","
                            for __rcl_dir in ${__rcl_dirs_lib}; do
                                IFS="${__rcl_IFS}"
                                test ".${__rcl_dir}" = . && continue
                                AC_MSG_VERBOSE([-- extending WITH_[]m4_translit([$2],[a-z],[A-Z])[]_LDFLAGS: -L${__rcl_builddir}/${__rcl_dir}])
                                WITH_[]m4_translit([$2],[a-z],[A-Z])[]_LDFLAGS="${WITH_[]m4_translit([$2],[a-z],[A-Z])[]_LDFLAGS} -L${__rcl_builddir}/${__rcl_dir}"
                            done
                            IFS="${__rcl_IFS}"
                        fi
                        AC_MSG_VERBOSE([-- extending WITH_[]m4_translit([$2],[a-z],[A-Z])[]_LDFLAGS: -L${__rcl_builddir}])
                        WITH_[]m4_translit([$2],[a-z],[A-Z])[]_LDFLAGS="${WITH_[]m4_translit([$2],[a-z],[A-Z])[]_LDFLAGS} -L${__rcl_builddir}"
                        AC_MSG_VERBOSE([-- extending WITH_[]m4_translit([$2],[a-z],[A-Z])[]_LIBS: -l${__rcl_firstlib}])
                        WITH_[]m4_translit([$2],[a-z],[A-Z])[]_LIBS="${WITH_[]m4_translit([$2],[a-z],[A-Z])[]_LIBS} -l${__rcl_firstlib}"
                        __rcl_result_hint="internal: sub-directory __rcl_subdir"
                        break
                    else
                        AC_MSG_VERBOSE([-- skipping not existing local sub-directory: __rcl_subdir])
                    fi
                ])
                elif test ".${__rcl_location}" = .external; then
                    dnl # detection of library in arbitrary external location
                    if (pkg-config --exists $2 || pkg-config --exists lib$2) 2>/dev/null; then
                        dnl # via pkg-config(1) script in PATH
                        m4_define([__rcl_query_pkgconfig], [
                            __rcl_flags=""
                            for __rcl_query in $][2; do
                                if test ".$__rcl_flags" != .; then
                                    __rcl_flags="${__rcl_flags} `($][1 $__rcl_query) 2>/dev/null`"
                                else
                                    __rcl_flags="`($][1 $__rcl_query) 2>/dev/null`"
                                fi
                            done
                            if test ".${__rcl_flags}" != .; then
                                AC_MSG_VERBOSE(-- extending $][3: ${__rcl_flags})
                                $][3="${$][3} ${__rcl_flags}"
                            fi
                        ])
                        if (pkg-config --exists $2) 2>/dev/null; then
                            __rcl_query_pkgconfig([pkg-config $2], [--cflags-only-other], [CFLAGS])
                            __rcl_query_pkgconfig([pkg-config $2], [--cflags-only-I], [CPPFLAGS])
                            __rcl_query_pkgconfig([pkg-config $2], [--libs-only-L --libs-only-other], [LDFLAGS])
                            __rcl_query_pkgconfig([pkg-config $2], [--libs-only-l], [LIBS])
                            __rcl_result_hint="external: via pkg-config $2"
                        else
                            __rcl_query_pkgconfig([pkg-config lib$2], [--cflags-only-other], [CFLAGS])
                            __rcl_query_pkgconfig([pkg-config lib$2], [--cflags-only-I], [CPPFLAGS])
                            __rcl_query_pkgconfig([pkg-config lib$2], [--libs-only-L --libs-only-other], [LDFLAGS])
                            __rcl_query_pkgconfig([pkg-config lib$2], [--libs-only-l], [LIBS])
                            __rcl_result_hint="external: via pkg-config lib$2"
                        fi
                        break
                    elif test ".`($2-config --version; lib$2-config --version) 2>/dev/null`" != .; then
                        dnl # via config script in PATH
                        m4_define([__rcl_query_config], [
                            __rcl_flags="`($][1 $][2) 2>/dev/null`"
                            if test ".${__rcl_flags}" != .; then
                                AC_MSG_VERBOSE(-- extending $][3: ${__rcl_flags})
                                $][3="${$][3} ${__rcl_flags}"
                            fi
                        ])
                        if test ".`($2-config --version) 2>/dev/null`" != .; then
                            __rcl_query_config([$2-config], [--cppflags], [CPPFLAGS])
                            __rcl_query_config([$2-config], [--cflags],   [CFLAGS])
                            __rcl_query_config([$2-config], [--ldflags],  [LDFLAGS])
                            __rcl_query_config([$2-config], [--libs],     [LIBS])
                            __rcl_result_hint="external: via $2-config"
                        else
                            __rcl_query_config([lib$2-config], [--cppflags], [CPPFLAGS])
                            __rcl_query_config([lib$2-config], [--cflags],   [CFLAGS])
                            __rcl_query_config([lib$2-config], [--ldflags],  [LDFLAGS])
                            __rcl_query_config([lib$2-config], [--libs],     [LIBS])
                            __rcl_result_hint="external: via lib$2-config"
                        fi
                        break
                    elif test ".${__rcl_found}" = .no; then
                        dnl # via implicit flags attribution of previous checks or
                        dnl # in standard system locations (usually /usr/include and /usr/lib)
                        __rcl_found_hdr=no
                        __rcl_found_lib=no
                        AC_PREPROC_IFELSE([AC_LANG_SOURCE([@%:@include <$5>])], [ __rcl_found_hdr=yes ])
                        m4_foreach_w([__rcl_lib], [$3], [
                            __rcl_safe_LIBS="${LIBS}"
                            LIBS="-l[]m4_defn([__rcl_lib]) ${LIBS}"
                            AC_LINK_IFELSE([AC_LANG_CALL([], [$4])], [ __rcl_found_lib=yes ])
                            LIBS="${__rcl_safe_LIBS}"
                        ])
                        if test ".${__rcl_found_hdr}:${__rcl_found_lib}" = ".yes:yes"; then
                            __rcl_result_hint="external: implicit or default location"
                            break
                        fi
                    fi
                elif test ".${__rcl_location}" = .system; then
                    dnl # detection of library in external locations controlled
                    dnl # by explicit build flags or in standard system locations
                    dnl # (usually /usr/include and /usr/lib)
                    __rcl_found_hdr=no
                    __rcl_found_lib=no
                    AC_PREPROC_IFELSE([AC_LANG_SOURCE([@%:@include <$5>])], [ __rcl_found_hdr=yes ])
                    m4_foreach_w([__rcl_lib], [$3], [
                        __rcl_safe_LIBS="${LIBS}"
                        LIBS="-l[]m4_defn([__rcl_lib]) ${LIBS}"
                        AC_LINK_IFELSE([AC_LANG_CALL([], [$4])], [ __rcl_found_lib=yes ])
                        LIBS="${__rcl_safe_LIBS}"
                    ])
                    if test ".${__rcl_found_hdr}:${__rcl_found_lib}" = ".yes:yes"; then
                        __rcl_result_hint="system: explicitly controlled or system location"
                        break
                    fi
                elif test -d "${__rcl_location}"; then
                    dnl # detection of library in particular external location
                    __rcl_found=no
                    dnl # via config script
                    for __rcl_dir in ${__rcl_location}/bin ${__rcl_location}; do
                        if test -f "${__rcl_dir}/$2-config" && test ! -f "${__rcl_dir}/$2-config.in"; then
                            if test ".`(${__rcl_dir}/$2-config --version) 2>/dev/null`" != .; then
                                __rcl_query_config([${__rcl_dir}/$2-config], [--cppflags], [CPPFLAGS])
                                __rcl_query_config([${__rcl_dir}/$2-config], [--cflags],   [CFLAGS])
                                __rcl_query_config([${__rcl_dir}/$2-config], [--ldflags],  [LDFLAGS])
                                __rcl_query_config([${__rcl_dir}/$2-config], [--libs],     [LIBS])
                                __rcl_result_hint="external: via ${__rcl_dir}/$2-config"
                                __rcl_found=yes
                                break
                            fi
                        elif test -f "${__rcl_dir}/lib$2-config" && test ! -f "${__rcl_dir}/lib$2-config.in"; then
                            if test ".`(${__rcl_dir}/lib$2-config --version) 2>/dev/null`" != .; then
                                __rcl_query_config([${__rcl_dir}/lib$2-config], [--cppflags], [CPPFLAGS])
                                __rcl_query_config([${__rcl_dir}/lib$2-config], [--cflags],   [CFLAGS])
                                __rcl_query_config([${__rcl_dir}/lib$2-config], [--ldflags],  [LDFLAGS])
                                __rcl_query_config([${__rcl_dir}/lib$2-config], [--libs],     [LIBS])
                                __rcl_result_hint="external: via ${__rcl_dir}/lib$2-config"
                                __rcl_found=yes
                                break
                            fi
                        fi
                    done
                    dnl # via pkg-config(1) script
                    if test ".${__rcl_found}" = .no; then
                        for __rcl_dir in ${__rcl_location}/bin ${__rcl_location}; do
                            if test -f "${__rcl_dir}/pkg-config"; then
                                if (${__rcl_dir}/pkg-config --exists $2) 2>/dev/null; then
                                    __rcl_query_pkgconfig([${__rcl_dir}/pkg-config $2], [--cflags-only-other], [CFLAGS])
                                    __rcl_query_pkgconfig([${__rcl_dir}/pkg-config $2], [--cflags-only-I], [CPPFLAGS])
                                    __rcl_query_pkgconfig([${__rcl_dir}/pkg-config $2], [--libs-only-L --libs-only-other], [LDFLAGS])
                                    __rcl_query_pkgconfig([${__rcl_dir}/pkg-config $2], [--libs-only-l], [LIBS])
                                    __rcl_result_hint="external: via ${__rcl_dir}/pkg-config $2"
                                    __rcl_found=yes
                                    break
                                elif (${__rcl_dir}/pkg-config --exists lib$2) 2>/dev/null; then
                                    __rcl_query_pkgconfig([${__rcl_dir}/pkg-config lib$2], [--cflags-only-other], [CFLAGS])
                                    __rcl_query_pkgconfig([${__rcl_dir}/pkg-config lib$2], [--cflags-only-I], [CPPFLAGS])
                                    __rcl_query_pkgconfig([${__rcl_dir}/pkg-config lib$2], [--libs-only-L --libs-only-other], [LDFLAGS])
                                    __rcl_query_pkgconfig([${__rcl_dir}/pkg-config lib$2], [--libs-only-l], [LIBS])
                                    __rcl_result_hint="external: via ${__rcl_dir}/pkg-config lib$2"
                                    __rcl_found=yes
                                    break
                                fi
                            fi
                        done
                    fi
                    dnl # in standard sub-areas
                    if test ".${__rcl_found}" = .no; then
                        for __rcl_dir in ${__rcl_location}/include/$2 ${__rcl_location}/include ${__rcl_location}; do
                            if test -f "${__rcl_dir}/$5"; then
                                if test ".${__rcl_dir}" != "./usr/include"; then
                                    AC_MSG_VERBOSE([-- extending CPPFLAGS: -I${__rcl_dir}])
                                    CPPFLAGS="${CPPFLAGS} -I${__rcl_dir}"
                                fi
                                __rcl_found=yes
                                break
                            fi
                        done
                        if test ".${__rcl_found}" = .yes; then
                            __rcl_found=no
                            for __rcl_dir in ${__rcl_location}/lib/$2 ${__rcl_location}/lib ${__rcl_location}; do
                                m4_foreach_w([__rcl_lib], [$3], [
                                    if  test -f "${__rcl_dir}/lib[]m4_defn([__rcl_lib]).la" && \
                                        test -d "${__rcl_dir}/.libs"; then
                                        if test ".${__rcl_dir}" != "./usr/lib"; then
                                            AC_MSG_VERBOSE([-- extending LDFLAGS: -L${__rcl_dir}])
                                            AC_MSG_VERBOSE([-- extending LDFLAGS: -L${__rcl_dir}/.libs])
                                            LDFLAGS="${LDFLAGS} -L${__rcl_dir} -L${__rcl_dir}/.libs"
                                        fi
                                        __rcl_found=yes
                                        break
                                    fi
                                    if  test -f "${__rcl_dir}/lib[]m4_defn([__rcl_lib]).a"  || \
                                        test -f "${__rcl_dir}/lib[]m4_defn([__rcl_lib]).so" || \
                                        test -f "${__rcl_dir}/lib[]m4_defn([__rcl_lib]).sl" || \
                                        test -f "${__rcl_dir}/lib[]m4_defn([__rcl_lib]).dylib"; then
                                        if test ".${__rcl_dir}" != "./usr/lib"; then
                                            AC_MSG_VERBOSE([-- extending LDFLAGS: -L${__rcl_dir}])
                                            LDFLAGS="${LDFLAGS} -L${__rcl_dir}"
                                        fi
                                        __rcl_found=yes
                                        break
                                    fi
                                ])
                            done
                        fi
                        if test ".${__rcl_found}" = .yes; then
                            __rcl_result_hint="external: directory ${__rcl_location}"
                        fi
                    fi
                    dnl # in any sub-area
                    if test ".${__rcl_found}" = .no; then
                        for __rcl_file in _ `find ${__rcl_location} -name "$5" -type f -print 2>/dev/null`; do
                            test .${__rcl_file} = ._ && continue
                            __rcl_dir=`echo ${__rcl_file} | sed -e 's;[[^/]]*[$];;' -e 's;\(.\)/[$];\1;'`
                            if test ".${__rcl_dir}" != "./usr/include"; then
                                AC_MSG_VERBOSE([-- extending CPPFLAGS: -I${__rcl_dir}])
                                CPPFLAGS="${CPPFLAGS} -I${__rcl_dir}"
                            fi
                            __rcl_found=yes
                            break
                        done
                        if test ".${__rcl_found}" = .yes; then
                            __rcl_found=no
                            m4_foreach_w([__rcl_lib], [$3], [
                                for __rcl_file in _ `find ${__rcl_location} -name "lib[]m4_defn([__rcl_lib]).*" -type f -print 2>/dev/null | \
                                    egrep '\.(a|so|sl|dylib)$'`; do
                                    test .${__rcl_file} = ._ && continue
                                    __rcl_dir=`echo ${__rcl_file} | sed -e 's;[[^/]]*[$];;' -e 's;\(.\)/[$];\1;'`
                                    if test ".${__rcl_dir}" != "./usr/lib"; then
                                        AC_MSG_VERBOSE([-- extending LDFLAGS: -L${__rcl_dir}])
                                        LDFLAGS="${LDFLAGS} -L${__rcl_dir}"
                                    fi
                                    __rcl_found=yes
                                    break
                                done
                            ])
                        fi
                        if test ".${__rcl_found}" = .yes; then
                            __rcl_result_hint="external: tree ${__rcl_location}"
                        fi
                    fi
                    if test ".${__rcl_found}" = .yes; then
                        break
                    fi
                else
                    AC_MSG_ERROR([Unknown location specification $__rcl_location])
                fi
            done
            IFS="${__rcl_IFS}"

            dnl # check for actual availability of library
            m4_if([$7],,, [ if test ".${__rcl_location_last}" = .internal; then
                dnl # special case: still not existing "internal" library
                dnl # cannot be checked (and usually has not to be checked anyway)
                with_$2=yes
                if test ".${__rcl_location_$2}" != .internal; then
                    AC_MSG_ERROR([unable to find internal $1 library])
                fi
            else ])
                dnl # regular case: use standard Autoconf facilities
                dnl # and try to make sure both header and library is found
                __rcl_found=yes
                dnl # check for C header (in set of optionally multiple possibilities)
                AC_CHECK_HEADERS([$5], [], [ __rcl_found=no ])
                dnl # check for C library (in set of optionally multiple possibilities)
                __rcl_found_lib=no
                m4_foreach_w([__rcl_lib], [$3], [
                    AC_CHECK_LIB(m4_defn([__rcl_lib]), [$4])
                    dnl # manually check for success (do not use third argument to AC_CHECK_LIB
                    dnl # here as this would no longer set the LIBS variable (the default action)
                    test ".${m4_translit(ac_cv_lib_[]m4_defn([__rcl_lib])_$4,[.-,],[___])}" = .yes && __rcl_found_lib=yes
                ])
                test ".${__rcl_found_lib}" = .no && __rcl_found="no"
                dnl # determine final results
                with_$2=${__rcl_found}
            m4_if([$7],,, [ fi ])
            if test ".${with_$2}" = .yes && test ".${__rcl_result_hint}" = .; then
                dnl # was not found explicitly via searching above, but
                dnl # implicitly in standard location or via extended
                dnl # flags from previous searches
                __rcl_result_hint="external: implicitly"
            fi
        fi
    ])

    dnl ##
    dnl ## EPILOG
    dnl ##

    dnl # provide results
    if test ".${with_$2}" = .yes; then
        AC_DEFINE([WITH_]m4_translit([$2],[a-z],[A-Z]), 1, [Define as 1 if building with $1 library])
    fi
    [WITH_]m4_translit([$2],[a-z],[A-Z])="${with_$2}"
    AC_SUBST([WITH_]m4_translit([$2],[a-z],[A-Z]))
    m4_if([$7],,, [
        AC_SUBST([WITH_]m4_translit([$2],[a-z],[A-Z])[_SUBDIR])
        AC_SUBST([WITH_]m4_translit([$2],[a-z],[A-Z])[_CPPFLAGS])
        AC_SUBST([WITH_]m4_translit([$2],[a-z],[A-Z])[_LDFLAGS])
        AC_SUBST([WITH_]m4_translit([$2],[a-z],[A-Z])[_LIBS])
    ])

    dnl # report results
    AC_MSG_CHECKING(whether to build with $1 library)
    __rcl_msg="${with_$2}"
    if test ".${with_$2}" = .yes && test ".${__rcl_result_hint}" != .; then
        __rcl_msg="${__rcl_msg} (${__rcl_result_hint})"
    fi
    AC_MSG_RESULT([${__rcl_msg}])

    dnl # perform actions
    RPM_CHECK_LIB_LOCATION="${__rcl_location_last}"
    if test ".${with_$2}" = .yes; then
        AC_DEFINE([WITH_]m4_translit([$2],[a-z],[A-Z]), 1, [Define as 1 if building with $1 library])
        dnl # support optional <action-success>
        AC_MSG_VERBOSE([++ executing success action])
        m4_if([$8],, :, [$8])
    else
        dnl # support optional <action-failure>
        AC_MSG_VERBOSE([++ executing failure action])
        m4_if([$9],, [
            if  test ".${RPM_CHECK_LIB_LOCATION}" != . && \
                test ".${RPM_CHECK_LIB_LOCATION}" != .none; then
                AC_MSG_ERROR([unable to find usable $1 library])
            fi
        ], [$9])
    fi
    ${as_unset} RPM_CHECK_LIB_LOCATION
])

dnl ##
dnl ##  NAME:
dnl ##    AC_CHECK_VA_COPY -- Check for C99 va_copy
dnl ##
dnl ##  COPYRIGHT
dnl ##    Copyright (c) 2006 Ralf S. Engelschall <rse@engelschall.com>
dnl ##
dnl ##  DESCRIPTION:
dnl ##    This macro checks for C99 va_copy() implementation and
dnl ##    provides fallback implementation if necessary. Notice: this
dnl ##    check is rather complex because first because we really have
dnl ##    to try various possible implementations in sequence and
dnl ##    second, we cannot define a macro in config.h with parameters
dnl ##    directly.
dnl ##
dnl ##  USAGE:
dnl ##    configure.in:
dnl ##      AC_CHECK_VA_COPY
dnl ##    foo.c:
dnl ##      #include "config.h"
dnl ##      [...]
dnl ##      va_copy(d,s)
dnl ##

dnl #   test program for va_copy() implementation
changequote(<<,>>)
m4_define(__va_copy_test, <<[
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#define DO_VA_COPY(d, s) $1
void test(char *str, ...)
{
    va_list ap, ap2;
    int i;
    va_start(ap, str);
    DO_VA_COPY(ap2, ap);
    for (i = 1; i <= 9; i++) {
        int k = (int)va_arg(ap, int);
        if (k != i)
            abort();
    }
    DO_VA_COPY(ap, ap2);
    for (i = 1; i <= 9; i++) {
        int k = (int)va_arg(ap, int);
        if (k != i)
            abort();
    }
    va_end(ap);
}
int main(int argc, char *argv[])
{
    test("test", 1, 2, 3, 4, 5, 6, 7, 8, 9);
    exit(0);
}
]>>)
changequote([,])

dnl #   test driver for va_copy() implementation
m4_define(__va_copy_check, [
    AH_VERBATIM($1,
[/* Predefined possible va_copy() implementation (id: $1) */
#define __VA_COPY_USE_$1(d, s) $2])
    if test ".$ac_cv_va_copy" = .; then
        AC_RUN_IFELSE([AC_LANG_SOURCE(AC_LANG_SOURCE([[__va_copy_test($2)]]))],[ac_cv_va_copy="$1"],[],[])
    fi
])

dnl #   Autoconf check for va_copy() implementation checking
AC_DEFUN([AC_CHECK_VA_COPY],[
  dnl #   provide Autoconf display check message
  AC_MSG_CHECKING(for va_copy() function)
  dnl #   check for various implementations in priorized sequence   
  AC_CACHE_VAL(ac_cv_va_copy, [
    ac_cv_va_copy=""
    dnl #   1. check for standardized C99 macro
    __va_copy_check(C99, [va_copy((d), (s))])
    dnl #   2. check for alternative/deprecated GCC macro
    __va_copy_check(GCM, [VA_COPY((d), (s))])
    dnl #   3. check for internal GCC macro (high-level define)
    __va_copy_check(GCH, [__va_copy((d), (s))])
    dnl #   4. check for internal GCC macro (built-in function)
    __va_copy_check(GCB, [__builtin_va_copy((d), (s))])
    dnl #   5. check for assignment approach (assuming va_list is a struct)
    __va_copy_check(ASS, [do { (d) = (s); } while (0)])
    dnl #   6. check for assignment approach (assuming va_list is a pointer)
    __va_copy_check(ASP, [do { *(d) = *(s); } while (0)])
    dnl #   7. check for memory copying approach (assuming va_list is a struct)
    __va_copy_check(CPS, [memcpy((void *)&(d), (void *)&(s), sizeof((s)))])
    dnl #   8. check for memory copying approach (assuming va_list is a pointer)
    __va_copy_check(CPP, [memcpy((void *)(d), (void *)(s), sizeof(*(s)))])
    if test ".$ac_cv_va_copy" = .; then
        AC_MSG_ERROR([no working implementation found])
    fi
  ])
  dnl #   optionally activate the fallback implementation
  if test ".$ac_cv_va_copy" = ".C99"; then
      AC_DEFINE(HAVE_VA_COPY, 1, [Define if va_copy() macro exists (and no fallback implementation is required)])
  fi
  dnl #   declare which fallback implementation to actually use
  AC_DEFINE_UNQUOTED([__VA_COPY_USE], [__VA_COPY_USE_$ac_cv_va_copy],
      [Define to id of used va_copy() implementation])
  dnl #   provide activation hook for fallback implementation
  AH_VERBATIM([__VA_COPY_ACTIVATION],
[/* Optional va_copy() implementation activation */
#ifndef HAVE_VA_COPY
#define va_copy(d, s) __VA_COPY_USE(d, s)
#endif
])
  dnl #   provide Autoconf display result message
  if test ".$ac_cv_va_copy" = ".C99"; then
      AC_MSG_RESULT([yes])
  else
      AC_MSG_RESULT([no (using fallback implementation)])
  fi
])

dnl ##
dnl ##  NAME:
dnl ##    AC_CHECK_STATFS -- Check for "struct statfs" and friends
dnl ##

AC_DEFUN([AC_CHECK_STATFS], [
    dnl # 1. search for struct statfs
    AC_MSG_CHECKING(for struct statfs)
    found_struct_statfs=no
    if test ".$found_struct_statfs" = .no; then
        dnl # Solaris 2.6+ wants to use "statvfs"
        AC_COMPILE_IFELSE([
            AC_LANG_PROGRAM([[
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#include <sys/statvfs.h>
            ]], [[
                struct statvfs sfs;
            ]])
        ], [
            AC_MSG_RESULT(in <sys/statvfs.h>)
            AC_DEFINE(STATFS_IN_SYS_STATVFS, 1, [statfs in <sys/statvfs.h> (for Solaris 2.6+ systems)])
            found_struct_statfs=yes
        ], [])
    fi
    if test ".$found_struct_statfs" = .no; then
        dnl # Linux: first try including <sys/vfs.h>
        AC_COMPILE_IFELSE([
            AC_LANG_PROGRAM([[
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#include <sys/vfs.h>
            ]], [[
                struct statfs sfs;
            ]])
        ],[
            AC_MSG_RESULT(in <sys/vfs.h>)
            AC_DEFINE(STATFS_IN_SYS_VFS, 1, [statfs in <sys/vfs.h> (for Linux systems)])
            found_struct_statfs=yes
        ], [])
    fi
    if test ".$found_struct_statfs" = .no; then
        dnl # ...next try including <sys/mount.h>
        AC_COMPILE_IFELSE([
            AC_LANG_PROGRAM([[
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#include <sys/mount.h>
            ]], [[
                struct statfs sfs;
            ]])
        ],[
            AC_MSG_RESULT(in <sys/mount.h>)
            AC_DEFINE(STATFS_IN_SYS_MOUNT, 1, [statfs in <sys/mount.h> (for Digital Unix 4.0D systems)])
            found_struct_statfs=yes
        ], [])
    fi
    if test ".$found_struct_statfs" = Xno; then
        dnl ...still no joy. Try <sys/statfs.h>
        AC_COMPILE_IFELSE([
            AC_LANG_PROGRAM([[
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#include <sys/statfs.h>
            ]], [[
                struct statfs sfs;
            ]])
        ], [
            AC_MSG_RESULT(in <sys/statfs.h>)
            AC_DEFINE(STATFS_IN_SYS_STATFS, 1, [statfs in <sys/statfs.h> (for IRIX 6.4 systems)])
            found_struct_statfs=yes
        ], [])
    fi
    if test ".$found_struct_statfs" = .no; then
        dnl # ...no luck. Warn the user of impending doom.
        AC_MSG_RESULT([not found])
        AC_MSG_WARN([struct statfs not found])
    fi

    dnl # 2. search for f_bavail member of struct statfs
    if test ".$found_struct_statfs" = .yes; then
        AC_MSG_CHECKING(for f_bavail member in struct statfs)
        AC_COMPILE_IFELSE([
            AC_LANG_PROGRAM([[
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if STATFS_IN_SYS_STATVFS
#include <sys/statvfs.h>
  typedef struct statvfs STATFS_t;
#else
  typedef struct statfs STATFS_t;
#if STATFS_IN_SYS_VFS
#include <sys/vfs.h>
#elif STATFS_IN_SYS_MOUNT
#include <sys/mouht.h>
#elif STATFS_IN_SYS_STATFS
#include <sys/statfs.h>
#endif
#endif 
            ]], [[
                STATFS_t sfs;
                sfs.f_bavail = 0;
            ]])
        ],[
            AC_MSG_RESULT(yes)
            AC_DEFINE(STATFS_HAS_F_BAVAIL, 1, [Define if struct statfs has the f_bavail member])
        ],[
            AC_MSG_RESULT(no)
        ])
    fi

    dnl # 3. check to see if we have the 4-argument variant of statfs(2)
    if test ".$found_struct_statfs" = .yes; then
        AC_MSG_CHECKING([if statfs(2) requires 4 arguments])
        AC_RUN_IFELSE([
            AC_LANG_SOURCE([[
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef STATFS_IN_SYS_VFS
#include <sys/vfs.h>
#elif STATFS_IN_SYS_MOUNT
#include <sys/mouht.h>
#elif STATFS_IN_SYS_STATFS
#include <sys/statfs.h>
#endif
                main() {
                    struct statfs sfs;
                    exit (statfs(".", &sfs, sizeof(sfs), 0));
                }
            ]])
        ], [
            AC_MSG_RESULT(yes)
            AC_DEFINE(STAT_STATFS4, 1, [Define if statfs() takes 4 arguments])
        ], [
            AC_MSG_RESULT(no)
        ], [
            AC_MSG_RESULT(no)
        ])
    fi
])
dnl ##
dnl ##  NAME:
dnl ##    AC_CPP_FUNC 
dnl ##
dnl ## Checks to see if ISO C99 CPP variable __func__ works.
dnl ## If not, perhaps __FUNCTION__ works instead.
dnl ## If not, we'll just define __func__ to "".
dnl ## 
dnl ## Needed for the test support code; this was found at
dnl ## http://lists.gnu.org/archive/html/bug-autoconf/2002-07/msg00028.html
dnl
AC_DEFUN([AC_CPP_FUNC],
[AC_REQUIRE([AC_PROG_CC_STDC])dnl
AC_CACHE_CHECK([for an ANSI C99-conforming __func__], ac_cv_cpp_func,
[AC_COMPILE_IFELSE([AC_LANG_PROGRAM([],
[[char *foo = __func__;]])],
  [ac_cv_cpp_func=yes], 
  [AC_COMPILE_IFELSE([AC_LANG_PROGRAM([],
[[char *foo = __FUNCTION__;]])],
  [ac_cv_cpp_func=__FUNCTION__], 
  [ac_cv_cpp_func=no])])])
if test $ac_cv_cpp_func = __FUNCTION__; then
  AC_DEFINE(__func__,__FUNCTION__,
            [Define to __FUNCTION__ or "" if `__func__' does not conform to 
ANSI C.])
elif test $ac_cv_cpp_func = no; then
  AC_DEFINE(__func__,"",
            [Define to __FUNCTION__ or "" if `__func__' does not conform to 
ANSI C.])
fi
])# AC_CPP_FUNC

dnl ====> ac_check_class.m4
dnl @synopsis AC_CHECK_CLASS
dnl
dnl AC_CHECK_CLASS tests the existence of a given Java class, either in
dnl a jar or in a '.class' file.
dnl
dnl *Warning*: its success or failure can depend on a proper setting of the
dnl CLASSPATH env. variable.
dnl
dnl Note: This is part of the set of autoconf M4 macros for Java programs.
dnl It is VERY IMPORTANT that you download the whole set, some
dnl macros depend on other. Unfortunately, the autoconf archive does not
dnl support the concept of set of macros, so I had to break it for
dnl submission.
dnl The general documentation, as well as the sample configure.in, is
dnl included in the AC_PROG_JAVA macro.
dnl
dnl @author Stephane Bortzmeyer <bortzmeyer@pasteur.fr>
dnl @version $Id: acinclude.m4,v 2.31.2.2 2014/08/08 03:06:38 jbj Exp $
dnl
AC_DEFUN([AC_CHECK_CLASS],[
AC_REQUIRE([AC_PROG_JAVA])
ac_var_name=`echo $1 | sed 's/\./_/g'`
dnl Normaly I'd use a AC_CACHE_CHECK here but since the variable name is
dnl dynamic I need an extra level of extraction
AC_MSG_CHECKING([for $1 class])
AC_CACHE_VAL(ac_cv_class_$ac_var_name, [
if test x$ac_cv_prog_uudecode_base64 = xyes; then
dnl /**
dnl  * Test.java: used to test dynamicaly if a class exists.
dnl  */
dnl public class Test
dnl {
dnl
dnl public static void
dnl main( String[] argv )
dnl {
dnl     Class lib;
dnl     if (argv.length < 1)
dnl      {
dnl             System.err.println ("Missing argument");
dnl             System.exit (77);
dnl      }
dnl     try
dnl      {
dnl             lib = Class.forName (argv[0]);
dnl      }
dnl     catch (ClassNotFoundException e)
dnl      {
dnl             System.exit (1);
dnl      }
dnl     lib = null;
dnl     System.exit (0);
dnl }
dnl
dnl }
cat << \EOF > Test.uue
begin-base64 644 Test.class
yv66vgADAC0AKQcAAgEABFRlc3QHAAQBABBqYXZhL2xhbmcvT2JqZWN0AQAE
bWFpbgEAFihbTGphdmEvbGFuZy9TdHJpbmc7KVYBAARDb2RlAQAPTGluZU51
bWJlclRhYmxlDAAKAAsBAANlcnIBABVMamF2YS9pby9QcmludFN0cmVhbTsJ
AA0ACQcADgEAEGphdmEvbGFuZy9TeXN0ZW0IABABABBNaXNzaW5nIGFyZ3Vt
ZW50DAASABMBAAdwcmludGxuAQAVKExqYXZhL2xhbmcvU3RyaW5nOylWCgAV
ABEHABYBABNqYXZhL2lvL1ByaW50U3RyZWFtDAAYABkBAARleGl0AQAEKEkp
VgoADQAXDAAcAB0BAAdmb3JOYW1lAQAlKExqYXZhL2xhbmcvU3RyaW5nOylM
amF2YS9sYW5nL0NsYXNzOwoAHwAbBwAgAQAPamF2YS9sYW5nL0NsYXNzBwAi
AQAgamF2YS9sYW5nL0NsYXNzTm90Rm91bmRFeGNlcHRpb24BAAY8aW5pdD4B
AAMoKVYMACMAJAoAAwAlAQAKU291cmNlRmlsZQEACVRlc3QuamF2YQAhAAEA
AwAAAAAAAgAJAAUABgABAAcAAABtAAMAAwAAACkqvgSiABCyAAwSD7YAFBBN
uAAaKgMyuAAeTKcACE0EuAAaAUwDuAAasQABABMAGgAdACEAAQAIAAAAKgAK
AAAACgAAAAsABgANAA4ADgATABAAEwASAB4AFgAiABgAJAAZACgAGgABACMA
JAABAAcAAAAhAAEAAQAAAAUqtwAmsQAAAAEACAAAAAoAAgAAAAQABAAEAAEA
JwAAAAIAKA==
====
EOF
                if uudecode$EXEEXT Test.uue; then
                        :
                else
                        echo "configure: __oline__: uudecode had trouble decoding base 64 file 'Test.uue'" >&AC_FD_CC
                        echo "configure: failed file was:" >&AC_FD_CC
                        cat Test.uue >&AC_FD_CC
                        ac_cv_prog_uudecode_base64=no
                fi
        rm -f Test.uue
        if AC_TRY_COMMAND($JAVA $JAVAFLAGS Test $1) >/dev/null 2>&1; then
                eval "ac_cv_class_$ac_var_name=yes"
        else
                eval "ac_cv_class_$ac_var_name=no"
        fi
        rm -f Test.class
else
        AC_TRY_COMPILE_JAVA([$1], , [eval "ac_cv_class_$ac_var_name=yes"],
                [eval "ac_cv_class_$ac_var_name=no"])
fi
eval "ac_var_val=$`eval echo ac_cv_class_$ac_var_name`"
eval "HAVE_$ac_var_name=$`echo ac_cv_class_$ac_var_val`"
HAVE_LAST_CLASS=$ac_var_val
if test x$ac_var_val = xyes; then
        ifelse([$2], , :, [$2])
else
        ifelse([$3], , :, [$3])
fi
])
dnl for some reason the above statment didn't fall though here?
dnl do scripts have variable scoping?
eval "ac_var_val=$`eval echo ac_cv_class_$ac_var_name`"
AC_MSG_RESULT($ac_var_val)
])

dnl ====> ac_check_classpath.m4
dnl @synopsis AC_CHECK_CLASSPATH
dnl
dnl AC_CHECK_CLASSPATH just displays the CLASSPATH, for the edification
dnl of the user.
dnl
dnl Note: This is part of the set of autoconf M4 macros for Java programs.
dnl It is VERY IMPORTANT that you download the whole set, some
dnl macros depend on other. Unfortunately, the autoconf archive does not
dnl support the concept of set of macros, so I had to break it for
dnl submission.
dnl The general documentation, as well as the sample configure.in, is
dnl included in the AC_PROG_JAVA macro.
dnl
dnl @author Stephane Bortzmeyer <bortzmeyer@pasteur.fr>
dnl @version $Id: acinclude.m4,v 2.31.2.2 2014/08/08 03:06:38 jbj Exp $
dnl
AC_DEFUN([AC_CHECK_CLASSPATH],[
if test "x$CLASSPATH" = x; then
        echo "You have no CLASSPATH, I hope it is good"
else
        echo "You have CLASSPATH $CLASSPATH, hope it is correct"
fi
])

dnl ====> ac_check_junit.m4
dnl @synopsis AC_CHECK_JUNIT
dnl
dnl AC_CHECK_JUNIT tests the availability of the Junit testing
dnl framework, and set some variables for conditional compilation
dnl of the test suite by automake.
dnl
dnl If available, JUNIT is set to a command launching the text
dnl based user interface of Junit, @JAVA_JUNIT@ is set to $JAVA_JUNIT
dnl and @TESTS_JUNIT@ is set to $TESTS_JUNIT, otherwise they are set
dnl to empty values.
dnl
dnl You can use these variables in your Makefile.am file like this :
dnl
dnl  # Some of the following classes are built only if junit is available
dnl  JAVA_JUNIT  = Class1Test.java Class2Test.java AllJunitTests.java
dnl
dnl  noinst_JAVA = Example1.java Example2.java @JAVA_JUNIT@
dnl
dnl  EXTRA_JAVA  = $(JAVA_JUNIT)
dnl
dnl  TESTS_JUNIT = AllJunitTests
dnl
dnl  TESTS       = StandaloneTest1 StandaloneTest2 @TESTS_JUNIT@
dnl
dnl  EXTRA_TESTS = $(TESTS_JUNIT)
dnl
dnl  AllJunitTests :
dnl     echo "#! /bin/sh" > $@
dnl     echo "exec @JUNIT@ my.package.name.AllJunitTests" >> $@
dnl     chmod +x $@
dnl
dnl @author Luc Maisonobe
dnl @version $Id: acinclude.m4,v 2.31.2.2 2014/08/08 03:06:38 jbj Exp $
dnl
AC_DEFUN([AC_CHECK_JUNIT],[
AC_CACHE_VAL(ac_cv_prog_JUNIT,[
AC_CHECK_CLASS(junit.textui.TestRunner)
if test x"`eval 'echo $ac_cv_class_junit_textui_TestRunner'`" != xno ; then
  ac_cv_prog_JUNIT='$(CLASSPATH_ENV) $(JAVA) $(JAVAFLAGS) junit.textui.TestRunner'
fi])
AC_MSG_CHECKING([for junit])
if test x"`eval 'echo $ac_cv_prog_JUNIT'`" != x ; then
  JUNIT="$ac_cv_prog_JUNIT"
  JAVA_JUNIT='$(JAVA_JUNIT)'
  TESTS_JUNIT='$(TESTS_JUNIT)'
else
  JUNIT=
  JAVA_JUNIT=
  TESTS_JUNIT=
fi
AC_MSG_RESULT($JAVA_JUNIT)
AC_SUBST(JUNIT)
AC_SUBST(JAVA_JUNIT)
AC_SUBST(TESTS_JUNIT)])

dnl ====> ac_check_rqrd_class.m4
dnl @synopsis AC_CHECK_RQRD_CLASS
dnl
dnl AC_CHECK_RQRD_CLASS tests the existence of a given Java class, either in
dnl a jar or in a '.class' file and fails if it doesn't exist.
dnl Its success or failure can depend on a proper setting of the
dnl CLASSPATH env. variable.
dnl
dnl Note: This is part of the set of autoconf M4 macros for Java programs.
dnl It is VERY IMPORTANT that you download the whole set, some
dnl macros depend on other. Unfortunately, the autoconf archive does not
dnl support the concept of set of macros, so I had to break it for
dnl submission.
dnl The general documentation, as well as the sample configure.in, is
dnl included in the AC_PROG_JAVA macro.
dnl
dnl @author Stephane Bortzmeyer <bortzmeyer@pasteur.fr>
dnl @version $Id: acinclude.m4,v 2.31.2.2 2014/08/08 03:06:38 jbj Exp $
dnl

AC_DEFUN([AC_CHECK_RQRD_CLASS],[
CLASS=`echo $1|sed 's/\./_/g'`
AC_CHECK_CLASS($1)
if test "$HAVE_LAST_CLASS" = "no"; then
        AC_MSG_ERROR([Required class $1 missing, exiting.])
fi
])

dnl ====> ac_java_options.m4
dnl @synopsis AC_JAVA_OPTIONS
dnl
dnl AC_JAVA_OPTIONS adds configure command line options used for Java m4
dnl macros. This Macro is optional.
dnl
dnl Note: This is part of the set of autoconf M4 macros for Java programs.
dnl It is VERY IMPORTANT that you download the whole set, some
dnl macros depend on other. Unfortunately, the autoconf archive does not
dnl support the concept of set of macros, so I had to break it for
dnl submission.
dnl The general documentation, as well as the sample configure.in, is
dnl included in the AC_PROG_JAVA macro.
dnl
dnl @author Devin Weaver <ktohg@tritarget.com>
dnl @version $Id: acinclude.m4,v 2.31.2.2 2014/08/08 03:06:38 jbj Exp $
dnl
AC_DEFUN([AC_JAVA_OPTIONS],[
AC_ARG_WITH(java-prefix,
                        [  --with-java-prefix=PFX  prefix where Java runtime is installed (optional)])
AC_ARG_WITH(javac-flags,
                        [  --with-javac-flags=FLAGS flags to pass to the Java compiler (optional)])
AC_ARG_WITH(java-flags,
                        [  --with-java-flags=FLAGS flags to pass to the Java VM (optional)])
JAVAPREFIX=$with_java_prefix
JAVACFLAGS=$with_javac_flags
JAVAFLAGS=$with_java_flags
AC_SUBST(JAVAPREFIX)dnl
AC_SUBST(JAVACFLAGS)dnl
AC_SUBST(JAVAFLAGS)dnl
AC_SUBST(JAVA)dnl
AC_SUBST(JAVAC)dnl
])

dnl ====> ac_jni_include_dirs.m4
dnl @synopsis AC_JNI_INCLUDE_DIR
dnl
dnl AC_JNI_INCLUDE_DIR finds include directories needed
dnl for compiling programs using the JNI interface.
dnl
dnl JNI include directories are usually in the java distribution
dnl This is deduced from the value of JAVAC.  When this macro
dnl completes, a list of directories is left in the variable
dnl JNI_INCLUDE_DIRS.
dnl
dnl Example usage follows:
dnl
dnl 	AC_JNI_INCLUDE_DIR
dnl
dnl	for JNI_INCLUDE_DIR in $JNI_INCLUDE_DIRS
dnl	do
dnl		CPPFLAGS="$CPPFLAGS -I$JNI_INCLUDE_DIR"
dnl	done
dnl
dnl If you want to force a specific compiler:
dnl
dnl - at the configure.in level, set JAVAC=yourcompiler before calling
dnl AC_JNI_INCLUDE_DIR
dnl
dnl - at the configure level, setenv JAVAC
dnl
dnl Note: This macro can work with the autoconf M4 macros for Java programs.
dnl This particular macro is not part of the original set of macros.
dnl
dnl @author Don Anderson
dnl @version $Id: acinclude.m4,v 2.31.2.2 2014/08/08 03:06:38 jbj Exp $
dnl
AC_DEFUN([AC_JNI_INCLUDE_DIR],[

JNI_INCLUDE_DIRS=""

test "x$JAVAC" = x && AC_MSG_ERROR(['$JAVAC' undefined])
AC_PATH_PROG(_ACJNI_JAVAC, $JAVAC, $JAVAC)
test ! -x "$_ACJNI_JAVAC" && AC_MSG_ERROR([$JAVAC could not be found in path])
AC_MSG_CHECKING(absolute path of $JAVAC)
case "$_ACJNI_JAVAC" in
/*)	AC_MSG_RESULT($_ACJNI_JAVAC);;
*)	AC_MSG_ERROR([$_ACJNI_JAVAC is not an absolute path name]);;
esac

_ACJNI_FOLLOW_SYMLINKS("$_ACJNI_JAVAC")
_JTOPDIR=`echo "$_ACJNI_FOLLOWED" | sed -e 's://*:/:g' -e 's:/[[^/]]*$::'`
case "$host_os" in
	darwin*)	_JTOPDIR=`echo "$_JTOPDIR" | sed -e 's:/[[^/]]*$::'`
			_JINC="$_JTOPDIR/Headers";;
	*)		_JINC="$_JTOPDIR/include";;
esac

# If we find jni.h in /usr/include, then it's not a java-only tree, so
# don't add /usr/include or subdirectories to the list of includes.
# An extra -I/usr/include can foul things up with newer gcc's.
#
# If we don't find jni.h, just keep going.  Hopefully javac knows where
# to find its include files, even if we can't.
if test -r "$_JINC/jni.h"; then
	if test "$_JINC" != "/usr/include"; then
		JNI_INCLUDE_DIRS="$JNI_INCLUDE_DIRS $_JINC"
	fi
else
	_JTOPDIR=`echo "$_JTOPDIR" | sed -e 's:/[[^/]]*$::'`
	if test -r "$_JTOPDIR/include/jni.h"; then
		if test "$_JTOPDIR" != "/usr"; then
			JNI_INCLUDE_DIRS="$JNI_INCLUDE_DIRS $_JTOPDIR/include"
		fi
	fi
fi

# get the likely subdirectories for system specific java includes
if test "$_JTOPDIR" != "/usr"; then
	case "$host_os" in
	aix*)		_JNI_INC_SUBDIRS="aix";;
	bsdi*)		_JNI_INC_SUBDIRS="bsdos";;
	cygwin*)	_JNI_INC_SUBDIRS="win32";;
	freebsd*)	_JNI_INC_SUBDIRS="freebsd";;
	hp*)		_JNI_INC_SUBDIRS="hp-ux";;
	linux*)		_JNI_INC_SUBDIRS="linux genunix";;
	osf*)		_JNI_INC_SUBDIRS="alpha";;
	solaris*)	_JNI_INC_SUBDIRS="solaris";;
	*)		_JNI_INC_SUBDIRS="genunix";;
	esac
fi

# add any subdirectories that are present
for _JINCSUBDIR in $_JNI_INC_SUBDIRS
do
	if test -d "$_JTOPDIR/include/$_JINCSUBDIR"; then
		JNI_INCLUDE_DIRS="$JNI_INCLUDE_DIRS $_JTOPDIR/include/$_JINCSUBDIR"
	fi
done
])

# _ACJNI_FOLLOW_SYMLINKS <path>
# Follows symbolic links on <path>,
# finally setting variable _ACJNI_FOLLOWED
# --------------------
AC_DEFUN([_ACJNI_FOLLOW_SYMLINKS],[
# find the include directory relative to the javac executable
_cur="$1"
while ls -ld "$_cur" 2>/dev/null | grep " -> " >/dev/null; do
	AC_MSG_CHECKING(symlink for $_cur)
	_slink=`ls -ld "$_cur" | sed 's/.* -> //'`
	case "$_slink" in
	/*) _cur="$_slink";;
	# 'X' avoids triggering unwanted echo options.
	*) _cur=`echo "X$_cur" | sed -e 's/^X//' -e 's:[[^/]]*$::'`"$_slink";;
	esac
	AC_MSG_RESULT($_cur)
done
_ACJNI_FOLLOWED="$_cur"
])# _ACJNI

dnl ====> ac_prog_jar.m4
dnl @synopsis AC_PROG_JAR
dnl
dnl AC_PROG_JAR tests for an existing jar program. It uses the environment
dnl variable JAR then tests in sequence various common jar programs.
dnl
dnl If you want to force a specific compiler:
dnl
dnl - at the configure.in level, set JAR=yourcompiler before calling
dnl AC_PROG_JAR
dnl
dnl - at the configure level, setenv JAR
dnl
dnl You can use the JAR variable in your Makefile.in, with @JAR@.
dnl
dnl Note: This macro depends on the autoconf M4 macros for Java programs.
dnl It is VERY IMPORTANT that you download that whole set, some
dnl macros depend on other. Unfortunately, the autoconf archive does not
dnl support the concept of set of macros, so I had to break it for
dnl submission.
dnl
dnl The general documentation of those macros, as well as the sample
dnl configure.in, is included in the AC_PROG_JAVA macro.
dnl
dnl @author Egon Willighagen <egonw@sci.kun.nl>
dnl @version $Id: acinclude.m4,v 2.31.2.2 2014/08/08 03:06:38 jbj Exp $
dnl
AC_DEFUN([AC_PROG_JAR],[
AC_REQUIRE([AC_EXEEXT])dnl
if test "x$JAVAPREFIX" = x; then
        test "x$JAR" = x && AC_CHECK_PROGS(JAR, jar$EXEEXT)
else
        test "x$JAR" = x && AC_CHECK_PROGS(JAR, jar, $JAVAPREFIX)
fi
test "x$JAR" = x && AC_MSG_ERROR([no acceptable jar program found in \$PATH])
AC_PROVIDE([$0])dnl
])

dnl ====> ac_prog_javac.m4
dnl @synopsis AC_PROG_JAVAC
dnl
dnl AC_PROG_JAVAC tests an existing Java compiler. It uses the environment
dnl variable JAVAC then tests in sequence various common Java compilers. For
dnl political reasons, it starts with the free ones.
dnl
dnl If you want to force a specific compiler:
dnl
dnl - at the configure.in level, set JAVAC=yourcompiler before calling
dnl AC_PROG_JAVAC
dnl
dnl - at the configure level, setenv JAVAC
dnl
dnl You can use the JAVAC variable in your Makefile.in, with @JAVAC@.
dnl
dnl *Warning*: its success or failure can depend on a proper setting of the
dnl CLASSPATH env. variable.
dnl
dnl TODO: allow to exclude compilers (rationale: most Java programs cannot compile
dnl with some compilers like guavac).
dnl
dnl Note: This is part of the set of autoconf M4 macros for Java programs.
dnl It is VERY IMPORTANT that you download the whole set, some
dnl macros depend on other. Unfortunately, the autoconf archive does not
dnl support the concept of set of macros, so I had to break it for
dnl submission.
dnl The general documentation, as well as the sample configure.in, is
dnl included in the AC_PROG_JAVA macro.
dnl
dnl @author Stephane Bortzmeyer <bortzmeyer@pasteur.fr>
dnl @version $Id: acinclude.m4,v 2.31.2.2 2014/08/08 03:06:38 jbj Exp $
dnl
AC_DEFUN([AC_PROG_JAVAC],[
AC_REQUIRE([AC_EXEEXT])dnl
if test "x$JAVAPREFIX" = x; then
        test "x$JAVAC" = x && AC_CHECK_PROGS(JAVAC, javac$EXEEXT "gcj$EXEEXT -C" guavac$EXEEXT jikes$EXEEXT)
else
        test "x$JAVAC" = x && AC_CHECK_PROGS(JAVAC, javac$EXEEXT "gcj$EXEEXT -C" guavac$EXEEXT jikes$EXEEXT, $JAVAPREFIX)
fi
test "x$JAVAC" = x && AC_MSG_ERROR([no acceptable Java compiler found in \$PATH])
AC_PROG_JAVAC_WORKS
AC_PROVIDE([$0])dnl
])

dnl ====> ac_prog_javac_works.m4
dnl @synopsis AC_PROG_JAVAC_WORKS
dnl
dnl Internal use ONLY.
dnl
dnl Note: This is part of the set of autoconf M4 macros for Java programs.
dnl It is VERY IMPORTANT that you download the whole set, some
dnl macros depend on other. Unfortunately, the autoconf archive does not
dnl support the concept of set of macros, so I had to break it for
dnl submission.
dnl The general documentation, as well as the sample configure.in, is
dnl included in the AC_PROG_JAVA macro.
dnl
dnl @author Stephane Bortzmeyer <bortzmeyer@pasteur.fr>
dnl @version $Id: acinclude.m4,v 2.31.2.2 2014/08/08 03:06:38 jbj Exp $
dnl
AC_DEFUN([AC_PROG_JAVAC_WORKS],[
AC_CACHE_CHECK([if $JAVAC works], ac_cv_prog_javac_works, [
JAVA_TEST=Test.java
CLASS_TEST=Test.class
cat << \EOF > $JAVA_TEST
/* [#]line __oline__ "configure" */
public class Test {
}
EOF
if AC_TRY_COMMAND($JAVAC $JAVACFLAGS $JAVA_TEST) >/dev/null 2>&1; then
  ac_cv_prog_javac_works=yes
else
  AC_MSG_ERROR([The Java compiler $JAVAC failed (see config.log, check the CLASSPATH?)])
  echo "configure: failed program was:" >&AC_FD_CC
  cat $JAVA_TEST >&AC_FD_CC
fi
rm -f $JAVA_TEST $CLASS_TEST
])
AC_PROVIDE([$0])dnl
])

dnl ====> ac_prog_javadoc.m4
dnl @synopsis AC_PROG_JAVADOC
dnl
dnl AC_PROG_JAVADOC tests for an existing javadoc generator. It uses the environment
dnl variable JAVADOC then tests in sequence various common javadoc generator.
dnl
dnl If you want to force a specific compiler:
dnl
dnl - at the configure.in level, set JAVADOC=yourgenerator before calling
dnl AC_PROG_JAVADOC
dnl
dnl - at the configure level, setenv JAVADOC
dnl
dnl You can use the JAVADOC variable in your Makefile.in, with @JAVADOC@.
dnl
dnl Note: This macro depends on the autoconf M4 macros for Java programs.
dnl It is VERY IMPORTANT that you download that whole set, some
dnl macros depend on other. Unfortunately, the autoconf archive does not
dnl support the concept of set of macros, so I had to break it for
dnl submission.
dnl
dnl The general documentation of those macros, as well as the sample
dnl configure.in, is included in the AC_PROG_JAVA macro.
dnl
dnl @author Egon Willighagen <egonw@sci.kun.nl>
dnl @version $Id: acinclude.m4,v 2.31.2.2 2014/08/08 03:06:38 jbj Exp $
dnl
AC_DEFUN([AC_PROG_JAVADOC],[
AC_REQUIRE([AC_EXEEXT])dnl
if test "x$JAVAPREFIX" = x; then
        test "x$JAVADOC" = x && AC_CHECK_PROGS(JAVADOC, javadoc$EXEEXT)
else
        test "x$JAVADOC" = x && AC_CHECK_PROGS(JAVADOC, javadoc, $JAVAPREFIX)
fi
test "x$JAVADOC" = x && AC_MSG_ERROR([no acceptable javadoc generator found in \$PATH])
AC_PROVIDE([$0])dnl
])


dnl ====> ac_prog_javah.m4
dnl @synopsis AC_PROG_JAVAH
dnl
dnl AC_PROG_JAVAH tests the availability of the javah header generator
dnl and looks for the jni.h header file. If available, JAVAH is set to
dnl the full path of javah and CPPFLAGS is updated accordingly.
dnl
dnl @author Luc Maisonobe
dnl @version $Id: acinclude.m4,v 2.31.2.2 2014/08/08 03:06:38 jbj Exp $
dnl
AC_DEFUN([AC_PROG_JAVAH],[
AC_REQUIRE([AC_CANONICAL_SYSTEM])dnl
AC_REQUIRE([AC_PROG_CPP])dnl
AC_PATH_PROG(JAVAH,javah)
if test x"`eval 'echo $ac_cv_path_JAVAH'`" != x ; then
  AC_TRY_CPP([#include <jni.h>],,[
    ac_save_CPPFLAGS="$CPPFLAGS"
changequote(, )dnl
    ac_dir=`echo $ac_cv_path_JAVAH | sed 's,\(.*\)/[^/]*/[^/]*$,\1/include,'`
    ac_machdep=`echo $build_os | sed 's,[-0-9].*,,'`
changequote([, ])dnl
    CPPFLAGS="$ac_save_CPPFLAGS -I$ac_dir -I$ac_dir/$ac_machdep"
    AC_TRY_CPP([#include <jni.h>],
               ac_save_CPPFLAGS="$CPPFLAGS",
               AC_MSG_WARN([unable to include <jni.h>]))
    CPPFLAGS="$ac_save_CPPFLAGS"])
fi])

dnl ====> ac_prog_java.m4
dnl @synopsis AC_PROG_JAVA
dnl
dnl Here is a summary of the main macros:
dnl
dnl AC_PROG_JAVAC: finds a Java compiler.
dnl
dnl AC_PROG_JAVA: finds a Java virtual machine.
dnl
dnl AC_CHECK_CLASS: finds if we have the given class (beware of CLASSPATH!).
dnl
dnl AC_CHECK_RQRD_CLASS: finds if we have the given class and stops otherwise.
dnl
dnl AC_TRY_COMPILE_JAVA: attempt to compile user given source.
dnl
dnl AC_TRY_RUN_JAVA: attempt to compile and run user given source.
dnl
dnl AC_JAVA_OPTIONS: adds Java configure options.
dnl
dnl AC_PROG_JAVA tests an existing Java virtual machine. It uses the
dnl environment variable JAVA then tests in sequence various common Java
dnl virtual machines. For political reasons, it starts with the free ones.
dnl You *must* call [AC_PROG_JAVAC] before.
dnl
dnl If you want to force a specific VM:
dnl
dnl - at the configure.in level, set JAVA=yourvm before calling AC_PROG_JAVA
dnl   (but after AC_INIT)
dnl
dnl - at the configure level, setenv JAVA
dnl
dnl You can use the JAVA variable in your Makefile.in, with @JAVA@.
dnl
dnl *Warning*: its success or failure can depend on a proper setting of the
dnl CLASSPATH env. variable.
dnl
dnl TODO: allow to exclude virtual machines (rationale: most Java programs
dnl cannot run with some VM like kaffe).
dnl
dnl Note: This is part of the set of autoconf M4 macros for Java programs.
dnl It is VERY IMPORTANT that you download the whole set, some
dnl macros depend on other. Unfortunately, the autoconf archive does not
dnl support the concept of set of macros, so I had to break it for
dnl submission.
dnl
dnl A Web page, with a link to the latest CVS snapshot is at
dnl <http://www.internatif.org/bortzmeyer/autoconf-Java/>.
dnl
dnl This is a sample configure.in
dnl Process this file with autoconf to produce a configure script.
dnl
dnl    AC_INIT(UnTag.java)
dnl
dnl    dnl Checks for programs.
dnl    AC_CHECK_CLASSPATH
dnl    AC_PROG_JAVAC
dnl    AC_PROG_JAVA
dnl
dnl    dnl Checks for classes
dnl    AC_CHECK_RQRD_CLASS(org.xml.sax.Parser)
dnl    AC_CHECK_RQRD_CLASS(com.jclark.xml.sax.Driver)
dnl
dnl    AC_OUTPUT(Makefile)
dnl
dnl @author Stephane Bortzmeyer <bortzmeyer@pasteur.fr>
dnl @version $Id: acinclude.m4,v 2.31.2.2 2014/08/08 03:06:38 jbj Exp $
dnl
dnl Note: Modified to prefer java over kaffe. [#8059]
dnl
AC_DEFUN([AC_PROG_JAVA],[
AC_REQUIRE([AC_EXEEXT])dnl
if test x$JAVAPREFIX = x; then
        test x$JAVA = x && AC_CHECK_PROGS(JAVA, java$EXEEXT kaffe$EXEEXT)
else
        test x$JAVA = x && AC_CHECK_PROGS(JAVA, java$EXEEXT kaffe$EXEEXT, $JAVAPREFIX)
fi
test x$JAVA = x && AC_MSG_ERROR([no acceptable Java virtual machine found in \$PATH])
AC_PROG_JAVA_WORKS
AC_PROVIDE([$0])dnl
])

dnl ====> ac_prog_java_works.m4
dnl @synopsis AC_PROG_JAVA_WORKS
dnl
dnl Internal use ONLY.
dnl
dnl Note: This is part of the set of autoconf M4 macros for Java programs.
dnl It is VERY IMPORTANT that you download the whole set, some
dnl macros depend on other. Unfortunately, the autoconf archive does not
dnl support the concept of set of macros, so I had to break it for
dnl submission.
dnl The general documentation, as well as the sample configure.in, is
dnl included in the AC_PROG_JAVA macro.
dnl
dnl @author Stephane Bortzmeyer <bortzmeyer@pasteur.fr>
dnl @version $Id: acinclude.m4,v 2.31.2.2 2014/08/08 03:06:38 jbj Exp $
dnl
AC_DEFUN([AC_PROG_JAVA_WORKS], [
AC_CHECK_PROG(uudecode, uudecode$EXEEXT, yes)
if test x$uudecode = xyes; then
AC_CACHE_CHECK([if uudecode can decode base 64 file], ac_cv_prog_uudecode_base64, [
dnl /**
dnl  * Test.java: used to test if java compiler works.
dnl  */
dnl public class Test
dnl {
dnl
dnl public static void
dnl main( String[] argv )
dnl {
dnl     System.exit (0);
dnl }
dnl
dnl }
cat << \EOF > Test.uue
begin-base64 644 Test.class
yv66vgADAC0AFQcAAgEABFRlc3QHAAQBABBqYXZhL2xhbmcvT2JqZWN0AQAE
bWFpbgEAFihbTGphdmEvbGFuZy9TdHJpbmc7KVYBAARDb2RlAQAPTGluZU51
bWJlclRhYmxlDAAKAAsBAARleGl0AQAEKEkpVgoADQAJBwAOAQAQamF2YS9s
YW5nL1N5c3RlbQEABjxpbml0PgEAAygpVgwADwAQCgADABEBAApTb3VyY2VG
aWxlAQAJVGVzdC5qYXZhACEAAQADAAAAAAACAAkABQAGAAEABwAAACEAAQAB
AAAABQO4AAyxAAAAAQAIAAAACgACAAAACgAEAAsAAQAPABAAAQAHAAAAIQAB
AAEAAAAFKrcAErEAAAABAAgAAAAKAAIAAAAEAAQABAABABMAAAACABQ=
====
EOF
if uudecode$EXEEXT Test.uue; then
        ac_cv_prog_uudecode_base64=yes
else
        echo "configure: __oline__: uudecode had trouble decoding base 64 file 'Test.uue'" >&AC_FD_CC
        echo "configure: failed file was:" >&AC_FD_CC
        cat Test.uue >&AC_FD_CC
        ac_cv_prog_uudecode_base64=no
fi
rm -f Test.uue])
fi
if test x$ac_cv_prog_uudecode_base64 != xyes; then
        rm -f Test.class
        AC_MSG_WARN([I have to compile Test.class from scratch])
        if test x$ac_cv_prog_javac_works = xno; then
                AC_MSG_ERROR([Cannot compile java source. $JAVAC does not work properly])
        fi
        if test x$ac_cv_prog_javac_works = x; then
                AC_PROG_JAVAC
        fi
fi
AC_CACHE_CHECK(if $JAVA works, ac_cv_prog_java_works, [
JAVA_TEST=Test.java
CLASS_TEST=Test.class
TEST=Test
changequote(, )dnl
cat << \EOF > $JAVA_TEST
/* [#]line __oline__ "configure" */
public class Test {
public static void main (String args[]) {
        System.exit (0);
} }
EOF
changequote([, ])dnl
if test x$ac_cv_prog_uudecode_base64 != xyes; then
        if AC_TRY_COMMAND($JAVAC $JAVACFLAGS $JAVA_TEST) && test -s $CLASS_TEST; then
                :
        else
          echo "configure: failed program was:" >&AC_FD_CC
          cat $JAVA_TEST >&AC_FD_CC
          AC_MSG_ERROR(The Java compiler $JAVAC failed (see config.log, check the CLASSPATH?))
        fi
fi
if AC_TRY_COMMAND($JAVA $JAVAFLAGS $TEST) >/dev/null 2>&1; then
  ac_cv_prog_java_works=yes
else
  echo "configure: failed program was:" >&AC_FD_CC
  cat $JAVA_TEST >&AC_FD_CC
  AC_MSG_ERROR(The Java VM $JAVA failed (see config.log, check the CLASSPATH?))
fi
rm -fr $JAVA_TEST $CLASS_TEST Test.uue
])
AC_PROVIDE([$0])dnl
]
)

dnl ====> ac_try_compile_java.m4
dnl @synopsis AC_TRY_COMPILE_JAVA
dnl
dnl AC_TRY_COMPILE_JAVA attempt to compile user given source.
dnl
dnl *Warning*: its success or failure can depend on a proper setting of the
dnl CLASSPATH env. variable.
dnl
dnl Note: This is part of the set of autoconf M4 macros for Java programs.
dnl It is VERY IMPORTANT that you download the whole set, some
dnl macros depend on other. Unfortunately, the autoconf archive does not
dnl support the concept of set of macros, so I had to break it for
dnl submission.
dnl The general documentation, as well as the sample configure.in, is
dnl included in the AC_PROG_JAVA macro.
dnl
dnl @author Devin Weaver <ktohg@tritarget.com>
dnl @version $Id: acinclude.m4,v 2.31.2.2 2014/08/08 03:06:38 jbj Exp $
dnl
AC_DEFUN([AC_TRY_COMPILE_JAVA],[
AC_REQUIRE([AC_PROG_JAVAC])dnl
cat << \EOF > Test.java
/* [#]line __oline__ "configure" */
ifelse([$1], , , [import $1;])
public class Test {
[$2]
}
EOF
if AC_TRY_COMMAND($JAVAC $JAVACFLAGS Test.java) && test -s Test.class
then
dnl Don't remove the temporary files here, so they can be examined.
  ifelse([$3], , :, [$3])
else
  echo "configure: failed program was:" >&AC_FD_CC
  cat Test.java >&AC_FD_CC
ifelse([$4], , , [  rm -fr Test*
  $4
])dnl
fi
rm -fr Test*])

dnl ====> ac_try_run_javac.m4
dnl @synopsis AC_TRY_RUN_JAVA
dnl
dnl AC_TRY_RUN_JAVA attempt to compile and run user given source.
dnl
dnl *Warning*: its success or failure can depend on a proper setting of the
dnl CLASSPATH env. variable.
dnl
dnl Note: This is part of the set of autoconf M4 macros for Java programs.
dnl It is VERY IMPORTANT that you download the whole set, some
dnl macros depend on other. Unfortunately, the autoconf archive does not
dnl support the concept of set of macros, so I had to break it for
dnl submission.
dnl The general documentation, as well as the sample configure.in, is
dnl included in the AC_PROG_JAVA macro.
dnl
dnl @author Devin Weaver <ktohg@tritarget.com>
dnl @version $Id: acinclude.m4,v 2.31.2.2 2014/08/08 03:06:38 jbj Exp $
dnl
AC_DEFUN([AC_TRY_RUN_JAVA],[
AC_REQUIRE([AC_PROG_JAVAC])dnl
AC_REQUIRE([AC_PROG_JAVA])dnl
cat << \EOF > Test.java
/* [#]line __oline__ "configure" */
ifelse([$1], , , [include $1;])
public class Test {
[$2]
}
EOF
if AC_TRY_COMMAND($JAVAC $JAVACFLAGS Test.java) && test -s Test.class && ($JAVA $JAVAFLAGS Test; exit) 2>/dev/null
then
dnl Don't remove the temporary files here, so they can be examined.
  ifelse([$3], , :, [$3])
else
  echo "configure: failed program was:" >&AC_FD_CC
  cat Test.java >&AC_FD_CC
ifelse([$4], , , [  rm -fr Test*
  $4
])dnl
fi
rm -fr Test*])

