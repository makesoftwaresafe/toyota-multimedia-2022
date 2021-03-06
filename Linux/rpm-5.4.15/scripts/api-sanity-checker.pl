#!/usr/bin/perl
###########################################################################
# API Sanity Checker (ASC) 1.12.9
# Unit test generator for a shared C/C++ library API
#
# Copyright (C) 2011 Institute for System Programming, RAS
# Copyright (C) 2011 ROSA Laboratory
#
# Written by Andrey Ponomarenko
#
# PLATFORMS
# =========
#  Linux, FreeBSD, Mac OS X, MS Windows
#
# REQUIREMENTS
# ============
#  Linux
#    - GCC (3.0-4.6.2, recommended 4.5 or newer)
#    - GNU Binutils (readelf, c++filt, objdump)
#    - Perl (5.8-5.14)
#
#  Mac OS X
#    - Xcode (gcc, otool, c++filt)
#
#  MS Windows
#    - MinGW (gcc.exe, c++filt.exe)
#    - Active Perl
#    - MS Visual C++ (dumpbin.exe, undname.exe, cl.exe)
#    - Add gcc.exe path (C:\MinGW\bin\) to your system PATH variable
#    - Run vsvars32.bat (C:\Microsoft Visual Studio 9.0\Common7\Tools\)
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License or the GNU Lesser
# General Public License as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# and the GNU Lesser General Public License along with this program.
# If not, see <http://www.gnu.org/licenses/>.
###########################################################################
use Getopt::Long;
Getopt::Long::Configure ("posix_default", "no_ignore_case");
use POSIX qw(setsid);
use File::Path qw(mkpath rmtree);
use File::Temp qw(tempdir);
use File::Copy qw(copy);
use Cwd qw(abs_path cwd);
use Config;

my $API_SANITY_CHECKER_VERSION = "1.12.9";

my ($Help, $InfoMsg, $TargetLibraryName, $GenerateTests, $TargetInterfaceName, $BuildTests, $RunTests, $CleanTests,
$DisableReuse, $LongVarNames, %Descriptor, $UseXvfb, $TestSystem, $MinimumCode, $TestDataPath, $MaximumCode,
$RandomCode, $GenerateDescriptorTemplate, $GenerateSpecTypeTemplate, $InterfacesListPath, $SpecTypes_PackagePath,
$CheckReturn, $DisableDefaultValues, $CheckStdCxx, $DisableIncludeOptimization, $ShowRetVal, $CheckHeadersOnly,
$Template2Code, $Standalone, $ShowVersion, $MakeIsolated, $ParameterNamesFilePath, $CleanSources, $SplintAnnotations,
$DumpVersion, $TargetHeaderName, $RelativeDirectory, $TargetLibraryFullName, $TargetVersion, $StrictGen, $StrictBuild,
$StrictRun, $Strict, $DebugMode);

my $CmdName = get_filename($0);
my $OSgroup = get_OSgroup();
my %OS_LibExt=(
    "default"=>"so",
    "macos"=>"dylib",
    "windows"=>"lib",
    "hpux"=>"sl",
    "symbian"=>"dso"
);
my $LIB_EXT = $OS_LibExt{$OSgroup}?$OS_LibExt{$OSgroup}:$OS_LibExt{"default"};
my $SHARED = $OSgroup=~/macos|windows/?"dynamic":"shared";
my $ORIG_DIR = cwd();
my $TMP_DIR = tempdir(CLEANUP=>1);

my %HomePage = (
    "Wiki"=>"http://ispras.linux-foundation.org/index.php/API_Sanity_Autotest",
    "Dev"=>"http://forge.ispras.ru/projects/api-sanity-autotest"
);

my $ShortUsage = "API Sanity Checker (ASC) $API_SANITY_CHECKER_VERSION
Unit test generator for a shared C/C++ library API
Copyright (C) 2011 ROSA Laboratory
License: GNU LGPL or GNU GPL

Usage: $CmdName [options]
Example: $CmdName -lib NAME -d VERSION.xml -gen -build -run

VERSION.xml is XML-descriptor:

    <version>
        1.0
    </version>

    <headers>
        /path/to/headers/
    </headers>

    <libs>
        /path/to/libraries/
    </libs>

More info: $CmdName --help\n";

if($#ARGV==-1) {
    print $ShortUsage;
    exit(0);
}

foreach (2 .. $#ARGV)
{# correct comma separated options
    if($ARGV[$_-1] eq ",") {
        $ARGV[$_-2].=",".$ARGV[$_];
        splice(@ARGV, $_-1, 2);
    }
    elsif($ARGV[$_-1]=~/,\Z/) {
        $ARGV[$_-1].=$ARGV[$_];
        splice(@ARGV, $_, 1);
    }
    elsif($ARGV[$_]=~/\A,/
    and $ARGV[$_] ne ",") {
        $ARGV[$_-1].=$ARGV[$_];
        splice(@ARGV, $_, 1);
    }
}

my @INPUT_OPTIONS = @ARGV;

GetOptions("h|help!" => \$Help,
  "info!" => \$InfoMsg,
  "v|version!" => \$ShowVersion,
  "dumpversion!" => \$DumpVersion,
#general options
  "l|lib|library=s" => \$TargetLibraryName,
  "d|descriptor=s" => \$Descriptor{"Path"},
  "gen|generate!" => \$GenerateTests,
  "build|make!" => \$BuildTests,
  "run!" => \$RunTests,
  "clean!" => \$CleanTests,
  "s|symbol|f|function|i|interface=s" => \$TargetInterfaceName,
  "symbols-list|functions-list|interfaces-list=s" => \$InterfacesListPath,
  "header=s" => \$TargetHeaderName,
  "xvfb!" => \$UseXvfb,
  "t2c|template2code" => \$Template2Code,
  "splint-specs!" => \$SplintAnnotations,
  "strict-gen!" => \$StrictGen,
  "strict-build!" => \$StrictBuild,
  "strict-run!" => \$StrictRun,
  "strict!" => \$Strict,
#extra options
  "d-tmpl|descriptor-template!" =>\$GenerateDescriptorTemplate,
  "s-tmpl|specialized-type-template!" =>\$GenerateSpecTypeTemplate,
  "-vnum=s" =>\$TargetVersion,
  "r|random!" =>\$RandomCode,
  "min!" =>\$MinimumCode,
  "max!" =>\$MaximumCode,
  "show-retval!" => \$ShowRetVal,
  "check-retval!" => \$CheckReturn,
  "st|specialized-types=s" => \$SpecTypes_PackagePath,
  "td|test-data=s" => \$TestDataPath,
  "headers-only!" => \$CheckHeadersOnly,
  "isolated!" => \$MakeIsolated,
  "view-only!" => \$CleanSources,
  "disable-default-values!" => \$DisableDefaultValues,
  "p|params=s" => \$ParameterNamesFilePath,
  "l-full=s" => \$TargetLibraryFullName,
  "relpath|reldir=s" => \$RelativeDirectory,
  "debug!" => \$DebugMode,
#other options
  "test!" => \$TestSystem,
  "check-stdcxx-symbols!" => \$CheckStdCxx,
  "disable-variable-reuse!" => \$DisableReuse,
  "long-variable-names!" => \$LongVarNames
) or ERR_MESSAGE();

sub ERR_MESSAGE()
{
    print $ShortUsage;
    exit(1);
}

my $HelpMessage="
NAME:
  API Sanity Checker ($CmdName)
  Generate basic unit tests for a $SHARED C/C++ library API

DESCRIPTION:
  API Sanity Checker (ASC) is an automatic generator of basic unit tests
  for a $SHARED C/C++ library. It helps to quickly generate simple (\"sanity\"
  or \"shallow\"-quality) tests for every function in an API using their
  signatures, data type definitions and relationships between functions straight
  from the library header files (\"Header-Driven Generation\"). Each test case
  contains a function call with reasonable (in most, but unfortunately not all,
  cases) input parameters. The quality of generated tests allows to check
  absence of critical errors in simple use cases and can be greatly improved
  by involving of highly reusable specialized types for the library.

  ASAT can execute generated tests and detect crashes, aborts, all kinds of
  emitted signals, non-zero program return code, program hanging and requirement
  failures (if specified). ASAT can be considered as a tool for out-of-box
  low-cost sanity checking of library API or as a test development framework for
  initial generation of templates for advanced tests. Also it supports universal
  Template2Code format of tests, splint specifications, random test generation
  mode and other useful features.

  This tool is free software: you can redistribute it and/or modify it
  under the terms of the GNU LGPL or GNU GPL.

USAGE:
  $CmdName [options]

EXAMPLE:
  $CmdName -lib NAME -d VERSION.xml -gen -build -run

  VERSION.xml is XML-descriptor:

    <version>
        1.0
    </version>

    <headers>
        /path1/to/header(s)/
        /path2/to/header(s)/
         ...
    </headers>

    <libs>
        /path1/to/library(ies)/
        /path2/to/library(ies)/
         ...
    </libs>

INFORMATION OPTIONS:
  -h|-help
      Print this help.

  -info
      Print complete info.

  -v|-version
      Print version information.

  -dumpversion
      Print the tool version ($API_SANITY_CHECKER_VERSION) and don't do anything else.

GENERAL OPTIONS:
  -l|-lib|-library <name>
      Library name (without version). 
      It affects only on the path and the title of the reports.

  -d|-descriptor <path(s)>
      Path to the library descriptor.
      It may be one of the following:
      
         1. XML-descriptor (VERSION.xml file):
             <version>
                 1.0
             </version>

             <headers>
                 /some/path/include/
             </headers>

             <libs>
                 /some/path/lib/
             </libs>

                 ... (XML-descriptor format is defined in file
                         generated by -d-tmpl option)

         2. Directory with headers and libraries:
              $CmdName -lib NAME -d DIR1.0/ -gen -build -run
              
         3. Comma separated list of headers and libraries:
              $CmdName -lib NAME -d HEADER.h,LIB.$LIB_EXT -gen -build -run

      If you are using an alternative descriptor type then you should
      specify a version number with -vnum <ver> option too.

      For more information, please see:
        http://ispras.linux-foundation.org/index.php/Library_Descriptor

  -gen|-generate
      Generate test(s). Options -l and -d should be specified.
      To generate test for the particular function use it with -f option.
      Exit code: number of test cases failed to build.

  -build|-make
      Build test(s). Options -l and -d should be specified.
      To build test for the particular function use it with -f option.
      Exit code: number of test cases failed to generate.

  -run
      Run test(s), create test report. Options -l and -d should be specified.
      To run test for the particular function use it with -f option.
      Exit code: number of failed test cases.

  -clean
      Clean test(s). Options -l and -d should be specified.
      To clean test for the particular function use it with -f option.

  -vnum <version>
      Specify library version outside the descriptor.

  -f|-function|-s|-symbol|-i|-interface <name>
      Generate/Build/Run test for the specified function
      (mangled/symbol name in C++).

  -symbols-list|-functions-list|-interfaces-list <path>
      This option allows to specify a file with a list of functions
      (one per line, mangled/symbol name in C++) that should be tested,
      other library functions will not be tested.

  -header <name>
      This option allows to restrict a list of functions that should be tested
      by providing a header file name in which they are declared. This option
      is intended for step-by-step tests development.

  -xvfb
      Use Xvfb-server instead of current X-server (by default)
      for running tests. For more information, please see:
        http://en.wikipedia.org/wiki/Xvfb

  -t2c|-template2code
      Generate tests in the universal Template2Code format.
      For more information, please see:
        http://sourceforge.net/projects/template2code/

  -splint-specs
     Read splint specifications (annotations) in the header files.
     For more information, please see:
        http://www.splint.org/manual/

  -strict-gen
     Terminate the process of generating tests and return
     error code '1' if cannot generate at least one test case.

  -strict-build
     Terminate the process of building tesst and return
     error code '1' if cannot build at least one test case.

  -strict-run
     Terminate the process of running tests and return
     error code '1' if at least one test case failed.

  -strict
     This option enables all -strict-* options.\n";

sub HELP_MESSAGE() {
    print $HelpMessage."
MORE INFO:
     $CmdName --info\n\n";
}

sub INFO_MESSAGE()
{
    print "$HelpMessage
EXTRA OPTIONS:
  -d-tmpl|-descriptor-template
      Create XML-descriptor template ./VERSION.xml

  -s-tmpl|specialized-type-template
      Create specialized type template ./SPECTYPES.xml

  -r|-random
      Random test generation mode.

  -min
      Generate minimun code, call functions with minimum number of parameters
      to initialize parameters of other functions.

  -max
      Generate maximum code, call functions with maximum number of parameters
      to initialize parameters of other functions.

  -show-retval
      Show the function return type in the report.

  -check-retval
      Insert requirements on return values (retval!=NULL) for each
      called function.

  -st|-specialized-types <path>
      Path to the file with the collection of specialized types.
      For more information, please see:
        http://ispras.linux-foundation.org/index.php/Specialized_Type

  -td|-test-data <path>
      Path to the directory with the test data files.
      For more information, please see:
        http://ispras.linux-foundation.org/index.php/Specialized_Type

  -headers-only
      If the library is header-based and has no $SHARED objects this option
      allows to generate tests for headers only.

  -isolated
      Allow to restrict functions usage by the lists specified by the
      -functions-list option or by the group devision in the descriptor.

  -view-only
      Remove all files from the test suite except *.html files. This option
      allows to create a lightweight html-index for all tests.

  -disable-default-values
      Disable usage of default values for function parameters.

  -p|-params <path>
      Path to file with the function parameter names. It can be used for
      improving generated tests if the library header files don't contain
      parameter names. File format:

            func1;param1;param2;param3 ...
            func2;param1;param2;param3 ...
            ...

  -library-full-name <name>
      Library name in the report title.

  -relpath|-reldir <path>
      Replace {RELPATH} in the library descriptor to <path>.

  -debug
      Write extended log for debugging.

OTHER OPTIONS:
  -test
      Run internal tests. Create a simple library and run the tool on it.
      This option allows to check if the tool works correctly on the system.

  -check-stdcxx-symbols
      Enable checking of the libstdc++ impurity functions.

  -disable-variable-reuse
      Disable reusing of previously created variables in the test.

  -long-variable-names
      Enable long (complex) variable names instead of short names.

EXIT CODES:
    0 - Successful tests. The tool has run without any errors.
    non-zero - Tests failed or the tool has run with errors.

REPORT BUGS TO:
    api-sanity-autotest\@linuxtesting.org
    Andrey Ponomarenko <aponomarenko\@mandriva.org>

MORE INFORMATION:
    ".$HomePage{"Wiki"}."
    ".$HomePage{"Dev"}."\n\n";
}

my $Descriptor_Template = "
<?xml version=\"1.0\" encoding=\"utf-8\"?>
<descriptor>

/* XML-descriptor template */

/* Primary sections */

<version>
    /* Version of the library */
</version>

<headers>
    /* The list of paths to header files and/or
         directories with header files, one per line */
</headers>

<libs>
    /* The list of paths to libraries (*.$LIB_EXT) and/or
         directories with libraries, one per line */
</libs>

/* Optional sections */

<include_paths>
    /* The list of paths to be searched for header files
         needed for compiling of library headers, one per line.
         NOTE: If you define this section then the tool
         will not automatically detect include paths */
</include_paths>

<add_include_paths>
    /* The list of include paths that should be added
         to the automatically detected include paths, one per line */
</add_include_paths>

<gcc_options>
    /* Additional GCC options, one per line */
</gcc_options>

<include_preamble>
    /* The list of header files that should be included before other headers, one per line.
         For example, it is a tree.h for libxml2 and ft2build.h for freetype2 */
</include_preamble>

<libs_depend>
    /* The list of paths to $SHARED libraries that should be provided to gcc
         for resolving undefined symbols (if NEEDED elf section doesn't include it) */
</libs_depend>

<opaque_types>
    /* The list of opaque types, one per line */
</opaque_types>

<skip_symbols>
    /* The list of functions (mangled/symbol names in C++)
         that should not be called in the tests, one per line */
</skip_symbols>

<skip_headers>
    /* The list of header files and/or directories with header files
         that should not be processed, one name per line */
</skip_headers>

<skip_libs>
    /* The list of $SHARED libraries and/or directories with $SHARED libraries
         that should not be processed, one name per line */
</skip_libs>

<defines>
    /* Add defines at the headers compiling stage, one per line:
          #define A B
          #define C D */
</defines>

<libgroup>
    <name>
        /* Name of the libgroup */
    </name>

    <symbols>
        /* The list of symbols (mangled/symbol names in C++)
             in the group that should be tested, one per line */
    </symbols>
</libgroup>

<out_params>
    /* Associating of out(returned)-parameters
         with symbols, one entry per line:
           symbol:param_name
                 or
           symbol:param_number
         Examples:
           dbus_parse_address:entry
           dbus_parse_address:2       */
</out_params>

<skip_warnings>
    /* The list of warnings that should not be shown in the report, one pattern per line */
</skip_warnings>

</descriptor>";

my $SpecType_Template="
<?xml version=\"1.0\" encoding=\"utf-8\"?>
<collection>

<lib_version>
    /* Constraint on the library version
         to which this collection will be applied.
         Select it from the following list:
           x.y.z
           >=x.y.z
           <=x.y.z */
</lib_version>

<!--
 C/C++ language extensions in the code:
  \$(type) - instruction initializing an instance of data type
  \$[symbol] - instruction for symbol call with properly initialized parameters
  \$0 - an instance of the specialized type
  \$1, \$2, ... - references to 1st, 2nd and other parameters
  \$obj - reference to the object that current method calls on (C++ only)
  For more information, please see:
    http://ispras.linux-foundation.org/index.php/Specialized_Type
-->

<spec_type>
    <kind>
        /* Kind of the specialized type.
             Select it from the following list:
               normal
               common_param
               common_retval
               env
               common_env */
    </kind>

    <data_type>
        /* Name of the corresponding real data type.
             You can specify several data types if kind is 'common_param'
             or 'common_retval', one per line.
             This section is not used if kind is 'env' or 'common_env' */
    </data_type>

    <value>
        /* Value for initialization (true, 1.0, \"string\", ...) */
    </value>

    <pre_condition>
        /* Precondition on associated function parameter.
               Example: \$0!=NULL */
    </pre_condition>

    <post_condition>
        /* Postcondition on associated function return value or parameter.
               Example: \$0!=NULL && \$obj.style()==Qt::DotLine */
    </post_condition>

    <decl_code>
        /* Code that will be pasted instead of parameter automatic declaration.
               Example: char \$0[16]; */
    </decl_code>

    <init_code>
        /* Code that should be invoked before function call.
               Example: \$0->start(); */
    </init_code>

    <final_code>
        /* Code that should be invoked after function call
               Example: \$0->end(); */
    </final_code>

    <global_code>
        /* Declarations of auxiliary functions and global variables,
              header includes */
    </global_code>

    <associating>
        /* Several \"associating\" sections
              are allowed simultaneously */
        
        <symbols>
            /* List of symbols (mangled/symbol names in C++)
                  that will be associated with the specialized type, one per line */
        </symbols>

        <except>
            /* List of symbols (mangled/symbol names in C++)
                 that will not be associated with the specialized type, one per line.
                 This section is used if kind is 'common_env', 'common_param'
                 or 'common_return' */
        </except>

        <links>
            /* Associations with the return value, parameters
                 or/and object, one per line:
                   param1
                   param2
                   param3
                    ...
                   object
                   retval */
        </links>
        
        <param_names>
            /* Associations with the parameters by name, one per line:
                   param_name1
                   param_name2
                   param_name3
                    ...
                        */
        </param_names>
    </associating>
    
    <name>
        /* Name of the specialized type */
    </name>

    <libs>
        /* External $SHARED libraries, one per line.
           If spectype contains call of the functions from
           some external $SHARED libraries then these objects
           should be listed here. Corresponding external
           header files should be included in global_code */
    </libs>

    <lib_version>
        /* Constraint on the library version
             to which this spectype will be applied.
             Select it from the following list:
               x.y.z
               >=x.y.z
               <=x.y.z */
    </lib_version>
</spec_type>

<spec_type>
    /* Other specialized type */
</spec_type>

</collection>";

#Constants
my $BUFF_SIZE = 256;
my $DEFAULT_ARRAY_AMOUNT = 4;
my $MAX_PARAMS_INLINE = 3;
my $MAX_PARAMS_LENGTH_INLINE = 60;
my $MAX_COMMAND_LINE_ARGUMENTS = 4096;
my $POINTER_SIZE;
my $HANGED_EXECUTION_TIME = 7;
my $TOOL_SIGNATURE;
my $MIN_PARAMS_MATRIX = 8;
my $MATRIX_WIDTH = 4;
my $MATRIX_MAX_ELEM_LENGTH = 7;
my $MAX_FILE_NAME_LENGTH = 255;
my $LIBRARY_PREFIX_MAJORITY = 10;

my %Operator_Indication = (
    "not" => "~",
    "assign" => "=",
    "andassign" => "&=",
    "orassign" => "|=",
    "xorassign" => "^=",
    "or" => "|",
    "xor" => "^",
    "addr" => "&",
    "and" => "&",
    "lnot" => "!",
    "eq" => "==",
    "ne" => "!=",
    "lt" => "<",
    "lshift" => "<<",
    "lshiftassign" => "<<=",
    "rshiftassign" => ">>=",
    "call" => "()",
    "mod" => "%",
    "modassign" => "%=",
    "subs" => "[]",
    "land" => "&&",
    "lor" => "||",
    "rshift" => ">>",
    "ref" => "->",
    "le" => "<=",
    "deref" => "*",
    "mult" => "*",
    "preinc" => "++",
    "delete" => " delete",
    "vecnew" => " new[]",
    "vecdelete" => " delete[]",
    "predec" => "--",
    "postinc" => "++",
    "postdec" => "--",
    "plusassign" => "+=",
    "plus" => "+",
    "minus" => "-",
    "minusassign" => "-=",
    "gt" => ">",
    "ge" => ">=",
    "new" => " new",
    "multassign" => "*=",
    "divassign" => "/=",
    "div" => "/",
    "neg" => "-",
    "pos" => "+",
    "memref" => "->*",
    "compound" => "," );

my %CppKeywords_C = map {$_=>1} (
    # C++ 2003 keywords
    "public",
    "protected",
    "private",
    "default",
    "template",
    "new",
    #"asm",
    "dynamic_cast",
    "auto",
    "try",
    "namespace",
    "typename",
    "using",
    "reinterpret_cast",
    "friend",
    "class",
    "virtual",
    "const_cast",
    "mutable",
    "static_cast",
    "export",
    # C++0x keywords
    "noexcept",
    "nullptr",
    "constexpr",
    "static_assert",
    "explicit",
    # cannot be used as a macro name
    # as it is an operator in C++
    "and",
    #"and_eq",
    "not",
    #"not_eq",
    "or"
    #"or_eq",
    #"bitand",
    #"bitor",
    #"xor",
    #"xor_eq",
    #"compl"
);

my %CppKeywords_F = map {$_=>1} (
    "delete",
    "catch",
    "alignof",
    "thread_local",
    "decltype",
    "typeid"
);

my %CppKeywords_O = map {$_=>1} (
    "bool",
    "register",
    "inline",
    "operator"
);

my %CppKeywords_A = map {$_=>1} (
    "this",
    "throw"
);

foreach (keys(%CppKeywords_C),
keys(%CppKeywords_F),
keys(%CppKeywords_O)) {
    $CppKeywords_A{$_}=1;
}

my %IsKeyword=(
    "delete"=>1,
    "if"=>1,
    "else"=>1,
    "for"=>1,
    "public"=>1,
    "private"=>1,
    "new"=>1,
    "protected"=>1,
    "main"=>1,
    "sizeof"=>1,
    "malloc"=>1,
    "return"=>1,
    "include"=>1,
    "true"=>1,
    "false"=>1,
    "const"=>1,
    "int"=>1,
    "long"=>1,
    "void"=>1,
    "short"=>1,
    "float"=>1,
    "unsigned"=>1,
    "char"=>1,
    "double"=>1,
    "class"=>1,
    "struct"=>1,
    "union"=>1,
    "enum"=>1,
    "volatile"=>1,
    "restrict"=>1
);

my %GlibcHeader = map {$_=>1} (
    "aliases.h",
    "argp.h",
    "argz.h",
    "assert.h",
    "cpio.h",
    "ctype.h",
    "dirent.h",
    "envz.h",
    "errno.h",
    "error.h",
    "execinfo.h",
    "fcntl.h",
    "fstab.h",
    "ftw.h",
    "glob.h",
    "grp.h",
    "iconv.h",
    "ifaddrs.h",
    "inttypes.h",
    "langinfo.h",
    "limits.h",
    "link.h",
    "locale.h",
    "malloc.h",
    "math.h",
    "mntent.h",
    "monetary.h",
    "nl_types.h",
    "obstack.h",
    "printf.h",
    "pwd.h",
    "regex.h",
    "sched.h",
    "search.h",
    "setjmp.h",
    "shadow.h",
    "signal.h",
    "spawn.h",
    "stdarg.h",
    "stdint.h",
    "stdio.h",
    "stdlib.h",
    "string.h",
    "tar.h",
    "termios.h",
    "time.h",
    "ulimit.h",
    "unistd.h",
    "utime.h",
    "wchar.h",
    "wctype.h",
    "wordexp.h" );

my %GlibcDir = map {$_=>1} (
    "sys",
    "linux",
    "bits",
    "gnu",
    "netinet",
    "rpc" );

my %LocalIncludes = map {$_=>1} (
    "/usr/local/include",
    "/usr/local" );

my %ShortTokens=(
    "err"=>"error",
    "warn"=>"warning" );

my %OS_AddPath=(
# this data needed if tool can't determine paths automatically
"macos"=>{
    "include"=>{"/Library"=>1,"/Developer/usr/include"=>1},
    "lib"=>{"/Library"=>1,"/Developer/usr/lib"=>1},
    "bin"=>{"/Developer/usr/bin"=>1}},
"beos"=>{
    "include"=>{"/boot/common"=>1,"/boot/develop"=>1},
    "lib"=>{"/boot/common/lib"=>1,"/boot/system/lib"=>1,"/boot/apps"=>1},
    "bin"=>{"/boot/common/bin"=>1,"/boot/system/bin"=>1,"/boot/develop/abi"=>1}}
    # Haiku has GCC 2.95.3 by default, try to find GCC>=3.0 in /boot/develop/abi
);

my %Slash_Type=(
    "default"=>"/",
    "windows"=>"\\"
);

my $SLASH = $Slash_Type{$OSgroup}?$Slash_Type{$OSgroup}:$Slash_Type{"default"};

#Global variables
my $ST_ID=0;
my $REPORT_PATH;
my $TEST_SUITE_PATH;
my $LOG_PATH;
my %Interface_TestDir;
my %CompilerOptions_Libs;
my $CompilerOptions_Headers;
my %Language;
my %LibInfo;
my %Cache;
my $TestedInterface;
my $COMMON_LANGUAGE="C";
my $STDCXX_TESTING;
my $GLIBC_TESTING;
my $MAIN_CPP_DIR;
my %SubClass_Created;
my $PreprocessedUnit;
my %Constants;
my %AllDefines;
my $MaxTypeId_Start;
my $STAT_FIRST_LINE = "";

#Types
my %TypeDescr;
my %TemplateInstance;
my %TemplateInstance_Func;
my %TemplateHeader;
my %TypedefToAnon;
my %OpaqueTypes;
my %Tid_TDid;
my %TName_Tid;
my %StructUnionPName_Tid;
my %Class_Constructors;
my %Class_Destructors;
my %ReturnTypeId_Interface;
my %BaseType_PLevel_Return;
my %OutParam_Interface;
my %BaseType_PLevel_OutParam;
my %Interface_OutParam;
my %Interface_OutParam_NoUsing;
my %OutParamInterface_Pos;
my %OutParamInterface_Pos_NoUsing;
my %Class_SubClasses;
my %Type_Typedef;
my %Typedef_BaseName;
my %Typedef_Translations;
my %Typedef_Equivalent;
my %StdCxxTypedef;
my %NameSpaces;
my %NestedNameSpaces;
my %EnumMembers;
my %SubClass_Instance;
my %SubClass_ObjInstance;
my %ClassInTargetLibrary;
my %TemplateNotInst;
my %EnumMembName_Id;
my %ExplicitTypedef;
my %StructIDs;
my %BaseType_PLevel_Type;
my %Struct_SubClasses;
my %Struct_Parent;
my %Library_Prefixes;
my %Member_Struct;
my %IgnoreTmplInst;
my %ClassesWithConstructors;
my %ClassesWithMethods;
my %Struct_Mapping;

#Interfaces
my %FuncDescr;
my %CompleteSignature;
my %SkipInterfaces;
my %SkipInterfaces_Pattern;
my %tr_name;
my %mangled_name_gcc;
my %mangled_name;
my %Interface_Library;
my %InterfaceVersion_Library;
my %NeededInterface_Library;
my %NeededInterfaceVersion_Library;
my %Class_PureVirtFunc;
my %Class_Method;
my %Interface_Overloads;
my %OverloadedInterface;
my %InlineTmplCtor;
my %InterfacesList;
my %MethodNames;
my %FuncNames;
my %GlobVarNames;
my %InternalInterfaces;
my %Func_TypeId;
my %Header_Interface;
my %SoLib_IntPrefix;
my $NodeInterface;
my %LibGroups;
my %Interface_LibGroup;
my %AddIntParams;
my %Func_ShortName_MangledName;
my %UserDefinedOutParam;
my %MangledNames;
my $LibraryMallocFunc;
my %LibraryInitFunc;
my %LibraryExitFunc;

#Headers
my %Include_Preamble;
my %Target_Headers;
my %Header_Dependency;
my %Include_Paths;
my $INC_PATH_AUTODETECT = 1;
my %Add_Include_Paths;
my %DependencyHeaders_All;
my %DependencyHeaders_All_FullPath;
my %Header_ErrorRedirect;
my %Header_Includes;
my %Header_ShouldNotBeUsed;
my $Header_MaxIncludes;
my $IsHeaderListSpecified = 1;
my %Header_TopHeader;
my %Header_Include_Prefix;
my %Include_Prefixes;
my %IncDir_Prefixes;
my %Header_Prefix;
my %RecursiveIncludes;
my %RecursiveIncludes_Inverse;
my %RegisteredHeaders;
my %RegisteredHeaders_Short;
my %RegisteredDirs;
my %SpecTypeHeaders;
my %SkipHeaders;
my %SkipHeaders_Pattern;
my %SkipLibs;
my %SkipLibs_Pattern;
my %SkipWarnings;
my %SkipWarnings_Pattern;
my %Include_Order;
my %Include_RevOrder;
my %TUnit_NameSpaces;
my $C99Mode = 0;

# Binaries
my %DefaultBinPaths;
my ($GCC_PATH, $GPP_PATH, $CPP_FILT);

#Shared objects
my %SharedObjects;
my %SharedObject_Paths;
my %SharedObject_UndefinedSymbols;
my %SharedObject_RecurDeps;
my %SoLib_DefaultPath;
my %CheckedSoLib;

#System shared objects
my %SystemObjects;
my %DefaultLibPaths;
my %SystemObjects_Needed;

#System header files
my %SystemHeaders;
my %DefaultCppPaths;
my %DefaultGccPaths;
my %DefaultIncPaths;
my %DefaultCppHeader;
my %DefaultGccHeader;

#Test results
my %GenResult;
my %BuildResult;
my %RunResult;
my %ResultCounter;

#Signals
my %SigNo;
my %SigName;

#Recursion locks
my @RecurTypeId;
my @RecurInterface;
my @RecurSpecType;
my @RecurHeader;
my @RecurLib;
my @RecurInclude;
my @RecurSymlink;

#System
my %SystemPaths;

#Global state
my (%ValueCollection, %Block_Variable, %UseVarEveryWhere, %SpecEnv, %Block_InsNum, $MaxTypeId, %Wrappers,
%Wrappers_SubClasses, %IntSubClass, %IntrinsicNum, %AuxType, %AuxFunc, %UsedConstructors,
%ConstraintNum, %RequirementsCatalog, %UsedProtectedMethods, %Create_SubClass, %SpecCode,
%SpecLibs, %UsedInterfaces, %OpenStreams, %IntSpecType, %Block_Param, %Class_SubClassTypedef, %AuxHeaders,
%Template2Code_Defines, %TraceFunc);

#Block initialization
my $CurrentBlock;

#Special types
my %SpecType;
my %InterfaceSpecType;
my %InterfaceSupplement;
my %InterfaceSupplement_Lib;
my %Common_SpecEnv;
my %Common_SpecType_Exceptions;
my %ProxyValue = ();

#Annotations
my %Interface_PreCondition;
my %Interface_PostCondition;

#Report
my $ContentSpanStart = "<span class=\"section\" onclick=\"javascript:showContent(this, 'CONTENT_ID')\"><span class='ext' style='padding-right:2px'>[+]</span>\n";
my $ContentSpanStart_Title = "<span class=\"section_title\" onclick=\"javascript:showContent(this, 'CONTENT_ID')\"><span class='ext_title' style='padding-right:2px'>[+]</span>\n";
my $ContentSpanEnd = "</span>\n";
my $ContentDivStart = "<div id=\"CONTENT_ID\" style=\"display:none;\">\n";
my $ContentDivEnd = "</div>\n";
my $ContentID = 1;
my $Content_Counter = 0;

#Debug
my %DebugInfo;

sub get_CmdPath($)
{
    my $Name = $_[0];
    return "" if(not $Name);
    return $Cache{"get_CmdPath"}{$Name} if(defined $Cache{"get_CmdPath"}{$Name});
    if($Name eq "c++filt")
    {# search for c++filt in the same directory as GCC
        my $Path = get_CmdPath("gcc");
        if($Path=~s/([\/\\])gcc\Z/$1c++filt/
        and (-f $Path or -f $Path.".exe")) {
            $Cache{"get_CmdPath"}{$Name} = $Path;
            return $Cache{"get_CmdPath"}{$Name};
        }
    }
    if(my $DefaultPath = get_CmdPath_Default($Name)) {
        $Cache{"get_CmdPath"}{$Name} = $DefaultPath;
        return $Cache{"get_CmdPath"}{$Name};
    }
    foreach my $Path (sort {length($a)<=>length($b)} keys(%{$SystemPaths{"bin"}}))
    {
        if(-f $Path."/".$Name or -f $Path."/".$Name.".exe")
        {
            if($Name eq "gcc" or $Name eq "g++") {
                next if(not check_gcc_version(joinPath($Path,$Name), 3));
            }
            $Cache{"get_CmdPath"}{$Name} = joinPath($Path,$Name);
            return $Cache{"get_CmdPath"}{$Name};
        }
    }
    $Cache{"get_CmdPath"}{$Name} = "";
    return "";
}

sub get_CmdPath_Default($)
{# search in PATH
    my $Name = $_[0];
    return "" if(not $Name);
    return $Cache{"get_CmdPath_Default"}{$Name} if(defined $Cache{"get_CmdPath_Default"}{$Name});
    if($Name eq "find")
    {# special case: search for "find" utility
        if(`find . -maxdepth 0 2>$TMP_DIR/null`) {
            $Cache{"get_CmdPath_Default"}{$Name} = "find";
            return "find";
        }
    }
    elsif($Name eq "gcc" or $Name eq "g++") {
        return check_gcc_version($Name, 3);
    }
    if(get_version($Name)) {
        $Cache{"get_CmdPath_Default"}{$Name} = $Name;
        return $Name;
    }
    if($OSgroup eq "windows"
    and `$Name /? 2>$TMP_DIR/null`) {
        $Cache{"get_CmdPath_Default"}{$Name} = $Name;
        return $Name;
    }
    if($Name ne "which") {
        my $WhichCmd = get_CmdPath("which");
        if($WhichCmd and `$WhichCmd $Name 2>$TMP_DIR/null`) {
            $Cache{"get_CmdPath_Default"}{$Name} = $Name;
            return $Cache{"get_CmdPath_Default"}{$Name};
        }
    }
    foreach my $Path (sort {length($a)<=>length($b)} keys(%DefaultBinPaths))
    {
        if(-f $Path."/".$Name or -f $Path."/".$Name.".exe") {
            $Cache{"get_CmdPath_Default"}{$Name} = joinPath($Path,$Name);
            return $Cache{"get_CmdPath_Default"}{$Name};
        }
    }
    $Cache{"get_CmdPath_Default"}{$Name} = "";
    return "";
}

sub get_OSgroup()
{
    if($Config{"osname"}=~/macos|darwin|rhapsody/i) {
        return "macos";
    }
    elsif($Config{"osname"}=~/freebsd|openbsd|netbsd/i) {
        return "bsd";
    }
    elsif($Config{"osname"}=~/haiku|beos/i) {
        return "beos";
    }
    elsif($Config{"osname"}=~/symbian|epoc/i) {
        return "symbian";
    }
    elsif($Config{"osname"}=~/win/i) {
        return "windows";
    }
    else {
        return $Config{"osname"};
    }
}

sub detectDisplay()
{
    my $DISPLAY_NUM=9; #default display number
    if(my $XPropCmd = get_CmdPath("xprop"))
    {# OK, found xprop, now use it to get a free display number
        foreach my $DNUM (9, 8, 7, 6, 5, 4, 3, 2, 10, 11, 12)
        {# try these display numbers only
            system("$XPropCmd -display :$DNUM".".0 -root >$TMP_DIR/null 2>&1");
            if($? ne 0) {
                # no properties found for this display, guess it is free
                $DISPLAY_NUM=$DNUM;
                last;
            }
        }
    }
    else {
        print STDERR "WARNING: can't find xprop\n";
    }
    return ":$DISPLAY_NUM.0";
}

sub runXvfb()
{
    my $Xvfb = get_CmdPath("Xvfb");
    if(not $Xvfb) {
        print STDERR "ERROR: can't find Xvfb\n";
        exit(1);
    }
    # Find a free display to use for Xvfb
    my $XT_DISPLAY = detectDisplay();
    my $TEST_DISPLAY=$XT_DISPLAY;
    my $running=0;
    if(my $PidofCmd = get_CmdPath("pidof")) {
        $running=`$PidofCmd Xvfb`;
        chomp($running);
    }
    else {
        print STDERR "WARNING: can't find pidof\n";
    }
    my $PsCmd = get_CmdPath("ps");
    if(not $running or $OSgroup!~/\A(linux|bsd)\Z/ or not $PsCmd)
    {
        print("starting X Virtual Frame Buffer on the display $TEST_DISPLAY\n");
        system("$Xvfb -screen 0 1024x768x24 $TEST_DISPLAY -ac +bs +kb -fp /usr/share/fonts/misc/ >$TMP_DIR/null 2>&1 & sleep 1");
        if($?) {
            print STDERR "ERROR: can't start Xvfb: $?\n";
            exit(1);
        }
        $ENV{"DISPLAY"}=$TEST_DISPLAY;
        $ENV{"G_SLICE"}="always-malloc";
        return 1;
    }
    else
    {
        #Xvfb is running, determine the display number
        my $CMD_XVFB=`$PsCmd -p "$running" -f | tail -n 1`;
        chomp($CMD_XVFB);
        $CMD_XVFB=~/(\:\d+\.0)/;
        $XT_DISPLAY = $1;
        $ENV{"DISPLAY"}=$XT_DISPLAY;
        $ENV{"G_SLICE"}="always-malloc";
        print("Xvfb is already running (display: $XT_DISPLAY), so it will be used.\n");
        return 0;
    }
}

sub stopXvfb($)
{
    return if($_[0]!=1);
    if(my $PidofCmd = get_CmdPath("pidof"))
    {
        my $pid = `$PidofCmd Xvfb`;
        chomp($pid);
        if($pid) {
            kill(9, $pid);
        }
    }
    else {
        print STDERR "WARNING: can't find pidof\n";
    }
}

sub parseTag($$)
{
    my ($CodeRef, $Tag) = @_;
    return "" if(not $CodeRef or not ${$CodeRef} or not $Tag);
    if(${$CodeRef}=~s/\<\Q$Tag\E\>((.|\n)+?)\<\/\Q$Tag\E\>//)
    {
        my $Content = $1;
        $Content=~s/\A[\n]+//g;
        while($Content=~s/\A([ \t]+[\n]+)//g){}
        $Content=~s/\A[\n]+//g;
        $Content=~s/\s+\Z//g;
        if($Content=~/\n/) {
            $Content = alignSpaces($Content);
        }
        else {
            $Content=~s/\A[ \t]+//g;
        }
        return $Content;
    }
    else {
        return "";
    }
}

sub add_os_spectypes()
{
    if($OSgroup eq "beos")
    {#http://www.haiku-os.org/legacy-docs/bebook/TheKernelKit_Miscellaneous.html
        readSpecTypes("
        <spec_type>
            <name>
                disable debugger in Haiku
            </name>
            <kind>
                common_env
            </kind>
            <global_code>
                #include <kernel/OS.h>
            </global_code>
            <init_code>
                disable_debugger(1);
            </init_code>
            <libs>
                libroot.so
            </libs>
            <associating>
                <except>
                    disable_debugger
                </except>
            </associating>
        </spec_type>");
    }
}

sub read_annotations_Splint()
{
    #reading interface specifications
    my (%HeaderInts, %InterfaceAnnotations,
    %TypeAnnotations, %ParamTypes, %HeaderTypes) = ();
    foreach my $Interface (keys(%Interface_Library))
    {
        if(my $Header = $CompleteSignature{$Interface}{"Header"})
        {
            $HeaderInts{$Header}{$Interface} = 1;
            foreach my $ParamPos (sort {int($a) <=> int($b)} keys(%{$CompleteSignature{$Interface}{"Param"}}))
            {
                $ParamTypes{$CompleteSignature{$Interface}{"Param"}{$ParamPos}{"type"}} = 1;
            }
        }
    }
    foreach my $Header (keys(%HeaderInts))
    {
        if(my ($Inc, $Path) = identify_header($Header))
        {
            my @Lines = split(/\n/, readFile($Path));
            foreach my $Interface (keys(%{$HeaderInts{$Header}}))
            {
                my $IntLine = $CompleteSignature{$Interface}{"Line"}-1;
                my $Section = $Lines[$IntLine];
                my $LineNum = $IntLine;
                while($LineNum<=$#Lines-1 and $Lines[$LineNum]!~/;/)
                {#forward search
                    $LineNum+=1;
                    $Section .= "\n".$Lines[$LineNum];
                }
                $LineNum = $IntLine-1;
                while($LineNum>=0 and $Lines[$LineNum]!~/;/)
                {#backward search
                    $Section = $Lines[$LineNum]."\n".$Section;
                    $LineNum-=1;
                }
                my $ShortName = $CompleteSignature{$Interface}{"ShortName"};
                if($Section=~/\W$ShortName\s*\(/)
                {
                    foreach my $ParamPos (sort {int($a) <=> int($b)} keys(%{$CompleteSignature{$Interface}{"Param"}}))
                    {
                        my $ParamName = $CompleteSignature{$Interface}{"Param"}{$ParamPos}{"name"};
                        if($Section=~/(,|\()\s*([^,()]+?)\W$ParamName\s*(,|\)|\*)/) {
                            my $ParamInfo = $2;
                            while($ParamInfo=~s/\/\*\s*\@\s*(returned|out|notnull|null)\s*\@\s*\*\///) {
                                $InterfaceAnnotations{$Interface}{"Param"}{$ParamPos}{$1} = 1;
                            }
                        }
                    }
                    if($Section=~/\/\*\s*\@\s*modifies\s*(.+?)\s*\@\s*\*\//) {
                        foreach my $Arg (split(/\s*,\s*/, $1)) {
                            $InterfaceAnnotations{$Interface}{"modifies"}{$Arg} = 1;
                        }
                    }
                    if($Section=~/\/\*\s*\@\s*requires\s*(.+?)\s*\@\s*\*\//) {
                        $InterfaceAnnotations{$Interface}{"requires"} = $1;
                    }
                    if($Section=~/\/\*\s*\@\s*ensures\s*(.+?)\s*\@\s*\*\//) {
                        $InterfaceAnnotations{$Interface}{"ensures"} = $1;
                    }
                }
            }
        }
    }
    foreach my $TypeId (keys(%ParamTypes))
    {
        if(my $Header = get_TypeHeader($TypeId)) {
            if($DependencyHeaders_All{$Header}) {
                $HeaderTypes{$Header}{$TypeId} = 1;
            }
        }
    }
    #reading data type specifications
    foreach my $Header (keys(%HeaderTypes))
    {
        if(my ($Inc, $Path) = identify_header($Header))
        {
            my @Lines = split(/\n/, readFile($Path));
            foreach my $TypeId (keys(%{$HeaderTypes{$Header}}))
            {
                my $TypeLine = $TypeDescr{$Tid_TDid{$TypeId}}{$TypeId}{"Line"}-1;
                my $Section = $Lines[$TypeLine];
                $Section .= $Lines[$TypeLine-1]."\n".$Section if($TypeLine>1);
                $Section .= "\n".$Lines[$TypeLine+1] if($TypeLine<=$#Lines-1);
                if($Section=~/\/\*\s*\@\s*(abstract|concrete)\s*\@\s*\*\//) {
                    $TypeAnnotations{$TypeId}{$1} = 1;
                }
            }
        }
    }
    foreach my $Interface (keys(%InterfaceAnnotations))
    {
        foreach my $ParamPos (sort {int($a) <=> int($b)} keys(%{$InterfaceAnnotations{$Interface}{"Param"}}))
        {
            my $ParamTypeId = $CompleteSignature{$Interface}{"Param"}{$ParamPos}{"type"};
            my $ParamName = $CompleteSignature{$Interface}{"Param"}{$ParamPos}{"name"};
            my %ParamAttr = %{$InterfaceAnnotations{$Interface}{"Param"}{$ParamPos}};
            if($ParamAttr{"out"} or $ParamAttr{"returned"}) {
                register_out_param($Interface, $ParamPos, $ParamName, $ParamTypeId);
            }
            if($ParamAttr{"notnull"}) {
                $Interface_PreCondition{$Interface}{"\$".($ParamPos+1)."!=0"} = 1;
            }
        }
        if(my $PreCondition = $InterfaceAnnotations{$Interface}{"requires"}) {
            $Interface_PreCondition{$Interface}{condition_to_template($Interface, $PreCondition)} = 1;
        }
        if(my $PostCondition = $InterfaceAnnotations{$Interface}{"ensures"}) {
            $Interface_PostCondition{$Interface}{condition_to_template($Interface, $PostCondition)} = 1;
        }
    }
    foreach my $TypeId (keys(%TypeAnnotations)) {
        if($TypeAnnotations{$TypeId}{"abstract"}) {
            $OpaqueTypes{get_TypeName($TypeId)} = 1;
        }
    }
}

sub condition_to_template($$$)
{
    my ($Interface, $Condition) = @_;
    return "" if(not $Interface or not $Condition);
    return "" if($Condition=~/(\W|\A)(maxSet|maxRead)(\W|\Z)/);
    foreach my $ParamPos (sort {int($b) <=> int($a)} keys(%{$CompleteSignature{$Interface}{"Param"}}))
    {
        my $ParamName = $CompleteSignature{$Interface}{"Param"}{$ParamPos}{"name"};
        if($Condition=~/(\A|\W)$ParamName(\Z|\W)/) {
            my $ParamNum = $ParamPos+1;
            $Condition=~s/(\A|\W)$ParamName(\Z|\W)/$1\$$ParamNum$2/g;
        }
    }
    $Condition=~s/\/\\/&&/g;
    return $Condition;
}

sub register_out_param($$$$)
{
    my ($Interface, $ParamPos, $ParamName, $ParamTypeId) = @_;
    $OutParamInterface_Pos{$Interface}{$ParamPos}=1;
    $Interface_OutParam{$Interface}{$ParamName}=1;
    $BaseType_PLevel_OutParam{get_FoundationTypeId($ParamTypeId)}{get_PointerLevel($Tid_TDid{$ParamTypeId}, $ParamTypeId)-1}{$Interface}=1;
    foreach my $TypeId (get_OutParamFamily($ParamTypeId, 0)) {
        $OutParam_Interface{$TypeId}{$Interface}=$ParamPos;
    }
}

sub cmpVersions($$)
{
    my ($First, $Second) = @_;
    if($First eq "" and $Second eq "") {
        return 0;
    }
    elsif($First eq ""
    or $Second eq "current") {
        return -1;
    }
    elsif($Second eq ""
    or $First eq "current") {
        return 1;
    }
    $First=~s/(\d)([a-z])/$1.$2/ig;
    $Second=~s/(\d)([a-z])/$1.$2/ig;
    $First=~s/[_~-]/./g;
    $Second=~s/[_~-]/./g;
    $First=~s/\A[.]+//g;
    $Second=~s/\A[.]+//g;
    $First=~s/\A[0]+([1-9]\d*)\Z/$1/g;
    $Second=~s/\A[0]+([1-9]\d*)\Z/$1/g;
    $First=~s/\A[0]+\Z/0/g;
    $Second=~s/\A[0]+\Z/0/g;
    if($First!~/\./ and $Second!~/\./) {
        return mixedCmp($_[0], $_[1]);
    }
    elsif($First!~/\./) {
        return cmpVersions($First.".0", $Second);
    }
    elsif($Second!~/\./) {
        return cmpVersions($First, $Second.".0");
    }
    else
    {
        my ($Part1, $Part2) = ();
        if($First =~ s/\A([^\.]+)\.//o) {
            $Part1 = $1;
        }
        if($Second =~ s/\A([^\.]+)\.//o) {
            $Part2 = $1;
        }
        if(my $CmpPartRes = mixedCmp($Part1, $Part2))
        {# compare first parts
            return $CmpPartRes;
        }
        else
        {# compare other parts
            return cmpVersions($First, $Second);
        }
    }
}

sub tokensCmp($$)
{
    my ($First, $Second) = @_;
    if($First eq $Second) {
        return 0;
    }
    elsif($First=~/\A[^a-z0-9]+\Z/i
    and $Second=~/\A[^a-z0-9]+\Z/i) {
        return 0;
    }
    elsif($First=~/\A[a-z]+\Z/i
    and $Second=~/\A[a-z]+\Z/i) {
        return trivialSymbCmp($_[0], $Second);
    }
    elsif($First=~/\A\d+\Z/
    and $Second=~/\A\d+\Z/) {
        return trivialNumerCmp($_[0], $Second);
    }
    elsif($First=~/\A[a-z]+\Z/i
    and $Second=~/\A\d+\Z/) {
        return -1;
    }
    elsif($First=~/\A\d+\Z/
    and $Second=~/\A[a-z]+\Z/i) {
        return 1;
    }
    elsif($First and $Second eq "") {
        return 1;
    }
    elsif($Second and $First eq "") {
        return -1;
    }
    else {
        return "undef";
    }
}

sub mixedCmp($$)
{
    my ($First, $Second) = @_;
    if($First eq $Second) {
        return 0;
    }
    while($First ne ""
    and $Second ne "")
    {
        my $First_Token = get_Token($First);
        my $Second_Token = get_Token($Second);
        my $CmpRes = tokensCmp($First_Token, $Second_Token);
        if($CmpRes eq "undef")
        {# safety lock
            return 0;
        }
        elsif($CmpRes != 0) {
            return $CmpRes;
        }
        else {
            $First =~ s/\A\Q$First_Token\E//g;
            $Second =~ s/\A\Q$Second_Token\E//g;
        }
    }
    if($First ne ""
    or $First eq "0") {
        return 1;
    }
    elsif($Second ne ""
    or $Second eq "0") {
        return -1;
    }
    else {
        return 0;
    }
}

sub get_Token($)
{
    if($_[0]=~/\A(\d+)[a-z]/i) {
        return $1;
    }
    elsif($_[0]=~/\A([a-z]+)\d/i) {
        return $1;
    }
    elsif($_[0]=~/\A(\d+)[^a-z0-9]/i) {
        return $1;
    }
    elsif($_[0]=~/\A([a-z]+)[^a-z0-9]/i) {
        return $1;
    }
    elsif($_[0]=~/\A([^a-z0-9]+)/i) {
        return $1;
    }
    else {
        return $_[0];
    }
}

sub trivialSymbCmp($$)
{
    if($_[0] gt $_[1]) {
        return 1;
    }
    elsif($_[0] eq $_[1]) {
        return 0;
    }
    else {
        return -1;
    }
}

sub trivialNumerCmp($$)
{
    if(int($_[0]) > int($_[1])) {
        return 1;
    }
    elsif($_[0] eq $_[1]) {
        return 0;
    }
    else {
        return -1;
    }
}

sub verify_version($$)
{
    my ($Version, $Constraint) = @_;
    if($Constraint=~/>=\s*(.+)/) {
        return (cmpVersions($Version,$1)!=-1);
    }
    elsif($Constraint=~/<=\s*(.+)/) {
        return (cmpVersions($Version,$1)!=1);
    }
    else {
        return (cmpVersions($Version, $Constraint)==0);
    }
}

sub numToStr($)
{
    my $Number = int($_[0]);
    if($Number>3) {
        return $Number."th";
    }
    elsif($Number==1) {
        return "1st";
    }
    elsif($Number==2) {
        return "2nd";
    }
    elsif($Number==3) {
        return "3rd";
    }
    else {
        return $Number;
    }
}

sub readSpecTypes($)
{
    my $Package = $_[0];
    return if(not $Package);
    $Package=~s/\/\*(.|\n)+?\*\///g;#removing C++ comments
    $Package=~s/<\!--(.|\n)+?-->//g;#removing XML comments
    if($Package!~/<collection>/ or $Package!~/<\/collection>/)
    {# add <collection> tag (support for old spectype packages)
        $Package = "<collection>\n".$Package."\n</collection>";
    }
    while(my $Collection = parseTag(\$Package, "collection"))
    {
        # verifying library version
        my $Collection_Copy = $Collection;
        while(parseTag(\$Collection_Copy, "spec_type")){};
        if(my $Collection_VersionConstraints = parseTag(\$Collection_Copy, "lib_version"))
        {
            my $Verified = 1;
            foreach my $Constraint (split(/\n/, $Collection_VersionConstraints))
            {
                $Constraint=~s/\A\s+|\s+\Z//g;
                if(not verify_version($Descriptor{"Version"}, $Constraint)) {
                    $Verified=0;
                    last;
                }
            }
            next if(not $Verified);
        }
        # importing specialized types
        while(my $SpecType = parseTag(\$Collection, "spec_type"))
        {
            if(my $SpecType_VersionConstraints = parseTag(\$SpecType, "lib_version"))
            {
                my $Verified = 1;
                foreach my $Constraint (split(/\n/, $SpecType_VersionConstraints))
                {
                    $Constraint=~s/\A\s+|\s+\Z//g;
                    if(not verify_version($Descriptor{"Version"}, $Constraint)) {
                        $Verified=0;
                        last;
                    }
                }
                next if(not $Verified);
            }
            $ST_ID+=1;
            my (%Attr, %DataTypes) = ();
            $Attr{"Kind"} = parseTag(\$SpecType, "kind");
            $Attr{"Kind"} = "normal" if(not $Attr{"Kind"});
            foreach my $DataType (split(/\n/, parseTag(\$SpecType, "data_type")),
            split(/\n/, parseTag(\$SpecType, "data_types")))
            {# data_type==data_types, support of <= 1.5 versions
                $DataTypes{$DataType} = 1;
                if(not get_TypeIdByName($DataType)) {
                    print STDERR "\nERROR: unknown data type \'$DataType\' in one of the \'".$Attr{"Kind"}."\' spectypes, try to define it more exactly\n";
                }
            }
            if(not keys(%DataTypes) and $Attr{"Kind"}=~/\A(normal|common_param|common_retval)\Z/) {
                print STDERR "\nERROR: missed \'data_type\' attribute in one of the \'".$Attr{"Kind"}."\' spectypes\n";
                next;
            }
            $Attr{"Name"} = parseTag(\$SpecType, "name");
            $Attr{"Value"} = parseTag(\$SpecType, "value");
            $Attr{"PreCondition"} = parseTag(\$SpecType, "pre_condition");
            $Attr{"PostCondition"} = parseTag(\$SpecType, "post_condition");
            if(not $Attr{"PostCondition"})
            {# constraint==post_condition, support of <= 1.6 versions
                $Attr{"PostCondition"} = parseTag(\$SpecType, "constraint");
            }
            $Attr{"InitCode"} = parseTag(\$SpecType, "init_code");
            $Attr{"DeclCode"} = parseTag(\$SpecType, "decl_code");
            $Attr{"FinalCode"} = parseTag(\$SpecType, "final_code");
            $Attr{"GlobalCode"} = parseTag(\$SpecType, "global_code");
            foreach my $Lib (split(/\n/, parseTag(\$SpecType, "libs"))) {
                $Attr{"Libs"}{$Lib} = 1;
            }
            if($Attr{"Kind"} eq "common_env") {
                $Common_SpecEnv{$ST_ID} = 1;
            }
            while(my $Associating = parseTag(\$SpecType, "associating"))
            {
                my (%Interfaces, %Except) = ();
                foreach my $Interface (split(/\n/, parseTag(\$Associating, "interfaces")),
                split(/\n/, parseTag(\$Associating, "symbols")))
                {
                    $Interface=~s/\A\s+|\s+\Z//g;
                    $Interfaces{$Interface} = 1;
                    $Common_SpecType_Exceptions{$Interface}{$ST_ID} = 0;
                    if($Interface=~/\*/) {
                        $Interface=~s/\*/.*/;
                        foreach my $Int (keys(%CompleteSignature))
                        {
                            if($Int=~/\A$Interface\Z/) {
                                $Common_SpecType_Exceptions{$Int}{$ST_ID} = 0;
                                $Interfaces{$Interface} = 1;
                            }
                        }
                    }
                    elsif(not defined $CompleteSignature{$Interface}
                    or not $CompleteSignature{$Interface}{"ShortName"}) {
                        print STDERR "\nWARNING: unknown symbol $Interface\n";
                    }
                }
                foreach my $Interface (split(/\n/, parseTag(\$Associating, "except")))
                {
                    $Interface=~s/\A\s+|\s+\Z//g;
                    $Except{$Interface} = 1;
                    $Common_SpecType_Exceptions{$Interface}{$ST_ID} = 1;
                    if($Interface=~/\*/) {
                        $Interface=~s/\*/.*/;
                        foreach my $Int (keys(%CompleteSignature))
                        {
                            if($Int=~/\A$Interface\Z/) {
                                $Common_SpecType_Exceptions{$Int}{$ST_ID} = 1;
                                $Except{$Int} = 1;
                            }
                        }
                    }
                }
                if($Attr{"Kind"} eq "env") {
                    foreach my $Interface (keys(%Interfaces)) {
                        next if($Except{$Interface});
                        $InterfaceSpecType{$Interface}{"SpecEnv"} = $ST_ID;
                    }
                }
                else
                {
                    foreach my $Link (split(/\n/, parseTag(\$Associating, "links").parseTag(\$Associating, "param_num")))
                    {
                        $Link=~s/\A\s+|\s+\Z//g;
                        if(lc($Link)=~/\Aparam(\d+)\Z/)
                        {
                            my $Param_Num = $1;
                            foreach my $Interface (keys(%Interfaces))
                            {
                                next if($Except{$Interface});
                                if(defined $InterfaceSpecType{$Interface}{"SpecParam"}{$Param_Num - 1}) {
                                    print STDERR "\nWARNING: more than one spectypes have been linked to ".numToStr($Param_Num)." parameter of $Interface\n";
                                }
                                $InterfaceSpecType{$Interface}{"SpecParam"}{$Param_Num - 1} = $ST_ID;
                            }
                        }
                        elsif(lc($Link)=~/\Aobject\Z/)
                        {
                            foreach my $Interface (keys(%Interfaces))
                            {
                                next if($Except{$Interface});
                                if(defined $InterfaceSpecType{$Interface}{"SpecObject"}) {
                                    print STDERR "\nWARNING: more than one spectypes have been linked to calling object of $Interface\n";
                                }
                                $InterfaceSpecType{$Interface}{"SpecObject"} = $ST_ID;
                            }
                        }
                        elsif(lc($Link)=~/\Aretval\Z/)
                        {
                            foreach my $Interface (keys(%Interfaces))
                            {
                                next if($Except{$Interface});
                                if(defined $InterfaceSpecType{$Interface}{"SpecReturn"}) {
                                    print STDERR "\nWARNING: more than one spectypes have been linked to return value of $Interface\n";
                                }
                                $InterfaceSpecType{$Interface}{"SpecReturn"} = $ST_ID;
                            }
                        }
                        else {
                            print STDERR "\nWARNING: unrecognized link \'$Link\' in one of the \'".$Attr{"Kind"}."\' spectypes\n";
                        }
                    }
                    foreach my $Name (split(/\n/, parseTag(\$Associating, "param_name")))
                    {
                        $Name=~s/\A\s+|\s+\Z//g;
                        if(keys(%Interfaces))
                        {
                            foreach my $Interface (keys(%Interfaces))
                            {
                                next if($Except{$Interface});
                                foreach my $ParamPos (keys(%{$CompleteSignature{$Interface}{"Param"}}))
                                {
                                    if($Name eq $CompleteSignature{$Interface}{"Param"}{$ParamPos}{"name"}) {
                                        $InterfaceSpecType{$Interface}{"SpecParam"}{$ParamPos} = $ST_ID;
                                    }
                                }
                            }
                        }
                        else
                        {
                            foreach my $Interface (keys(%CompleteSignature))
                            {
                                next if($Except{$Interface});
                                foreach my $ParamPos (keys(%{$CompleteSignature{$Interface}{"Param"}}))
                                {
                                    if($Name eq $CompleteSignature{$Interface}{"Param"}{$ParamPos}{"name"})
                                    {
                                        my $TypeId_Param = $CompleteSignature{$Interface}{"Param"}{$ParamPos}{"type"};
                                        my $FTypeId_Param = get_FoundationTypeId($TypeId_Param);
                                        my $FTypeType_Param = get_TypeType($FTypeId_Param);
                                        my $FTypeName_Param = get_TypeName($FTypeId_Param);
                                        foreach my $DataType (keys(%DataTypes))
                                        {
                                            my $TypeId = get_TypeIdByName($DataType);
                                            if(my $FTypeId = get_FoundationTypeId($TypeId) and $FTypeId_Param) {
                                                if($FTypeType_Param eq "Intrinsic"?$TypeId==$TypeId_Param:$FTypeId==$FTypeId_Param) {
                                                    $InterfaceSpecType{$Interface}{"SpecParam"}{$ParamPos} = $ST_ID;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            if($Attr{"Kind"}=~/\A(common_param|common_retval)\Z/) {
                foreach my $DataType (keys(%DataTypes)) {
                    $Attr{"DataType"} = $DataType;
                    %{$SpecType{$ST_ID}} = %Attr;
                    $ST_ID+=1;
                }
            }
            elsif($Attr{"Kind"} eq "normal") {
                $Attr{"DataType"} = (keys(%DataTypes))[0];
                %{$SpecType{$ST_ID}} = %Attr;
            }
            else {
                %{$SpecType{$ST_ID}} = %Attr;
            }
        }
    }
}

sub createDescriptor_Rpm_Deb($)
{
    my $VirtualPath = $_[0];
    return if(not $VirtualPath or not -e $VirtualPath);
    my $IncDir = "none";
    $IncDir = $VirtualPath."/usr/include/" if(-d $VirtualPath."/usr/include/");
    $IncDir = $VirtualPath."/include/" if(-d $VirtualPath."/include/");
    my $LibDir = "none";
    $LibDir = $VirtualPath."/lib64/" if(-d $VirtualPath."/lib64/");
    $LibDir = $VirtualPath."/usr/lib64/" if(-d $VirtualPath."/usr/lib64/");
    $LibDir = $VirtualPath."/lib/" if(-d $VirtualPath."/lib/");
    $LibDir = $VirtualPath."/usr/lib/" if(-d $VirtualPath."/usr/lib/");
    if($IncDir ne "none")
    {
        my @Objects = ();
        foreach my $Path (cmd_find($LibDir,"f",".*\\.$LIB_EXT\[0-9.]*","")) {
            push(@Objects, $Path);
        }
        $LibDir = "none" if($#Objects==-1);
    }
    if($IncDir eq "none" and $LibDir eq "none") {
        print STDERR "ERROR: can't find headers and $SHARED libraries in the package\n";
        exit(1);
    }
    readDescriptor("
      <version>
          ".$TargetVersion."
      </version>

      <headers>
          $IncDir
      </headers>

      <libs>
          $LibDir
      </libs>");
}

sub checkVersionNum($)
{
    my $Path = $_[0];
    if($TargetVersion) {
        return $TargetVersion;
    }
    my $UsedAltDescr = 0;
    foreach my $Part (split(/\s*,\s*/, $Path))
    {
        if($Part=~/\.$LIB_EXT[\d\.\-\_]*\Z/i
        or is_header($Part, 1) or -d $Part) {
            $UsedAltDescr = 1;
            if(my $VerNum = readStringVersion($Part)) {
                $TargetVersion = $VerNum;
                print STDERR "WARNING: set version number to $VerNum (use -vnum <number> option to change it)\n";
                return $TargetVersion;
            }
        }
    }
    if($UsedAltDescr) {
        print STDERR "ERROR: version number is not set (use -vnum <number> option)\n";
        exit(1);
    }
}

sub readStringVersion($)
{
    my $Str = $_[0];
    return "" if(not $Str);
    $Str=~s/\Q$TargetLibraryName\E//g;
    if($Str=~/(\/|\\|\w|\A)[\-\_]*(\d+[\d\.\-]+\d+|\d+)/
    or $Str=~/(\.$LIB_EXT)\.([\d\.\-\_]+)\Z/i) {
        return $2;
    }
    return "";
}

sub createDescriptor($)
{
    my $Path = $_[0];
    return if(not $Path or not -e $Path);
    if($Path=~/\.deb\Z/i)
    {# Deb "devel" package
        my $ArCmd = get_CmdPath("ar");
        if(not $ArCmd) {
            print STDERR "ERROR: can't find \"ar\"\n";
            exit(1);
        }
        my $VirtualPath = "$TMP_DIR/package/";
        mkpath($VirtualPath);
        system("cd $VirtualPath && $ArCmd p ".abs_path($Path)." data.tar.gz | tar zx");
        if($?) {
            print STDERR "ERROR: can't extract package\n";
            exit(1);
        }
        if(not $TargetVersion)
        {# auto detect version of Deb package
            system("cd $VirtualPath && $ArCmd p ".abs_path($Path)." control.tar.gz | tar zx");
            if(-f "$VirtualPath/control")
            {
                my $ControlInfo = `grep Version $VirtualPath/control`;
                if($ControlInfo=~/Version\s*:\s*([\d\.\-\_]*\d)/) {
                    $TargetVersion = $1;
                }
            }
        }
        if(not $TargetVersion)
        {
            if($Path=~/_([\d\.\-\_]*\d)(.+?)\.deb\Z/i)
            {# auto detect version of Deb package
                $TargetVersion = $1;
            }
        }
        if(not $TargetVersion) {
            print STDERR "ERROR: specify version number (-vnum <number> option)\n";
            exit(1);
        }
        createDescriptor_Rpm_Deb($VirtualPath);
    }
    elsif($Path=~/\.rpm\Z/i)
    {# RPM "devel" package
        my $Rpm2CpioCmd = get_CmdPath("rpm2cpio");
        if(not $Rpm2CpioCmd) {
            print STDERR "ERROR: can't find \"rpm2cpio\"\n";
            exit(1);
        }
        my $CpioCmd = get_CmdPath("cpio");
        if(not $CpioCmd) {
            print STDERR "ERROR: can't find \"cpio\"\n";
            exit(1);
        }
        my $VirtualPath = "$TMP_DIR/package/";
        mkpath($VirtualPath);
        system("cd $VirtualPath && $Rpm2CpioCmd ".abs_path($Path)." | cpio -id --quiet");
        if($?) {
            print STDERR "ERROR: can't extract package\n";
            exit(1);
        }
        my $RpmCmd = get_CmdPath("rpm");
        if(not $RpmCmd) {
            print STDERR "ERROR: can't find \"rpm\"\n";
            exit(1);
        }
        if(not $TargetVersion) {
            $TargetVersion = `rpm -qp --queryformat \%{version} $Path 2>$TMP_DIR/null`;
        }
        if(not $TargetVersion)
        {
            if($Path=~/-([\d\.\-\_]*\d)(.+?)\.rpm\Z/i)
            {# auto detect version of Rpm package
                $TargetVersion = $1;
            }
        }
        if(not $TargetVersion) {
            print STDERR "ERROR: specify version number (-vnum <number> option)\n";
            exit(1);
        }
        createDescriptor_Rpm_Deb($VirtualPath);
    }
    elsif(-d $Path)
    {# directory with headers files and shared objects
        readDescriptor("
          <version>
              ".$TargetVersion."
          </version>

          <headers>
              $Path
          </headers>

          <libs>
              $Path
          </libs>");
    }
    else
    {# files
        if(is_header($Path, 1))
        {# header file
            readDescriptor("
              <version>
                  ".$TargetVersion."
              </version>

              <headers>
                  $Path
              </headers>

              <libs>
                  none
              </libs>");
        }
        elsif($Path=~/\.$LIB_EXT[\d\.\-\_]*\Z/)
        {# shared object
            readDescriptor("
              <version>
                  ".$TargetVersion."
              </version>

              <headers>
                  none
              </headers>

              <libs>
                  $Path
              </libs>");
        }
        else
        {# standard XML-descriptor
            readDescriptor(readFile($Path));
        }
    }
}

sub readDescriptor($)
{
    my $Content = $_[0];
    if(not $Content) {
        print STDERR "ERROR: library descriptor is empty\n";
        exit(1);
    }
    if($Content!~/\</) {
        print STDERR "ERROR: descriptor should be one of the following: XML-descriptor or directory with headers and $SHARED libraries.\n";
        exit(1);
    }
    $Content=~s/\/\*(.|\n)+?\*\///g;
    $Content=~s/<\!--(.|\n)+?-->//g;
    $Descriptor{"Version"} = parseTag(\$Content, "version");
    $Descriptor{"Version"} = $TargetVersion if($TargetVersion);
    if(not $Descriptor{"Version"})
    {
        print STDERR "ERROR: version in the descriptor is not specified (section <version>)\n";
        exit(1);
    }
    if($Content=~/{RELPATH}/)
    {
        if($RelativeDirectory)  {
            $Content =~ s/{RELPATH}/$RelativeDirectory/g;
        }
        else {
            print STDERR "ERROR: you have not specified -relpath option, but the descriptor contains {RELPATH} macro\n";
            exit(1);
        }
    }
    if(my $DHeaders = parseTag(\$Content, "headers"))
    {
        if(lc($DHeaders) ne "none") {
            $Descriptor{"Headers"} .= "\n".$DHeaders;
        }
    }
    if(not $Descriptor{"Headers"}) {
        print STDERR "ERROR: header files in the descriptor were not specified (section <headers>)\n";
        exit(1);
    }
    if(my $DObjects = parseTag(\$Content, "libs"))
    {
        if(lc($DObjects) ne "none") {
            $Descriptor{"Libs"} .= "\n".$DObjects;
        }
    }
    if(not $Descriptor{"Libs"} and not $CheckHeadersOnly)
    {
        print STDERR "ERROR: $SHARED libraries in the descriptor were not specified (section <libs>)\n";
        exit(1);
    }
    foreach my $Dest (split(/\n/, parseTag(\$Content, "include_paths")))
    {
        $Dest=~s/\A\s+|\s+\Z//g;
        next if(not $Dest);
        $Descriptor{"IncludePaths"}{$Dest} = 1;
    }
    foreach my $Dest (split(/\n/, parseTag(\$Content, "add_include_paths")))
    {
        $Dest=~s/\A\s+|\s+\Z//g;
        next if(not $Dest);
        $Descriptor{"AddIncludePaths"}{$Dest} = 1;
    }
    $Descriptor{"GccOptions"} = parseTag(\$Content, "gcc_options");
    foreach my $Option (split(/\n/, $Descriptor{"GccOptions"}))
    {
        $Option=~s/\A\s+|\s+\Z//g;
        next if(not $Option);
        if($Option=~/\.$LIB_EXT[0-9.]*\Z/) {
            $CompilerOptions_Libs{$Option} = 1;
        }
        else {
            $CompilerOptions_Headers .= " ".$Option;
        }
    }
    $Descriptor{"OutParams"} = parseTag(\$Content, "out_params");
    foreach my $IntParam (split(/\n/, $Descriptor{"OutParams"}))
    {
        $IntParam=~s/\A\s+|\s+\Z//g;
        next if(not $IntParam);
        if($IntParam=~/(.+)(:|;)(.+)/) {
            $UserDefinedOutParam{$1}{$3} = 1;
        }
    }
    $Descriptor{"LibsDepend"} = parseTag(\$Content, "libs_depend");
    foreach my $Dep (split(/\n/, $Descriptor{"LibsDepend"}))
    {
        $Dep=~s/\A\s+|\s+\Z//g;
        next if(not $Dep);
        if(not -f $Dep)
        {
            print STDERR "ERROR: can't access \'$Dep\': no such file\n";
            next;
        }
        $Dep = abs_path($Dep) if($Dep!~/\A(\/|\w+:[\/\\])/);
        $CompilerOptions_Libs{$Dep} = 1;
    }
    $Descriptor{"SkipHeaders"} = parseTag(\$Content, "skip_headers");
    foreach my $Name (split(/\n/, $Descriptor{"SkipHeaders"}))
    {
        $Name=~s/\A\s+|\s+\Z//g;
        next if(not $Name);
        if($Name=~/\*|[\/\\]/)
        {
            $Name=~s/\*/.*/g;
            $SkipHeaders_Pattern{$Name} = 1;
        }
        else {
            $SkipHeaders{$Name} = 1;
        }
    }
    $Descriptor{"SkipLibs"} = parseTag(\$Content, "skip_libs");
    foreach my $Name (split(/\n/, $Descriptor{"SkipLibs"}))
    {
        $Name=~s/\A\s+|\s+\Z//g;
        next if(not $Name);
        if($Name=~/\*|[\/\\]/)
        {
            $Name=~s/\*/.*/g;
            $SkipLibs_Pattern{$Name} = 1;
        }
        else {
            $SkipLibs{$Name} = 1;
        }
    }
    if(my $DDefines = parseTag(\$Content, "defines"))
    {
        $Descriptor{"Defines"} .= "\n".$DDefines;
    }
    foreach my $Order (split(/\n/, parseTag(\$Content, "include_order")))
    {
        $Order=~s/\A\s+|\s+\Z//g;
        next if(not $Order);
        if($Order=~/\A(.+):(.+)\Z/)
        {
            $Include_Order{$1} = $2;
            $Include_RevOrder{$2} = $1;
        }
    }
    foreach my $Warning (split(/\n/, parseTag(\$Content, "skip_warnings")))
    {
        $Warning=~s/\A\s+|\s+\Z//g;
        next if(not $Warning);
        if($Warning=~s/\*/.*/g) {
            $SkipWarnings_Pattern{$Warning} = 1;
        }
        else {
            $SkipWarnings{$Warning} = 1;
        }
    }
    foreach my $Type_Name (split(/\n/, parseTag(\$Content, "opaque_types")))
    {
        $Type_Name=~s/\A\s+|\s+\Z//g;
        next if(not $Type_Name);
        $OpaqueTypes{$Type_Name} = 1;
    }
    foreach my $Interface_Name (split(/\n/, parseTag(\$Content, "skip_interfaces")),
    split(/\n/, parseTag(\$Content, "skip_symbols")))
    {
        $Interface_Name=~s/\A\s+|\s+\Z//g;
        next if(not $Interface_Name);
        if($Interface_Name=~s/\*/.*/g) {
            $SkipInterfaces_Pattern{$Interface_Name} = 1;
        }
        else {
            $SkipInterfaces{$Interface_Name} = 1;
        }
    }
    if(my $DIncPreamble = parseTag(\$Content, "include_preamble"))
    {
        $Descriptor{"IncludePreamble"} .= "\n".$DIncPreamble;
    }
    while(my $LibGroupTag = parseTag(\$Content, "libgroup"))
    {
        my $LibGroupName = parseTag(\$LibGroupTag, "name");
        foreach my $Interface (split(/\n/, parseTag(\$LibGroupTag, "interfaces")),
        split(/\n/, parseTag(\$LibGroupTag, "symbols")))
        {
            $Interface=~s/\A\s+|\s+\Z//g;
            $LibGroups{$LibGroupName}{$Interface} = 1;
            $Interface_LibGroup{$Interface}=$LibGroupName;
        }
    }
    if(keys(%Interface_LibGroup))
    {
        if(keys(%InterfacesList)) {
            %InterfacesList=();
        }
        foreach my $LibGroup (keys(%LibGroups))
        {
            foreach my $Interface (keys(%{$LibGroups{$LibGroup}})) {
                $InterfacesList{$Interface}=1;
            }
        }
    }
}

sub getArch()
{
    my $Arch = $ENV{"CPU"};
    if(not $Arch and my $UnameCmd = get_CmdPath("uname"))
    {
        $Arch = `$UnameCmd -m`;
        chomp($Arch);
        if(not $Arch) {
            $Arch = `$UnameCmd -p`;
            chomp($Arch);
        }
    }
    $Arch = $Config{"archname"} if(not $Arch);
    $Arch = "x86" if($Arch=~/i[3-7]86/);
    if($OSgroup eq "windows")
    {
        $Arch = "x86" if($Arch=~/win32/i);
        $Arch = "x86-64" if($Arch=~/win64/i);
    }
    $Arch=~s/\-multi\-thread(-|\Z)//g;
    return $Arch;
}

sub get_Summary()
{
    my $Summary = "<h2 class='title2'>Summary</h2><hr/>";
    $Summary .= "<table cellpadding='3' class='summary'>";
    my $Verdict = "";
    if($ResultCounter{"Run"}{"Fail"} > 0)
    {
        $Verdict = "<span style='color:Red;'><b>Test Failed</b></span>";
        $STAT_FIRST_LINE .= "verdict:failed;";
    }
    else
    {
        $Verdict = "<span style='color:Green;'><b>Test Passed</b></span>";
        $STAT_FIRST_LINE .= "verdict:passed;";
    }
    $Summary .= "<tr><td class='table_header summary_item'>Total tests</td><td align='right' class='summary_item_value'>".($ResultCounter{"Run"}{"Success"}+$ResultCounter{"Run"}{"Fail"})."</td></tr>";
    $STAT_FIRST_LINE .= "total:".($ResultCounter{"Run"}{"Success"}+$ResultCounter{"Run"}{"Fail"}).";";
    my $Success_Tests_Link = "0";
    $Success_Tests_Link = $ResultCounter{"Run"}{"Success"} if($ResultCounter{"Run"}{"Success"}>0);
    $STAT_FIRST_LINE .= "passed:".$ResultCounter{"Run"}{"Success"}.";";
    my $Failed_Tests_Link = "0";
    $Failed_Tests_Link = "<a href='#Failed_Tests' style='color:Blue;'>".$ResultCounter{"Run"}{"Fail"}."</a>" if($ResultCounter{"Run"}{"Fail"}>0);
    $STAT_FIRST_LINE .= "failed:".$ResultCounter{"Run"}{"Fail"}.";";
    $Summary .= "<tr><td class='table_header summary_item'>Passed / Failed tests</td><td align='right' class='summary_item_value'>$Success_Tests_Link / $Failed_Tests_Link</td></tr>";
    if($ResultCounter{"Run"}{"Warnings"}>0)
    {
        my $Warnings_Link = "<a href='#Warnings' style='color:Blue;'>".$ResultCounter{"Run"}{"Warnings"}."</a>";
        $Summary .= "<tr><td class='table_header summary_item'>Warnings</td><td align='right' class='summary_item_value'>$Warnings_Link</td></tr>";
    }
    $STAT_FIRST_LINE .= "warnings:".$ResultCounter{"Run"}{"Warnings"};
    $Summary .= "<tr><td class='table_header summary_item'>Verdict</td><td align='right'>$Verdict</td></tr>";
    $Summary .= "</table>\n";
    return $Summary;
}

sub get_Problem_Summary()
{
    my $Problem_Summary = "";
    my %ProblemType_Interface = ();
    foreach my $Interface (keys(%RunResult))
    {
        next if($RunResult{$Interface}{"Warnings"});
        $ProblemType_Interface{$RunResult{$Interface}{"Type"}}{$Interface} = 1;
    }
    my $ColSpan = 1;
    my $SignalException = ($OSgroup eq "windows")?"Exception":"Signal";
    my $ProblemType = "Received_".$SignalException;
    if(keys(%{$ProblemType_Interface{$ProblemType}}))
    {
        my %SignalName_Interface = ();
        foreach my $Interface (keys(%{$ProblemType_Interface{"Received_$SignalException"}})) {
            $SignalName_Interface{$RunResult{$Interface}{"Value"}}{$Interface} = 1;
        }
        if(keys(%SignalName_Interface)==1)
        {
            my $SignalName = (keys(%SignalName_Interface))[0];
            my $Amount = keys(%{$SignalName_Interface{$SignalName}});
            my $Link = "<a href=\'#".$ProblemType."_".$SignalName."\' style='color:Blue;'>$Amount</a>";
            $STAT_FIRST_LINE .= lc($ProblemType."_".$SignalName.":$Amount;");
            $Problem_Summary .= "<tr><td class='table_header summary_item'>Received ".lc($SignalException)." $SignalName</td><td align='right' class='summary_item_value'>$Link</td></tr>";
        }
        elsif(keys(%SignalName_Interface)>1)
        {
            $Problem_Summary .= "<tr><td class='table_header summary_item' rowspan='".keys(%SignalName_Interface)."'>Received ".lc($SignalException)."</td>";
            my $num = 1;
            foreach my $SignalName (sort keys(%SignalName_Interface))
            {
                my $Amount = keys(%{$SignalName_Interface{$SignalName}});
                my $Link = "<a href=\'#".$ProblemType."_".$SignalName."\' style='color:Blue;'>$Amount</a>";
                $STAT_FIRST_LINE .= lc($ProblemType."_".$SignalName.":$Amount;");
                $Problem_Summary .= (($num!=1)?"<tr>":"")."<td class='table_header summary_item'>$SignalName</td><td align='right' class='summary_item_value'>$Link</td></tr>";
                $num+=1;
            }
            $ColSpan = 2;
        }
    }
    if(keys(%{$ProblemType_Interface{"Exited_With_Value"}}))
    {
        my %ExitValue_Interface = ();
        foreach my $Interface (keys(%{$ProblemType_Interface{"Exited_With_Value"}}))
        {
            $ExitValue_Interface{$RunResult{$Interface}{"Value"}}{$Interface} = 1;
        }
        if(keys(%ExitValue_Interface)==1)
        {
            my $ExitValue = (keys(%ExitValue_Interface))[0];
            my $Amount = keys(%{$ExitValue_Interface{$ExitValue}});
            my $Link = "<a href=\'#Exited_With_Value_$ExitValue\' style='color:Blue;'>$Amount</a>";
            $STAT_FIRST_LINE .= lc("Exited_With_Value_$ExitValue:$Amount;");
            $Problem_Summary .= "<tr><td class='table_header summary_item' colspan=\'$ColSpan\'>Exited with value \"$ExitValue\"</td><td align='right' class='summary_item_value'>$Link</td></tr>";
        }
        elsif(keys(%ExitValue_Interface)>1)
        {
            $Problem_Summary .= "<tr><td class='table_header summary_item' rowspan='".keys(%ExitValue_Interface)."'>Exited with value</td>";
            foreach my $ExitValue (sort keys(%ExitValue_Interface))
            {
                my $Amount = keys(%{$ExitValue_Interface{$ExitValue}});
                my $Link = "<a href=\'#Exited_With_Value_$ExitValue\' style='color:Blue;'>$Amount</a>";
                $STAT_FIRST_LINE .= lc("Exited_With_Value_$ExitValue:$Amount;");
                $Problem_Summary .= "<td class='table_header summary_item'>\"$ExitValue\"</td><td align='right' class='summary_item_value'>$Link</td></tr>";
            }
            $Problem_Summary .= "</tr>";
            $ColSpan = 2;
        }
    }
    if(keys(%{$ProblemType_Interface{"Hanged_Execution"}}))
    {
        my $Amount = keys(%{$ProblemType_Interface{"Hanged_Execution"}});
        my $Link = "<a href=\'#Hanged_Execution\' style='color:Blue;'>$Amount</a>";
        $STAT_FIRST_LINE .= "hanged_execution:$Amount;";
        $Problem_Summary .= "<tr><td class='table_header summary_item' colspan=\'$ColSpan\'>Hanged execution</td><td align='right' class='summary_item_value'>$Link</td></tr>";
    }
    if(keys(%{$ProblemType_Interface{"Requirement_Failed"}}))
    {
        my $Amount = keys(%{$ProblemType_Interface{"Requirement_Failed"}});
        my $Link = "<a href=\'#Requirement_Failed\' style='color:Blue;'>$Amount</a>";
        $STAT_FIRST_LINE .= "requirement_failed:$Amount;";
        $Problem_Summary .= "<tr><td class='table_header summary_item' colspan=\'$ColSpan\'>Requirement failed</td><td align='right' class='summary_item_value'>$Link</td></tr>";
    }
    if(keys(%{$ProblemType_Interface{"Other_Problems"}}))
    {
        my $Amount = keys(%{$ProblemType_Interface{"Other_Problems"}});
        my $Link = "<a href=\'#Other_Problems\' style='color:Blue;'>$Amount</a>";
        $STAT_FIRST_LINE .= "other_problems:$Amount;";
        $Problem_Summary .= "<tr><td class='table_header summary_item' colspan=\'$ColSpan\'>Other problems</td><td align='right' class='summary_item_value'>$Link</td></tr>";
    }
    if($Problem_Summary)
    {
        $Problem_Summary = "<h2 class='title2'>Problem Summary</h2><hr/>"."<table cellpadding='3' class='summary'>".$Problem_Summary."</table>\n";
        return $Problem_Summary;
    }
    else
    {
        return "";
    }
}

sub get_Report_Header()
{
    my $Report_Header = "<h1 class='title1'>Test results for the <span style='color:Blue;'>$TargetLibraryFullName</span>-<span style='color:Blue;'>".$Descriptor{"Version"}."</span> library on <span style='color:Blue;'>".getArch()."</span></h1>\n";
    return $Report_Header;
}

sub get_TestSuite_Header()
{
    my $Report_Header = "<h1 class='title1'>Test suite for the <span style='color:Blue;'>$TargetLibraryFullName</span>-<span style='color:Blue;'>".$Descriptor{"Version"}."</span> library on <span style='color:Blue;'>".getArch()."</span></h1>\n";
    return $Report_Header;
}

sub get_problem_title($$)
{
    my ($ProblemType, $Value) = @_;
    if($ProblemType eq "Received_Signal") {
        return "Received signal $Value";
    }
    elsif($ProblemType eq "Received_Exception") {
        return "Received exception $Value";
    }
    elsif($ProblemType eq "Exited_With_Value") {
        return "Exited with value \"$Value\"";
    }
    elsif($ProblemType eq "Requirement_Failed") {
        return "Requirement failed";
    }
    elsif($ProblemType eq "Hanged_Execution") {
        return "Hanged execution";
    }
    elsif($ProblemType eq "Unexpected_Output") {
        return "Unexpected Output";
    }
    elsif($ProblemType eq "Other_Problems") {
        return "Other problems";
    }
    else {
        return "";
    }
}

sub get_count_title($$)
{
    my ($Word, $Number) = @_;
    if($Number>=2 or $Number==0) {
        return "$Number $Word"."s";
    }
    elsif($Number==1) {
        return "1 $Word";
    }
}

sub get_TestView($$)
{
    my ($Test, $Interface) = @_;
    $Test = highlight_code($Test, $Interface);
    $Test = htmlSpecChars($Test);
    $Test=~s/\@LT\@/</g;
    $Test=~s/\@GT\@/>/g;
    $Test=~s/\@SP\@/ /g;
    $Test=~s/\@NL\@/\n/g;
    return "<table class='test_view'><tr><td>".$Test."</td></tr></table>\n";
}

sub rm_prefix($)
{
    my $Str = $_[0];
    $Str=~s/\A[_~]+//g;
    return $Str;
}

sub get_IntNameSpace($)
{
    my $Interface = $_[0];
    return "" if(not $Interface);
    return $Cache{"get_IntNameSpace"}{$Interface} if(defined $Cache{"get_IntNameSpace"}{$Interface});
    if(get_Signature($Interface)=~/\:\:/)
    {
        my $FounNameSpace = 0;
        foreach my $NameSpace (sort {get_depth_symbol($b,"::")<=>get_depth_symbol($a,"::")} keys(%NestedNameSpaces))
        {
            if(get_Signature($Interface)=~/\A\Q$NameSpace\E\:\:/) {
                $Cache{"get_IntNameSpace"}{$Interface} = $NameSpace;
                return $NameSpace;
            }
        }
    }
    else
    {
        $Cache{"get_IntNameSpace"}{$Interface} = "";
        return "";
    }
}

sub get_TestSuite_List()
{
    my ($TEST_LIST, %LibGroup_Header_Interface);
    my $Tests_Num = 0;
    my %Interface_Signature = ();
    return "" if(not keys(%Interface_TestDir));
    foreach my $Interface (keys(%Interface_TestDir))
    {
        my $Header = $CompleteSignature{$Interface}{"Header"};
        my $SharedObject = get_filename($Interface_Library{$Interface});
        $SharedObject = get_filename($NeededInterface_Library{$Interface}) if(not $SharedObject);
        $LibGroup_Header_Interface{$Interface_LibGroup{$Interface}}{$SharedObject}{$Header}{$Interface} = 1;
        $Tests_Num += 1;
    }
    foreach my $LibGroup (sort {lc($a) cmp lc($b)} keys(%LibGroup_Header_Interface))
    {
        foreach my $SoName (sort {lc($a) cmp lc($b)} keys(%{$LibGroup_Header_Interface{$LibGroup}}))
        {
            foreach my $HeaderName (sort {lc($a) cmp lc($b)} keys(%{$LibGroup_Header_Interface{$LibGroup}{$SoName}}))
            {
                $TEST_LIST .= "<div style='height:15px;'></div><span class='header'>$HeaderName</span>".(($SoName ne "WithoutLib")?", <span class='solib'>$SoName</span>":"").(($LibGroup)?"&nbsp;<span class='libgroup'>\"$LibGroup\"</span>":"")."<br/>\n";
                my %NameSpace_Interface = ();
                foreach my $Interface (keys(%{$LibGroup_Header_Interface{$LibGroup}{$SoName}{$HeaderName}})) {
                    $NameSpace_Interface{get_IntNameSpace($Interface)}{$Interface} = 1;
                }
                foreach my $NameSpace (sort keys(%NameSpace_Interface))
                {
                    $TEST_LIST .= ($NameSpace)?"<span class='ns_title'>namespace</span> <span class='ns'>$NameSpace</span>"."<br/>\n":"";
                    my @SortedInterfaces = sort {lc(rm_prefix($CompleteSignature{$a}{"ShortName"})) cmp lc(rm_prefix($CompleteSignature{$b}{"ShortName"}))} keys(%{$NameSpace_Interface{$NameSpace}});
                    @SortedInterfaces = sort {$CompleteSignature{$a}{"Destructor"} <=> $CompleteSignature{$b}{"Destructor"}} @SortedInterfaces;
                    @SortedInterfaces = sort {lc(get_TypeName($CompleteSignature{$a}{"Class"})) cmp lc(get_TypeName($CompleteSignature{$b}{"Class"}))} @SortedInterfaces;
                    foreach my $Interface (@SortedInterfaces)
                    {
                        my $RelPath = $Interface_TestDir{$Interface};
                        $RelPath=~s/\A\Q$TEST_SUITE_PATH\E[\/\\]*//g;
                        my $Signature = get_Signature($Interface);
                        if($NameSpace) {
                            $Signature=~s/(\W|\A)\Q$NameSpace\E\:\:(\w)/$1$2/g;
                        }
                        $RelPath=~s/:/\%3A/g;
                        $TEST_LIST .= "<a class='link' href=\'$RelPath/view.html\'><span class='int'>".highLight_Signature_Italic_Color($Signature)."</span></a><br/>\n";
                    }
                }
            }
        }
    }
    $STAT_FIRST_LINE .= "total:$Tests_Num";
    return "<h2 class='title2'>Unit Tests ($Tests_Num)</h2><hr/>\n".$TEST_LIST."<br/><a style='font-size:11px;' href='#Top'>to the top</a><br/>\n";
}

sub get_FailedTests($)
{
    my $Failures_Or_Warning = $_[0];
    my ($FAILED_TESTS, %Type_Value_LibGroup_Header_Interface);
    foreach my $Interface (keys(%RunResult))
    {
        if($Failures_Or_Warning eq "Failures") {
            next if($RunResult{$Interface}{"Warnings"});
        }
        elsif($Failures_Or_Warning eq "Warnings") {
            next if(not $RunResult{$Interface}{"Warnings"});
        }
        my $Header = $RunResult{$Interface}{"Header"};
        my $SharedObject = $RunResult{$Interface}{"SharedObject"};
        my $ProblemType = $RunResult{$Interface}{"Type"};
        my $ProblemValue = $RunResult{$Interface}{"Value"};
        $Type_Value_LibGroup_Header_Interface{$ProblemType}{$ProblemValue}{$Interface_LibGroup{$Interface}}{$SharedObject}{$Header}{$Interface} = 1;
    }
    foreach my $ProblemType ("Received_Signal", "Received_Exception", "Exited_With_Value", "Hanged_Execution", "Requirement_Failed", "Unexpected_Output", "Other_Problems")
    {
        next if(not keys(%{$Type_Value_LibGroup_Header_Interface{$ProblemType}}));
        foreach my $ProblemValue (sort keys(%{$Type_Value_LibGroup_Header_Interface{$ProblemType}}))
        {
            my $PROBLEM_REPORT = "";
            my $Problems_Count = 0;
            foreach my $LibGroup (sort {lc($a) cmp lc($b)} keys(%{$Type_Value_LibGroup_Header_Interface{$ProblemType}{$ProblemValue}}))
            {
                foreach my $SoName (sort {lc($a) cmp lc($b)} keys(%{$Type_Value_LibGroup_Header_Interface{$ProblemType}{$ProblemValue}{$LibGroup}}))
                {
                    foreach my $HeaderName (sort {lc($a) cmp lc($b)} keys(%{$Type_Value_LibGroup_Header_Interface{$ProblemType}{$ProblemValue}{$LibGroup}{$SoName}}))
                    {
                        next if(not $HeaderName or not $SoName);
                        my $HEADER_LIB_REPORT = "<div style='height:15px;'></div><span class='header'>$HeaderName</span>".(($SoName ne "WithoutLib")?", <span class='solib'>$SoName</span>":"").(($LibGroup)?"&nbsp;<span class='libgroup'>\"$LibGroup\"</span>":"")."<br/>\n";
                        my %NameSpace_Interface = ();
                        foreach my $Interface (keys(%{$Type_Value_LibGroup_Header_Interface{$ProblemType}{$ProblemValue}{$LibGroup}{$SoName}{$HeaderName}})) {
                            $NameSpace_Interface{$RunResult{$Interface}{"NameSpace"}}{$Interface} = 1;
                        }
                        foreach my $NameSpace (sort keys(%NameSpace_Interface))
                        {
                            $HEADER_LIB_REPORT .= ($NameSpace)?"<span class='ns_title'>namespace</span> <span class='ns'>$NameSpace</span>"."<br/>\n":"";
                            my @SortedInterfaces = sort {$RunResult{$a}{"Signature"} cmp $RunResult{$b}{"Signature"}} keys(%{$NameSpace_Interface{$NameSpace}});
                            foreach my $Interface (@SortedInterfaces)
                            {
                                my $Signature = $RunResult{$Interface}{"Signature"};
                                if($NameSpace) {
                                    $Signature=~s/(\W|\A)\Q$NameSpace\E\:\:(\w)/$1$2/g;
                                }
                                my $Info = $RunResult{$Interface}{"Info"};
                                my $Test = $RunResult{$Interface}{"Test"};
                                if($Interface=~/\A(_Z|\?)/)
                                {
                                    if($Signature) {
                                        $HEADER_LIB_REPORT .= $ContentSpanStart.highLight_Signature_Italic_Color($Signature).$ContentSpanEnd."<br/>\n$ContentDivStart<span class='mangled'>[ symbol: <b>$Interface</b> ]</span><br/>";
                                    }
                                    else {
                                        $HEADER_LIB_REPORT .= $ContentSpanStart.$Interface.$ContentSpanEnd."<br/>\n$ContentDivStart";
                                    }
                                }
                                else
                                {
                                    if($Signature) {
                                        $HEADER_LIB_REPORT .= $ContentSpanStart.highLight_Signature_Italic_Color($Signature).$ContentSpanEnd."<br/>\n$ContentDivStart";
                                    }
                                    else {
                                        $HEADER_LIB_REPORT .= $ContentSpanStart.$Interface.$ContentSpanEnd."<br/>\n$ContentDivStart";
                                    }
                                }
                                my $RESULT_INFO = "<table class='test_result' cellpadding='2'><tr><td>".htmlSpecChars($Info)."</td></tr></table>";
                                $HEADER_LIB_REPORT .= $RESULT_INFO.$Test."<br/>".$ContentDivEnd;
                                $HEADER_LIB_REPORT = insertIDs($HEADER_LIB_REPORT);
                                $HEADER_LIB_REPORT = $HEADER_LIB_REPORT;
                                $Problems_Count += 1;
                            }
                        }
                        $PROBLEM_REPORT .= $HEADER_LIB_REPORT;
                    }
                }
            }
            if($PROBLEM_REPORT)
            {
                $PROBLEM_REPORT = "<a name=\'".$ProblemType.(($ProblemValue)?"_".$ProblemValue:"")."\'></a>".$ContentSpanStart_Title.get_problem_title($ProblemType, $ProblemValue)." <span class='ext_title'>(".get_count_title(($Failures_Or_Warning eq "Failures")?"problem":"warning", $Problems_Count).")</span>".$ContentSpanEnd."<br/>\n$ContentDivStart\n".$PROBLEM_REPORT."<br/><a style='font-size:11px;' href='#Top'>to the top</a><br/><br/>\n".$ContentDivEnd;
                $PROBLEM_REPORT = insertIDs($PROBLEM_REPORT);
                $FAILED_TESTS .= $PROBLEM_REPORT;
            }
        }
    }
    if($FAILED_TESTS)
    {
        if($Failures_Or_Warning eq "Failures") {
            $FAILED_TESTS = "<a name='Failed_Tests'></a><h2 class='title2'>Failed Tests (".$ResultCounter{"Run"}{"Fail"}.")</h2><hr/>\n".$FAILED_TESTS."<br/>\n";
        }
        elsif($Failures_Or_Warning eq "Warnings") {
            $FAILED_TESTS = "<a name='Warnings'></a><h2 class='title2'>Warnings (".$ResultCounter{"Run"}{"Warnings"}.")</h2><hr/>\n".$FAILED_TESTS."<br/>\n";
        }
    }
    return $FAILED_TESTS;
}

sub composeHTML_Head($$$$)
{
    my ($Title, $Keywords, $Description, $OtherInHead) = @_;
    return "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"en\" lang=\"en\">\n<head>
    <meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />
    <meta name=\"keywords\" content=\"$Keywords\" />
    <meta name=\"description\" content=\"$Description\" />
    <title>\n        $Title\n    </title>\n$OtherInHead\n</head>";
}

sub create_Index()
{
    my $CssStyles = "
    <style type=\"text/css\">
    body {
        font-family:Arial;
    }
    hr {
        color:Black;
        background-color:Black;
        height:1px;
        border:0;
    }
    h1.title1 {
        margin-bottom:0px;
        padding-bottom:0px;
        font-size:26px;
    }
    h2.title2 {
        margin-bottom:0px;
        padding-bottom:0px;
        font-size:20px;
    }
    span.int {
        font-weight:bold;
        cursor:pointer;
        margin-left:7px;
        font-size:16px;
        font-family:Arial;
        color:#003E69;
    }
    span.int_p {
        font-weight:normal;
    }
    span:hover.int {
        color:#336699;
    }
    span.header {
        color:#cc3300;
        font-size:14px;
        font-family:Arial;
        font-weight:bold;
    }
    span.ns_title {
        margin-left:2px;
        color:#408080;
        font-size:13px;
        font-family:Arial;
    }
    span.ns {
        color:#408080;
        font-size:13px;
        font-family:Arial;
        font-weight:bold;
    }
    span.solib {
        color:Green;
        font-size:14px;
        font-family:Arial;
        font-weight:bold;
    }
    span.libgroup {
        color:Black;
        font-size:14px;
        font-family:Arial;
        font-weight:bold;
    }
    span.mangled {
        padding-left:20px;
        font-size:13px;
        cursor:text;
        color:#444444;
    }
    span.color_p {
        font-style:italic;
        color:Brown;
    }
    span.focus_p {
        font-style:italic;
        color:Red;
    }
    a.link {
        text-decoration:none;
    }
    </style>";
    my $SuiteHeader = get_TestSuite_Header();
    if(my $SuiteList = get_TestSuite_List())
    {# initialized $STAT_FIRST_LINE variable
        my $Title = $TargetLibraryFullName."-".$Descriptor{"Version"}.": Test suite";
        my $Keywords = "$TargetLibraryFullName, tests, API";
        my $Description = "Test suite for the $TargetLibraryFullName-".$Descriptor{"Version"}." library on ".getArch();
        writeFile("$TEST_SUITE_PATH/view_tests.html", "<!-- $STAT_FIRST_LINE -->\n".composeHTML_Head($Title, $Keywords, $Description, $CssStyles)."\n<body>\n<div><a name='Top'></a>\n$SuiteHeader<br/>\n$SuiteList</div>\n"."<br/><br/>$TOOL_SIGNATURE\n<div style='height:99px;'></div>\n</body></html>");
    }
}

sub get_TestView_Style()
{
    return "
    table.l_num td {
        background-color:white;
        padding-right:3px;
        padding-left:3px;
        text-align:right;
    }
    table.code_lines td {
        padding-left:15px;
        text-align:left;
        white-space:nowrap;
    }
    span.comm {
        color:#888888;
    }
    span.str {
        color:#FF00FF;
    }
    span.var {
        color:Black;
        font-style:italic;
    }
    span.prepr {
        color:Green;
    }
    span.type {
        color:Brown;
    }
    span.keyw {
        font-weight:bold;
    }
    span.num {
        color:Blue;
    }
    span.targ {
        color:Red;
    }
    span.color_p {
        font-style:italic;
        color:Brown;
    }
    span.focus_p {
        font-style:italic;
        color:Red;
    }";
}

sub create_HtmlReport()
{
    my $CssStyles = "
    <style type=\"text/css\">
    body {
        font-family:Arial;
    }
    hr {
        color:Black;
        background-color:Black;
        height:1px;
        border:0;
    }
    h1.title1 {
        margin-bottom:0px;
        padding-bottom:0px;
        font-size:26px;
    }
    h2.title2 {
        margin-bottom:0px;
        padding-bottom:0px;
        font-size:20px;
    }
    span.section {
        white-space:normal;
        font-weight:bold;
        cursor:pointer;
        margin-left:7px;
        font-size:16px;
        font-family:Arial;
        color:#003E69;
    }
    span:hover.section {
        color:#336699;
    }
    span.section_title {
        font-weight:bold;
        cursor:pointer;
        font-size:18px;
        font-family:Arial;
        color:Black;
    }
    span:hover.section_title {
        color:#585858;
    }
    span.ext {
        font-weight:100;
    }
    span.ext_title {
        font-weight:100;
        font-size:16px;
    }
    span.header {
        color:#cc3300;
        font-size:14px;
        font-family:Arial;
        font-weight:bold;
    }
    span.ns_title {
        margin-left:2px;
        color:#408080;
        font-size:13px;
        font-family:Arial;
    }
    span.ns {
        color:#408080;
        font-size:13px;
        font-family:Arial;
        font-weight:bold;
    }
    span.solib {
        color:Green;
        font-size:14px;
        font-family:Arial;
        font-weight:bold;
    }
    span.libgroup {
        color:Black;
        font-size:14px;
        font-family:Arial;
        font-weight:bold;
    }
    span.int_p {
        font-weight:normal;
    }
    table.summary {
        border-collapse:collapse;
        border:1px outset black;
    }
    td.table_header {
        background-color:#eeeeee;
    }
    td.summary_item {
        font-size:15px;
        font-family:Arial;
        text-align:left;
        border:1px inset black;
    }
    td.summary_item_value {
        padding-right:5px;
        padding-left:5px;
        text-align:right;
        font-size:16px;
        border:1px inset black;
    }
    span.mangled {
        padding-left:20px;
        font-size:13px;
        cursor:text;
        color:#444444;
    }
    ".get_TestView_Style()."
    table.test_view {
        cursor:text;
        margin-top:7px;
        width:75%;
        margin-left:20px;
        font-family:Monaco, \"Courier New\", Courier;
        font-size:14px;
        padding:10px;
        border:1px solid #e0e8e5;
        color:#444444;
        background-color:#eff3f2;
        overflow:auto;
    }
    table.test_result {
        margin-top:3px;
        line-height:16px;
        margin-left:20px;
        font-family:Arial;
        border:0;
        background-color:#FFE4E1;
    }</style>";
    my $JScripts = "<script type=\"text/javascript\" language=\"JavaScript\">
    <!--
    function showContent(header, id)   {
        e = document.getElementById(id);
        if(e.style.display == 'none')
        {
            e.style.display = '';
            e.style.visibility = 'visible';
            header.innerHTML = header.innerHTML.replace(/\\\[[^0-9 ]\\\]/gi,\"[&minus;]\");
        }
        else
        {
            e.style.display = 'none';
            e.style.visibility = 'hidden';
            header.innerHTML = header.innerHTML.replace(/\\\[[^0-9 ]\\\]/gi,\"[+]\");
        }
    }
    -->
    </script>";
    my $Summary = get_Summary();# initialized $STAT_FIRST_LINE variable
    my $Title = $TargetLibraryFullName."-".$Descriptor{"Version"}.": test results";
    my $Keywords = "$TargetLibraryFullName, test, API";
    my $Description = "Test results for the $TargetLibraryFullName-".$Descriptor{"Version"}." library on ".getArch();
    writeFile("$REPORT_PATH/test_results.html", "<!-- $STAT_FIRST_LINE -->\n".composeHTML_Head($Title, $Keywords, $Description, $CssStyles."\n".$JScripts)."\n<body>\n<div><a name='Top'></a>\n".get_Report_Header()."<br/>\n$Summary<br/>\n".get_Problem_Summary()."<br/>\n".get_FailedTests("Failures")."<br/>\n".get_FailedTests("Warnings")."</div>\n"."<br/><br/>$TOOL_SIGNATURE\n<div style='height:999px;'></div>\n</body></html>");
}

sub detect_lib_default_paths()
{
    if($OSgroup eq "bsd")
    {
        if(my $LdConfig = get_CmdPath("ldconfig"))
        {
            foreach my $Line (split(/\n/, `$LdConfig -r`))
            {
                if($Line=~/\A[ \t]*\d+:\-l(.+) \=\> (.+)\Z/)
                {
                    $SoLib_DefaultPath{"lib".$1} = $2;
                    $DefaultLibPaths{get_dirname($2)} = 1;
                }
            }
        }
        else
        {
            print STDERR "WARNING: can't find ldconfig\n";
        }
    }
    else
    {
        if(my $LdConfig = get_CmdPath("ldconfig"))
        {
            foreach my $Line (split(/\n/, `$LdConfig -p`))
            {
                if($Line=~/\A[ \t]*([^ \t]+) .* \=\> (.+)\Z/)
                {
                    $SoLib_DefaultPath{$1} = $2;
                    $DefaultLibPaths{get_dirname($2)} = 1;
                }
            }
        }
        elsif($Config{"osname"}=~/\A(linux)\Z/) {
            print STDERR "WARNING: can't find ldconfig\n";
        }
    }
}

sub detect_include_default_paths()
{
    return if(not $GCC_PATH);
    writeFile("$TMP_DIR/empty.h", "");
    foreach my $Line (split(/\n/, `$GCC_PATH -v -x c++ -E $TMP_DIR/empty.h 2>&1`))
    {# detecting gcc default include paths
        if($Line=~/\A[ \t]*((\/|\w+:\\)[^ ]+)[ \t]*\Z/)
        {
            my $Path = $1;
            while($Path=~s&(\/|\\)[^\/\\]+(\/|\\)\.\.(\/|\\)&$1&){};
            $Path=~s/[\/\\]+\Z//g;
            $Path=~s/\//\\/g if($OSgroup eq "windows");
            if($Path=~/c\+\+|g\+\+/) {
                $DefaultCppPaths{$Path}=1;
                if(not defined $MAIN_CPP_DIR
                or get_depth($MAIN_CPP_DIR)>get_depth($Path)) {
                    $MAIN_CPP_DIR = $Path;
                }
            }
            elsif($Path=~/gcc/) {
                $DefaultGccPaths{$Path}=1;
            }
            else {
                next if($Path=~/local(\/|\\)include/);
                $DefaultIncPaths{$Path}=1;
            }
        }
    }
    unlink("$TMP_DIR/empty.h");
}

sub detect_bin_default_paths()
{
    my $EnvPaths = $ENV{"PATH"};
    if($OSgroup eq "beos") {
        $EnvPaths.=":".$ENV{"BETOOLS"};
    }
    my $Sep = ($OSgroup eq "windows")?";":":|;";
    foreach my $Path (sort {length($a)<=>length($b)} split(/$Sep/, $EnvPaths))
    {
        $Path=~s/[\/\\]+\Z//g;
        next if(not $Path);
        $DefaultBinPaths{$Path} = 1;
    }
}

sub getSymbols()
{
    print "\rlibrary(ies) analysis: [10.00%]";
    my @SoLibPaths = getSoPaths();
    print "\rlibrary(ies) analysis: [30.00%]";
    if($#SoLibPaths==-1) {
        if($OSgroup eq "windows") {# the tool searches for static *.lib in windows
            print STDERR "\nERROR: libraries (*.$LIB_EXT) were not found\n";
        }
        else {
            print STDERR "\nERROR: $SHARED libraries (*.$LIB_EXT) were not found\n";
        }
        exit(1);
    }
    foreach my $SoLibPath (sort {length($a)<=>length($b)} @SoLibPaths)
    {
        $SharedObjects{$SoLibPath} = 1;
        getSymbols_Lib($SoLibPath);
    }
    foreach my $SharedObject (keys(%SharedObject_UndefinedSymbols))
    {# checking dependencies
        foreach my $Symbol (keys(%{$SharedObject_UndefinedSymbols{$SharedObject}}))
        {
            if((not $NeededInterfaceVersion_Library{$Symbol}
            and not $InterfaceVersion_Library{$Symbol}) or $SharedObjects{$SharedObject})
            {
                foreach my $SoPath (find_symbol_libs($Symbol))
                {
                    if(not $SharedObject_RecurDeps{$SharedObject}{$SoPath}) {
                        $SystemObjects_Needed{$SharedObject}{$SoPath} = 1;
                    }
                }
            }
        }
    }
}

my %Symbol_Prefix_Lib=(
# symbols for autodetecting library dependencies (by prefix)
    "pthread_" => ["libpthread.$LIB_EXT"],
    "g_" => ["libglib-2.0.$LIB_EXT", "libgobject-2.0.$LIB_EXT", "libgio-2.0.$LIB_EXT"],
    "cairo_" => ["libcairo.$LIB_EXT"],
    "gtk_" => ["libgtk-x11-2.0.$LIB_EXT"],
    "atk_" => ["libatk-1.0.$LIB_EXT"],
    "gdk_" => ["libgdk-x11-2.0.$LIB_EXT"],
    "gl[A-Z][a-z]" => ["libGL.$LIB_EXT"],
    "glu[A-Z][a-z]" => ["libGLU.$LIB_EXT"],
    "popt[A-Z][a-z]" => ["libpopt.$LIB_EXT"],
    "Py[A-Z][a-z]" => ["libpython"],
    "jpeg_" => ["libjpeg.$LIB_EXT"],
    "BZ2_" => ["libbz2.$LIB_EXT"],
    "Fc[A-Z][a-z]" => ["libfontconfig.$LIB_EXT"],
    "Xft[A-Z][a-z]" => ["libXft.$LIB_EXT"],
    "SSL_" => ["libssl.$LIB_EXT"],
    "sem_" => ["libpthread.$LIB_EXT"],
    "snd_" => ["libasound.$LIB_EXT"],
    "art_" => ["libart_lgpl_2.$LIB_EXT"],
    "dbus_g" => ["libdbus-glib-1.$LIB_EXT"],
    "SL[a-z]" => ["libslang.$LIB_EXT"],
    "GOMP_" => ["libgomp.$LIB_EXT"],
    "omp_" => ["libgomp.$LIB_EXT"],
    "cms[A-Z][a-z]" => ["liblcms.$LIB_EXT"]
);

my %Symbol_Lib=(
# symbols for autodetecting library dependencies (by name)
    "pow" => ["libm.$LIB_EXT"],
    "fmod" => ["libm.$LIB_EXT"],
    "sin" => ["libm.$LIB_EXT"],
    "floor" => ["libm.$LIB_EXT"],
    "cos" => ["libm.$LIB_EXT"],
    "dlopen" => ["libdl.$LIB_EXT"],
    "deflate" => ["libz.$LIB_EXT"],
    "inflate" => ["libz.$LIB_EXT"],
    "move_panel" => ["libpanel.$LIB_EXT"],
    "XOpenDisplay" => ["libX11.$LIB_EXT"],
    "resize_term" => ["libncurses.$LIB_EXT"],
    "clock_gettime" => ["librt.$LIB_EXT"]
);

sub find_symbol_libs($)
{# FIXME: needed more appropriate patterns for symbols
    my $Symbol = $_[0];
    return () if(not $Symbol);
    if($InterfaceVersion_Library{$Symbol}) {
        return ($InterfaceVersion_Library{$Symbol});
    }
    if($Interface_Library{$Symbol}) {
        return ($Interface_Library{$Symbol});
    }
    return () if($Symbol=~/\Ag_/ and $Symbol=~/[A-Z]/);
    my %LibPaths = ();
    foreach my $LibSymbol (keys(%Symbol_Lib))
    {
        if($Symbol eq $LibSymbol)
        {
            foreach my $SoName (@{$Symbol_Lib{$LibSymbol}})
            {
                if(my $SoPath = find_solib_path($SoName)) {
                    $LibPaths{$SoPath} = 1;
                }
            }
        }
    }
    return keys(%LibPaths) if(keys(%LibPaths));
    foreach my $Prefix (keys(%Symbol_Prefix_Lib))
    {
        if($Symbol=~/\A$Prefix/)
        {
            foreach my $SoName (@{$Symbol_Prefix_Lib{$Prefix}})
            {
                if(my $SoPath = find_solib_path($SoName)) {
                    $LibPaths{$SoPath} = 1;
                }
            }
        }
    }
    return keys(%LibPaths) if(keys(%LibPaths));
    if(my $Prefix = getPrefix($Symbol))
    {# try to find library by symbol prefix
        $Prefix=~s/[_]+\Z//g;
        if($Prefix eq "inotify" and
        get_symbol_version($Symbol)=~/GLIBC/)
        {
            if(my $SoPath = find_solib_path("libc.$LIB_EXT")) {
                $LibPaths{$SoPath} = 1;
            }
        }
        else
        {
            if(my $SoPath = find_solib_path_by_prefix($Prefix)) {
                $LibPaths{$SoPath} = 1;
            }
        }
    }
    return keys(%LibPaths);
}

sub find_solib_path_by_prefix($)
{
    my $Prefix = $_[0];
    return "" if(not $Prefix);
    if(my $SoPath = find_solib_path("lib$Prefix-2.$LIB_EXT"))
    {# libgnome-2.so
        return $SoPath;
    }
    elsif(my $SoPath = find_solib_path("lib$Prefix"."2.$LIB_EXT"))
    {# libxml2.so
        return $SoPath;
    }
    elsif(my $SoPath = find_solib_path("lib$Prefix-1.$LIB_EXT"))
    {# libgsf-1.so, libdbus-1.so
        return $SoPath;
    }
    elsif(my $SoPath = find_solib_path("lib$Prefix.$LIB_EXT")) {
        return $SoPath;
    }
    return "";
}

sub get_symbol_version($)
{
    if($_[0]=~/[\@]+(.+)\Z/) {
        return $1;
    }
    else {
        return "";
    }
}

sub getSoPaths()
{
    my @SoPaths = ();
    foreach my $Dest (split(/\n/, $Descriptor{"Libs"}))
    {
        $Dest=~s/\A\s+|\s+\Z//g;
        next if(not $Dest);
        if(not -e $Dest) {
            print STDERR "\nERROR: can't access \'$Dest\'\n";
            next;
        }
        $Dest = abs_path($Dest) if($Dest!~/\A(\/|\w+:[\/\\])/);
        my @SoPaths_Dest = getSOPaths_Dest($Dest);
        foreach (@SoPaths_Dest) {
            push(@SoPaths,$_);
        }
    }
    return @SoPaths;
}

sub skip_lib($)
{
    my $Path = $_[0];
    return 1 if(not $Path);
    my $LibName = get_filename($Path);
    my $ShortName = short_soname($LibName);
    return 1 if($SkipLibs{$LibName});
    return 1 if($SkipLibs{$ShortName});
    foreach my $Pattern (keys(%SkipLibs_Pattern))
    {
        return 1 if($LibName=~/$Pattern/);
        return 1 if($Pattern=~/[\/\\]/ and $Path=~/$Pattern/);
    }
    return 0;
}

sub skip_header($)
{
    my $Path = $_[0];
    return 1 if(not $Path);
    my $HeaderName = get_filename($Path);
    return 1 if($SkipHeaders{$HeaderName});
    foreach my $Pattern (keys(%SkipHeaders_Pattern))
    {
        return 1 if($HeaderName=~/$Pattern/);
        return 1 if($Pattern=~/[\/\\]/ and $Path=~/$Pattern/);
    }
    return 0;
}

sub getSOPaths_Dest($)
{
    my $Dest = $_[0];
    if(-f $Dest) {
        $SharedObject_Paths{get_filename($Dest)}{$Dest} = 1;
        return ($Dest);
    }
    elsif(-d $Dest)
    {
        $Dest=~s/[\/\\]+\Z//g;
        my @AllObjects = ();
        if($SystemPaths{"lib"}{$Dest})
        {
            foreach my $Path (cmd_find($Dest,"","*".esc($TargetLibraryName)."*\.$LIB_EXT*",2))
            {# all files and symlinks that match the name of a library
                if(get_filename($Path)=~/\A(|lib)\Q$TargetLibraryName\E[\d\-]*\.$LIB_EXT[\d\.]*\Z/i)
                {
                    $SharedObject_Paths{get_filename($Path)}{$Path} = 1;
                    push(@AllObjects, resolve_symlink($Path));
                }
            }
        }
        else
        {# all files and symlinks
            foreach my $Path (cmd_find($Dest,"",".*\\.$LIB_EXT\[0-9.]*",""))
            {
                next if(ignore_path($Dest, $Path));
                next if(skip_lib($Path));
                $SharedObject_Paths{get_filename($Path)}{$Path} = 1;
                push(@AllObjects, resolve_symlink($Path));
            }
            if($OSgroup eq "macos")
            {# shared libraries on MacOS may have no extension
                foreach my $Path (cmd_find($Dest,"f","",""))
                {
                    next if(ignore_path($Dest, $Path));
                    next if(skip_lib($Path));
                    if(get_filename($Path)!~/\./
                    and cmd_file($Path)=~/(shared|dynamic)\s+library/i)
                    {
                        $SharedObject_Paths{get_filename($Path)}{$Path} = 1;
                        push(@AllObjects, resolve_symlink($Path));
                    }
                }
            }
        }
        return @AllObjects;
    }
    else {
        return ();
    }
}

sub read_symlink($)
{
    my $Path = $_[0];
    return "" if(not $Path or not -f $Path);
    return $Cache{"read_symlink"}{$Path} if(defined $Cache{"read_symlink"}{$Path});
    if(my $ReadlinkCmd = get_CmdPath("readlink"))
    {
        my $Res = `$ReadlinkCmd -n $Path`;
        $Cache{"read_symlink"}{$Path} = $Res;
        return $Res;
    }
    elsif(my $FileCmd = get_CmdPath("file"))
    {
        my $Info = `$FileCmd $Path`;
        if($Info=~/symbolic\s+link\s+to\s+['`"]*([\w\d\.\-\/\\]+)['`"]*/i)
        {
            $Cache{"read_symlink"}{$Path} = $1;
            return $Cache{"read_symlink"}{$Path};
        }
    }
    $Cache{"read_symlink"}{$Path} = "";
    return "";
}

sub resolve_symlink($)
{
    my $Path = $_[0];
    return "" if(not $Path or not -f $Path);
    return $Path if(isCyclical(\@RecurSymlink, $Path));
    push(@RecurSymlink, $Path);
    if(-l $Path and my $Redirect=read_symlink($Path))
    {
        if($Redirect=~/\A(\/|\w+:[\/\\])/)
        {
            my $Res = resolve_symlink($Redirect);
            pop(@RecurSymlink);
            return $Res;
        }
        elsif($Redirect=~/\.\.[\/\\]/)
        {
            $Redirect = joinPath(get_dirname($Path),$Redirect);
            while($Redirect=~s&(/|\\)[^\/\\]+(\/|\\)\.\.(\/|\\)&$1&){};
            my $Res = resolve_symlink($Redirect);
            pop(@RecurSymlink);
            return $Res;
        }
        elsif(-f get_dirname($Path)."/".$Redirect)
        {
            my $Res = resolve_symlink(joinPath(get_dirname($Path),$Redirect));
            pop(@RecurSymlink);
            return $Res;
        }
        else {
            return $Path;
        }
    }
    else
    {
        pop(@RecurSymlink);
        return $Path;
    }
}

sub get_ShortName($)
{
    my $MnglName = $_[0];
    return $MnglName if($MnglName!~/\A(_Z[A-Z]*)(\d+)/);
    my $Prefix = $1;
    my $Length = $2;
    return substr($MnglName, length($Prefix)+length($Length), $Length);
}

sub readlile_ELF($)
{
    if($_[0]=~/\s*\d+:\s+(\w*)\s+\w+\s+(\w+)\s+(\w+)\s+(\w+)\s+(\w+)\s((\w|@|\.)+)/)
    { # the line of 'readelf' output corresponding to the symbol
        my ($value, $type, $bind, $vis,
        $Ndx, $fullname)=($1, $2, $3, $4, $5, $6);
        if($type eq "FUNC" and $bind eq "LOCAL")
        {
            my ($realname, $version) = get_symbol_name_version($fullname);
            if(not defined $InternalInterfaces{$realname}) {
                $InternalInterfaces{$realname} = 1;
            }
        }
        if($bind!~/\A(WEAK|GLOBAL)\Z/) {
            return ();
        }
        if($type!~/\A(FUNC|IFUNC|OBJECT|COMMON|NOTYPE)\Z/) {
            return ();
        }
        if($vis!~/\A(DEFAULT|PROTECTED)\Z/) {
            return ();
        }
        if($Ndx eq "ABS" and $value!~/\D|1|2|3|4|5|6|7|8|9/) {
            return ();
        }
        return ($fullname, $value, $Ndx, $type);
    }
    else {
        return ();
    }
}

sub get_symbol_name_version($)
{
    my $fullname = $_[0];
    my ($realname, $version) = ($fullname, "");
    if($fullname=~/\A([^\@\$\?]+)([\@\$]+)([^\@\$]+)\Z/) {
        ($realname, $version) = ($1, $3);
    }
    return ($realname, $version);
}

sub getSymbols_Lib($$)
{
    my ($Lib_Path, $IsNeededLib) = @_;
    return () if(not $Lib_Path or not -f $Lib_Path);
    my ($Lib_Dir, $Lib_SoName) = separate_path(resolve_symlink($Lib_Path));
    return () if($CheckedSoLib{$Lib_SoName} and $IsNeededLib);
    return () if(isCyclical(\@RecurLib, $Lib_SoName));# or $#RecurLib>=5
    $CheckedSoLib{$Lib_SoName} = 1;
    push(@RecurLib, $Lib_SoName);
    my %NeededLib = ();
    $STDCXX_TESTING = 1 if($Lib_SoName=~/\Alibstdc\+\+\.$LIB_EXT/ and not $IsNeededLib);
    $GLIBC_TESTING = 1 if($Lib_SoName=~/\Alibc.$LIB_EXT/ and not $IsNeededLib);
    if($OSgroup eq "macos")
    {
        my $OtoolCmd = get_CmdPath("otool");
        if(not $OtoolCmd)
        {
            print STDERR "ERROR: can't find \"otool\"\n";
            exit(1);
        }
        open(DYLIB, "$OtoolCmd -TV $Lib_Path 2>$TMP_DIR/null |");
        while(<DYLIB>)
        {
            if(/[^_]+\s+_([\w\$]+)\s*\Z/)
            {
                my $fullname = $1;
                my ($realname, $version) = get_symbol_name_version($fullname);
                if(not $SoLib_IntPrefix{$Lib_SoName})
                {# collecting prefixes
                    $SoLib_IntPrefix{$Lib_SoName} = get_int_prefix($realname);
                }
                if($IsNeededLib)
                {
                    if($SharedObject_Paths{get_filename($Lib_Path)}{$Lib_Path})
                    {# other shared objects in the same package
                        if(not $NeededInterface_Library{$realname})
                        {
                            $NeededInterface_Library{$realname} = $Lib_Path;
                            if($fullname ne $realname) {
                                $NeededInterfaceVersion_Library{$fullname} = $Lib_Path;
                            }
                        }
                    }
                }
                else
                {
                    if(not $Interface_Library{$realname})
                    {
                        $Interface_Library{$realname} = $Lib_Path;
                        if($fullname ne $realname) {
                            $InterfaceVersion_Library{$fullname} = $Lib_Path;
                        }
                    }
                }
                if(not $Language{$Lib_SoName})
                {
                    if($realname=~/\A(_Z|\?)/)
                    {
                        $Language{$Lib_SoName} = "C++";
                        if(not $IsNeededLib) {
                            $COMMON_LANGUAGE = "C++";
                        }
                    }
                }
            }
        }
        close(DYLIB);
        open(DYLIB, "$OtoolCmd -L $Lib_Path 2>$TMP_DIR/null |");
        while(<DYLIB>)
        {
            if(/\s*([\/\\].+\.$LIB_EXT)\s*/
            and $1 ne $Lib_Path) {
                $NeededLib{$1} = 1;
            }
        }
        close(DYLIB);
    }
    elsif($OSgroup eq "windows")
    {
        my $DumpBinCmd = get_CmdPath("dumpbin");
        if(not $DumpBinCmd)
        {
            print STDERR "ERROR: can't find \"dumpbin\"\n";
            exit(1);
        }
        open(DLL, "$DumpBinCmd /EXPORTS \"$Lib_Path\" 2>$TMP_DIR/null |");
        while(<DLL>)
        {
            if(/(\?[\w\?\@]+)/ or /[^_]+\s+_([\w]+)/)
            {#DLL: /\A\s*\w+\s+\w+\s+\w+\s+([\w\?\@]+)\s*\Z/
                my $realname = $1;
                if(lc($realname) eq "summary") {
                    last;# end of output
                }
                $realname=~s/\A_//g;
                if(not $SoLib_IntPrefix{$Lib_SoName})
                {# collecting prefixes
                    $SoLib_IntPrefix{$Lib_SoName} = get_int_prefix($realname);
                }
                if($IsNeededLib)
                {
                    if($SharedObject_Paths{get_filename($Lib_Path)}{$Lib_Path})
                    {# other shared objects in the same package
                        if(not $NeededInterface_Library{$realname}) {
                            $NeededInterface_Library{$realname} = $Lib_Path;
                        }
                    }
                }
                else
                {
                    if(not $Interface_Library{$realname}) {
                        $Interface_Library{$realname} = $Lib_Path;
                    }
                }
                if(not $Language{$Lib_SoName})
                {
                    if($realname=~/\A(_Z|\?)/)
                    {
                        $Language{$Lib_SoName} = "C++";
                        if(not $IsNeededLib) {
                            $COMMON_LANGUAGE = "C++";
                        }
                    }
                }
            }
        }
        close(DLL);
        open(DLL, "$DumpBinCmd /DEPENDENTS $Lib_Path 2>$TMP_DIR/null |");
        while(<DLL>)
        {
            if(/\s*(.+?\.$LIB_EXT)\s*/i
            and $1 ne $Lib_Path) {
                $NeededLib{$1} = 1;
            }
        }
        close(DLL);
    }
    else
    {
        my $ReadelfCmd = get_CmdPath("readelf");
        if(not $ReadelfCmd)
        {
            print STDERR "ERROR: can't find \"readelf\"\n";
            exit(1);
        }
        open(SOLIB, "$ReadelfCmd -WhlSsdA $Lib_Path 2>$TMP_DIR/null |");
        my $symtab=0; # indicates that we are processing 'symtab' section of 'readelf' output
        while(<SOLIB>)
        {
            if(/NEEDED.+\[([^\[\]]+)\]/)
            {
                $NeededLib{$1} = 1;
            }
            elsif(/'.dynsym'/) {
                $symtab=0;
            }
            elsif($symtab == 1) {
                # do nothing with symtab (but there are some plans for the future)
                next;
            }
            elsif(/'.symtab'/) {
                $symtab=1;
            }
            elsif(my ($fullname, $idx, $Ndx, $type) = readlile_ELF($_)) {
                my ($realname, $version) = get_symbol_name_version($fullname);
                $fullname=~s/\@\@/\@/g;
                if( $Ndx eq "UND" ) {
                    # ignore symbols that are exported form somewhere else
                    $SharedObject_UndefinedSymbols{$Lib_Path}{$fullname} = 1 if(not $IsNeededLib);
                    next;
                }
                if($type eq "NOTYPE") {
                    next;
                }
                if( $symtab == 1 ) {
                    # do nothing with symtab (but there are some plans for the future)
                    next;
                }
                $InternalInterfaces{$realname} = 0;
                if(not $SoLib_IntPrefix{$Lib_SoName})
                {# collecting prefixes
                    $SoLib_IntPrefix{$Lib_SoName} = get_int_prefix($realname);
                }
                if($IsNeededLib)
                {
                    if($SharedObject_Paths{get_filename($Lib_Path)}{$Lib_Path})
                    {# other shared objects in the same package
                        if(not $NeededInterface_Library{$realname})
                        {
                            $NeededInterface_Library{$realname} = $Lib_Path;
                            if($fullname ne $realname) {
                                $NeededInterfaceVersion_Library{$fullname} = $Lib_Path;
                            }
                        }
                    }
                }
                else
                {
                    if(not $Interface_Library{$realname})
                    {
                        $Interface_Library{$realname} = $Lib_Path;
                        if($fullname ne $realname) {
                            $InterfaceVersion_Library{$fullname} = $Lib_Path;
                        }
                    }
                }
                if(not $Language{$Lib_SoName})
                {
                    if($realname=~/\A(_Z|\?)/)
                    {
                        $Language{$Lib_SoName} = "C++";
                        if(not $IsNeededLib) {
                            $COMMON_LANGUAGE = "C++";
                        }
                    }
                }
            }
        }
        close(SOLIB);
    }
    if(not $Language{$Lib_SoName}) {
        $Language{$Lib_SoName} = "C";
    }
    foreach my $SoLib (sort {length($a)<=>length($b)} keys(%NeededLib))
    {# dependencies
        my $DepPath = find_solib_path($SoLib);
        if($DepPath and -f $DepPath)
        {
            if(get_SONAME($DepPath) eq get_filename($SoLib)) {
                $SharedObject_RecurDeps{$Lib_Path}{$DepPath} = 1;
            }
            foreach my $RecurDep (getSymbols_Lib($DepPath, 1)) {
                $SharedObject_RecurDeps{$Lib_Path}{$RecurDep} = 1;
            }
        }
    }
    pop(@RecurLib);
    return keys(%{$SharedObject_RecurDeps{$Lib_Path}});
}

sub get_SONAME($)
{
    my $Path = $_[0];
    return "" if(not $Path or not -e $Path);
    if($OSgroup ne "linux" and $OSgroup ne "bsd")
    {# linker search for exact file name and soname match
        return get_filename($Path);
    }
    my $ObjdumpCmd = get_CmdPath("objdump");
    if(not $ObjdumpCmd) {
        print STDERR "ERROR: can't find \"objdump\"\n";
        exit(1);
    }
    my $NameInfo = `$ObjdumpCmd -x $Path 2>&1 | grep SONAME`;
    if($NameInfo=~/SONAME\s+([^\s]+)/) {
        return $1;
    }
    else {
        return "";
    }
}

sub find_solib_path($)
{
    my $SoName = $_[0];
    return "" if(not $SoName);
    return $SoName if($SoName=~/\A(\/|\w+:[\/\\])/);
    my $ShortName = short_soname($SoName);
    if($ShortName ne $SoName
    and my $Path = find_solib_path($ShortName)) {
        return $Path;
    }
    return $Cache{"find_solib_path"}{$SoName} if(defined $Cache{"find_solib_path"}{$SoName});
    if(my @Paths = sort keys(%{$SharedObject_Paths{$SoName}}))
    {
        $Cache{"find_solib_path"}{$SoName} = $Paths[0];
        return $Cache{"find_solib_path"}{$SoName};
    }
    elsif(my $DefaultPath = $SoLib_DefaultPath{$SoName})
    {
        $Cache{"find_solib_path"}{$SoName} = $DefaultPath;
        return $Cache{"find_solib_path"}{$SoName};
    }
    else
    {
        foreach my $Dir (sort keys(%DefaultLibPaths), sort keys(%{$SystemPaths{"lib"}}))
        {#search in default linker paths and then in the all system paths
            if(-f $Dir."/".$SoName)
            {
                $Cache{"find_solib_path"}{$SoName} = joinPath($Dir,$SoName);
                return $Cache{"find_solib_path"}{$SoName};
            }
        }
        detectSystemObjects() if(not keys(%SystemObjects));
        if(my @AllObjects = sort keys(%{$SystemObjects{$SoName}}))
        {
            $Cache{"find_solib_path"}{$SoName} = $AllObjects[0];
            return $Cache{"find_solib_path"}{$SoName};
        }
        $Cache{"find_solib_path"}{$SoName} = "";
        return "";
    }
}

sub cmd_file($)
{
    my $Path = $_[0];
    return "" if(not $Path or not -e $Path);
    if(my $CmdPath = get_CmdPath("file"))
    {
        my $Cmd = $CmdPath." -b ".esc($Path);
        my $Cmd_Out = `$Cmd`;
        return $Cmd_Out;
    }
    else {
        return "";
    }
}

sub getIncString($$)
{
    my ($ArrRef, $Style) = @_;
    return if(not $ArrRef or $#{$ArrRef}<0);
    my $String = "";
    foreach (@{$ArrRef}) {
        $String .= " ".inc_opt($_, $Style);
    }
    return $String;
}

sub getIncPaths(@)
{
    my @HeaderPaths = @_;
    my @IncludePaths = ();
    if($INC_PATH_AUTODETECT)
    {# autodetecting dependencies
        my %Includes = ();
        foreach my $HPath (@HeaderPaths) {
            foreach my $Dir (get_HeaderDeps($HPath)) {
                $Includes{$Dir}=1;
            }
        }
        foreach my $Dir (keys(%Add_Include_Paths))
        {
            next if($Includes{$Dir});
            push(@IncludePaths, $Dir);
        }
        foreach my $Dir (@{sortIncPaths([keys(%Includes)])}) {
            push(@IncludePaths, $Dir);
        }
    }
    else
    {# user defined paths
        foreach my $Dir (sort {get_depth($a)<=>get_depth($b)}
        sort {$b cmp $a} keys(%Include_Paths)) {
            push(@IncludePaths, $Dir);
        }
    }
    return \@IncludePaths;
}

sub callPreprocessor($$$$)
{
    my ($Path, $Inc, $Lang, $Grep) = @_;
    return "" if(not $Path or not -f $Path);
    my $IncludeString=$Inc;
    if(not $Inc) {
        $IncludeString = getIncString(getIncPaths($Path), "GCC");
    }
    my $GccCall = $GCC_PATH;
    if($Lang eq "C++") {
        $GccCall .= " -x c++-header";
    }
    else {
        $GccCall .= " -x c-header";
    }
    my $Cmd = "$GccCall -dD -E ".esc($Path)." 2>$TMP_DIR/null $CompilerOptions_Headers $IncludeString";
    if($OSgroup eq "windows") {
        $Cmd .= " | find \"#\"";
    }
    else {
        $Cmd .= " | grep \"$Grep\"";
    }
    my $Path = "$TMP_DIR/preprocessed";
    system("$Cmd >$Path");
    return $Path;
}

sub cmd_find($$$$)
{
    my ($Path, $Type, $Name, $MaxDepth) = @_;
    return () if(not $Path or not -e $Path);
    if($OSgroup eq "windows") {
        my $DirCmd = get_CmdPath("dir");
        if(not $DirCmd) {
            print STDERR "ERROR: can't find \"dir\" command\n";
            exit(1);
        }
        $Path = abs_path($Path);
        $Path=~s/\//\\/g;
        my $Cmd = $DirCmd." \"$Path\" /B /O";
        if($MaxDepth!=1) {
            $Cmd .= " /S";
        }
        if($Type eq "d") {
            $Cmd .= " /AD";
        }
        my @Files = ();
        if($Name) {
            $Name=~s/\*/.*/g if($Name!~/\]/);
            foreach my $File (split(/\n/, `$Cmd`))
            {
                if($File=~/$Name\Z/i) {
                    push(@Files, $File);
                }
            }
        }
        else {
            @Files = split(/\n/, `$Cmd 2>$TMP_DIR/null`);
        }
        my @AbsPaths = ();
        foreach my $File (@Files) {
            if($File!~/\A(\/|\w+:[\/\\])/) {
                $File = joinPath($Path, $File);
            }
            push(@AbsPaths, $File);
        }
        if($Type eq "d") {
            push(@AbsPaths, $Path);
        }
        return @AbsPaths;
    }
    else
    {
        my $FindCmd = get_CmdPath("find");
        if(not $FindCmd) {
            print STDERR "ERROR: can't find a \"find\" command\n";
            exit(1);
        }
        my $Cmd = $FindCmd." ".esc(abs_path($Path));
        if($MaxDepth) {
            $Cmd .= " -maxdepth $MaxDepth";
        }
        if($Type) {
            $Cmd .= " -type $Type";
        }
        if($Name) {
            if($Name=~/\]/) {
                $Cmd .= " -regex \"$Name\"";
            }
            else {
                $Cmd .= " -name \"$Name\"";
            }
        }
        return split(/\n/, `$Cmd 2>$TMP_DIR/null`);
    }
}

sub is_header_file($)
{# allowed extensions for header files
    if($_[0]=~/\.(h|hh|hp|hxx|hpp|h\+\+|tcc)\Z/i) {
        return $_[0];
    }
    return 0;
}

sub is_header($$)
{
    my ($Header, $UserDefined) = @_;
    return 0 if(-d $Header);
    if(-f $Header) {
        $Header = abs_path($Header);
    }
    else {
        if($Header=~/\A(\/|\w+:[\/\\])/) {
            # incorrect absolute path
            return 0;
        }
        if(my ($HInc, $HPath) = identify_header($Header)) {
            $Header = $HPath;
        }
        else {
            # can't find header
            return 0;
        }
    }
    if($Header=~/\.\w+\Z/) {
        # have an extension
        return is_header_file($Header);
    }
    else {
        if($UserDefined) {
            # user defined file without extension
            # treated as header file by default
            return $Header;
        }
        else {
            # directory content
            if(cmd_file($Header)=~/C[\+]*\s+program/i) {
                # !~/HTML|XML|shared|dynamic/i
                return $Header;
            }
        }
    }
    return 0;
}

sub get_filename($)
{# much faster than basename() from File::Basename module
    return $Cache{"get_filename"}{$_[0]} if($Cache{"get_filename"}{$_[0]});
    if($_[0]=~/([^\/\\]+)\Z/) {
        return ($Cache{"get_filename"}{$_[0]} = $1);
    }
    return "";
}

sub get_dirname($)
{# much faster than dirname() from File::Basename module
    if($_[0]=~/\A(.*)[\/\\]+([^\/\\]*)\Z/) {
        return $1;
    }
    return "";
}

sub separate_path($)
{
    return (get_dirname($_[0]), get_filename($_[0]));
}

sub find_in_dependencies($)
{
    my $Header = $_[0];
    return "" if(not $Header);
    return $Cache{"find_in_dependencies"}{$Header} if(defined $Cache{"find_in_dependencies"}{$Header});
    foreach my $Dependency (sort {get_depth($a)<=>get_depth($b)} keys(%Header_Dependency))
    {
        next if(not $Dependency);
        if(-f $Dependency."/".$Header)
        {
            $Cache{"find_in_dependencies"}{$Header} = $Dependency;
            return $Cache{"find_in_dependencies"}{$Header};
        }
        if($OSgroup eq "macos" and $Header=~/([^\/]+\.framework)\/Headers\/([^\/]+)/)
        {# searching in frameworks
            my ($HFramework, $HName) = ($1, $2);
            if(get_filename($Dependency) eq $HFramework
            and -e get_dirname($Dependency)."/".$Header)
            {
                $Cache{"find_in_dependencies"}{$Header} = get_dirname($Dependency);
                return $Cache{"find_in_dependencies"}{$Header};
            }
        }
    }
    return "";
}

sub find_in_defaults($)
{
    my $Header = $_[0];
    return "" if(not $Header);
    foreach my $DefaultPath (sort {get_depth($a)<=>get_depth($b)}
    (keys(%DefaultIncPaths),keys(%DefaultGccPaths),keys(%DefaultCppPaths)))
    {
        next if(not $DefaultPath);
        if(-f $DefaultPath."/".$Header) {
            return $DefaultPath;
        }
    }
    return "";
}

sub register_header($$)
{#input: header absolute path, relative path or name
    my ($Header, $Position) = @_;
    return if(not $Header);
    if($Header=~/\A(\/|\w+:[\/\\])/ and not -f $Header) {
        print STDERR "\nERROR: can't access \'$Header\'\n";
        return;
    }
    return if(skip_header($Header));
    my ($Header_Inc, $Header_Path) = identify_header($Header);
    return if(not $Header_Path);
    if(my $RHeader_Path = $Header_ErrorRedirect{$Header_Path}{"Path"})
    {
        return if(skip_header($RHeader_Path));
        $Header_Path = $RHeader_Path;
        return if($RegisteredHeaders{$Header_Path});
    }
    elsif($Header_ShouldNotBeUsed{$Header_Path}) {
        return;
    }
    $Target_Headers{$Header_Path}{"Name"} = get_filename($Header);
    $Target_Headers{$Header_Path}{"Position"} = $Position;
    $RegisteredHeaders{$Header_Path} = 1;
    $RegisteredHeaders_Short{get_filename($Header_Path)} = 1;
}

sub parse_redirect($$)
{
    my ($Content, $Path) = @_;
    return $Cache{"parse_redirect"}{$Path} if(defined $Cache{"parse_redirect"}{$Path});
    return "" if(not $Content);
    my @ErrorMacros = ();
    while($Content=~s/#\s*error\s+([^\n]+?)\s*(\n|\Z)//)
    {
        push(@ErrorMacros, $1);
    }
    my $Redirect = "";
    foreach my $ErrorMacro (@ErrorMacros)
    {
        if($ErrorMacro=~/(only|must\s+include|update\s+to\s+include|replaced\s+with|replaced\s+by|renamed\s+to|is\s+in)\s+(<[^<>]+>|[a-z0-9-_\\\/]+\.(h|hh|hp|hxx|hpp|h\+\+|tcc))/i)
        {
            $Redirect = $2;
            last;
        }
        elsif($ErrorMacro=~/(include|use|is\s+in)\s+(<[^<>]+>|[a-z0-9-_\\\/]+\.(h|hh|hp|hxx|hpp|h\+\+|tcc))\s+instead/i)
        {
            $Redirect = $2;
            last;
        }
        elsif($ErrorMacro=~/this\s+header\s+should\s+not\s+be\s+used/i
        or $ErrorMacro=~/programs\s+should\s+not\s+directly\s+include/i
        or $ErrorMacro=~/you\s+should\s+not\s+include/i
        or $ErrorMacro=~/you\s+should\s+not\s+be\s+including\s+this\s+file/i
        or $ErrorMacro=~/you\s+should\s+not\s+be\s+using\s+this\s+header/i
        or $ErrorMacro=~/is\s+not\s+supported\s+API\s+for\s+general\s+use/i)
        {
            $Header_ShouldNotBeUsed{$Path} = 1;
        }
    }
    $Redirect=~s/\A<//g;
    $Redirect=~s/>\Z//g;
    $Cache{"parse_redirect"}{$Path} = $Redirect;
    return $Redirect;
}

sub parse_includes($$)
{
    my ($Content, $Path) = @_;
    my %Includes = ();
    while($Content=~s/#([ \t]*)(include|import)([ \t]*)(<|")([^<>"]+)(>|")//)
    {# C/C++: include, Objective C/C++: import directive
        my ($Header, $Method) = ($5, $4);
        if(($Method eq "\"" and -e joinPath(get_dirname($Path),$Header))
        or $Header=~/\A(\/|\w+:[\/\\])/) {
        # include "path/header.h" that doesn't exist is equal to include <path/header.h>
            $Includes{$Header} = -1;
        }
        else {
            $Includes{$Header} = 1;
        }
    }
    return \%Includes;
}

sub register_directory($)
{
    my $Dir = $_[0];
    $Dir=~s/[\/\\]+\Z//g;
    return 0 if(not $Dir or not -d $Dir);
    return 0 if(skip_header($Dir));
    $Dir = abs_path($Dir) if($Dir!~/\A(\/|\w+:[\/\\])/);
    return 1 if($RegisteredDirs{$Dir});
    $Header_Dependency{$Dir} = 1;
    foreach my $Path (sort {length($b)<=>length($a)} cmd_find($Dir,"f","",""))
    {
        next if(not is_header($Path, 0));
        next if(ignore_path($Dir, $Path));
        next if(skip_header($Path));
        $DependencyHeaders_All_FullPath{get_filename($Path)} = $Path;
        $DependencyHeaders_All{get_filename($Path)} = $Path;
        my $Prefix = get_filename(get_dirname($Path));
        $DependencyHeaders_All_FullPath{joinPath($Prefix, get_filename($Path))} = $Path;
        $DependencyHeaders_All{joinPath($Prefix, get_filename($Path))} = $Path;
    }
    $RegisteredDirs{$Dir} = 1;
    if(get_filename($Dir) eq "include")
    {# search for "lib/include" directory
        my $LibDir = $Dir;
        if($LibDir=~s/([\/\\])include\Z/$1lib/g and -d $LibDir) {
            register_directory($LibDir);
        }
    }
    return 1;
}

sub joinPath($$)
{
    return join($SLASH, @_);
}

sub ignore_path($$)
{
    my ($Prefix, $Path) = @_;
    return 1 if(not $Path or not -e $Path
      or not $Prefix or not -e $Prefix);
    return 1 if($Path=~/\~\Z/);# skipping system backup files
    # skipping hidden .svn, .git, .bzr, .hg and CVS directories
    return 1 if(cut_path_prefix($Path, $Prefix)=~/(\A|[\/\\]+)(\.(svn|git|bzr|hg)|CVS)([\/\\]+|\Z)/);
    return 0;
}

sub natural_header_sorting($$)
{
    my ($H1, $H2) = @_;
    $H1=~s/\.[a-z]+\Z//ig;
    $H2=~s/\.[a-z]+\Z//ig;
    if($H1 eq $H2) {
        return 0;
    }
    elsif($H1=~/\A\Q$H2\E/) {
        return 1;
    }
    elsif($H2=~/\A\Q$H1\E/) {
        return -1;
    }
    elsif($H1=~/config/i
    and $H2!~/config/i) {
        return -1;
    }
    elsif($H2=~/config/i
    and $H1!~/config/i) {
        return 1;
    }
    else {
        return (lc($H1) cmp lc($H2));
    }
}

sub register_dependency($)
{
    my ($Path) = @_;
    foreach my $Dir (cmd_find($Path,"d","","")) {
        $Dir=~s/[\/\\]+\Z//g;
        $Header_Dependency{$Dir} = 1;
    }
}

sub searchForHeaders()
{
    # detecting system header paths
    foreach my $Path (sort {get_depth($b) <=> get_depth($a)} keys(%DefaultGccPaths))
    {
        foreach my $HeaderPath (sort {get_depth($a) <=> get_depth($b)} cmd_find($Path,"f","",""))
        {
            my $FileName = get_filename($HeaderPath);
            next if($DefaultGccHeader{$FileName}{"Path"});
            $DefaultGccHeader{$FileName}{"Path"} = $HeaderPath;
            $DefaultGccHeader{$FileName}{"Inc"} = cut_path_prefix($HeaderPath, $Path);
        }
    }
    if(($COMMON_LANGUAGE eq "C++" or $CheckHeadersOnly) and not $STDCXX_TESTING)
    {
        foreach my $CppDir (sort {get_depth($b) <=> get_depth($a)} keys(%DefaultCppPaths))
        {
            my @AllCppHeaders = cmd_find($CppDir,"f","","");
            foreach my $Path (sort {get_depth($a) <=> get_depth($b)} @AllCppHeaders)
            {
                my $FileName = get_filename($Path);
                next if($DefaultCppHeader{$FileName}{"Path"});
                $DefaultCppHeader{$FileName}{"Path"} = $Path;
                $DefaultCppHeader{$FileName}{"Inc"} = cut_path_prefix($DefaultCppHeader{$FileName}{"Inc"}, $CppDir);
            }
            detect_headers_array_includes(@AllCppHeaders);
        }
    }
    # detecting library header paths
    foreach my $Dest (keys(%{$Descriptor{"IncludePaths"}}),
    keys(%{$Descriptor{"AddIncludePaths"}}))
    {
        my $IDest = $Dest;
        if(not -e $Dest) {
            print STDERR "\nERROR: can't access \'$Dest\'\n";
        }
        elsif(-f $Dest) {
            print STDERR "\nERROR: \'$Dest\' - not a directory\n";
        }
        elsif(-d $Dest)
        {
            $Dest = abs_path($Dest) if($Dest!~/\A(\/|\w+:[\/\\])/);
            $Header_Dependency{$Dest} = 1;
            foreach my $Path (sort {length($b)<=>length($a)} cmd_find($Dest,"f","",""))
            {
                next if(ignore_path($Dest, $Path));
                my $Header = get_filename($Path);
                $DependencyHeaders_All{$Header} = $Path;
                $DependencyHeaders_All_FullPath{$Header} = $Path;
            }
            if($Descriptor{"AddIncludePaths"}{$IDest}) {
                $Add_Include_Paths{$Dest} = 1;
            }
            else {
                $Include_Paths{$Dest} = 1;
            }
        }
    }
    if(keys(%Include_Paths)) {
        $INC_PATH_AUTODETECT = 0;
    }
    # registering directories
    foreach my $Dest (split(/\n/, $Descriptor{"Headers"}))
    {
        $Dest=~s/\A\s+|\s+\Z//g;
        next if(not $Dest or not -e $Dest);
        if($Dest!~/\A(\/|\w+:[\/\\])/) {
            $Dest = abs_path($Dest);
        }
        if(-d $Dest)
        {
            if(register_directory($Dest))
            {# register all sub directories
                register_dependency($Dest);
            }
        }
        elsif(-f $Dest)
        {
            my $Dir = get_dirname($Dest);
            if(not $SystemPaths{"include"}{$Dir}
            and not $LocalIncludes{$Dir})
            {
                if(register_directory($Dir))
                {# register all sub directories
                    register_dependency($Dir);
                }
                if(my $OutDir = get_dirname($Dir))
                {# registering the outer directory
                    if(not $SystemPaths{"include"}{$OutDir}
                    and not $LocalIncludes{$OutDir}) {
                        register_directory($OutDir);
                    }
                }
            }
        }
    }
    foreach my $Prefix (sort {get_depth($b) <=> get_depth($a)} keys(%Header_Dependency))
    {# remove default prefixes in DependencyHeaders_All
        if($Prefix=~/\A(\/|\w+:[\/\\])/)
        {
            foreach my $Header (keys(%DependencyHeaders_All)) {
                $DependencyHeaders_All{$Header} = cut_path_prefix($DependencyHeaders_All{$Header}, $Prefix);
            }
        }
    }
    # detecting library header includes
    detect_headers_array_includes(values(%DependencyHeaders_All_FullPath));
    foreach my $Dir (keys(%DefaultIncPaths))
    {
        if(-d $Dir."/bits") {
            detect_headers_array_includes(cmd_find($Dir."/bits","f","",""));
            last;
        }
    }
    # recursive redirects
    foreach my $Path (keys(%Header_ErrorRedirect))
    {
        my $RedirectInc = $Header_ErrorRedirect{$Path}{"Inc"};
        my $RedirectPath = $Header_ErrorRedirect{$Path}{"Path"};
        if($Path eq $RedirectPath) {
            delete($Header_ErrorRedirect{$Path});
        }
        if(my $RecurRedirectPath = $Header_ErrorRedirect{$RedirectPath}{"Path"})
        {
            if($Path ne $RecurRedirectPath)
            {
                $Header_ErrorRedirect{$Path}{"Inc"} = $Header_ErrorRedirect{$RedirectPath}{"Inc"};
                $Header_ErrorRedirect{$Path}{"Path"} = $RecurRedirectPath;
            }
        }
    }
    # registering headers
    my $Position = 0;
    foreach my $Dest (split(/\n/, $Descriptor{"Headers"}))
    {
        $Dest=~s/\A\s+|\s+\Z//g;
        next if(not $Dest);
        if($Dest=~/\A(\/|\w+:[\/\\])/ and not -e $Dest) {
            print STDERR "\nERROR: can't access \'$Dest\'\n";
            next;
        }
        if(is_header($Dest, 1))
        {
            register_header($Dest, $Position);
            $Position += 1;
        }
        elsif(-d $Dest)
        {
            foreach my $Path (sort {natural_header_sorting($a, $b)} cmd_find($Dest,"f","",""))
            {
                next if(ignore_path($Dest, $Path));
                next if(not is_header($Path, 0));
                $IsHeaderListSpecified = 0;
                register_header($Path, $Position);
                $Position += 1;
            }
        }
        else {
            print STDERR "\nERROR: can't identify \'$Dest\' as a header file\n";
        }
    }
    #preparing preamble headers
    my $Preamble_Position=0;
    foreach my $Header (split(/\n/, $Descriptor{"IncludePreamble"}))
    {
        $Header=~s/\A\s+|\s+\Z//g;
        next if(not $Header);
        if($Header=~/\A(\/|\w+:[\/\\])/ and not -f $Header)
        {
            print STDERR "\nERROR: can't access file \'$Header\'\n";
            next;
        }
        if(my $Header_Path = is_header($Header, 1))
        {
            $Include_Preamble{$Header_Path}{"Position"} = $Preamble_Position;
            $Preamble_Position+=1;
        }
        else {
            print STDERR "\nERROR: can't identify \'$Header\' as a header file\n";
        }
    }
    detect_headers_array_includes(keys(%RegisteredHeaders), keys(%Include_Preamble));
    foreach my $Path (keys(%Header_Includes))
    {
        next if(not $RegisteredHeaders{$Path});
        if(keys(%{$Header_Includes{$Path}})>$Header_MaxIncludes
        or not defined $Header_MaxIncludes) {
            $Header_MaxIncludes = keys(%{$Header_Includes{$Path}});
        }
    }
    foreach my $AbsPath (keys(%Header_Includes)) {
        detect_recursive_includes($AbsPath);
    }
    foreach my $AbsPath (keys(%RecursiveIncludes_Inverse)) {
        detect_top_header($AbsPath);
    }
    foreach my $HeaderName (keys(%Include_Order))
    {# ordering headers according to descriptor
        my $PairName=$Include_Order{$HeaderName};
        my ($Pos, $PairPos)=(-1, -1);
        my ($Path, $PairPath)=();
        foreach my $Header_Path (sort {int($Target_Headers{$a}{"Position"})<=>int($Target_Headers{$b}{"Position"})} keys(%Target_Headers))  {
            if(get_filename($Header_Path) eq $PairName)
            {
                $PairPos = $Target_Headers{$Header_Path}{"Position"};
                $PairPath = $Header_Path;
            }
            if(get_filename($Header_Path) eq $HeaderName)
            {
                $Pos = $Target_Headers{$Header_Path}{"Position"};
                $Path = $Header_Path;
            }
        }
        if($PairPos!=-1 and $Pos!=-1
        and int($PairPos)<int($Pos))
        {
            my %Tmp = %{$Target_Headers{$Path}};
            %{$Target_Headers{$Path}} = %{$Target_Headers{$PairPath}};
            %{$Target_Headers{$PairPath}} = %Tmp;
        }
    }
    if(not keys(%Target_Headers))
    {
        print STDERR "ERROR: header files were not found\n";
        exit(1);
    }
}

sub detect_headers_array_includes(@)
{
    my @HeaderPaths = @_;
    foreach my $Path (@HeaderPaths) {
        detect_header_includes($Path);
    }
}

sub detect_header_includes($)
{
    my $Path = $_[0];
    return if(not $Path or not -e $Path);
    return if($Cache{"detect_header_includes"}{$Path});
    my $Content = readFile($Path);
    if($Content=~/#[ \t]*error[ \t]+/ and (my $Redirect = parse_redirect($Content, $Path)))
    {# detecting error directive in the headers
        if(my ($RInc, $RPath) = identify_header($Redirect)) {
            if($RPath=~/\/usr\/include\// and $Path!~/\/usr\/include\//) {
                ($RInc, $RPath) = identify_header(get_filename($Redirect));
            }
            $Header_ErrorRedirect{$Path}{"Inc"} = $RInc;
            $Header_ErrorRedirect{$Path}{"Path"} = $RPath;
        }
    }
    my $Inc = parse_includes($Content, $Path);
    foreach my $Include (keys(%{$Inc}))
    {# detecting includes
        $Header_Includes{$Path}{$Include} = $Inc->{$Include};
        if(my $Prefix = get_dirname($Include)) {
            $Header_Prefix{get_filename($Include)} = $Prefix;
        }
    }
    $Cache{"detect_header_includes"}{$Path} = 1;
}

sub detect_recursive_includes($)
{
    my $AbsPath = $_[0];
    return () if(not $AbsPath);
    if(isCyclical(\@RecurInclude, $AbsPath)) {
        return keys(%{$RecursiveIncludes{$AbsPath}});
    }
    return () if($SystemPaths{"include"}{get_dirname($AbsPath)} and $GlibcHeader{get_filename($AbsPath)});
    return () if($SystemPaths{"include"}{get_dirname(get_dirname($AbsPath))}
    and (get_dirname($AbsPath)=~/[\/\\](asm-.+)\Z/ or $GlibcDir{get_filename(get_dirname($AbsPath))}));
    if(keys(%{$RecursiveIncludes{$AbsPath}})) {
        return keys(%{$RecursiveIncludes{$AbsPath}});
    }
    return () if($OSgroup ne "windows" and get_filename($AbsPath)=~/windows|win32|win64/i);
    return () if(get_filename($AbsPath)=~/atomic/i);
    return () if(skip_header($AbsPath));
    return () if($MAIN_CPP_DIR and $AbsPath=~/\A\Q$MAIN_CPP_DIR\E/ and not $STDCXX_TESTING);
    if($DefaultGccPaths{get_dirname($AbsPath)}) {
        push(@RecurInclude, $AbsPath);
        foreach my $Path (keys(%{detect_real_includes($AbsPath)})) {
            $RecursiveIncludes{$AbsPath}{$Path} = 1;
        }
        pop(@RecurInclude);
        return keys(%{$RecursiveIncludes{$AbsPath}});
    }
    push(@RecurInclude, $AbsPath);
    if(not keys(%{$Header_Includes{$AbsPath}})) {
        detect_header_includes($AbsPath);
    }
    foreach my $Include (keys(%{$Header_Includes{$AbsPath}}))
    {
        my ($HInc, $HPath)=identify_header($Include);
        next if(not $HPath);
        $RecursiveIncludes{$AbsPath}{$HPath} = 1;
        $RecursiveIncludes_Inverse{$HPath}{$AbsPath} = 1 if($AbsPath ne $HPath);
        if($Header_Includes{$AbsPath}{$Include}==1)
        { # only include < ... >, skip include " ... " prefixes
            $Header_Include_Prefix{$AbsPath}{$HPath}{get_dirname($Include)} = 1;# empty entries are counted
            if(my $IncDir = get_dirname($Include)) {
                $Include_Prefixes{$HPath}{$IncDir} = 1;
                $IncDir_Prefixes{get_dirname($HPath)}{$IncDir} = 1;
            }
        }
        foreach my $IncPath (detect_recursive_includes($HPath))
        {
            $RecursiveIncludes{$AbsPath}{$IncPath} = 1;
            $RecursiveIncludes_Inverse{$IncPath}{$AbsPath} = 1 if($AbsPath ne $IncPath);
            foreach my $Prefix (keys(%{$Header_Include_Prefix{$HPath}{$IncPath}})) {
                $Header_Include_Prefix{$AbsPath}{$IncPath}{$Prefix} = 1;
            }
        }
        foreach my $Dep (keys(%{$Header_Include_Prefix{$AbsPath}}))
        {
            if($GlibcHeader{get_filename($Dep)} and keys(%{$Header_Include_Prefix{$AbsPath}{$Dep}})>=2
            and defined $Header_Include_Prefix{$AbsPath}{$Dep}{""})
            {# distinguish math.h from glibc and math.h from the tested library
                delete($Header_Include_Prefix{$AbsPath}{$Dep}{""});
                last;
            }
        }
    }
    pop(@RecurInclude);
    return keys(%{$RecursiveIncludes{$AbsPath}});
}

sub detect_top_header($)
{
    my $AbsPath = $_[0];
    return "" if(not $AbsPath);
    foreach my $Path (sort {keys(%{$Header_Includes{$b}})<=>keys(%{$Header_Includes{$a}})} keys(%{$RecursiveIncludes_Inverse{$AbsPath}}))
    {
        if($RegisteredHeaders{$Path}
        and not $Header_ErrorRedirect{$Path}{"Path"}) {
            $Header_TopHeader{$AbsPath} = $Path;
            last;
        }
    }
}

sub get_depth($)
{
    if(defined $Cache{"get_depth"}{$_[0]}) {
        return $Cache{"get_depth"}{$_[0]}
    }
    return ($Cache{"get_depth"}{$_[0]} = ($_[0]=~tr![/\]!!));
}

sub get_depth_symbol($$)
{
    my ($Str, $Sym) = @_;
    return $Cache{"get_depth_symbol"}{$Str}{$Sym} if(defined $Cache{"get_depth_symbol"}{$Str}{$Sym});
    $Cache{"get_depth_symbol"}{$Str}{$Sym} = scalar ( ( ) = $Str=~/($Sym)/g );
    return $Cache{"get_depth_symbol"}{$Str}{$Sym};
}

sub unmangleArray(@)
{
    if(@_[0]=~/\A\?/)
    {# "cl" mangling
        my $UndNameCmd = get_CmdPath("undname");
        if(not $UndNameCmd) {
            print STDERR "ERROR: can't find \"undname\"\n";
            exit(1);
        }
        writeFile("$TMP_DIR/unmangle", join("\n", @_));
        return split(/\n/, `$UndNameCmd 0x8386 $TMP_DIR/unmangle`);
    }
    else
    {# "gcc" mangling
        my $CppFiltInfo = `$CPP_FILT -h 2>&1`;
        if($CppFiltInfo=~/\@<file>/)
        {# new version of c++filt can take a file
            my $Und = ($OSgroup eq "macos" or $OSgroup eq "windows")?"_":"";
            writeFile("$TMP_DIR/unmangle", $Und.join("\n".$Und, @_));
            return split(/\n/, `$CPP_FILT \@\"$TMP_DIR/unmangle\"`);
        }
        else {
            return unmangleArray_Old(@_);
        }
    }
}

sub unmangleArray_Old(@)
{
    if($#_>$MAX_COMMAND_LINE_ARGUMENTS) {
        my @Half = splice(@_, 0, ($#_+1)/2);
        return (unmangleArray(@Half), unmangleArray(@_))
    }
    else {
        my $Und = ($OSgroup eq "macos" or $OSgroup eq "windows")?"_":"";
        my $UnmangleCommand = $CPP_FILT." $Und".join(" $Und", @_);
        return split(/\n/, `$UnmangleCommand`);
    }
}

sub translateSymbols(@)
{
    my (@MnglNames1, @MnglNames2, @UnMnglNames) = ();
    foreach my $Interface (sort @_)
    {
        if($Interface=~/\A_Z/)
        {
            next if($tr_name{$Interface});
            $Interface=~s/[\@\$]+(.*)\Z//;
            push(@MnglNames1, $Interface);
        }
        elsif($Interface=~/\A\?/)
        {
            push(@MnglNames2, $Interface);
        }
        else
        {
            $tr_name{$Interface} = $Interface;
            $mangled_name_gcc{$tr_name{$Interface}} = $Interface;
            $mangled_name{$tr_name{$Interface}} = $Interface;
        }
    }
    if($#MnglNames1 > -1)
    {# gcc names
        @UnMnglNames = reverse(unmangleArray(@MnglNames1));
        foreach my $MnglName (@MnglNames1)
        {
            $tr_name{$MnglName} = correctName(canonifyName(pop(@UnMnglNames)));
            $mangled_name_gcc{$tr_name{$MnglName}} = $MnglName;
        }
    }
    if($#MnglNames2 > -1)
    {# cl names
        @UnMnglNames = reverse(unmangleArray(@MnglNames2));
        foreach my $MnglName (@MnglNames2)
        {
            $tr_name{$MnglName} = correctName(pop(@UnMnglNames));
            $mangled_name{$tr_name{$MnglName}} = $MnglName;
        }
    }
}

sub canonifyName($)
{# make TIFFStreamOpen(char const*, std::basic_ostream<char, std::char_traits<char> >*)
# to be TIFFStreamOpen(char const*, std::basic_ostream<char>*)
    my $Name = $_[0];
    $Name=~s/,\s*std::(allocator|less|char_traits|regex_traits)<\w+> //g;
    return $Name;
}

my %check_node=(
    "array_type"=>1,
    "binfo"=>1,
    "boolean_type"=>1,
    "complex_type"=>1,
    "const_decl"=>1,
    "enumeral_type"=>1,
    "field_decl"=>1,
    "function_decl"=>1,
    "function_type"=>1,
    "identifier_node"=>1,
    "integer_cst"=>1,
    "integer_type"=>1,
    "method_type"=>1,
    "namespace_decl"=>1,
    "parm_decl"=>1,
    "pointer_type"=>1,
    "real_cst"=>1,
    "real_type"=>1,
    "record_type"=>1,
    "reference_type"=>1,
    "string_cst"=>1,
    "template_decl"=>1,
    "template_type_parm"=>1,
    "tree_list"=>1,
    "tree_vec"=>1,
    "type_decl"=>1,
    "union_type"=>1,
    "var_decl"=>1,
    "void_type"=>1);

sub getInfo($)
{
    my $InfoPath = $_[0];
    return if(not $InfoPath or not -f $InfoPath);
    if($Config{"osname"}=~/linux/i)
    {
        my $InfoPath_New = $InfoPath.".1";
        system("sed ':a;N;\$!ba;s/\\n[^\@]/ /g' ".esc($InfoPath)."|sed 's/ [ ]\\+/  /g' > ".esc($InfoPath_New));
        unlink($InfoPath);
        print "\rheader(s) analysis: [40.00%]";
        open(INFO, $InfoPath_New) || die ("\ncan't open file \'$InfoPath_New\': $!\n");
        foreach (<INFO>)
        {
            if(/\A@(\d+)[ \t]+([a-z_]+)[ \t]+(.*)\Z/i)
            {
                next if(not $check_node{$2});
                $LibInfo{$1}{"info_type"}=$2;
                $LibInfo{$1}{"info"}=$3;
            }
        }
        close(INFO);
        unlink($InfoPath_New);
    }
    else
    {
        my $Content = readFile($InfoPath);
        $Content=~s/\n[^\@]/ /g;
        $Content=~s/[ ]{2,}/ /g;
        foreach my $Line (split(/\n/, $Content))
        {
            if($Line=~/\A@(\d+)[ \t]+([a-z_]+)[ \t]+(.*)\Z/i)
            {
                next if(not $check_node{$2});
                $LibInfo{$1}{"info_type"}=$2;
                $LibInfo{$1}{"info"}=$3;
            }
        }
        unlink($InfoPath);
    }
    print "\rheader(s) analysis: [45.00%]";
    # processing info
    setTemplateParams_All();
    print "\rheader(s) analysis: [50.00%]";
    setAnonTypedef_All();
    print "\rheader(s) analysis: [55.00%]";
    getTypeDescr_All();
    renameTmplInst();
    print "\rheader(s) analysis: [65.00%]";
    getFuncDescr_All();
    print "\rheader(s) analysis: [70.00%]";
    foreach my $NameSpace (keys(%Interface_Overloads))
    {
        foreach my $ClassName (keys(%{$Interface_Overloads{$NameSpace}}))
        {
            foreach my $ShortName (keys(%{$Interface_Overloads{$NameSpace}{$ClassName}}))
            {
                if(keys(%{$Interface_Overloads{$NameSpace}{$ClassName}{$ShortName}})>1)
                {
                    foreach my $MnglName (keys(%{$Interface_Overloads{$NameSpace}{$ClassName}{$ShortName}})) {
                        $OverloadedInterface{$MnglName} = keys(%{$Interface_Overloads{$NameSpace}{$ClassName}{$ShortName}});
                    }
                }
                delete($Interface_Overloads{$NameSpace}{$ClassName}{$ShortName});
            }
        }
    }
    getVarDescr_All();
    print "\rheader(s) analysis: [75.00%]";
    add_InlineTemplates();
    add_InlineConstructors();
    print "\rheader(s) analysis: [80.00%]";
    foreach my $TDid (keys(%TypeDescr))
    {
        foreach my $Tid (keys(%{$TypeDescr{$TDid}}))
        {
            my %Type = get_FoundationType($TDid, $Tid);
            my $PointerLevel = get_PointerLevel($TDid, $Tid);
            if($Type{"Type"}=~/\A(Struct|Union)\Z/
            and $Type{"Name"}!~/\:\:/ and $PointerLevel>=1) {
                $StructUnionPName_Tid{get_TypeName($Tid)} = $Tid;
            }
        }
    }
    %LibInfo = ();
    %InlineTmplCtor = ();
    %TypedefToAnon = ();
    %Interface_Overloads = ();
}

sub renameTmplInst()
{
    foreach my $Base (keys(%Typedef_Translations))
    {
        next if($Base!~/</);
        my @Translations = keys(%{$Typedef_Translations{$Base}});
        if($#Translations==0
        and length($Translations[0])<=length($Base)) {
            $Typedef_Equivalent{$Base} = $Translations[0];
        }
    }
    foreach my $TDid (keys(%TypeDescr))
    {
        foreach my $Tid (keys(%{$TypeDescr{$TDid}}))
        {
            my $TypeName = $TypeDescr{$TDid}{$Tid}{"Name"};
            if($TypeName=~/>/)
            {# template instances only
                foreach my $Base (sort {length($b)<=>length($a)}
                keys(%Typedef_Equivalent))
                {
                    next if(not $Base);
                    next if(index($TypeName,$Base)==-1);
                    next if(length($Base)>=length($TypeName)-2);
                    my $TypedefName = $Typedef_Equivalent{$Base};
                    $TypeName=~s/<\Q$Base\E(\W|\Z)/<$TypedefName$1/g;
                    $TypeName=~s/<\Q$Base\E(\w|\Z)/<$TypedefName $1/g;
                    last if($TypeName!~/>/);
                }
                if($TypeName ne $TypeDescr{$TDid}{$Tid}{"Name"}) {
                    $TypeDescr{$TDid}{$Tid}{"Name"} = correctName($TypeName);
                    $TName_Tid{$TypeName} = $Tid;
                }
            }
        }
    }
}

sub getTypeDeclId($)
{
    my $TypeInfo = $LibInfo{$_[0]}{"info"};
    if($TypeInfo=~/name[ ]*:[ ]*@(\d+)/) {
        return $1;
    }
    else {
        return "";
    }
}

sub isFuncPtr($)
{
    my $Ptd = pointTo($_[0]);
    return 0 if(not $Ptd);
    if($LibInfo{$_[0]}{"info"}=~/unql[ ]*:/
    and $LibInfo{$_[0]}{"info"}!~/qual[ ]*:/) {
        return 0;
    }
    elsif($LibInfo{$_[0]}{"info_type"} eq "pointer_type"
    and $LibInfo{$Ptd}{"info_type"} eq "function_type") {
        return 1;
    }
    return 0;
}

sub isMethodPtr($)
{
    my $Ptd = pointTo($_[0]);
    return 0 if(not $Ptd);
    if($LibInfo{$_[0]}{"info_type"} eq "record_type"
    and $LibInfo{$Ptd}{"info_type"} eq "method_type"
    and $LibInfo{$_[0]}{"info"}=~/ ptrmem /) {
        return 1;
    }
    return 0;
}

sub isFieldPtr($)
{
    if($LibInfo{$_[0]}{"info_type"} eq "offset_type"
    and $LibInfo{$_[0]}{"info"}=~/ ptrmem /) {
        return 1;
    }
    return 0;
}

sub pointTo($)
{
    my $TypeInfo = $LibInfo{$_[0]}{"info"};
    if($TypeInfo=~/ptd[ ]*:[ ]*@(\d+)/) {
        return $1;
    }
    else {
        return "";
    }
}

sub getTypeDescr_All()
{
    foreach (sort {int($a)<=>int($b)} keys(%LibInfo))
    {# detecting explicit typedefs
        if($LibInfo{$_}{"info_type"} eq "type_decl")
        {
            my $TypeId = getTreeAttr($_, "type");
            my $TypeDeclId = getTypeDeclId($TypeId);
            my $TypedefName = getNameByInfo($_);
            if($TypeDeclId and ($TypeDeclId ne $_)
            and getNameByInfo($TypeDeclId) ne $TypedefName
            and isNotAnon($TypedefName))
            {
                $ExplicitTypedef{$TypeId}{$_} = 1;
                $Type_Typedef{$TypeId}{-$TypeId} = 1;
            }
        }
    }
    foreach my $Tid (sort {int($a)<=>int($b)} keys(%ExplicitTypedef))
    {# reflecting explicit typedefs to the parallel flatness
        foreach my $TDid (sort {int($a)<=>int($b)} keys(%{$ExplicitTypedef{$Tid}}))
        {
            getTypeDescr($TDid, $Tid);
            if($TypeDescr{$TDid}{$Tid}{"Name"})
            {
                $TypeDescr{$TDid}{$Tid}{"Tid"} = -$Tid;
                $TypeDescr{$TDid}{$Tid}{"TDid"} = $TDid;
                %{$TypeDescr{$TDid}{-$Tid}} = %{$TypeDescr{$TDid}{$Tid}};
                $Tid_TDid{-$Tid} = $TDid;
            }
            else {
                delete($ExplicitTypedef{$Tid}{$TDid});
            }
            delete($TypeDescr{$TDid}{$Tid});
        }
    }
    foreach (sort {int($a)<=>int($b)} keys(%LibInfo))
    {# enumerations
        if($LibInfo{$_}{"info_type"} eq "const_decl") {
            getTypeDescr($_, getTreeAttr($_, "type"));
        }
    }
    foreach (sort {int($a)<=>int($b)} keys(%LibInfo))
    {# other types
        if($LibInfo{$_}{"info_type"}=~/_type\Z/
        and $LibInfo{$_}{"info_type"}!~/function_type|method_type/) {
            getTypeDescr(getTypeDeclId($_), $_);
        }
    }
    $TypeDescr{""}{-1}{"Name"} = "...";
    $TypeDescr{""}{-1}{"Type"} = "Intrinsic";
    $TypeDescr{""}{-1}{"Tid"} = -1;
    $TName_Tid{"..."} = -1;
    detectStructBases();
    detectStructMembers();
    $MaxTypeId_Start = $MaxTypeId;
    foreach my $TypeId (keys(%Type_Typedef))
    {
        foreach my $TypedefId (keys(%{$Type_Typedef{$TypeId}}))
        {
            if(not get_TypeName($TypedefId)
            or get_TypeName($TypedefId) eq get_TypeName($TypeId)) {
                delete($Type_Typedef{$TypeId}{$TypedefId});
            }
        }
    }
}

sub detectStructMembers()
{
    foreach my $TypeId (sort {int($a)<=>int($b)} keys(%StructIDs))
    {
        my %Type = %{$TypeDescr{$Tid_TDid{$TypeId}}{$TypeId}};
        foreach my $MembPos (sort {int($a)<=>int($b)} keys(%{$Type{"Memb"}}))
        {
            my $MembTypeId = $Type{"Memb"}{$MembPos}{"type"};
            my $MembFTypeId = get_FoundationTypeId($MembTypeId);
            if(get_TypeType($MembFTypeId)=~/\A(Struct|Union)\Z/) {
                $Member_Struct{$MembTypeId}{$TypeId}=1;
            }
        }
    }
}

sub init_struct_mapping($$$)
{
    my ($TypeId, $Ref, $KeysRef) = @_;
    my @Keys = @{$KeysRef};
    if($#Keys>=1)
    {
        my $FirstKey = $Keys[0];
        splice(@Keys, 0, 1);
        if(not defined $Ref->{$FirstKey}) {
            %{$Ref->{$FirstKey}} = ();
        }
        init_struct_mapping($TypeId, $Ref->{$FirstKey}, \@Keys);
    }
    elsif($#Keys==0) {
        $Ref->{$Keys[0]}{"Types"}{$TypeId} = 1;
    }
}

sub read_struct_mapping($)
{
    my $Ref = $_[0];
    my %LevelTypes = ();
    @LevelTypes{keys(%{$Ref->{"Types"}})} = values(%{$Ref->{"Types"}});
    foreach my $Key (keys(%{$Ref}))
    {
        next if($Key eq "Types");
        foreach my $SubClassId (read_struct_mapping($Ref->{$Key}))
        {
            $LevelTypes{$SubClassId} = 1;
            foreach my $ParentId (keys(%{$Ref->{"Types"}})) {
                $Struct_SubClasses{$ParentId}{$SubClassId} = 1;
            }
        }
    }
    return keys(%LevelTypes);
}

sub detectStructBases()
{
    foreach my $TypeId (sort {int($a)<=>int($b)} keys(%StructIDs))
    {
        my %Type = %{$TypeDescr{$Tid_TDid{$TypeId}}{$TypeId}};
        next if(not keys(%{$Type{"Memb"}}));
        my $FirstId = $Type{"Memb"}{0}{"type"};
        if($Type{"Memb"}{0}{"name"}=~/parent/i
        and get_TypeType(get_FoundationTypeId($FirstId)) eq "Struct"
        and get_TypeName($FirstId)!~/gobject/i) {
            $Struct_Parent{$TypeId} = $FirstId;
        }
        my @Keys = ();
        foreach my $MembPos (sort {int($a)<=>int($b)} keys(%{$Type{"Memb"}})) {
            push(@Keys, $Type{"Memb"}{$MembPos}{"name"}.":".$Type{"Memb"}{$MembPos}{"type"});
        }
        init_struct_mapping($TypeId, \%Struct_Mapping, \@Keys);
    }
    read_struct_mapping(\%Struct_Mapping);
}

sub getTypeDescr($$)
{
    my ($TypeDeclId, $TypeId) = @_;
    $MaxTypeId = $TypeId if($TypeId>$MaxTypeId or not defined $MaxTypeId);
    if(not $ExplicitTypedef{$TypeId}{$TypeDeclId}) {
        $Tid_TDid{$TypeId} = $TypeDeclId;
    }
    %{$TypeDescr{$TypeDeclId}{$TypeId}} = getTypeAttr($TypeDeclId, $TypeId);
    if(not $TypeDescr{$TypeDeclId}{$TypeId}{"Name"})
    {
        delete($TypeDescr{$TypeDeclId}{$TypeId});
        return;
    }
    if(not keys(%{$TypeDescr{$TypeDeclId}}))
    {
        delete($TypeDescr{$TypeDeclId});
        return;
    }
    if(not $TName_Tid{$TypeDescr{$TypeDeclId}{$TypeId}{"Name"}})
    {
        $TName_Tid{$TypeDescr{$TypeDeclId}{$TypeId}{"Name"}} = $TypeId;
        $TName_Tid{delete_keywords($TypeDescr{$TypeDeclId}{$TypeId}{"Name"})} = $TypeId;
    }
    my $BaseTypeId = get_FoundationTypeId($TypeId);
    my $PLevel = get_PointerLevel($TypeDeclId, $TypeId);
    $BaseType_PLevel_Type{$BaseTypeId}{$PLevel}{$TypeId}=1;
    if(my $Prefix = getPrefix($TypeDescr{$TypeDeclId}{$TypeId}{"Name"})) {
        $Library_Prefixes{$Prefix} += 1;
    }
}

sub getPrefix($)
{
    my $Str = $_[0];
    if($Str=~/\A[_]*(([a-z]|[A-Z])[a-z]+)[A-Z]/) {
        return $1;
    }
    elsif($Str=~/\A[_]*([A-Z]+)[A-Z][a-z]+([A-Z][a-z]+|\Z)/) {
        return $1;
    }
    elsif($Str=~/\A([a-z0-9]+_)[a-z]+/i) {
        return $1;
    }
    elsif($Str=~/\A(([a-z])\2{1,})/i)
    {# ffopen
        return $1;
    }
    else {
        return "";
    }
}

sub getNodeType($)
{
    return $LibInfo{$_[0]}{"info_type"};
}

sub getNodeIntCst($)
{
    my $CstId = $_[0];
    my $CstTypeId = getTreeAttr($CstId, "type");
    if($EnumMembName_Id{$CstId}) {
        return $EnumMembName_Id{$CstId};
    }
    elsif((my $Value = getTreeValue($CstId)) ne "")
    {
        if($Value eq "0")
        {
            if(getNodeType($CstTypeId) eq "boolean_type") {
                return "false";
            }
            else {
                return "0";
            }
        }
        elsif($Value eq "1")
        {
            if(getNodeType($CstTypeId) eq "boolean_type") {
                return "true";
            }
            else {
                return "1";
            }
        }
        else {
            return $Value;
        }
    }
    else {
        return "";
    }
}

sub getNodeStrCst($)
{# FIXME: identify spaces
    if($LibInfo{$_[0]}{"info"}=~/strg[ ]*:[ ]*(.+)[ ]+lngt/
    and $1 ne " ") {
        return $1;
    }
    else {
        return "";
    }
}

sub getArraySize($$)
{
    my ($TypeId, $BaseName) = @_;
    my $SizeBytes = getSize($TypeId)/8;
    while($BaseName=~s/\s*\[(\d+)\]//) {
        $SizeBytes/=$1;
    }
    my $BasicId = $TName_Tid{$BaseName};
    my $BasicSize = $TypeDescr{getTypeDeclId($BasicId)}{$BasicId}{"Size"};
    $SizeBytes/=$BasicSize if($BasicSize);
    return $SizeBytes;
}

sub getTypeAttr($$)
{
    my ($TypeDeclId, $TypeId) = @_;
    my ($BaseTypeSpec, %TypeAttr) = ();
    if($TypeDescr{$TypeDeclId}{$TypeId}{"Name"}) {
        return %{$TypeDescr{$TypeDeclId}{$TypeId}};
    }
    $TypeAttr{"Tid"} = $TypeId;
    $TypeAttr{"TDid"} = $TypeDeclId;
    $TypeAttr{"Type"} = getTypeType($TypeDeclId, $TypeId);
    if($TypeAttr{"Type"} eq "Unknown") {
        return ();
    }
    elsif($TypeAttr{"Type"}=~/(Func|Method|Field)Ptr/)
    {
        %{$TypeDescr{$TypeDeclId}{$TypeId}} = getMemPtrAttr(pointTo($TypeId), $TypeDeclId, $TypeId, $TypeAttr{"Type"});
        $TName_Tid{$TypeDescr{$TypeDeclId}{$TypeId}{"Name"}} = $TypeId;
        return %{$TypeDescr{$TypeDeclId}{$TypeId}};
    }
    elsif($TypeAttr{"Type"} eq "Array")
    {
        ($TypeAttr{"BaseType"}{"Tid"}, $TypeAttr{"BaseType"}{"TDid"}, $BaseTypeSpec) = selectBaseType($TypeDeclId, $TypeId);
        my %BaseTypeAttr = getTypeAttr($TypeAttr{"BaseType"}{"TDid"}, $TypeAttr{"BaseType"}{"Tid"});
        if($TypeAttr{"Size"} = getArraySize($TypeId, $BaseTypeAttr{"Name"}))
        {
            if($BaseTypeAttr{"Name"}=~/\A([^\[\]]+)(\[(\d+|)\].*)\Z/) {
                $TypeAttr{"Name"} = $1."[".$TypeAttr{"Size"}."]".$2;
            }
            else {
                $TypeAttr{"Name"} = $BaseTypeAttr{"Name"}."[".$TypeAttr{"Size"}."]";
            }
        }
        else
        {
            if($BaseTypeAttr{"Name"}=~/\A([^\[\]]+)(\[(\d+|)\].*)\Z/) {
                $TypeAttr{"Name"} = $1."[]".$2;
            }
            else {
                $TypeAttr{"Name"} = $BaseTypeAttr{"Name"}."[]";
            }
        }
        $TypeAttr{"Name"} = correctName($TypeAttr{"Name"});
        if($BaseTypeAttr{"Header"})  {
            $TypeAttr{"Header"} = $BaseTypeAttr{"Header"};
        }
        %{$TypeDescr{$TypeDeclId}{$TypeId}} = %TypeAttr;
        $TName_Tid{$TypeDescr{$TypeDeclId}{$TypeId}{"Name"}} = $TypeId;
        return %TypeAttr;
    }
    elsif($TypeAttr{"Type"}=~/\A(Intrinsic|Union|Struct|Enum|Class)\Z/)
    {
        if(defined $TemplateInstance{$TypeDeclId}{$TypeId})
        {
            my @Template_Params = ();
            foreach my $Param_Pos (sort {int($a)<=>int($b)} keys(%{$TemplateInstance{$TypeDeclId}{$TypeId}}))
            {
                my $Param_TypeId = $TemplateInstance{$TypeDeclId}{$TypeId}{$Param_Pos};
                my $Param = get_TemplateParam($Param_TypeId);
                if($Param eq "") {
                    return ();
                }
                elsif($Param_Pos>=1 and $Param=~/\Astd::(allocator|less|char_traits|regex_traits)\</)
                {# template<typename _Tp, typename _Alloc = std::allocator<_Tp> >
                 # template<typename _Key, typename _Compare = std::less<_Key>
                 # template<typename _CharT, typename _Traits = std::char_traits<_CharT> >
                 # template<typename _Ch_type, typename _Rx_traits = regex_traits<_Ch_type> >
                    next;
                }
                elsif($Param ne "\@skip\@") {
                    @Template_Params = (@Template_Params, $Param);
                }
            }
            %{$TypeDescr{$TypeDeclId}{$TypeId}} = getTrivialTypeAttr($TypeDeclId, $TypeId);
            my $PrmsInLine = join(", ", @Template_Params);
            $PrmsInLine = " ".$PrmsInLine." " if($PrmsInLine=~/>\Z/);
            $TypeDescr{$TypeDeclId}{$TypeId}{"Name"} = $TypeDescr{$TypeDeclId}{$TypeId}{"Name"}."<".$PrmsInLine.">";
            $TypeDescr{$TypeDeclId}{$TypeId}{"Name"} = correctName($TypeDescr{$TypeDeclId}{$TypeId}{"Name"});
            if(not $TName_Tid{$TypeDescr{$TypeDeclId}{$TypeId}{"Name"}}) {
                $TName_Tid{$TypeDescr{$TypeDeclId}{$TypeId}{"Name"}} = $TypeId;
            }
            return %{$TypeDescr{$TypeDeclId}{$TypeId}};
        }
        else
        {
            return () if($TemplateNotInst{$TypeDeclId}{$TypeId});
            return () if($IgnoreTmplInst{$TypeDeclId});
            %{$TypeDescr{$TypeDeclId}{$TypeId}} = getTrivialTypeAttr($TypeDeclId, $TypeId);
            return %{$TypeDescr{$TypeDeclId}{$TypeId}};
        }
    }
    else
    {
        ($TypeAttr{"BaseType"}{"Tid"}, $TypeAttr{"BaseType"}{"TDid"}, $BaseTypeSpec) = selectBaseType($TypeDeclId, $TypeId);
        if(not $ExplicitTypedef{$TypeId}{$TypeDeclId} and keys(%{$ExplicitTypedef{$TypeAttr{"BaseType"}{"Tid"}}})==1)
        {# replace the type to according explicit typedef
            my $NewBase_TDid = (keys(%{$ExplicitTypedef{$TypeAttr{"BaseType"}{"Tid"}}}))[0];
            my $NewBase_Tid = -$TypeAttr{"BaseType"}{"Tid"};
            if($TypeDescr{$NewBase_TDid}{$NewBase_Tid}{"Name"})
            {
                $TypeAttr{"BaseType"}{"TDid"} = $NewBase_TDid;
                $TypeAttr{"BaseType"}{"Tid"} = $NewBase_Tid;
            }
        }
        my %BaseTypeAttr = getTypeAttr($TypeAttr{"BaseType"}{"TDid"}, $TypeAttr{"BaseType"}{"Tid"});
        my $BaseTypeName = $BaseTypeAttr{"Name"};
        if($BaseTypeName=~/\Astd::/ and $StdCxxTypedef{$BaseTypeName}{"Tid"}
        and ($TypeAttr{"Type"} ne "Typedef") and ($TypeId ne $StdCxxTypedef{$BaseTypeName}{"Tid"}))
        {
            $TypeAttr{"BaseType"}{"TDid"} = $StdCxxTypedef{$BaseTypeName}{"TDid"};
            $TypeAttr{"BaseType"}{"Tid"} = $StdCxxTypedef{$BaseTypeName}{"Tid"};
            %BaseTypeAttr = getTypeAttr($TypeAttr{"BaseType"}{"TDid"}, $TypeAttr{"BaseType"}{"Tid"});
        }
        if($BaseTypeAttr{"Type"} eq "Typedef")
        {# relinking typedefs
            my %BaseBase = get_Type($BaseTypeAttr{"BaseType"}{"TDid"},$BaseTypeAttr{"BaseType"}{"Tid"});
            if($BaseTypeAttr{"Name"} eq $BaseBase{"Name"}) {
                ($TypeAttr{"BaseType"}{"Tid"},$TypeAttr{"BaseType"}{"TDid"}) = ($BaseBase{"Tid"},$BaseBase{"TDid"});
            }
        }
        if($BaseTypeSpec and $BaseTypeName)
        {
            if(($TypeAttr{"Type"} eq "Pointer")
            and $BaseTypeName=~/\([\*]+\)/)
            {
                $TypeAttr{"Name"} = $BaseTypeName;
                $TypeAttr{"Name"}=~s/\(([\*]+)\)/($1*)/g;
            }
            else {
                $TypeAttr{"Name"} = $BaseTypeName." ".$BaseTypeSpec;
            }
        }
        elsif($BaseTypeName) {
            $TypeAttr{"Name"} = $BaseTypeName;
        }
        if($TypeAttr{"Type"} eq "Typedef")
        {
            $TypeAttr{"Name"} = getNameByInfo($TypeDeclId);
            $TypeAttr{"NameSpace"} = getNameSpace($TypeDeclId);
            return () if($TypeAttr{"NameSpace"} eq "\@skip\@");
            if($ExplicitTypedef{$TypeId}{$TypeDeclId}
            and length($TypeAttr{"Name"})>length($BaseTypeName)) {
                return ();
            }
            # register typedef
            if(my $NS = $TypeAttr{"NameSpace"})
            {
                my $TypeName = $TypeAttr{"Name"};
                if($NS=~/\A(struct |union |class |)((.+)::|)\Q$TypeName\E\Z/)
                {# "some_type" is the typedef to "struct some_type" in C++
                    if($3) {
                        $TypeAttr{"Name"} = $3."::".$TypeName;
                    }
                }
                else
                {
                    $TypeAttr{"Name"} = $NS."::".$TypeAttr{"Name"};
                    if($TypeAttr{"Name"}!~/\A\Q$BaseTypeName\E/)
                    {# skip v8::Handle<v8::Boolean>::Handle that is typedef to v8::Handle<v8::Boolean>
                        $Type_Typedef{$BaseTypeAttr{"Tid"}}{$TypeAttr{"Tid"}} = 1;
                    }
                }
            }
            elsif($TypeAttr{"Name"}!~/\A\Q$BaseTypeName\E/) {
                $Type_Typedef{$BaseTypeAttr{"Tid"}}{$TypeAttr{"Tid"}} = 1;
            }
            $TypeAttr{"Header"} = getHeader($TypeDeclId);
            if($TypeAttr{"NameSpace"}=~/\Astd(::|\Z)/ and $BaseTypeAttr{"NameSpace"}=~/\Astd(::|\Z)/
            and $BaseTypeName=~/</ and $TypeAttr{"Name"}!~/>(::\w+)+\Z/)
            {
                $StdCxxTypedef{$BaseTypeName}{"Name"} = $TypeAttr{"Name"};
                $StdCxxTypedef{$BaseTypeName}{"Tid"} = $TypeId;
                $StdCxxTypedef{$BaseTypeName}{"TDid"} = $TypeDeclId;
                if(length($TypeAttr{"Name"})<=length($BaseTypeName)) {
                    $Typedef_Equivalent{$BaseTypeName} = $TypeAttr{"Name"};
                }
            }
            if($TypeAttr{"Name"} ne $BaseTypeAttr{"Name"}
            and $TypeAttr{"Name"}!~/>(::\w+)+\Z/ and $BaseTypeAttr{"Name"}!~/>(::\w+)+\Z/) {
                $Typedef_BaseName{$TypeAttr{"Name"}} = $BaseTypeName;
                $Typedef_Translations{$BaseTypeName}{$TypeAttr{"Name"}} = 1;
            }
            delete($TypeAttr{"NameSpace"}) if(not $TypeAttr{"NameSpace"});
        }
        if(not $TypeAttr{"Size"})
        {
            if($TypeAttr{"Type"} eq "Pointer") {
                $TypeAttr{"Size"} = $POINTER_SIZE;
            }
            else {
                $TypeAttr{"Size"} = $BaseTypeAttr{"Size"};
            }
        }
        $TypeAttr{"Name"} = correctName($TypeAttr{"Name"});
        $TypeAttr{"Header"} = $BaseTypeAttr{"Header"} if(not $TypeAttr{"Header"});
        if(defined $SplintAnnotations)
        {
            $TypeAttr{"Line"} = getLine($TypeDeclId);
            $TypeAttr{"Line"} = $BaseTypeAttr{"Line"} if(not $TypeAttr{"Line"});
        }
        %{$TypeDescr{$TypeDeclId}{$TypeId}} = %TypeAttr;
        if(not $TName_Tid{$TypeAttr{"Name"}}) {
            $TName_Tid{$TypeAttr{"Name"}} = $TypeId;
        }
        return %TypeAttr;
    }
}

sub get_TemplateParam($)
{
    my $Type_Id = $_[0];
    return "" if(not $Type_Id);
    if(getNodeType($Type_Id) eq "integer_cst") {
        return getNodeIntCst($Type_Id);
    }
    elsif(getNodeType($Type_Id) eq "string_cst") {
        return getNodeStrCst($Type_Id);
    }
    elsif(getNodeType($Type_Id) eq "tree_vec") {
        return "\@skip\@";
    }
    else
    {
        my $Type_DId = getTypeDeclId($Type_Id);
        my %ParamAttr = getTypeAttr($Type_DId, $Type_Id);
        if(not $ParamAttr{"Name"}) {
            return "";
        }
        if($ParamAttr{"Name"}=~/\>/)
        {
            if(my $Covered = cover_stdcxx_typedef($ParamAttr{"Name"})) {
                return $Covered;
            }
            else {
                return $ParamAttr{"Name"};
            }
        }
        else {
            return $ParamAttr{"Name"};
        }
    }
}

sub cover_stdcxx_typedef($)
{
    my $TypeName = $_[0];
    if(my $Cover = $StdCxxTypedef{$TypeName}{"Name"}) {
        return $Cover;
    }
    my $TypeName_Covered = $TypeName;
    while($TypeName=~s/>[ ]*(const|volatile|restrict| |\*|\&)\Z/>/g){};
    if(my $Cover = $StdCxxTypedef{$TypeName}{"Name"})
    {
        $TypeName_Covered=~s/\Q$TypeName\E(\W|\Z)/$Cover$1/g;
        $TypeName_Covered=~s/\Q$TypeName\E(\w|\Z)/$Cover $1/g;
    }
    return correctName($TypeName_Covered);
}

sub getMemPtrAttr($$$$)
{# function, method and field pointers
    my ($PtrId, $TypeDeclId, $TypeId, $Type) = @_;
    my $MemInfo = $LibInfo{$PtrId}{"info"};
    if($Type eq "FieldPtr") {
        $MemInfo = $LibInfo{$TypeId}{"info"};
    }
    my $MemInfo_Type = $LibInfo{$PtrId}{"info_type"};
    my $MemPtrName = "";
    my %TypeAttr = ("Size"=>$POINTER_SIZE, "Type"=>$Type, "TDid"=>$TypeDeclId, "Tid"=>$TypeId);
    # Return
    if($Type eq "FieldPtr") {
        my %ReturnAttr = getTypeAttr(getTypeDeclId($PtrId), $PtrId);
        $MemPtrName .= $ReturnAttr{"Name"};
    }
    else
    {
        if($MemInfo=~/retn[ ]*:[ ]*\@(\d+) /)
        {
            my $ReturnTypeId = $1;
            my %ReturnAttr = getTypeAttr(getTypeDeclId($ReturnTypeId), $ReturnTypeId);
            $MemPtrName .= $ReturnAttr{"Name"};
            $TypeAttr{"Return"} = $ReturnTypeId;
        }
    }
    # Class
    if($MemInfo=~/(clas|cls)[ ]*:[ ]*@(\d+) /)
    {
        my $ClassId = $2;
        my %Class = getTypeAttr(getTypeDeclId($ClassId), $ClassId);
        if($Class{"Name"}) {
            $MemPtrName .= " (".$Class{"Name"}."\:\:*)";
        }
        else {
            $MemPtrName .= " (*)";
        }
    }
    else {
        $MemPtrName .= " (*)";
    }
    # Parameters
    if($Type eq "FuncPtr"
    or $Type eq "MethodPtr")
    {
        my @ParamTypeName = ();
        if($MemInfo=~/prms[ ]*:[ ]*@(\d+) /)
        {
            my $ParamTypeInfoId = $1;
            my $Position = 0;
            while($ParamTypeInfoId)
            {
                my $ParamTypeInfo = $LibInfo{$ParamTypeInfoId}{"info"};
                last if($ParamTypeInfo!~/valu[ ]*:[ ]*@(\d+) /);
                my $ParamTypeId = $1;
                my %ParamAttr = getTypeAttr(getTypeDeclId($ParamTypeId), $ParamTypeId);
                last if($ParamAttr{"Name"} eq "void");
                if($Position!=0 or $Type ne "MethodPtr")
                {
                    $TypeAttr{"Param"}{$Position}{"type"} = $ParamTypeId;
                    push(@ParamTypeName, $ParamAttr{"Name"});
                }
                last if($ParamTypeInfo!~/(chan|chain)[ ]*:[ ]*@(\d+) /);
                $ParamTypeInfoId = $2;
                $Position+=1;
            }
        }
        $MemPtrName .= " (".join(", ", @ParamTypeName).")";
    }
    $TypeAttr{"Name"} = correctName($MemPtrName);
    return %TypeAttr;
}

sub getTreeTypeName($)
{
    my $Info = $LibInfo{$_[0]}{"info"};
    if($Info=~/name[ ]*:[ ]*@(\d+) /) {
        return getNameByInfo($1);
    }
    else
    {
        if($LibInfo{$_[0]}{"info_type"} eq "integer_type")
        {
            if($LibInfo{$_[0]}{"info"}=~/unsigned/) {
                return "unsigned int";
            }
            else {
                return "int";
            }
        }
        else {
            return "";
        }
    }
}

sub getTypeType($$)
{
    my ($TypeDeclId, $TypeId) = @_;
    return "Typedef" if($ExplicitTypedef{$TypeId}{$TypeDeclId});
    return "Typedef" if($LibInfo{$TypeId}{"info"}=~/unql[ ]*:/ and $LibInfo{$TypeId}{"info"}!~/qual[ ]*:/);# or ($LibInfo{$TypeId}{"info"}=~/qual[ ]*:/ and $LibInfo{$TypeId}{"info"}=~/name[ ]*:/)));
    return "Const" if($LibInfo{$TypeId}{"info"}=~/qual[ ]*:[ ]*c / and $LibInfo{$TypeId}{"info"}=~/unql[ ]*:[ ]*\@/);
    return "Volatile" if($LibInfo{$TypeId}{"info"}=~/qual[ ]*:[ ]*v / and $LibInfo{$TypeId}{"info"}=~/unql[ ]*:[ ]*\@/);
    return "Restrict" if($LibInfo{$TypeId}{"info"}=~/qual[ ]*:[ ]*r / and $LibInfo{$TypeId}{"info"}=~/unql[ ]*:[ ]*\@/);
    my $TypeType = getTypeTypeByTypeId($TypeId);
    if($TypeType eq "Struct")
    {
        if($TypeDeclId and $LibInfo{$TypeDeclId}{"info_type"} eq "template_decl") {
            return "Template";
        }
        else {
            return "Struct";
        }
    }
    else {
        return $TypeType;
    }
}

sub getQual($)
{
    my $TypeId = $_[0];
    if($LibInfo{$TypeId}{"info"}=~ /qual[ ]*:[ ]*(r|c|v)/)
    {
        my $Qual = $1;
        if($Qual eq "r") {
            return "restrict";
        }
        elsif($Qual eq "v") {
            return "volatile";
        }
        elsif($Qual eq "c") {
            return "const";
        }
    }
    return "";
}

sub selectBaseType($$)
{
    my ($TypeDeclId, $TypeId) = @_;
    if($ExplicitTypedef{$TypeId}{$TypeDeclId}) {
        return ($TypeId, getTypeDeclId($TypeId), "");
    }
    my $TypeInfo = $LibInfo{$TypeId}{"info"};
    my $BaseTypeDeclId;
    my $Type_Type = getTypeType($TypeDeclId, $TypeId);
    # qualifiers
    if($LibInfo{$TypeId}{"info"}=~/unql[ ]*:/
    and $LibInfo{$TypeId}{"info"}=~/qual[ ]*:/
    and $LibInfo{$TypeId}{"info"}=~/name[ ]*:[ ]*\@(\d+) /
    and (getTypeId($1) ne $TypeId))
    {# typedefs
        return (getTypeId($1), $1, getQual($TypeId));
    }
    elsif($LibInfo{$TypeId}{"info"}!~/qual[ ]*:/
    and $LibInfo{$TypeId}{"info"}=~/unql[ ]*:[ ]*\@(\d+) /)
    {# typedefs
        return ($1, getTypeDeclId($1), "");
    }
    elsif($LibInfo{$TypeId}{"info"}=~/qual[ ]*:[ ]*c /
    and $LibInfo{$TypeId}{"info"}=~/unql[ ]*:[ ]*\@(\d+) /)
    {
        return ($1, getTypeDeclId($1), "const");
    }
    elsif($LibInfo{$TypeId}{"info"}=~/qual[ ]*:[ ]*r /
    and $LibInfo{$TypeId}{"info"}=~/unql[ ]*:[ ]*\@(\d+) /)
    {
        return ($1, getTypeDeclId($1), "restrict");
    }
    elsif($LibInfo{$TypeId}{"info"}=~/qual[ ]*:[ ]*v /
    and $LibInfo{$TypeId}{"info"}=~/unql[ ]*:[ ]*\@(\d+) /)
    {
        return ($1, getTypeDeclId($1), "volatile");
    }
    elsif($LibInfo{$TypeId}{"info_type"} eq "reference_type")
    {
        if($TypeInfo=~/refd[ ]*:[ ]*@(\d+) /) {
            return ($1, getTypeDeclId($1), "&");
        }
        else {
            return (0, 0, "");
        }
    }
    elsif($LibInfo{$TypeId}{"info_type"} eq "array_type")
    {
        if($TypeInfo=~/elts[ ]*:[ ]*@(\d+) /) {
            return ($1, getTypeDeclId($1), "");
        }
        else {
            return (0, 0, "");
        }
    }
    elsif($LibInfo{$TypeId}{"info_type"} eq "pointer_type")
    {
        if($TypeInfo=~/ptd[ ]*:[ ]*@(\d+) /) {
            return ($1, getTypeDeclId($1), "*");
        }
        else {
            return (0, 0, "");
        }
    }
    else {
        return (0, 0, "");
    }
}

sub getFuncDescr_All()
{
    foreach (sort {int($a)<=>int($b)} keys(%LibInfo))
    {
        if($LibInfo{$_}{"info_type"} eq "function_decl") {
            getFuncDescr($_);
        }
    }
}

sub getVarDescr_All()
{
    foreach (sort {int($b)<=>int($a)} keys(%LibInfo))
    {
        if($LibInfo{$_}{"info_type"} eq "var_decl") {
            getVarDescr($_);
        }
    }
}

sub getVarDescr($)
{
    my $InfoId = $_[0];
    if($LibInfo{getNameSpaceId($InfoId)}{"info_type"} eq "function_decl") {
        return;
    }
    $FuncDescr{$InfoId}{"Header"} = getHeader($InfoId);
    if(not $FuncDescr{$InfoId}{"Header"}
    or $FuncDescr{$InfoId}{"Header"}=~/\<built\-in\>|\<internal\>|\A\./) {
        delete($FuncDescr{$InfoId});
        return;
    }
    setFuncAccess($InfoId);
    if($FuncDescr{$InfoId}{"Private"}) {
        delete($FuncDescr{$InfoId});
        return;
    }
    $FuncDescr{$InfoId}{"ShortName"} = getVarShortName($InfoId);
    if($FuncDescr{$InfoId}{"ShortName"}=~/\Atmp_add_class_\d+\Z/)
    {
        delete($FuncDescr{$InfoId});
        return;
    }
    if(my $Prefix = getPrefix($FuncDescr{$InfoId}{"ShortName"})) {
        $Library_Prefixes{$Prefix} += 1;
    }
    $FuncDescr{$InfoId}{"MnglName"} = getFuncMnglName($InfoId);
    if($FuncDescr{$InfoId}{"MnglName"}
    and $FuncDescr{$InfoId}{"MnglName"}!~/\A(_Z|\?)/) {
        delete($FuncDescr{$InfoId});
        return;
    }
    $FuncDescr{$InfoId}{"Data"} = 1;
    if(not $FuncDescr{$InfoId}{"MnglName"}) {
        $FuncDescr{$InfoId}{"Name"} = $FuncDescr{$InfoId}{"ShortName"};
        $FuncDescr{$InfoId}{"MnglName"} = $FuncDescr{$InfoId}{"ShortName"};
    }
    if($FuncDescr{$InfoId}{"MnglName"}=~/\A(_ZTI|_ZTI|_ZTS|_ZTT|_ZTV|_ZThn|_ZTv0_n|_ZGV)/) {
        delete($FuncDescr{$InfoId});
        return;
    }
    $FuncDescr{$InfoId}{"Return"} = getTypeId($InfoId);
    delete($FuncDescr{$InfoId}{"Return"}) if(not $FuncDescr{$InfoId}{"Return"});
    set_Class_And_Namespace($InfoId);
    if($FuncDescr{$InfoId}{"ShortName"}=~/\A(_Z|\?)/) {
        delete($FuncDescr{$InfoId}{"ShortName"});
    }
    if(get_TypeName($FuncDescr{$InfoId}{"Return"}) ne "void") {
        $ReturnTypeId_Interface{$FuncDescr{$InfoId}{"Return"}}{$FuncDescr{$InfoId}{"MnglName"}} = 1;
    }
}

sub setTemplateParams_All()
{
    foreach (sort {int($a)<=>int($b)} keys(%LibInfo))
    {
        if($LibInfo{$_}{"info_type"} eq "template_decl")
        {
            $IgnoreTmplInst{getTreeAttr($_,"rslt")} = 1;
            setTemplateParams($_);
        }
    }
}

sub setTemplateParams($)
{
    my $TypeInfoId = $_[0];
    my $Info = $LibInfo{$TypeInfoId}{"info"};
    my $TypeId = getTypeId($TypeInfoId);
    my $TDid = getTypeDeclId($TypeId);
    $TemplateHeader{getNameByInfo($TDid)} = getHeader($TDid);
    if($Info=~/(inst|spcs)[ ]*:[ ]*@(\d+) /)
    {
        my $TmplInst_InfoId = $2;
        setTemplateInstParams($TmplInst_InfoId);
        my $TmplInst_Info = $LibInfo{$TmplInst_InfoId}{"info"};
        while($TmplInst_Info=~/(chan|chain)[ ]*:[ ]*@(\d+) /)
        {
            $TmplInst_InfoId = $2;
            $TmplInst_Info = $LibInfo{$TmplInst_InfoId}{"info"};
            setTemplateInstParams($TmplInst_InfoId);
        }
    }
}

sub setTemplateInstParams($)
{
    my $TmplInst_Id = $_[0];
    my $Info = $LibInfo{$TmplInst_Id}{"info"};
    my ($Params_InfoId, $ElemId) = ();
    if($Info=~/purp[ ]*:[ ]*@(\d+) /) {
        $Params_InfoId = $1;
    }
    if($Info=~/valu[ ]*:[ ]*@(\d+) /) {
        $ElemId = $1;
    }
    if($Params_InfoId and $ElemId)
    {
        my $Params_Info = $LibInfo{$Params_InfoId}{"info"};
        while($Params_Info=~s/ (\d+)[ ]*:[ ]*@(\d+) //)
        {
            my ($Param_Pos, $Param_TypeId) = ($1, $2);
            if($LibInfo{$Param_TypeId}{"info_type"} eq "template_type_parm") {
                $TemplateNotInst{getTypeDeclId($ElemId)}{$ElemId} = 1;
                return;
            }
            if($LibInfo{$ElemId}{"info_type"} eq "function_decl") {
                $TemplateInstance_Func{$ElemId}{$Param_Pos} = $Param_TypeId;
            }
            else {
                $TemplateInstance{getTypeDeclId($ElemId)}{$ElemId}{$Param_Pos} = $Param_TypeId;
            }
        }
    }
}

sub has_methods($)
{
    my $TypeId = $_[0];
    return getTreeAttr($TypeId, "fncs");
}

sub getIntLang($)
{
    my $Interface = $_[0];
    return "" if(not $Interface);
    if(my $Lib = get_filename($Interface_Library{$Interface})) {
        return $Language{$Lib};
    }
    elsif(my $Lib = get_filename($NeededInterface_Library{$Interface})) {
        return $Language{$Lib};
    }
    else {
        return (($Interface=~/\A(_Z|\?)|\A_tg_inln_tmpl_\d+\Z/)?"C++":"C");
    }
}

sub setAnonTypedef_All()
{
    foreach (sort {int($a)<=>int($b)} keys(%LibInfo))
    {
        if($LibInfo{$_}{"info_type"} eq "type_decl") {
            setAnonTypedef($_);
        }
    }
}

sub setAnonTypedef($)
{
    my $TypeInfoId = $_[0];
    if(getNameByInfo($TypeInfoId)=~/\._\d+/) {
        $TypedefToAnon{getTypeId($TypeInfoId)} = 1;
    }
}

sub getTrivialTypeAttr($$)
{
    my ($TypeInfoId, $TypeId) = @_;
    $MaxTypeId = $TypeId if($TypeId>$MaxTypeId or not defined $MaxTypeId);
    my %TypeAttr = ();
    return () if(getTypeTypeByTypeId($TypeId)!~/\A(Intrinsic|Union|Struct|Enum|Class)\Z/);
    setTypeAccess($TypeId, \%TypeAttr);
    $TypeAttr{"Header"} = getHeader($TypeInfoId);
    if($TypeAttr{"Header"}=~/\<built\-in\>|\<internal\>|\A\./) {
        delete($TypeAttr{"Header"});
    }
    $TypeAttr{"Name"} = getNameByInfo($TypeInfoId);
    $TypeAttr{"ShortName"} = $TypeAttr{"Name"};
    $TypeAttr{"Name"} = getTreeTypeName($TypeId) if(not $TypeAttr{"Name"});
    if(my $NameSpaceId = getNameSpaceId($TypeInfoId))
    {
        if($NameSpaceId ne $TypeId)
        {
            $TypeAttr{"NameSpace"} = getNameSpace($TypeInfoId);
            return () if($TypeAttr{"NameSpace"} eq "\@skip\@");
            if($LibInfo{$NameSpaceId}{"info_type"} eq "record_type") {
                $TypeAttr{"NameSpaceClassId"} = $NameSpaceId;
            }
        }
    }
    if($TypeAttr{"NameSpace"} and isNotAnon($TypeAttr{"Name"})
    and (not $TypeAttr{"NameSpaceClassId"} or (has_methods($TypeAttr{"NameSpaceClassId"}) and $COMMON_LANGUAGE eq "C++"))) {
        $TypeAttr{"Name"} = $TypeAttr{"NameSpace"}."::".$TypeAttr{"Name"};
    }
    $TypeAttr{"Name"} = correctName($TypeAttr{"Name"});
    if(isAnon($TypeAttr{"Name"}))
    {
        my $HeaderLine = getLine($TypeInfoId);
        $TypeAttr{"Name"} = "anon-";
        $TypeAttr{"Name"} .= $TypeAttr{"Header"}."-".$HeaderLine;
    }
    if($TypeAttr{"Name"}=~/\Acomplex (int|float|double|long double)\Z/
    and $COMMON_LANGUAGE eq "C") {
        $TypeAttr{"Name"}=~s/complex/_Complex/g;
    }
    $TypeAttr{"Line"} = getLine($TypeInfoId) if(defined $SplintAnnotations);
    if($TypeAttr{"Name"} eq "__exception"
    and $TypeAttr{"Header"} eq "math.h")
    {# libm
        $TypeAttr{"Name"} = "exception";
    }
    $TypeAttr{"Size"} = getSize($TypeId)/8;
    $TypeAttr{"Type"} = getTypeType($TypeInfoId, $TypeId);
    if($TypeAttr{"Type"} eq "Struct"
    and has_methods($TypeId) and $COMMON_LANGUAGE eq "C++") {
        $TypeAttr{"Type"} = "Class";
    }
    if($TypeAttr{"Type"} eq "Class") {
        setBaseClasses($TypeInfoId, $TypeId, \%TypeAttr);
    }
    if(my $Prefix = getPrefix($TypeAttr{"Name"})) {
        $Library_Prefixes{$Prefix} += 1;
    }
    if($TypeAttr{"Type"}=~/\A(Struct|Union|Enum)\Z/)
    {
        if(not isAnonTypedef($TypeId) and not $TypedefToAnon{$TypeId}
        and not keys(%{$TemplateInstance{$TypeInfoId}{$TypeId}}))
        {
            if(not $TName_Tid{$TypeAttr{"Name"}} and isNotAnon($TypeAttr{"Name"})) {
                $TName_Tid{$TypeAttr{"Name"}} = $TypeId;
            }
            $TypeAttr{"Name"} = lc($TypeAttr{"Type"})." ".$TypeAttr{"Name"};
        }
    }
    setTypeMemb($TypeId, \%TypeAttr);
    $TypeAttr{"Tid"} = $TypeId;
    $TypeAttr{"TDid"} = $TypeInfoId;
    $Tid_TDid{$TypeId} = $TypeInfoId;
    if(not $TName_Tid{$TypeAttr{"Name"}} and isNotAnon($TypeAttr{"Name"})) {
        $TName_Tid{$TypeAttr{"Name"}} = $TypeId;
    }
    delete($TypeAttr{"NameSpace"}) if(not $TypeAttr{"NameSpace"});
    if($TypeAttr{"Type"} eq "Struct") {
        $StructIDs{$TypeId}=1;
    }
    return %TypeAttr;
}

sub setBaseClasses($$$)
{
    my ($TypeInfoId, $TypeId, $TypeAttr) = @_;
    my $Info = $LibInfo{$TypeId}{"info"};
    if($Info=~/binf[ ]*:[ ]*@(\d+) /)
    {
        $Info = $LibInfo{$1}{"info"};
        while($Info=~s/(pub|prot|priv|)[ ]+binf[ ]*:[ ]*@(\d+) //)
        {
            my ($Access, $BInfoId) = ($1, $2);
            my $ClassId = getBinfClassId($BInfoId);
            if($Access eq "pub") {
                $TypeAttr->{"BaseClass"}{$ClassId} = "public";
            }
            elsif($Access eq "prot") {
                $TypeAttr->{"BaseClass"}{$ClassId} = "protected";
            }
            else {
                $TypeAttr->{"BaseClass"}{$ClassId} = "private";
            }
            $Class_SubClasses{$ClassId}{$TypeId} = 1;
        }
    }
}

sub getBinfClassId($)
{
    my $Info = $LibInfo{$_[0]}{"info"};
    $Info=~/type[ ]*:[ ]*@(\d+) /;
    return $1;
}

sub get_Type($$)
{
    my ($TypeDId, $TypeId) = @_;
    return "" if(not $TypeId or not $TypeDescr{$TypeDId}{$TypeId});
    return %{$TypeDescr{$TypeDId}{$TypeId}};
}

sub construct_signature($)
{
    my $InfoId = $_[0];
    if($Cache{"construct_signature"}{$InfoId}) {
        return $Cache{"construct_signature"}{$InfoId};
    }
    my $PureSignature = $FuncDescr{$InfoId}{"ShortName"};
    my @ParamTypes = ();
    if(not $FuncDescr{$InfoId}{"Data"})
    {
        foreach my $ParamPos (sort {int($a) <=> int($b)} keys(%{$FuncDescr{$InfoId}{"Param"}}))
        {# checking parameters
            my $ParamType_Id = $FuncDescr{$InfoId}{"Param"}{$ParamPos}{"type"};
            my $ParamType_Name = uncover_typedefs(get_TypeName($ParamType_Id));
            if($OSgroup eq "windows") {
                $ParamType_Name=~s/(\W|\A)long long(\W|\Z)/$1__int64$2/;
            }
            @ParamTypes = (@ParamTypes, $ParamType_Name);
        }
        $PureSignature .= "(".join(", ", @ParamTypes).")";
        $PureSignature = delete_keywords($PureSignature);
    }
    if(my $ClassId = $FuncDescr{$InfoId}{"Class"}) {
        $PureSignature = get_TypeName($ClassId)."::".$PureSignature;
    }
    elsif(my $NS = $FuncDescr{$InfoId}{"NameSpace"}) {
        $PureSignature = $NS."::".$PureSignature;
    }
    $Cache{"construct_signature"}{$InfoId} = correctName($PureSignature);
    return $Cache{"construct_signature"}{$InfoId};
}

sub delete_keywords($)
{
    my $TypeName = $_[0];
    $TypeName=~s/(\W|\A)(enum |struct |union |class )/$1/g;
    return $TypeName;
}

sub uncover_typedefs($)
{
    my $TypeName = $_[0];
    return "" if(not $TypeName);
    return $Cache{"uncover_typedefs"}{$TypeName} if(defined $Cache{"uncover_typedefs"}{$TypeName});
    my ($TypeName_New, $TypeName_Pre) = (correctName($TypeName), "");
    while($TypeName_New ne $TypeName_Pre)
    {
        $TypeName_Pre = $TypeName_New;
        my $TypeName_Copy = $TypeName_New;
        my %Words = ();
        while($TypeName_Copy=~s/(\W|\A)([a-z_][\w:]*)(\W|\Z)//io)
        {
            my $Word = $2;
            next if(not $Word or $Word=~/\A(true|false|const|int|long|void|short|float|unsigned|char|double|class|struct|union|enum)\Z/);
            $Words{$Word} = 1;
        }
        foreach my $Word (sort keys(%Words))
        {
            my $BaseType_Name = $Typedef_BaseName{$Word};
            next if($TypeName_New=~/(\W|\A)(class|struct|union|enum)\s+\Q$Word\E(\W|\Z)/);
            next if(not $BaseType_Name);
            if($BaseType_Name=~/\([\*]+\)/)
            {
                if($TypeName_New=~/\Q$Word\E(.*)\Z/)
                {
                    my $Type_Suffix = $1;
                    $TypeName_New = $BaseType_Name;
                    if($TypeName_New=~s/\(([\*]+)\)/($1 $Type_Suffix)/) {
                        $TypeName_New = correctName($TypeName_New);
                    }
                }
            }
            else
            {
                if($TypeName_New=~s/(\W|\A)\Q$Word\E(\W|\Z)/$1$BaseType_Name$2/g) {
                    $TypeName_New = correctName($TypeName_New);
                }
            }
        }
    }
    $Cache{"uncover_typedefs"}{$TypeName} = $TypeName_New;
    return $TypeName_New;
}

sub get_type_short_name($)
{
    my $TypeName = $_[0];
    $TypeName=~s/[ ]*<.*>[ ]*//g;
    $TypeName=~s/\Astruct //g;
    $TypeName=~s/\Aunion //g;
    $TypeName=~s/\Aclass //g;
    return $TypeName;
}

sub add_InlineTemplates()
{
    my $NewId = (sort {int($b)<=>int($a)} keys(%FuncDescr))[0] + 10;
    foreach my $TDid (sort keys(%TemplateInstance))
    {
        foreach my $Tid (sort keys(%{$TemplateInstance{$TDid}}))
        {
            my $ClassName = get_type_short_name($TypeDescr{$TDid}{$Tid}{"Name"});
            foreach my $FuncInfoId (sort {int($a)<=>int($b)} keys(%{$InlineTmplCtor{$ClassName}}))
            {
                my $FuncClassId = $FuncDescr{$FuncInfoId}{"Class"};
                if(not keys(%{$FuncDescr{$FuncInfoId}{"Param"}})
                or (keys(%{$FuncDescr{$FuncInfoId}{"Param"}})==1 and keys(%{$TemplateInstance{$TDid}{$Tid}})==1
                and $FuncDescr{$FuncInfoId}{"Param"}{0}{"type"} eq $TemplateInstance{$Tid_TDid{$FuncClassId}}{$FuncClassId}{0}))
                {
                    $FuncDescr{$NewId}{"Header"} = $FuncDescr{$FuncInfoId}{"Header"};
                    if($FuncDescr{$FuncInfoId}{"Protected"}) {
                        $FuncDescr{$NewId}{"Protected"}=1;
                    }
                    if($FuncDescr{$FuncInfoId}{"Private"}) {
                        $FuncDescr{$NewId}{"Private"}=1;
                    }
                    $FuncDescr{$NewId}{"Class"} = $Tid;
                    $FuncDescr{$NewId}{"Constructor"} = 1;
                    $FuncDescr{$NewId}{"MnglName"} = "_tg_inln_tmpl_$NewId";
                    $Class_Constructors{$Tid}{$FuncDescr{$NewId}{"MnglName"}} = 1;
                    if(keys(%{$FuncDescr{$FuncInfoId}{"Param"}})==1) {
                        $FuncDescr{$NewId}{"Param"}{0}{"type"} = $TemplateInstance{$TDid}{$Tid}{0};
                    }
                    $TypeDescr{$TDid}{$Tid}{"Type"} = "Class";
                    $NewId+=1;
                    last;
                }
            }
        }
    }
}

sub add_InlineConstructors()
{
    my $NewId = (sort {int($b)<=>int($a)} keys(%FuncDescr))[0] + 10;
    foreach my $ClassId (keys(%ClassesWithMethods))
    {
        if(not $ClassesWithConstructors{$ClassId})
        {# classes without constructors
            my %Descr = ();
            my %CInfo = get_Type($Tid_TDid{$ClassId}, $ClassId);
            $Descr{"Header"} = $CInfo{"Header"};
            $Descr{"Class"} = $ClassId;
            $Descr{"Constructor"} = 1;
            $Descr{"MnglName"} = "_tg_inln_ctor_$NewId";
            %{$FuncDescr{$NewId++}} = %Descr;
        }
    }
}

sub isInternal($)
{
    my $FuncInfoId = $_[0];
    my $FuncInfo = $LibInfo{$_[0]}{"info"};
    return 0 if($FuncInfo!~/mngl[ ]*:[ ]*@(\d+) /);
    my $FuncMnglNameInfoId = $1;
    return ($LibInfo{$FuncMnglNameInfoId}{"info"}=~/\*[ ]*INTERNAL[ ]*\*/);
}

sub getFuncDescr($)
{
    my $FuncInfoId = $_[0];
    return if(isInternal($FuncInfoId));
    $FuncDescr{$FuncInfoId}{"Header"} = getHeader($FuncInfoId);
    if(defined $SplintAnnotations) {
        $FuncDescr{$FuncInfoId}{"Line"} = getLine($FuncInfoId);
    }
    if(not $FuncDescr{$FuncInfoId}{"Header"}
    or $FuncDescr{$FuncInfoId}{"Header"}=~/\<built\-in\>|\<internal\>|\A\./)
    {
        delete($FuncDescr{$FuncInfoId});
        return;
    }
    if(getFuncSpec($FuncInfoId) eq "PureVirt")
    {# pure virtual methods
        $FuncDescr{$FuncInfoId}{"PureVirt"} = 1;
    }
    setFuncAccess($FuncInfoId);
    setFuncKind($FuncInfoId);
    set_Class_And_Namespace($FuncInfoId);
    if($FuncDescr{$FuncInfoId}{"Constructor"})
    { # register relation between the constructor and class
        $ClassesWithConstructors{$FuncDescr{$FuncInfoId}{"Class"}} = 1;
    }
    if($FuncDescr{$FuncInfoId}{"Private"}
    and not $FuncDescr{$FuncInfoId}{"PureVirt"}) {
        delete($FuncDescr{$FuncInfoId});
        return;
    }
    if($FuncDescr{$FuncInfoId}{"PseudoTemplate"}) {
        delete($FuncDescr{$FuncInfoId});
        return;
    }
    $FuncDescr{$FuncInfoId}{"Type"} = getFuncType($FuncInfoId);
    $FuncDescr{$FuncInfoId}{"Return"} = getFuncReturn($FuncInfoId);
    delete($FuncDescr{$FuncInfoId}{"Return"}) if(not $FuncDescr{$FuncInfoId}{"Return"});
    if(keys(%{$ExplicitTypedef{$FuncDescr{$FuncInfoId}{"Return"}}})==1)
    {# replace the type to according explicit typedef
        $FuncDescr{$FuncInfoId}{"Return"} = -$FuncDescr{$FuncInfoId}{"Return"};
    }
    $FuncDescr{$FuncInfoId}{"ShortName"} = getFuncShortName(getFuncOrig($FuncInfoId));
    if(my $Prefix = getPrefix($FuncDescr{$FuncInfoId}{"ShortName"})) {
        $Library_Prefixes{$Prefix} += 1;
    }
    if($FuncDescr{$FuncInfoId}{"ShortName"}=~/\._/) {
        delete($FuncDescr{$FuncInfoId});
        return;
    }
    if(defined $TemplateInstance_Func{$FuncInfoId})
    {
        my @TmplParams = ();
        foreach my $ParamPos (sort {int($a) <=> int($b)} keys(%{$TemplateInstance_Func{$FuncInfoId}}))
        {
            my $Param = get_TemplateParam($TemplateInstance_Func{$FuncInfoId}{$ParamPos});
            if($Param eq "") {
                delete($FuncDescr{$FuncInfoId});
                return;
            }
            elsif($Param ne "\@skip\@") {
                push(@TmplParams, $Param);
            }
        }
        my $PrmsInLine = join(", ", @TmplParams);
        $PrmsInLine = " ".$PrmsInLine." " if($PrmsInLine=~/>\Z/);
        $FuncDescr{$FuncInfoId}{"ShortName"} .= " <".$PrmsInLine.">";
    }
    $FuncDescr{$FuncInfoId}{"MnglName"} = getFuncMnglName($FuncInfoId);
    if($FuncDescr{$FuncInfoId}{"MnglName"}
    and $FuncDescr{$FuncInfoId}{"MnglName"}!~/\A(_Z|\?)/) {
        delete($FuncDescr{$FuncInfoId});
        return;
    }
# std symbols may used to generate tests for non-std symbols
#     if($FuncDescr{$FuncInfoId}{"MnglName"} and not $STDCXX_TESTING and not $CheckStdCxx
#     and (not $TargetInterfaceName or $FuncDescr{$FuncInfoId}{"MnglName"} ne $TargetInterfaceName))
#     {# stdc++ symbols
#         if($FuncDescr{$FuncInfoId}{"MnglName"}=~/\A(_ZS|_ZNS|_ZNKS)/) {
#             delete($FuncDescr{$FuncInfoId});
#             return;
#         }
#     }
    setFuncParams($FuncInfoId);
    if($FuncDescr{$FuncInfoId}{"Skip"}) {
        delete($FuncDescr{$FuncInfoId});
        return;
    }
    if($FuncDescr{$FuncInfoId}{"ShortName"}=~/\A(__base_ctor|__comp_ctor)\Z/) {
        $FuncDescr{$FuncInfoId}{"ShortName"} = get_TypeName($FuncDescr{$FuncInfoId}{"Class"});
    }
    if($Interface_Library{$FuncDescr{$FuncInfoId}{"ShortName"}}
    and $FuncDescr{$FuncInfoId}{"Type"} eq "Function") {
        # functions (C++): not mangled in library
        if(not $FuncDescr{$FuncInfoId}{"MnglName"}
          or not $Interface_Library{$FuncDescr{$FuncInfoId}{"MnglName"}}) {
            $FuncDescr{$FuncInfoId}{"MnglName"} = $FuncDescr{$FuncInfoId}{"ShortName"};
        }
    }
    if(not $FuncDescr{$FuncInfoId}{"MnglName"} and not $FuncDescr{$FuncInfoId}{"Class"})
    {# functions (C++): mangled in library, but is not mangled in syntax tree
        if(my $MangledByGcc = $mangled_name_gcc{construct_signature($FuncInfoId)}) {
            $FuncDescr{$FuncInfoId}{"MnglName"} = $MangledByGcc;
        }
        elsif(my $Mangled = $mangled_name{synchTr(construct_signature($FuncInfoId))}) {
            $FuncDescr{$FuncInfoId}{"MnglName"} = $Mangled;
        }
    }
    if($FuncDescr{$FuncInfoId}{"Constructor"}
    or $FuncDescr{$FuncInfoId}{"Destructor"}) {
        delete($FuncDescr{$FuncInfoId}{"Return"});
    }
    if(($FuncDescr{$FuncInfoId}{"Type"} eq "Method")
    or $FuncDescr{$FuncInfoId}{"Constructor"} or $FuncDescr{$FuncInfoId}{"Destructor"})
    {
        if($FuncDescr{$FuncInfoId}{"MnglName"}!~/\A(_Z|\?)/)
        {
            delete($FuncDescr{$FuncInfoId});
            return;
        }
    }
    if(not $FuncDescr{$FuncInfoId}{"MnglName"}
    and not $FuncDescr{$FuncInfoId}{"Class"} and ($FuncDescr{$FuncInfoId}{"Type"} eq "Function")) {
        $FuncDescr{$FuncInfoId}{"MnglName"} = $FuncDescr{$FuncInfoId}{"ShortName"};
    }
    if($InternalInterfaces{$FuncDescr{$FuncInfoId}{"MnglName"}})
    {
        delete($FuncDescr{$FuncInfoId});
        return;
    }
    if($MangledNames{$FuncDescr{$FuncInfoId}{"MnglName"}})
    {# one instance for one mangled name only
        delete($FuncDescr{$FuncInfoId});
        return;
    }
    else {
        $MangledNames{$FuncDescr{$FuncInfoId}{"MnglName"}} = 1;
    }
    if(isInline($FuncInfoId)) {
        $FuncDescr{$FuncInfoId}{"InLine"} = 1;
    }
    if($Interface_Library{$FuncDescr{$FuncInfoId}{"MnglName"}}
    and $FuncDescr{$FuncInfoId}{"Class"}) {
        $ClassInTargetLibrary{$FuncDescr{$FuncInfoId}{"Class"}} = 1;
    }
    if($FuncDescr{$FuncInfoId}{"MnglName"}=~/\A(_Z|\?)/ and $FuncDescr{$FuncInfoId}{"Class"})
    {
        if($FuncDescr{$FuncInfoId}{"Type"} eq "Function")
        {# static methods
            $FuncDescr{$FuncInfoId}{"Static"} = 1;
        }
    }
    if(hasThrow($FuncInfoId)) {
        $FuncDescr{$FuncInfoId}{"Throw"} = 1;
    }
    if(getFuncLink($FuncInfoId) eq "Static") {
        $FuncDescr{$FuncInfoId}{"Static"} = 1;
    }
    if($FuncDescr{$FuncInfoId}{"Type"} eq "Function"
    and not $FuncDescr{$FuncInfoId}{"Class"}) {
        $FuncDescr{$FuncInfoId}{"TypeId"} = getFuncTypeId($FuncInfoId);
    }
    if($FuncDescr{$FuncInfoId}{"Type"} eq "Function"
    and not $FuncDescr{$FuncInfoId}{"Class"}) {
        $Func_TypeId{$FuncDescr{$FuncInfoId}{"TypeId"}}{$FuncDescr{$FuncInfoId}{"MnglName"}} = 1;
    }
    if(not $Interface_Library{$FuncDescr{$FuncInfoId}{"MnglName"}} and $FuncDescr{$FuncInfoId}{"Constructor"}
    and keys(%{$TemplateInstance{$Tid_TDid{$FuncDescr{$FuncInfoId}{"Class"}}}{$FuncDescr{$FuncInfoId}{"Class"}}})>0)
    {
        my $ClassName = get_type_short_name(get_TypeName($FuncDescr{$FuncInfoId}{"Class"}));
        $InlineTmplCtor{$ClassName}{$FuncInfoId} = 1;
    }
    if($FuncDescr{$FuncInfoId}{"Type"} eq "Method") {
        $ClassesWithMethods{$FuncDescr{$FuncInfoId}{"Class"}} = 1;
    }
    if($Interface_Library{$FuncDescr{$FuncInfoId}{"MnglName"}}
    and $GlibcHeader{$FuncDescr{$FuncInfoId}{"Header"}} and not $GLIBC_TESTING)
    {# solve collisions with simple name of headers (error.h)
        if(my $HeaderDir = find_in_dependencies($FuncDescr{$FuncInfoId}{"Header"})) {
            $FuncDescr{$FuncInfoId}{"Header"} = joinPath(get_filename($HeaderDir),$FuncDescr{$FuncInfoId}{"Header"});
        }
    }
}

sub is_transit_function($)
{
    my $ShortName = $_[0];
    return 1 if($ShortName=~/(_|\A)dup(_|\Z)|(dup\Z)|_dup/i);
    return 1 if($ShortName=~/replace|merge|search|copy|append|duplicat|find|query|open|handle|first|next|entry/i);
    return grep(/\A(get|prev|last|from|of|dup)\Z/i, @{get_tokens($ShortName)});
}

sub get_TypeLib($)
{
    my $TypeId = $_[0];
    return $Cache{"get_TypeLib"}{$TypeId} if(defined $Cache{"get_TypeLib"}{$TypeId} and not defined $AuxType{$TypeId});
    my $Header = $TypeDescr{$Tid_TDid{$TypeId}}{$TypeId}{"Header"};
    foreach my $Interface (sort keys(%{$Header_Interface{$Header}}))
    {
        if(my $SoLib = get_filename($Interface_Library{$Interface}))
        {
            $Cache{"get_TypeLib"}{$TypeId} = $SoLib;
            return $SoLib;
        }
        elsif(my $SoLib = get_filename($NeededInterface_Library{$Interface}))
        {
            $Cache{"get_TypeLib"}{$TypeId} = $SoLib;
            return $SoLib;
        }
    }
    $Cache{"get_TypeLib"}{$TypeId} = "unknown";
    return $Cache{"get_TypeLib"}{$TypeId};
}

sub getTypeShortName($)
{
    my $TypeName = $_[0];
    $TypeName=~s/\<.*\>//g;
    $TypeName=~s/.*\:\://g;
    return $TypeName;
}

sub getTypeId($)
{
    my $TypeInfo = $LibInfo{$_[0]}{"info"};
    if($TypeInfo=~/type[ ]*:[ ]*@(\d+) /) {
        return $1;
    }
    else {
        return "";
    }
}

sub getFuncId($)
{
    my $FuncInfo = $LibInfo{$_[0]}{"info"};
    if($FuncInfo=~/type[ ]*:[ ]*@(\d+) /) {
        return $1;
    }
    else {
        return "";
    }
}

sub setTypeMemb($$)
{
    my ($TypeId, $TypeAttr) = @_;
    my $TypeType = $TypeAttr->{"Type"};
    my $Position = 0;
    if($TypeType eq "Enum")
    {
        my $TypeMembInfoId = getEnumMembInfoId($TypeId);
        while($TypeMembInfoId)
        {
            $TypeAttr->{"Memb"}{$Position}{"value"} = getEnumMembVal($TypeMembInfoId);
            my $MembName = getEnumMembName($TypeMembInfoId);
            $TypeAttr->{"Memb"}{$Position}{"name"} = $MembName;
            $EnumMembers{$TypeAttr->{"Memb"}{$Position}{"name"}} = 1;
            $EnumMembName_Id{getTreeAttr($TypeMembInfoId, "valu")} = ($TypeAttr->{"NameSpace"})?$TypeAttr->{"NameSpace"}."::".$MembName:$MembName;
            $TypeMembInfoId = getNextMembInfoId($TypeMembInfoId);
            $Position += 1;
        }
    }
    elsif($TypeType=~/\A(Struct|Class|Union)\Z/)
    {
        my $TypeMembInfoId = getStructMembInfoId($TypeId);
        while($TypeMembInfoId)
        {
            if($LibInfo{$TypeMembInfoId}{"info_type"} ne "field_decl")
            {
                $TypeMembInfoId = getNextStructMembInfoId($TypeMembInfoId);
                next;
            }
            my $StructMembName = getStructMembName($TypeMembInfoId);
            if($StructMembName=~/_vptr\./)
            {#virtual tables
                $TypeMembInfoId = getNextStructMembInfoId($TypeMembInfoId);
                next;
            }
            if(not $StructMembName)
            {#base classes
                $TypeMembInfoId = getNextStructMembInfoId($TypeMembInfoId);
                next;
            }
            $TypeAttr->{"Memb"}{$Position}{"type"} = getStructMembType($TypeMembInfoId);
            $TypeAttr->{"Memb"}{$Position}{"name"} = $StructMembName;
            $TypeAttr->{"Memb"}{$Position}{"access"} = getTreeAccess($TypeMembInfoId);
            $TypeAttr->{"Memb"}{$Position}{"bitfield"} = getStructMembBitFieldSize($TypeMembInfoId);
            $TypeMembInfoId = getNextStructMembInfoId($TypeMembInfoId);
            $Position += 1;
        }
    }
}

sub isAnonTypedef($)
{
    my $TypeId = $_[0];
    if(isAnon(getTreeTypeName($TypeId))) {
        return 0;
    }
    else {
        return isAnon(getNameByInfo(anonTypedef($TypeId)));
    }
}

sub anonTypedef($)
{
    my $TypeId = $_[0];
    my $TypeMembInfoId;
    $TypeMembInfoId = getStructMembInfoId($TypeId);
    while($TypeMembInfoId)
    {
        my $NextTypeMembInfoId = getNextStructMembInfoId($TypeMembInfoId);
        if(not $NextTypeMembInfoId)
        {
            last;
        }
        $TypeMembInfoId = $NextTypeMembInfoId;
        if($LibInfo{$TypeMembInfoId}{"info_type"} eq "type_decl"
        and getNameByInfo($TypeMembInfoId) eq getTreeTypeName($TypeId)) {
            return 0;
        }
    }
    return $TypeMembInfoId;
}

sub setFuncParams($)
{
    my $FuncInfoId = $_[0];
    my $ParamInfoId = getFuncParamInfoId($FuncInfoId);
    if(getFuncType($FuncInfoId) eq "Method")
    {
        $ParamInfoId = getNextElem($ParamInfoId);
    }
    my ($Position, $Vtt_Pos) = (0, -1);
    while($ParamInfoId)
    {
        my $ParamTypeId = getFuncParamType($ParamInfoId);
        my $ParamName = getFuncParamName($ParamInfoId);
        last if(get_TypeName($ParamTypeId) eq "void");
        if(get_TypeType($ParamTypeId) eq "Restrict")
        {# delete restrict spec
            $ParamTypeId = getRestrictBase($ParamTypeId);
        }
        if(keys(%{$ExplicitTypedef{$ParamTypeId}})==1)
        {# replace the type to according explicit typedef
            $ParamTypeId = -$ParamTypeId;
        }
        if(not get_TypeName($ParamTypeId))
        {
            $FuncDescr{$FuncInfoId}{"Skip"} = 1;
            return;
        }
        if($ParamName eq "__vtt_parm"
        and get_TypeName($ParamTypeId) eq "void const**")
        {
            $Vtt_Pos = $Position;
            $ParamInfoId = getNextElem($ParamInfoId);
            next;
        }
        $FuncDescr{$FuncInfoId}{"Param"}{$Position}{"type"} = $ParamTypeId;
        $FuncDescr{$FuncInfoId}{"Param"}{$Position}{"name"} = $ParamName;
        if(not $FuncDescr{$FuncInfoId}{"Param"}{$Position}{"name"}) {
            $FuncDescr{$FuncInfoId}{"Param"}{$Position}{"name"} = "p".($Position+1);
        }
        $ParamInfoId = getNextElem($ParamInfoId);
        $Position += 1;
    }
    if(detect_nolimit_args($FuncInfoId, $Vtt_Pos)) {
        $FuncDescr{$FuncInfoId}{"Param"}{$Position}{"type"} = -1;
    }
}

sub detect_nolimit_args($$)
{
    my ($FuncInfoId, $Vtt_Pos) = @_;
    my $FuncTypeId = getFuncTypeId($FuncInfoId);
    my $ParamListElemId = getFuncParamTreeListId($FuncTypeId);
    my $FunctionType = getFuncType($FuncInfoId);
    if(getFuncType($FuncInfoId) eq "Method")
    {
        $ParamListElemId = getNextElem($ParamListElemId);
    }
    my $HaveVoid = 0;
    my $Position = 0;
    while($ParamListElemId)
    {
        if($Vtt_Pos!=-1 and $Position==$Vtt_Pos)
        {
            $Vtt_Pos=-1;
            $ParamListElemId = getNextElem($ParamListElemId);
            next;
        }
        my $ParamTypeId = getTreeAttr($ParamListElemId, "valu");
        if(getTreeAttr($ParamListElemId, "purp"))
        {
            $FuncDescr{$FuncInfoId}{"Param"}{$Position}{"default"} = 1;
        }
        if(get_TypeName($ParamTypeId) eq "void")
        {
            $HaveVoid = 1;
            last;
        }
        elsif(not defined $FuncDescr{$FuncInfoId}{"Param"}{$Position}{"type"})
        {
            $FuncDescr{$FuncInfoId}{"Param"}{$Position}{"type"} = $ParamTypeId;
            if(not $FuncDescr{$FuncInfoId}{"Param"}{$Position}{"name"}) {
                $FuncDescr{$FuncInfoId}{"Param"}{$Position}{"name"} = "p".($Position+1);
            }
        }
        $ParamListElemId = getNextElem($ParamListElemId);
        $Position += 1;
    }
    return ($Position>=1 and not $HaveVoid);
}

sub getFuncParamTreeListId($)
{
    my $FuncTypeId = $_[0];
    if($LibInfo{$FuncTypeId}{"info"}=~/prms[ ]*:[ ]*@(\d+) /) {
        return $1;
    }
    else {
        return "";
    }
}

sub getTreeAttr($$)
{
    my ($Id, $Attr) = @_;
    if($LibInfo{$Id}{"info"}=~/$Attr[ ]*:[ ]*@(\d+) /) {
        return $1;
    }
    else {
        return "";
    }
}

sub getTreeValue($)
{
    if($LibInfo{$_[0]}{"info"}=~/low[ ]*:[ ]*([^ ]+) /) {
        return $1;
    }
    else {
        return "";
    }
}

sub getRestrictBase($)
{
    my $TypeId = $_[0];
    my $TypeDeclId = getTypeDeclId($TypeId);
    my $BaseTypeId = $TypeDescr{$TypeDeclId}{$TypeId}{"BaseType"}{"Tid"};
    my $BaseTypeDeclId = $TypeDescr{$TypeDeclId}{$TypeId}{"BaseType"}{"TDid"};
    return $BaseTypeId;
}

sub getTreeAccess($)
{
    my $InfoId = $_[0];
    if($LibInfo{$InfoId}{"info"}=~/accs[ ]*:[ ]*([a-zA-Z]+) /)
    {
        my $Access = $1;
        if($Access eq "prot") {
            return "protected";
        }
        elsif($Access eq "priv") {
            return "private";
        }
    }
    elsif($LibInfo{$InfoId}{"info"}=~/ protected /)
    {# support for old GCC versions
        return "protected";
    }
    elsif($LibInfo{$InfoId}{"info"}=~/ private /)
    {# support for old GCC versions
        return "private";
    }
    return "public";
}


sub setFuncAccess($)
{
    my $FuncInfoId = $_[0];
    my $Access = getTreeAccess($FuncInfoId);
    if($Access eq "protected") {
        $FuncDescr{$FuncInfoId}{"Protected"} = 1;
    }
    elsif($Access eq "private") {
        $FuncDescr{$FuncInfoId}{"Private"} = 1;
    }
}

sub setTypeAccess($$)
{
    my ($TypeId, $TypeAttr) = @_;
    my $Access = getTreeAccess($TypeId);
    if($Access eq "protected") {
        $TypeAttr->{"Protected"} = 1;
    }
    elsif($Access eq "private") {
        $TypeAttr->{"Private"} = 1;
    }
}

sub setFuncKind($)
{
    my $FuncInfoId = $_[0];
    if($LibInfo{$FuncInfoId}{"info"}=~/pseudo tmpl/) {
        $FuncDescr{$FuncInfoId}{"PseudoTemplate"} = 1;
    }
    elsif($LibInfo{$FuncInfoId}{"info"}=~/ constructor /) {
        $FuncDescr{$FuncInfoId}{"Constructor"} = 1;
    }
    elsif($LibInfo{$FuncInfoId}{"info"}=~/ destructor /) {
        $FuncDescr{$FuncInfoId}{"Destructor"} = 1;
    }
}

sub getFuncSpec($)
{
    my $FuncInfoId = $_[0];
    my $FuncInfo = $LibInfo{$FuncInfoId}{"info"};
    if($FuncInfo=~/spec[ ]*:[ ]*pure /) {
        return "PureVirt";
    }
    elsif($FuncInfo=~/spec[ ]*:[ ]*virt /) {
        return "Virt";
    }
    elsif($FuncInfo=~/ pure virtual /)
    {# support for old GCC versions
        return "PureVirt";
    }
    elsif($FuncInfo=~/ virtual /)
    {# support for old GCC versions
        return "Virt";
    }
    return "";
}

sub set_Class_And_Namespace($)
{
    my $FuncInfoId = $_[0];
    my $FuncInfo = $LibInfo{$FuncInfoId}{"info"};
    if($FuncInfo=~/scpe[ ]*:[ ]*@(\d+) /)
    {
        my $NameSpaceInfoId = $1;
        if($LibInfo{$NameSpaceInfoId}{"info_type"} eq "namespace_decl")
        {
            if((my $NS = getNameSpace($FuncInfoId)) ne "\@skip\@") {
                $FuncDescr{$FuncInfoId}{"NameSpace"} = $NS;
            }
        }
        elsif($LibInfo{$NameSpaceInfoId}{"info_type"} eq "record_type") {
            $FuncDescr{$FuncInfoId}{"Class"} = $NameSpaceInfoId;
        }
    }
}

sub getFuncLink($)
{
    my $FuncInfoId = $_[0];
    my $FuncInfo = $LibInfo{$FuncInfoId}{"info"};
    if($FuncInfo=~/link[ ]*:[ ]*static /) {
        return "Static";
    }
    else
    {
        if($FuncInfo=~/link[ ]*:[ ]*([a-zA-Z]+) /) {
            return $1;
        }
        else {
            return "";
        }
    }
}

sub getNextElem($)
{
    my $FuncInfoId = $_[0];
    my $FuncInfo = $LibInfo{$FuncInfoId}{"info"};
    if($FuncInfo=~/(chan|chain)[ ]*:[ ]*@(\d+) /) {
        return $2;
    }
    else {
        return "";
    }
}

sub getFuncParamInfoId($)
{
    my $FuncInfoId = $_[0];
    my $FuncInfo = $LibInfo{$FuncInfoId}{"info"};
    if($FuncInfo=~/args[ ]*:[ ]*@(\d+) /) {
        return $1;
    }
    else {
        return "";
    }
}

sub getFuncParamType($)
{
    my $ParamInfoId = $_[0];
    my $ParamInfo = $LibInfo{$ParamInfoId}{"info"};
    if($ParamInfo=~/type[ ]*:[ ]*@(\d+) /) {
        return $1;
    }
    else {
        return "";
    }
}

sub getFuncParamName($)
{
    my $ParamInfoId = $_[0];
    my $ParamInfo = $LibInfo{$ParamInfoId}{"info"};
    if($ParamInfo=~/name[ ]*:[ ]*@(\d+) /) {
        return getTreeStr($1);
    }
    return "";
}

sub getEnumMembInfoId($)
{
    if($LibInfo{$_[0]}{"info"}=~/csts[ ]*:[ ]*@(\d+) /) {
        return $1;
    }
    else {
        return "";
    }
}

sub getStructMembInfoId($)
{
    if($LibInfo{$_[0]}{"info"}=~/flds[ ]*:[ ]*@(\d+) /) {
        return $1;
    }
    else {
        return "";
    }
}

sub getNameSpace($)
{
    my $TypeInfoId = $_[0];
    my $TypeInfo = $LibInfo{$TypeInfoId}{"info"};
    return "" if($TypeInfo!~/scpe[ ]*:[ ]*@(\d+) /);
    my $NameSpaceInfoId = $1;
    if($LibInfo{$NameSpaceInfoId}{"info_type"} eq "namespace_decl")
    {
        my $NameSpaceInfo = $LibInfo{$NameSpaceInfoId}{"info"};
        if($NameSpaceInfo=~/name[ ]*:[ ]*@(\d+) /)
        {
            my $NameSpace = getTreeStr($1);
            return "" if($NameSpace eq "::");
            $NameSpaces{$NameSpace} = 1;
            if(my $BaseNameSpace = getNameSpace($NameSpaceInfoId))
            {
                $NameSpace = $BaseNameSpace."::".$NameSpace;
                $NameSpaces{$BaseNameSpace} = 1;
            }
            $NestedNameSpaces{$NameSpace} = 1;
            return $NameSpace;
        }
        else {
            return "";
        }
    }
    elsif($LibInfo{$NameSpaceInfoId}{"info_type"} eq "record_type")
    {
        my %NameSpaceAttr = getTypeAttr(getTypeDeclId($NameSpaceInfoId), $NameSpaceInfoId);
        if(my $StructName = $NameSpaceAttr{"Name"}) {
            return $StructName;
        }
        else {
            return "\@skip\@";
        }
    }
    else {
        return "";
    }
}

sub getNameSpaceId($)
{
    if($LibInfo{$_[0]}{"info"}=~/scpe[ ]*:[ ]*@(\d+) /) {
        return $1;
    }
    else {
        return "";
    }
}

sub getEnumMembName($)
{
    if($LibInfo{$_[0]}{"info"}=~/purp[ ]*:[ ]*@(\d+) /) {
        return getTreeStr($1);
    }
    else {
        return "";
    }
}

sub getStructMembName($)
{
    if($LibInfo{$_[0]}{"info"}=~/name[ ]*:[ ]*@(\d+) /) {
        return getTreeStr($1);
    }
    else {
        return "";
    }
}

sub getEnumMembVal($)
{
    if($LibInfo{$_[0]}{"info"}=~/valu\s*:\s*\@(\d+)/)
    {
        if($LibInfo{$1}{"info"}=~/cnst\s*:\s*\@(\d+)/)
        {# in newer versions of GCC the value is in the "const_decl->cnst" node
            return getTreeValue($1);
        }
        else
        {# some old versions of GCC (3.3) have the value in the "integer_cst" node
            return getTreeValue($1);
        }
    }
    return "";
}

sub getSize($)
{
    if($LibInfo{$_[0]}{"info"}=~/size[ ]*:[ ]*@(\d+) /) {
        return getTreeValue($1);
    }
    else {
        return 0;
    }
}

sub getStructMembType($)
{
    if($LibInfo{$_[0]}{"info"}=~/type[ ]*:[ ]*@(\d+) /) {
        return $1;
    }
    else {
        return "";
    }
}

sub getStructMembBitFieldSize($)
{
    if($LibInfo{$_[0]}{"info"}=~/ bitfield /) {
        return getSize($_[0]);
    }
    else {
        return 0;
    }
}

sub getNextMembInfoId($)
{
    if($LibInfo{$_[0]}{"info"}=~/(chan|chain)[ ]*:[ ]*@(\d+) /) {
        return $2;
    }
    else {
        return "";
    }
}

sub getNextStructMembInfoId($)
{
    if($LibInfo{$_[0]}{"info"}=~/(chan|chain)[ ]*:[ ]*@(\d+) /) {
        return $2;
    }
    else {
        return "";
    }
}

sub fieldHasName($)
{
    my $TypeMembInfoId = $_[0];
    if($LibInfo{$TypeMembInfoId}{"info_type"} eq "field_decl")
    {
        if($LibInfo{$TypeMembInfoId}{"info"}=~/name[ ]*:[ ]*@(\d+) /) {
            return $1;
        }
        else {
            return "";
        }
    }
    else {
        return 0;
    }
}

sub getHeader($)
{
    my $TypeInfo = $LibInfo{$_[0]}{"info"};
    if($TypeInfo=~/srcp[ ]*:[ ]*([\w\-\<\>\.\+\/\\]+):(\d+) /) {
        return $1;
    }
    else {
        return "";
    }
}

sub getLine($)
{
    if($LibInfo{$_[0]}{"info"}=~/srcp[ ]*:[ ]*([\w\-\<\>\.\+\/\\]+):(\d+) /) {
        return $2;
    }
    else {
        return "";
    }
}

sub getTypeTypeByTypeId($)
{
    my $TypeId = $_[0];
    my $TypeType = $LibInfo{$TypeId}{"info_type"};
    if($TypeType=~/integer_type|real_type|boolean_type|void_type|complex_type/)
    {
        return "Intrinsic";
    }
    elsif(isFuncPtr($TypeId)) {
        return "FuncPtr";
    }
    elsif(isMethodPtr($TypeId)) {
        return "MethodPtr";
    }
    elsif(isFieldPtr($TypeId)) {
        return "FieldPtr";
    }
    elsif($TypeType eq "pointer_type") {
        return "Pointer";
    }
    elsif($TypeType eq "reference_type") {
        return "Ref";
    }
    elsif($TypeType eq "union_type") {
        return "Union";
    }
    elsif($TypeType eq "enumeral_type") {
        return "Enum";
    }
    elsif($TypeType eq "record_type") {
        return "Struct";
    }
    elsif($TypeType eq "array_type") {
        return "Array";
    }
    elsif($TypeType eq "complex_type") {
        return "Intrinsic";
    }
    elsif($TypeType eq "function_type") {
        return "FunctionType";
    }
    elsif($TypeType eq "method_type") {
        return "MethodType";
    }
    else {
        return "Unknown";
    }
}

sub getNameByInfo($)
{
    my $TypeInfo = $LibInfo{$_[0]}{"info"};
    return "" if($TypeInfo!~/name[ ]*:[ ]*@(\d+) /);
    if($LibInfo{$1}{"info"}=~/strg[ ]*:[ ]*(.*?)[ ]+lngt/)
    {# short unsigned int (may include spaces)
        return $1;
    }
    else {
        return "";
    }
}

sub getTreeStr($)
{
    my $Info = $LibInfo{$_[0]}{"info"};
    if($Info=~/strg[ ]*:[ ]*([^ ]*)/)
    {
        my $Str = $1;
        if($C99Mode and $Str=~/\Ac99_(.+)\Z/) {
            if($CppKeywords_A{$1}) {
                $Str=$1;
            }
        }
        return $Str;
    }
    else {
        return "";
    }
}

sub getVarShortName($)
{
    my $VarInfo = $LibInfo{$_[0]}{"info"};
    return "" if($VarInfo!~/name[ ]*:[ ]*@(\d+) /);
    return getTreeStr($1);
}

sub getFuncShortName($)
{
    my $FuncInfo = $LibInfo{$_[0]}{"info"};
    if($FuncInfo=~/ operator /)
    {
        if($FuncInfo=~/ conversion /) {
            return "operator ".get_TypeName($FuncDescr{$_[0]}{"Return"});
        }
        else
        {
            return "" if($FuncInfo!~/ operator[ ]+([a-zA-Z]+) /);
            return "operator".$Operator_Indication{$1};
        }
    }
    else
    {
        return "" if($FuncInfo!~/name[ ]*:[ ]*@(\d+) /);
        return getTreeStr($1);
    }
}

sub getFuncMnglName($)
{
    my $FuncInfo = $LibInfo{$_[0]}{"info"};
    return "" if($FuncInfo!~/mngl[ ]*:[ ]*@(\d+) /);
    return getTreeStr($1);
}

sub getFuncReturn($)
{
    my $FuncInfo = $LibInfo{$_[0]}{"info"};
    return "" if($FuncInfo!~/type[ ]*:[ ]*@(\d+) /);
    my $FuncTypeInfoId = $1;
    return "" if($LibInfo{$FuncTypeInfoId}{"info"}!~/retn[ ]*:[ ]*@(\d+) /);
    my $FuncReturnTypeId = $1;
    if(get_TypeType($FuncReturnTypeId) eq "Restrict")
    {#delete restrict spec
        $FuncReturnTypeId = getRestrictBase($FuncReturnTypeId);
    }
    return $FuncReturnTypeId;
}

sub isInline($)
{# "body: undefined" in the tree
 # -fkeep-inline-functions GCC option should be specified
    my $FuncInfo = $LibInfo{$_[0]}{"info"};
    if($FuncInfo=~/ undefined /i) {
        return 0;
    }
    return 1;
}

sub getFuncOrig($)
{
    my $FuncInfo = $LibInfo{$_[0]}{"info"};
    my $FuncOrigInfoId;
    if($FuncInfo=~/orig[ ]*:[ ]*@(\d+) /) {
        return $1;
    }
    else {
        return $_[0];
    }
}

sub getFuncType($)
{
    my $FuncInfo = $LibInfo{$_[0]}{"info"};
    return "" if($FuncInfo!~/type[ ]*:[ ]*@(\d+) /);
    my $FunctionType = $LibInfo{$1}{"info_type"};
    if($FunctionType eq "method_type") {
        return "Method";
    }
    elsif($FunctionType eq "function_type") {
        return "Function";
    }
    else {
        return $FunctionType;
    }
}

sub hasThrow($)
{
    my $FuncInfo = $LibInfo{$_[0]}{"info"};
    return 0 if($FuncInfo!~/type[ ]*:[ ]*@(\d+) /);
    return getTreeAttr($1, "unql");
}

sub getFuncTypeId($)
{
    my $FuncInfo = $LibInfo{$_[0]}{"info"};
    if($FuncInfo=~/type[ ]*:[ ]*@(\d+)( |\Z)/) {
        return $1;
    }
    else {
        return 0;
    }
}

sub cover_typedefs_in_template($)
{
    my $ClassId = $_[0];
    return "" if(not $ClassId);
    my $ClassName = get_TypeName($ClassId);
    my $ClassDId = $Tid_TDid{$ClassId};
    foreach my $Param_Pos (sort {int($a)<=>int($b)} keys(%{$TemplateInstance{$ClassDId}{$ClassId}}))
    {
        my $ParamTypeId = $TemplateInstance{$ClassDId}{$ClassId}{$Param_Pos};
        my $ParamTypeName = get_TypeName($ParamTypeId);
        my $ParamFTypeId = get_FoundationTypeId($ParamTypeId);
        if($ParamTypeName=~/<|>|::/ and get_TypeType($ParamFTypeId)=~/\A(Class|Struct)\Z/)
        {
            if(my $Typedef_Id = detect_typedef($ParamTypeId)) {
                $ClassName = cover_by_typedef($ClassName, $ParamFTypeId, $Typedef_Id);
            }
        }
    }
    return $ClassName;
}

sub detect_typedef($)
{
    my $Type_Id = $_[0];
    return "" if(not $Type_Id);
    my $Typedef_Id = get_base_typedef($Type_Id);
    if(not $Typedef_Id) {
        $Typedef_Id = get_type_typedef(get_FoundationTypeId($Type_Id));
    }
    return $Typedef_Id;
}

sub get_Signature($)
{
    my $Interface = $_[0];
    return $Cache{"get_Signature"}{$Interface} if($Cache{"get_Signature"}{$Interface});
    if(not $CompleteSignature{$Interface}{"Header"})
    {
        if($Interface=~/\A(_Z|\?)/)
        {
            $Cache{"get_Signature"}{$Interface} = $tr_name{$Interface};
            return $Cache{"get_Signature"}{$Interface};
        }
        else
        {
            $Cache{"get_Signature"}{$Interface} = $Interface;
            return $Interface;
        }
    }
    my ($Func_Signature, @Param_Types_FromUnmangledName) = ();
    my $ShortName = $CompleteSignature{$Interface}{"ShortName"};
    if($Interface=~/\A(_Z|\?)/)
    {
        if(my $ClassId = $CompleteSignature{$Interface}{"Class"})
        {
            if(get_TypeName($ClassId)=~/<|>|::/
            and my $Typedef_Id = detect_typedef($ClassId)) {
                $ClassId = $Typedef_Id;
            }
            $Func_Signature = get_TypeName($ClassId)."::".((($CompleteSignature{$Interface}{"Destructor"}))?"~":"").$ShortName;
        }
        elsif(my $NameSpace = $CompleteSignature{$Interface}{"NameSpace"}) {
            $Func_Signature = $NameSpace."::".$ShortName;
        }
        else {
            $Func_Signature = $ShortName;
        }
        @Param_Types_FromUnmangledName = get_Signature_Parts($tr_name{$Interface}, 0);
    }
    else {
        $Func_Signature = $Interface;
    }
    my @ParamArray = ();
    foreach my $Pos (sort {int($a) <=> int($b)} keys(%{$CompleteSignature{$Interface}{"Param"}}))
    {
        next if($Pos eq "");
        my $ParamTypeId = $CompleteSignature{$Interface}{"Param"}{$Pos}{"type"};
        next if(not $ParamTypeId);
        my $ParamTypeName = get_TypeName($ParamTypeId);
        $ParamTypeName = $Param_Types_FromUnmangledName[$Pos] if(not $ParamTypeName);
        my $ParamFTypeId = get_FoundationTypeId($ParamTypeId);
        if($ParamTypeName=~/<|>|::/ and get_TypeType($ParamFTypeId)=~/\A(Class|Struct)\Z/)
        {
            if(my $Typedef_Id = detect_typedef($ParamTypeId)) {
                $ParamTypeName = cover_by_typedef($ParamTypeName, $ParamFTypeId, $Typedef_Id);
            }
        }
        if(my $ParamName = $CompleteSignature{$Interface}{"Param"}{$Pos}{"name"}) {
            push(@ParamArray, create_member_decl($ParamTypeName, $ParamName));
        }
        else {
            push(@ParamArray, $ParamTypeName);
        }
    }
    if(not $CompleteSignature{$Interface}{"Data"})
    {
        if($Interface=~/\A(_Z|\?)/)
        {
            if($CompleteSignature{$Interface}{"Constructor"})
            {
                if($Interface=~/C1E/) {
                    $Func_Signature .= " [in-charge]";
                }
                elsif($Interface=~/C2E/) {
                    $Func_Signature .= " [not-in-charge]";
                }
            }
            elsif($CompleteSignature{$Interface}{"Destructor"})
            {
                if($Interface=~/D1E/) {
                    $Func_Signature .= " [in-charge]";
                }
                elsif($Interface=~/D2E/) {
                    $Func_Signature .= " [not-in-charge]";
                }
                elsif($Interface=~/D0E/) {
                    $Func_Signature .= " [in-charge-deleting]";
                }
            }
        }
        $Func_Signature .= " (".join(", ", @ParamArray).")";
        if($Interface=~/\A_ZNK/) {
            $Func_Signature .= " const";
        }
        if($CompleteSignature{$Interface}{"Static"}) {
            $Func_Signature .= " [static]";
        }
    }
    if(defined $ShowRetVal
    and my $ReturnTId = $CompleteSignature{$Interface}{"Return"})
    {
        my $ReturnTypeName = get_TypeName($ReturnTId);
        my $ReturnFTypeId = get_FoundationTypeId($ReturnTId);
        if($ReturnTypeName=~/<|>|::/ and get_TypeType($ReturnFTypeId)=~/\A(Class|Struct)\Z/)
        {
            if(my $Typedef_Id = detect_typedef($ReturnTId)) {
                $ReturnTypeName = cover_by_typedef($ReturnTypeName, $ReturnFTypeId, $Typedef_Id);
            }
        }
        $Func_Signature .= ":".$ReturnTypeName;
    }
    $Cache{"get_Signature"}{$Interface} = $Func_Signature;
    return $Func_Signature;
}

sub htmlSpecChars($)
{
    my $Str = $_[0];
    $Str=~s/\&([^#]|\Z)/&amp;$1/g;
    $Str=~s/</&lt;/g;
    $Str=~s/>/&gt;/g;
    $Str=~s/([^ ])( )([^ ])/$1\@ALONE_SP\@$3/g;
    $Str=~s/ /&nbsp;/g;
    $Str=~s/\@ALONE_SP\@/ /g;
    $Str=~s/\n/<br\/>/g;
    return $Str;
}

sub highLight_Signature($)
{
    my $Signature = $_[0];
    return highLight_Signature_PPos_Italic($Signature, "", 0, 0);
}

sub highLight_Signature_Italic_Color($)
{
    my $Signature = $_[0];
    return highLight_Signature_PPos_Italic($Signature, "", 1, 1);
}

sub highLight_Signature_PPos_Italic($$$$)
{
    my ($Signature, $Parameter_Position, $ItalicParams, $ColorParams) = @_;
    my ($Begin, $End, $Return) = (substr($Signature, 0, get_signature_center($Signature)), "", "");
    if($ShowRetVal and $Signature=~s/([^:]):([^:].+?)\Z/$1/g) {
        $Return = $2;
    }
    if($Signature=~/\)((| const)(| \[static\]))\Z/) {
        $End = $1;
    }
    my @Parts = ();
    my $Part_Num = 0;
    foreach my $Part (get_Signature_Parts($Signature, 1))
    {
        $Part=~s/\A\s+|\s+\Z//g;
        my ($Part_Styled, $ParamName) = (htmlSpecChars($Part), "");
        if($Part=~/\([\*]+(\w+)\)/i) {
            $ParamName = $1;#func-ptr
        }
        elsif($Part=~/(\w+)[\,\)]*\Z/i) {
            $ParamName = $1;
        }
        if(not $ParamName) {
            push(@Parts, $Part);
            next;
        }
        if($ItalicParams and not $TName_Tid{$Part})
        {
            if($Parameter_Position ne "" and $Part_Num==$Parameter_Position) {
                $Part_Styled =~ s!(\W)$ParamName([\,\)]|\Z)!$1<span class='focus_p'>$ParamName</span>$2!ig;
            }
            elsif($ColorParams) {
                $Part_Styled =~ s!(\W)$ParamName([\,\)]|\Z)!$1<span class='color_p'>$ParamName</span>$2!ig;
            }
            else {
                $Part_Styled =~ s!(\W)$ParamName([\,\)]|\Z)!$1<span style='font-style:italic;'>$ParamName</span>$2!ig;
            }
        }
        $Part_Styled=~s/,(\w)/, $1/g;
        if(length($Part)<=45) {
            $Part_Styled = "<span style='white-space:nowrap;'>".$Part_Styled."</span>";
        }
        push(@Parts, $Part_Styled);
        $Part_Num += 1;
    }
    $Signature = htmlSpecChars($Begin)."<span class='int_p'>"."(&nbsp;".join(" ", @Parts).(@Parts?"&nbsp;":"").")"."</span>".$End;
    if($Return) {
        $Signature .= "<span class='int_p' style='white-space:nowrap'> &nbsp;<b>:</b>&nbsp;&nbsp;".htmlSpecChars($Return)."</span>";
    }
    $Signature=~s!\[\]![<span style='padding-left:2px;'>]</span>!g;
    $Signature=~s!operator=!operator<span style='padding-left:2px'>=</span>!g;
    $Signature=~s!(\[in-charge\]|\[not-in-charge\]|\[in-charge-deleting\]|\[static\])!<span style='color:Black;font-weight:normal;'>$1</span>!g;
    return $Signature;
}

sub get_Signature_Parts($$)
{
    my ($Signature, $Comma) = @_;
    my @Parts = ();
    my ($Bracket_Num, $Bracket2_Num) = (0, 0);
    my $Parameters = $Signature;
    my $ShortName = substr($Parameters, 0, get_signature_center($Parameters));
    $Parameters=~s/\A\Q$ShortName\E\(//g;
    $Parameters=~s/\)(| const)(| \[static\])\Z//g;
    my $Part_Num = 0;
    foreach my $Pos (0 .. length($Parameters) - 1)
    {
        my $Symbol = substr($Parameters, $Pos, 1);
        $Bracket_Num += 1 if($Symbol eq "(");
        $Bracket_Num -= 1 if($Symbol eq ")");
        $Bracket2_Num += 1 if($Symbol eq "<");
        $Bracket2_Num -= 1 if($Symbol eq ">");
        if($Symbol eq "," and $Bracket_Num==0 and $Bracket2_Num==0)
        {
            $Parts[$Part_Num] .= $Symbol if($Comma);
            $Part_Num += 1;
        }
        else
        {
            $Parts[$Part_Num] .= $Symbol;
        }
    }
    return @Parts;
}

sub isNotAnon($)
{
    return (not isAnon($_[0]));
}

sub isAnon($)
{
    return (($_[0]=~/\.\_\d+/) or ($_[0]=~/anon-/));
}

sub unmangled_Compact($)
# Removes all non-essential (for C++ language) whitespace from a string.  If 
# the whitespace is essential it will be replaced with exactly one ' ' 
# character. Works correctly only for unmangled names.
# If level > 1 is supplied, can relax its intent to compact the string.
{
    return $Cache{"unmangled_Compact"}{$_[0]} if(defined $Cache{"unmangled_Compact"}{$_[0]});
    my $result=$_[0];
    # First, we reduce all spaces that we can
    my $coms='[-()<>:*&~!|+=%@~"?.,/[^'."']";
    my $coms_nobr='[-()<:*&~!|+=%@~"?.,'."']";
    my $clos='[),;:\]]';
    $result=~s/^\s+//gm;
    $result=~s/\s+$//gm;
    $result=~s/((?!\n)\s)+/ /g;
    $result=~s/(\w+)\s+($coms+)/$1$2/gm;
    $result=~s/($coms+)\s+(\w+)/$1$2/gm;
    $result=~s/(\w)\s+($clos)/$1$2/gm;
    $result=~s/($coms+)\s+($coms+)/$1 $2/gm;
    $result=~s/($coms_nobr+)\s+($coms+)/$1$2/gm;
    $result=~s/($coms+)\s+($coms_nobr+)/$1$2/gm;
    # don't forget about >> and <:.  In unmangled names global-scope modifier 
    # is not used, so <: will always be a digraph and requires no special treatment.
    # We also try to remove other parts that are better to be removed here than in other places
    # double-cv
    $result=~s/\bconst\s+const\b/const/gm;
    $result=~s/\bvolatile\s+volatile\b/volatile/gm;
    $result=~s/\bconst\s+volatile\b\s+const\b/const volatile/gm;
    $result=~s/\bvolatile\s+const\b\s+volatile\b/const volatile/gm;
    # Place cv in proper order
    $result=~s/\bvolatile\s+const\b/const volatile/gm;
    $Cache{"unmangled_Compact"}{$_[0]} = $result;
    return $result;
}

sub unmangled_PostProcess($)
{
    my $result = $_[0];
    #s/\bunsigned int\b/unsigned/g;
    $result=~s/\bshort unsigned int\b/unsigned short/g;
    $result=~s/\bshort int\b/short/g;
    $result=~s/\blong long unsigned int\b/unsigned long long/g;
    $result=~s/\blong unsigned int\b/unsigned long/g;
    $result=~s/\blong long int\b/long long/g;
    $result=~s/\blong int\b/long/g;
    $result=~s/\)const\b/\) const/g;
    $result=~s/\blong long unsigned\b/unsigned long long/g;
    $result=~s/\blong unsigned\b/unsigned long/g;
    return $result;
}

sub correctName($)
{# type name correction
    my $CorrectName = $_[0];
    $CorrectName = unmangled_Compact($CorrectName);
    $CorrectName = unmangled_PostProcess($CorrectName);
    return $CorrectName;
}

sub get_namespace_additions($)
{
    my $NameSpaces = $_[0];
    my ($Additions, $AddNameSpaceId) = ("", 1);
    foreach my $NS (sort {$a=~/_/ <=> $b=~/_/} sort {lc($a) cmp lc($b)} keys(%{$NameSpaces}))
    {
        next if(not $NS or $NameSpaces->{$NS}==-1);
        next if($NS=~/(\A|::)iterator(::|\Z)/i);
        next if($NS=~/\A__/i);
        next if(($NS=~/\Astd::/ or $NS=~/\A(std|tr1|rel_ops|fcntl)\Z/) and not $STDCXX_TESTING);
        $NestedNameSpaces{$NS} = 1;# for future use in reports
        my ($TypeDecl_Prefix, $TypeDecl_Suffix) = ();
        my @NS_Parts = split(/::/, $NS);
        next if($#NS_Parts==-1);
        next if($NS_Parts[0]=~/\A(random|or)\Z/);
        foreach my $NS_Part (@NS_Parts)
        {
            $TypeDecl_Prefix .= "namespace $NS_Part\{";
            $TypeDecl_Suffix .= "}";
        }
        my $TypeDecl = $TypeDecl_Prefix."typedef int tmp_add_type_$AddNameSpaceId;".$TypeDecl_Suffix;
        my $FuncDecl = "$NS\:\:tmp_add_type_$AddNameSpaceId tmp_add_func_$AddNameSpaceId(){return 0;};";
        $Additions.="  $TypeDecl\n  $FuncDecl\n";
        $AddNameSpaceId+=1;
    }
    return $Additions;
}

sub forward_slash($)
{# forward slash to pass into MinGW GCC
    my $Path = $_[0];
    if($OSgroup eq "windows") {
        $Path=~s/\\/\//g;
    }
    return $Path;
}

sub inc_opt($$)
{
    my ($Path, $Style) = @_;
    if($Style eq "GCC")
    {# GCC options
        if($OSgroup eq "windows")
        {# to MinGW GCC
            return "-I\"".forward_slash($Path)."\"";
        }
        elsif($OSgroup eq "macos"
        and $Path=~/\.framework\Z/)
        {# to Apple's GCC
            return "-F".esc(get_dirname($Path));
        }
        else {
            return "-I".esc($Path);
        }
    }
    elsif($Style eq "CL") {
        return "/I \"$Path\"";
    }
    return "";
}

sub getPlatformOpts()
{
    my $Arch = getArch();
    my $Options = "";    
    if($OSgroup eq "windows")
    {# emulate the CL compiler
        if($Arch eq "x86") {
            $Options .= " -D_M_IX86=300";
        }
        elsif($Arch eq "x86-64") {
            $Options .= " -D_M_AMD64=300";
        }
        elsif($Arch eq "ia64") {
            $Options .= " -D_M_IA64=300";
        }
        $Options .= " -D_STDCALL_SUPPORTED";
        $Options .= " -D__int64=\"long long\"";
        $Options .= " -D__int32=long";
        $Options .= " -D__int16=short";
        $Options .= " -D__int8=char";
        $Options .= " -D__possibly_notnullterminated=\" \"";
        $Options .= " -D__nullterminated=\" \"";
        $Options .= " -D__nullnullterminated=\" \"";
        $Options .= " -D__w64=\" \" -D__ptr32=\" \" -D__ptr64=\" \"";
        $Options .= " -D__forceinline=inline -D__inline=inline";
        $Options .= " -D_MSC_VER=1500";
    }
    return $Options;
}

my %C_Structure = (
# FIXME: Can't separate union and struct data types before dumping,
# so it sometimes cause compilation errors for unknown reason
# when trying to declare TYPE* tmp_add_class_N
# This is a list of such structures + list of other C structures
    "sigval"=>1,
    "sigevent"=>1,
    "sigaction"=>1,
    "sigvec"=>1,
    "sigstack"=>1,
    "timeval"=>1,
    "timezone"=>1,
    "rusage"=>1,
    "rlimit"=>1,
    "wait"=>1,
    "flock"=>1,
    "stat"=>1,
    "stat64"=>1,
    "if_nameindex"=>1,
    "usb_device"=>1,
    "sigaltstack"=>1,
    "sysinfo"=>1,
# Other C structures appearing in every dump
    "timespec"=>1,
    "random_data"=>1,
    "drand48_data"=>1,
    "_IO_marker"=>1,
    "_IO_FILE"=>1,
    "lconv"=>1,
    "sched_param"=>1,
    "tm"=>1,
    "itimerspec"=>1,
    "_pthread_cleanup_buffer"=>1
);

sub getCompileCmd($$$)
{
    my ($Path, $Opt, $Inc) = @_;
    my $GccCall = $GCC_PATH;
    if($Opt) {
        $GccCall .= " ".$Opt;
    }
    $GccCall .= " -x ";
    if($OSgroup eq "macos") {
        $GccCall .= "objective-";
    }
    if(check_gcc_version($GCC_PATH, 4))
    { # compile as "C++" header
      # to obtain complete dump using GCC 4.0
        $GccCall .= "c++-header";
    }
    else
    { # compile as "C++" source
      # GCC 3.3 cannot compile headers
        $GccCall .= "c++";
    }
    $GccCall .= " ".esc($Path);
    if($CompilerOptions_Headers)
    {# user-defined options
        $GccCall .= " ".$CompilerOptions_Headers;
    }
    if($Inc)
    {# include paths
        $GccCall .= " ".$Inc;
    }
    if(my $PlatformOpts = getPlatformOpts()) {
        $GccCall .= " ".$PlatformOpts;
    }
    return $GccCall;
}

sub getDump()
{
    my $TmpHeaderPath = "$TMP_DIR/dump.h";
    my $MHeaderPath = $TmpHeaderPath;
    open(LIB_HEADER, ">$TmpHeaderPath") || die ("can't open file \'$TmpHeaderPath\': $!\n");
    if(my $AddDefines = $Descriptor{"Defines"})
    {
        $AddDefines=~s/\n\s+/\n  /g;
        print LIB_HEADER "\n  // add defines\n  ".$AddDefines."\n";
    }
    print LIB_HEADER "\n  // add includes\n";
    my @PreambleHeaders = keys(%Include_Preamble);
    @PreambleHeaders = sort {int($Include_Preamble{$a}{"Position"})<=>int($Include_Preamble{$b}{"Position"})} @PreambleHeaders;
    foreach my $Header_Path (@PreambleHeaders) {
        print LIB_HEADER "  #include \"".forward_slash($Header_Path)."\"\n";
    }
    my @Headers = keys(%Target_Headers);
    @Headers = sort {int($Target_Headers{$a}{"Position"})<=>int($Target_Headers{$b}{"Position"})} @Headers;
    foreach my $Header_Path (@Headers)
    {
        next if($Include_Preamble{$Header_Path});
        print LIB_HEADER "  #include \"".forward_slash($Header_Path)."\"\n";
    }
    close(LIB_HEADER);
    my $IncludeString = getIncString(getIncPaths(@PreambleHeaders, @Headers), "GCC");
    # preprocessing stage
    checkPreprocessedUnit(callPreprocessor($TmpHeaderPath, $IncludeString, "C++", "define\\ \\|undef\\ \\|#[ ]\\+[0-9]\\+ \".*\""));
    if($COMMON_LANGUAGE eq "C"
    and $C99Mode!=-1)
    { # rename "C++" keywords in "C" code
        my $PreprocessCmd = getCompileCmd($TmpHeaderPath, "-E", $IncludeString);
        my $MContent = `$PreprocessCmd 2>$TMP_DIR/null`;
        my $RegExp_C = join("|", keys(%CppKeywords_C));
        my $RegExp_F = join("|", keys(%CppKeywords_F));
        my $RegExp_O = join("|", keys(%CppKeywords_O));
        while($MContent=~s/(\A|\n[^\#\/\n][^\n]*?|\n)(\*\s*|\s+|\@|\,|\()($RegExp_C|$RegExp_F)(\s*(\,|\)|\;|\-\>|\.|\:\s*\d))/$1$2c99_$3$4/g)
        { # MATCH:
          # int foo(int new, int class, int (*new)(int));
          # unsigned private: 8;
          # DO NOT MATCH:
          # #pragma GCC visibility push(default)
            $C99Mode = 1;
        }
        if($MContent=~s/([^\w\s]|\w\s+)(?<!operator )(delete)(\s*(\())/$1c99_$2$3/g)
        { # MATCH:
          # int delete(...);
          # int explicit(...);
          # DO NOT MATCH:
          # void operator delete(...)
            $C99Mode = 1;
        }
        if($MContent=~s/(\s+)($RegExp_O)(\s*(\;|\:))/$1c99_$2$3/g)
        { # MATCH:
          # int bool;
          # DO NOT MATCH:
          # bool X;
          # return *this;
          # throw;
            $C99Mode = 1;
        }
        if($MContent=~s/(\s+)(operator)(\s*(\(\s*\)\s*[^\(\s]|\(\s*[^\)\s]))/$1c99_$2$3/g)
        { # MATCH:
          # int operator(...);
          # DO NOT MATCH:
          # int operator()(...);
            $C99Mode = 1;
        }
        if($MContent=~s/(\*\s*|\s+)(operator)(\s*(\,\s*[^\(\s]|\)))/$1c99_$2$3/g)
        { # MATCH:
          # int foo(int operator);
          # int foo(int operator, int other);
          # DO NOT MATCH:
          # int operator,(...);
            $C99Mode = 1;
        }
        if($MContent=~s/(\w)([^\w\(\,\s]\s*|\s+)(this)(\s*(\,|\)))/$1$2c99_$3$4/g)
        { # MATCH:
          # int foo(int* this);
          # int bar(int this);
          # DO NOT MATCH:
          # baz(X, this);
            $C99Mode = 1;
        }
        if($C99Mode==1)
        { # compile the corrected preprocessor output
            print "\nUsing C99 compatibility mode\n";
            $MHeaderPath = "$TMP_DIR/dump.i";
            writeFile($MHeaderPath, $MContent);
        }
    }
    if($COMMON_LANGUAGE eq "C++" or $CheckHeadersOnly)
    {# add classes and namespaces to the dump
        my $CHdump = "-fdump-class-hierarchy -c";
        if($C99Mode==1) {
            $CHdump .= " -fpreprocessed";
        }
        my $ClassHierarchyCmd = getCompileCmd($MHeaderPath, $CHdump, $IncludeString);
        chdir($TMP_DIR);
        system("$ClassHierarchyCmd >null 2>&1");
        chdir($ORIG_DIR);
        if(my $ClassDump = (cmd_find($TMP_DIR,"f","*.class",1))[0])
        {
            my %AddClasses = ();
            my $Content = readFile($ClassDump);
            foreach my $ClassInfo (split(/\n\n/, $Content))
            {
                if($ClassInfo=~/\AClass\s+(.+)\s*/i)
                {
                    my $CName = $1;
                    next if($CName=~/\A(__|_objc_|_opaque_)/);
                    $TUnit_NameSpaces{$CName} = -1;
                    if($CName=~/\A[\w:]+\Z/)
                    {# classes
                        $AddClasses{$CName} = 1;
                    }
                    if($CName=~/(\w[\w:]*)::/)
                    {# namespaces
                        my $NS = $1;
                        if(not defined $TUnit_NameSpaces{$NS}) {
                            $TUnit_NameSpaces{$NS} = 1;
                        }
                    }
                }
            }
            unlink($ClassDump);
            if(my $NS_Add = get_namespace_additions(\%TUnit_NameSpaces))
            { # GCC on all supported platforms does not include namespaces to the dump by default
                appendFile($TmpHeaderPath, "\n  // add namespaces\n".$NS_Add);
            }
            # some GCC versions don't include class methods to the TU dump by default
            my ($AddClass, $ClassNum) = ("", 0);
            foreach my $CName (sort keys(%AddClasses))
            {
                next if($C_Structure{$CName});
                next if(not $STDCXX_TESTING and $CName=~/\Astd::/);
                next if(($CName=~tr![:]!!)>2);
                if($CName=~/\A(.+)::[^:]+\Z/
                and $AddClasses{$1}) {
                    next;
                }
                $AddClass .= "  $CName* tmp_add_class_".($ClassNum++).";\n";
            }
            appendFile($TmpHeaderPath, "\n  // add classes\n".$AddClass);
        }
    }
    appendFile($LOG_PATH, "Temporary header file \'$TmpHeaderPath\' with the following content will be compiled to create GCC syntax tree:\n".readFile($TmpHeaderPath)."\n");
    # create TU dump
    my $TUdump = "-fdump-translation-unit -fkeep-inline-functions -c";
    if($C99Mode==1) {
        $TUdump .= " -fpreprocessed";
    }
    my $SyntaxTreeCmd = getCompileCmd($MHeaderPath, $TUdump, $IncludeString);
    appendFile($LOG_PATH, "The GCC parameters:\n  $SyntaxTreeCmd\n\n");
    chdir($TMP_DIR);
    system($SyntaxTreeCmd." >$TMP_DIR/tu_errors 2>&1");
    if($?)
    { # failed to compile, but the TU dump still can be created
        my $Errors = readFile("$TMP_DIR/tu_errors");
        if($Errors=~/c99_/)
        { # disable c99 mode
            $C99Mode=-1;
            print "\nDisabling C99 compatibility mode\n";
            unlink($LOG_PATH);
            return getDump();
        }
        # FIXME: handle other errors and try to recompile
        appendFile($LOG_PATH, $Errors);# new line
        print STDERR "\nERROR: some errors occurred, see error log for details:\n  $LOG_PATH\n\n";
        appendFile($LOG_PATH, "\n");# new line
    }
    chdir($ORIG_DIR);
    unlink($TmpHeaderPath, $MHeaderPath);
    return (cmd_find($TMP_DIR,"f","*.tu",1))[0];
}

sub checkPreprocessedUnit($)
{
    my $Path = $_[0];
    my $CurHeader = "";
    open(PREPROC, "$Path");
    while(<PREPROC>)
    {# detecting public and private constants
        chomp($_);
        if(/#[ \t]+\d+[ \t]+\"(.+)\"/) {
            $CurHeader=$1;
        }
        elsif(/\#[ \t]*define[ \t]+([_A-Z0-9a-z]+)[ \t]+(.*)[ \t]*\Z/)
        {
            my ($Name, $Value) = ($1, $2);
            if($Name=~/[a-z]/) {
                $AllDefines{$Name} = 1;
            }
            else
            {
                $Constants{$Name}{"Access"} = "public";
                $Constants{$Name}{"Value"} = $Value;
                $Constants{$Name}{"Header"} = $CurHeader;
                $Constants{$Name}{"HeaderName"} = get_filename($CurHeader);
            }
        }
        elsif(/\#[ \t]*undef[ \t]+([_A-Z]+)[ \t]*/) {
            my $Name = $1;
            $Constants{$Name}{"Access"} = "private";
        }
        elsif(/namespace\s+(\w[\w:]*)\s*(\{|\Z)/) {
            $TUnit_NameSpaces{$1} = 1;
        }
    }
    close(PREPROC);
    foreach my $Constant (keys(%Constants))
    {
        if($Constants{$Constant}{"Access"} eq "private"
        or not $Constants{$Constant}{"Value"} or $Constant=~/_h\Z/i) {
            delete($Constants{$Constant});
        }
        else {
            delete($Constants{$Constant}{"Access"});
        }
    }
}

sub parseHeaders()
{
    print "\rheader(s) analysis: [25.00%]";
    my $DumpPath = getDump();
    if(not $DumpPath) {
        print STDERR "\nERROR: can't create gcc syntax tree for header(s)\n\n";
        exit(1);
    }
    print "\rheader(s) analysis: [35.00%]";
    getInfo($DumpPath);
}

sub prepareInterfaces()
{
    foreach my $FuncInfoId (keys(%FuncDescr))
    {
        my $MnglName = $FuncDescr{$FuncInfoId}{"MnglName"};
        next if(not $MnglName);
        next if($MnglName=~/\A(_Z|\?)/ and $tr_name{$MnglName}=~/\.\_\d+/);
        if($FuncDescr{$FuncInfoId}{"Data"})
        {
            if($MnglName=~/\A(_Z|\?)/) {
                $GlobVarNames{$tr_name{$MnglName}} = 1;
            }
            else {
                 $GlobVarNames{$FuncDescr{$FuncInfoId}{"ShortName"}} = 1;
            }
        }
        else
        {
            if($MnglName=~/\A(_Z|\?)/) {
                $MethodNames{$FuncDescr{$FuncInfoId}{"ShortName"}} = 1;
            }
            else {
                $FuncNames{$FuncDescr{$FuncInfoId}{"ShortName"}} = 1;
            }
        }
        %{$CompleteSignature{$MnglName}} = %{$FuncDescr{$FuncInfoId}};
        delete($FuncDescr{$FuncInfoId});
    }
    if($CheckHeadersOnly or $COMMON_LANGUAGE eq "C++"
    or $OSgroup eq "windows") {
        translateSymbols(keys(%CompleteSignature));
    }
    %FuncDescr=();
    if($OSgroup eq "windows")
    {# translate symbols from CL mangling to GCC
        foreach my $MnglName (keys(%CompleteSignature))
        {
            if($MnglName=~/\A_Z/)
            {
                if(my $TargetMangled = $mangled_name{synchTr($tr_name{$MnglName})}) {
                    %{$CompleteSignature{$TargetMangled}} = %{$CompleteSignature{$MnglName}};
                    delete($CompleteSignature{$MnglName});
                }
                else {
                    # pure virtual methods go here
                }
            }
        }
    }
}

sub setRegularities()
{
    foreach my $Interface (keys(%CompleteSignature))
    {
        delete($CompleteSignature{$Interface}{"MnglName"});
        setOutParams_Simple($Interface);
        setOutParams_Complex($Interface);
        setRelationships($Interface);
    }
}

sub setRelationships($)
{
    my $Interface = $_[0];
    my $ShortName = $CompleteSignature{$Interface}{"ShortName"};
    if($Interface=~/\A(_Z|\?)/ and not $CompleteSignature{$Interface}{"Class"}) {
        $Func_ShortName_MangledName{$CompleteSignature{$Interface}{"ShortName"}}{$Interface}=1;
    }
    if(not $CompleteSignature{$Interface}{"PureVirt"})
    {
        if($CompleteSignature{$Interface}{"Constructor"}) {
            $Class_Constructors{$CompleteSignature{$Interface}{"Class"}}{$Interface} = 1;
        }
        elsif($CompleteSignature{$Interface}{"Destructor"}) {
            $Class_Destructors{$CompleteSignature{$Interface}{"Class"}}{$Interface} = 1;
        }
        else
        {
            if(get_TypeName($CompleteSignature{$Interface}{"Return"}) ne "void")
            {
                my $DoNotUseReturn = 0;
                if(is_transit_function($ShortName))
                {
                    my $Return_FId = get_FoundationTypeId($CompleteSignature{$Interface}{"Return"});
                    foreach my $Pos (keys(%{$CompleteSignature{$Interface}{"Param"}}))
                    {
                        next if($InterfaceSpecType{$Interface}{"SpecParam"}{$Pos});
                        my $Param_FId = get_FoundationTypeId($CompleteSignature{$Interface}{"Param"}{$Pos}{"type"});
                        if(($CompleteSignature{$Interface}{"Param"}{$Pos}{"type"} eq $CompleteSignature{$Interface}{"Return"})
                        or (get_TypeType($Return_FId)!~/\A(Intrinsic|Enum|Array)\Z/ and $Return_FId eq $Param_FId)) {
                            $DoNotUseReturn = 1;
                            last;
                        }
                    }
                }
                if(not $DoNotUseReturn)
                {
                    $ReturnTypeId_Interface{$CompleteSignature{$Interface}{"Return"}}{$Interface}=1;
                    my $Return_FId = get_FoundationTypeId($CompleteSignature{$Interface}{"Return"});
                    my $PLevel = get_PointerLevel($Tid_TDid{$CompleteSignature{$Interface}{"Return"}}, $CompleteSignature{$Interface}{"Return"});
                    if(get_TypeType($Return_FId) ne "Intrinsic") {
                        $BaseType_PLevel_Return{$Return_FId}{$PLevel}{$Interface}=1;
                    }
                }
            }
        }
    }
    if($CompleteSignature{$Interface}{"Class"} and not $CompleteSignature{$Interface}{"Destructor"}
    and ($Interface!~/C2E/ or not $CompleteSignature{$Interface}{"Constructor"})) {
        $Interface_Overloads{$CompleteSignature{$Interface}{"NameSpace"}}{getTreeTypeName($CompleteSignature{$Interface}{"Class"})}{$ShortName}{$Interface} = 1;
    }
    if($CompleteSignature{$Interface}{"Type"} eq "Method"
    and not $CompleteSignature{$Interface}{"PureVirt"}) {
        $Class_Method{$CompleteSignature{$Interface}{"Class"}}{$Interface} = 1;
    }
    $Header_Interface{$CompleteSignature{$Interface}{"Header"}}{$Interface} = 1;
    if(not $CompleteSignature{$Interface}{"Class"} and not $LibraryMallocFunc
    and $Interface_Library{$Interface} and $Interface ne "malloc"
    and $ShortName!~/try|slice|trim|\d\Z/i and $ShortName=~/(\A|_|\d)(malloc|alloc)(\Z|_|\d)/i
    and keys(%{$CompleteSignature{$Interface}{"Param"}})==1
    and isIntegerType(get_TypeName($CompleteSignature{$Interface}{"Param"}{0}{"type"}))) {
        $LibraryMallocFunc = $Interface;
    }
    if(not $CompleteSignature{$Interface}{"Class"} and $Interface_Library{$Interface}
    and $ShortName=~/(\A[a-z]*_)(init|initialize|initializer|install)\Z/i) {
        $LibraryInitFunc{$Interface} = 1;
    }
    elsif(not $CompleteSignature{$Interface}{"Class"} and $Interface_Library{$Interface}
    and $ShortName=~/\A([a-z]*_)(exit|finalize|finish|clean|close|deinit|shutdown|cleanup|uninstall|end)\Z/i) {
        $LibraryExitFunc{$Interface} = 1;
    }
}

sub setOutParams_Simple($)
{
    my $Interface = $_[0];
    my $ReturnType_Id = $CompleteSignature{$Interface}{"Return"};
    my $ReturnType_Name_Short = get_TypeName($ReturnType_Id);
    while($ReturnType_Name_Short=~s/(\*|\&)([^<>()]+|)\Z/$2/g){};
    my ($ParamName_Prev, $ParamTypeId_Prev) = ();
    foreach my $ParamPos (sort {int($a)<=>int($b)} keys(%{$CompleteSignature{$Interface}{"Param"}}))
    {# detecting out-parameters by name
        if($CompleteSignature{$Interface}{"Param"}{$ParamPos}{"name"}=~/\Ap\d+\Z/
        and (my $NewParamName = $AddIntParams{$Interface}{$ParamPos}))
        {# names from the external file
            $CompleteSignature{$Interface}{"Param"}{$ParamPos}{"name"} = $NewParamName;
        }
        my $ParamName = $CompleteSignature{$Interface}{"Param"}{$ParamPos}{"name"};
        my $ParamTypeId = $CompleteSignature{$Interface}{"Param"}{$ParamPos}{"type"};
        my $ParamPLevel = get_PointerLevel($Tid_TDid{$ParamTypeId}, $ParamTypeId);
        my $ParamFTypeId = get_FoundationTypeId($ParamTypeId);
        my $ParamFTypeName = get_TypeName($ParamFTypeId);
        my $ParamTypeName = get_TypeName($ParamTypeId);
        
        if($UserDefinedOutParam{$Interface}{$ParamPos+1}
        or $UserDefinedOutParam{$Interface}{$ParamName})
        {# user defined by <out_params> section in the descriptor
            register_out_param($Interface, $ParamPos, $ParamName, $ParamTypeId);
            next;
        }
        
        # particular accept
        if($ParamPLevel>=2 and isCharType($ParamFTypeName)
        and not is_const_type($ParamTypeName) and $ParamName!~/argv/i and $ParamName!~/\A(s|str|string)\Z/i)
        {# soup_form_decode_multipart ( SoupMessage* msg, char const* file_control_name, char** filename, char** content_type, SoupBuffer** file )
         # direct_trim ( char** s )
            register_out_param($Interface, $ParamPos, $ParamName, $ParamTypeId);
            next;
        }
        if($ParamPLevel>=2 and not is_const_type($ParamTypeName) and $ParamName=~/handle/i and $CompleteSignature{$Interface}{"ShortName"}=~/_init\Z/i)
        {# gnutls_cipher_init ( gnutls_cipher_hd_t* handle, gnutls_cipher_algorithm_t cipher, gnutls_datum_t const* key, gnutls_datum_t const* iv )
            register_out_param($Interface, $ParamPos, $ParamName, $ParamTypeId);
            next;
        }
        if($ParamPLevel==1 and isNumericType($ParamFTypeName)
        and not is_const_type($ParamTypeName) and ($ParamName=~/((\A|_)(x|y|(lat|long|alt)itude)(\Z|_))|errnum|errcode|used|horizontal|vertical|width|height|error|length|count|time|status|state|min|max|weight|\An[_]*(row|col|axe|found|memb|key|space)|\An_/i or $ParamTypeName=~/bool/i
        or $ParamName=~/(\A|_)n(_|)(elem|item)/i or is_out_word($ParamName) or $ParamName=~/\Ais/i))
        {# gail_misc_get_origins ( GtkWidget* widget, gint* x_window, gint* y_window, gint* x_toplevel, gint* y_toplevel )
         # glXGetFBConfigs ( Display* dpy, int screen, int* nelements )
            register_out_param($Interface, $ParamPos, $ParamName, $ParamTypeId);
            next;
        }
        if(($ParamName=~/err/i and $ParamPLevel>=2 and $ParamTypeName=~/err/i)
        or ($ParamName=~/\A(error|err)(_|)(p|ptr)\Z/i and $ParamPLevel>=1))
        {# g_app_info_add_supports_type ( GAppInfo* appinfo, char const* content_type, GError** error )
         # rsvg_handle_new_from_data ( guint8 const* data, gsize data_len, GError** error )
            register_out_param($Interface, $ParamPos, $ParamName, $ParamTypeId);
            next;
        }
        
        # strong reject
        next if(get_TypeType(get_FoundationTypeId($ReturnType_Id))!~/\A(Intrinsic|Enum)\Z/
        or $CompleteSignature{$Interface}{"ShortName"}=~/\Q$ReturnType_Name_Short\E/
        or $CompleteSignature{$Interface}{"ShortName"}=~/$ParamName(_|)get(_|)\w+/i
        or $ReturnType_Name_Short=~/pointer|ptr/i);
        next if($ParamPLevel<=0);
        next if($ParamPLevel==1 and (isOpaque($ParamFTypeId)
        or get_TypeName($ParamFTypeId)=~/\A(((struct |)(_IO_FILE|__FILE|FILE))|void)\Z/));
        next if(is_const_type($ParamTypeName) and $ParamPLevel<=2);
        next if($CompleteSignature{$Interface}{"ShortName"}=~/memcpy|already/i);

        # allowed
        if((is_out_word($ParamName) and $CompleteSignature{$Interface}{"ShortName"}!~/free/i
        #! xmlC14NDocSaveTo (xmlDocPtr doc, xmlNodeSetPtr nodes, int exclusive, xmlChar** inclusive_ns_prefixes, int with_comments, xmlOutputBufferPtr buf)
        # XGetMotionEvents (Display* display, Window w, Time start, Time stop, int* nevents_return)
        
        and ($ParamTypeName=~/\*/ or $ParamTypeName!~/(ptr|pointer|p\Z)/i)
        
        # gsl_sf_bessel_il_scaled_array (int const lmax, double const x, double* result_array)
        and not grep(/\A(array)\Z/i, @{get_tokens($ParamName)})
        
        #! mysql_free_result ( MYSQL_RES* result )
        and not is_out_word($ParamTypeName))
        
        # snd_card_get_name (int card, char** name)
        # FMOD_Channel_GetMode (FMOD_CHANNEL* channel, FMOD_MODE* mode)
        or $CompleteSignature{$Interface}{"ShortName"}=~/(get|create)[_]*[0-9a-z]*$ParamName\Z/i
        
        # snd_config_get_ascii (snd_config_t const* config, char** value)
        or ($ParamPos==1 and $ParamName=~/value/i and $CompleteSignature{$Interface}{"ShortName"}=~/$ParamName_Prev[_]*get/i)
        
        # poptDupArgv (int argc, char const** argv, int* argcPtr, char const*** argvPtr)
        or ($ParamName=~/ptr|pointer|(p\Z)/i and $ParamPLevel>=3))
        {
            my $IsTransit = 0;
            foreach my $Pos (keys(%{$CompleteSignature{$Interface}{"Param"}}))
            {
                my $OtherParamTypeId = $CompleteSignature{$Interface}{"Param"}{$Pos}{"type"};
                my $OtherParamName = $CompleteSignature{$Interface}{"Param"}{$Pos}{"name"};
                next if($OtherParamName eq $ParamName);
                my $OtherParamFTypeId = get_FoundationTypeId($OtherParamTypeId);
                if($ParamFTypeId eq $OtherParamFTypeId) {
                    $IsTransit = 1;
                    last;
                }
            }
            if($IsTransit or get_TypeType($ParamFTypeId)=~/\A(Intrinsic|Enum|Array)\Z/) {
                $OutParamInterface_Pos_NoUsing{$Interface}{$ParamPos}=1;
                $Interface_OutParam_NoUsing{$Interface}{$ParamName}=1;
            }
            else {
                register_out_param($Interface, $ParamPos, $ParamName, $ParamTypeId);
            }
        }
        ($ParamName_Prev, $ParamTypeId_Prev) = ($ParamName, $ParamTypeId);
    }
}

sub setOutParams_Complex($)
{# detect out-parameters by function name and parameter type
    my $Interface = $_[0];
    my $Func_ShortName = $CompleteSignature{$Interface}{"ShortName"};
    my $ReturnType_Id = $CompleteSignature{$Interface}{"Return"};
    my $ReturnType_Name_Short = get_TypeName($ReturnType_Id);
    while($ReturnType_Name_Short=~s/(\*|\&)([^<>()]+|)\Z/$2/g){};
    return if(get_TypeType(get_FoundationTypeId($ReturnType_Id))!~/\A(Intrinsic|Enum)\Z/
    or $Func_ShortName=~/\Q$ReturnType_Name_Short\E/);
    if(get_TypeName($ReturnType_Id) eq "void*" and $Func_ShortName=~/data/i) {
        # void* repo_sidedata_create ( Repo* repo, size_t size )
        return;
    }
    return if($Func_ShortName!~/(new|create|open|top|update|start)/i and not is_alloc_func($Func_ShortName)
    and ($Func_ShortName!~/init/i or get_TypeName($ReturnType_Id) ne "void") and not $UserDefinedOutParam{$Interface});
    return if($Func_ShortName=~/obsolete|createdup|updates/i);
    return if(not keys(%{$CompleteSignature{$Interface}{"Param"}}));
    return if($Func_ShortName=~/(already)/i);
    if(not detect_out_parameters($Interface, 1)) {
        detect_out_parameters($Interface, 0);
    }
}

sub synchTr($)
{
    my $Translation = $_[0];
    if($OSgroup eq "windows") {
        $Translation=~s/\(\)(|\s*const)\Z/\(void\)$1/g;
    }
    return $Translation;
}

sub detect_out_parameters($$)
{
    my ($Interface, $Strong) = @_;
    foreach my $ParamPos (sort{int($a)<=>int($b)} keys(%{$CompleteSignature{$Interface}{"Param"}}))
    {
        my $ParamTypeId = $CompleteSignature{$Interface}{"Param"}{$ParamPos}{"type"};
        my $ParamName = $CompleteSignature{$Interface}{"Param"}{$ParamPos}{"name"};
        if(isOutParam($ParamTypeId, $ParamPos, $Interface, $Strong)) {
            register_out_param($Interface, $ParamPos, $ParamName, $ParamTypeId);
            return 1;
        }
    }
    return 0;
}

sub get_outparam_candidate($$)
{
    my ($Interface, $Right) = @_;
    my $Func_ShortName = $CompleteSignature{$Interface}{"ShortName"};
    if($Right) {
        if($Func_ShortName=~/([a-z0-9]+)(_|)(new|open|init)\Z/i) {
            return $1;
        }
    }
    else {
        if($Func_ShortName=~/(new|open|init)(_|)([a-z0-9]+)/i) {
            return $3;
        }
    }
}

sub isOutParam($$$$)
{
    my ($Param_TypeId, $ParamPos, $Interface, $Strong) = @_;
    my $Param_Name = $CompleteSignature{$Interface}{"Param"}{$ParamPos}{"name"};
    my $PLevel = get_PointerLevel($Tid_TDid{$Param_TypeId}, $Param_TypeId);
    my $TypeName = get_TypeName($Param_TypeId);
    my $Param_FTypeId = get_FoundationTypeId($Param_TypeId);
    my $Param_FTypeName = get_TypeName($Param_FTypeId);
    $Param_FTypeName=~s/\A(struct|union) //g;
    my $Param_FTypeType = get_TypeType($Param_FTypeId);
    return 0 if($PLevel<=0);
    return 0 if($PLevel==1 and isOpaque($Param_FTypeId));
    return 0 if($Param_FTypeType!~/\A(Struct|Union|Class)\Z/);
    return 0 if(keys(%{$BaseType_PLevel_Return{$Param_FTypeId}{$PLevel}}));
    return 0 if(keys(%{$ReturnTypeId_Interface{$Param_TypeId}}));
    return 0 if(is_const_type($TypeName));
    my $Func_ShortName = $CompleteSignature{$Interface}{"ShortName"};
    return 1 if($Func_ShortName=~/\A\Q$Param_FTypeName\E(_|)init/);
    if($Strong) {
        if(my $Candidate = get_outparam_candidate($Interface, 1)) {
            return ($Param_Name=~/\Q$Candidate\E/i);
        }
    }
    if(my $Candidate = get_outparam_candidate($Interface, 0)) {
        return 0 if($Param_Name!~/\Q$Candidate\E/i);
    }
    return 1 if(($Func_ShortName=~/(new|create|open|start)/i and $Func_ShortName!~/get|restart|set|test/)
    or is_alloc_func($Func_ShortName));
    return 1 if($Func_ShortName=~/top/i and $PLevel==2);
    # snd_config_top
    return 1 if($UserDefinedOutParam{$Interface}{$Param_Name}
    or $UserDefinedOutParam{$Interface}{$ParamPos+1});
    return 1 if($Func_ShortName=~/update/i and $Func_ShortName!~/add|append/i
    and $Func_ShortName=~/$Param_Name/i and $PLevel>=1);
    if($Func_ShortName=~/init/i) {
        if(keys(%{$CompleteSignature{$Interface}{"Param"}})==1
        or number_of_simple_params($Interface)==keys(%{$CompleteSignature{$Interface}{"Param"}})-1) {
            return 1;
        }
    }
    
    return 0;
}

sub number_of_simple_params($)
{
    my $Interface = $_[0];
    return 0 if(not $Interface);
    my $Count = 0;
    foreach my $Pos (keys(%{$CompleteSignature{$Interface}{"Param"}}))
    {
        my $TypeId = $CompleteSignature{$Interface}{"Param"}{$Pos}{"type"};
        my $PName = $CompleteSignature{$Interface}{"Param"}{$Pos}{"name"};
        if(get_TypeType($TypeId)=~/\A(Intrinsic|Enum)\Z/
        or isString($TypeId, $PName, $Interface)) {
            $Count+=1;
        }
    }
    return $Count;
}

sub get_OutParamFamily($$)
{
    my ($TypeId, $IncludeOuter) = @_;
    my $FTypeId = get_FoundationTypeId($TypeId);
    if(get_TypeType($FTypeId)=~/Struct|Union|Class/)
    {
        my @Types = ($IncludeOuter and ($TypeId ne $FTypeId))?($TypeId, $FTypeId):($FTypeId);
        while(my $ReducedTypeId = reduce_pointer_level($TypeId))
        {
            push(@Types, $ReducedTypeId);
            $TypeId = $ReducedTypeId;
        }
        return @Types;
    }
    else
    {
        my @Types = ($IncludeOuter)?($TypeId):();
        my $ReducedTypeId = reduce_pointer_level($TypeId);
        if(get_TypeType($ReducedTypeId) eq "Typedef") {
            push(@Types, $ReducedTypeId);
        }
        return @Types;
    }
    return ();
}

sub is_alloc_func($)
{
    my $FuncName = $_[0];
    return ($FuncName=~/alloc/i and $FuncName!~/dealloc|realloc/i);
}

sub markAbstractClasses()
{
    foreach my $Interface (keys(%CompleteSignature)) {
        if($CompleteSignature{$Interface}{"PureVirt"}) {
            markAbstractSubClasses($CompleteSignature{$Interface}{"Class"}, $Interface);
        }
    }
}

sub markAbstractSubClasses($$)
{
    my ($ClassId, $Interface) = @_;
    return if(not $ClassId or not $Interface);
    my $TargetSuffix = get_method_suffix($Interface);
    foreach my $InterfaceCandidate (keys(%{$Class_Method{$ClassId}}))
    {
        if($TargetSuffix eq get_method_suffix($InterfaceCandidate))
        {
            if($CompleteSignature{$Interface}{"Constructor"}) {
                return if($CompleteSignature{$InterfaceCandidate}{"Constructor"});
            }
            elsif($CompleteSignature{$Interface}{"Destructor"}) {
                return if($CompleteSignature{$InterfaceCandidate}{"Destructor"});
            }
            else {
                return if($CompleteSignature{$Interface}{"ShortName"} eq $CompleteSignature{$InterfaceCandidate}{"ShortName"});
            }
        }
    }
    $Class_PureVirtFunc{get_TypeName($ClassId)}{$Interface} = 1;
    foreach my $SubClass_Id (keys(%{$Class_SubClasses{$ClassId}})) {
        markAbstractSubClasses($SubClass_Id, $Interface);
    }
}

sub get_method_suffix($)
{
    my $Interface = $_[0];
    my $Signature = $tr_name{$Interface};
    my $Suffix = substr($Signature, get_signature_center($Signature));
    return $Suffix;
}

sub cleanName($)
{
    my $Name = $_[0];
    return "" if(not $Name);
    foreach my $Token (sort keys(%Operator_Indication)) {
        my $Token_Translate = $Operator_Indication{$Token};
        $Name=~s/\Q$Token_Translate\E/\_$Token\_/g;
    }
    $Name=~s/\,/_/g;
    $Name=~s/\./_p_/g;
    $Name=~s/\:/_/g;
    $Name=~s/\]/_rb_/g;
    $Name=~s/\[/_lb_/g;
    $Name=~s/\(/_/g;
    $Name=~s/\)/_/g;
    $Name=~s/ /_/g;
    while($Name=~/__/) {
        $Name=~s/__/_/g;
    }
    $Name=~s/\_\Z//;
    return $Name;
}

sub getTestName($)
{
    my $Interface = $_[0];
    $Interface=~s/\?//g;
    return $Interface;
}

sub getTestPath($)
{
    my $Interface = $_[0];
    my $TestPath = "";
    if($Interface_LibGroup{$Interface}) {
        $TestPath = $TEST_SUITE_PATH."/groups/".clean_libgroup_name($Interface_LibGroup{$Interface})."/".getTestName($Interface);
    }
    else
    {
        my $ClassName = get_TypeName($CompleteSignature{$Interface}{"Class"});
        if($OSgroup eq "windows") {
            $ClassName = cleanName($ClassName);
        }
        my $Header = $CompleteSignature{$Interface}{"Header"};
        $Header=~s/(\.\w+)\Z//g;
        $TestPath = $TEST_SUITE_PATH."/groups/".get_filename($Header)."/".(($ClassName)?"classes/".get_type_short_name($ClassName):"functions")."/".getTestName($Interface);
    }
    return $TestPath;
}

sub getLibGroupPath($$$)
{
    my ($C1, $C2, $TwoComponets) = @_;
    return () if(not $C1);
    $C1=clean_libgroup_name($C1);
    if($TwoComponets) {
        if($C2) {
            return ($TEST_SUITE_PATH."/$TargetLibraryName-t2c/", $C1, $C2);
        }
        else {
            return ($TEST_SUITE_PATH."/$TargetLibraryName-t2c/", $C1, "functions");
        }
    }
    else {
        return ($TEST_SUITE_PATH."/$TargetLibraryName-t2c/", "", $C1);
    }
}

sub getLibGroupName($$)
{
    my ($C1, $C2) = @_;
    return "" if(not $C1);
    if($C2) {
        return $C2;
    }
    else {
        return $C1;
    }
}

sub clean_libgroup_name($)
{
    my $LibGroup = $_[0];
    $LibGroup=~s/(\.\w+)\Z//g;
    $LibGroup=~s/( |-)/_/g;
    $LibGroup=~s/\([^()]+\)//g;
    $LibGroup=~s/[_]{2,}/_/g;
    return $LibGroup;
}

sub get_signature_center($)
{
    my $Signature = $_[0];
    my ($Bracket_Num, $Bracket2_Num) = (0, 0);
    my $Center = 0;
    if($Signature=~s/(operator([<>\-\=\*]+|\(\)))//g)
    {# operators: (),->,->*,<,<=,<<,<<=,>,>=,>>,>>=
        $Center+=length($1);
    }
    foreach my $Pos (0 .. length($Signature)-1)
    {
        my $Symbol = substr($Signature, $Pos, 1);
        $Bracket_Num += 1 if($Symbol eq "(");
        $Bracket_Num -= 1 if($Symbol eq ")");
        $Bracket2_Num += 1 if($Symbol eq "<");
        $Bracket2_Num -= 1 if($Symbol eq ">");
        if($Symbol eq "(" and
        $Bracket_Num==1 and $Bracket2_Num==0) {
            return $Center;
        }
        $Center+=1;
    }
    return 0;
}

sub interfaceFilter($)
{
    my $Interface = $_[0];
    return 0 if($SkipInterfaces{$Interface});
    foreach my $SkipPattern (keys(%SkipInterfaces_Pattern)) {
        return 0 if($Interface=~/$SkipPattern/);
    }
    return 0 if($Interface=~/\A_tg_inln_tmpl_\d+/);# templates
    return 0 if(not $CompleteSignature{$Interface}{"Header"});
    return 0 if($CompleteSignature{$Interface}{"Data"});
    if(not $STDCXX_TESTING and not $CheckStdCxx and not $TargetInterfaceName)
    {# stdc++ symbols
        return 0 if($Interface=~/\A(_ZS|_ZNS|_ZNKS)/);
    }
    if($Interface=~/\A_ZN9__gnu_cxx\d/) {
        return 0;
    }
    if($CompleteSignature{$Interface}{"Constructor"}) {
        my $ClassId = $CompleteSignature{$Interface}{"Class"};
        return ( not ($Interface=~/C1E/ and ($CompleteSignature{$Interface}{"Protected"} or isAbstractClass($ClassId))) );
    }
    elsif($CompleteSignature{$Interface}{"Destructor"}) {
        my $ClassId = $CompleteSignature{$Interface}{"Class"};
        return ( not ($Interface=~/D0E|D1E/ and ($CompleteSignature{$Interface}{"Protected"} or isAbstractClass($ClassId))) );
    }
    else {
        return 1;
    }
}

sub addHeaders($$)
{
    my ($NewHeaders, $OldHeaders) = @_;
    $OldHeaders = [] if(not $OldHeaders);
    $NewHeaders = [] if(not $NewHeaders);
    my (%Old, @Result_Before, @Result_After) = ();
    foreach (@{$OldHeaders}) {
        $Old{$_} = 1;
        @Result_After = (@Result_After, $_) if($_);
    }
    foreach my $NewHeader (@{$NewHeaders})
    {
        if(not $Old{$NewHeader} and $NewHeader) {
            @Result_Before = (@Result_Before, $NewHeader);
        }
    }
    my @Result = (@Result_Before, @Result_After);
    return \@Result;
}

sub getTypeHeaders($)
{
    my $TypeId = $_[0];
    return [] if(not $TypeId);
    my %Type = delete_quals($Tid_TDid{$TypeId}, $TypeId);
    my $Headers = [$TypeDescr{$Type{"TDid"}}{$Type{"Tid"}}{"Header"}];
    $Headers = addHeaders(tmplInstHeaders($Type{"Tid"}), $Headers);
    $Headers = addHeaders([$TemplateHeader{$Type{"ShortName"}}], $Headers);
    if($Type{"NameSpaceClassId"}) {
        $Headers = addHeaders(getTypeHeaders($Type{"NameSpaceClassId"}), $Headers);
    }
    return $Headers;
}

sub get_TypeName($)
{
    my $TypeId = $_[0];
    return $TypeDescr{$Tid_TDid{$TypeId}}{$TypeId}{"Name"};
}

sub get_TypeType($)
{
    my $TypeId = $_[0];
    return $TypeDescr{$Tid_TDid{$TypeId}}{$TypeId}{"Type"};
}

sub get_TypeSize($)
{
    my $TypeId = $_[0];
    return $TypeDescr{$Tid_TDid{$TypeId}}{$TypeId}{"Size"};
}

sub get_TypeHeader($)
{
    my $TypeId = $_[0];
    return $TypeDescr{$Tid_TDid{$TypeId}}{$TypeId}{"Header"};
}

sub isNotInCharge($)
{
    my $Interface = $_[0];
    return ($CompleteSignature{$Interface}{"Constructor"}
    and $Interface=~/C2E/);
}

sub isInCharge($)
{
    my $Interface = $_[0];
    return ($CompleteSignature{$Interface}{"Constructor"}
    and $Interface=~/C1E/);
}

sub replace_c2c1($)
{
    my $Interface = $_[0];
    if($CompleteSignature{$Interface}{"Constructor"}) {
        $Interface=~s/C2E/C1E/;
    }
    return $Interface;
}

sub getSubClassName($)
{
    my $ClassName = $_[0];
    return getSubClassBaseName($ClassName)."_SubClass";
}

sub getSubClassBaseName($)
{
    my $ClassName = $_[0];
    $ClassName=~s/\:\:|<|>|\(|\)|\[|\]|\ |,|\*/_/g;
    $ClassName=~s/[_][_]+/_/g;
    return $ClassName;
}

sub getNumOfParams($)
{
    my $Interface = $_[0];
    my @Params = keys(%{$CompleteSignature{$Interface}{"Param"}});
    return ($#Params + 1);
}

sub sort_byCriteria($$)
{
    my ($Interfaces, $Criteria) = @_;
    my (@NewInterfaces1, @NewInterfaces2) = ();
    foreach my $Interface (@{$Interfaces})
    {
        if(compare_byCriteria($Interface, $Criteria)) {
            push(@NewInterfaces1, $Interface);
        }
        else {
            push(@NewInterfaces2, $Interface);
        }
    }
    @{$Interfaces} = (@NewInterfaces1, @NewInterfaces2);
}

sub get_int_prefix($)
{
    my $Interface = $_[0];
    if($Interface=~/\A([a-z0-9]+)_[a-z0-9]/i) {
        return $1;
    }
    else {
        return "";
    }
}

sub sort_byLibrary($$)
{
    my ($Interfaces, $Library) = @_;
    return if(not $Library);
    my $LibPrefix = $SoLib_IntPrefix{$Library};
    my (@NewInterfaces1, @NewInterfaces2, @NewInterfaces3) = ();
    foreach my $Interface (@{$Interfaces})
    {
        my $IntPrefix = get_int_prefix($Interface);
        if(get_filename($Interface_Library{$Interface}) eq $Library
        or get_filename($NeededInterface_Library{$Interface}) eq $Library) {
            push(@NewInterfaces1, $Interface);
        }
        elsif(not $Interface_Library{$Interface}
        and not $NeededInterface_Library{$Interface}) {
            push(@NewInterfaces1, $Interface);
        }
        elsif($Interface=~/environment/i)
        {# functions to set evironment should NOT be sorted
            push(@NewInterfaces1, $Interface);
        }
        elsif($LibPrefix and ($LibPrefix eq $IntPrefix)) {
            push(@NewInterfaces2, $Interface);
        }
        else {
            push(@NewInterfaces3, $Interface);
        }
    }
    @{$Interfaces} = (@NewInterfaces1, @NewInterfaces2, @NewInterfaces3);
}

sub get_tokens($)
{
    my $Word = $_[0];
    return $Cache{"get_tokens"}{$Word} if(defined $Cache{"get_tokens"}{$Word});
    my @Tokens = ();
    if($Word=~/\s+|[_]+/)
    {
        foreach my $Elem (split(/[_:\s]+/, $Word))
        {
            foreach my $SubElem (@{get_tokens($Elem)}) {
                push(@Tokens, $SubElem);
            }
        }
    }
    else
    {
        my $WCopy = $Word;
        while($WCopy=~s/([A-Z]*[a-z]+|\d+)//) {
            push(@Tokens, lc($1));
        }
        $WCopy=$Word;
        while($WCopy=~s/([A-Z]{2,})//) {
            push(@Tokens, lc($1));
        }
        $WCopy=$Word;
        while($WCopy=~s/([A-Z][a-z]+)([A-Z]|\Z)/$2/) {
            push(@Tokens, lc($1));
        }
    }
    @Tokens = unique_array(@Tokens);
    $Cache{"get_tokens"}{$Word} = \@Tokens;
    return \@Tokens;
}

sub unique_array(@)
{
    my %seen = ();
    my @uniq = ();
    foreach my $item (@_) {
        unless ($seen{$item}) {
            # if we get here, we have not seen it before
            $seen{$item} = 1;
            push(@uniq, $item);
        }
    }
    return @uniq;
}

sub sort_byName($$$)
{
    my ($Words, $KeyWords, $Type) = @_;
    my %Word_Coincidence = ();
    foreach my $Word (@{$Words})
    {
        my $TargetWord = $Word;
        if($Word=~/\A(_Z|\?)/) {
            $TargetWord = $CompleteSignature{$Word}{"ShortName"}." ".get_TypeName($CompleteSignature{$Word}{"Class"});
        }
        $Word_Coincidence{$Word} = get_word_coinsidence($TargetWord, $KeyWords);
    }
    @{$Words} = sort {$Word_Coincidence{$b} <=> $Word_Coincidence{$a}} @{$Words};
    if($Type eq "Constants")
    {
        my @Words_With_Tokens = ();
        foreach my $Word (@{$Words})
        {
            if($Word_Coincidence{$Word}>0) {
                push(@Words_With_Tokens, $Word);
            }
        }
        @{$Words} = @Words_With_Tokens;
    }
}

sub sort_FileOpen($)
{
    my @Interfaces = @{$_[0]};
    my (@FileOpen, @Other) = ();
    foreach my $Interface (sort {length($a) <=> length($b)} @Interfaces)
    {
        if($CompleteSignature{$Interface}{"ShortName"}=~/fopen/i) {
            push(@FileOpen, $Interface);
        }
        else {
            push(@Other, $Interface);
        }
    }
    @{$_[0]} = (@FileOpen, @Other);
}

sub get_word_coinsidence($$)
{
    my ($Word, $KeyWords) = @_;
    my @WordTokens1 = @{get_tokens($Word)};
    return 0 if($#WordTokens1==-1);
    my %WordTokens_Inc = ();
    my $WordTokens_Num = 0;
    foreach my $Token (@WordTokens1)
    {
        next if($Token=~/\A(get|create|new|insert)\Z/);
        $WordTokens_Inc{$Token} = ++$WordTokens_Num;
    }
    my @WordTokens2 = @{get_tokens($KeyWords)};
    return 0 if($#WordTokens2==-1);
    my $Weight=$#WordTokens2+2;
    my $StartWeight = $Weight;
    my $WordCoincidence = 0;
    foreach my $Token (@WordTokens2)
    {
        next if($Token=~/\A(get|create|new|insert)\Z/);
        if(defined $WordTokens_Inc{$Token} or defined $WordTokens_Inc{$ShortTokens{$Token}})
        {
            if($WordTokens_Inc{$Token}==1
            and $Library_Prefixes{$Token}+$Library_Prefixes{$Token."_"}>=$LIBRARY_PREFIX_MAJORITY)
            {# first token is usually a library prefix
                $WordCoincidence+=$Weight;
            }
            else {
                $WordCoincidence+=$Weight-$WordTokens_Num/($StartWeight+$WordTokens_Num);
            }
        }
        $Weight-=1;
    }
    return $WordCoincidence*100/($#WordTokens2+1);
}

sub compare_byCriteria($$)
{
    my ($Interface, $Criteria) = @_;
    if($Criteria eq "DeleteSmth") {
        return $CompleteSignature{$Interface}{"ShortName"}!~/delete|remove|destroy|cancel/i;
    }
    elsif($Criteria eq "InLine") {
        return $CompleteSignature{$Interface}{"InLine"};
    }
    elsif($Criteria eq "Function") {
        return $CompleteSignature{$Interface}{"Type"} eq "Function";
    }
    elsif($Criteria eq "WithParams") {
        return getNumOfParams($Interface);
    }
    elsif($Criteria eq "WithoutParams") {
        return getNumOfParams($Interface)==0;
    }
    elsif($Criteria eq "Public") {
        return (not $CompleteSignature{$Interface}{"Protected"});
    }
    elsif($Criteria eq "Default") {
        return ($Interface=~/default/i);
    }
    elsif($Criteria eq "VaList") {
        return ($Interface!~/valist/i);
    }
    elsif($Criteria eq "NotInCharge") {
        return (not isNotInCharge($Interface));
    }
    elsif($Criteria eq "Class") {
        return (get_TypeName($CompleteSignature{$Interface}{"Class"}) ne "QApplication");
    }
    elsif($Criteria eq "Data") {
        return (not $CompleteSignature{$Interface}{"Data"});
    }
    elsif($Criteria eq "FirstParam_Intrinsic")
    {
        if(defined $CompleteSignature{$Interface}{"Param"}
        and defined $CompleteSignature{$Interface}{"Param"}{"0"}) {
            my $FirstParamType_Id = $CompleteSignature{$Interface}{"Param"}{"0"}{"type"};
            return (get_TypeType(get_FoundationTypeId($FirstParamType_Id)) eq "Intrinsic");
        }
        else {
            return 0;
        }
    }
    elsif($Criteria eq "FirstParam_Enum")
    {
        if(defined $CompleteSignature{$Interface}{"Param"}
        and defined $CompleteSignature{$Interface}{"Param"}{"0"}) {
            my $FirstParamType_Id = $CompleteSignature{$Interface}{"Param"}{"0"}{"type"};
            return (get_TypeType(get_FoundationTypeId($FirstParamType_Id)) eq "Enum");
        }
        else {
            return 0;
        }
    }
    elsif($Criteria eq "FirstParam_PKc")
    {
        if(defined $CompleteSignature{$Interface}{"Param"}
        and defined $CompleteSignature{$Interface}{"Param"}{"0"})
        {
            my $FirstParamType_Id = $CompleteSignature{$Interface}{"Param"}{"0"}{"type"};
            return (get_TypeName($FirstParamType_Id) eq "char const*");
        }
        else {
            return 0;
        }
    }
    elsif($Criteria eq "FirstParam_char")
    {
        if(defined $CompleteSignature{$Interface}{"Param"}
        and defined $CompleteSignature{$Interface}{"Param"}{"0"})
        {
            my $FirstParamType_Id = $CompleteSignature{$Interface}{"Param"}{"0"}{"type"};
            return (get_TypeName($FirstParamType_Id) eq "char");
        }
        else {
            return 0;
        }
    }
    elsif($Criteria eq "Operator") {
        return ($tr_name{$Interface}!~/operator[^a-z]/i);
    }
    elsif($Criteria eq "Library") {
        return ($Interface_Library{$Interface} or $ClassInTargetLibrary{$CompleteSignature{$Interface}{"Class"}});
    }
    elsif($Criteria eq "Internal") {
        return ($CompleteSignature{$Interface}{"ShortName"}!~/\A_/);
    }
    elsif($Criteria eq "Internal") {
        return ($CompleteSignature{$Interface}{"ShortName"}!~/debug/i);
    }
    elsif($Criteria eq "FileManipulating")
    {
        return 0 if($CompleteSignature{$Interface}{"ShortName"}=~/fopen|file/);
        foreach my $ParamPos (keys(%{$CompleteSignature{$Interface}{"Param"}}))
        {
            my $ParamTypeId = $CompleteSignature{$Interface}{"Param"}{$ParamPos}{"type"};
            my $ParamName = $CompleteSignature{$Interface}{"Param"}{$ParamPos}{"name"};
            if(isString($ParamTypeId, $ParamName, $Interface)) {
                return 0 if(isStr_FileName($ParamPos, $ParamName, $CompleteSignature{$Interface}{"ShortName"})
                or isStr_Dir($ParamName, $CompleteSignature{$Interface}{"ShortName"}));
            }
            else {
                return 0 if(isFD($ParamTypeId, $ParamName));
            }
        }
        return 1;
    }
    else {
        return 1;
    }
}

sub sort_byRecurParams($)
{
    my @Interfaces = @{$_[0]};
    my (@Other, @WithRecurParams) = ();
    foreach my $Interface (@Interfaces)
    {
        my $WithRecur = 0;
        foreach my $ParamPos (keys(%{$CompleteSignature{$Interface}{"Param"}}))
        {
            my $ParamType_Id = $CompleteSignature{$Interface}{"Param"}{$ParamPos}{"type"};
            if(isCyclical(\@RecurTypeId, get_TypeStackId($ParamType_Id))) {
                $WithRecur=1;
                last;
            }
        }
        if($WithRecur) {
            push(@WithRecurParams, $Interface);
        }
        else
        {
            if($CompleteSignature{$Interface}{"ShortName"}!~/copy|duplicate/i) {
                push(@Other, $Interface);
            }
        }
    }
    @{$_[0]} = (@Other, @WithRecurParams);
    return $#WithRecurParams;
}

sub sort_LibMainFunc($)
{
    my @Interfaces = @{$_[0]};
    my (@First, @Other) = ();
    foreach my $Interface (@Interfaces)
    {
        my $ShortName = cut_NamePrefix($CompleteSignature{$Interface}{"ShortName"});
        if($ShortName=~/\A(create|default|get|new|init)\Z/i) {
            push(@First, $Interface);
        }
        else {
             push(@Other, $Interface);
        }
    }
    @{$_[0]} = (@First, @Other);
}

sub sort_CreateParam($$)
{
    my @Interfaces = @{$_[0]};
    my $KeyWords = $_[1];
    foreach my $Prefix (keys(%Library_Prefixes))
    {
        if($Library_Prefixes{$Prefix}>=$LIBRARY_PREFIX_MAJORITY) {
            $KeyWords=~s/(\A| )$Prefix/$1/g;
        }
    }
    $KeyWords=~s/(\A|_)(new|get|create|default|alloc|init)(_|\Z)//g;
    my (@First, @Other) = ();
    foreach my $Interface (@Interfaces)
    {
        my $ShortName = $CompleteSignature{$Interface}{"ShortName"};
        if($ShortName=~/create|default|get|new|init/i
        and get_word_coinsidence($ShortName, $KeyWords)>0) {
            push(@First, $Interface);
        }
        else {
             push(@Other, $Interface);
        }
    }
    @{$_[0]} = (@First, @Other);
}

sub grep_token($$)
{
    my ($Word, $Token) = @_;
    return grep(/\A($Token)\Z/i, @{get_tokens($Word)});
}

sub cut_NamePrefix($)
{
    my $Name = $_[0];
    return "" if(not $Name);
    if(my $Prefix = getPrefix($Name))
    {
        if($Library_Prefixes{$Prefix}>=$LIBRARY_PREFIX_MAJORITY) {
            $Name=~s/\A\Q$Prefix\E//;
        }
    }
    return $Name;
}

sub sort_GetCreate($)
{
    my @Interfaces = @{$_[0]};
    my (@Open, @Root, @Create, @Default, @New, @Alloc, @Init, @Get, @Other, @Copy, @Wait) = ();
    foreach my $Interface (@Interfaces)
    {
        my $ShortName = $CompleteSignature{$Interface}{"ShortName"};
        if(grep_token($ShortName, "open")) {
            push(@Open, $Interface);
        }
        elsif(grep_token($ShortName, "root")
        and grep_token($ShortName, "default")) {
            push(@Root, $Interface);
        }
        elsif(grep_token($ShortName, "create")) {
            push(@Create, $Interface);
        }
        elsif(grep_token($ShortName, "default")
        and not grep_token($ShortName, "get")) {
            push(@Default, $Interface);
        }
        elsif(grep_token($ShortName, "new")) {
            push(@New, $Interface);
        }
        elsif(is_alloc_func($ShortName)) {
            push(@Alloc, $Interface);
        }
        elsif(grep_token($ShortName, "init")) {
            push(@Init, $Interface);
        }
        elsif(grep_token($ShortName, "get")) {
            push(@Get, $Interface);
        }
        elsif(grep_token($ShortName, "copy")) {
            push(@Copy, $Interface);
        }
        elsif(grep_token($ShortName, "wait")) {
            push(@Wait, $Interface);
        }
        else {
            push(@Other, $Interface);
        }
    }
    my @PrimaryGroup = (@Open, @Root, @Create, @Default);
    sort_byCriteria(\@PrimaryGroup, "WithoutParams");
    @{$_[0]} = (@PrimaryGroup, @New, @Alloc, @Init, @Get, @Other, @Copy, @Wait);
}

sub get_CompatibleInterfaces($$$)
{
    my ($TypeId, $Method, $KeyWords) = @_;
    return () if(not $TypeId or not $Method);
    my @Ints = compatible_interfaces($TypeId, $Method, $KeyWords);
    sort_byRecurParams(\@Ints) if(get_TypeName($TypeId)!~/time_t/);
    return @Ints;
}

sub compatible_interfaces($$$)
{
    my ($TypeId, $Method, $KeyWords) = @_;
    return () if(not $TypeId or not $Method);
    return @{$Cache{"compatible_interfaces"}{$TypeId}{$Method}{$KeyWords}} if(defined $Cache{"compatible_interfaces"}{$TypeId}{$Method}{$KeyWords} and not defined $RandomCode and not defined $AuxType{$TypeId});
    my @CompatibleInterfaces = ();
    if($Method eq "Construct") {
        foreach my $Constructor (keys(%{$Class_Constructors{$TypeId}})) {
            @CompatibleInterfaces = (@CompatibleInterfaces, $Constructor);
        }
    }
    elsif($Method eq "Return") {
        foreach my $Interface (keys(%{$ReturnTypeId_Interface{$TypeId}})) {
            next if($CompleteSignature{$Interface}{"PureVirt"});
            @CompatibleInterfaces = (@CompatibleInterfaces, $Interface);
        }
    }
    elsif($Method eq "OutParam") {
        foreach my $Interface (keys(%{$OutParam_Interface{$TypeId}}))
        {
            next if($CompleteSignature{$Interface}{"Protected"});
            next if($CompleteSignature{$Interface}{"PureVirt"});
            @CompatibleInterfaces = (@CompatibleInterfaces, $Interface);
        }
    }
    elsif($Method eq "OnlyReturn") {
        foreach my $Interface (keys(%{$ReturnTypeId_Interface{$TypeId}}))
        {
            next if($CompleteSignature{$Interface}{"PureVirt"});
            next if($CompleteSignature{$Interface}{"Data"});
            @CompatibleInterfaces = (@CompatibleInterfaces, $Interface);
        }
    }
    elsif($Method eq "OnlyData") {
        foreach my $Interface (keys(%{$ReturnTypeId_Interface{$TypeId}})) {
            next if(not $CompleteSignature{$Interface}{"Data"});
            @CompatibleInterfaces = (@CompatibleInterfaces, $Interface);
        }
    }
    else {
        @{$Cache{"compatible_interfaces"}{$TypeId}{$Method}{$KeyWords}} = ();
        return ();
    }
    if($#CompatibleInterfaces==-1) {
        @{$Cache{"compatible_interfaces"}{$TypeId}{$Method}{$KeyWords}} = ();
        return ();
    }
    elsif($#CompatibleInterfaces==0) {
        @{$Cache{"compatible_interfaces"}{$TypeId}{$Method}{$KeyWords}} = @CompatibleInterfaces;
        return @CompatibleInterfaces;
    }
    # sort by name
    @CompatibleInterfaces = sort @CompatibleInterfaces;
    @CompatibleInterfaces = sort {$CompleteSignature{$a}{"ShortName"} cmp $CompleteSignature{$b}{"ShortName"}} (@CompatibleInterfaces);
    @CompatibleInterfaces = sort {length($CompleteSignature{$a}{"ShortName"}) <=> length($CompleteSignature{$b}{"ShortName"})} (@CompatibleInterfaces);
    # sort by number of parameters
    if(defined $MinimumCode) {
        @CompatibleInterfaces = sort {int(keys(%{$CompleteSignature{$a}{"Param"}}))<=>int(keys(%{$CompleteSignature{$b}{"Param"}}))} (@CompatibleInterfaces);
    }
    elsif(defined $MaximumCode) {
        @CompatibleInterfaces = sort {int(keys(%{$CompleteSignature{$b}{"Param"}}))<=>int(keys(%{$CompleteSignature{$a}{"Param"}}))} (@CompatibleInterfaces);
    }
    else
    {
        sort_byCriteria(\@CompatibleInterfaces, "FirstParam_Intrinsic");
        sort_byCriteria(\@CompatibleInterfaces, "FirstParam_char");
        sort_byCriteria(\@CompatibleInterfaces, "FirstParam_PKc");
        sort_byCriteria(\@CompatibleInterfaces, "FirstParam_Enum") if(get_TypeName($TypeId)!~/char|string/i or $Method ne "Construct");
        @CompatibleInterfaces = sort {int(keys(%{$CompleteSignature{$a}{"Param"}}))<=>int(keys(%{$CompleteSignature{$b}{"Param"}}))} (@CompatibleInterfaces);
        @CompatibleInterfaces = sort {$b=~/virtual/i <=> $a=~/virtual/i} (@CompatibleInterfaces);
        sort_byCriteria(\@CompatibleInterfaces, "WithoutParams");
        sort_byCriteria(\@CompatibleInterfaces, "WithParams") if($Method eq "Construct");
    }
    sort_byCriteria(\@CompatibleInterfaces, "Operator");
    sort_byCriteria(\@CompatibleInterfaces, "FileManipulating");
    if($Method ne "Construct")
    {
        sort_byCriteria(\@CompatibleInterfaces, "Class");
        sort_CreateParam(\@CompatibleInterfaces, $KeyWords);
        sort_GetCreate(\@CompatibleInterfaces);
        sort_byName(\@CompatibleInterfaces, $KeyWords, "Interfaces");
        sort_FileOpen(\@CompatibleInterfaces) if(get_TypeName(get_FoundationTypeId($TypeId))=~/\A(struct |)(_IO_FILE|__FILE|FILE|_iobuf)\Z/);
        sort_LibMainFunc(\@CompatibleInterfaces);
        sort_byCriteria(\@CompatibleInterfaces, "Data");
        sort_byCriteria(\@CompatibleInterfaces, "Function");
        sort_byCriteria(\@CompatibleInterfaces, "Library");
        sort_byCriteria(\@CompatibleInterfaces, "Internal");
        sort_byCriteria(\@CompatibleInterfaces, "Debug");
        if(get_TypeName($TypeId) ne "GType"
        and (my $Lib = get_TypeLib($TypeId)) ne "unknown") {
            sort_byLibrary(\@CompatibleInterfaces, $Lib);
        }
    }
    if(defined $RandomCode) {
        @CompatibleInterfaces = mix_array(@CompatibleInterfaces);
    }
    sort_byCriteria(\@CompatibleInterfaces, "Public");
    sort_byCriteria(\@CompatibleInterfaces, "NotInCharge") if($Method eq "Construct");
    @{$Cache{"compatible_interfaces"}{$TypeId}{$Method}{$KeyWords}} = @CompatibleInterfaces if(not defined $RandomCode);
    return @CompatibleInterfaces;
}

sub mix_array(@)
{
    my @Array = @_;
    return sort {2 * rand($#Array) - $#Array} @_;
}

sub getSomeConstructor($)
{
    my $ClassId = $_[0];
    my @Constructors = get_CompatibleInterfaces($ClassId, "Construct", "");
    return $Constructors[0];
}

sub getFirstEnumMember($)
{
    my $EnumId = $_[0];
    my %Enum = get_Type($Tid_TDid{$EnumId}, $EnumId);
    my $FirstMemberName = $Enum{"Memb"}{0}{"name"};
    my $NameSpace = $Enum{"NameSpace"};
    if($NameSpace and $FirstMemberName
    and getIntLang($TestedInterface) eq "C++") {
        $FirstMemberName = $NameSpace."::".$FirstMemberName;
    }
    return $FirstMemberName;
}

sub getTypeParString($)
{
    my $Interface = $_[0];
    my $NumOfParams = getNumOfParams($Interface);
    if($NumOfParams == 0) {
        return ("()", "()", "()");
    }
    else
    {
        my (@TypeParList, @ParList, @TypeList) = ();
        foreach my $Param_Pos (sort {int($a)<=>int($b)} keys(%{$CompleteSignature{$Interface}{"Param"}}))
        {
            next if(apply_default_value($Interface, $Param_Pos) and not $CompleteSignature{$Interface}{"PureVirt"});
            my $ParamName = $CompleteSignature{$Interface}{"Param"}{$Param_Pos}{"name"};
            $ParamName = "p".($Param_Pos + 1) if(not $ParamName);
            my $TypeId = $CompleteSignature{$Interface}{"Param"}{$Param_Pos}{"type"};
            my %Type = get_Type($Tid_TDid{$TypeId}, $TypeId);
            next if($Type{"Name"} eq "...");
            push(@ParList, $ParamName);
            push(@TypeList, $Type{"Name"});
            push(@TypeParList, create_member_decl($Type{"Name"}, $ParamName));
        }
        my $TypeParString .= "(".create_list(\@TypeParList, "    ").")";
        my $ParString .= "(".create_list(\@ParList, "    ").")";
        my $TypeString .= "(".create_list(\@TypeList, "    ").")";
        return ($TypeParString, $ParString, $TypeString);
    }
}

sub getValueClass($)
{
    my $Value = $_[0];
    $Value=~/([^()"]+)\(.*\)[^()]*/;
    my $ValueClass = $1;
    $ValueClass=~s/[ ]+\Z//g;
    if(get_TypeIdByName($ValueClass)) {
        return $ValueClass;
    }
    else {
        return "";
    }
}

sub get_FoundationType($$)
{
    my ($TypeDId, $TypeId) = @_;
    return "" if(not $TypeId);
    if(defined $Cache{"get_FoundationType"}{$TypeDId}{$TypeId}
    and not defined $AuxType{$TypeId}) {
        return %{$Cache{"get_FoundationType"}{$TypeDId}{$TypeId}};
    }
    return "" if(not $TypeDescr{$TypeDId}{$TypeId});
    my %Type = %{$TypeDescr{$TypeDId}{$TypeId}};
    return %Type if(not $Type{"BaseType"}{"TDid"} and not $Type{"BaseType"}{"Tid"});
    return %Type if($Type{"Type"} eq "Array");
    %Type = get_FoundationType($Type{"BaseType"}{"TDid"}, $Type{"BaseType"}{"Tid"});
     %{$Cache{"get_FoundationType"}{$TypeDId}{$TypeId}} = %Type;
    return %Type;
}

sub get_BaseType($$)
{
    my ($TypeDId, $TypeId) = @_;
    return "" if(not $TypeId);
    if(defined $Cache{"get_BaseType"}{$TypeDId}{$TypeId}
    and not defined $AuxType{$TypeId}) {
        return %{$Cache{"get_BaseType"}{$TypeDId}{$TypeId}};
    }
    return "" if(not $TypeDescr{$TypeDId}{$TypeId});
    my %Type = %{$TypeDescr{$TypeDId}{$TypeId}};
    return %Type if(not $Type{"BaseType"}{"TDid"} and not $Type{"BaseType"}{"Tid"});
    %Type = get_BaseType($Type{"BaseType"}{"TDid"}, $Type{"BaseType"}{"Tid"});
     %{$Cache{"get_BaseType"}{$TypeDId}{$TypeId}} = %Type;
    return %Type;
}

sub get_FoundationTypeId($)
{
    my $TypeId = $_[0];
    return $Cache{"get_FoundationTypeId"}{$TypeId} if(defined $Cache{"get_FoundationTypeId"}{$TypeId} and not defined $AuxType{$TypeId});
    my %BaseType = get_FoundationType($Tid_TDid{$TypeId}, $TypeId);
    $Cache{"get_FoundationTypeId"}{$TypeId} = $BaseType{"Tid"};
    return $BaseType{"Tid"};
}

sub create_SubClass($)
{
    my $ClassId = $_[0];
    return () if(not $ClassId);
    my ($Declaration, $Headers, $Code);
    foreach my $Constructor (keys(%{$UsedConstructors{$ClassId}}))
    {
        if(isNotInCharge($Constructor)
        and my $InChargeConstructor = replace_c2c1($Constructor))
        {
            if($CompleteSignature{$InChargeConstructor})
            {
                $UsedConstructors{$ClassId}{$Constructor} = 0;
                $UsedConstructors{$ClassId}{$InChargeConstructor} = 1;
            }
        }
    }
    $Headers = addHeaders(getTypeHeaders($ClassId), $Headers);
    my $ClassName = ($Class_SubClassTypedef{$ClassId})?get_TypeName($Class_SubClassTypedef{$ClassId}):get_TypeName($ClassId);
    my $ClassNameChild = getSubClassName($ClassName);
    $Declaration .= "class $ClassNameChild".": public $ClassName\n{\n";
    $Declaration .= "public:\n";
    if(not keys(%{$UsedConstructors{$ClassId}}))
    {
        if(my $SomeConstructor = getSomeConstructor($ClassId)) {
            $UsedConstructors{$ClassId}{$SomeConstructor} = 1;
        }
    }
    if(defined $UsedConstructors{$ClassId}
    and keys(%{$UsedConstructors{$ClassId}}))
    {
        foreach my $Constructor (sort keys(%{$UsedConstructors{$ClassId}}))
        {
            next if(not $Constructor);
            my $PreviousBlock = $CurrentBlock;
            $CurrentBlock = $Constructor;
            if($UsedConstructors{$ClassId}{$Constructor})
            {
                my ($TypeParString, $ParString, $TypeString) = getTypeParString($Constructor);
                $TypeParString = alignCode($TypeParString, "    ", 1);
                $ParString = alignCode($ParString, "        ", 1);
                $Declaration .= "    $ClassNameChild"."$TypeParString\:$ClassName"."$ParString\{\}\n\n";
                foreach my $Param_Pos (sort {int($b)<=>int($a)} keys(%{$CompleteSignature{$Constructor}{"Param"}}))
                {
                    my $Param_TypeId = $CompleteSignature{$Constructor}{"Param"}{$Param_Pos}{"type"};
                    my $Param_Name = $CompleteSignature{$Constructor}{"Param"}{$Param_Pos}{"name"};
                    $Param_Name = "p".($Param_Pos + 1) if(not $Param_Name);
                    $ValueCollection{$CurrentBlock}{$Param_Name} = $Param_TypeId;
                    $Block_Param{$CurrentBlock}{$Param_Name} = $Param_TypeId;
                    $Block_Variable{$CurrentBlock}{$Param_Name} = 1;
                }
            }
            $CurrentBlock = $PreviousBlock;
        }
    }
    else {
        $Declaration .= "    ".$ClassNameChild."();\n";
    }
    if(defined $Class_PureVirtFunc{$ClassName})
    {
        my %RedefinedTwice = ();
        foreach my $PureVirtualMethod (sort {lc($CompleteSignature{$a}{"ShortName"}) cmp lc($CompleteSignature{$b}{"ShortName"})} keys(%{$Class_PureVirtFunc{$ClassName}}))
        {
            my $PreviousBlock = $CurrentBlock;
            $CurrentBlock = $PureVirtualMethod;
            delete($ValueCollection{$CurrentBlock});
            delete($Block_Variable{$CurrentBlock});
            my $ReturnTypeId = $CompleteSignature{$PureVirtualMethod}{"Return"};
            my $ReturnTypeName = get_TypeName($ReturnTypeId);
            my ($TypeParString, $ParString, $TypeString) = getTypeParString($PureVirtualMethod);
            $TypeParString = alignCode($TypeParString, "    ", 1);
            my ($PureVirtualMethodName, $ShortName) = ("", "");
            if($CompleteSignature{$PureVirtualMethod}{"Constructor"})
            {
                $ShortName = $ClassNameChild;
                $PureVirtualMethodName = "    ".$ShortName.$TypeParString;
            }
            if($CompleteSignature{$PureVirtualMethod}{"Destructor"})
            {
                $ShortName = "~".$ClassNameChild;
                $PureVirtualMethodName = "   ".$ShortName.$TypeParString;
            }
            else
            {
                $ShortName = $CompleteSignature{$PureVirtualMethod}{"ShortName"};
                $PureVirtualMethodName = "    ".$ReturnTypeName." ".$ShortName.$TypeParString;
            }
            if($CompleteSignature{$PureVirtualMethod}{"Throw"}) {
                $PureVirtualMethodName .= " throw()";
            }
            my $Const = ($PureVirtualMethod=~/\A_ZNK/)?" const":"";
            next if($RedefinedTwice{$ShortName.$TypeString.$Const});
            $RedefinedTwice{$ShortName.$TypeString.$Const} = 1;
            $Declaration .= "\n" if($PureVirtualMethodName=~/\n/);
            foreach my $Param_Pos (sort {int($b)<=>int($a)} keys(%{$CompleteSignature{$PureVirtualMethod}{"Param"}}))
            {
                my $Param_TypeId = $CompleteSignature{$PureVirtualMethod}{"Param"}{$Param_Pos}{"type"};
                my $Param_Name = $CompleteSignature{$PureVirtualMethod}{"Param"}{$Param_Pos}{"name"};
                $Param_Name = "p".($Param_Pos + 1) if(not $Param_Name);
                $ValueCollection{$CurrentBlock}{$Param_Name} = $Param_TypeId;
                $Block_Param{$CurrentBlock}{$Param_Name} = $Param_TypeId;
                $Block_Variable{$CurrentBlock}{$Param_Name} = 1;
            }
            if(get_TypeName($ReturnTypeId) eq "void"
            or $CompleteSignature{$PureVirtualMethod}{"Constructor"}
            or $CompleteSignature{$PureVirtualMethod}{"Destructor"}) {
                $Declaration .= $PureVirtualMethodName.$Const."\{\}\n\n";
            }
            else
            {
                $Declaration .= $PureVirtualMethodName.$Const." {\n";
                my $ReturnTypeHeaders = getTypeHeaders($ReturnTypeId);
                push(@RecurInterface, $PureVirtualMethod);
                my %Param_Init = initializeParameter((
                    "ParamName" => "retval",
                    "AccessToParam" => {"obj"=>"no object"},
                    "TypeId" => $ReturnTypeId,
                    "Key" => "_ret",
                    "InLine" => 1,
                    "Value" => "no value",
                    "CreateChild" => 0,
                    "SpecType" => 0,
                    "Usage" => "Common",
                    "RetVal" => 1));
                pop(@RecurInterface);
                $Code .= $Param_Init{"Code"};
                $Headers = addHeaders($Param_Init{"Headers"}, $Headers);
                $Headers = addHeaders($ReturnTypeHeaders, $Headers);
                $Param_Init{"Init"} = alignCode($Param_Init{"Init"}, "       ", 0);
                $Param_Init{"Call"} = alignCode($Param_Init{"Call"}, "       ", 1);
                $Declaration .= $Param_Init{"Init"}."       return ".$Param_Init{"Call"}.";\n    }\n\n";
            }
            $CurrentBlock = $PreviousBlock;
        }
    }
    if(defined $UsedProtectedMethods{$ClassId})
    {
        foreach my $ProtectedMethod (sort {lc($CompleteSignature{$a}{"ShortName"}) cmp lc($CompleteSignature{$b}{"ShortName"})} keys(%{$UsedProtectedMethods{$ClassId}}))
        {
            my $ReturnType_Id = $CompleteSignature{$ProtectedMethod}{"Return"};
            my $ReturnType_Name = get_TypeName($ReturnType_Id);
            my $ReturnType_PointerLevel = get_PointerLevel($Tid_TDid{$ReturnType_Id}, $ReturnType_Id);
            my $ReturnFType_Id = get_FoundationTypeId($ReturnType_Id);
            my $ReturnFType_Name = get_TypeName($ReturnFType_Id);
            my $Break = ((length($ReturnType_Name)>20)?"\n":" ");
            my $ShortName = $CompleteSignature{$ProtectedMethod}{"ShortName"};
            my $ShortNameAdv = $ShortName."_Wrapper";
            $ShortNameAdv = cleanName($ShortNameAdv);
            $Declaration .= "    ".$ReturnType_Name." ".$ShortNameAdv."() {\n";
            if($Wrappers{$ProtectedMethod}{"Init"}) {
                $Declaration .= alignCode($Wrappers{$ProtectedMethod}{"Init"}, "       ", 0);
            }
            $Declaration .= alignCode($Wrappers{$ProtectedMethod}{"PreCondition"}, "      ", 0);
            my $FuncCall = "this->".alignCode($ShortName.$Wrappers{$ProtectedMethod}{"Parameters_Call"}, "      ", 1);
            if($Wrappers{$ProtectedMethod}{"PostCondition"} or $Wrappers{$ProtectedMethod}{"FinalCode"})
            {
                my $PostCode = alignCode($Wrappers{$ProtectedMethod}{"PostCondition"}, "      ", 0).alignCode($Wrappers{$ProtectedMethod}{"FinalCode"}, "      ", 0);
                # FIXME: destructors
                if($ReturnFType_Name eq "void" and $ReturnType_PointerLevel==0) {
                    $Declaration .= "       $FuncCall;\n".$PostCode;
                }
                else
                {
                    my $RetVal = select_var_name("retval", "");
                    my ($InitializedEType_Id, $Ret_Declarations, $Ret_Headers) = get_ExtTypeId($RetVal, $ReturnType_Id);
                    $Code .= $Ret_Declarations;
                    $Headers = addHeaders($Ret_Headers, $Headers);
                    my $InitializedType_Name = get_TypeName($InitializedEType_Id);
                    if($InitializedType_Name eq $ReturnType_Name) {
                        $Declaration .= "      ".$InitializedType_Name.$Break.$RetVal." = $FuncCall;\n".$PostCode;
                    }
                    else {
                        $Declaration .= "      ".$InitializedType_Name.$Break.$RetVal." = ($InitializedType_Name)$FuncCall;\n".$PostCode;
                    }
                    $Block_Variable{$ProtectedMethod}{$RetVal} = 1;
                    $Declaration .= "       return $RetVal;\n";
                }
            }
            else
            {
                if($ReturnFType_Name eq "void" and $ReturnType_PointerLevel==0) {
                    $Declaration .= "       $FuncCall;\n";
                }
                else {
                    $Declaration .= "       return $FuncCall;\n";
                }
            }
            $Code .= $Wrappers{$ProtectedMethod}{"Code"};
            $Declaration .= "    }\n\n";
            foreach my $ClassId (keys(%{$Wrappers_SubClasses{$ProtectedMethod}})) {
                $Create_SubClass{$ClassId} = 1;
            }
        }
    }
    $Declaration .= "};//$ClassNameChild\n\n";
    return ($Code.$Declaration, $Headers);
}

sub create_SubClasses(@)
{
    my ($Code, $Headers) = ("", []);
    foreach my $ClassId (sort @_)
    {
        my (%Before, %After, %New) = ();
        next if(not $ClassId or $SubClass_Created{$ClassId});
        %Create_SubClass = ();
        push(@RecurTypeId, $ClassId);
        my ($Code_SubClass, $Headers_SubClass) = create_SubClass($ClassId);
        $SubClass_Created{$ClassId} = 1;
        if(keys(%Create_SubClass))
        {
            my ($Code_Depend, $Headers_Depend) = create_SubClasses(keys(%Create_SubClass));
            $Code_SubClass = $Code_Depend.$Code_SubClass;
            $Headers_SubClass = addHeaders($Headers_Depend, $Headers_SubClass);
        }
        pop(@RecurTypeId);
        $Code .= $Code_SubClass;
        $Headers = addHeaders($Headers_SubClass, $Headers);
    }
    return ($Code, $Headers);
}

sub save_state()
{
    my %Saved_State = ();
    foreach (keys(%IntSubClass))
    {
        @{$Saved_State{"IntSubClass"}{$_}}{keys(%{$IntSubClass{$_}})} = values %{$IntSubClass{$_}};
    }
    foreach (keys(%Wrappers))
    {
        @{$Saved_State{"Wrappers"}{$_}}{keys(%{$Wrappers{$_}})} = values %{$Wrappers{$_}};
    }
    foreach (keys(%Wrappers_SubClasses))
    {
        @{$Saved_State{"Wrappers_SubClasses"}{$_}}{keys(%{$Wrappers_SubClasses{$_}})} = values %{$Wrappers_SubClasses{$_}};
    }
    foreach (keys(%ValueCollection))
    {
        @{$Saved_State{"ValueCollection"}{$_}}{keys(%{$ValueCollection{$_}})} = values %{$ValueCollection{$_}};
    }
    foreach (keys(%Block_Variable))
    {
        @{$Saved_State{"Block_Variable"}{$_}}{keys(%{$Block_Variable{$_}})} = values %{$Block_Variable{$_}};
    }
    foreach (keys(%UseVarEveryWhere))
    {
        @{$Saved_State{"UseVarEveryWhere"}{$_}}{keys(%{$UseVarEveryWhere{$_}})} = values %{$UseVarEveryWhere{$_}};
    }
    foreach (keys(%OpenStreams))
    {
        @{$Saved_State{"OpenStreams"}{$_}}{keys(%{$OpenStreams{$_}})} = values %{$OpenStreams{$_}};
    }
    foreach (keys(%Block_Param))
    {
        @{$Saved_State{"Block_Param"}{$_}}{keys(%{$Block_Param{$_}})} = values %{$Block_Param{$_}};
    }
    foreach (keys(%UsedConstructors))
    {
        @{$Saved_State{"UsedConstructors"}{$_}}{keys(%{$UsedConstructors{$_}})} = values %{$UsedConstructors{$_}};
    }
    foreach (keys(%UsedProtectedMethods))
    {
        @{$Saved_State{"UsedProtectedMethods"}{$_}}{keys(%{$UsedProtectedMethods{$_}})} = values %{$UsedProtectedMethods{$_}};
    }
    foreach (keys(%IntSpecType))
    {
        @{$Saved_State{"IntSpecType"}{$_}}{keys(%{$IntSpecType{$_}})} = values %{$IntSpecType{$_}};
    }
    foreach (keys(%RequirementsCatalog))
    {
        @{$Saved_State{"RequirementsCatalog"}{$_}}{keys(%{$RequirementsCatalog{$_}})} = values %{$RequirementsCatalog{$_}};
    }
    @{$Saved_State{"Template2Code_Defines"}}{keys(%Template2Code_Defines)} = values %Template2Code_Defines;
    @{$Saved_State{"TraceFunc"}}{keys(%TraceFunc)} = values %TraceFunc;
    $Saved_State{"MaxTypeId"} = $MaxTypeId;
    @{$Saved_State{"IntrinsicNum"}}{keys(%IntrinsicNum)} = values %IntrinsicNum;
    @{$Saved_State{"AuxHeaders"}}{keys(%AuxHeaders)} = values %AuxHeaders;
    @{$Saved_State{"Class_SubClassTypedef"}}{keys(%Class_SubClassTypedef)} = values %Class_SubClassTypedef;
    @{$Saved_State{"SubClass_Instance"}}{keys(%SubClass_Instance)} = values %SubClass_Instance;
    @{$Saved_State{"SubClass_ObjInstance"}}{keys(%SubClass_ObjInstance)} = values %SubClass_ObjInstance;
    @{$Saved_State{"SpecEnv"}}{keys(%SpecEnv)} = values %SpecEnv;
    @{$Saved_State{"Block_InsNum"}}{keys(%Block_InsNum)} = values %Block_InsNum;
    @{$Saved_State{"AuxType"}}{keys %AuxType} = values %AuxType;
    @{$Saved_State{"AuxFunc"}}{keys %AuxFunc} = values %AuxFunc;
    @{$Saved_State{"Create_SubClass"}}{keys %Create_SubClass} = values %Create_SubClass;
    @{$Saved_State{"SpecCode"}}{keys %SpecCode} = values %SpecCode;
    @{$Saved_State{"SpecLibs"}}{keys %SpecLibs} = values %SpecLibs;
    @{$Saved_State{"UsedInterfaces"}}{keys %UsedInterfaces} = values %UsedInterfaces;
    @{$Saved_State{"ConstraintNum"}}{keys %ConstraintNum} = values %ConstraintNum;
    return \%Saved_State;
}

sub restore_state($)
{
    restore_state_p($_[0], 0);
}

sub restore_local_state($)
{
    restore_state_p($_[0], 1);
}

sub restore_state_p($$)
{
    my ($Saved_State, $Local) = @_;
    if(not $Local)
    {
        foreach my $AuxType_Id (keys(%AuxType))
        {
            if(my $OldName = $TypeDescr{$Tid_TDid{$AuxType_Id}}{$AuxType_Id}{"Name_Old"})
            {
                $TypeDescr{$Tid_TDid{$AuxType_Id}}{$AuxType_Id}{"Name"} = $OldName;
            }
        }
        if(not $Saved_State)
        {#restoring aux types
            foreach my $AuxType_Id (sort {int($a)<=>int($b)} keys(%AuxType))
            {
                if(not $TypeDescr{""}{$AuxType_Id}{"Name_Old"})
                {
                    delete($TypeDescr{""}{$AuxType_Id});
                }
                delete($TName_Tid{$AuxType{$AuxType_Id}});
                delete($AuxType{$AuxType_Id});
            }
            $MaxTypeId = $MaxTypeId_Start;
        }
        elsif($Saved_State->{"MaxTypeId"})
        {
            foreach my $AuxType_Id (sort {int($a)<=>int($b)} keys(%AuxType))
            {
                if($AuxType_Id<=$MaxTypeId and $AuxType_Id>$Saved_State->{"MaxTypeId"})
                {
                    if(not $TypeDescr{""}{$AuxType_Id}{"Name_Old"})
                    {
                        delete($TypeDescr{""}{$AuxType_Id});
                    }
                    delete($TName_Tid{$AuxType{$AuxType_Id}});
                    delete($AuxType{$AuxType_Id});
                }
            }
        }
    }
    (%Block_Variable, %UseVarEveryWhere, %OpenStreams, %SpecEnv, %Block_InsNum,
    %ValueCollection, %IntrinsicNum, %ConstraintNum, %SubClass_Instance,
    %SubClass_ObjInstance, %Block_Param,%Class_SubClassTypedef, %AuxHeaders, %Template2Code_Defines) = ();
    if(not $Local)
    {
        (%Wrappers, %Wrappers_SubClasses, %IntSubClass, %AuxType, %AuxFunc,
        %UsedConstructors, %UsedProtectedMethods, %Create_SubClass, %SpecCode,
        %SpecLibs, %UsedInterfaces, %IntSpecType, %RequirementsCatalog, %TraceFunc) = ();
    }
    if(not $Saved_State)
    {#initializing
        %IntrinsicNum=(
            "Char"=>64,
            "Int"=>0,
            "Str"=>0,
            "Float"=>0);
        return;
    }
    foreach (keys(%{$Saved_State->{"Block_Variable"}}))
    {
        @{$Block_Variable{$_}}{keys(%{$Saved_State->{"Block_Variable"}{$_}})} = values %{$Saved_State->{"Block_Variable"}{$_}};
    }
    foreach (keys(%{$Saved_State->{"UseVarEveryWhere"}}))
    {
        @{$UseVarEveryWhere{$_}}{keys(%{$Saved_State->{"UseVarEveryWhere"}{$_}})} = values %{$Saved_State->{"UseVarEveryWhere"}{$_}};
    }
    foreach (keys(%{$Saved_State->{"OpenStreams"}}))
    {
        @{$OpenStreams{$_}}{keys(%{$Saved_State->{"OpenStreams"}{$_}})} = values %{$Saved_State->{"OpenStreams"}{$_}};
    }
    @SpecEnv{keys(%{$Saved_State->{"SpecEnv"}})} = values %{$Saved_State->{"SpecEnv"}};
    @Block_InsNum{keys(%{$Saved_State->{"Block_InsNum"}})} = values %{$Saved_State->{"Block_InsNum"}};
    foreach (keys(%{$Saved_State->{"ValueCollection"}}))
    {
        @{$ValueCollection{$_}}{keys(%{$Saved_State->{"ValueCollection"}{$_}})} = values %{$Saved_State->{"ValueCollection"}{$_}};
    }
    @Template2Code_Defines{keys(%{$Saved_State->{"Template2Code_Defines"}})} = values %{$Saved_State->{"Template2Code_Defines"}};
    @IntrinsicNum{keys(%{$Saved_State->{"IntrinsicNum"}})} = values %{$Saved_State->{"IntrinsicNum"}};
    @ConstraintNum{keys(%{$Saved_State->{"ConstraintNum"}})} = values %{$Saved_State->{"ConstraintNum"}};
    @SubClass_Instance{keys(%{$Saved_State->{"SubClass_Instance"}})} = values %{$Saved_State->{"SubClass_Instance"}};
    @SubClass_ObjInstance{keys(%{$Saved_State->{"SubClass_ObjInstance"}})} = values %{$Saved_State->{"SubClass_ObjInstance"}};
    foreach (keys(%{$Saved_State->{"Block_Param"}}))
    {
        @{$Block_Param{$_}}{keys(%{$Saved_State->{"Block_Param"}{$_}})} = values %{$Saved_State->{"Block_Param"}{$_}};
    }
    @Class_SubClassTypedef{keys(%{$Saved_State->{"Class_SubClassTypedef"}})} = values %{$Saved_State->{"Class_SubClassTypedef"}};
    @AuxHeaders{keys(%{$Saved_State->{"AuxHeaders"}})} = values %{$Saved_State->{"AuxHeaders"}};
    if(not $Local)
    {
        foreach my $AuxType_Id (sort {int($a)<=>int($b)} keys(%{$Saved_State->{"AuxType"}}))
        {
            $TypeDescr{$Tid_TDid{$AuxType_Id}}{$AuxType_Id}{"Name"} = $Saved_State->{"AuxType"}{$AuxType_Id};
            $TName_Tid{$Saved_State->{"AuxType"}{$AuxType_Id}} = $AuxType_Id;
        }
        foreach (keys(%{$Saved_State->{"IntSubClass"}}))
        {
            @{$IntSubClass{$_}}{keys(%{$Saved_State->{"IntSubClass"}{$_}})} = values %{$Saved_State->{"IntSubClass"}{$_}};
        }
        foreach (keys(%{$Saved_State->{"Wrappers"}}))
        {
            @{$Wrappers{$_}}{keys(%{$Saved_State->{"Wrappers"}{$_}})} = values %{$Saved_State->{"Wrappers"}{$_}};
        }
        foreach (keys(%{$Saved_State->{"Wrappers_SubClasses"}}))
        {
            @{$Wrappers_SubClasses{$_}}{keys(%{$Saved_State->{"Wrappers_SubClasses"}{$_}})} = values %{$Saved_State->{"Wrappers_SubClasses"}{$_}};
        }
        foreach (keys(%{$Saved_State->{"UsedConstructors"}}))
        {
            @{$UsedConstructors{$_}}{keys(%{$Saved_State->{"UsedConstructors"}{$_}})} = values %{$Saved_State->{"UsedConstructors"}{$_}};
        }
        foreach (keys(%{$Saved_State->{"UsedProtectedMethods"}}))
        {
            @{$UsedProtectedMethods{$_}}{keys(%{$Saved_State->{"UsedProtectedMethods"}{$_}})} = values %{$Saved_State->{"UsedProtectedMethods"}{$_}};
        }
        foreach (keys(%{$Saved_State->{"IntSpecType"}}))
        {
            @{$IntSpecType{$_}}{keys(%{$Saved_State->{"IntSpecType"}{$_}})} = values %{$Saved_State->{"IntSpecType"}{$_}};
        }
        foreach (keys(%{$Saved_State->{"RequirementsCatalog"}}))
        {
            @{$RequirementsCatalog{$_}}{keys(%{$Saved_State->{"RequirementsCatalog"}{$_}})} = values %{$Saved_State->{"RequirementsCatalog"}{$_}};
        }
        $MaxTypeId = $Saved_State->{"MaxTypeId"};
        @AuxType{keys(%{$Saved_State->{"AuxType"}})} = values %{$Saved_State->{"AuxType"}};
        @TraceFunc{keys(%{$Saved_State->{"TraceFunc"}})} = values %{$Saved_State->{"TraceFunc"}};
        @AuxFunc{keys(%{$Saved_State->{"AuxFunc"}})} = values %{$Saved_State->{"AuxFunc"}};
        @Create_SubClass{keys(%{$Saved_State->{"Create_SubClass"}})} = values %{$Saved_State->{"Create_SubClass"}};
        @SpecCode{keys(%{$Saved_State->{"SpecCode"}})} = values %{$Saved_State->{"SpecCode"}};
        @SpecLibs{keys(%{$Saved_State->{"SpecLibs"}})} = values %{$Saved_State->{"SpecLibs"}};
        @UsedInterfaces{keys(%{$Saved_State->{"UsedInterfaces"}})} = values %{$Saved_State->{"UsedInterfaces"}};
        @IntSpecType{keys(%{$Saved_State->{"IntSpecType"}})} = values %{$Saved_State->{"IntSpecType"}};
    }
}

sub isAbstractClass($)
{
    my $ClassId = $_[0];
    return (keys(%{$Class_PureVirtFunc{get_TypeName($ClassId)}}) > 0);
}

sub needToInherit($)
{
    my $Interface = $_[0];
    return ($CompleteSignature{$Interface}{"Class"} and (isAbstractClass($CompleteSignature{$Interface}{"Class"}) or isNotInCharge($Interface) or ($CompleteSignature{$Interface}{"Protected"})));
}

sub parseCode($$)
{
    my ($Code, $Mode) = @_;
    my $Global_State = save_state();
    my %ParsedCode = parseCode_m($Code, $Mode);
    if(not $ParsedCode{"IsCorrect"}) {
        restore_state($Global_State);
        return ();
    }
    else {
        return %ParsedCode;
    }
}

sub get_TypeIdByName($)
{
    my $TypeName = $_[0];
    if(my $ExactId = $TName_Tid{correctName($TypeName)}) {
        return $ExactId;
    }
    else {
        return $TName_Tid{remove_quals(correctName($TypeName))};
    }
}

sub callInterfaceParameters(@)
{
    my %Init_Desc = @_;
    my $Interface = $Init_Desc{"Interface"};
    return () if(not $Interface);
    return () if($SkipInterfaces{$Interface});
    foreach my $SkipPattern (keys(%SkipInterfaces_Pattern)) {
        return () if($Interface=~/$SkipPattern/);
    }
    if(defined $MakeIsolated and $Interface_Library{$Interface}
    and keys(%InterfacesList) and not $InterfacesList{$Interface}) {
        return ();
    }
    my $Global_State = save_state();
    return () if(isCyclical(\@RecurInterface, $Interface));
    push(@RecurInterface, $Interface);
    my $PreviousBlock = $CurrentBlock;
    if($CompleteSignature{$Interface}{"Protected"}
    and not $CompleteSignature{$Interface}{"Constructor"}) {
        $CurrentBlock = $Interface;
    }
    $NodeInterface = $Interface;
    $UsedInterfaces{$NodeInterface} = 1;
    my %Params_Init = callInterfaceParameters_m(%Init_Desc);
    $CurrentBlock = $PreviousBlock;
    if(not $Params_Init{"IsCorrect"})
    {
        pop(@RecurInterface);
        restore_state($Global_State);
        if($DebugMode) {
            $DebugInfo{"Init_InterfaceParams"}{$Interface} = 1;
        }
        return ();
    }
    pop(@RecurInterface);
    if($InterfaceSpecType{$Interface}{"SpecEnv"}) {
        $SpecEnv{$InterfaceSpecType{$Interface}{"SpecEnv"}} = 1;
    }
    $Params_Init{"ReturnTypeId"} = $CompleteSignature{$Interface}{"Return"};
    return %Params_Init;
}

sub detectInLineParams($)
{
    my $Interface = $_[0];
    my ($SpecAttributes, %Param_SpecAttributes, %InLineParam) = ();
    foreach my $Param_Pos (keys(%{$InterfaceSpecType{$Interface}{"SpecParam"}}))
    {
        my $SpecType_Id = $InterfaceSpecType{$Interface}{"SpecParam"}{$Param_Pos};
        my %SpecType = %{$SpecType{$SpecType_Id}};
        $Param_SpecAttributes{$Param_Pos} = $SpecType{"Value"}.$SpecType{"PreCondition"}.$SpecType{"PostCondition"}.$SpecType{"InitCode"}.$SpecType{"DeclCode"}.$SpecType{"FinalCode"};
        $SpecAttributes .= $Param_SpecAttributes{$Param_Pos};
    }
    foreach my $Param_Pos (sort {int($a)<=>int($b)} keys(%{$CompleteSignature{$Interface}{"Param"}}))
    {
        my $Param_Num = $Param_Pos + 1;
        if($SpecAttributes=~/\$$Param_Num(\W|\Z)/
        or $Param_SpecAttributes{$Param_Pos}=~/\$0(\W|\Z)/) {
            $InLineParam{$Param_Num} = 0;
        }
        else {
            $InLineParam{$Param_Num} = 1;
        }
    }
    return %InLineParam;
}

sub detectParamsOrder($)
{
    my $Interface = $_[0];
    my ($SpecAttributes, %OrderParam) = ();
    foreach my $Param_Pos (keys(%{$InterfaceSpecType{$Interface}{"SpecParam"}}))
    {#detect all dependencies
        my $SpecType_Id = $InterfaceSpecType{$Interface}{"SpecParam"}{$Param_Pos};
        my %SpecType = %{$SpecType{$SpecType_Id}};
        $SpecAttributes .= $SpecType{"Value"}.$SpecType{"PreCondition"}.$SpecType{"PostCondition"}.$SpecType{"InitCode"}.$SpecType{"DeclCode"}.$SpecType{"FinalCode"};
    }
    my $Orded = 1;
    foreach my $Param_Pos (sort {int($a)<=>int($b)} keys(%{$CompleteSignature{$Interface}{"Param"}}))
    {
        my $Param_Num = $Param_Pos + 1;
        if($SpecAttributes=~/\$$Param_Num(\W|\Z)/)
        {
            $OrderParam{$Param_Num} = $Orded;
            $Orded += 1;
        }
    }
    foreach my $Param_Pos (sort {int($a)<=>int($b)} keys(%{$CompleteSignature{$Interface}{"Param"}}))
    {
        my $Param_Num = $Param_Pos + 1;
        if(not defined $OrderParam{$Param_Pos+1})
        {
            $OrderParam{$Param_Num} = $Orded;
            $Orded += 1;
        }
    }
    return %OrderParam;
}

sub chooseSpecType($$$)
{
    my ($TypeId, $Kind, $Interface) = @_;
    if(my $SpecTypeId_Strong = chooseSpecType_Strong($TypeId, $Kind, $Interface, 1)) {
        return $SpecTypeId_Strong;
    }
    elsif(get_TypeType(get_FoundationTypeId($TypeId))!~/\A(Intrinsic)\Z/) {
        return chooseSpecType_Strong($TypeId, $Kind, $Interface, 0);
    }
    else {
        return "";
    }
}

sub chooseSpecType_Strong($$$$)
{
    my ($TypeId, $Kind, $Interface, $Strong) = @_;
    return 0 if(not $TypeId or not $Kind);
    foreach my $SpecType_Id (sort {int($a)<=>int($b)} keys(%SpecType))
    {
        next if($Interface and $Common_SpecType_Exceptions{$Interface}{$SpecType_Id});
        if($SpecType{$SpecType_Id}{"Kind"} eq $Kind)
        {
            if($Strong)
            {
                if($TypeId==get_TypeIdByName($SpecType{$SpecType_Id}{"DataType"})) {
                    return $SpecType_Id;
                }
            }
            else
            {
                my $FoundationTypeId = get_FoundationTypeId($TypeId);
                my $SpecType_FTypeId = get_FoundationTypeId(get_TypeIdByName($SpecType{$SpecType_Id}{"DataType"}));
                if($FoundationTypeId==$SpecType_FTypeId) {
                    return $SpecType_Id;
                }
            }
        }
    }
    return 0;
}

sub getAutoConstraint($)
{
    my $ReturnType_Id = $_[0];
    if(get_PointerLevel($Tid_TDid{$ReturnType_Id}, $ReturnType_Id) > 0) {
        return ("\$0 != ".get_null(), $ReturnType_Id);
    }
    else {
        return ();
    }
}

sub isValidForOutput($)
{
    my $TypeId = $_[0];
    my $PointerLevel = get_PointerLevel($Tid_TDid{$TypeId}, $TypeId);
    my $FoundationType_Id = get_FoundationTypeId($TypeId);
    my $FoundationType_Name = get_TypeName($FoundationType_Id);
    my $FoundationType_Type = get_TypeType($FoundationType_Id);
    if($PointerLevel eq 0)
    {
        if($FoundationType_Name eq "QString") {
            return 1;
        }
        elsif($FoundationType_Type eq "Enum"
        or $FoundationType_Type eq "Intrinsic") {
            return 1;
        }
        else {
            return 0;
        }
    }
    elsif($PointerLevel eq 1)
    {
        if($FoundationType_Name eq "char"
        or $FoundationType_Name eq "unsigned char") {
            return 1;
        }
        else {
            return 0;
        }
    }
    else {
        return 0;
    }
}

sub requirementReturn($$$$)
{
    my ($Interface, $Ireturn, $Ispecreturn, $CallObj) = @_;
    return "" if(defined $Template2Code and $Interface ne $TestedInterface);
    return "" if(not $Ireturn or not $Interface);
    my ($PostCondition, $TargetTypeId, $Requirement_Code) = ();
    if($Ispecreturn) {
        ($PostCondition, $TargetTypeId) = ($SpecType{$Ispecreturn}{"PostCondition"}, get_TypeIdByName($SpecType{$Ispecreturn}{"DataType"}));
    }
    elsif(defined $CheckReturn) {
        ($PostCondition, $TargetTypeId) = getAutoConstraint($Ireturn);
    }
    return "" if(not $PostCondition or not $TargetTypeId);
    my $IreturnTypeName = get_TypeName($Ireturn);
    my $BaseIreturnTypeName = get_TypeName(get_FoundationTypeId($Ireturn));
    my $PointerLevelReturn = get_PointerLevel($Tid_TDid{$Ireturn}, $Ireturn);
    my ($TargetCallReturn, $TmpPreamble) =
    convert_familiar_types((
        "InputTypeName"=>get_TypeName($Ireturn),
        "InputPointerLevel"=>$PointerLevelReturn,
        "OutputTypeId"=>$TargetTypeId,
        "Value"=>"\$0",
        "Key"=>"\$0",
        "Destination"=>"Target",
        "MustConvert"=>0));
    if($TmpPreamble) {
        $Requirement_Code .= $TmpPreamble."\n";
    }
    if($TargetCallReturn=~/\A\*/
    or $TargetCallReturn=~/\A\&/) {
        $TargetCallReturn = "(".$TargetCallReturn.")";
    }
    if($CallObj=~/\A\*/
    or $CallObj=~/\A\&/) {
        $CallObj = "(".$CallObj.")";
    }
    $PostCondition=~s/\$0/$TargetCallReturn/g;
    if($CallObj ne "no object") {
        $PostCondition=~s/\$obj/$CallObj/g;
    }
    $PostCondition = clearSyntax($PostCondition);
    my $NormalResult = $PostCondition;
    while($PostCondition=~s/([^\\])"/$1\\\"/g){}
    $ConstraintNum{$Interface}+=1;
    $RequirementsCatalog{$Interface}{$ConstraintNum{$Interface}} = "constraint for the return value: \'$PostCondition\'";
    my $ReqId = short_interface_name($Interface).".".normalize_num($ConstraintNum{$Interface});
    if(my $Format = is_printable(get_TypeName($TargetTypeId)))
    {
        my $Comment = "constraint for the return value failed: \'$PostCondition\', returned value: $Format";
        $Requirement_Code .= "REQva(\"$ReqId\",\n$NormalResult,\n\"$Comment\",\n$TargetCallReturn);\n";
        $TraceFunc{"REQva"}=1;
    }
    else
    {
        my $Comment = "constraint for the return value failed: \'$PostCondition\'";
        $Requirement_Code .= "REQ(\"$ReqId\",\n\"$Comment\",\n$NormalResult);\n";
        $TraceFunc{"REQ"}=1;
    }
    return $Requirement_Code;
}

sub is_printable($)
{
    my $TypeName = remove_quals(uncover_typedefs($_[0]));
    if(isIntegerType($TypeName)) {
        return "\%d";
    }
    elsif($TypeName=~/\A(char|unsigned char|wchar_t|void|short|unsigned short) const\*\Z/) {
        return "\%s";
    }
    elsif(isCharType($TypeName)) {
        return "\%c";
    }
    elsif($TypeName=~/\A(float|double|long double)\Z/) {
        return "\%f";
    }
    else {
        return "";
    }
}

sub short_interface_name($)
{
    my $Interface = $_[0];
    my $ClassName = get_TypeName($CompleteSignature{$Interface}{"Class"});
    return (($ClassName)?$ClassName."::":"").$CompleteSignature{$Interface}{"ShortName"};
}

sub normalize_num($)
{
    my $Num = $_[0];
    if(int($Num)<10) {
        return "0".$Num;
    }
    else {
        return $Num;
    }
}

sub get_PointerLevel($$)
{
    my ($TypeDId, $TypeId) = @_;
    return "" if(not $TypeId);
    if(defined $Cache{"get_PointerLevel"}{$TypeDId}{$TypeId}
    and not defined $AuxType{$TypeId}) {
        return $Cache{"get_PointerLevel"}{$TypeDId}{$TypeId};
    }
    return "" if(not $TypeDescr{$TypeDId}{$TypeId});
    my %Type = %{$TypeDescr{$TypeDId}{$TypeId}};
    return 0 if(not $Type{"BaseType"}{"TDid"} and not $Type{"BaseType"}{"Tid"});
    return 0 if($Type{"Type"} eq "Array");
    my $PointerLevel = 0;
    if($Type{"Type"} eq "Pointer") {
        $PointerLevel += 1;
    }
    $PointerLevel += get_PointerLevel($Type{"BaseType"}{"TDid"}, $Type{"BaseType"}{"Tid"});
    $Cache{"get_PointerLevel"}{$TypeDId}{$TypeId} = $PointerLevel;
    return $PointerLevel;
}

sub select_ValueFromCollection(@)
{
    my %Init_Desc = @_;
    my ($TypeId, $Name, $Interface, $CreateChild, $IsObj) = ($Init_Desc{"TypeId"}, $Init_Desc{"ParamName"}, $Init_Desc{"Interface"}, $Init_Desc{"CreateChild"}, $Init_Desc{"ObjectInit"});
    return () if($Init_Desc{"DoNotReuse"});
    my $TypeName = get_TypeName($TypeId);
    my $FTypeId = get_FoundationTypeId($TypeId);
    my $FTypeName = get_TypeName($FTypeId);
    my $PointerLevel = get_PointerLevel($Tid_TDid{$TypeId}, $TypeId);
    my $ShortName = $CompleteSignature{$Interface}{"ShortName"};
    my $IsRef = (uncover_typedefs(get_TypeName($TypeId))=~/&/);
    return () if(isString($TypeId, $Name, $Interface));
    return () if(uncover_typedefs($TypeName)=~/\A(char|unsigned char|wchar_t|void\*)\Z/);
    return () if(isCyclical(\@RecurTypeId, get_TypeStackId($TypeId)));
    if($CurrentBlock and keys(%{$ValueCollection{$CurrentBlock}}))
    {
        my (@Name_Type_Coinsidence, @Name_FType_Coinsidence, @Type_Coinsidence, @FType_Coinsidence) = ();
        foreach my $Value (sort {$b=~/$Name/i<=>$a=~/$Name/i} sort keys(%{$ValueCollection{$CurrentBlock}}))
        {
            return () if($Name=~/dest|source/i and $Value=~/source|dest/i and $ShortName=~/copy|move|backup/i);
            my $Value_TypeId = $ValueCollection{$CurrentBlock}{$Value};
            my $PointerLevel_Value = get_PointerLevel($Tid_TDid{$Value_TypeId}, $Value_TypeId);
            if($Value!~/\A(argc|argv)\Z/)
            {
                if(get_TypeName($Value_TypeId)=~/(string|date|time|file)/i and $Name!~/\Ap\d+\Z/)
                {# date, time arguments
                    unless($Name=~/_elem\Z/ and $PointerLevel_Value==0)
                    {# array elements may be reused
                        next;
                    }
                }
                next if($CreateChild and not $SubClass_Instance{$Value});
                #next if(not $IsObj and $SubClass_Instance{$Value});
                next if(($Interface eq $TestedInterface) and ($Name ne $Value)
                and not $UseVarEveryWhere{$CurrentBlock}{$Value});#and $Name!~/\Ap\d+\Z/
            }
            if($TypeName eq get_TypeName($Value_TypeId))
            {
                if($Value=~/\A(argc|argv)\Z/) {
                    next if($PointerLevel > $PointerLevel_Value);
                }
                else
                {
                    if(isNumericType($TypeName)
                    and $Name!~/\Q$Value\E/i and $TypeName!~/[A-Z]|_t/)
                    {# do not reuse intrinsic values
                        next;
                    }
                }
                if($Name=~/\A[_]*$Value(|[_]*[a-zA-Z0-9]|[_]*ptr)\Z/i) {
                    push(@Name_Type_Coinsidence, $Value);
                }
                else {
                    next if($Value=~/\A(argc|argv)\Z/ and $CurrentBlock eq "main");
                    push(@Type_Coinsidence, $Value);
                }
            }
            else
            {
                if($Value=~/\A(argc|argv)\Z/) {
                    next if($PointerLevel > $PointerLevel_Value);
                }
                else
                {
                    if(isNumericType($FTypeName) and $Name!~/\Q$Value\E/i)
                    {# do not reuse intrinsic values
                        next;
                    }
                    if($PointerLevel-$PointerLevel_Value!=1)
                    {
                        if($PointerLevel > $PointerLevel_Value) {
                            next;
                        }
                        elsif($PointerLevel ne $PointerLevel_Value)
                        {
                            if(get_TypeType($FTypeId)=~/\A(Intrinsic|Array|Enum)\Z/
                            or isArray($Value_TypeId, $Value, $Interface)) {
                                next;
                            }
                        }
                    }
                    if($PointerLevel<$PointerLevel_Value
                    and $Init_Desc{"OuterType_Type"} eq "Array") {
                        next;
                    }
                }
                my $Value_FTypeId = get_FoundationTypeId($Value_TypeId);
                if($FTypeName eq get_TypeName($Value_FTypeId))
                {
                    if($Name=~/\A[_]*\Q$Value\E(|[_]*[a-z0-9]|[_]*ptr)\Z/i) {
                        push(@Name_FType_Coinsidence, $Value);
                    }
                    else {
                        next if($Value=~/\A(argc|argv)\Z/ and $CurrentBlock eq "main");
                        push(@FType_Coinsidence, $Value);
                    }
                }
            }
        }
        my @All_Coinsidence = (@Name_Type_Coinsidence, @Name_FType_Coinsidence, @Type_Coinsidence, @FType_Coinsidence);
        if($#All_Coinsidence>-1) {
            return ($All_Coinsidence[0], $ValueCollection{$CurrentBlock}{$All_Coinsidence[0]});
        }
    }
    return ();
}

sub get_interface_param_pos($$)
{
    my ($Interface, $Name) = @_;
    foreach my $Pos (keys(%{$CompleteSignature{$Interface}{"Param"}}))
    {
        if($CompleteSignature{$Interface}{"Param"}{$Pos}{"name"} eq $Name)
        {
            return $Pos;
        }
    }
    return "";
}

sub hasLength($$)
{
    my ($ParamName, $Interface) = @_;
    my $ParamPos = get_interface_param_pos($Interface, $ParamName);
    if(defined $CompleteSignature{$Interface}{"Param"}{$ParamPos+1})
    {
      return (isIntegerType(get_TypeName($CompleteSignature{$Interface}{"Param"}{$ParamPos+1}{"type"}))
      and is_array_count($ParamName, $CompleteSignature{$Interface}{"Param"}{$ParamPos+1}{"name"}));
    }
    return 0;
}

sub isArrayName($)
{
    my $Name = $_[0];
    if($Name=~/([a-z][a-rt-z]s\Z|matrix|list|set|range|array)/i) {
        return 1;
    }
    return 0;
}

sub isArray($$$)
{# detecting parameter semantic
    my ($TypeId, $ParamName, $Interface) = @_;
    return 0 if(not $TypeId or not $ParamName);
    my $I_ShortName = $CompleteSignature{$Interface}{"ShortName"};
    my $FTypeId = get_FoundationTypeId($TypeId);
    my $FTypeType = get_TypeType($FTypeId);
    my $FTypeName = get_TypeName($FTypeId);
    my $TypeName = get_TypeName($TypeId);
    my $PLevel = get_PointerLevel($Tid_TDid{$TypeId}, $TypeId);
    my $ParamPos = get_interface_param_pos($Interface, $ParamName);
    
    return 1 if(get_TypeType($TypeId) eq "Array");
    
    # strong reject
    return 0 if($PLevel <= 0);
    return 0 if(isString($TypeId, $ParamName, $Interface));
    return 0 if($PLevel==1 and (isOpaque($FTypeId) or $FTypeName eq "void"));
    return 0 if($ParamName=~/ptr|pointer/i and $FTypeType=~/\A(Struct|Union|Class)\Z/);
    return 0 if($Interface_OutParam{$Interface}{$ParamName});
    
    # particular reject
    # FILE *fopen(const char *path, const char *__modes)
    return 0 if(is_const_type($TypeName) and isCharType($FTypeName)
    and $PLevel==1 and $ParamName=~/mode/i);
    
    # returned by function
    return 0 if(($FTypeType=~/\A(Struct|Union|Class)\Z/
    or ($TypeName ne uncover_typedefs($TypeName) and $TypeName!~/size_t|int/))
    and check_type_returned($TypeId, isArrayName($TypeName)));
    
    # array followed by the number
    return 1 if(not is_const_type($TypeName) and hasLength($ParamName, $Interface));
    
    return 0 if($PLevel>=2 and isCharType($FTypeName)
    and not is_const_type($TypeName));
    
    # allowed configurations
    # array of arguments
    return 1 if($ParamName=~/argv/i);
    # array, list, matrix
    if($ParamName!~/out|context|name/i and isArrayName($ParamName)
    and (getParamNameByTypeName($TypeName) ne $ParamName or $TypeName!~/\*/)
    and $TypeName!~/$ParamName/i)
    {#  foo(struct type* list)
     #! curl_slist_free_all ( struct curl_slist* p1 )
        return 1;
    }
    # array of function pointers
    return 1 if($PLevel==1 and $FTypeType=~/\A(FuncPtr|Array)\Z/);
    # QString::vsprintf ( char const* format, va_list ap )
    return 1 if($ParamName!~/out|context/i and isArrayName($TypeName) and $TypeName!~/$ParamName/i);
    # high pointer level
    # xmlSchemaSAXPlug (xmlSchemaValidCtxtPtr ctxt, xmlSAXHandlerPtr* sax, void** user_data)
    return 1 if($PLevel>=2);
    # symbol array for reading
    return 1 if($PLevel==1 and not is_const_type($TypeName) and isCharType($FTypeName)
    and not grep(/\A(name|cur|current|out|ret|return|buf|buffer|res|result|rslt)\Z/i, @{get_tokens($ParamName)}));
    # array followed by the two numbers
    return 1 if(not is_const_type($TypeName) and defined $CompleteSignature{$Interface}{"Param"}{$ParamPos+1}
    and defined $CompleteSignature{$Interface}{"Param"}{$ParamPos+2}
    and isIntegerType(get_TypeName($CompleteSignature{$Interface}{"Param"}{$ParamPos+1}{"type"}))
    and isIntegerType(get_TypeName($CompleteSignature{$Interface}{"Param"}{$ParamPos+2}{"type"}))
    and is_array_count($ParamName, $CompleteSignature{$Interface}{"Param"}{$ParamPos+2}{"name"}));
    # numeric arrays for reading
    return 1 if(is_const_type($TypeName) and isNumericType($FTypeName));
    # symbol buffer for reading
    return 1 if(is_const_type($TypeName) and $ParamName=~/buf/i and $I_ShortName=~/memory/i
    and isCharType($FTypeName));
    
    # isn't array
    return 0;
}

sub check_type_returned($$)
{
    my ($TypeId, $Strong) = @_;
    return 0 if(not $TypeId);
    my $BaseTypeId = get_FoundationTypeId($TypeId);
    if(get_TypeType($BaseTypeId) ne "Intrinsic") {
        # by return value
        return 1 if(keys(%{$ReturnTypeId_Interface{$TypeId}}) or keys(%{$ReturnTypeId_Interface{$BaseTypeId}}));
        if(not $Strong) {
            # base type and plevel match
            my $PLevel = get_PointerLevel($Tid_TDid{$TypeId}, $TypeId);
            foreach (0 .. $PLevel)
            {
                return 1 if(keys(%{$BaseType_PLevel_OutParam{$BaseTypeId}{$_}})
                or keys(%{$BaseType_PLevel_Return{$BaseTypeId}{$_}}));
            }
        }
        
    }
    return 0;
}

sub isBuffer($$$)
{
    my ($TypeId, $ParamName, $Interface) = @_;
    return 0 if(not $TypeId or not $ParamName);
    my $I_ShortName = $CompleteSignature{$Interface}{"ShortName"};
    my $FTypeId = get_FoundationTypeId($TypeId);
    my $FTypeType = get_TypeType($FTypeId);
    my $FTypeName = get_TypeName($FTypeId);
    my $TypeName = get_TypeName($TypeId);
    my $PLevel = get_PointerLevel($Tid_TDid{$TypeId}, $TypeId);
    
    # exceptions
    # bmp_read24 (uintptr_t addr)
    # bmp_write24 (uintptr_t addr, int c)
    return 1 if($PLevel==0 and $ParamName=~/addr/i and isIntegerType($FTypeName));
    # cblas_zdotu_sub (int const N, void const* X, int const incX, void const* Y, int const incY, void* dotu)
    return 1 if($PLevel==1 and $FTypeName eq "void");
    if(get_TypeType($FTypeId) eq "Array" and $Interface)
    {
        my $ArrayElemType_Id = get_FoundationTypeId(get_OneStep_BaseTypeId($Tid_TDid{$FTypeId}, $FTypeId));
        if(get_TypeType($ArrayElemType_Id)=~/\A(Intrinsic|Enum)\Z/)
        {
            return 1 if(get_TypeSize($FTypeId)>1024);
        }
        else
        {
            return 1 if(get_TypeSize($FTypeId)>256);
        }
    }
    
    # strong reject
    return 0 if($PLevel <= 0);
    return 0 if(is_const_type($TypeName));
    return 0 if(isString($TypeId, $ParamName, $Interface));
    return 0 if($PLevel==1 and isOpaque($FTypeId));
    return 0 if(($FTypeType=~/\A(Struct|Union|Class)\Z/
    or ($TypeName ne uncover_typedefs($TypeName) and $TypeName!~/size_t|int/))
    and check_type_returned($TypeId, isArrayName($TypeName)));
    
    # allowed configurations
    # symbol buffer for writing
    return 1 if(isSymbolBuffer($TypeId, $ParamName, $Interface));
    if($ParamName=~/\Ap\d+\Z/)
    {
        # buffer of void* type for writing
        return 1 if($PLevel==1 and $FTypeName eq "void");
        # buffer of arrays for writing
        return 1 if($FTypeType eq "Array");
    }
    return 1 if(is_out_word($ParamName));
    # gsl_fft_real_radix2_transform (double* data, size_t const stride, size_t const n)
    return 1 if($PLevel==1 and isNumericType($FTypeName) and $ParamName!~/(len|size)/i);
    
    # isn't array
    return 0;
}

sub is_out_word($)
{
    my $Word = $_[0];
    return grep(/\A(out|output|dest|buf|buff|buffer|ptr|pointer|result|res|ret|return|rtrn)\Z/i, @{get_tokens($Word)});
}

sub isSymbolBuffer($$$)
{
    my ($TypeId, $ParamName, $Interface) = @_;
    return 0 if(not $TypeId or not $ParamName);
    my $FTypeId = get_FoundationTypeId($TypeId);
    my $FTypeName = get_TypeName($FTypeId);
    my $TypeName = get_TypeName($TypeId);
    my $PLevel = get_PointerLevel($Tid_TDid{$TypeId}, $TypeId);
    return (not is_const_type($TypeName) and $PLevel==1
    and isCharType($FTypeName)
    and $ParamName!~/data|value|arg|var/i and $TypeName!~/list|va_/
    and (grep(/\A(name|cur|current)\Z/i, @{get_tokens($ParamName)}) or is_out_word($ParamName)));
}

sub isOutParam_NoUsing($$$)
{
    my ($TypeId, $ParamName, $Interface) = @_;
    return 0 if(not $TypeId or not $ParamName);
    my $Func_ShortName = $CompleteSignature{$Interface}{"ShortName"};
    my $FTypeId = get_FoundationTypeId($TypeId);
    my $FTypeName = get_TypeName($FTypeId);
    my $TypeName = get_TypeName($TypeId);
    my $PLevel = get_PointerLevel($Tid_TDid{$TypeId}, $TypeId);
    return 0 if($PLevel==1 and isOpaque($FTypeId)); # size of the structure/union is unknown
    return 0 if(is_const_type($TypeName) or $PLevel<=0);
    return 1 if(grep(/\A(err|error)(_|)(p|ptr|)\Z/i, @{get_tokens($ParamName." ".$TypeName)}) and $Func_ShortName!~/error/i);
    return 1 if(grep(/\A(out|ret|return)\Z/i, @{get_tokens($ParamName)}));
    return 1 if($PLevel>=2 and isCharType($FTypeName) and not is_const_type($TypeName));
    return 0;
}

sub isString($$$)
{
    my ($TypeId, $ParamName, $Interface) = @_;
    return 0 if(not $TypeId or not $ParamName);
    my $TypeName_Trivial = uncover_typedefs(get_TypeName($TypeId));
    my $PLevel = get_PointerLevel($Tid_TDid{$TypeId}, $TypeId);
    my $TypeName = get_TypeName($TypeId);
    my $FoundationTypeName = get_TypeName(get_FoundationTypeId($TypeId));
    # not a pointer
    return 0 if($ParamName=~/ptr|pointer/i);
    # standard string (std::string)
    return 1 if($PLevel==0 and $FoundationTypeName eq "std::basic_string<char>");
    if($FoundationTypeName=~/\A(char|unsigned char|wchar_t|short|unsigned short)\Z/)
    {
        # char const*, unsigned char const*, wchar_t const*
        # void const*, short const*, unsigned short const*
        # ChannelGroup::getName ( char* name, int namelen )
        return 1 if($PLevel==1 and is_const_type($TypeName_Trivial));
        if(not hasLength($ParamName, $Interface))
        {
            return 1 if($PLevel==1 and $CompleteSignature{$Interface}{"ShortName"}!~/get|encode/i
            and $ParamName=~/\A(file|)(_|)path\Z|description|label|name/i);
            # direct_trim ( char** s )
            return 1 if($PLevel>=1 and $ParamName=~/\A(s|str|string)\Z/i);
        }
    }
    
    # isn't a string
    return 0;
}

sub isOpaque($)
{
    my $TypeId = $_[0];
    return 0 if(not $TypeId);
    my %Type = get_Type($Tid_TDid{$TypeId}, $TypeId);
    return ($Type{"Type"}=~/\A(Struct|Union)\Z/ and not keys(%{$Type{"Memb"}}) and not $Type{"Memb"}{0}{"name"});
}

sub isStr_FileName($$$)
{#should be called after the "isString" function
    my ($ParamPos, $ParamName, $Interface_ShortName) = @_;
    return 0 if(not $ParamName);
    if($ParamName=~/ext/i)
    { # not an extension
        return 0;
    }
    if($ParamName=~/file|dtd/i
    and $ParamName!~/type|opt/i)
    { # any files, dtds
        return 1;
    }
    return 1 if(lc($ParamName) eq "fname");
    # files as buffers
    return 1 if($ParamName=~/buf/i and $Interface_ShortName!~/memory|write/i and $Interface_ShortName=~/file/i);
    # name of the file at the first parameter of read/write/open functions
    # return 1 if($ParamName=~/\A[_]*name\Z/i and $Interface_ShortName=~/read|write|open/i and $ParamPos=="0");
    # file path
    return 1 if($ParamName=~/path/i
    and $Interface_ShortName=~/open|create|file/i
    and $Interface_ShortName!~/(open|_)dir(_|\Z)/i);
    # path to the configs
    return 1 if($ParamName=~/path|cfgs/i and $Interface_ShortName=~/config/i);
    # parameter of the string constructor
    return 1 if($ParamName=~/src/i and $Interface_ShortName!~/string/i and $ParamPos=="0");
    # uri/url of the local files
    return 1 if($ParamName=~/uri|url/i and $Interface_ShortName!~/http|ftp/i);
    
    # isn't a file path
    return 0;
}

sub isStr_Dir($$)
{
    my ($ParamName, $Interface_ShortName) = @_;
    return 0 if(not $ParamName);
    return 1 if($ParamName=~/path/i
    and $Interface_ShortName=~/(open|_)dir(_|\Z)/i);
    return 1 if($ParamName=~/dir/i);
    
    # isn't a directory
    return 0;
}

sub equal_types($$)
{
    my ($Type1_Id, $Type2_Id) = @_;
    return (uncover_typedefs(get_TypeName($Type1_Id)) eq uncover_typedefs(get_TypeName($Type2_Id)));
}

sub reduce_pointer_level($)
{
    my $TypeId = $_[0];
    my %PureType = get_PureType($Tid_TDid{$TypeId}, $TypeId);
    my $BaseTypeId = get_OneStep_BaseTypeId($PureType{"TDid"}, $PureType{"Tid"});
    return ($BaseTypeId eq $TypeId)?"":$BaseTypeId;
}

sub reassemble_array($)
{
    my $TypeId = $_[0];
    return () if(not $TypeId);
    my $FoundationTypeId = get_FoundationTypeId($TypeId);
    if(get_TypeType($FoundationTypeId) eq "Array")
    {
        my ($BaseName, $Length) = (get_TypeName($FoundationTypeId), 1);
        while($BaseName=~s/\[(\d+)\]//) {
            $Length*=$1;
        }
        return ($BaseName, $Length);
    }
    else {
        return ();
    }
}

sub get_call_malloc($)
{
    my $TypeId = $_[0];
    return "" if(not $TypeId);
    my $FoundationTypeId = get_FoundationTypeId($TypeId);
    my $FoundationTypeName = get_TypeName($FoundationTypeId);
    my $PointerLevel = get_PointerLevel($Tid_TDid{$TypeId}, $TypeId);
    my $Conv = ($FoundationTypeName ne "void")?"(".get_TypeName($TypeId).") ":"";
    $Conv=~s/\&//g;
    my $BuffSize = 0;
    if(get_TypeType($FoundationTypeId) eq "Array")
    {
        my ($Array_BaseName, $Array_Length) = reassemble_array($TypeId);
        $Conv = "($Array_BaseName*)";
        $BuffSize = $Array_Length;
        $FoundationTypeName = $Array_BaseName;
        my %ArrayBase = get_BaseType($Tid_TDid{$TypeId}, $TypeId);
        $FoundationTypeId = $ArrayBase{"Tid"};
    }
    else {
        $BuffSize = $BUFF_SIZE;
    }
    my $MallocCall = "malloc";
    if($LibraryMallocFunc)
    {
        $MallocCall = $CompleteSignature{$LibraryMallocFunc}{"ShortName"};
        if(my $NS = $CompleteSignature{$LibraryMallocFunc}{"NameSpace"}) {
            $MallocCall = $NS."::".$MallocCall;
        }
    }
    if($FoundationTypeName eq "void") {
        return $Conv.$MallocCall."($BuffSize)";
    }
    else
    {
        if(isOpaque($FoundationTypeId))
        {# opaque buffers
            if(get_TypeType($FoundationTypeId) eq "Array") {
                $BuffSize*=$BUFF_SIZE;
            }
            else {
                $BuffSize*=4;
            }
            return $Conv.$MallocCall."($BuffSize)";
        }
        else
        {
            if($PointerLevel==1) {
                my $ReducedTypeId = reduce_pointer_level($TypeId);
                return $Conv.$MallocCall."(sizeof(".get_TypeName($ReducedTypeId).")".($BuffSize>1?"*$BuffSize":"").")";
            }
            else {
                return $Conv.$MallocCall."(sizeof($FoundationTypeName)".($BuffSize>1?"*$BuffSize":"").")";
            }
        }
    }
}

sub isKnownExt($)
{
    my $Ext = $_[0];
    if($Ext=~/\A(png|tiff|zip|bmp|bitmap|nc)/i)
    {
        return $1;
    }
    return "";
}

sub add_VirtualSpecType(@)
{
    my %Init_Desc = @_;
    my %NewInit_Desc = %Init_Desc;
    if($Init_Desc{"Value"} eq "") {
        $Init_Desc{"Value"} = "no value";
    }
    my ($TypeId, $ParamName, $Interface) = ($Init_Desc{"TypeId"}, $Init_Desc{"ParamName"}, $Init_Desc{"Interface"});
    my $FoundationTypeId = get_FoundationTypeId($TypeId);
    my $FoundationTypeName = get_TypeName($FoundationTypeId);
    my $PointerLevel = get_PointerLevel($Tid_TDid{$TypeId}, $TypeId);
    my $FoundationTypeType = $TypeDescr{$Tid_TDid{$FoundationTypeId}}{$FoundationTypeId}{"Type"};
    my $TypeName = get_TypeName($TypeId);
    my $TypeType = get_TypeType($TypeId);
    my $I_ShortName = $CompleteSignature{$Init_Desc{"Interface"}}{"ShortName"};
    my $I_Header = $CompleteSignature{$Init_Desc{"Interface"}}{"Header"};
    my $BlockInterface_ShortName = $CompleteSignature{$CurrentBlock}{"ShortName"};
    if($Init_Desc{"Value"} eq "no value"
    or (defined $ValueCollection{$CurrentBlock}{$ParamName} and $ValueCollection{$CurrentBlock}{$ParamName}==$TypeId))
    {# create value atribute
        if($CurrentBlock and keys(%{$ValueCollection{$CurrentBlock}}) and not $Init_Desc{"InLineArray"})
        {
            ($NewInit_Desc{"Value"}, $NewInit_Desc{"ValueTypeId"}) = select_ValueFromCollection(%Init_Desc);
            if($NewInit_Desc{"Value"} and $NewInit_Desc{"ValueTypeId"})
            {
                my ($Call, $TmpPreamble)=convert_familiar_types((
                    "InputTypeName"=>get_TypeName($NewInit_Desc{"ValueTypeId"}),
                    "InputPointerLevel"=>get_PointerLevel($Tid_TDid{$NewInit_Desc{"ValueTypeId"}}, $NewInit_Desc{"ValueTypeId"}),
                    "OutputTypeId"=>($Init_Desc{"TypeId_Changed"})?$Init_Desc{"TypeId_Changed"}:$TypeId,
                    "Value"=>$NewInit_Desc{"Value"},
                    "Key"=>$LongVarNames?$Init_Desc{"Key"}:$ParamName,
                    "Destination"=>"Param",
                    "MustConvert"=>0));
                if($Call and not $TmpPreamble)
                {#try to create simple value
                    $NewInit_Desc{"ValueTypeId"}=$TypeId;
                    $NewInit_Desc{"Value"} = $Call;
                }
                if($NewInit_Desc{"ValueTypeId"}==$TypeId) {
                    $NewInit_Desc{"InLine"} = 1;
                }
                $NewInit_Desc{"Reuse"} = 1;
                return %NewInit_Desc;
            }
        }
        if($TypeName=~/\&/
        or not $Init_Desc{"InLine"}) {
            $NewInit_Desc{"InLine"} = 0;
        }
        else {
            $NewInit_Desc{"InLine"} = 1;
        }
        # creating virtual specialized type
        if($TypeName eq "...")
        {
            $NewInit_Desc{"Value"} = get_null();
            $NewInit_Desc{"ValueTypeId"} = get_TypeIdByName("int");
        }
        elsif($I_ShortName eq "time" and $I_Header eq "time.h")
        {# spectype for time_t time(time_t *t) from time.h
            $NewInit_Desc{"Value"} = get_null();
            $NewInit_Desc{"ValueTypeId"} = $TypeId;
        }
        elsif($ParamName=~/unused/i and $PointerLevel>=1)
        {# curl_getdate ( char const* p, time_t const* unused )
            $NewInit_Desc{"Value"} = get_null();
            $NewInit_Desc{"ValueTypeId"} = $TypeId;
        }
        elsif($FoundationTypeName eq "int" and $ParamName=~/\Aargc(_|)(p|ptr|)\Z/i
        and not $Interface_OutParam{$Interface}{$ParamName} and $PointerLevel>=1
        and my $Value_TId = register_new_type(get_TypeIdByName("int"), 1))
        {# gtk_init ( int* argc, char*** argv )
            $NewInit_Desc{"Value"} = "&argc";
            $NewInit_Desc{"ValueTypeId"} = $Value_TId;
        }
        elsif($FoundationTypeName eq "char" and $ParamName=~/\Aargv(_|)(p|ptr|)\Z/i
        and not $Interface_OutParam{$Interface}{$ParamName} and $PointerLevel>=3
        and my $Value_TId = register_new_type(get_TypeIdByName("char"), 3))
        {# gtk_init ( int* argc, char*** argv )
            $NewInit_Desc{"Value"} = "&argv";
            $NewInit_Desc{"ValueTypeId"} = $Value_TId;
        }
        elsif($FoundationTypeName eq "complex float")
        {
            $NewInit_Desc{"Value"} = getIntrinsicValue("float")." + I*".getIntrinsicValue("float");
            $NewInit_Desc{"ValueTypeId"} = $FoundationTypeId;
        }
        elsif($FoundationTypeName eq "complex double")
        {
            $NewInit_Desc{"Value"} = getIntrinsicValue("double")." + I*".getIntrinsicValue("double");
            $NewInit_Desc{"ValueTypeId"} = $FoundationTypeId;
        }
        elsif($FoundationTypeName eq "complex long double")
        {
            $NewInit_Desc{"Value"} = getIntrinsicValue("long double")." + I*".getIntrinsicValue("long double");
            $NewInit_Desc{"ValueTypeId"} = $FoundationTypeId;
        }
        elsif((($Interface_OutParam{$Interface}{$ParamName} and $PointerLevel>=1) or ($Interface_OutParam_NoUsing{$Interface}{$ParamName}
        and $PointerLevel>=1)) and not grep(/\A(in|input)\Z/, @{get_tokens($ParamName)}) and not isSymbolBuffer($TypeId, $ParamName, $Interface))
        {
            $NewInit_Desc{"InLine"} = 0;
            $NewInit_Desc{"ValueTypeId"} = reduce_pointer_level($TypeId);
            if($PointerLevel>=2) {
                $NewInit_Desc{"Value"} = get_null();
            }
            elsif($PointerLevel==1 and isNumericType(get_TypeName($FoundationTypeId)))
            {
                $NewInit_Desc{"Value"} = "0";
                $NewInit_Desc{"OnlyByValue"} = 1;
            }
            else {
                $NewInit_Desc{"OnlyDecl"} = 1;
            }
            $NewInit_Desc{"UseableValue"} = 1;
        }
        elsif($FoundationTypeName eq "void" and $PointerLevel==1
        and my $SimilarType_Id = find_similar_type($NewInit_Desc{"TypeId"}, $ParamName)
        and $TypeName=~/(\W|\A)void(\W|\Z)/ and not $NewInit_Desc{"TypeId_Changed"})
        {
            $NewInit_Desc{"TypeId"} = $SimilarType_Id;
            $NewInit_Desc{"DenyMalloc"} = 1;
            %NewInit_Desc = add_VirtualSpecType(%NewInit_Desc);
            $NewInit_Desc{"TypeId_Changed"} = $TypeId;
        }
        elsif(isArray($TypeId, $ParamName, $Interface)
        and not $Init_Desc{"IsString"})
        {
            $NewInit_Desc{"FoundationType_Type"} = "Array";
            if($ParamName=~/matrix/) {
                $NewInit_Desc{"ArraySize"} = 16;
            }
            $NewInit_Desc{"TypeType_Changed"} = 1;
        }
        elsif($Init_Desc{"FuncPtrName"}=~/realloc/i and $PointerLevel==1
        and $Init_Desc{"RetVal"} and $Init_Desc{"FuncPtrTypeId"})
        {
            my %FuncPtrType = get_Type($Tid_TDid{$Init_Desc{"FuncPtrTypeId"}}, $Init_Desc{"FuncPtrTypeId"});
            my ($IntParam, $IntParam2, $PtrParam, $PtrTypeId) = ("", "", "", 0);
            foreach my $ParamPos (sort {int($a) <=> int($b)} keys(%{$FuncPtrType{"Memb"}}))
            {
                my $ParamTypeId = $FuncPtrType{"Memb"}{$ParamPos}{"type"};
                my $ParamName = $FuncPtrType{"Memb"}{$ParamPos}{"name"};
                $ParamName = "p".($ParamPos+1) if(not $ParamName);
                my $ParamFTypeId = get_FoundationTypeId($ParamTypeId);
                if(isIntegerType(get_TypeName($ParamTypeId)))
                {
                    if(not $IntParam) {
                        $IntParam = $ParamName;
                    }
                    elsif(not $IntParam2) {
                        $IntParam2 = $ParamName;
                    }
                }
                elsif(get_PointerLevel($Tid_TDid{$ParamTypeId}, $ParamTypeId)==1
                and get_TypeType($ParamFTypeId) eq "Intrinsic")
                {
                    $PtrParam = $ParamName;
                    $PtrTypeId = $ParamTypeId;
                }
            }
            if($IntParam and $PtrParam)
            {# function has an integer parameter
                my $Conv = ($FoundationTypeName ne "void")?"(".get_TypeName($TypeId).") ":"";
                $Conv=~s/\&//g;
                my $VoidConv = (get_TypeName(get_FoundationTypeId($PtrTypeId)) ne "void")?"(void*)":"";
                if($IntParam2) {
                    $NewInit_Desc{"Value"} = $Conv."realloc($VoidConv$PtrParam, $IntParam2)";
                }
                else {
                    $NewInit_Desc{"Value"} = $Conv."realloc($VoidConv$PtrParam, $IntParam)";
                }
            }
            else {
                $NewInit_Desc{"Value"} = get_call_malloc($TypeId);
            }
            $NewInit_Desc{"ValueTypeId"} = $TypeId;
            $NewInit_Desc{"InLine"} = ($Init_Desc{"RetVal"} or ($Init_Desc{"OuterType_Type"} eq "Array"))?1:0;
            if($LibraryMallocFunc and (not $IntParam or not $PtrParam)) {
                $NewInit_Desc{"Headers"} = addHeaders([$CompleteSignature{$LibraryMallocFunc}{"Header"}], $NewInit_Desc{"Headers"});
            }
            else {
                $NewInit_Desc{"Headers"} = addHeaders(["stdlib.h"], $NewInit_Desc{"Headers"});
                $AuxHeaders{"stdlib.h"} = 1;
            }
        }
        elsif($Init_Desc{"FuncPtrName"}=~/alloc/i and $PointerLevel==1
        and $Init_Desc{"RetVal"} and $Init_Desc{"FuncPtrTypeId"})
        {
            my %FuncPtrType = get_Type($Tid_TDid{$Init_Desc{"FuncPtrTypeId"}}, $Init_Desc{"FuncPtrTypeId"});
            my $IntParam = "";
            foreach my $ParamPos (sort {int($a) <=> int($b)} keys(%{$FuncPtrType{"Memb"}}))
            {
                my $ParamTypeId = $FuncPtrType{"Memb"}{$ParamPos}{"type"};
                my $ParamName = $FuncPtrType{"Memb"}{$ParamPos}{"name"};
                $ParamName = "p".($ParamPos+1) if(not $ParamName);
                if(isIntegerType(get_TypeName($ParamTypeId)))
                {
                    $IntParam = $ParamName;
                    last;
                }
            }
            if($IntParam)
            {# function has an integer parameter
                my $Conv = ($FoundationTypeName ne "void")?"(".get_TypeName($TypeId).") ":"";
                $Conv=~s/\&//g;
                $NewInit_Desc{"Value"} = $Conv."malloc($IntParam)";
            }
            else {
                $NewInit_Desc{"Value"} = get_call_malloc($TypeId);
            }
            $NewInit_Desc{"ValueTypeId"} = $TypeId;
            $NewInit_Desc{"InLine"} = ($Init_Desc{"RetVal"} or ($Init_Desc{"OuterType_Type"} eq "Array"))?1:0;
            if($LibraryMallocFunc and not $IntParam) {
                $NewInit_Desc{"Headers"} = addHeaders([$CompleteSignature{$LibraryMallocFunc}{"Header"}], $NewInit_Desc{"Headers"});
            }
            else {
                $NewInit_Desc{"Headers"} = addHeaders(["stdlib.h"], $NewInit_Desc{"Headers"});
                $AuxHeaders{"stdlib.h"} = 1;
            }
        }
        elsif((isBuffer($TypeId, $ParamName, $Interface)
        or ($PointerLevel==1 and $I_ShortName=~/free/i and $FoundationTypeName=~/\A(void|char|unsigned char|wchar_t)\Z/))
        and not $NewInit_Desc{"InLineArray"} and not $Init_Desc{"IsString"} and not $Init_Desc{"DenyMalloc"})
        {
            if(get_TypeName($TypeId) eq "char const*"
            and (my $NewTypeId = get_TypeIdByName("char*"))) {
                $TypeId = $NewTypeId;
            }
            $NewInit_Desc{"Value"} = get_call_malloc($TypeId);
            $NewInit_Desc{"ValueTypeId"} = $TypeId;
            $NewInit_Desc{"InLine"} = ($Init_Desc{"RetVal"} or ($Init_Desc{"OuterType_Type"} eq "Array"))?1:0;
            if($LibraryMallocFunc) {
                $NewInit_Desc{"Headers"} = addHeaders([$CompleteSignature{$LibraryMallocFunc}{"Header"}], $NewInit_Desc{"Headers"});
            }
            else {
                $NewInit_Desc{"Headers"} = addHeaders(["stdlib.h"], $NewInit_Desc{"Headers"});
                $AuxHeaders{"stdlib.h"} = 1;
            }
        }
        elsif(isString($TypeId, $ParamName, $Interface)
        or $Init_Desc{"IsString"})
        {
            my @Values = ();
            if($ParamName and $ParamName!~/\Ap\d+\Z/)
            {
                if($I_ShortName=~/Display/ and $ParamName=~/name|display/i)
                {
                    @Values = ("getenv(\"DISPLAY\")");
                    $NewInit_Desc{"Headers"} = addHeaders(["stdlib.h"], $NewInit_Desc{"Headers"});
                    $AuxHeaders{"stdlib.h"} = 1;
                }
                elsif($ParamName=~/uri|url|href/i
                and $I_ShortName!~/file/i) {
                    @Values = ("\"http://ispras.linuxfoundation.org\"", "\"http://www.w3.org/\"");
                }
                elsif($ParamName=~/language/i) {
                    @Values = ("\"$COMMON_LANGUAGE\"");
                }
                elsif($ParamName=~/mount/i and $ParamName=~/path/i) {
                    @Values = ("\"/dev\"");
                }
                elsif(isStr_FileName($Init_Desc{"ParamPos"}, $ParamName, $I_ShortName))
                {
                    if($I_ShortName=~/sqlite/i) {
                        @Values = ("TG_TEST_DATA_DB");
                    }
                    elsif($TestedInterface=~/\A(ov_|vorbis_)/i) {
                        @Values = ("TG_TEST_DATA_AUDIO");
                    }
                    elsif($TestedInterface=~/\A(zip_)/i) {
                        @Values = ("TG_TEST_DATA_ZIP_FILE");
                    }
                    elsif($ParamName=~/dtd/i or $I_ShortName=~/dtd/i) {
                        @Values = ("TG_TEST_DATA_DTD_FILE");
                    }
                    elsif($ParamName=~/xml/i or $I_ShortName=~/xml/i
                    or ($Init_Desc{"OuterType_Type"}=~/\A(Struct|Union)\Z/ and get_TypeName($Init_Desc{"OuterType_Id"})=~/xml/i))
                    {
                        @Values = ("TG_TEST_DATA_XML_FILE");
                    }
                    elsif($ParamName=~/html/i or $I_ShortName=~/html/i
                    or ($Init_Desc{"OuterType_Type"}=~/\A(Struct|Union)\Z/ and get_TypeName($Init_Desc{"OuterType_Id"})=~/html/i))
                    {
                        @Values = ("TG_TEST_DATA_HTML_FILE");
                    }
                    elsif($ParamName=~/path/i and $I_ShortName=~/\Asnd_/)
                    {# ALSA
                        @Values = ("TG_TEST_DATA_ASOUNDRC_FILE");
                    }
                    else
                    {
                        my $KnownExt = isKnownExt(getPrefix($I_ShortName));
                        $KnownExt = isKnownExt($Init_Desc{"Key"}) if(not $KnownExt);
                        $KnownExt = isKnownExt($TestedInterface) if(not $KnownExt);
                        $KnownExt = isKnownExt($I_ShortName) if(not $KnownExt);
                        if($KnownExt) {
                            @Values = ("TG_TEST_DATA_FILE_".uc($KnownExt));
                        }
                        else {
                            @Values = ("TG_TEST_DATA_PLAIN_FILE");
                        }
                    }
                }
                elsif(isStr_Dir($ParamName, $I_ShortName)
                or ($ParamName=~/path/ and get_TypeName($Init_Desc{"OuterType_Id"})=~/Dir|directory/))
                {
                    @Values = ("TG_TEST_DATA_DIRECTORY");
                }
                elsif($ParamName=~/path/i and $I_ShortName=~/\Adbus_/)
                {# D-Bus
                    @Values = ("TG_TEST_DATA_ABS_FILE");
                }
                elsif($ParamName=~/path/i) {
                    @Values = ("TG_TEST_DATA_PLAIN_FILE");
                }
                elsif($ParamName=~/\A(ext|extension(s|))\Z/i) {
                    @Values = ("\".txt\"", "\".$LIB_EXT\"");
                }
                elsif($ParamName=~/mode/i and $I_ShortName=~/fopen/i)
                {# FILE *fopen(const char *path, const char *mode)
                    @Values = ("\"r+\"");
                }
                elsif($ParamName=~/mode/i and $I_ShortName=~/open/i) {
                    @Values = ("\"rw\"");
                }
                elsif($ParamName=~/date/i) {
                    @Values = ("\"Sun, 06 Nov 1994 08:49:37 GMT\"");
                }
                elsif($ParamName=~/day/i) {
                    @Values = ("\"monday\"", "\"tuesday\"");
                }
                elsif($ParamName=~/month/i) {
                    @Values = ("\"november\"", "\"october\"");
                }
                elsif($ParamName=~/name/i and $I_ShortName=~/font/i)
                {
                    if($I_ShortName=~/\A[_]*X/) {
                        @Values = ("\"10x20\"", "\"fixed\"");
                    }
                    else {
                        @Values = ("\"times\"", "\"arial\"", "\"courier\"");
                    }
                }
                elsif($ParamName=~/version/i) {
                    @Values = ("\"1.0\"", "\"2.0\"");
                }
                elsif($ParamName=~/encoding/i
                or $Init_Desc{"Key"}=~/encoding/i) {
                    @Values = ("\"utf-8\"", "\"koi-8\"");
                }
                elsif($ParamName=~/method/i
                and $I_ShortName=~/http|ftp|url|uri|request/i) {
                    @Values = ("\"GET\"", "\"PUT\"");
                }
                elsif($I_ShortName=~/cast/i
                and $CompleteSignature{$Interface}{"Class"}) {
                    @Values = ("\"".get_TypeName($CompleteSignature{$Interface}{"Class"})."\"");
                }
                elsif($I_ShortName=~/\Asnd_/ and $I_ShortName!~/\Asnd_seq_/ and $ParamName=~/name/i) {
                    @Values = ("\"hw:0\"");# ALSA
                }
                elsif($ParamName=~/var/i and $I_ShortName=~/env/i) {
                    @Values = ("\"HOME\"", "\"PATH\"");
                }
                elsif($ParamName=~/error_name/i and $I_ShortName=~/\Adbus_/) {# D-Bus
                    if($Constants{"DBUS_ERROR_FAILED"}{"Value"}) {
                        @Values = ("DBUS_ERROR_FAILED");
                    }
                    else {
                        @Values = ("\"org.freedesktop.DBus.Error.Failed\"");
                    }
                }
                elsif($ParamName=~/name/i and $I_ShortName=~/\Adbus_/) {# D-Bus
                    @Values = ("\"sample.bus\"");
                }
                elsif($ParamName=~/interface/i and $I_ShortName=~/\Adbus_/) {
                    @Values = ("\"sample.interface\"");# D-Bus
                }
                elsif($ParamName=~/address/i and $I_ShortName=~/\Adbus_server/) {
                    @Values = ("\"unix:tmpdir=/tmp\"");# D-Bus
                }
                elsif($CompleteSignature{$Interface}{"Constructor"} and not $Init_Desc{"ParamRenamed"})
                {
                    my $KeyPart = $Init_Desc{"Key"};
                    my $IgnoreSiffix = lc($I_ShortName)."_".$ParamName;
                    $KeyPart=~s/_\Q$ParamName\E\Z// if($I_ShortName=~/string|char/i and $KeyPart!~/(\A|_)\Q$IgnoreSiffix\E\Z/);
                    $KeyPart=~s/_\d+\Z//g;
                    $KeyPart=~s/\A.*_([^_]+)\Z/$1/g;
                    if($KeyPart!~/(\A|_)p\d+\Z/)
                    {
                        $NewInit_Desc{"ParamName"} = $KeyPart;
                        $NewInit_Desc{"ParamRenamed"} = 1;
                        %NewInit_Desc = add_VirtualSpecType(%NewInit_Desc);
                    }
                    else {
                        @Values = ("\"".$ParamName."\"");
                    }
                }
                else {
                    @Values = ("\"".$ParamName."\"");
                }
            }
            else
            {
                if($I_ShortName=~/Display/)
                {
                    @Values = ("getenv(\"DISPLAY\")");
                    $NewInit_Desc{"Headers"} = addHeaders(["stdlib.h"], $NewInit_Desc{"Headers"});
                    $AuxHeaders{"stdlib.h"} = 1;
                }
                elsif($I_ShortName=~/cast/ and $CompleteSignature{$Interface}{"Class"}) {
                    @Values = ("\"".get_TypeName($CompleteSignature{$Interface}{"Class"})."\"");
                }
                else {
                    @Values = (getIntrinsicValue("char*"));
                }
            }
            if($FoundationTypeName eq "wchar_t")
            {
                foreach my $Str (@Values) {
                    $Str = "L".$Str if($Str=~/\A\"/);
                }
                $NewInit_Desc{"ValueTypeId"} = get_TypeIdByName("wchar_t const*");
            }
            elsif($FoundationTypeType eq "Intrinsic") {
                $NewInit_Desc{"ValueTypeId"} = get_TypeIdByName("char const*");
            }
            else
            {# std::string
                $NewInit_Desc{"ValueTypeId"} = $FoundationTypeId;
            }
            $NewInit_Desc{"Value"} = vary_values(\@Values, \%Init_Desc) if($#Values>=0);
            if(not is_const_type(uncover_typedefs(get_TypeName($TypeId))) and not $Init_Desc{"IsString"})
            {# FIXME: inlining strings
                #$NewInit_Desc{"InLine"} = 0;
            }
        }
        elsif(($FoundationTypeName eq "void") and ($PointerLevel==1))
        {
            $NewInit_Desc{"FoundationType_Type"} = "Array";
            $NewInit_Desc{"TypeType_Changed"} = 1;
            $NewInit_Desc{"TypeId"} = get_TypeIdByName("char*");
            $NewInit_Desc{"TypeId_Changed"} = $TypeId;
        }
        elsif($FoundationTypeType eq "Intrinsic")
        {
            if($PointerLevel==1 and $ParamName=~/matrix/i)
            {
                $NewInit_Desc{"FoundationType_Type"} = "Array";
                $NewInit_Desc{"TypeType_Changed"} = 1;
                $NewInit_Desc{"ArraySize"} = 16;
            }
            elsif(isIntegerType($FoundationTypeName))
            {
                if($PointerLevel==0)
                {
                    if($Init_Desc{"RetVal"}
                    and $CurrentBlock=~/read/i) {
                        $NewInit_Desc{"Value"} = "0";
                    }
                    elsif($Init_Desc{"RetVal"}
                    and $TypeName=~/err/i) {
                        $NewInit_Desc{"Value"} = "1";
                    }
                    elsif($ParamName=~/socket|block/i) {
                        $NewInit_Desc{"Value"} = vary_values(["0"], \%Init_Desc);
                    }
                    elsif($ParamName=~/freq/i) {
                        $NewInit_Desc{"Value"} = vary_values(["50"], \%Init_Desc);
                    }
                    elsif(lc($ParamName) eq "id") {
                        $NewInit_Desc{"Value"} = "0";
                    }
                    elsif($ParamName=~/verbose/i) {
                        $NewInit_Desc{"Value"} = vary_values(["0", "1"], \%Init_Desc);
                    }
                    elsif($ParamName=~/year/i
                    or ($ParamName eq "y" and $I_ShortName=~/date/i)) {
                        $NewInit_Desc{"Value"} = vary_values(["2009", "2010"], \%Init_Desc);
                    }
                    elsif($ParamName eq "sa_family"
                    and get_TypeName($Init_Desc{"OuterType_Id"}) eq "struct sockaddr") {
                        $NewInit_Desc{"Value"} = vary_values(["AF_INET", "AF_INET6"], \%Init_Desc);
                    }
                    elsif($ParamName=~/day/i or ($ParamName eq "d" and $I_ShortName=~/date/i)) {
                        $NewInit_Desc{"Value"} = vary_values(["30", "13"], \%Init_Desc);
                    }
                    elsif($ParamName=~/month/i
                    or ($ParamName eq "m" and $I_ShortName=~/date/i)) {
                        $NewInit_Desc{"Value"} = vary_values(["11", "10"], \%Init_Desc);
                    }
                    elsif($ParamName=~/\Ac\Z/i and $I_ShortName=~/char/i) {
                        $NewInit_Desc{"Value"} = vary_values([get_CharNum()], \%Init_Desc);
                    }
                    elsif($ParamName=~/n_param_values/i) {
                        $NewInit_Desc{"Value"} = vary_values(["2"], \%Init_Desc);
                    }
                    elsif($ParamName=~/debug/i) {
                        $NewInit_Desc{"Value"} = vary_values(["0", "1"], \%Init_Desc);
                    }
                    elsif($ParamName=~/hook/i)
                    {
                        $NewInit_Desc{"Value"} = vary_values(["128"], \%Init_Desc);
                    }
                    elsif($ParamName=~/size|len|count/i
                    and $I_ShortName=~/char|string/i) {
                        $NewInit_Desc{"Value"} = vary_values(["7"], \%Init_Desc);
                    }
                    elsif($ParamName=~/size|len|capacity|count|max|(\A(n|l|s|c)_)/i) {
                        $NewInit_Desc{"Value"} = vary_values([$DEFAULT_ARRAY_AMOUNT], \%Init_Desc);
                    }
                    elsif($ParamName=~/time/i and $ParamName=~/req/i) {
                        $NewInit_Desc{"Value"} = vary_values([$HANGED_EXECUTION_TIME], \%Init_Desc);
                    }
                    elsif($ParamName=~/time/i
                    or ($ParamName=~/len/i and $ParamName!~/error/i)) {
                        $NewInit_Desc{"Value"} = vary_values(["1", "0"], \%Init_Desc);
                    }
                    elsif($ParamName=~/depth/i) {
                        $NewInit_Desc{"Value"} = vary_values(["1"], \%Init_Desc);
                    }
                    elsif($ParamName=~/delay/i) {
                        $NewInit_Desc{"Value"} = vary_values(["0", "1"], \%Init_Desc);
                    }
                    elsif($TypeName=~/(count|size)_t/i
                    and $ParamName=~/items/) {
                        $NewInit_Desc{"Value"} = vary_values([$DEFAULT_ARRAY_AMOUNT], \%Init_Desc);
                    }
                    elsif($ParamName=~/exists|start/i) {
                        $NewInit_Desc{"Value"} = vary_values(["0", "1"], \%Init_Desc);
                    }
                    elsif($ParamName=~/make/i) {
                        $NewInit_Desc{"Value"} = vary_values(["1", "0"], \%Init_Desc);
                    }
                    elsif($ParamName=~/\A(n|l|s|c)[0-9_]*\Z/i
                    # gsl_vector_complex_float_alloc (size_t const n)
                    # gsl_matrix_complex_float_alloc (size_t const n1, size_t const n2)
                    or (is_alloc_func($I_ShortName) and $ParamName=~/(num|len)[0-9_]*/i))
                    {
                        if($I_ShortName=~/column/) {
                            $NewInit_Desc{"Value"} = vary_values(["0"], \%Init_Desc);
                        }
                        else {
                            $NewInit_Desc{"Value"} = vary_values([$DEFAULT_ARRAY_AMOUNT], \%Init_Desc);
                        }
                    }
                    elsif($Init_Desc{"OuterType_Type"} eq "Array"
                    and $Init_Desc{"Index"} ne "") {
                        $NewInit_Desc{"Value"} = vary_values([$Init_Desc{"Index"}], \%Init_Desc);
                    }
                    elsif(($ParamName=~/index|from|pos|field|line|column|row/i and $ParamName!~/[a-z][a-rt-z]s\Z/i)
                    or $ParamName=~/\A(i|j|k|icol)\Z/i)
                    # gsl_vector_complex_float_get (gsl_vector_complex_float const* v, size_t const i)
                    {
                        if($Init_Desc{"OuterType_Type"} eq "Array") {
                            $NewInit_Desc{"Value"} = vary_values([$Init_Desc{"Index"}], \%Init_Desc);
                        }
                        else {
                            $NewInit_Desc{"Value"} = vary_values(["0"], \%Init_Desc);
                        }
                    }
                    elsif($TypeName=~/bool/i) {
                        $NewInit_Desc{"Value"} = vary_values(["1", "0"], \%Init_Desc);
                    }
                    elsif($ParamName=~/with/i) {
                        $NewInit_Desc{"Value"} = vary_values(["1", "0"], \%Init_Desc);
                    }
                    elsif($ParamName=~/sign/i) {
                        $NewInit_Desc{"Value"} = vary_values(["1", "0"], \%Init_Desc);
                    }
                    elsif($ParamName=~/endian|order/i) {
                        $NewInit_Desc{"Value"} = vary_values(["1", "0"], \%Init_Desc);
                    }
                    elsif($ParamName=~/\A(w|width)\d*\Z/i
                    and $I_ShortName=~/display/i) {
                        $NewInit_Desc{"Value"} = vary_values(["640"], \%Init_Desc);
                    }
                    elsif($ParamName=~/\A(h|height)\d*\Z/i
                    and $I_ShortName=~/display/i) {
                        $NewInit_Desc{"Value"} = vary_values(["480"], \%Init_Desc);
                    }
                    elsif($ParamName=~/width|height/i
                    or $ParamName=~/\A(x|y|z|w|h)\d*\Z/i) {
                        $NewInit_Desc{"Value"} = vary_values([8 * getIntrinsicValue($FoundationTypeName)], \%Init_Desc);
                    }
                    elsif($ParamName=~/offset/i) {
                        $NewInit_Desc{"Value"} = vary_values(["8", "16"], \%Init_Desc);
                    }
                    elsif($ParamName=~/stride|step|spacing|iter|interval|move/i
                    or $ParamName=~/\A(to)\Z/) {
                        $NewInit_Desc{"Value"} = vary_values(["1"], \%Init_Desc);
                    }
                    elsif($ParamName=~/channels|frames/i and $I_ShortName=~/\Asnd_/i)
                    {# ALSA
                        $NewInit_Desc{"Value"} = vary_values([$DEFAULT_ARRAY_AMOUNT], \%Init_Desc);
                    }
                    elsif($ParamName=~/first/i and ($Init_Desc{"OuterType_Type"} eq "Struct" and get_TypeName($Init_Desc{"OuterType_Id"})=~/_snd_/i))
                    {# ALSA
                        $NewInit_Desc{"Value"} = vary_values([8 * getIntrinsicValue($FoundationTypeName)], \%Init_Desc);
                    }
                    elsif(isFD($TypeId, $ParamName))
                    {
                        $NewInit_Desc{"Value"} = vary_values(["open(TG_TEST_DATA_PLAIN_FILE, O_RDWR)"], \%Init_Desc);
                        $NewInit_Desc{"Headers"} = addHeaders(["sys/stat.h", "fcntl.h"], $NewInit_Desc{"Headers"});
                        $AuxHeaders{"sys/stat.h"}=1;
                        $NewInit_Desc{"InLine"}=0;
                        $AuxHeaders{"fcntl.h"}=1;
                        $FuncNames{"open"} = 1;
                    }
                    elsif(($TypeName=~/enum/i or $ParamName=~/message_type/i)
                    and my $EnumConstant = selectConstant($TypeName, $ParamName, $Interface))
                    {# or ($TypeName eq "int" and $ParamName=~/\Amode|type\Z/i and $I_ShortName=~/\Asnd_/i) or $ParamName=~/mask/
                        $NewInit_Desc{"Value"} = vary_values([$EnumConstant], \%Init_Desc);
                        $NewInit_Desc{"Headers"} = addHeaders([$Constants{$EnumConstant}{"Header"}], $NewInit_Desc{"Headers"});
                    }
                    elsif($TypeName=~/enum/i
                    or $ParamName=~/mode|type|flag|option/i) {
                        $NewInit_Desc{"Value"} = vary_values(["0"], \%Init_Desc);
                    }
                    elsif($ParamName=~/mask|alloc/i) {
                        $NewInit_Desc{"Value"} = vary_values(["0"], \%Init_Desc);
                    }
                    elsif($ParamName=~/screen|format/i) {
                        $NewInit_Desc{"Value"} = vary_values(["1"], \%Init_Desc);
                    }
                    elsif($ParamName=~/ed\Z/i) {
                        $NewInit_Desc{"Value"} = vary_values(["0"], \%Init_Desc);
                    }
                    elsif($ParamName=~/key/i
                    and $I_ShortName=~/\A[_]*X/) {#X11
                        $NewInit_Desc{"Value"} = vary_values(["9"], \%Init_Desc);
                    }
                    elsif($ParamName=~/\Ap\d+\Z/
                    and $Init_Desc{"ParamPos"}==$Init_Desc{"MaxParamPos"}
                    and $I_ShortName=~/create|intern|privat/i) {
                        $NewInit_Desc{"Value"} = vary_values(["0"], \%Init_Desc);
                    }
                    elsif($TypeName=~/size/i) {
                        $NewInit_Desc{"Value"} = vary_values([$DEFAULT_ARRAY_AMOUNT], \%Init_Desc);
                    }
                    else {
                        $NewInit_Desc{"Value"} = vary_values([getIntrinsicValue($FoundationTypeName)], \%Init_Desc);
                    }
                }
                else {
                    $NewInit_Desc{"Value"} = "0";
                }
            }
            elsif(isCharType($FoundationTypeName)
            and $TypeName=~/bool/i) {
                $NewInit_Desc{"Value"} = vary_values([1, 0], \%Init_Desc);
            }
            else {
                $NewInit_Desc{"Value"} = vary_values([getIntrinsicValue($FoundationTypeName)], \%Init_Desc);
            }
            $NewInit_Desc{"ValueTypeId"} = ($PointerLevel==0)?$TypeId:$FoundationTypeId;
        }
        elsif($FoundationTypeType eq "Enum")
        {
            if(my $EnumMember = getSomeEnumMember($FoundationTypeId))
            {
                if(defined $Template2Code and $PointerLevel==0)
                {
                    my $Members = [];
                    foreach my $Member (@{getEnumMembers($FoundationTypeId)})
                    {
                        if(is_valid_constant($Member)) {
                            push(@{$Members}, $Member);
                        }
                    }
                    if($#{$Members}>=0) {
                        $NewInit_Desc{"Value"} = vary_values($Members, \%Init_Desc);
                    }
                    else {
                        $NewInit_Desc{"Value"} = vary_values(getEnumMembers($FoundationTypeId), \%Init_Desc);
                    }
                }
                else {
                    $NewInit_Desc{"Value"} = $EnumMember;
                }
            }
            else {
                $NewInit_Desc{"Value"} = "0";
            }
            $NewInit_Desc{"ValueTypeId"} = $FoundationTypeId;
        }
    }
    else
    {
        if(not $NewInit_Desc{"ValueTypeId"})
        {#for union spectypes
            $NewInit_Desc{"ValueTypeId"} = $TypeId;
        }
    }
    if($NewInit_Desc{"Value"} eq "")
    {
        $NewInit_Desc{"Value"} = "no value";
    }
    return %NewInit_Desc;
}

sub is_valid_constant($)
{
    my $Constant = $_[0];
    return $Constant!~/(unknown|invalid|null|err|none|(_|\A)(ms|win\d*|no)(_|\Z))/i;
}

sub get_CharNum()
{
    $IntrinsicNum{"Char"}=64 if($IntrinsicNum{"Char"} > 89 or $IntrinsicNum{"Char"} < 64);
    if($RandomCode) {
        $IntrinsicNum{"Char"} = 64+int(rand(25));
    }
    $IntrinsicNum{"Char"}+=1;
    return $IntrinsicNum{"Char"};
}

sub vary_values($$)
{
    my ($ValuesArrayRef, $Init_Desc) = @_;
    my @ValuesArray = @{$ValuesArrayRef};
    return "" if($#ValuesArray==-1);
    if(defined $Template2Code and ($Init_Desc->{"Interface"} eq $TestedInterface) and not $Init_Desc->{"OuterType_Type"} and length($Init_Desc->{"ParamName"})>=2 and $Init_Desc->{"ParamName"}!~/\Ap\d+\Z/i)
    {
        my $Define = uc($Init_Desc->{"ParamName"});
        if(defined $Constants{$Define}) {
            $Define = "_".$Define;
        }
        $Define = select_var_name($Define, "");
        $Block_Variable{$CurrentBlock}{$Define} = 1;
        my $DefineWithNum = keys(%Template2Code_Defines).":".$Define;
        if($#ValuesArray>=1) {
            $Template2Code_Defines{$DefineWithNum} = "SET(".$ValuesArray[0]."; ".$ValuesArray[1].")";
        }
        else {
            $Template2Code_Defines{$DefineWithNum} = $ValuesArray[0];
        }
        return $Define;
    }
    else
    {#standalone
        return $ValuesArray[0];
    }
}

sub selectConstant($$$)
{
    my ($TypeName, $ParamName, $Interface) = @_;
    return $Cache{"selectConstant"}{$TypeName}{$ParamName}{$Interface} if(defined $Cache{"selectConstant"}{$TypeName}{$ParamName}{$Interface});
    my @Csts = ();
    foreach (keys(%Constants))
    {
        if($RegisteredHeaders_Short{$Constants{$_}{"HeaderName"}})
        {
            if($Constants{$_}{"Value"}=~/\A\d/) {
                push(@Csts, $_);
            }
        }
    }
    @Csts = sort @Csts;
    @Csts = sort {length($a)<=>length($b)} @Csts;
    @Csts = sort {$CompleteSignature{$Interface}{"Header"} cmp $Constants{$a}{"HeaderName"}} @Csts;
    my (@Valid, @Invalid) = ();
    foreach (@Csts)
    {
        if(is_valid_constant($_)) {
            push(@Valid, $_);
        }
        else {
            push(@Invalid, $_);
        }
    }
    @Csts = (@Valid, @Invalid);
    sort_byName(\@Csts, $ParamName." ".$CompleteSignature{$Interface}{"ShortName"}." ".$TypeName, "Constants");
    if($#Csts>=0)
    {
        $Cache{"selectConstant"}{$TypeName}{$ParamName}{$Interface} = $Csts[0];
        return $Csts[0];
    }
    else
    {
        $Cache{"selectConstant"}{$TypeName}{$ParamName}{$Interface} = "";
        return "";
    }
}

sub isFD($$)
{
    my ($TypeId, $ParamName) = @_;
    my $FoundationTypeId = get_FoundationTypeId($TypeId);
    my $FoundationTypeName = get_TypeName($FoundationTypeId);
    if($ParamName=~/(\A|[_]+)fd(s|)\Z/i
    and isIntegerType($FoundationTypeName)) {
        return (selectSystemHeader("sys/stat.h") and selectSystemHeader("fcntl.h"));
    }
    else {
        return "";
    }
}

sub find_similar_type($$)
{
    my ($TypeId, $ParamName) = @_;
    return 0 if(not $TypeId or not $ParamName);
    return 0 if($ParamName=~/\A(p\d+|data|object)\Z/i or length($ParamName)<=2 or is_out_word($ParamName));
    return $Cache{"find_similar_type"}{$TypeId}{$ParamName} if(defined $Cache{"find_similar_type"}{$TypeId}{$ParamName} and not defined $AuxType{$TypeId});
    my $PointerLevel = get_PointerLevel($Tid_TDid{$TypeId}, $TypeId);
    $ParamName=~s/([a-z][a-df-rt-z])s\Z/$1/i;
    my @TypeNames = ();
    foreach my $TypeName (keys(%StructUnionPName_Tid))
    {
        if($TypeName=~/\Q$ParamName\E/i)
        {
            my $Tid = $StructUnionPName_Tid{$TypeName};
            next if(not $Tid);
            next if(not $DependencyHeaders_All{get_TypeHeader($Tid)});
            my $FTid = get_FoundationTypeId($Tid);
            next if(get_TypeType($FTid)!~/\A(Struct|Union)\Z/);
            next if(isOpaque($FTid) and not keys(%{$ReturnTypeId_Interface{$Tid}}));
            next if(get_PointerLevel($Tid_TDid{$Tid}, $Tid)!=$PointerLevel);
            push(@TypeNames, $TypeName);
        }
    }
    @TypeNames = sort {lc($a) cmp lc($b)} @TypeNames;
    @TypeNames = sort {length($a)<=>length($b)} @TypeNames;
    @TypeNames = sort {$a=~/\*/<=>$b=~/\*/} @TypeNames;
    #@TypeNames = sort {keys(%{$ReturnTypeId_Interface{$TName_Tid{$b}}})<=>keys(%{$ReturnTypeId_Interface{$TName_Tid{$a}}})} @TypeNames;
    if($#TypeNames>=0)
    {
        $Cache{"find_similar_type"}{$TypeId}{$ParamName} = $TName_Tid{$TypeNames[0]};
        return $StructUnionPName_Tid{$TypeNames[0]};
    }
    else
    {
        $Cache{"find_similar_type"}{$TypeId}{$ParamName} = 0;
        return 0;
    }
}

sub isCyclical($$)
{
    return (grep {$_ eq $_[1]} @{$_[0]});
}

sub tmplInstHeaders($)
{
    my $ClassId = $_[0];
    my $Headers = [];
    foreach my $Param_Pos (sort {int($a)<=>int($b)} keys(%{$TemplateInstance{$Tid_TDid{$ClassId}}{$ClassId}}))
    {
        my $Type_Id = $TemplateInstance{$Tid_TDid{$ClassId}}{$ClassId}{$Param_Pos};
        $Headers = addHeaders(getTypeHeaders($Type_Id), $Headers);
    }
    return $Headers;
}

sub convert_familiar_types(@)
{
    my %Conv = @_;
    return () if(not $Conv{"OutputTypeId"} or not $Conv{"InputTypeName"} or not $Conv{"Value"} or not $Conv{"Key"});
    my $OutputType_PointerLevel = get_PointerLevel($Tid_TDid{$Conv{"OutputTypeId"}}, $Conv{"OutputTypeId"});
    my $OutputType_Name = get_TypeName($Conv{"OutputTypeId"});
    my $OutputFType_Id = get_FoundationTypeId($Conv{"OutputTypeId"});
    my $OutputFType_Name = get_TypeName($OutputFType_Id);
    my $OutputType_BaseTypeType = get_TypeType($OutputFType_Id);
    my $PLevelDelta = $OutputType_PointerLevel - $Conv{"InputPointerLevel"};
    return ($Conv{"Value"}, "") if($OutputType_Name eq "...");
    my $Tmp_Var = $Conv{"Key"};
    $Tmp_Var .= ($Conv{"Destination"} eq "Target")?"_tp":"_p";
    my $NeedTypeConvertion = 0;
    my ($Preamble, $ToCall) = ();
    # pointer convertion
    if($PLevelDelta==0) {
        $ToCall = $Conv{"Value"};
    }
    elsif($PLevelDelta==1)
    {
        if($Conv{"Value"}=~/\A\&/)
        {
            $Preamble .= $Conv{"InputTypeName"}." $Tmp_Var = (".$Conv{"InputTypeName"}.")".$Conv{"Value"}.";\n";
            $Block_Variable{$CurrentBlock}{$Tmp_Var} = 1;
            $ToCall = "&".$Tmp_Var;
        }
        else {
            $ToCall = "&".$Conv{"Value"};
        }
    }
    elsif($PLevelDelta<0)
    {
        foreach (0 .. - 1 - $PLevelDelta) {
            $ToCall = $ToCall."*";
        }
        $ToCall = $ToCall.$Conv{"Value"};
    }
    else
    {#this section must be deprecated in future
        my $Stars = "**";
        if($Conv{"Value"}=~/\A\&/)
        {
            $Preamble .= $Conv{"InputTypeName"}." $Tmp_Var = (".$Conv{"InputTypeName"}.")".$Conv{"Value"}.";\n";
            $Block_Variable{$CurrentBlock}{$Tmp_Var} = 1;
            $Conv{"Value"} = $Tmp_Var;
            $Tmp_Var .= "p";
        }
        $Preamble .= $Conv{"InputTypeName"}." *$Tmp_Var = (".$Conv{"InputTypeName"}." *)&".$Conv{"Value"}.";\n";
        $Block_Variable{$CurrentBlock}{$Tmp_Var} = 1;
        my $Tmp_Var_Pre = $Tmp_Var;
        foreach my $Itr (1 .. $PLevelDelta - 1)
        {
            my $ItrM1 = $Itr - 1;
            $Tmp_Var .= "p";
            $Block_Variable{$CurrentBlock}{$Tmp_Var} = 1;
            $Preamble .= $Conv{"InputTypeName"}." $Stars$Tmp_Var = &$Tmp_Var_Pre;\n";
            $Stars .= "*";
            $NeedTypeConvertion = 1;
            $Tmp_Var_Pre = $Tmp_Var;
            $ToCall = $Tmp_Var;
        }
    }
    $Preamble .= "\n" if($Preamble);
    $NeedTypeConvertion = 1 if(get_base_type_name($OutputType_Name) ne get_base_type_name($Conv{"InputTypeName"}));
    $NeedTypeConvertion = 1 if(not is_equal_types($OutputType_Name,$Conv{"InputTypeName"}) and $PLevelDelta==0);
    $NeedTypeConvertion = 1 if(not is_const_type($OutputType_Name) and is_const_type($Conv{"InputTypeName"}));
    $NeedTypeConvertion = 0 if(($OutputType_PointerLevel eq 0) and (($OutputType_BaseTypeType eq "Class") or ($OutputType_BaseTypeType eq "Struct")));
    $NeedTypeConvertion = 1 if((($OutputType_Name=~/\&/) or $Conv{"MustConvert"}) and ($OutputType_PointerLevel > 0) and (($OutputType_BaseTypeType eq "Class") or ($OutputType_BaseTypeType eq "Struct")));
    $NeedTypeConvertion = 1 if($OutputType_PointerLevel eq 2);
    $NeedTypeConvertion = 0 if($OutputType_Name eq $Conv{"InputTypeName"});
    $NeedTypeConvertion = 0 if(uncover_typedefs($OutputType_Name)=~/\[(\d+|)\]/);#arrays
    
    #type convertion
    if($NeedTypeConvertion and ($Conv{"Destination"} eq "Param"))
    {
        if($ToCall=~/\-\>/) {
            $ToCall = "(".$OutputType_Name.")"."(".$ToCall.")";
        }
        else {
            $ToCall = "(".$OutputType_Name.")".$ToCall;
        }
    }
    return ($ToCall, $Preamble);
}

sub sortTypes_ByPLevel($$)
{
    my ($Types, $PLevel) = @_;
    my (@Eq, @Lt, @Gt) = ();
    foreach my $TypeId (@{$Types})
    {
        my $Type_PLevel = get_PointerLevel($Tid_TDid{$TypeId}, $TypeId);
        if($Type_PLevel==$PLevel) {
            push(@Eq, $TypeId);
        }
        elsif($Type_PLevel<$PLevel) {
            push(@Lt, $TypeId);
        }
        elsif($Type_PLevel>$PLevel) {
            push(@Gt, $TypeId);
        }
    }
    @{$Types} = (@Eq, @Lt, @Gt);
}

sub familyTypes($)
{
    my $TypeId = $_[0];
    return [] if(not $TypeId);
    my $FoundationTypeId = get_FoundationTypeId($TypeId);
    return $Cache{"familyTypes"}{$TypeId} if($Cache{"familyTypes"}{$TypeId} and not defined $AuxType{$TypeId});
    my (@FamilyTypes_Const, @FamilyTypes_NotConst) = ();
    foreach my $TDid (sort {int($a)<=>int($b)} keys(%TypeDescr))
    {
        foreach my $Tid (sort {int($a)<=>int($b)} keys(%{$TypeDescr{$TDid}}))
        {
            if((get_FoundationTypeId($Tid) eq $FoundationTypeId) and ($Tid ne $TypeId))
            {
                if(is_const_type(get_TypeName($Tid))) {
                    @FamilyTypes_Const = (@FamilyTypes_Const, $Tid);
                }
                else {
                    @FamilyTypes_NotConst = (@FamilyTypes_NotConst, $Tid);
                }
            }
        }
    }
    sortTypes_ByPLevel(\@FamilyTypes_Const, get_PointerLevel($Tid_TDid{$TypeId}, $TypeId));
    sortTypes_ByPLevel(\@FamilyTypes_NotConst, get_PointerLevel($Tid_TDid{$TypeId}, $TypeId));
    my @FamilyTypes = ((is_const_type(get_TypeName($TypeId)))?(@FamilyTypes_NotConst, $TypeId, @FamilyTypes_Const):($TypeId, @FamilyTypes_NotConst, @FamilyTypes_Const));
    $Cache{"familyTypes"}{$TypeId} = \@FamilyTypes;
    return \@FamilyTypes;
}

sub register_ExtType($$$)
{
    my ($Type_Name, $Type_Type, $BaseTypeId) = @_;
    return "" if(not $Type_Name or not $Type_Type or not $BaseTypeId);
    return $TName_Tid{$Type_Name} if($TName_Tid{$Type_Name});
    $MaxTypeId += 1;
    $TName_Tid{$Type_Name} = $MaxTypeId;
    $Tid_TDid{$MaxTypeId}="";
    %{$TypeDescr{""}{$MaxTypeId}}=(
        "Name" => $Type_Name,
        "Type" => $Type_Type,
        "BaseType"=>{"TDid" => $Tid_TDid{$BaseTypeId},
                     "Tid" => $BaseTypeId},
        "TDid" => "",
        "Tid" => $MaxTypeId,
        "Headers"=>getTypeHeaders($BaseTypeId)
    );
    $AuxType{$MaxTypeId}=$Type_Name;
    return $MaxTypeId;
}


sub get_ExtTypeId($$)
{
    my ($Key, $TypeId) = @_;
    return () if(not $TypeId);
    my ($Declarations, $Headers) = ("", []);
    if(get_TypeType($TypeId) eq "Typedef") {
        return ($TypeId, "", "");
    }
    my $FTypeId = get_FoundationTypeId($TypeId);
    my %BaseType = goToFirst($Tid_TDid{$TypeId}, $TypeId, "Typedef");
    my $BaseTypeId = $BaseType{"Tid"};
    if(not $BaseTypeId)
    {
        $BaseTypeId = $FTypeId;
        if(get_TypeName($BaseTypeId)=~/\Astd::/)
        {
            if(my $CxxTypedefId = get_type_typedef($BaseTypeId)) {
                $BaseTypeId = $CxxTypedefId;
            }
        }
    }
    my $PointerLevel = get_PointerLevel($Tid_TDid{$TypeId}, $TypeId) - get_PointerLevel($Tid_TDid{$BaseTypeId}, $BaseTypeId);
    if(get_TypeType($FTypeId) eq "Array")
    {
        my ($Array_BaseName, $Array_Length) = reassemble_array($FTypeId);
        $BaseTypeId = get_TypeIdByName($Array_BaseName);
        $PointerLevel+=1;
    }
    my $BaseTypeName = get_TypeName($BaseTypeId);
    my $BaseTypeType = get_TypeType($BaseTypeId);
    if($BaseTypeType eq "FuncPtr") {
        $Declarations .= declare_funcptr_typedef($Key, $BaseTypeId);
    }
    if(isAnon($BaseTypeName))
    {
        if($BaseTypeType eq "Struct")
        {
            my ($AnonStruct_Declarations, $AnonStruct_Headers) = declare_anon_struct($Key, $BaseTypeId);
            $Declarations .= $AnonStruct_Declarations;
            $Headers = addHeaders($AnonStruct_Headers, $Headers);
        }
        elsif($BaseTypeType eq "Union")
        {
            my ($AnonUnion_Declarations, $AnonUnion_Headers) = declare_anon_union($Key, $BaseTypeId);
            $Declarations .= $AnonUnion_Declarations;
            $Headers = addHeaders($AnonUnion_Headers, $Headers);
        }
    }
    if($PointerLevel>=1)
    {
#         if(get_TypeType(get_FoundationTypeId($TypeId)) eq "FuncPtr" and get_TypeName($TypeId)=~/\A[^*]+const\W/)
#         {
#             $BaseTypeId = register_ExtType(get_TypeName($BaseTypeId)." const", "Const", $BaseTypeId);
#         }
        
        my $ExtTypeId = register_new_type($BaseTypeId, $PointerLevel);
        return ($ExtTypeId, $Declarations, $Headers);
    }
    else {
        return ($BaseTypeId, $Declarations, $Headers);
    }
}

sub register_new_type($$)
{
    my ($BaseTypeId, $PLevel) = @_;
    my $ExtTypeName = get_TypeName($BaseTypeId);
    my $ExtTypeId = $BaseTypeId;
    foreach (1 .. $PLevel)
    {
        $ExtTypeName .= "*";
        $ExtTypeName = correctName($ExtTypeName);
        if(not $TName_Tid{$ExtTypeName}) {
            register_ExtType($ExtTypeName, "Pointer", $ExtTypeId);
        }
        $ExtTypeId = $TName_Tid{$ExtTypeName};
    }
    return $ExtTypeId;
}

sub correct_init_stmt($$$)
{
    my ($String, $TypeName, $ParamName) = @_;
    my $Stmt = $TypeName." ".$ParamName." = ".$TypeName;
    if($String=~/\Q$Stmt\E\:\:/) {
        return $String;
    }
    else
    {
        $String=~s/(\W|\A)\Q$Stmt\E\(\)(\W|\Z)/$1$TypeName $ParamName$2/g;
        $String=~s/(\W|\A)\Q$Stmt\E(\W|\Z)/$1$TypeName $ParamName$2/g;
        return $String;
    }
}

sub isValidConv($)
{
    return ($_[0]!~/\A(__va_list_tag|...)\Z/);
}

sub emptyDeclaration(@)
{
    my %Init_Desc = @_;
    my %Type_Init = ();
    $Init_Desc{"Var"} = select_var_name($LongVarNames?$Init_Desc{"Key"}:$Init_Desc{"ParamName"}, $Init_Desc{"ParamNameExt"});
    my $Var = $Init_Desc{"Var"};
    my $InitializedType_PLevel = get_PointerLevel($Tid_TDid{$Init_Desc{"ValueTypeId"}}, $Init_Desc{"ValueTypeId"});
    my ($ETypeId, $Declarations, $Headers) = get_ExtTypeId($LongVarNames?$Init_Desc{"Key"}:$Init_Desc{"ParamName"}, $Init_Desc{"ValueTypeId"});
    my $InitializedType_Name = get_TypeName($ETypeId);
    if($InitializedType_Name eq "void") {
        $InitializedType_Name = "int";
    }
    $Type_Init{"Code"} .= $Declarations;
    $Type_Init{"Headers"} = addHeaders($Headers, $Type_Init{"Headers"});
    $Type_Init{"Headers"} = addHeaders($Headers, getTypeHeaders($ETypeId));
    $Type_Init{"Headers"} = addHeaders($Headers, getTypeHeaders(get_FoundationTypeId($ETypeId))) if($InitializedType_PLevel==0);
    $Type_Init{"Init"} = $InitializedType_Name." ".$Var.";\n";
    $Block_Variable{$CurrentBlock}{$Var} = 1;
    #create call
    my ($Call, $Preamble) = convert_familiar_types((
        "InputTypeName"=>$InitializedType_Name,
        "InputPointerLevel"=>$InitializedType_PLevel,
        "OutputTypeId"=>($Init_Desc{"TypeId_Changed"})?$Init_Desc{"TypeId_Changed"}:$Init_Desc{"TypeId"},
        "Value"=>$Var,
        "Key"=>$Var,
        "Destination"=>"Param",
        "MustConvert"=>0));
    $Type_Init{"Init"} .= $Preamble;
    $Type_Init{"Call"} = $Call;
    #call to constraint
    if($Init_Desc{"TargetTypeId"}==$Init_Desc{"TypeId"}) {
        $Type_Init{"TargetCall"} = $Type_Init{"Call"};
    }
    else
    {
        my ($TargetCall, $TargetPreamble) =
        convert_familiar_types((
            "InputTypeName"=>$InitializedType_Name,
            "InputPointerLevel"=>$InitializedType_PLevel,
            "OutputTypeId"=>$Init_Desc{"TargetTypeId"},
            "Value"=>$Var,
            "Key"=>$Var,
            "Destination"=>"Target",
            "MustConvert"=>0));
        $Type_Init{"TargetCall"} = $TargetCall;
        $Type_Init{"Init"} .= $TargetPreamble;
    }
    $Type_Init{"IsCorrect"} = 1;
    return %Type_Init;
}

sub initializeByValue(@)
{
    my %Init_Desc = @_;
    return () if($Init_Desc{"DoNotAssembly"} and $Init_Desc{"ByNull"});
    my %Type_Init = ();
    $Init_Desc{"InLine"} = 1 if($Init_Desc{"Value"}=~/\$\d+/);
    my $TName_Trivial = get_TypeName($Init_Desc{"TypeId"});
    $TName_Trivial=~s/&//g;
    my $TType = get_TypeType($Init_Desc{"TypeId"});
    my $FoundationType_Id = get_FoundationTypeId($Init_Desc{"TypeId"});
    #$Type_Init{"Headers"} = addHeaders(getTypeHeaders($FoundationType_Id), $Type_Init{"Headers"});
    $Type_Init{"Headers"} = addHeaders(getTypeHeaders($Init_Desc{"TypeId"}), $Type_Init{"Headers"});
    if(uncover_typedefs(get_TypeName($Init_Desc{"TypeId"}))=~/\&/
    and $Init_Desc{"OuterType_Type"}=~/\A(Struct|Union|Array)\Z/) {
        $Init_Desc{"InLine"} = 0;
    }
    my $FoundationType_Name = get_TypeName($FoundationType_Id);
    my $FoundationType_Type = get_TypeType($FoundationType_Id);
    my $PointerLevel = get_PointerLevel($Tid_TDid{$Init_Desc{"TypeId"}}, $Init_Desc{"TypeId"});
    my $Target_PointerLevel = get_PointerLevel($Tid_TDid{$Init_Desc{"TargetTypeId"}}, $Init_Desc{"TargetTypeId"});
    if($FoundationType_Name eq "...") {
        $PointerLevel=get_PointerLevel($Tid_TDid{$Init_Desc{"ValueTypeId"}}, $Init_Desc{"ValueTypeId"});
        $Target_PointerLevel=$PointerLevel;
    }
    my $Value_PointerLevel = get_PointerLevel($Tid_TDid{$Init_Desc{"ValueTypeId"}}, $Init_Desc{"ValueTypeId"});
    return () if(not $Init_Desc{"ValueTypeId"} or $Init_Desc{"Value"} eq "");
    $Init_Desc{"Var"} = select_var_name($LongVarNames?$Init_Desc{"Key"}:$Init_Desc{"ParamName"}, $Init_Desc{"ParamNameExt"});
    my $Var = $Init_Desc{"Var"};
    my ($Value_ETypeId, $Declarations, $Headers) = get_ExtTypeId($LongVarNames?$Init_Desc{"Key"}:$Init_Desc{"ParamName"}, $Init_Desc{"ValueTypeId"});
    my $Value_ETypeName = get_TypeName($Value_ETypeId);
    $Type_Init{"Code"} .= $Declarations;
    $Type_Init{"Headers"} = addHeaders($Headers, $Type_Init{"Headers"});
    if($FoundationType_Type eq "Class")
    {# classes
        my ($ChildCreated, $CallDestructor) = (0, 1);
        if(my $ValueClass = getValueClass($Init_Desc{"Value"}) and $Target_PointerLevel eq 0)
        {#class object construction by constructor in value
            if($FoundationType_Name eq $ValueClass)
            {
                if(isAbstractClass($FoundationType_Id) or $Init_Desc{"CreateChild"})
                {#when don't know constructor in value, so declaring all in the child
                    my $ChildClassName = getSubClassName($FoundationType_Name);
                    my $FoundationChildName = getSubClassName($FoundationType_Name);
                    $ChildCreated = 1;
                    if($Init_Desc{"Value"}=~/\Q$FoundationType_Name\E/
                    and $Init_Desc{"Value"}!~/\Q$ChildClassName\E/) {
                        substr($Init_Desc{"Value"}, index($Init_Desc{"Value"}, $FoundationType_Name), pos($FoundationType_Name) + length($FoundationType_Name)) = $FoundationChildName;
                    }
                    $IntSubClass{$TestedInterface}{$FoundationType_Id} = 1;
                    $Create_SubClass{$FoundationType_Id} = 1;
                    foreach my $ClassConstructor (getClassConstructors($FoundationType_Id)) {
                        $UsedConstructors{$FoundationType_Id}{$ClassConstructor} = 1;
                    }
                    $FoundationType_Name = $ChildClassName;
                }
            }
            else
            {#new class
                $FoundationType_Name = $ValueClass;
            }
            if($Init_Desc{"InLine"} and ($PointerLevel eq 0))
            {
                $Type_Init{"Call"} = $Init_Desc{"Value"};
                $CallDestructor = 0;
            }
            else
            {
                $Block_Variable{$CurrentBlock}{$Var} = 1;
                if(not defined $DisableReuse) {
                    $ValueCollection{$CurrentBlock}{$Var} = $FoundationType_Id;
                }
                $Type_Init{"Init"} .= $FoundationType_Name." $Var = ".$Init_Desc{"Value"}.";".($Init_Desc{"ByNull"}?" //can't initialize":"")."\n";
                $Type_Init{"Headers"} = addHeaders(getTypeHeaders($FoundationType_Id), $Type_Init{"Headers"});
                $Type_Init{"Init"} = correct_init_stmt($Type_Init{"Init"}, $FoundationType_Name, $Var);
                my ($Call, $TmpPreamble) =
                convert_familiar_types((
                    "InputTypeName"=>$FoundationType_Name,
                    "InputPointerLevel"=>$Value_PointerLevel,
                    "OutputTypeId"=>($Init_Desc{"TypeId_Changed"})?$Init_Desc{"TypeId_Changed"}:$Init_Desc{"TypeId"},
                    "Value"=>$Var,
                    "Key"=>$Var,
                    "Destination"=>"Param",
                    "MustConvert"=>0));
                $Type_Init{"Init"} .= $TmpPreamble;
                $Type_Init{"Call"} = $Call;
            }
        }
        else
        {#class object returned by some interface in value
            if($Init_Desc{"CreateChild"})
            {
                $ChildCreated = 1;
                my $FoundationChildName = getSubClassName($FoundationType_Name);
                my $TNameChild = $TName_Trivial;
                substr($Value_ETypeName, index($Value_ETypeName, $FoundationType_Name), pos($FoundationType_Name) + length($FoundationType_Name)) = $FoundationChildName;
                substr($TNameChild, index($TNameChild, $FoundationType_Name), pos($FoundationType_Name) + length($FoundationType_Name)) = $FoundationChildName;
                $IntSubClass{$TestedInterface}{$FoundationType_Id} = 1;
                $Create_SubClass{$FoundationType_Id} = 1;
                if($Value_PointerLevel==0
                and my $SomeConstructor = getSomeConstructor($FoundationType_Id)) {
                    $UsedConstructors{$FoundationType_Id}{$SomeConstructor} = 1;
                }
                if($Init_Desc{"InLine"} and ($PointerLevel eq $Value_PointerLevel))
                {
                    if($Init_Desc{"Value"} eq "NULL"
                    or $Init_Desc{"Value"} eq "0") {
                        $Type_Init{"Call"} = "($TNameChild) ".$Init_Desc{"Value"};
                    }
                    else
                    {
                        my ($Call, $TmpPreamble) =
                        convert_familiar_types((
                            "InputTypeName"=>get_TypeName($Init_Desc{"ValueTypeId"}),
                            "InputPointerLevel"=>$Value_PointerLevel,
                            "OutputTypeId"=>($Init_Desc{"TypeId_Changed"})?$Init_Desc{"TypeId_Changed"}:$Init_Desc{"TypeId"},
                            "Value"=>$Init_Desc{"Value"},
                            "Key"=>$LongVarNames?$Init_Desc{"Key"}:$Init_Desc{"ParamName"},
                            "Destination"=>"Param",
                            "MustConvert"=>1));
                        $Type_Init{"Call"} = $Call;
                        $Type_Init{"Init"} .= $TmpPreamble;
                    }
                    $CallDestructor = 0;
                }
                else
                {
                    $Block_Variable{$CurrentBlock}{$Var} = 1;
                    if((not defined $DisableReuse and ($Init_Desc{"Value"} ne "NULL") and ($Init_Desc{"Value"} ne "0"))
                    or $Init_Desc{"ByNull"} or $Init_Desc{"UseableValue"}) {
                        $ValueCollection{$CurrentBlock}{$Var} = $Value_ETypeId;
                    }
                    $Type_Init{"Init"} .= $Value_ETypeName." $Var = ($Value_ETypeName)".$Init_Desc{"Value"}.";".($Init_Desc{"ByNull"}?" //can't initialize":"")."\n";
                    $Type_Init{"Headers"} = addHeaders(getTypeHeaders($Value_ETypeId), $Type_Init{"Headers"});
                    my ($Call, $TmpPreamble) =
                    convert_familiar_types((
                        "InputTypeName"=>$Value_ETypeName,
                        "InputPointerLevel"=>$Value_PointerLevel,
                        "OutputTypeId"=>($Init_Desc{"TypeId_Changed"})?$Init_Desc{"TypeId_Changed"}:$Init_Desc{"TypeId"},
                        "Value"=>$Var,
                        "Key"=>$Var,
                        "Destination"=>"Param",
                        "MustConvert"=>0));
                    $Type_Init{"Init"} .= $TmpPreamble;
                    $Type_Init{"Call"} = $Call;
                }
            }
            else
            {
                if($Init_Desc{"InLine"} and $PointerLevel eq $Value_PointerLevel)
                {
                    if($Init_Desc{"Value"} eq "NULL"
                    or $Init_Desc{"Value"} eq "0") {
                        $Type_Init{"Call"} = "($TName_Trivial) ".$Init_Desc{"Value"};
                        $CallDestructor = 0;
                    }
                    else
                    {
                        $CallDestructor = 0;
                        my ($Call, $TmpPreamble) =
                        convert_familiar_types((
                            "InputTypeName"=>get_TypeName($Init_Desc{"ValueTypeId"}),
                            "InputPointerLevel"=>$Value_PointerLevel,
                            "OutputTypeId"=>($Init_Desc{"TypeId_Changed"})?$Init_Desc{"TypeId_Changed"}:$Init_Desc{"TypeId"},
                            "Value"=>$Init_Desc{"Value"},
                            "Key"=>$LongVarNames?$Init_Desc{"Key"}:$Init_Desc{"ParamName"},
                            "Destination"=>"Param",
                            "MustConvert"=>1));
                        $Type_Init{"Call"} = $Call;
                        $Type_Init{"Init"} .= $TmpPreamble;
                    }
                }
                else
                {
                    $Block_Variable{$CurrentBlock}{$Var} = 1;
                    if((not defined $DisableReuse and $Init_Desc{"Value"} ne "NULL" and $Init_Desc{"Value"} ne "0")
                    or $Init_Desc{"ByNull"} or $Init_Desc{"UseableValue"}) {
                        $ValueCollection{$CurrentBlock}{$Var} = $Value_ETypeId;
                    }
                    $Type_Init{"Init"} .= $Value_ETypeName." $Var = ".$Init_Desc{"Value"}.";".($Init_Desc{"ByNull"}?" //can't initialize":"")."\n";
                    $Type_Init{"Headers"} = addHeaders(getTypeHeaders($Value_ETypeId), $Type_Init{"Headers"});
                    my ($Call, $TmpPreamble) =
                    convert_familiar_types((
                        "InputTypeName"=>$Value_ETypeName,
                        "InputPointerLevel"=>$Value_PointerLevel,
                        "OutputTypeId"=>($Init_Desc{"TypeId_Changed"})?$Init_Desc{"TypeId_Changed"}:$Init_Desc{"TypeId"},
                        "Value"=>$Var,
                        "Key"=>$Var,
                        "Destination"=>"Param",
                        "MustConvert"=>0));
                    $Type_Init{"Init"} .= $TmpPreamble;
                    $Type_Init{"Call"} = $Call;
                }
            }
        }
        
        # create destructor call for class object
        if($CallDestructor and
        ((has_public_destructor($FoundationType_Id, "D2") and $ChildCreated) or
        (has_public_destructor($FoundationType_Id, "D0") and not $ChildCreated)) )
        {
            if($Value_PointerLevel > 0)
            {
                if($Value_PointerLevel eq 1) {
                    $Type_Init{"Destructors"} .= "delete($Var);\n";
                }
                else
                {
                    $Type_Init{"Destructors"} .= "delete(";
                    foreach (0 .. $Value_PointerLevel - 2) {
                        $Type_Init{"Destructors"} .= "*";
                    }
                    $Type_Init{"Destructors"} .= $Var.");\n";
                }
            }
        }
    }
    else
    {# intrinsics, structs
        if($Init_Desc{"InLine"} and ($PointerLevel eq $Value_PointerLevel))
        {
            if(($Init_Desc{"Value"} eq "NULL") or ($Init_Desc{"Value"} eq "0"))
            {
                if((getIntLang($TestedInterface) eq "C++" or $Init_Desc{"StrongConvert"})
                and isValidConv($TName_Trivial) and ($Init_Desc{"OuterType_Type"} ne "Array"))
                {
                    $Type_Init{"Call"} = "($TName_Trivial) ".$Init_Desc{"Value"};
                }
                else
                {
                    $Type_Init{"Call"} = $Init_Desc{"Value"};
                }
            }
            else
            {
                if((not is_equal_types(get_TypeName($Init_Desc{"TypeId"}), get_TypeName($Init_Desc{"ValueTypeId"})) or $Init_Desc{"StrongConvert"}) and isValidConv($TName_Trivial))
                {
                    $Type_Init{"Call"} = "($TName_Trivial) ".$Init_Desc{"Value"};
                }
                else
                {
                    $Type_Init{"Call"} = $Init_Desc{"Value"};
                }
            }
        }
        else
        {
            $Block_Variable{$CurrentBlock}{$Var} = 1;
            if((not defined $DisableReuse and ($Init_Desc{"Value"} ne "NULL") and ($Init_Desc{"Value"} ne "0"))
            or $Init_Desc{"ByNull"} or $Init_Desc{"UseableValue"})
            {
                $ValueCollection{$CurrentBlock}{$Var} = $Value_ETypeId;
            }
            $Type_Init{"Init"} .= $Value_ETypeName." $Var = ".$Init_Desc{"Value"}.";".($Init_Desc{"ByNull"}?" //can't initialize":"")."\n";
            $Type_Init{"Headers"} = addHeaders(getTypeHeaders($Value_ETypeId), $Type_Init{"Headers"});
            my ($Call, $TmpPreamble) =
            convert_familiar_types((
                "InputTypeName"=>$Value_ETypeName,
                "InputPointerLevel"=>$Value_PointerLevel,
                "OutputTypeId"=>($Init_Desc{"TypeId_Changed"})?$Init_Desc{"TypeId_Changed"}:$Init_Desc{"TypeId"},
                "Value"=>$Var,
                "Key"=>$Var,
                "Destination"=>"Param",
                "MustConvert"=>$Init_Desc{"StrongConvert"}));
            $Type_Init{"Init"} .= $TmpPreamble;
            $Type_Init{"Call"} = $Call;
        }
    }
    #call to constraint
    if($Init_Desc{"TargetTypeId"}==$Init_Desc{"TypeId"})
    {
        $Type_Init{"TargetCall"} = $Type_Init{"Call"};
    }
    else
    {
        my ($TargetCall, $TargetPreamble) =
        convert_familiar_types((
            "InputTypeName"=>$Value_ETypeName,
            "InputPointerLevel"=>$Value_PointerLevel,
            "OutputTypeId"=>$Init_Desc{"TargetTypeId"},
            "Value"=>$Var,
            "Key"=>$Var,
            "Destination"=>"Target",
            "MustConvert"=>0));
        $Type_Init{"TargetCall"} = $TargetCall;
        $Type_Init{"Init"} .= $TargetPreamble;
    }
    if(get_TypeType($Init_Desc{"TypeId"}) eq "Ref")
    {#ref handler
        my $BaseRefId = get_OneStep_BaseTypeId($Tid_TDid{$Init_Desc{"TypeId"}}, $Init_Desc{"TypeId"});
        my $BaseRefName = get_TypeName($BaseRefId);
        if(get_PointerLevel($Tid_TDid{$BaseRefId}, $BaseRefId) > $Value_PointerLevel)
        {
            $Type_Init{"Init"} .= $BaseRefName." ".$Var."_ref = ".$Type_Init{"Call"}.";\n";
            $Type_Init{"Call"} = $Var."_ref";
            $Block_Variable{$CurrentBlock}{$Var."_ref"} = 1;
            if(not defined $DisableReuse and ($Init_Desc{"Value"} ne "NULL") and ($Init_Desc{"Value"} ne "0"))
            {
                $ValueCollection{$CurrentBlock}{$Var."_ref"} = $Init_Desc{"TypeId"};
            }
        }
    }
    $Type_Init{"Code"} = $Type_Init{"Code"};
    $Type_Init{"IsCorrect"} = 1;
    $Type_Init{"ByNull"} = 1 if($Init_Desc{"ByNull"});
    return %Type_Init;
}

sub remove_quals($)
{
    my $Type_Name = $_[0];
    $Type_Name=~s/ (const|volatile|restrict)\Z//g;
    $Type_Name=~s/\A(const|volatile|restrict) //g;
    while($Type_Name=~s/(\W|\A|>)(const|volatile|restrict)(\W([^<>()]+|)|)\Z/$1$3/g){};
    return correctName($Type_Name);
}

sub is_equal_types($$)
{
    my ($Type1_Name, $Type2_Name) = @_;
    return (remove_quals(uncover_typedefs($Type1_Name)) eq
            remove_quals(uncover_typedefs($Type2_Name)));
}

sub get_base_type_name($)
{
    my $Type_Name = $_[0];
    while($Type_Name=~s/(\*|\&)([^<>()]+|)\Z/$2/g){};
    my $Type_Name = remove_quals(uncover_typedefs($Type_Name));
    while($Type_Name=~s/(\*|\&)([^<>()]+|)\Z/$2/g){};
    return $Type_Name;
}

sub isIntegerType($)
{
    my $TName = remove_quals(uncover_typedefs($_[0]));
    return 0 if($TName=~/[(<*]/);
    if($TName eq "bool")
    {
        return (getIntLang($TestedInterface) ne "C++");
    }
    return ($TName=~/(\W|\A| )(int)(\W|\Z| )/
    or $TName=~/\A(short|size_t|unsigned|long|long long|unsigned long|unsigned long long|unsigned short)\Z/);
}

sub isCharType($)
{
    my $TName = remove_quals(uncover_typedefs($_[0]));
    return 0 if($TName=~/[(<*]/);
    return ($TName=~/\A(char|unsigned char|signed char|wchar_t)\Z/);
}

sub isNumericType($)
{
    my $TName = uncover_typedefs($_[0]);
    return 0 if($TName=~/[(<*]/);
    if(isIntegerType($TName))
    {
        return 1;
    }
    else
    {
        return ($TName=~/\A(float|double|long double|float const|double const|long double const)\Z/);
    }
}

sub getIntrinsicValue($)
{
    my $TypeName = $_[0];
    $IntrinsicNum{"Char"}=64 if($IntrinsicNum{"Char"}>89 or $IntrinsicNum{"Char"}<64);
    $IntrinsicNum{"Int"}=0 if($IntrinsicNum{"Int"} >= 10);
    if($RandomCode)
    {
        $IntrinsicNum{"Char"} = 64+int(rand(25));
        $IntrinsicNum{"Int"} = int(rand(5));
    }
    if($TypeName eq "char*")
    {
        $IntrinsicNum{"Str"}+=1;
        if($IntrinsicNum{"Str"}==1)
        {
            return "\"str\"";
        }
        else
        {
            return "\"str".$IntrinsicNum{"Str"}."\"";
        }
    }
    elsif($TypeName=~/(\A| )char(\Z| )/)
    {
        $IntrinsicNum{"Char"} += 1;
        return "'".chr($IntrinsicNum{"Char"})."'";
    }
    elsif($TypeName eq "wchar_t")
    {
        $IntrinsicNum{"Char"}+=1;
        return "L'".chr($IntrinsicNum{"Char"})."'";
    }
    elsif($TypeName eq "wchar_t*")
    {
        $IntrinsicNum{"Str"}+=1;
        if($IntrinsicNum{"Str"}==1)
        {
            return "L\"str\"";
        }
        else
        {
            return "L\"str".$IntrinsicNum{"Str"}."\"";
        }
    }
    elsif($TypeName eq "wint_t")
    {
        $IntrinsicNum{"Int"}+=1;
        return "L".$IntrinsicNum{"Int"};
    }
    elsif($TypeName=~/\A(long|long int)\Z/)
    {
        $IntrinsicNum{"Int"} += 1;
        return $IntrinsicNum{"Int"}."L";
    }
    elsif($TypeName=~/\A(long long|long long int)\Z/)
    {
        $IntrinsicNum{"Int"} += 1;
        return $IntrinsicNum{"Int"}."LL";
    }
    elsif(isIntegerType($TypeName))
    {
        $IntrinsicNum{"Int"} += 1;
        return $IntrinsicNum{"Int"};
    }
    elsif($TypeName eq "float")
    {
        $IntrinsicNum{"Float"} += 1;
        return $IntrinsicNum{"Float"}.".5f";
    }
    elsif($TypeName eq "double")
    {
        $IntrinsicNum{"Float"} += 1;
        return $IntrinsicNum{"Float"}.".5";
    }
    elsif($TypeName eq "long double")
    {
        $IntrinsicNum{"Float"} += 1;
        return $IntrinsicNum{"Float"}.".5L";
    }
    elsif($TypeName eq "bool")
    {
        if(getIntLang($TestedInterface) eq "C++")
        {
            return "true";
        }
        else
        {
            return "1";
        }
    }
    else
    {#void, "..." and other
        return "";
    }
}

sub findInterface_OutParam($$$$$$)
{
    my ($TypeId, $Key, $StrongTypeCompliance, $Var, $ParamName, $Strong) = @_;
    return () if(not $TypeId);
    foreach my $FamilyTypeId (get_OutParamFamily($TypeId, 1))
    {
        foreach my $Interface (get_CompatibleInterfaces($FamilyTypeId, "OutParam", $ParamName))
        {#find interface to create some type in the family as output parameter
            if($Strong)
            {
                foreach my $PPos (keys(%{$CompleteSignature{$Interface}{"Param"}}))
                {# only one possible structural out parameter
                    my $PTypeId = $CompleteSignature{$Interface}{"Param"}{$PPos}{"type"};
                    my $P_FTypeId = get_FoundationTypeId($PTypeId);
                    return () if(get_TypeType($P_FTypeId)!~/\A(Intrinsic|Enum)\Z/
                    and $P_FTypeId ne get_FoundationTypeId($FamilyTypeId)
                    and not is_const_type(get_TypeName($PTypeId)));
                }
            }
            my $OutParam_Pos = $OutParam_Interface{$FamilyTypeId}{$Interface};
            my %Interface_Init = callInterface((
                "Interface"=>$Interface, 
                "Key"=>$Key, 
                "OutParam"=>$OutParam_Pos,
                "OutVar"=>$Var));
            if($Interface_Init{"IsCorrect"})
            {
                $Interface_Init{"Interface"} = $Interface;
                $Interface_Init{"OutParamPos"} = $OutParam_Pos;
                return %Interface_Init;
            }
        }
    }
    return ();
}

sub findInterface(@)
{
    my %Init_Desc = @_;
    my ($TypeId, $Key, $StrongTypeCompliance, $ParamName) = ($Init_Desc{"TypeId"}, $Init_Desc{"Key"}, $Init_Desc{"StrongTypeCompliance"}, $Init_Desc{"ParamName"});
    return () if(not $TypeId);
    my @FamilyTypes = ();
    if($StrongTypeCompliance)
    {
        @FamilyTypes = ($TypeId);
        # try to initialize basic typedef
        my $BaseTypeId = $TypeId;
        $BaseTypeId = get_OneStep_BaseTypeId($Tid_TDid{$TypeId}, $TypeId) if(get_TypeType($BaseTypeId) eq "Const");
        $BaseTypeId = get_OneStep_BaseTypeId($Tid_TDid{$TypeId}, $TypeId) if(get_TypeType($BaseTypeId) eq "Pointer");
        push(@FamilyTypes, $BaseTypeId) if(get_TypeType($BaseTypeId) eq "Typedef");
    }
    else {
        @FamilyTypes = @{familyTypes($TypeId)};
    }
    my @Ints = ();
    foreach my $FamilyTypeId (@FamilyTypes)
    {
        next if((get_PointerLevel($Tid_TDid{$TypeId}, $TypeId)<get_PointerLevel($Tid_TDid{$FamilyTypeId}, $FamilyTypeId)) and $Init_Desc{"OuterType_Type"} eq "Array");
        next if(get_TypeType($TypeId) eq "Class" and get_PointerLevel($Tid_TDid{$FamilyTypeId}, $FamilyTypeId)==0);
        if($Init_Desc{"OnlyData"}) {
            @Ints = (@Ints, get_CompatibleInterfaces($FamilyTypeId, "OnlyData",
                              $Init_Desc{"Interface"}." ".$ParamName." ".$Init_Desc{"KeyWords"}));
        }
        elsif($Init_Desc{"OnlyReturn"}) {
            @Ints = (@Ints, get_CompatibleInterfaces($FamilyTypeId, "OnlyReturn",
                              $Init_Desc{"Interface"}." ".$ParamName." ".$Init_Desc{"KeyWords"}));
        }
        else {
            @Ints = (@Ints, get_CompatibleInterfaces($FamilyTypeId, "Return",
                              $Init_Desc{"Interface"}." ".$ParamName." ".$Init_Desc{"KeyWords"}));
        }
    }
    sort_byCriteria(\@Ints, "DeleteSmth");
    foreach my $Interface (@Ints)
    {# find interface for returning some type in the family
        my %Interface_Init = callInterface((
            "Interface"=>$Interface, 
            "Key"=>$Key,
            "RetParam"=>$ParamName));
        if($Interface_Init{"IsCorrect"}) {
            $Interface_Init{"Interface"} = $Interface;
            return %Interface_Init;
        }
    }
    return ();
}

sub initializeByInterface_OutParam(@)
{
    my %Init_Desc = @_;
    return () if(not $Init_Desc{"TypeId"});
    my $Global_State = save_state();
    my %Type_Init = ();
    my $FTypeId = get_FoundationTypeId($Init_Desc{"TypeId"});
    my $PointerLevel = get_PointerLevel($Tid_TDid{$Init_Desc{"TypeId"}}, $Init_Desc{"TypeId"});
    $Init_Desc{"Var"} = select_var_name($LongVarNames?$Init_Desc{"Key"}:$Init_Desc{"ParamName"}, $Init_Desc{"ParamNameExt"});
    my $Var = $Init_Desc{"Var"};
    $Block_Variable{$CurrentBlock}{$Var} = 1;
    my %Interface_Init = findInterface_OutParam($Init_Desc{"TypeId"}, $Init_Desc{"Key"}, $Init_Desc{"StrongTypeCompliance"}, "\@OUT_PARAM\@", $Init_Desc{"ParamName"}, $Init_Desc{"Strong"});
    if(not $Interface_Init{"IsCorrect"})
    {
        restore_state($Global_State);
        return ();
    }
    $Type_Init{"Init"} = $Interface_Init{"Init"};
    $Type_Init{"Destructors"} = $Interface_Init{"Destructors"};
    $Type_Init{"Code"} .= $Interface_Init{"Code"};
    $Type_Init{"Headers"} = addHeaders($Interface_Init{"Headers"}, $Type_Init{"Headers"});
    
    #initialization
    my $OutParam_Pos = $Interface_Init{"OutParamPos"};
    my $OutParam_TypeId = $CompleteSignature{$Interface_Init{"Interface"}}{"Param"}{$OutParam_Pos}{"type"};
    my $PLevel_Out = get_PointerLevel($Tid_TDid{$OutParam_TypeId}, $OutParam_TypeId);
    my ($InitializedEType_Id, $Declarations, $Headers) = get_ExtTypeId($LongVarNames?$Init_Desc{"Key"}:$Init_Desc{"ParamName"}, $OutParam_TypeId);
    my $InitializedType_Name = get_TypeName($InitializedEType_Id);
    $Type_Init{"Code"} .= $Declarations;
    $Type_Init{"Headers"} = addHeaders($Headers, $Type_Init{"Headers"});
    my $InitializedFType_Id = get_FoundationTypeId($OutParam_TypeId);
    my $InitializedFType_Type = get_TypeType($InitializedFType_Id);
    my $InitializedType_PointerLevel = get_PointerLevel($Tid_TDid{$OutParam_TypeId}, $OutParam_TypeId);
    my $VarNameForReplace = $Var;
    if($PLevel_Out>1 or ($PLevel_Out==1 and not isOpaque($InitializedFType_Id)))
    {
        $OutParam_TypeId = reduce_pointer_level($InitializedEType_Id);
        $InitializedType_Name=get_TypeName($OutParam_TypeId);
        $VarNameForReplace="&".$Var;
        $InitializedType_PointerLevel-=1;
    }
    foreach (keys(%Interface_Init))
    {
        $Interface_Init{$_}=~s/\@OUT_PARAM\@/$VarNameForReplace/g;
        $Interface_Init{$_} = clearSyntax($Interface_Init{$_});
    }
    if(uncover_typedefs($InitializedType_Name)=~/&|\[/ or $PLevel_Out==1)
    {
#         if($InitializedFType_Type eq "Struct")
#         {
#             my %Struct_Desc = %Init_Desc;
#             $Struct_Desc{"TypeId"} = $OutParam_TypeId;
#             $Struct_Desc{"InLine"} = 0;
#             my $Key = $Struct_Desc{"Key"};
#             delete($Block_Variable{$CurrentBlock}{$Var});
#             my %Assembly = assembleStruct(%Struct_Desc);
#             $Block_Variable{$CurrentBlock}{$Var} = 1;
#             $Type_Init{"Init"} .= $Assembly{"Init"};
#             $Type_Init{"Code"} .= $Assembly{"Code"};
#             $Type_Init{"Headers"} = addHeaders($Assembly{"Headers"}, $Type_Init{"Headers"});
#         }
#         else
#         {
        $Type_Init{"Init"} .= $InitializedType_Name." $Var;\n";
        if(get_TypeType($InitializedFType_Id) eq "Struct")
        {
            my %Type = get_Type($Tid_TDid{$InitializedFType_Id}, $InitializedFType_Id);
            foreach my $MemPos (keys(%{$Type{"Memb"}}))
            {
                if($Type{"Memb"}{$MemPos}{"name"}=~/initialized/i
                and isNumericType(get_TypeName($Type{"Memb"}{$MemPos}{"type"})))
                {
                    $Type_Init{"Init"} .= "$Var.initialized = 0;\n";
                    last;
                }
            }
        }
    }
    else
    {
        $Type_Init{"Init"} .= $InitializedType_Name." $Var = ".get_null().";\n";
    }
    if(not defined $DisableReuse)
    {
        $ValueCollection{$CurrentBlock}{$Var} = $OutParam_TypeId;
    }
    $Type_Init{"Init"} .= $Interface_Init{"PreCondition"} if($Interface_Init{"PreCondition"});
    $Type_Init{"Init"} .= $Interface_Init{"Call"}.";\n";
    $Type_Init{"Headers"} = addHeaders(getTypeHeaders($Init_Desc{"TypeId"}), $Type_Init{"Headers"});
    $Type_Init{"Init"} .= $Interface_Init{"PostCondition"} if($Interface_Init{"PostCondition"});
    if($Interface_Init{"FinalCode"})
    {
        $Type_Init{"Init"} .= "//final code\n";
        $Type_Init{"Init"} .= $Interface_Init{"FinalCode"}."\n";
    }
    #create call
    my ($Call, $Preamble) = convert_familiar_types((
        "InputTypeName"=>$InitializedType_Name,
        "InputPointerLevel"=>$InitializedType_PointerLevel,
        "OutputTypeId"=>($Init_Desc{"TypeId_Changed"})?$Init_Desc{"TypeId_Changed"}:$Init_Desc{"TypeId"},
        "Value"=>$Var,
        "Key"=>$Var,
        "Destination"=>"Param",
        "MustConvert"=>0));
    $Type_Init{"Init"} .= $Preamble;
    $Type_Init{"Call"} = $Call;
    #create call to constraint
    if($Init_Desc{"TargetTypeId"}==$Init_Desc{"TypeId"})
    {
        $Type_Init{"TargetCall"} = $Type_Init{"Call"};
    }
    else
    {
        my ($TargetCall, $TargetPreamble) = convert_familiar_types((
            "InputTypeName"=>$InitializedType_Name,
            "InputPointerLevel"=>$InitializedType_PointerLevel,
            "OutputTypeId"=>$Init_Desc{"TargetTypeId"},
            "Value"=>$Var,
            "Key"=>$Var,
            "Destination"=>"Target",
            "MustConvert"=>0));
        $Type_Init{"TargetCall"} = $TargetCall;
        $Type_Init{"Init"} .= $TargetPreamble;
    }
    if(get_TypeType($Init_Desc{"TypeId"}) eq "Ref")
    {#ref handler
        my $BaseRefTypeId = get_OneStep_BaseTypeId($Tid_TDid{$Init_Desc{"TypeId"}}, $Init_Desc{"TypeId"});
        if(get_PointerLevel($Tid_TDid{$BaseRefTypeId}, $BaseRefTypeId) > $InitializedType_PointerLevel)
        {
            my $BaseRefTypeName = get_TypeName($BaseRefTypeId);
            $Type_Init{"Init"} .= $BaseRefTypeName." ".$Var."_ref = ".$Type_Init{"Call"}.";\n";
            $Type_Init{"Call"} = $Var."_ref";
            $Block_Variable{$CurrentBlock}{$Var."_ref"} = 1;
            if(not defined $DisableReuse)
            {
                $ValueCollection{$CurrentBlock}{$Var."_ref"} = $Init_Desc{"TypeId"};
            }
        }
    }
    $Type_Init{"Init"} .= "\n";
    $Type_Init{"IsCorrect"} = 1;
    return %Type_Init;
}

sub declare_funcptr_typedef($$)
{
    my ($Key, $TypeId) = @_;
    return "" if($AuxType{$TypeId} or not $TypeId or not $Key);
    my $TypedefTo = $Key."_type";
    my $Typedef = "typedef ".get_TypeName($TypeId).";\n";
    $Typedef=~s/[ ]*\(\*\)[ ]*/ \(\*$TypedefTo\) /;
    $AuxType{$TypeId} = $TypedefTo;
    $TypeDescr{$Tid_TDid{$TypeId}}{$TypeId}{"Name_Old"} = get_TypeName($TypeId);
    $TypeDescr{$Tid_TDid{$TypeId}}{$TypeId}{"Name"} = $AuxType{$TypeId};
    $TName_Tid{$TypedefTo} = $TypeId;
    return $Typedef;
}

sub have_copying_constructor($)
{
    my $ClassId = $_[0];
    return 0 if(not $ClassId);
    foreach my $Constructor (keys(%{$Class_Constructors{$ClassId}}))
    {
        if(keys(%{$CompleteSignature{$Constructor}{"Param"}})==1
        and not $CompleteSignature{$Constructor}{"Protected"}
        and not $CompleteSignature{$Constructor}{"Private"})
        {
            my $FirstParamTypeId = $CompleteSignature{$Constructor}{"Param"}{0}{"type"};
            if(get_FoundationTypeId($FirstParamTypeId) eq $ClassId
            and get_PointerLevel($Tid_TDid{$FirstParamTypeId}, $FirstParamTypeId)==0) {
                return 1;
            }
        }
    }
    return 0;
}

sub initializeByInterface(@)
{
    my %Init_Desc = @_;
    return () if(not $Init_Desc{"TypeId"});
    my $Global_State = save_state();
    my %Type_Init = ();
    my $PointerLevel = get_PointerLevel($Tid_TDid{$Init_Desc{"TypeId"}}, $Init_Desc{"TypeId"});
    my $FTypeId = get_FoundationTypeId($Init_Desc{"TypeId"});
    if(get_TypeType($FTypeId) eq "Class" and $PointerLevel==0
    and not have_copying_constructor($FTypeId)) {
        return ();
    }
    my %Interface_Init = ();
    if($Init_Desc{"ByInterface"})
    {
        %Interface_Init = callInterface((
          "Interface"=>$Init_Desc{"ByInterface"}, 
          "Key"=>$Init_Desc{"Key"},
          "RetParam"=>$Init_Desc{"ParamName"},
          "OnlyReturn"=>1));
    }
    else {
        %Interface_Init = findInterface(%Init_Desc);
    }
    if(not $Interface_Init{"IsCorrect"})
    {
        restore_state($Global_State);
        return ();
    }
    $Init_Desc{"Var"} = select_var_name($LongVarNames?$Init_Desc{"Key"}:$Init_Desc{"ParamName"}, $Init_Desc{"ParamNameExt"});
    $Type_Init{"Init"} = $Interface_Init{"Init"};
    $Type_Init{"Destructors"} = $Interface_Init{"Destructors"};
    $Type_Init{"Code"} = $Interface_Init{"Code"};
    $Type_Init{"Headers"} = addHeaders($Interface_Init{"Headers"}, $Type_Init{"Headers"});
    if(keys(%{$CompleteSignature{$Interface_Init{"Interface"}}{"Param"}})>$MAX_PARAMS_INLINE)
    {
        $Init_Desc{"InLine"} = 0;
    }
    #initialization
    my $ReturnType_PointerLevel = get_PointerLevel($Tid_TDid{$Interface_Init{"ReturnTypeId"}}, $Interface_Init{"ReturnTypeId"});
    if($ReturnType_PointerLevel==$PointerLevel and $Init_Desc{"InLine"}
    and not $Interface_Init{"PreCondition"} and not $Interface_Init{"PostCondition"}
    and not $Interface_Init{"ReturnFinalCode"})
    {
        my ($Call, $Preamble) = convert_familiar_types((
            "InputTypeName"=>get_TypeName($Interface_Init{"ReturnTypeId"}),
            "InputPointerLevel"=>$ReturnType_PointerLevel,
            "OutputTypeId"=>($Init_Desc{"TypeId_Changed"})?$Init_Desc{"TypeId_Changed"}:$Init_Desc{"TypeId"},
            "Value"=>$Interface_Init{"Call"},
            "Key"=>$Init_Desc{"Var"},
            "Destination"=>"Param",
            "MustConvert"=>0));
        $Type_Init{"Init"} .= $Preamble;
        $Type_Init{"Call"} = $Call;
        $Type_Init{"TypeName"} = get_TypeName($Interface_Init{"ReturnTypeId"});
    }
    else
    {
        my $Var = $Init_Desc{"Var"};
        $Block_Variable{$CurrentBlock}{$Var} = 1;
        my ($InitializedEType_Id, $Declarations, $Headers) = get_ExtTypeId($LongVarNames?$Init_Desc{"Key"}:$Init_Desc{"ParamName"}, $Interface_Init{"ReturnTypeId"});
        my $InitializedType_Name = get_TypeName($InitializedEType_Id);
        $Type_Init{"TypeName"} = $InitializedType_Name;
        $Type_Init{"Code"} .= $Declarations;
        $Type_Init{"Headers"} = addHeaders($Headers, $Type_Init{"Headers"});
        my %ReturnType = get_Type($Tid_TDid{$Interface_Init{"ReturnTypeId"}}, $Interface_Init{"ReturnTypeId"});
        if(not defined $DisableReuse)
        {
            $ValueCollection{$CurrentBlock}{$Var} = $Interface_Init{"ReturnTypeId"};
        }
        $Type_Init{"Init"} .= $Interface_Init{"PreCondition"} if($Interface_Init{"PreCondition"});
        if(($InitializedType_Name eq $ReturnType{"Name"}))
        {
            $Type_Init{"Init"} .= $InitializedType_Name." $Var = ".$Interface_Init{"Call"}.";\n";
        }
        else
        {
            $Type_Init{"Init"} .= $InitializedType_Name." $Var = "."(".$InitializedType_Name.")".$Interface_Init{"Call"}.";\n";
        }
        if($Interface_Init{"Interface"} eq "fopen")
        {
            $OpenStreams{$CurrentBlock}{$Var} = 1;
        }
        #create call
        my ($Call, $Preamble) = convert_familiar_types((
            "InputTypeName"=>$InitializedType_Name,
            "InputPointerLevel"=>$ReturnType_PointerLevel,
            "OutputTypeId"=>($Init_Desc{"TypeId_Changed"})?$Init_Desc{"TypeId_Changed"}:$Init_Desc{"TypeId"},
            "Value"=>$Var,
            "Key"=>$Var,
            "Destination"=>"Param",
            "MustConvert"=>0));
        $Type_Init{"Init"} .= $Preamble;
        $Type_Init{"Call"} = $Call;
        #create call to constraint
        if($Init_Desc{"TargetTypeId"}==$Init_Desc{"TypeId"})
        {
            $Type_Init{"TargetCall"} = $Type_Init{"Call"};
        }
        else
        {
            my ($TargetCall, $TargetPreamble) = convert_familiar_types((
                "InputTypeName"=>$InitializedType_Name,
                "InputPointerLevel"=>$ReturnType_PointerLevel,
                "OutputTypeId"=>$Init_Desc{"TargetTypeId"},
                "Value"=>$Var,
                "Key"=>$Var,
                "Destination"=>"Target",
                "MustConvert"=>0));
            $Type_Init{"TargetCall"} = $TargetCall;
            $Type_Init{"Init"} .= $TargetPreamble;
        }
        if(get_TypeType($Init_Desc{"TypeId"}) eq "Ref")
        {#ref handler
            my $BaseRefTypeId = get_OneStep_BaseTypeId($Tid_TDid{$Init_Desc{"TypeId"}}, $Init_Desc{"TypeId"});
            if(get_PointerLevel($Tid_TDid{$BaseRefTypeId}, $BaseRefTypeId) > $ReturnType_PointerLevel)
            {
                my $BaseRefTypeName = get_TypeName($BaseRefTypeId);
                $Type_Init{"Init"} .= $BaseRefTypeName." ".$Var."_ref = ".$Type_Init{"Call"}.";\n";
                $Type_Init{"Call"} = $Var."_ref";
                $Block_Variable{$CurrentBlock}{$Var."_ref"} = 1;
                if(not defined $DisableReuse)
                {
                    $ValueCollection{$CurrentBlock}{$Var."_ref"} = $Init_Desc{"TypeId"};
                }
            }
        }
        if($Interface_Init{"ReturnRequirement"})
        {
            $Interface_Init{"ReturnRequirement"}=~s/(\$0|\$retval)/$Var/gi;
            $Type_Init{"Init"} .= $Interface_Init{"ReturnRequirement"};
        }
        if($Interface_Init{"ReturnFinalCode"})
        {
            $Interface_Init{"ReturnFinalCode"}=~s/(\$0|\$retval)/$Var/gi;
            $Type_Init{"Init"} .= "//final code\n";
            $Type_Init{"Init"} .= $Interface_Init{"ReturnFinalCode"}."\n";
        }
    }
    $Type_Init{"Init"} .= $Interface_Init{"PostCondition"} if($Interface_Init{"PostCondition"});
    if($Interface_Init{"FinalCode"})
    {
        $Type_Init{"Init"} .= "//final code\n";
        $Type_Init{"Init"} .= $Interface_Init{"FinalCode"}."\n";
    }
    $Type_Init{"IsCorrect"} = 1;
    return %Type_Init;
}

sub initializeFuncPtr(@)
{
    my %Init_Desc = @_;
    my %Type_Init = initializeByInterface(%Init_Desc);
    if($Type_Init{"IsCorrect"}) {
        return %Type_Init;
    }
    else {
        return assembleFuncPtr(%Init_Desc);
    }
}

sub get_OneStep_BaseTypeId($$)
{
    my ($TypeDId, $TypeId) = @_;
    my %Type = %{$TypeDescr{$TypeDId}{$TypeId}};
    return $Type{"Tid"} if(not $Type{"BaseType"}{"TDid"} and not $Type{"BaseType"}{"Tid"});
    return $Type{"BaseType"}{"Tid"};
}

sub get_OneStep_BaseType($$)
{
    my ($TypeDId, $TypeId) = @_;
    my %Type = %{$TypeDescr{$TypeDId}{$TypeId}};
    return %Type if(not $Type{"BaseType"}{"TDid"} and not $Type{"BaseType"}{"Tid"});
    return get_Type($Type{"BaseType"}{"TDid"}, $Type{"BaseType"}{"Tid"});
}

sub initializeArray(@)
{
    my %Init_Desc = @_;
    if($Init_Desc{"TypeType_Changed"})
    {
        my %Type_Init = assembleArray(%Init_Desc);
        if($Type_Init{"IsCorrect"}) {
            return %Type_Init;
        }
        else {
            $Init_Desc{"FoundationType_Type"} = get_TypeType(get_FoundationTypeId($Init_Desc{"TypeId"}));
            return selectInitializingWay(%Init_Desc);
        }
    }
    else
    {
        $Init_Desc{"StrongTypeCompliance"} = 1;
        my %Type_Init = initializeByInterface(%Init_Desc);
        if($Type_Init{"IsCorrect"}) {
            return %Type_Init;
        }
        else
        {
            %Type_Init = initializeByInterface_OutParam(%Init_Desc);
            if($Type_Init{"IsCorrect"}) {
                return %Type_Init;
            }
            else {
                $Init_Desc{"StrongTypeCompliance"} = 0;
                return assembleArray(%Init_Desc);
            }
        }
    }
}

sub get_PureType($$)
{
    my ($TypeDId, $TypeId) = @_;
    return () if(not $TypeId);
    if(defined $Cache{"get_PureType"}{$TypeDId}{$TypeId}
    and not defined $AuxType{$TypeId}) {
        return %{$Cache{"get_PureType"}{$TypeDId}{$TypeId}};
    }
    return () if(not $TypeDescr{$TypeDId}{$TypeId});
    my %Type = %{$TypeDescr{$TypeDId}{$TypeId}};
    return %Type if(not $Type{"BaseType"}{"TDid"} and not $Type{"BaseType"}{"Tid"});
    if($Type{"Type"}=~/\A(Ref|Const|Volatile|Restrict|Typedef)\Z/) {
        %Type = get_PureType($Type{"BaseType"}{"TDid"}, $Type{"BaseType"}{"Tid"});
    }
    $Cache{"get_PureType"}{$TypeDId}{$TypeId} = \%Type;
    return %Type;
}

sub get_PureTypeId($$)
{
    my ($TypeDId, $TypeId) = @_;
    my %Type = get_PureType($TypeDId, $TypeId);
    return $Type{"Tid"};
}

sub delete_quals($$)
{
    my ($TypeDId, $TypeId) = @_;
    return () if(not $TypeId);
    if(defined $Cache{"delete_quals"}{$TypeDId}{$TypeId}
    and not defined $AuxType{$TypeId}) {
        return %{$Cache{"delete_quals"}{$TypeDId}{$TypeId}};
    }
    return () if(not $TypeDescr{$TypeDId}{$TypeId});
    my %Type = %{$TypeDescr{$TypeDId}{$TypeId}};
    return %Type if(not $Type{"BaseType"}{"TDid"} and not $Type{"BaseType"}{"Tid"});
    if($Type{"Type"}=~/\A(Ref|Const|Volatile|Restrict)\Z/) {
        %Type = delete_quals($Type{"BaseType"}{"TDid"}, $Type{"BaseType"}{"Tid"});
    }
    $Cache{"delete_quals"}{$TypeDId}{$TypeId} = \%Type;
    return %Type;
}

sub goToFirst($$$)
{
    my ($TypeDId, $TypeId, $Type_Type) = @_;
    if(defined $Cache{"goToFirst"}{$TypeDId}{$TypeId}{$Type_Type}
    and not defined $AuxType{$TypeId}) {
        return %{$Cache{"goToFirst"}{$TypeDId}{$TypeId}{$Type_Type}};
    }
    return () if(not $TypeDescr{$TypeDId}{$TypeId});
    my %Type = %{$TypeDescr{$TypeDId}{$TypeId}};
    return () if(not $Type{"Type"});
    if($Type{"Type"} ne $Type_Type)
    {
        return () if(not $Type{"BaseType"}{"Tid"});
        %Type = goToFirst($Type{"BaseType"}{"TDid"}, $Type{"BaseType"}{"Tid"}, $Type_Type);
    }
    $Cache{"goToFirst"}{$TypeDId}{$TypeId}{$Type_Type} = \%Type;
    return %Type;
}

sub detectArrayTypeId($)
{
    my $TypeId = $_[0];
    my $ArrayType_Id = get_FoundationTypeId($TypeId);
    my $PointerLevel = get_PointerLevel($Tid_TDid{$TypeId}, $TypeId);
    if(get_TypeType($ArrayType_Id) eq "Array")# and $PointerLevel==0
    {
        return $ArrayType_Id;
    }
    else
    {#this branch for types like arrays (char* like char[])
        return get_PureTypeId($Tid_TDid{$TypeId}, $TypeId);
    }
}

sub assembleArray(@)
{
    my %Init_Desc = @_;
    my %Type_Init = ();
    my $Global_State = save_state();
    my $PointerLevel = get_PointerLevel($Tid_TDid{$Init_Desc{"TypeId"}}, $Init_Desc{"TypeId"});
    my %Type = get_Type($Tid_TDid{$Init_Desc{"TypeId"}}, $Init_Desc{"TypeId"});
    # determine array base
    my $ArrayType_Id = detectArrayTypeId($Init_Desc{"TypeId"});
    my %ArrayType = get_Type($Tid_TDid{$ArrayType_Id}, $ArrayType_Id);
    my $AmountArray = ($ArrayType{"Type"} eq "Array")?$ArrayType{"Size"}:(($Init_Desc{"ArraySize"})?$Init_Desc{"ArraySize"}:$DEFAULT_ARRAY_AMOUNT);
    if($AmountArray>1024)
    {# such too long arrays should be initialized by other methods
        restore_state($Global_State);
        return ();
    }
    # array base type attributes
    my $ArrayElemType_Id = get_OneStep_BaseTypeId($Tid_TDid{$ArrayType_Id}, $ArrayType_Id);
    my $ArrayElemType_Name = remove_quals(get_TypeName($ArrayElemType_Id));
    my $ArrayElemType_PLevel = get_PointerLevel($Tid_TDid{$ArrayElemType_Id}, $ArrayElemType_Id);
    my $ArrayElemFType_Id = get_FoundationTypeId($ArrayElemType_Id);
    my $IsInlineDef = (($ArrayType{"Type"} eq "Array") and $PointerLevel==0 and ($Type{"Type"} ne "Ref") and $Init_Desc{"InLine"} or $Init_Desc{"InLineArray"});
    $Init_Desc{"Var"} = select_var_name($LongVarNames?$Init_Desc{"Key"}:$Init_Desc{"ParamName"}, $Init_Desc{"ParamNameExt"});
    my $Var = $Init_Desc{"Var"};
    if(not $IsInlineDef) {
        $Block_Variable{$CurrentBlock}{$Var} = 1;
    }
    if(not isCharType(get_TypeName($ArrayElemFType_Id)) and not $IsInlineDef)
    {
        my ($ExtTypeId, $Declarations, $Headers) = get_ExtTypeId($LongVarNames?$Init_Desc{"Key"}:$Init_Desc{"ParamName"}, $ArrayElemType_Id);
        $ArrayElemType_Id = $ExtTypeId;
        $Type_Init{"Code"} .= $Declarations;
        $Type_Init{"Headers"} = addHeaders($Headers, $Type_Init{"Headers"});
    }
    my @ElemStr = ();
    foreach my $Elem_Pos (1 .. $AmountArray)
    {# initialize array members
        my $ElemName = "";
        if(isCharType(get_TypeName($ArrayElemFType_Id))
        and $ArrayElemType_PLevel==1) {
            $ElemName = $Init_Desc{"ParamName"}."_".$Elem_Pos;
        }
        elsif(my $EName = getParamNameByTypeName($ArrayElemType_Name)) {
            $ElemName = $EName;
        }
        else {
            $ElemName = $Init_Desc{"ParamName"}.((not defined $DisableReuse)?"_elem":"");
            $ElemName=~s/es_elem\Z/e/g;
        }
        my %Elem_Init = initializeParameter((
            "TypeId" => $ArrayElemType_Id,
            "Key" => $Init_Desc{"Key"}."_".$Elem_Pos,
            "InLine" => 1,
            "Value" => "no value",
            "ValueTypeId" => 0,
            "TargetTypeId" => 0,
            "CreateChild" => 0,
            "Usage" => "Common",
            "ParamName" => $ElemName,
            "OuterType_Type" => "Array",
            "Index" => $Elem_Pos-1,
            "InLineArray" => ($ArrayElemType_PLevel==1 and isCharType(get_TypeName($ArrayElemFType_Id)) and $Init_Desc{"ParamName"}=~/text|txt|doc/i)?1:0,
            "IsString" => ($ArrayElemType_PLevel==1 and isCharType(get_TypeName($ArrayElemFType_Id)) and $Init_Desc{"ParamName"}=~/prefixes/i)?1:0 ));
        if(not $Elem_Init{"IsCorrect"} or $Elem_Init{"ByNull"}) {
            restore_state($Global_State);
            return ();
        }
        if($Elem_Pos eq 1) {
            $Type_Init{"Headers"} = addHeaders($Elem_Init{"Headers"}, $Type_Init{"Headers"});
        }
        @ElemStr = (@ElemStr, $Elem_Init{"Call"});
        $Type_Init{"Init"} .= $Elem_Init{"Init"};
        $Type_Init{"Destructors"} .= $Elem_Init{"Destructors"};
        $Type_Init{"Code"} .= $Elem_Init{"Code"};
    }
    if(($ArrayType{"Type"} ne "Array") and not isNumericType($ArrayElemType_Name))
    {# the last array element
        if($ArrayElemType_PLevel==0
        and get_TypeName($ArrayElemFType_Id)=~/\A(char|unsigned char)\Z/) {
            @ElemStr = (@ElemStr, "\'\\0\'");
        }
        elsif($ArrayElemType_PLevel==0
        and is_equal_types($ArrayElemType_Name, "wchar_t")) {
            @ElemStr = (@ElemStr, "L\'\\0\'");
        }
        elsif($ArrayElemType_PLevel>=1) {
            @ElemStr = (@ElemStr, get_null());
        }
        elsif($ArrayElemType_PLevel==0
        and get_TypeType($ArrayElemFType_Id)=~/\A(Struct|Union)\Z/) {
            @ElemStr = (@ElemStr, "($ArrayElemType_Name) "."{0}");
        }
    }
    # initialization
    if($IsInlineDef) {
        $Type_Init{"Call"} = "{".create_matrix(\@ElemStr, "    ")."}";
    }
    else
    {
        if(not defined $DisableReuse) {
            $ValueCollection{$CurrentBlock}{$Var} = $ArrayType_Id;
        }
        #$Type_Init{"Init"} .= "//parameter initialization\n";
        $Type_Init{"Init"} .= $ArrayElemType_Name." $Var [".(($ArrayType{"Type"} eq "Array")?$AmountArray:"")."] = {".create_matrix(\@ElemStr, "    ")."};\n";
        #create call
        my ($Call, $TmpPreamble) =
        convert_familiar_types((
            "InputTypeName"=>correctName($ArrayElemType_Name."*"),
            "InputPointerLevel"=>get_PointerLevel($Tid_TDid{$ArrayType_Id}, $ArrayType_Id),
            "OutputTypeId"=>($Init_Desc{"TypeId_Changed"})?$Init_Desc{"TypeId_Changed"}:$Init_Desc{"TypeId"},
            "Value"=>$Var,
            "Key"=>$Var,
            "Destination"=>"Param",
            "MustConvert"=>0));
        $Type_Init{"Init"} .= $TmpPreamble;
        $Type_Init{"Call"} = $Call;
        # create type
        
        # create call to constraint
        if($Init_Desc{"TargetTypeId"}==$Init_Desc{"TypeId"}) {
            $Type_Init{"TargetCall"} = $Type_Init{"Call"};
        }
        else
        {
            my ($TargetCall, $Target_TmpPreamble) =
            convert_familiar_types((
                "InputTypeName"=>correctName($ArrayElemType_Name."*"),
                "InputPointerLevel"=>get_PointerLevel($Tid_TDid{$ArrayType_Id}, $ArrayType_Id),
                "OutputTypeId"=>$Init_Desc{"TargetTypeId"},
                "Value"=>$Var,
                "Key"=>$Var,
                "Destination"=>"Target",
                "MustConvert"=>0));
            $Type_Init{"TargetCall"} = $TargetCall;
            $Type_Init{"Init"} .= $Target_TmpPreamble;
        }
        # ref handler
        if($Type{"Type"} eq "Ref")
        {
            my $BaseRefId = get_OneStep_BaseTypeId($Tid_TDid{$Init_Desc{"TypeId"}}, $Init_Desc{"TypeId"});
            if($ArrayType{"Type"} eq "Pointer" or (get_PointerLevel($Tid_TDid{$BaseRefId}, $BaseRefId) > 0))
            {
                my $BaseRefName = get_TypeName($BaseRefId);
                $Type_Init{"Init"} .= $BaseRefName." ".$Var."_ref = ".$Type_Init{"Call"}.";\n";
                $Type_Init{"Call"} = $Var."_ref";
                $Block_Variable{$CurrentBlock}{$Var."_ref"} = 1;
                if(not defined $DisableReuse) {
                    $ValueCollection{$CurrentBlock}{$Var."_ref"} = $Init_Desc{"TypeId"};
                }
            }
        }
    }
    $Type_Init{"TypeName"} = $ArrayElemType_Name." [".(($ArrayType{"Type"} eq "Array")?$AmountArray:"")."]";
    $Type_Init{"IsCorrect"} = 1;
    return %Type_Init;
}

sub get_null()
{
    if(getIntLang($TestedInterface) eq "C++"
    and $Constants{"NULL"}) {
        return "NULL";
    }
    else {
        return "0";
    }
}

sub create_list($$)
{
    my ($Array, $Spaces) = @_;
    my @Elems = @{$Array};
    my ($MaxLength, $SumLength);
    foreach my $Elem (@Elems)
    {
        $SumLength += length($Elem);
        if(not defined $MaxLength
        or $MaxLength<length($Elem)) {
            $MaxLength = length($Elem);
        }
    }
    if(($#Elems+1>$MAX_PARAMS_INLINE)
    or ($SumLength>$MAX_PARAMS_LENGTH_INLINE and $#Elems>0)
    or join("", @Elems)=~/\n/) {
        return "\n$Spaces".join(",\n$Spaces", @Elems);
    }
    else {
        return join(", ", @Elems);
    }
}

sub create_matrix($$)
{
    my ($Array, $Spaces) = @_;
    my @Elems = @{$Array};
    my $MaxLength;
    foreach my $Elem (@Elems)
    {
        if(length($Elem) > $MATRIX_MAX_ELEM_LENGTH) {
            return create_list($Array, $Spaces);
        }
        if(not defined $MaxLength
        or $MaxLength<length($Elem)) {
            $MaxLength = length($Elem);
        }
    }
    if($#Elems+1 >= $MIN_PARAMS_MATRIX)
    {
        my (@Rows, @Row) = ();
        foreach my $Num (0 .. $#Elems)
        {
            my $Elem = $Elems[$Num];
            if($Num%$MATRIX_WIDTH==0 and $Num!=0)
            {
                push(@Rows, join(", ", @Row));
                @Row = ();
            }
            push(@Row, aligh_str($Elem, $MaxLength));
        }
        push(@Rows, join(", ", @Row)) if($#Row>=0);
        return "\n$Spaces".join(",\n$Spaces", @Rows);
    }
    else {
        return create_list($Array, $Spaces);
    }
}

sub aligh_str($$)
{
    my ($Str, $Length) = @_;
    if(length($Str)<$Length) {
        foreach (1 .. $Length - length($Str)) {
            $Str = " ".$Str;
        }
    }
    return $Str;
}

sub findFuncPtr_RealFunc($$)
{
    my ($FuncTypeId, $ParamName) = @_;
    my @AvailableRealFuncs = ();
    foreach my $Interface (sort {length($a)<=>length($b)} sort {$a cmp $b} keys(%{$Func_TypeId{$FuncTypeId}}))
    {
        next if(isCyclical(\@RecurInterface, $Interface));
        if($Interface_Library{$Interface}
        or $NeededInterface_Library{$Interface}) {
            push(@AvailableRealFuncs, $Interface);
        }
    }
    sort_byCriteria(\@AvailableRealFuncs, "Internal");
    @AvailableRealFuncs = sort {($b=~/\Q$ParamName\E/i)<=>($a=~/\Q$ParamName\E/i)} @AvailableRealFuncs if($ParamName!~/\Ap\d+\Z/);
    sort_byName(\@AvailableRealFuncs, $ParamName, "Interfaces");
    if($#AvailableRealFuncs>=0) {
        return $AvailableRealFuncs[0];
    }
    else {
        return "";
    }
}

sub get_base_typedef($)
{
    my $TypeId = $_[0];
    my %TypeDef = goToFirst($Tid_TDid{$TypeId}, $TypeId, "Typedef");
    return 0 if(not $TypeDef{"Type"});
    if(get_PointerLevel($TypeDef{"TDid"}, $TypeDef{"Tid"})==0) {
        return $TypeDef{"Tid"};
    }
    my $BaseTypeId = get_OneStep_BaseTypeId($TypeDef{"TDid"}, $TypeDef{"Tid"});
    return get_base_typedef($BaseTypeId);
}

sub assembleFuncPtr(@)
{
    my %Init_Desc = @_;
    my %Type_Init = ();
    my $Global_State = save_state();
    my %Type = get_Type($Tid_TDid{$Init_Desc{"TypeId"}}, $Init_Desc{"TypeId"});
    my $FuncPtr_TypeId = get_FoundationTypeId($Init_Desc{"TypeId"});
    my %FuncPtrType = get_Type($Tid_TDid{$FuncPtr_TypeId}, $FuncPtr_TypeId);
    my ($TypeName, $AuxFuncName) = ($FuncPtrType{"Name"}, "");
    if(get_PointerLevel($Tid_TDid{$Init_Desc{"TypeId"}}, $Init_Desc{"TypeId"})>0)
    {
        if(my $Typedef_Id = get_base_typedef($Init_Desc{"TypeId"})) {
            $TypeName = get_TypeName($Typedef_Id);
        }
        elsif(my $Typedef_Id = get_type_typedef($FuncPtr_TypeId))
        {
            $Type_Init{"Headers"} = addHeaders(getTypeHeaders($Typedef_Id), $Type_Init{"Headers"});
            $TypeName = get_TypeName($Typedef_Id);
        }
        else
        {
            $Type_Init{"Code"} .= declare_funcptr_typedef($Init_Desc{"Key"}, $FuncPtr_TypeId);
            $TypeName = get_TypeName($FuncPtr_TypeId);
        }
    }
    if($FuncPtrType{"Name"} eq "void*(*)(size_t)")
    {
        $Type_Init{"Headers"} = addHeaders(["stdlib.h"], $Type_Init{"Headers"});
        $AuxHeaders{"stdlib.h"} = 1;
        $AuxFuncName = "malloc";
    }
    elsif(my $Interface_FuncPtr = findFuncPtr_RealFunc($FuncPtrType{"FuncTypeId"}, $Init_Desc{"ParamName"}))
    {
        $UsedInterfaces{$Interface_FuncPtr} = 1;
        $Type_Init{"Headers"} = addHeaders([$CompleteSignature{$Interface_FuncPtr}{"Header"}], $Type_Init{"Headers"});
        $AuxFuncName = $CompleteSignature{$Interface_FuncPtr}{"ShortName"};
        if($CompleteSignature{$Interface_FuncPtr}{"NameSpace"}) {
            $AuxFuncName = $CompleteSignature{$Interface_FuncPtr}{"NameSpace"}."::".$AuxFuncName;
        }
    }
    else
    {
        if($AuxFunc{$FuncPtr_TypeId}) {
            $AuxFuncName = $AuxFunc{$FuncPtr_TypeId};
        }
        else
        {
            my @FuncParams = ();
            $AuxFuncName = select_func_name($LongVarNames?$Init_Desc{"Key"}:(($Init_Desc{"ParamName"}=~/\Ap\d+\Z/)?"aux_func":$Init_Desc{"ParamName"}));
            #global
            $AuxFunc{$FuncPtr_TypeId} = $AuxFuncName;
            my $PreviousBlock = $CurrentBlock;
            $CurrentBlock = $AuxFuncName;
            #function declaration
            my $FuncReturnType_Id = $FuncPtrType{"Return"};
            my $FuncReturnFType_Id = get_FoundationTypeId($FuncReturnType_Id);
            my $FuncReturnFType_Type = get_TypeType($FuncReturnFType_Id);
            foreach my $ParamPos (sort {int($a)<=>int($b)} keys(%{$FuncPtrType{"Param"}}))
            {
                my $ParamTypeId = $FuncPtrType{"Param"}{$ParamPos}{"type"};
                $Type_Init{"Headers"} = addHeaders(getTypeHeaders($ParamTypeId), $Type_Init{"Headers"});
                my $ParamName = $FuncPtrType{"Param"}{$ParamPos}{"name"};
                $ParamName = "p".($ParamPos+1) if(not $ParamName);
                #my ($ParamEType_Id, $Param_Declarations, $Param_Headers) = get_ExtTypeId($LongVarNames?$Init_Desc{"Key"}:$Init_Desc{"ParamName"}, $ParamTypeId);
                my $ParamTypeName = get_TypeName($ParamTypeId);#get_TypeName($ParamEType_Id);
                #$Type_Init{"Header"} = addHeaders($Param_Headers, $Type_Init{"Header"});
                #$Type_Init{"Code"} .= $Param_Declarations;
                if($ParamTypeName and ($ParamTypeName ne "..."))
                {
                    my $Field = create_member_decl($ParamTypeName, $ParamName);
                    @FuncParams = (@FuncParams, $Field);
                }
                $ValueCollection{$AuxFuncName}{$ParamName} = $ParamTypeId;
                $Block_Param{$AuxFuncName}{$ParamName} = $ParamTypeId;
                $Block_Variable{$CurrentBlock}{$ParamName} = 1;
            }
            #definition of function
            if(get_TypeName($FuncReturnType_Id) eq "void")
            {
                my $FuncDef = "//auxiliary function\n";
                $FuncDef .= "void\n".$AuxFuncName."(".create_list(\@FuncParams, "    ").")";
                if($AuxFuncName=~/free/i)
                {
                    my $PtrParam = "";
                    foreach my $ParamPos (sort {int($a)<=>int($b)} keys(%{$FuncPtrType{"Param"}}))
                    {
                        my $ParamTypeId = $FuncPtrType{"Param"}{$ParamPos}{"type"};
                        my $ParamName = $FuncPtrType{"Param"}{$ParamPos}{"name"};
                        $ParamName = "p".($ParamPos+1) if(not $ParamName);
                        my $ParamFTypeId = get_FoundationTypeId($ParamTypeId);
                        if(get_PointerLevel($Tid_TDid{$ParamTypeId}, $ParamTypeId)==1
                        and get_TypeType($ParamFTypeId) eq "Intrinsic")
                        {
                            $PtrParam = $ParamName;
                            last;
                        }
                    }
                    if($PtrParam)
                    {
                        $FuncDef .= "{\n";
                        $FuncDef .= "    free($PtrParam);\n";
                        $FuncDef .= "}\n\n";
                    }
                    else {
                        $FuncDef .= "{}\n\n";
                    }
                }
                else {
                    $FuncDef .= "{}\n\n";
                }
                $Type_Init{"Code"} .= "\n".$FuncDef;
            }
            else
            {
                my %ReturnType_Init = initializeParameter((
                    "TypeId" => $FuncReturnType_Id,
                    "Key" => "retval",
                    "InLine" => 1,
                    "Value" => "no value",
                    "ValueTypeId" => 0,
                    "TargetTypeId" => 0,
                    "CreateChild" => 0,
                    "Usage" => "Common",
                    "RetVal" => 1,
                    "ParamName" => "retval",
                    "FuncPtrTypeId" => $FuncPtr_TypeId),
                    "FuncPtrName" => $AuxFuncName);
                if(not $ReturnType_Init{"IsCorrect"})
                {
                    restore_state($Global_State);
                    $CurrentBlock = $PreviousBlock;
                    return ();
                }
                $ReturnType_Init{"Init"} = alignCode($ReturnType_Init{"Init"}, "    ", 0);
                $ReturnType_Init{"Call"} = alignCode($ReturnType_Init{"Call"}, "    ", 1);
                $Type_Init{"Code"} .= $ReturnType_Init{"Code"};
                $Type_Init{"Headers"} = addHeaders($ReturnType_Init{"Headers"}, $Type_Init{"Headers"});
                my ($FuncReturnEType_Id, $Declarations, $Headers) = get_ExtTypeId($LongVarNames?$Init_Desc{"Key"}:$Init_Desc{"ParamName"}, $FuncReturnType_Id);
                my $FuncReturnType_Name = get_TypeName($FuncReturnEType_Id);
                $Type_Init{"Code"} .= $Declarations;
                $Type_Init{"Headers"} = addHeaders($Headers, $Type_Init{"Headers"});
                my $FuncDef = "//auxiliary function\n";
                $FuncDef .= $FuncReturnType_Name."\n".$AuxFuncName."(".create_list(\@FuncParams, "    ").")";
                $FuncDef .= "{\n";
                $FuncDef .= $ReturnType_Init{"Init"};
                $FuncDef .= "    return ".$ReturnType_Init{"Call"}.";\n}\n\n";
                $Type_Init{"Code"} .= "\n".$FuncDef;
            }
            $CurrentBlock = $PreviousBlock;
        }
    }
    
    $Init_Desc{"Var"} = select_var_name($LongVarNames?$Init_Desc{"Key"}:$Init_Desc{"ParamName"}, $Init_Desc{"ParamNameExt"});
    my $Var = $Init_Desc{"Var"};
    
    #create call
    my ($Call, $TmpPreamble) =
    convert_familiar_types((
        "InputTypeName"=>$TypeName,
        "InputPointerLevel"=>0,
        "OutputTypeId"=>($Init_Desc{"TypeId_Changed"})?$Init_Desc{"TypeId_Changed"}:$Init_Desc{"TypeId"},
        "Value"=>"&".$AuxFuncName,
        "Key"=>$LongVarNames?$Init_Desc{"Key"}:$Init_Desc{"ParamName"},
        "Destination"=>"Param",
        "MustConvert"=>0));
    $Type_Init{"Init"} .= $TmpPreamble;
    $Type_Init{"Call"} = $Call;
    #create type
    $Type_Init{"TypeName"} = get_TypeName($Init_Desc{"TypeId"});
    #create call to constraint
    if($Init_Desc{"TargetTypeId"}==$Init_Desc{"TypeId"}) {
        $Type_Init{"TargetCall"} = $Type_Init{"Call"};
    }
    else
    {
        my ($TargetCall, $Target_TmpPreamble) =
        convert_familiar_types((
            "InputTypeName"=>$TypeName,
            "InputPointerLevel"=>0,
            "OutputTypeId"=>$Init_Desc{"TargetTypeId"},
            "Value"=>"&".$AuxFuncName,
            "Key"=>$LongVarNames?$Init_Desc{"Key"}:$Init_Desc{"ParamName"},
            "Destination"=>"Target",
            "MustConvert"=>0));
        $Type_Init{"TargetCall"} = $TargetCall;
        $Type_Init{"Init"} .= $Target_TmpPreamble;
    }
    
    #ref handler
    if($Type{"Type"} eq "Ref")
    {
        my $BaseRefId = get_OneStep_BaseTypeId($Tid_TDid{$Init_Desc{"TypeId"}}, $Init_Desc{"TypeId"});
        if(get_PointerLevel($Tid_TDid{$BaseRefId}, $BaseRefId) > 0)
        {
            my $BaseRefName = get_TypeName($BaseRefId);
            $Type_Init{"Init"} .= $BaseRefName." ".$Var."_ref = ".$Type_Init{"Call"}.";\n";
            $Type_Init{"Call"} = $Var."_ref";
            $Block_Variable{$CurrentBlock}{$Var."_ref"} = 1;
        }
    }
    $Type_Init{"IsCorrect"} = 1;
    return %Type_Init;
}

sub declare_anon_union($$)
{
    my ($Key, $UnionId) = @_;
    return "" if($AuxType{$UnionId} or not $UnionId or not $Key);
    my %Union = get_Type($Tid_TDid{$UnionId}, $UnionId);
    my @MembStr = ();
    my ($Headers, $Declarations) = ([], "");
    foreach my $Member_Pos (sort {int($a)<=>int($b)} keys(%{$Union{"Memb"}}))
    {#create member types string
        my $Member_Name = $Union{"Memb"}{$Member_Pos}{"name"};
        my $MemberType_Id = $Union{"Memb"}{$Member_Pos}{"type"};
        my $MemberFType_Id = get_FoundationTypeId($MemberType_Id);
        my $MemberType_Name = "";
        if(isAnon(get_TypeName($MemberFType_Id)))
        {
            my ($FieldEType_Id, $Field_Declarations, $Field_Headers) = get_ExtTypeId($Key, $MemberType_Id);
            $Headers = addHeaders($Field_Headers, $Headers);
            $Declarations .= $Field_Declarations;
            $MemberType_Name = get_TypeName($FieldEType_Id);
        }
        else {
            $MemberType_Name = get_TypeName($MemberFType_Id);
        }
        my $MembDecl = create_member_decl($MemberType_Name, $Member_Name);
        @MembStr = (@MembStr, $MembDecl);
    }
    my $Type_Name = select_type_name("union_type_".$Key);
    $Declarations .= "//auxilliary union type\nunion ".$Type_Name;
    $Declarations .= "{\n    ".join(";\n    ", @MembStr).";};\n\n";
    $AuxType{$UnionId} = "union ".$Type_Name;
    $TName_Tid{$AuxType{$UnionId}} = $UnionId;
    $TName_Tid{$Type_Name} = $UnionId;
    $TypeDescr{$Tid_TDid{$UnionId}}{$UnionId}{"Name_Old"} = $Union{"Name"};
    $TypeDescr{$Tid_TDid{$UnionId}}{$UnionId}{"Name"} = $AuxType{$UnionId};
    return ($Declarations, $Headers);
}

sub declare_anon_struct($$)
{
    my ($Key, $StructId) = @_;
    return () if($AuxType{$StructId} or not $StructId or not $Key);
    my %Struct = get_Type($Tid_TDid{$StructId}, $StructId);
    my @MembStr = ();
    my ($Headers, $Declarations) = ([], "");
    foreach my $Member_Pos (sort {int($a)<=>int($b)} keys(%{$Struct{"Memb"}}))
    {
        my $Member_Name = $Struct{"Memb"}{$Member_Pos}{"name"};
        my $MemberType_Id = $Struct{"Memb"}{$Member_Pos}{"type"};
        my $MemberFType_Id = get_FoundationTypeId($MemberType_Id);
        my $MemberType_Name = "";
        if(isAnon(get_TypeName($MemberFType_Id)))
        {
            my ($FieldEType_Id, $Field_Declarations, $Field_Headers) = get_ExtTypeId($Key, $MemberType_Id);
            $Headers = addHeaders($Field_Headers, $Headers);
            $Declarations .= $Field_Declarations;
            $MemberType_Name = get_TypeName($FieldEType_Id);
        }
        else {
            $MemberType_Name = get_TypeName($MemberFType_Id);
        }
        my $MembDecl = create_member_decl($MemberType_Name, $Member_Name);
        @MembStr = (@MembStr, $MembDecl);
    }
    my $Type_Name = select_type_name("struct_type_".$Key);
    $Declarations .= "//auxiliary struct type\nstruct ".$Type_Name;
    $Declarations .= "{\n    ".join(";\n    ", @MembStr).";};\n\n";
    $AuxType{$StructId} = "struct ".$Type_Name;
    $TName_Tid{$AuxType{$StructId}} = $StructId;
    $TName_Tid{$Type_Name} = $StructId;
    $TypeDescr{$Tid_TDid{$StructId}}{$StructId}{"Name_Old"} = $Struct{"Name"};
    $TypeDescr{$Tid_TDid{$StructId}}{$StructId}{"Name"} = $AuxType{$StructId};
    return ($Declarations, $Headers);
}

sub create_member_decl($$)
{
    my ($TName, $Member) = @_;
    if($TName=~/\([\*]+\)/)
    {
        $TName=~s/\(([\*]+)\)/\($1$Member\)/;
        return $TName;
    }
    else
    {
        my @ArraySizes = ();
        while($TName=~s/(\[[^\[\]]*\])\Z//) {
            push(@ArraySizes, $1);
        }
        return $TName." ".$Member.join("", @ArraySizes);
    }
}

sub assembleStruct(@)
{
    my %Init_Desc = @_;
    my %Type_Init = ();
    my %Type = get_Type($Tid_TDid{$Init_Desc{"TypeId"}}, $Init_Desc{"TypeId"});
    my $Type_PointerLevel = get_PointerLevel($Tid_TDid{$Init_Desc{"TypeId"}}, $Init_Desc{"TypeId"});
    my $StructId = get_FoundationTypeId($Init_Desc{"TypeId"});
    my $StructName = get_TypeName($StructId);
    return () if($OpaqueTypes{$StructName});
    my %Struct = get_Type($Tid_TDid{$StructId}, $StructId);
    return () if(not keys(%{$Struct{"Memb"}}));
    my $Global_State = save_state();
    $Init_Desc{"Var"} = select_var_name($LongVarNames?$Init_Desc{"Key"}:$Init_Desc{"ParamName"}, $Init_Desc{"ParamNameExt"});
    my $Var = $Init_Desc{"Var"};
    if($Type_PointerLevel>0 or $Type{"Type"} eq "Ref"
    or not $Init_Desc{"InLine"}) {
        $Block_Variable{$CurrentBlock}{$Var} = 1;
    }
    $Type_Init{"Headers"} = addHeaders([$Struct{"Header"}], $Type_Init{"Headers"});
    my @ParamStr = ();
    foreach my $Member_Pos (sort {int($a)<=>int($b)} keys(%{$Struct{"Memb"}}))
    {# initialize members
        my $Member_Name = $Struct{"Memb"}{$Member_Pos}{"name"};
        if(getIntLang($TestedInterface) eq "C")
        {
            if($Member_Name eq "c_class"
            and $StructName=~/\A(struct |)(XWindowAttributes|Visual|XVisualInfo)\Z/)
            {# for X11
                $Member_Name = "class";
            }
            elsif($Member_Name eq "c_explicit"
            and $StructName=~/\A(struct |)(_XkbServerMapRec)\Z/)
            {# for X11
                $Member_Name = "explicit";
            }
            elsif($Member_Name=~/\A(__|)fds_bits\Z/ and $StructName eq "fd_set")
            {# for libc
                if(defined $Constants{"__USE_XOPEN"}) {
                    $Member_Name = "fds_bits";
                }
                else {
                    $Member_Name = "__fds_bits";
                }
            }
        }
        my $MemberType_Id = $Struct{"Memb"}{$Member_Pos}{"type"};
        my $MemberFType_Id = get_FoundationTypeId($MemberType_Id);
        if(get_TypeType($MemberFType_Id) eq "Array")
        {
            my $ArrayElemType_Id = get_FoundationTypeId(get_OneStep_BaseTypeId($Tid_TDid{$MemberFType_Id}, $MemberFType_Id));
            if(get_TypeType($ArrayElemType_Id)=~/\A(Intrinsic|Enum)\Z/)
            {
                if(get_TypeSize($MemberFType_Id)>1024) {
                    next;
                }
            }
            else
            {
                if(get_TypeSize($MemberFType_Id)>256) {
                    next;
                }
            }
        }
        my $Member_Access = $Struct{"Memb"}{$Member_Pos}{"access"};
        #return () if($Member_Access eq "private" or $Member_Access eq "protected");
        my $Memb_Key = "";
        if($Member_Name) {
            $Memb_Key = ($Init_Desc{"Key"})?$Init_Desc{"Key"}."_".$Member_Name:$Member_Name;
        }
        else {
            $Memb_Key = ($Init_Desc{"Key"})?$Init_Desc{"Key"}."_".($Member_Pos+1):"m".($Member_Pos+1);
        }
        my %Memb_Init = initializeParameter((
            "TypeId" => $MemberType_Id,
            "Key" => $Memb_Key,
            "InLine" => 1,
            "Value" => "no value",
            "ValueTypeId" => 0,
            "TargetTypeId" => 0,
            "CreateChild" => 0,
            "Usage" => "Common",
            "ParamName" => $Member_Name,
            "OuterType_Type" => "Struct",
            "OuterType_Id" => $StructId));
        if(not $Memb_Init{"IsCorrect"}) {
            restore_state($Global_State);
            return ();
        }
        $Type_Init{"Code"} .= $Memb_Init{"Code"};
        $Type_Init{"Headers"} = addHeaders($Memb_Init{"Headers"}, $Type_Init{"Headers"});
        $Memb_Init{"Call"} = alignCode($Memb_Init{"Call"}, get_paragraph($Memb_Init{"Call"}, 1)."    ", 1);
        if(getIntLang($TestedInterface) eq "C" and $OSgroup ne "windows") {
            @ParamStr = (@ParamStr, "\.$Member_Name = ".$Memb_Init{"Call"});
        }
        else {
            @ParamStr = (@ParamStr, $Memb_Init{"Call"});
        }
        $Type_Init{"Init"} .= $Memb_Init{"Init"};
        $Type_Init{"Destructors"} .= $Memb_Init{"Destructors"};
    }
    if(my $Typedef_Id = get_type_typedef($StructId)) {
        $StructName = get_TypeName($Typedef_Id);
    }
    #initialization
    if($Type_PointerLevel==0 and ($Type{"Type"} ne "Ref") and $Init_Desc{"InLine"})
    {
        my $Conversion = (isNotAnon($StructName) and isNotAnon($Struct{"Name_Old"}))?"(".$Type{"Name"}.") ":"";
        $Type_Init{"Call"} = $Conversion."{".create_list(\@ParamStr, "    ")."}";
        $Type_Init{"TypeName"} = $Type{"Name"};
    }
    else
    {
        if(not defined $DisableReuse) {
            $ValueCollection{$CurrentBlock}{$Var} = $StructId;
        }
        if(isAnon($StructName))
        {
            my ($AnonStruct_Declarations, $AnonStruct_Headers) = declare_anon_struct($LongVarNames?$Init_Desc{"Key"}:$Init_Desc{"ParamName"}, $StructId);
            $Type_Init{"Code"} .= $AnonStruct_Declarations;
            $Type_Init{"Headers"} = addHeaders($AnonStruct_Headers, $Type_Init{"Headers"});
            $Type_Init{"Init"} .= get_TypeName($StructId)." $Var = {".create_list(\@ParamStr, "    ")."};\n";
            $Type_Init{"TypeName"} = get_TypeName($StructId);
            foreach (1 .. $Type_PointerLevel) {
                $Type_Init{"TypeName"} .= "*";
            }
        }
        else
        {
            $Type_Init{"Init"} .= $StructName." $Var = {".create_list(\@ParamStr, "    ")."};\n";
            $Type_Init{"TypeName"} = $Type{"Name"};
        }
        # create call
        my ($Call, $TmpPreamble) =
        convert_familiar_types((
            "InputTypeName"=>get_TypeName($StructId),
            "InputPointerLevel"=>0,
            "OutputTypeId"=>($Init_Desc{"TypeId_Changed"})?$Init_Desc{"TypeId_Changed"}:$Init_Desc{"TypeId"},
            "Value"=>$Var,
            "Key"=>$Var,
            "Destination"=>"Param",
            "MustConvert"=>0));
        $Type_Init{"Init"} .= $TmpPreamble;
        $Type_Init{"Call"} = $Call;
        # create call for constraint
        if($Init_Desc{"TargetTypeId"}==$Init_Desc{"TypeId"}) {
            $Type_Init{"TargetCall"} = $Type_Init{"Call"};
        }
        else
        {
            my ($TargetCall, $Target_TmpPreamble) =
            convert_familiar_types((
                "InputTypeName"=>get_TypeName($StructId),
                "InputPointerLevel"=>0,
                "OutputTypeId"=>$Init_Desc{"TargetTypeId"},
                "Value"=>$Var,
                "Key"=>$Var,
                "Destination"=>"Target",
                "MustConvert"=>0));
            $Type_Init{"TargetCall"} = $TargetCall;
            $Type_Init{"Init"} .= $Target_TmpPreamble;
        }
        #ref handler
        if($Type{"Type"} eq "Ref")
        {
            my $BaseRefId = get_OneStep_BaseTypeId($Tid_TDid{$Init_Desc{"TypeId"}}, $Init_Desc{"TypeId"});
            if(get_PointerLevel($Tid_TDid{$BaseRefId}, $BaseRefId) > 0)
            {
                my $BaseRefName = get_TypeName($BaseRefId);
                $Type_Init{"Init"} .= $BaseRefName." ".$Var."_ref = ".$Type_Init{"Call"}.";\n";
                $Type_Init{"Call"} = $Var."_ref";
                $Block_Variable{$CurrentBlock}{$Var."_ref"} = 1;
                if(not defined $DisableReuse) {
                    $ValueCollection{$CurrentBlock}{$Var."_ref"} = $Init_Desc{"TypeId"};
                }
            }
        }
    }
    $Type_Init{"IsCorrect"} = 1;
    return %Type_Init;
}

sub getSomeEnumMember($)
{
    my $EnumId = $_[0];
    my %Enum = get_Type($Tid_TDid{$EnumId}, $EnumId);
    return "" if(not keys(%{$Enum{"Memb"}}));
    my @Members = ();
    foreach my $MembPos (sort{int($a)<=>int($b)} keys(%{$Enum{"Memb"}})) {
        push(@Members, $Enum{"Memb"}{$MembPos}{"name"});
    }
    if($RandomCode) {
        @Members = mix_array(@Members);
    }
    my @ValidMembers = ();
    foreach my $Member (@Members)
    {
        if(is_valid_constant($Member)) {
            push(@ValidMembers, $Member);
        }
    }
    my $MemberName = $Members[0];
    if($#ValidMembers>=0) {
        $MemberName = $ValidMembers[0];
    }
    if($Enum{"NameSpace"} and $MemberName
    and getIntLang($TestedInterface) eq "C++") {
        $MemberName = $Enum{"NameSpace"}."::".$MemberName;
    }
    return $MemberName;
}

sub getEnumMembers($)
{
    my $EnumId = $_[0];
    my %Enum = get_Type($Tid_TDid{$EnumId}, $EnumId);
    return () if(not keys(%{$Enum{"Memb"}}));
    my @Members = ();
    foreach my $MembPos (sort{int($a)<=>int($b)} keys(%{$Enum{"Memb"}})) {
        push(@Members, $Enum{"Memb"}{$MembPos}{"name"});
    }
    return \@Members;
}

sub add_NullSpecType(@)
{
    my %Init_Desc = @_;
    my %NewInit_Desc = %Init_Desc;
    my $PointerLevel = get_PointerLevel($Tid_TDid{$Init_Desc{"TypeId"}}, $Init_Desc{"TypeId"});
    my $TypeName = get_TypeName($Init_Desc{"TypeId"});
    if($TypeName=~/\&/ or not $Init_Desc{"InLine"}) {
        $NewInit_Desc{"InLine"} = 0;
    }
    else {
        $NewInit_Desc{"InLine"} = 1;
    }
    if($PointerLevel>=1)
    {
        if($Init_Desc{"OuterType_Type"}!~/\A(Struct|Union|Array)\Z/
        and (isOutParam_NoUsing($Init_Desc{"TypeId"}, $Init_Desc{"ParamName"}, $Init_Desc{"Interface"})
        or $Interface_OutParam{$Init_Desc{"Interface"}}{$Init_Desc{"ParamName"}}
        or $Interface_OutParam_NoUsing{$Init_Desc{"Interface"}}{$Init_Desc{"ParamName"}} or $PointerLevel>=2))
        {
            $NewInit_Desc{"InLine"} = 0;
            $NewInit_Desc{"ValueTypeId"} = reduce_pointer_level($Init_Desc{"TypeId"});
            if($PointerLevel>=2) {
                $NewInit_Desc{"Value"} = get_null();
            }
            else {
                $NewInit_Desc{"OnlyDecl"} = 1;
            }
        }
        else
        {
            $NewInit_Desc{"Value"} = get_null();
            $NewInit_Desc{"ValueTypeId"} = $Init_Desc{"TypeId"};
            $NewInit_Desc{"ByNull"}=1;
        }
    }
    else {
        $NewInit_Desc{"Value"} = "no value";
    }
    return %NewInit_Desc;
}

sub initializeIntrinsic(@)
{
    my %Init_Desc = @_;
    $Init_Desc{"StrongTypeCompliance"} = 1;
    my %Type_Init = initializeByInterface(%Init_Desc);
    if($Type_Init{"IsCorrect"}) {
        return %Type_Init;
    }
    else {
        return initializeByInterface_OutParam(%Init_Desc);
    }
}

sub initializeRetVal(@)
{
    my %Init_Desc = @_;
    return () if(get_TypeName($Init_Desc{"TypeId"}) eq "void*");
    my %Type_Init = initializeByInterface(%Init_Desc);
    if($Type_Init{"IsCorrect"}) {
        return %Type_Init;
    }
    else {
        return initializeByInterface_OutParam(%Init_Desc);
    }
}

sub initializeEnum(@)
{
    my %Init_Desc = @_;
    return initializeByInterface(%Init_Desc);
}

sub is_geometry_body($)
{
    my $TypeId = $_[0];
    return 0 if(not $TypeId);
    my $StructId = get_FoundationTypeId($TypeId);
    my %Struct = get_Type($Tid_TDid{$StructId}, $StructId);
    return 0 if($Struct{"Name"}!~/rectangle|line/i);
    return 0 if($Struct{"Type"} ne "Struct");
    foreach my $Member_Pos (sort {int($a)<=>int($b)} keys(%{$Struct{"Memb"}}))
    {
        if(get_TypeType(get_FoundationTypeId($Struct{"Memb"}{$Member_Pos}{"type"}))!~/\A(Intrinsic|Enum)\Z/) {
            return 0;
        }
    }
    return 1;
}

sub initializeUnion(@)
{
    my %Init_Desc = @_;
    $Init_Desc{"Strong"}=1;
    my %Type_Init = initializeByInterface_OutParam(%Init_Desc);
    if($Type_Init{"IsCorrect"}) {
        return %Type_Init;
    }
    else
    {
        delete($Init_Desc{"Strong"});
        %Type_Init = initializeByInterface(%Init_Desc);
        if($Type_Init{"IsCorrect"}) {
            return %Type_Init;
        }
        else
        {
            %Type_Init = assembleUnion(%Init_Desc);
            if($Type_Init{"IsCorrect"}) {
                return %Type_Init;
            }
            else {
                return initializeByInterface_OutParam(%Init_Desc);
            }
        }
    }
}

sub initializeStruct(@)
{
    my %Init_Desc = @_;
    if(is_geometry_body($Init_Desc{"TypeId"}))
    {# GdkRectangle
        return assembleStruct(%Init_Desc);
    }
#     $Init_Desc{"Strong"}=1;
#     my %Type_Init = initializeByInterface_OutParam(%Init_Desc);
#     if($Type_Init{"IsCorrect"})
#     {
#         return %Type_Init;
#     }
#     else
#     {
#         delete($Init_Desc{"Strong"});
    $Init_Desc{"OnlyReturn"}=1;
    my %Type_Init = initializeByInterface(%Init_Desc);
    if($Type_Init{"IsCorrect"}) {
        return %Type_Init;
    }
    else
    {
        return () if($Init_Desc{"OnlyByInterface"});
        delete($Init_Desc{"OnlyReturn"});
        %Type_Init = initializeByInterface_OutParam(%Init_Desc);
        if($Type_Init{"IsCorrect"}) {
            return %Type_Init;
        }
        else
        {
            $Init_Desc{"OnlyData"}=1;
            %Type_Init = initializeByInterface(%Init_Desc);
            if($Type_Init{"IsCorrect"}) {
                return %Type_Init;
            }
            else
            {
                delete($Init_Desc{"OnlyData"});
                %Type_Init = initializeByAlienInterface(%Init_Desc);
                if($Type_Init{"IsCorrect"}) {
                    return %Type_Init;
                }
                else
                {
                    %Type_Init = initializeSubClass_Struct(%Init_Desc);
                    if($Type_Init{"IsCorrect"}) {
                        return %Type_Init;
                    }
                    else
                    {
                        if($Init_Desc{"DoNotAssembly"}) {
                            return initializeByField(%Init_Desc);
                        }
                        else
                        {
                            %Type_Init = assembleStruct(%Init_Desc);
                            if($Type_Init{"IsCorrect"}) {
                                return %Type_Init;
                            }
                            else
                            {
                                %Type_Init = assembleClass(%Init_Desc);
                                if($Type_Init{"IsCorrect"}) {
                                    return %Type_Init;
                                }
                                else {
                                    return initializeByField(%Init_Desc);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

sub initializeByAlienInterface(@)
{# GtkWidget*  gtk_plug_new (GdkNativeWindow socket_id)
 # return GtkPlug*
    my %Init_Desc = @_;
    if($Init_Desc{"ByInterface"} = find_alien_interface($Init_Desc{"TypeId"}))
    {
        my %Type_Init = initializeByInterface(%Init_Desc);
        if(not $Type_Init{"ByNull"}) {
            return %Type_Init;
        }
    }
    return ();
}

sub find_alien_interface($)
{
    my $TypeId = $_[0];
    return "" if(not $TypeId);
    return "" if(get_PointerLevel($Tid_TDid{$TypeId}, $TypeId)!=1);
    my $StructId = get_FoundationTypeId($TypeId);
    return "" if(get_TypeType($StructId) ne "Struct");
    my $Desirable = get_TypeName($StructId);
    $Desirable=~s/\Astruct //g;
    $Desirable=~s/\A[_]+//g;
    while($Desirable=~s/([a-z]+)([A-Z][a-z]+)/$1_$2/g){};
    $Desirable = lc($Desirable);
    my @Cnadidates = ($Desirable."_new", $Desirable."_create");
    foreach my $Candiate (@Cnadidates)
    {
        if(defined $CompleteSignature{$Candiate}
        and $CompleteSignature{$Candiate}{"Header"}
        and get_PointerLevel($Tid_TDid{$CompleteSignature{$Candiate}{"Return"}}, $CompleteSignature{$Candiate}{"Return"})==1)  {
            return $Candiate;
        }
    }
    return "";
}

sub initializeByField(@)
{# FIXME: write body of this function
    my %Init_Desc = @_;
    my @Structs = keys(%{$Member_Struct{$Init_Desc{"TypeId"}}});
    return ();
}

sub initializeSubClass_Struct(@)
{
    my %Init_Desc = @_;
    $Init_Desc{"TypeId_Changed"} = $Init_Desc{"TypeId"} if(not $Init_Desc{"TypeId_Changed"});
    my $StructId = get_FoundationTypeId($Init_Desc{"TypeId"});
    my $StructName = get_TypeName($StructId);
    my $PLevel = get_PointerLevel($Tid_TDid{$Init_Desc{"TypeId"}}, $Init_Desc{"TypeId"});
    return () if(get_TypeType($StructId) ne "Struct" or $PLevel==0);
    foreach my $SubClassId (keys(%{$Struct_SubClasses{$StructId}}))
    {
        $Init_Desc{"TypeId"} = get_TypeId($SubClassId, $PLevel);
        next if(not $Init_Desc{"TypeId"});
        $Init_Desc{"DoNotAssembly"} = 1;
        my %Type_Init = initializeType(%Init_Desc);
        if($Type_Init{"IsCorrect"}) {
            return %Type_Init;
        }
    }
    if(my $ParentId = get_TypeId($Struct_Parent{$StructId}, $PLevel))
    {
        $Init_Desc{"TypeId"} = $ParentId;
        $Init_Desc{"DoNotAssembly"} = 1;
        $Init_Desc{"OnlyByInterface"} = 1;
        $Init_Desc{"KeyWords"} = $StructName;
        $Init_Desc{"KeyWords"}=~s/\Astruct //;
        my %Type_Init = initializeType(%Init_Desc);
        if($Type_Init{"IsCorrect"}
        and (not $Type_Init{"Interface"} or get_word_coinsidence($Type_Init{"Interface"}, $Init_Desc{"KeyWords"})>0)) {
            return %Type_Init;
        }
    }
}

sub get_TypeId($$)
{
    my ($BaseTypeId, $PLevel) = @_;
    return 0 if(not $BaseTypeId);
    if(my @DerivedTypes = sort {length($a)<=>length($b)}
    keys(%{$BaseType_PLevel_Type{$BaseTypeId}{$PLevel}})) {
        return $DerivedTypes[0];
    }
    elsif(my $NewTypeId = register_new_type($BaseTypeId, $PLevel)) {
        return $NewTypeId;
    }
    else {
        return 0;
    }
}

sub assembleUnion(@)
{
    my %Init_Desc = @_;
    my %Type_Init = ();
    my %Type = get_Type($Tid_TDid{$Init_Desc{"TypeId"}}, $Init_Desc{"TypeId"});
    my $Type_PointerLevel = get_PointerLevel($Tid_TDid{$Init_Desc{"TypeId"}}, $Init_Desc{"TypeId"});
    my $UnionId = get_FoundationTypeId($Init_Desc{"TypeId"});
    my %UnionType = get_Type($Tid_TDid{$UnionId}, $UnionId);
    my $UnionName = $UnionType{"Name"};
    return () if($OpaqueTypes{$UnionName});
    return () if(not keys(%{$UnionType{"Memb"}}));
    my $Global_State = save_state();
    $Init_Desc{"Var"} = select_var_name($LongVarNames?$Init_Desc{"Key"}:$Init_Desc{"ParamName"}, $Init_Desc{"ParamNameExt"});
    my $Var = $Init_Desc{"Var"};
    if($Type_PointerLevel>0 or $Type{"Type"} eq "Ref"
    or not $Init_Desc{"InLine"}) {
        $Block_Variable{$CurrentBlock}{$Var} = 1;
    }
    $Type_Init{"Headers"} = addHeaders([$UnionType{"Header"}], $Type_Init{"Headers"});
    my (%Memb_Init, $SelectedMember_Name) = ();
    foreach my $Member_Pos (sort {int($a)<=>int($b)} keys(%{$UnionType{"Memb"}}))
    {#initialize members
        my $Member_Name = $UnionType{"Memb"}{$Member_Pos}{"name"};
        my $MemberType_Id = $UnionType{"Memb"}{$Member_Pos}{"type"};
        my $Memb_Key = "";
        if($Member_Name) {
            $Memb_Key = ($Init_Desc{"Key"})?$Init_Desc{"Key"}."_".$Member_Name:$Member_Name;
        }
        else {
            $Memb_Key = ($Init_Desc{"Key"})?$Init_Desc{"Key"}."_".($Member_Pos+1):"m".($Member_Pos+1);
        }
        %Memb_Init = initializeParameter((
            "TypeId" => $MemberType_Id,
            "Key" => $Memb_Key,
            "InLine" => 1,
            "Value" => "no value",
            "ValueTypeId" => 0,
            "TargetTypeId" => 0,
            "CreateChild" => 0,
            "Usage" => "Common",
            "ParamName" => $Member_Name,
            "OuterType_Type" => "Union",
            "OuterType_Id" => $UnionId));
        next if(not $Memb_Init{"IsCorrect"});
        $SelectedMember_Name = $Member_Name;
        last;
    }
    if(not $Memb_Init{"IsCorrect"})
    {
        restore_state($Global_State);
        return ();
    }
    $Type_Init{"Code"} .= $Memb_Init{"Code"};
    $Type_Init{"Headers"} = addHeaders($Memb_Init{"Headers"}, $Type_Init{"Headers"});
    $Type_Init{"Init"} .= $Memb_Init{"Init"};
    $Type_Init{"Destructors"} .= $Memb_Init{"Destructors"};
    $Memb_Init{"Call"} = alignCode($Memb_Init{"Call"}, get_paragraph($Memb_Init{"Call"}, 1)."    ", 1);
    if(my $Typedef_Id = get_type_typedef($UnionId)) {
        $UnionName = get_TypeName($Typedef_Id);
    }
    #initialization
    if($Type_PointerLevel==0 and ($Type{"Type"} ne "Ref") and $Init_Desc{"InLine"})
    {
        my $Conversion = (isNotAnon($UnionName) and isNotAnon($UnionType{"Name_Old"}))?"(".$Type{"Name"}.") ":"";
        if($TestedInterface=~/\A(_Z|\?)/) { # C++
            $Type_Init{"Call"} = $Conversion."{".$Memb_Init{"Call"}."}";
        }
        else {
            $Type_Init{"Call"} = $Conversion."{\.$SelectedMember_Name = ".$Memb_Init{"Call"}."}";
        }
        $Type_Init{"TypeName"} = $Type{"Name"};
    }
    else
    {
        if(not defined $DisableReuse) {
            $ValueCollection{$CurrentBlock}{$Var} = $UnionId;
        }
        if(isAnon($UnionName))
        {
            my ($AnonUnion_Declarations, $AnonUnion_Headers) = declare_anon_union($LongVarNames?$Init_Desc{"Key"}:$Init_Desc{"ParamName"}, $UnionId);
            $Type_Init{"Code"} .= $AnonUnion_Declarations;
            $Type_Init{"Headers"} = addHeaders($AnonUnion_Headers, $Type_Init{"Headers"});
            if($TestedInterface=~/\A(_Z|\?)/) { # C++
                $Type_Init{"Init"} .= get_TypeName($UnionId)." $Var = {".$Memb_Init{"Call"}."};\n";
            }
            else {
                $Type_Init{"Init"} .= get_TypeName($UnionId)." $Var = {\.$SelectedMember_Name = ".$Memb_Init{"Call"}."};\n";
            }
            $Type_Init{"TypeName"} = "union ".get_TypeName($UnionId);
            foreach (1 .. $Type_PointerLevel) {
                $Type_Init{"TypeName"} .= "*";
            }
        }
        else
        {
            if($TestedInterface=~/\A(_Z|\?)/) { # C++
                $Type_Init{"Init"} .= $UnionName." $Var = {".$Memb_Init{"Call"}."};\n";
            }
            else {
                $Type_Init{"Init"} .= $UnionName." $Var = {\.$SelectedMember_Name = ".$Memb_Init{"Call"}."};\n";
            }
            $Type_Init{"TypeName"} = $Type{"Name"};
        }
        #create call
        my ($Call, $TmpPreamble) =
        convert_familiar_types((
            "InputTypeName"=>get_TypeName($UnionId),
            "InputPointerLevel"=>0,
            "OutputTypeId"=>($Init_Desc{"TypeId_Changed"})?$Init_Desc{"TypeId_Changed"}:$Init_Desc{"TypeId"},
            "Value"=>$Var,
            "Key"=>$Var,
            "Destination"=>"Param",
            "MustConvert"=>0));
        $Type_Init{"Init"} .= $TmpPreamble;
        $Type_Init{"Call"} = $Call;
        #create call in constraint
        if($Init_Desc{"TargetTypeId"}==$Init_Desc{"TypeId"}) {
            $Type_Init{"TargetCall"} = $Type_Init{"Call"};
        }
        else
        {
            my ($TargetCall, $Target_TmpPreamble) =
            convert_familiar_types((
                "InputTypeName"=>get_TypeName($UnionId),
                "InputPointerLevel"=>0,
                "OutputTypeId"=>$Init_Desc{"TargetTypeId"},
                "Value"=>$Var,
                "Key"=>$Var,
                "Destination"=>"Target",
                "MustConvert"=>0));
            $Type_Init{"TargetCall"} = $TargetCall;
            $Type_Init{"Init"} .= $Target_TmpPreamble;
        }
        #ref handler
        if($Type{"Type"} eq "Ref")
        {
            my $BaseRefId = get_OneStep_BaseTypeId($Tid_TDid{$Init_Desc{"TypeId"}}, $Init_Desc{"TypeId"});
            if(get_PointerLevel($Tid_TDid{$BaseRefId}, $BaseRefId) > 0)
            {
                my $BaseRefName = get_TypeName($BaseRefId);
                $Type_Init{"Init"} .= $BaseRefName." ".$Var."_ref = ".$Type_Init{"Call"}.";\n";
                $Type_Init{"Call"} = $Var."_ref";
                $Block_Variable{$CurrentBlock}{$Var."_ref"} = 1;
                if(not defined $DisableReuse) {
                    $ValueCollection{$CurrentBlock}{$Var."_ref"} = $Init_Desc{"TypeId"};
                }
            }
        }
    }
    $Type_Init{"IsCorrect"} = 1;
    return %Type_Init;
}

sub initializeClass(@)
{
    my %Init_Desc = @_;
    my %Type_Init = ();
    if($Init_Desc{"CreateChild"})
    {
        $Init_Desc{"InheritingPriority"} = "High";
        return assembleClass(%Init_Desc);
    }
    else
    {
        if((get_TypeType($Init_Desc{"TypeId"}) eq "Typedef"))
        {# try to initialize typedefs by interface return value
            %Type_Init = initializeByInterface(%Init_Desc);
            if($Type_Init{"IsCorrect"}) {
                return %Type_Init;
            }
        }
        $Init_Desc{"InheritingPriority"} = "Low";
        %Type_Init = assembleClass(%Init_Desc);
        if($Type_Init{"IsCorrect"}) {
            return %Type_Init;
        }
        else
        {
            if(isAbstractClass(get_FoundationTypeId($Init_Desc{"TypeId"})))
            {
                $Init_Desc{"InheritingPriority"} = "High";
                %Type_Init = assembleClass(%Init_Desc);
                if($Type_Init{"IsCorrect"}) {
                    return %Type_Init;
                }
                else {
                    return initializeByInterface(%Init_Desc);
                }
            }
            else
            {
                %Type_Init = initializeByInterface(%Init_Desc);
                if($Type_Init{"IsCorrect"}) {
                    return %Type_Init;
                }
                else
                {
                    $Init_Desc{"InheritingPriority"} = "High";
                    %Type_Init = assembleClass(%Init_Desc);
                    if($Type_Init{"IsCorrect"}) {
                        return %Type_Init;
                    }
                    else {
                        return initializeByInterface_OutParam(%Init_Desc);
                    }
                }
            }
        }
    }
}

sub has_public_destructor($$)
{
    my ($ClassId, $DestrType) = @_;
    my $ClassName = get_TypeName($ClassId);
    return $Cache{"has_public_destructor"}{$ClassId}{$DestrType} if($Cache{"has_public_destructor"}{$ClassId}{$DestrType});
    foreach my $Destructor (sort keys(%{$Class_Destructors{$ClassId}}))
    {
        if($Destructor=~/\Q$DestrType\E/)
        {
            if(not $CompleteSignature{$Destructor}{"Protected"})
            {
                $Cache{"has_public_destructor"}{$ClassId}{$DestrType} = $Destructor;
                return $Destructor;
            }
            else {
                return "";
            }
        }
    }
    $Cache{"has_public_destructor"}{$ClassId}{$DestrType} = "Default";
    return "Default";
}

sub findConstructor($$)
{
    my ($ClassId, $Key) = @_;
    return () if(not $ClassId);
    foreach my $Constructor (get_CompatibleInterfaces($ClassId, "Construct", ""))
    {
        my %Interface_Init = callInterfaceParameters((
            "Interface"=>$Constructor,
            "Key"=>$Key,
            "ObjectCall"=>"no object"));
        if($Interface_Init{"IsCorrect"})
        {
            $Interface_Init{"Interface"} = $Constructor;
            return %Interface_Init;
        }
    }
    return ();
}

sub assembleClass(@)
{
    my %Init_Desc = @_;
    my %Type_Init = ();
    my $Global_State = save_state();
    my $CreateDestructor = 1;
    $Type_Init{"TypeName"} = get_TypeName($Init_Desc{"TypeId"});
    my $ClassId = get_FoundationTypeId($Init_Desc{"TypeId"});
    my $ClassName = get_TypeName($ClassId);
    my $PointerLevel = get_PointerLevel($Tid_TDid{$Init_Desc{"TypeId"}}, $Init_Desc{"TypeId"});
    $Init_Desc{"Var"} = select_var_name($LongVarNames?$Init_Desc{"Key"}:$Init_Desc{"ParamName"}, $Init_Desc{"ParamNameExt"});
    my $Var = $Init_Desc{"Var"};
    $Block_Variable{$CurrentBlock}{$Var} = 1;
    my %Obj_Init = findConstructor($ClassId, $Init_Desc{"Key"});
    if(not $Obj_Init{"IsCorrect"}) {
        restore_state($Global_State);
        return ();
    }
    $Type_Init{"Init"} = $Obj_Init{"Init"};
    $Type_Init{"Destructors"} = $Obj_Init{"Destructors"};
    $Type_Init{"Code"} = $Obj_Init{"Code"};
    $Type_Init{"Headers"} = addHeaders($Obj_Init{"Headers"}, $Type_Init{"Headers"});
    my $NeedToInheriting = (isAbstractClass($ClassId) or $Init_Desc{"CreateChild"} or isNotInCharge($Obj_Init{"Interface"}) or $CompleteSignature{$Obj_Init{"Interface"}}{"Protected"});
    if($Init_Desc{"InheritingPriority"} eq "Low"
    and $NeedToInheriting) {
        restore_state($Global_State);
        return ();
    }
    my $HeapStack = (($PointerLevel eq 0) and has_public_destructor($ClassId, "D1") and not $Init_Desc{"ObjectInit"} and (not $Init_Desc{"RetVal"} or get_TypeType($Init_Desc{"TypeId"}) ne "Ref"))?"Stack":"Heap";
    my $ChildName = getSubClassName($ClassName);
    if($NeedToInheriting)
    {
        if($Obj_Init{"Call"}=~/\A(\Q$ClassName\E([\n]*)\()/) {
            substr($Obj_Init{"Call"}, index($Obj_Init{"Call"}, $1), pos($1) + length($1)) = $ChildName.$2."(";
        }
        $UsedConstructors{$ClassId}{$Obj_Init{"Interface"}} = 1;
        $IntSubClass{$TestedInterface}{$ClassId} = 1;
        $Create_SubClass{$ClassId} = 1;
        $SubClass_Instance{$Var} = 1;
        $SubClass_ObjInstance{$Var} = 1 if($Init_Desc{"ObjectInit"});
    }
    my %AutoFinalCode_Init = ();
    my $Typedef_Id = detect_typedef($Init_Desc{"TypeId"});
    if(get_TypeName($ClassId)=~/list/i or get_TypeName($Typedef_Id)=~/list/i)
    {# auto final code
        %AutoFinalCode_Init = get_AutoFinalCode($Obj_Init{"Interface"}, ($HeapStack eq "Stack")?$Var:"*".$Var);
        if($AutoFinalCode_Init{"IsCorrect"}) {
            $Init_Desc{"InLine"} = 0;
        }
    }
    if($Obj_Init{"PreCondition"}
    or $Obj_Init{"PostCondition"}) {
        $Init_Desc{"InLine"} = 0;
    }
    # check precondition
    if($Obj_Init{"PreCondition"}) {
        $Type_Init{"Init"} .= $Obj_Init{"PreCondition"}."\n";
    }
    if($HeapStack eq "Stack")
    {
        $CreateDestructor = 0;
        if($Init_Desc{"InLine"} and ($PointerLevel eq 0))
        {
            $Type_Init{"Call"} = $Obj_Init{"Call"};
            $Type_Init{"TargetCall"} = $Type_Init{"Call"};
            delete($Block_Variable{$CurrentBlock}{$Var});
        }
        else
        {
            if(not defined $DisableReuse) {
                $ValueCollection{$CurrentBlock}{$Var} = $ClassId;
            }
            # $Type_Init{"Init"} .= "//parameter initialization\n";
            my $ConstructedName = ($NeedToInheriting)?$ChildName:$ClassName;
            $Type_Init{"Init"} .= correct_init_stmt($ConstructedName." $Var = ".$Obj_Init{"Call"}.";\n", $ConstructedName, $Var);
            # create call
            my ($Call, $TmpPreamble) =
            convert_familiar_types((
                "InputTypeName"=>$ConstructedName,
                "InputPointerLevel"=>0,
                "OutputTypeId"=>($Init_Desc{"TypeId_Changed"})?$Init_Desc{"TypeId_Changed"}:$Init_Desc{"TypeId"},
                "Value"=>$Var,
                "Key"=>$Var,
                "Destination"=>"Param",
                "MustConvert"=>0));
            $Type_Init{"Init"} .= $TmpPreamble;
            $Type_Init{"Call"} = $Call;
            #call to constraint
            if($Init_Desc{"TargetTypeId"}==$Init_Desc{"TypeId"}) {
                $Type_Init{"TargetCall"} = $Type_Init{"Call"};
            }
            else
            {
                my ($TargetCall, $Target_TmpPreamble) =
                convert_familiar_types((
                    "InputTypeName"=>$ConstructedName,
                    "InputPointerLevel"=>0,
                    "OutputTypeId"=>$Init_Desc{"TargetTypeId"},
                    "Value"=>$Var,
                    "Key"=>$Var,
                    "Destination"=>"Target",
                    "MustConvert"=>0));
                $Type_Init{"TargetCall"} = $TargetCall;
                $Type_Init{"Init"} .= $Target_TmpPreamble;
            }
        }
    }
    elsif($HeapStack eq "Heap")
    {
        if($Init_Desc{"InLine"} and ($PointerLevel eq 1))
        {
            $Type_Init{"Call"} = "new ".$Obj_Init{"Call"};
            $Type_Init{"TargetCall"} = $Type_Init{"Call"};
            $CreateDestructor = 0;
            delete($Block_Variable{$CurrentBlock}{$Var});
        }
        else
        {
            if(not defined $DisableReuse) {
                $ValueCollection{$CurrentBlock}{$Var} = get_TypeIdByName("$ClassName*");
            }
            #$Type_Init{"Init"} .= "//parameter initialization\n";
            if($NeedToInheriting)
            {
                if($Init_Desc{"ConvertToBase"}) {
                    $Type_Init{"Init"} .= $ClassName."* $Var = ($ClassName*)new ".$Obj_Init{"Call"}.";\n";
                }
                else {
                    $Type_Init{"Init"} .= $ChildName."* $Var = new ".$Obj_Init{"Call"}.";\n";
                }
            }
            else {
                $Type_Init{"Init"} .= $ClassName."* $Var = new ".$Obj_Init{"Call"}.";\n";
            }
            #create call
            my ($Call, $TmpPreamble) =
            convert_familiar_types((
                "InputTypeName"=>"$ClassName*",
                "InputPointerLevel"=>1,
                "OutputTypeId"=>($Init_Desc{"TypeId_Changed"})?$Init_Desc{"TypeId_Changed"}:$Init_Desc{"TypeId"},
                "Value"=>$Var,
                "Key"=>$Var,
                "Destination"=>"Param",
                "MustConvert"=>0));
            $Type_Init{"Init"} .= $TmpPreamble;
            $Type_Init{"Call"} = $Call;
            #call to constraint
            if($Init_Desc{"TargetTypeId"}==$Init_Desc{"TypeId"}) {
                $Type_Init{"TargetCall"} = $Type_Init{"Call"};
            }
            else
            {
                my ($TargetCall, $Target_TmpPreamble) =
                convert_familiar_types((
                    "InputTypeName"=>"$ClassName*",
                    "InputPointerLevel"=>1,
                    "OutputTypeId"=>$Init_Desc{"TargetTypeId"},
                    "Value"=>$Var,
                    "Key"=>$Var,
                    "Destination"=>"Target",
                    "MustConvert"=>0));
                $Type_Init{"TargetCall"} = $TargetCall;
                $Type_Init{"Init"} .= $Target_TmpPreamble;
            }
        }
        #destructor for object
        if($CreateDestructor) #mayCallDestructors($ClassId)
        {
            if($HeapStack eq "Heap")
            {
                if($NeedToInheriting)
                {
                    if(has_public_destructor($ClassId, "D2")) {
                        $Type_Init{"Destructors"} .= "delete($Var);\n";
                    }
                }
                else
                {
                    if(has_public_destructor($ClassId, "D0")) {
                        $Type_Init{"Destructors"} .= "delete($Var);\n";
                    }
                }
            }
        }
    }
    # check postcondition
    if($Obj_Init{"PostCondition"}) {
        $Type_Init{"Init"} .= $Obj_Init{"PostCondition"}."\n";
    }
    if($Obj_Init{"ReturnRequirement"})
    {
        if($HeapStack eq "Stack") {
            $Obj_Init{"ReturnRequirement"}=~s/(\$0|\$obj)/$Var/gi;
        }
        else {
            $Obj_Init{"ReturnRequirement"}=~s/(\$0|\$obj)/*$Var/gi;
        }
        $Type_Init{"Init"} .= $Obj_Init{"ReturnRequirement"}."\n";
    }
    if($Obj_Init{"FinalCode"})
    {
        $Type_Init{"Init"} .= "//final code\n";
        $Type_Init{"Init"} .= $Obj_Init{"FinalCode"}."\n";
    }
    if(get_TypeType($Init_Desc{"TypeId"}) eq "Ref")
    {#obsolete
        my $BaseRefId = get_OneStep_BaseTypeId($Tid_TDid{$Init_Desc{"TypeId"}}, $Init_Desc{"TypeId"});
        if($HeapStack eq "Heap")
        {
            if(get_PointerLevel($Tid_TDid{$BaseRefId}, $BaseRefId)>1)
            {
                my $BaseRefName = get_TypeName($BaseRefId);
                $Type_Init{"Init"} .= $BaseRefName." ".$Var."_ref = ".$Type_Init{"Call"}.";\n";
                $Type_Init{"Call"} = $Var."_ref";
                $Block_Variable{$CurrentBlock}{$Var."_ref"} = 1;
                if(not defined $DisableReuse) {
                    $ValueCollection{$CurrentBlock}{$Var."_ref"} = $Init_Desc{"TypeId"};
                }
            }
        }
        else
        {
            if(get_PointerLevel($Tid_TDid{$BaseRefId}, $BaseRefId)>0)
            {
                my $BaseRefName = get_TypeName($BaseRefId);
                $Type_Init{"Init"} .= $BaseRefName." ".$Var."_ref = ".$Type_Init{"Call"}.";\n";
                $Type_Init{"Call"} = $Var."_ref";
                $Block_Variable{$CurrentBlock}{$Var."_ref"} = 1;
                if(not defined $DisableReuse) {
                    $ValueCollection{$CurrentBlock}{$Var."_ref"} = $Init_Desc{"TypeId"};
                }
            }
        }
    }
    $Type_Init{"IsCorrect"} = 1;
    if($Typedef_Id)
    {
        $Type_Init{"Headers"} = addHeaders(getTypeHeaders($Typedef_Id), $Type_Init{"Headers"});
        foreach my $Elem ("Call", "Init") {
            $Type_Init{$Elem} = cover_by_typedef($Type_Init{$Elem}, $ClassId, $Typedef_Id);
        }
    }
    else {
        $Type_Init{"Headers"} = addHeaders(getTypeHeaders($ClassId), $Type_Init{"Headers"});
    }
    if($AutoFinalCode_Init{"IsCorrect"})
    {
        $Type_Init{"Init"} = $AutoFinalCode_Init{"Init"}.$Type_Init{"Init"}.$AutoFinalCode_Init{"PreCondition"}.$AutoFinalCode_Init{"Call"}.";\n".$AutoFinalCode_Init{"FinalCode"}.$AutoFinalCode_Init{"PostCondition"};
        $Type_Init{"Code"} .= $AutoFinalCode_Init{"Code"};
        $Type_Init{"Destructors"} .= $AutoFinalCode_Init{"Destructors"};
        $Type_Init{"Headers"} = addHeaders($AutoFinalCode_Init{"Headers"}, $Type_Init{"Headers"});
    }
    return %Type_Init;
}

sub cover_by_typedef($$$)
{
    my ($Code, $Type_Id, $Typedef_Id) = @_;
    if($Class_SubClassTypedef{$Type_Id}) {
        $Typedef_Id = $Class_SubClassTypedef{$Type_Id};
    }
    return "" if(not $Code or not $Type_Id or not $Typedef_Id);
    return $Code if(not $Type_Id or not $Typedef_Id);
    return $Code if(get_TypeType($Type_Id)!~/\A(Class|Struct)\Z/);
    my $Type_Name = get_TypeName($Type_Id);
    my $Typedef_Name = get_TypeName($Typedef_Id);
    if(length($Typedef_Name)>=length($Type_Name)) {
        return $Code;
    }
    my $Child_Name_Old = getSubClassName($Type_Name);
    my $Child_Name_New = getSubClassName($Typedef_Name);
    $Class_SubClassTypedef{$Type_Id}=$Typedef_Id;
    $Code=~s/(\W|\A)\Q$Child_Name_Old\E(\W|\Z)/$1$Child_Name_New$2/g;
    if($Type_Name=~/\W\Z/) {
        $Code=~s/(\W|\A)\Q$Type_Name\E(\W|\Z)/$1$Typedef_Name$2/g;
        $Code=~s/(\W|\A)\Q$Type_Name\E(\w|\Z)/$1$Typedef_Name $2/g;
    }
    else {
        $Code=~s/(\W|\A)\Q$Type_Name\E(\W|\Z)/$1$Typedef_Name$2/g;
    }
    return $Code;
}

sub get_type_typedef($)
{
    my $ClassId = $_[0];
    if($Class_SubClassTypedef{$ClassId}) {
        return $Class_SubClassTypedef{$ClassId};
    }
    my @Types = (keys(%{$Type_Typedef{$ClassId}}));
    @Types = sort {lc(get_TypeName($a)) cmp lc(get_TypeName($b))} @Types;
    @Types = sort {length(get_TypeName($a)) <=> length(get_TypeName($b))} @Types;
    if($#Types==0) {
        return $Types[0];
    }
    else {
        return 0;
    }
}

sub is_used_var($$)
{
    my ($Block, $Var) = @_;
    return ($Block_Variable{$Block}{$Var} or $ValueCollection{$Block}{$Var}
    or not is_allowed_var_name($Var));
}

sub select_var_name($$)
{
    my ($Var_Name, $SuffixCandidate) = @_;
    my $OtherVarPrefix = 1;
    my $Candidate = $Var_Name;
    if($Var_Name=~/\Ap\d+\Z/)
    {
        $Var_Name = "p";
        while(is_used_var($CurrentBlock, $Candidate))
        {
            $Candidate = $Var_Name.$OtherVarPrefix;
            $OtherVarPrefix += 1;
        }
    }
    else
    {
        if($SuffixCandidate)
        {
            $Candidate = $Var_Name."_".$SuffixCandidate;
            if(not is_used_var($CurrentBlock, $Candidate)) {
                return $Candidate;
            }
        }
        if($Var_Name eq "description" and is_used_var($CurrentBlock, $Var_Name)
        and not is_used_var($CurrentBlock, "desc")) {
            return "desc";
        }
        elsif($Var_Name eq "system" and is_used_var($CurrentBlock, $Var_Name)
        and not is_used_var($CurrentBlock, "sys")) {
            return "sys";
        }
        while(is_used_var($CurrentBlock, $Candidate))
        {
            $Candidate = $Var_Name."_".$OtherVarPrefix;
            $OtherVarPrefix += 1;
        }
    }
    return $Candidate;
}

sub select_type_name($)
{
    my $Type_Name = $_[0];
    my $OtherPrefix = 1;
    my $NameCandidate = $Type_Name;
    while($TName_Tid{$NameCandidate})
    {
        $NameCandidate = $Type_Name."_".$OtherPrefix;
        $OtherPrefix += 1;
    }
    return $NameCandidate;
}

sub select_func_name($)
{
    my $FuncName = $_[0];
    my $OtherFuncPrefix = 1;
    my $Candidate = $FuncName;
    while(is_used_func_name($Candidate))
    {
        $Candidate = $FuncName."_".$OtherFuncPrefix;
        $OtherFuncPrefix += 1;
    }
    return $Candidate;
}

sub is_used_func_name($)
{
    my $FuncName = $_[0];
    return 1 if($FuncNames{$FuncName});
    foreach my $FuncTypeId (keys(%AuxFunc))
    {
        if($AuxFunc{$FuncTypeId} eq $FuncName) {
            return 1;
        }
    }
    return 0;
}

sub get_TypeStackId($)
{
    my $TypeId = $_[0];
    my $FoundationId = get_FoundationTypeId($TypeId);
    if(get_TypeType($FoundationId) eq "Intrinsic")
    {
        my %BaseTypedef = goToFirst($Tid_TDid{$TypeId}, $TypeId, "Typedef");
        if(get_TypeType($BaseTypedef{"Tid"}) eq "Typedef") {
            return $BaseTypedef{"Tid"};
        }
        else {
            return $FoundationId;
        }
    }
    else {
        return $FoundationId;
    }
}

sub initializeType(@)
{
    my %Init_Desc = @_;
    return () if(not $Init_Desc{"TypeId"});
    my %Type_Init = ();
    my $Global_State = save_state();
    my $TypeName = get_TypeName($Init_Desc{"TypeId"});
    my $SpecValue = $Init_Desc{"Value"};
    %Init_Desc = add_VirtualSpecType(%Init_Desc);
    $Init_Desc{"Var"} = select_var_name($LongVarNames?$Init_Desc{"Key"}:$Init_Desc{"ParamName"}, $Init_Desc{"ParamNameExt"});
    if(($TypeName eq "...") and (($Init_Desc{"Value"} eq "no value") or ($Init_Desc{"Value"} eq "")))
    {
        $Type_Init{"IsCorrect"} = 1;
        $Type_Init{"Call"} = "";
        return %Type_Init;
    }
    my $FoundationId = get_FoundationTypeId($Init_Desc{"TypeId"});
    if(not $Init_Desc{"FoundationType_Type"}) {
        $Init_Desc{"FoundationType_Type"} = get_TypeType($FoundationId);
    }
    my $TypeStackId = get_TypeStackId($Init_Desc{"TypeId"});
    if(isCyclical(\@RecurTypeId, $TypeStackId))
    {#initialize by null for cyclical types
        if($Init_Desc{"Value"} ne "no value" and $Init_Desc{"Value"} ne "")
        {
            return () if(get_TypeType($TypeStackId) eq "Typedef" and $TypeName!~/_t/);
            %Type_Init = initializeByValue(%Init_Desc);
            $Type_Init{"Headers"} = addHeaders($Init_Desc{"Headers"}, $Type_Init{"Headers"});
            return %Type_Init;
        }
        else
        {
            %Init_Desc = add_NullSpecType(%Init_Desc);
            if($Init_Desc{"OnlyDecl"})
            {
                %Type_Init = emptyDeclaration(%Init_Desc);
                $Type_Init{"Headers"} = addHeaders($Init_Desc{"Headers"}, $Type_Init{"Headers"});
                return %Type_Init;
            }
            elsif(($Init_Desc{"Value"} ne "no value") and ($Init_Desc{"Value"} ne ""))
            {
                %Type_Init = initializeByValue(%Init_Desc);
                $Type_Init{"Headers"} = addHeaders($Init_Desc{"Headers"}, $Type_Init{"Headers"});
                return %Type_Init;
            }
            else {
                return ();
            }
        }
    }
    else
    {
        if($Init_Desc{"FoundationType_Type"} ne "Array") {
            push(@RecurTypeId, $TypeStackId);
        }
    }
    if(not $Init_Desc{"TargetTypeId"})
    {#repair target type
        $Init_Desc{"TargetTypeId"} = $Init_Desc{"TypeId"};
    }
    if($Init_Desc{"RetVal"} and get_PointerLevel($Tid_TDid{$Init_Desc{"TypeId"}}, $Init_Desc{"TypeId"})>=1
    and not $Init_Desc{"TypeType_Changed"} and $TypeName!~/(\W|\Z)const(\W|\Z)/)
    {#return value
        if(($Init_Desc{"Value"} ne "no value") and ($Init_Desc{"Value"} ne ""))
        {#try to initialize type by value
            %Type_Init = initializeByValue(%Init_Desc);
            if($Type_Init{"IsCorrect"})
            {
                if($Init_Desc{"FoundationType_Type"} ne "Array") {
                    pop(@RecurTypeId);
                }
                $Type_Init{"Headers"} = addHeaders($Init_Desc{"Headers"}, $Type_Init{"Headers"});
                return %Type_Init;
            }
        }
        else
        {
            %Type_Init = initializeRetVal(%Init_Desc);
            if($Type_Init{"IsCorrect"})
            {
                if($Init_Desc{"FoundationType_Type"} ne "Array") {
                    pop(@RecurTypeId);
                }
                $Type_Init{"Headers"} = addHeaders($Init_Desc{"Headers"}, $Type_Init{"Headers"});
                return %Type_Init;
            }
        }
    }
    if($Init_Desc{"OnlyDecl"})
    {
        %Type_Init = emptyDeclaration(%Init_Desc);
        $Type_Init{"Headers"} = addHeaders($Init_Desc{"Headers"}, $Type_Init{"Headers"});
        if($Init_Desc{"FoundationType_Type"} ne "Array") {
            pop(@RecurTypeId);
        }
        return %Type_Init;
    }
    my $RealTypeId = ($Init_Desc{"TypeId_Changed"})?$Init_Desc{"TypeId_Changed"}:$Init_Desc{"TypeId"};
    my $RealFTypeType = get_TypeType(get_FoundationTypeId($RealTypeId));
    if(($RealFTypeType eq "Intrinsic") and not $SpecValue and not $Init_Desc{"Reuse"} and not $Init_Desc{"OnlyByValue"} and $Init_Desc{"ParamName"}!~/num|width|height/i)
    {#initializing intrinsics by the interface
        my %BaseTypedef = goToFirst($Tid_TDid{$RealTypeId}, $RealTypeId, "Typedef");
        if(get_TypeType($BaseTypedef{"Tid"}) eq "Typedef"
            and $BaseTypedef{"Name"}!~/(int|short|long|error|real|float|double|bool|boolean|pointer|count|byte|len)\d*(_t|)\Z/i
            and $BaseTypedef{"Name"}!~/char|str|size|enum/i
            and $BaseTypedef{"Name"}!~/(\A|::)u(32|64)/i)
        {#try to initialize typedefs to intrinsic types
            my $Global_State1 = save_state();
            my %Init_Desc_Copy = %Init_Desc;
            $Init_Desc_Copy{"InLine"} = 0 if($Init_Desc{"ParamName"}!~/\Ap\d+\Z/);
            $Init_Desc_Copy{"TypeId"} = $RealTypeId;
            restore_state($Global_State);
            %Type_Init = initializeIntrinsic(%Init_Desc_Copy);
            if($Type_Init{"IsCorrect"})
            {
                if($Init_Desc{"FoundationType_Type"} ne "Array") {
                    pop(@RecurTypeId);
                }
                return %Type_Init;
            }
            else {
                restore_state($Global_State1);
            }
        }
    }
    if(($Init_Desc{"Value"} ne "no value") and ($Init_Desc{"Value"} ne ""))
    {#try to initialize type by value
        %Type_Init = initializeByValue(%Init_Desc);
        $Type_Init{"Headers"} = addHeaders($Init_Desc{"Headers"}, $Type_Init{"Headers"});
        if($Init_Desc{"FoundationType_Type"} ne "Array") {
            pop(@RecurTypeId);
        }
        return %Type_Init;
    }
    else {
        %Type_Init = selectInitializingWay(%Init_Desc);
    }
    if($Type_Init{"IsCorrect"})
    {
        $Type_Init{"Headers"} = addHeaders($Init_Desc{"Headers"}, $Type_Init{"Headers"});
        if($Init_Desc{"FoundationType_Type"} ne "Array") {
            pop(@RecurTypeId);
        }
        return %Type_Init;
    }
    else {
        restore_state($Global_State);
    }
    if($Init_Desc{"TypeId_Changed"})
    {
        $Init_Desc{"TypeId"} = $Init_Desc{"TypeId_Changed"};
        %Init_Desc = add_VirtualSpecType(%Init_Desc);
        if(($Init_Desc{"Value"} ne "no value") and ($Init_Desc{"Value"} ne ""))
        {#try to initialize type by value
            %Type_Init = initializeByValue(%Init_Desc);
            $Type_Init{"Headers"} = addHeaders($Init_Desc{"Headers"}, $Type_Init{"Headers"});
            if($Init_Desc{"FoundationType_Type"} ne "Array") {
                pop(@RecurTypeId);
            }
            return %Type_Init;
        }
    }
    #finally initializing by null (0)
    %Init_Desc = add_NullSpecType(%Init_Desc);
    if($Init_Desc{"OnlyDecl"})
    {
        %Type_Init = emptyDeclaration(%Init_Desc);
        $Type_Init{"Headers"} = addHeaders($Init_Desc{"Headers"}, $Type_Init{"Headers"});
        if($Init_Desc{"FoundationType_Type"} ne "Array") {
            pop(@RecurTypeId);
        }
        return %Type_Init;
    }
    elsif(($Init_Desc{"Value"} ne "no value") and ($Init_Desc{"Value"} ne ""))
    {
        %Type_Init = initializeByValue(%Init_Desc);
        $Type_Init{"Headers"} = addHeaders($Init_Desc{"Headers"}, $Type_Init{"Headers"});
        if($Init_Desc{"FoundationType_Type"} ne "Array") {
            pop(@RecurTypeId);
        }
        return %Type_Init;
    }
    else
    {
        if($Init_Desc{"FoundationType_Type"} ne "Array") {
            pop(@RecurTypeId);
        }
        return ();
    }
}

sub selectInitializingWay(@)
{
    my %Init_Desc = @_;
    if($Init_Desc{"FoundationType_Type"} eq "Class") {
        return initializeClass(%Init_Desc);
    }
    elsif($Init_Desc{"FoundationType_Type"} eq "Intrinsic") {
        return initializeIntrinsic(%Init_Desc);
    }
    elsif($Init_Desc{"FoundationType_Type"} eq "Struct") {
        return initializeStruct(%Init_Desc);
    }
    elsif($Init_Desc{"FoundationType_Type"} eq "Union") {
        return initializeUnion(%Init_Desc);
    }
    elsif($Init_Desc{"FoundationType_Type"} eq "Enum") {
        return initializeEnum(%Init_Desc);
    }
    elsif($Init_Desc{"FoundationType_Type"} eq "Array") {
        return initializeArray(%Init_Desc);
    }
    elsif($Init_Desc{"FoundationType_Type"} eq "FuncPtr") {
        return initializeFuncPtr(%Init_Desc);
    }
    else {
        return ();
    }
}

sub is_const_type($)
{# char const*
 #! char*const
    my $TypeName = uncover_typedefs($_[0]);
    return ($TypeName=~/(\W|\A)const(\W)/);
}

sub clearSyntax($)
{
    my $Expression = $_[0];
    $Expression=~s/\*\&//g;
    $Expression=~s/\&\*//g;
    $Expression=~s/\(\*(\w+)\)\./$1\-\>/ig;
    $Expression=~s/\(\&(\w+)\)\-\>/$1\./ig;
    $Expression=~s/\*\(\&(\w+)\)/$1/ig;
    $Expression=~s/\*\(\(\&(\w+)\)\)/$1/ig;
    $Expression=~s/\&\(\*(\w+)\)/$1/ig;
    $Expression=~s/\&\(\(\*(\w+)\)\)/$1/ig;
    $Expression=~s/(?<=[\s()])\(([a-z_]\w*)\)[ ]*,/$1,/ig;
    $Expression=~s/,(\s*)\(([a-z_]\w*)\)[ ]*(\)|,)/,$1$2/ig;
    $Expression=~s/(?<=[^\$])\(\(([a-z_]\w*)\)\)/\($1\)/ig;
    return $Expression;
}

sub get_random_number($)
{
    my $Range = $_[0];
    return `echo \$(( (\`od -An -N2 -i /dev/random\` )\%$Range ))`;
}

sub apply_default_value($$)
{
    my ($Interface, $ParamPos) = @_;
    return 0 if(defined $DisableDefaultValues);
    return 0 if(not defined $CompleteSignature{$Interface}{"Param"}{$ParamPos});
    return 0 if(not $CompleteSignature{$Interface}{"Param"}{$ParamPos}{"default"});
    if($Interface eq $TestedInterface
    or replace_c2c1($Interface) eq replace_c2c1($TestedInterface))
    {# do not use defaults for target symbol
        return 0;
    }
    return 1;
}

sub sort_AppendInsert(@)
{
    my @Interfaces = @_;
    my (@Add, @Append, @Push, @Init, @Insert) = ();
    foreach my $Interface (@Interfaces)
    {
        if($CompleteSignature{$Interface}{"ShortName"}=~/add/i) {
            push(@Add, $Interface);
        }
        elsif($CompleteSignature{$Interface}{"ShortName"}=~/append/i) {
            push(@Append, $Interface);
        }
        elsif($CompleteSignature{$Interface}{"ShortName"}=~/push/i) {
            push(@Push, $Interface);
        }
        elsif($CompleteSignature{$Interface}{"ShortName"}=~/init/i) {
            push(@Init, $Interface);
        }
        elsif($CompleteSignature{$Interface}{"ShortName"}=~/insert/) {
            push(@Insert, $Interface);
        }
    }
    return (@Add, @Append, @Push, @Init, @Insert);
}

sub get_AutoFinalCode($$)
{
    my ($Interface, $ObjectCall) = @_;
    my (@AddMethods, @AppendMethods, @PushMethods, @InitMethods, @InsertMethods) = ();
    if($CompleteSignature{$Interface}{"Constructor"})
    {
        my $ClassId = $CompleteSignature{$Interface}{"Class"};
        my @Methods = sort_AppendInsert(keys(%{$Class_Method{$ClassId}}));
        return () if($#Methods<0);
        foreach my $Method (@Methods)
        {
            my %Method_Init = callInterface((
            "Interface"=>$Method,
            "ObjectCall"=>$ObjectCall,
            "DoNotReuse"=>1,
            "InsertCall"));
            if($Method_Init{"IsCorrect"}) {
                return %Method_Init;
            }
        }
        return ();
    }
    else {
        return ();
    }
}

sub initializeParameter(@)
{
    my %ParamDesc = @_;
    my $ParamPos = $ParamDesc{"ParamPos"};
    my ($TypeOfSpecType, $SpectypeCode, $SpectypeValue);
    my (%Param_Init, $PreCondition, $PostCondition, $InitCode, $DeclCode);
    my $ObjectCall = $ParamDesc{"AccessToParam"}->{"obj"};
    my $FoundationType_Id = get_FoundationTypeId($ParamDesc{"TypeId"});
    if((not $ParamDesc{"SpecType"}) and ($ObjectCall ne "create object")
    and not $Interface_OutParam_NoUsing{$ParamDesc{"Interface"}}{$ParamDesc{"ParamName"}}
    and not $Interface_OutParam{$ParamDesc{"Interface"}}{$ParamDesc{"ParamName"}}) {
        $ParamDesc{"SpecType"} = chooseSpecType($ParamDesc{"TypeId"}, "common_param", $ParamDesc{"Interface"});
    }
    if($ParamDesc{"SpecType"} and not isCyclical(\@RecurSpecType, $ParamDesc{"SpecType"}))
    {
        $IntSpecType{$TestedInterface}{$ParamDesc{"SpecType"}} = 1;
        $SpectypeCode = $SpecType{$ParamDesc{"SpecType"}}{"GlobalCode"} if(not $SpecCode{$ParamDesc{"SpecType"}});
        $SpecCode{$ParamDesc{"SpecType"}} = 1;
        push(@RecurSpecType, $ParamDesc{"SpecType"});
        $TypeOfSpecType = get_TypeIdByName($SpecType{$ParamDesc{"SpecType"}}{"DataType"});
        $SpectypeValue = $SpecType{$ParamDesc{"SpecType"}}{"Value"};
        if($SpectypeValue=~/\A[A-Z_0-9]+\Z/
        and get_TypeType($FoundationType_Id)=~/\A(Struct|Union)\Z/i) {
            $ParamDesc{"InLine"} = 1;
        }
        $DeclCode = $SpecType{$ParamDesc{"SpecType"}}{"DeclCode"};
        if($DeclCode)
        {
            $DeclCode .= "\n";
            if($DeclCode=~/\$0/ or $DeclCode=~/\$$ParamPos(\Z|\D)/) {
                $ParamDesc{"InLine"} = 0;
            }
        }
        $InitCode = $SpecType{$ParamDesc{"SpecType"}}{"InitCode"};
        if($InitCode)
        {
            $InitCode .= "\n";
            if($InitCode=~/\$0/ or $InitCode=~/\$$ParamPos(\Z|\D)/) {
                $ParamDesc{"InLine"} = 0;
            }
        }
        $Param_Init{"FinalCode"} = $SpecType{$ParamDesc{"SpecType"}}{"FinalCode"};
        if($Param_Init{"FinalCode"})
        {
            $Param_Init{"FinalCode"} .= "\n";
            if($Param_Init{"FinalCode"}=~/\$0/
            or $Param_Init{"FinalCode"}=~/\$$ParamPos(\Z|\D)/) {
                $ParamDesc{"InLine"} = 0;
            }
        }
        $PreCondition = $SpecType{$ParamDesc{"SpecType"}}{"PreCondition"};
        if($PreCondition=~/\$0/ or $PreCondition=~/\$$ParamPos(\Z|\D)/) {
            $ParamDesc{"InLine"} = 0;
        }
        $PostCondition = $SpecType{$ParamDesc{"SpecType"}}{"PostCondition"};
        if($PostCondition=~/\$0/ or $PostCondition=~/\$$ParamPos(\Z|\D)/) {
            $ParamDesc{"InLine"} = 0;
        }
        foreach my $Lib (keys(%{$SpecType{$ParamDesc{"SpecType"}}{"Libs"}})) {
            $SpecLibs{$Lib} = 1;
        }
        
    }
    elsif(apply_default_value($ParamDesc{"Interface"}, $ParamDesc{"ParamPos"}))
    {
        $Param_Init{"IsCorrect"} = 1;
        $Param_Init{"Call"} = "";
        return %Param_Init;
    }
    if(($ObjectCall ne "no object") and ($ObjectCall ne "create object"))
    {
        if(($ObjectCall=~/\A\*/) or ($ObjectCall=~/\A\&/)) {
            $ObjectCall = "(".$ObjectCall.")";
        }
        $SpectypeValue=~s/\$obj/$ObjectCall/g;
        $SpectypeValue = clearSyntax($SpectypeValue);
    }
    if($ParamDesc{"Value"} ne ""
    and $ParamDesc{"Value"} ne "no value") {
        $SpectypeValue = $ParamDesc{"Value"};
    }
    if($SpectypeValue=~/\$[^\(\[]/)
    {# access to other parameters
        foreach my $ParamKey (keys(%{$ParamDesc{"AccessToParam"}}))
        {
            my $AccessToParam_Value = $ParamDesc{"AccessToParam"}->{$ParamKey};
            $SpectypeValue=~s/\$\Q$ParamKey\E([^0-9]|\Z)/$AccessToParam_Value$1/g;
        }
    }
    if($SpectypeValue)
    {
        my %ParsedValueCode = parseCode($SpectypeValue, "Value");
        if(not $ParsedValueCode{"IsCorrect"})
        {
            pop(@RecurSpecType);
            return ();
        }
        $Param_Init{"Init"} .= $ParsedValueCode{"CodeBefore"};
        $Param_Init{"FinalCode"} .= $ParsedValueCode{"CodeAfter"};
        $SpectypeValue = $ParsedValueCode{"Code"};
        $Param_Init{"Headers"} = addHeaders($ParsedValueCode{"Headers"}, $ParsedValueCode{"Headers"});
        $Param_Init{"Code"} .= $ParsedValueCode{"NewGlobalCode"};
    }
    if(get_TypeType($FoundationType_Id)=~/\A(Struct|Class|Union)\Z/i
    and $CompleteSignature{$ParamDesc{"Interface"}}{"Constructor"}
    and get_PointerLevel($Tid_TDid{$ParamDesc{"TypeId"}}, $ParamDesc{"TypeId"})==0) {
        $ParamDesc{"InLine"} = 0;
    }
    if($DeclCode)
    {
        $Param_Init{"Headers"} = addHeaders(getTypeHeaders($ParamDesc{"TypeId"}), $Param_Init{"Headers"});
        $Param_Init{"Call"} = select_var_name($ParamDesc{"ParamName"}, "");
        $Param_Init{"TargetCall"} = $Param_Init{"Value"}?$Param_Init{"Value"}:$Param_Init{"Call"};
    }
    elsif($ParamDesc{"Usage"} eq "Common")
    {
        my %Type_Init = initializeType((
        "Interface" => $ParamDesc{"Interface"},
        "TypeId" => $ParamDesc{"TypeId"},
        "Key" => $ParamDesc{"Key"},
        "InLine" => $ParamDesc{"InLine"},
        "Value" => $SpectypeValue,
        "ValueTypeId" => $TypeOfSpecType,
        "TargetTypeId" => $TypeOfSpecType,
        "CreateChild" => $ParamDesc{"CreateChild"},
        "ParamName" => $ParamDesc{"ParamName"},
        "ParamPos" => $ParamDesc{"ParamPos"},
        "ConvertToBase" => $ParamDesc{"ConvertToBase"},
        "StrongConvert" => $ParamDesc{"StrongConvert"},
        "ObjectInit" => $ParamDesc{"ObjectInit"},
        "DoNotReuse" => $ParamDesc{"DoNotReuse"},
        "RetVal" => $ParamDesc{"RetVal"},
        "ParamNameExt" => $ParamDesc{"ParamNameExt"},
        "MaxParamPos" => $ParamDesc{"MaxParamPos"},
        "OuterType_Id" => $ParamDesc{"OuterType_Id"},
        "OuterType_Type" => $ParamDesc{"OuterType_Type"},
        "Index" => $ParamDesc{"Index"},
        "InLineArray" => $ParamDesc{"InLineArray"},
        "IsString" => $ParamDesc{"IsString"},
        "FuncPtrName" => $ParamDesc{"FuncPtrName"},
        "FuncPtrTypeId" => $ParamDesc{"FuncPtrTypeId"}));
        if(not $Type_Init{"IsCorrect"})
        {
            pop(@RecurSpecType);
            return ();
        }
        $Param_Init{"Init"} .= $Type_Init{"Init"};
        $Param_Init{"Call"} .= $Type_Init{"Call"};
        $Param_Init{"TargetCall"} = $Type_Init{"TargetCall"};
        $Param_Init{"Code"} .= $Type_Init{"Code"};
        $Param_Init{"Destructors"} .= $Type_Init{"Destructors"};
        $Param_Init{"FinalCode"} .= $Type_Init{"FinalCode"};
        $Param_Init{"PreCondition"} .= $Type_Init{"PreCondition"};
        $Param_Init{"PostCondition"} .= $Type_Init{"PostCondition"};
        $Param_Init{"Headers"} = addHeaders($Type_Init{"Headers"}, $Param_Init{"Headers"});
        $Param_Init{"ByNull"} = $Type_Init{"ByNull"};
    }
    else
    {
        $Param_Init{"Headers"} = addHeaders(getTypeHeaders($ParamDesc{"TypeId"}), $Param_Init{"Headers"});
        if(my $Target = $ParamDesc{"AccessToParam"}->{"0"}) {
            $Param_Init{"TargetCall"} = $Target;
        }
    }
    my $TargetCall = $Param_Init{"TargetCall"};
    if($TargetCall=~/\A(\*|\&)/) {
        $TargetCall = "(".$TargetCall.")";
    }
    if($SpectypeCode)
    {
        my $PreviousBlock = $CurrentBlock;
        $CurrentBlock = $CurrentBlock."_code_".$ParamDesc{"SpecType"};
        my %ParsedCode = parseCode($SpectypeCode, "Code");
        $CurrentBlock = $PreviousBlock;
        if(not $ParsedCode{"IsCorrect"})
        {
            pop(@RecurSpecType);
            return ();
        }
        foreach my $Header (@{$ParsedCode{"Headers"}}) {
            $SpecTypeHeaders{get_filename($Header)}=1;
        }
        $Param_Init{"Headers"} = addHeaders($ParsedCode{"Headers"}, $Param_Init{"Headers"});
        $Param_Init{"Code"} .= $ParsedCode{"NewGlobalCode"}.$ParsedCode{"Code"};
    }
    if($ObjectCall eq "create object")
    {
        $ObjectCall = $Param_Init{"Call"};
        if($ObjectCall=~/\A\*/ or $ObjectCall=~/\A\&/) {
            $ObjectCall = "(".$ObjectCall.")";
        }
    }
    if($DeclCode)
    {
        if($ObjectCall ne "no object") {
            $DeclCode=~s/\$obj/$ObjectCall/g;
        }
        $DeclCode=~s/\$0/$TargetCall/g;
        my %ParsedCode = parseCode($DeclCode, "Code");
        if(not $ParsedCode{"IsCorrect"})
        {
            pop(@RecurSpecType);
            return ();
        }
        $DeclCode = clearSyntax($DeclCode);
        $Param_Init{"Headers"} = addHeaders($ParsedCode{"Headers"}, $Param_Init{"Headers"});
        $Param_Init{"Code"} .= $ParsedCode{"NewGlobalCode"};
        $DeclCode = $ParsedCode{"Code"};
        $Param_Init{"Init"} .= "//decl code\n".$DeclCode."\n";
    }
    if($InitCode)
    {
        if($ObjectCall ne "no object") {
            $InitCode=~s/\$obj/$ObjectCall/g;
        }
        $InitCode=~s/\$0/$TargetCall/g;
        my %ParsedCode = parseCode($InitCode, "Code");
        if(not $ParsedCode{"IsCorrect"})
        {
            pop(@RecurSpecType);
            return ();
        }
        $InitCode = clearSyntax($InitCode);
        $Param_Init{"Headers"} = addHeaders($ParsedCode{"Headers"}, $Param_Init{"Headers"});
        $Param_Init{"Code"} .= $ParsedCode{"NewGlobalCode"};
        $InitCode = $ParsedCode{"Code"};
        $Param_Init{"Init"} .= "//init code\n".$InitCode."\n";
    }
    if($Param_Init{"FinalCode"})
    {
        if($ObjectCall ne "no object") {
            $Param_Init{"FinalCode"}=~s/\$obj/$ObjectCall/g;
        }
        $Param_Init{"FinalCode"}=~s/\$0/$TargetCall/g;
        my %ParsedCode = parseCode($Param_Init{"FinalCode"}, "Code");
        if(not $ParsedCode{"IsCorrect"})
        {
            pop(@RecurSpecType);
            return ();
        }
        $Param_Init{"FinalCode"} = clearSyntax($Param_Init{"FinalCode"});
        $Param_Init{"Headers"} = addHeaders($ParsedCode{"Headers"}, $Param_Init{"Headers"});
        $Param_Init{"Code"} .= $ParsedCode{"NewGlobalCode"};
        $Param_Init{"FinalCode"} = $ParsedCode{"Code"};
    }
    if(not defined $Template2Code or $ParamDesc{"Interface"} eq $TestedInterface)
    {
        $Param_Init{"PreCondition"} .= constraint_for_parameter($ParamDesc{"Interface"}, $SpecType{$ParamDesc{"SpecType"}}{"DataType"}, "precondition", $PreCondition, $ObjectCall, $TargetCall);
        $Param_Init{"PostCondition"} .= constraint_for_parameter($ParamDesc{"Interface"}, $SpecType{$ParamDesc{"SpecType"}}{"DataType"}, "postcondition", $PostCondition, $ObjectCall, $TargetCall);
    }
    pop(@RecurSpecType);
    $Param_Init{"IsCorrect"} = 1;
    return %Param_Init;
}

sub constraint_for_parameter($$$$$$)
{
    my ($Interface, $DataType, $ConditionType, $Condition, $ObjectCall, $TargetCall) = @_;
    return "" if(not $Interface or not $ConditionType or not $Condition);
    my $Condition_Comment = $Condition;
    $Condition_Comment=~s/\$obj/$ObjectCall/g if($ObjectCall ne "no object" and $ObjectCall ne "");
    $Condition_Comment=~s/\$0/$TargetCall/g if($TargetCall ne "");
    $Condition_Comment = clearSyntax($Condition_Comment);
    $Condition = $Condition_Comment;
    while($Condition_Comment=~s/([^\\])"/$1\\\"/g){}
    $ConstraintNum{$Interface}+=1;
    my $ParameterObject = ($ObjectCall eq "create object")?"object":"parameter";
    $RequirementsCatalog{$Interface}{$ConstraintNum{$Interface}} = "$ConditionType for the $ParameterObject: \'$Condition_Comment\'";
    my $ReqId = short_interface_name($Interface).".".normalize_num($ConstraintNum{$Interface});
    if(my $Format = is_printable($DataType))
    {
        my $Comment = "$ConditionType for the $ParameterObject failed: \'$Condition_Comment\', parameter value: $Format";
        $TraceFunc{"REQva"}=1;
        return "REQva(\"$ReqId\",\n$Condition,\n\"$Comment\",\n$TargetCall);\n";
    }
    else
    {
        my $Comment = "$ConditionType for the $ParameterObject failed: \'$Condition_Comment\'";
        $TraceFunc{"REQ"}=1;
        return "REQ(\"$ReqId\",\n\"$Comment\",\n$Condition);\n";
    }
}

sub constraint_for_interface($$$)
{
    my ($Interface, $ConditionType, $Condition) = @_;
    return "" if(not $Interface or not $ConditionType or not $Condition);
    my $Condition_Comment = $Condition;
    $Condition_Comment = clearSyntax($Condition_Comment);
    $Condition = $Condition_Comment;
    while($Condition_Comment=~s/([^\\])"/$1\\\"/g){}
    $ConstraintNum{$Interface}+=1;
    $RequirementsCatalog{$Interface}{$ConstraintNum{$Interface}} = "$ConditionType for the interface: \'$Condition_Comment\'";
    my $ReqId = short_interface_name($Interface).".".normalize_num($ConstraintNum{$Interface});
    my $Comment = "$ConditionType for the interface failed: \'$Condition_Comment\'";
    $TraceFunc{"REQ"}=1;
    return "REQ(\"$ReqId\",\n\"$Comment\",\n$Condition);\n";
}

sub is_array_count($$)
{
    my ($ParamName_Prev, $ParamName_Next) = @_;
    return ($ParamName_Next=~/\A(\Q$ParamName_Prev\E|)[_]*(n|l|c|s)[_]*(\Q$ParamName_Prev\E|)\Z/i
    or $ParamName_Next=~/len|size|amount|count|num|number/i);
}

sub add_VirtualProxy($$$$)
{
    my ($Interface, $OutParamPos, $Order, $Step) = @_;
    return if(keys(%{$CompleteSignature{$Interface}{"Param"}})<$Step+1);
    foreach my $Param_Pos (sort {($Order eq "forward")?int($a)<=>int($b):int($b)<=>int($a)} keys(%{$CompleteSignature{$Interface}{"Param"}}))
    {
        if(apply_default_value($Interface, $Param_Pos)) {
            next;
        }
        my $Prev_Pos = ($Order eq "forward")?$Param_Pos-$Step:$Param_Pos+$Step;
        next if(($Order eq "forward")?$Prev_Pos<0:$Prev_Pos>keys(%{$CompleteSignature{$Interface}{"Param"}})-1);
        my $ParamName = $CompleteSignature{$Interface}{"Param"}{$Param_Pos}{"name"};
        my $ParamTypeId = $CompleteSignature{$Interface}{"Param"}{$Param_Pos}{"type"};
        my $ParamTypeName = get_TypeName($ParamTypeId);
        my $ParamName_Prev = $CompleteSignature{$Interface}{"Param"}{$Prev_Pos}{"name"};
        my $ParamTypeId_Prev = $CompleteSignature{$Interface}{"Param"}{$Prev_Pos}{"type"};
        my $ParamTypeName_Prev = get_TypeName($ParamTypeId_Prev);
        if(not $InterfaceSpecType{$Interface}{"SpecParam"}{$Param_Pos})
        {
            next if($OutParamPos ne "" and $OutParamPos==$Prev_Pos);
            my $ParamFTypeId = get_FoundationTypeId($ParamTypeId);
            my $ParamFTypeId_Previous = get_FoundationTypeId($ParamTypeId_Prev);
            my $ParamTypePLevel_Previous = get_PointerLevel($Tid_TDid{$ParamTypeId_Prev}, $ParamTypeId_Prev);
            if(isIntegerType(get_TypeName($ParamFTypeId)) and get_PointerLevel($Tid_TDid{$ParamTypeId}, $ParamTypeId)==0
            and get_PointerLevel($Tid_TDid{$ParamTypeId_Prev}, $ParamTypeId_Prev)>=1 and $ParamName_Prev
            and is_array_count($ParamName_Prev, $ParamName) and not isOutParam_NoUsing($ParamTypeId_Prev, $ParamName_Prev, $Interface)
            and not $OutParamInterface_Pos{$Interface}{$Prev_Pos} and not $OutParamInterface_Pos_NoUsing{$Interface}{$Prev_Pos})
            {
                if(isArray($ParamTypeId_Prev, $ParamName_Prev, $Interface)) {
                    $ProxyValue{$Interface}{$Param_Pos} = $DEFAULT_ARRAY_AMOUNT;
                }
                elsif(isBuffer($ParamTypeId_Prev, $ParamName_Prev, $Interface)) {
                    $ProxyValue{$Interface}{$Param_Pos} = $BUFF_SIZE;
                }
                elsif(isString($ParamTypeId_Prev, $ParamName_Prev, $Interface))
                {
                    if($ParamName_Prev=~/file|src|uri|buf|dir|url/i) {
                        $ProxyValue{$Interface}{$Param_Pos} = "1";
                    }
                    elsif($ParamName_Prev!~/\Ap\d+\Z/i) {
                        $ProxyValue{$Interface}{$Param_Pos} = length($ParamName_Prev);
                    }
                }
                elsif($ParamName_Prev=~/buf/i) {
                    $ProxyValue{$Interface}{$Param_Pos} = "1";
                }
            }
            elsif($Order eq "forward" and isString($ParamTypeId_Prev, $ParamName_Prev, $Interface)
            and ($ParamName_Prev=~/\A[_0-9]*(format|fmt)[_0-9]*\Z/i) and ($ParamTypeName eq "..."))
            {
                $ProxyValue{$Interface}{$Param_Pos-1} = "\"\%d\"";
                $ProxyValue{$Interface}{$Param_Pos} = "1";
            }
        }
    }
}

sub isExactValueAble($)
{
    my $TypeName = $_[0];
    return $TypeName=~/\A(char const\*|wchar_t const\*|wint_t|int|bool|double|float|long double|char|long|long long|long long int|long int)\Z/;
}

sub select_obj_name($$)
{
    my ($Key, $ClassId) = @_;
    my $ClassName = get_TypeName($ClassId);
    if(my $NewName = getParamNameByTypeName($ClassName)) {
        return $NewName;
    }
    else {
        return (($Key)?"src":"obj");
    }
}

sub getParamNameByTypeName($)
{
    my $TypeName = get_type_short_name(remove_quals($_[0]));
    return "" if(not $TypeName or $TypeName=~/\(|\)|<|>/);
    while($TypeName=~s/\A\w+\:\://g){ };
    while($TypeName=~s/(\*|\&|\[|\])//g){ };
    $TypeName=~s/(\A\s+|\s+\Z)//g;
    return "Db" if($TypeName eq "sqlite3");
    return "tif" if($TypeName eq "TIFF");
    my $ShortTypeName = cut_NamePrefix($TypeName);
    if($ShortTypeName ne $TypeName
    and is_allowed_var_name(lc($ShortTypeName)))
    {
        $TypeName = $ShortTypeName;
        return lc($ShortTypeName);
    }
    if($TypeName=~/[A-Z]+/)
    {
        if(is_allowed_var_name(lc($TypeName))) {
            return lc($TypeName);
        }
    }
    return "";
}

sub is_allowed_var_name($)
{
    my $Candidate = $_[0];
    return (not $IsKeyword{$Candidate} and not $TName_Tid{$Candidate}
    and not $NameSpaces{$Candidate} and not $EnumMembers{$Candidate}
    and not $GlobVarNames{$Candidate} and not $FuncNames{$Candidate});
}

sub callInterfaceParameters_m(@)
{
    my %Init_Desc = @_;
    my (@ParamList, %ParametersOrdered, %Params_Init, $IsWrapperCall);
    my ($Interface, $Key, $ObjectCall) = ($Init_Desc{"Interface"}, $Init_Desc{"Key"}, $Init_Desc{"ObjectCall"});
    add_VirtualProxy($Interface, $Init_Desc{"OutParam"},  "forward", 1);
    add_VirtualProxy($Interface, $Init_Desc{"OutParam"},  "forward", 2);
    add_VirtualProxy($Interface, $Init_Desc{"OutParam"}, "backward", 1);
    add_VirtualProxy($Interface, $Init_Desc{"OutParam"}, "backward", 2);
    my (%KeyTable, %AccessToParam, %TargetAccessToParam, %InvOrder, %Interface_Init, $SubClasses_Before) = ();
    $AccessToParam{"obj"} = $ObjectCall;
    $TargetAccessToParam{"obj"} = $ObjectCall;
    return () if(needToInherit($Interface) and isInCharge($Interface));
    $Interface_Init{"Headers"} = addHeaders([$CompleteSignature{$Interface}{"Header"}], $Interface_Init{"Headers"});
    if(not $CompleteSignature{$Interface}{"Constructor"}
    and not $CompleteSignature{$Interface}{"Destructor"}) {
        $Interface_Init{"Headers"} = addHeaders(getTypeHeaders($CompleteSignature{$Interface}{"Return"}), $Interface_Init{"Headers"});
    }
    my $ShortName = $CompleteSignature{$Interface}{"ShortName"};
    if($CompleteSignature{$Interface}{"Constructor"}) {
        $Interface_Init{"Call"} .= get_TypeName($CompleteSignature{$Interface}{"Class"});
    }
    else {
        $Interface_Init{"Call"} .= $ShortName;
    }
    my $IsWrapperCall = (($CompleteSignature{$Interface}{"Protected"}) and (not $CompleteSignature{$Interface}{"Constructor"}));
    if($IsWrapperCall)
    {
        $Interface_Init{"Call"} .= "_Wrapper";
        $Interface_Init{"Call"} = cleanName($Interface_Init{"Call"});
        @{$SubClasses_Before}{keys %Create_SubClass} = values %Create_SubClass;
        %Create_SubClass = ();
    }
    my $NumOfParams = getNumOfParams($Interface);
    # detecting inline parameters
    my %InLineParam = detectInLineParams($Interface);
    my %Order = detectParamsOrder($Interface);
    @InvOrder{values %Order} = keys %Order;
    foreach my $Param_Pos (sort {int($a)<=>int($b)} keys(%{$CompleteSignature{$Interface}{"Param"}}))
    {
        $ParametersOrdered{$Order{$Param_Pos + 1} - 1}{"type"} = $CompleteSignature{$Interface}{"Param"}{$Param_Pos}{"type"};
        $ParametersOrdered{$Order{$Param_Pos + 1} - 1}{"name"} = $CompleteSignature{$Interface}{"Param"}{$Param_Pos}{"name"};
    }
    # initializing parameters
    if(keys(%{$CompleteSignature{$Interface}{"Param"}})>0
    and defined $CompleteSignature{$Interface}{"Param"}{0})
    {
        my $MaxParamPos = keys(%{$CompleteSignature{$Interface}{"Param"}}) - 1;
        foreach my $Param_Pos (sort {int($a)<=>int($b)} keys(%{$CompleteSignature{$Interface}{"Param"}}))
        {
            next if($Param_Pos eq "");
            my $TruePos = $InvOrder{$Param_Pos + 1} - 1;
            my $TypeId = $ParametersOrdered{$Param_Pos}{"type"};
            my $FTypeId = get_FoundationTypeId($TypeId);
            my $TypeName = get_TypeName($TypeId);
            my $Param_Name = $ParametersOrdered{$Param_Pos}{"name"};
            if($Param_Name=~/\Ap\d+\Z/
            and (my $NewParamName = getParamNameByTypeName($TypeName))) {
                $Param_Name = $NewParamName;
            }
            my $Param_Name_Ext = "";
            if(is_used_var($CurrentBlock, $Param_Name) and not $LongVarNames
            and ($Key=~/(_|\A)\Q$Param_Name\E(_|\Z)/))
            {
                if($TypeName=~/string/i) {
                    $Param_Name_Ext="str";
                }
                elsif($TypeName=~/char/i) {
                    $Param_Name_Ext="ch";
                }
            }
            $Param_Name = "p".($TruePos+1) if(not $Param_Name);
            my $TypeType = get_TypeType($TypeId);
            my $TypeName_Uncovered = uncover_typedefs($TypeName);
            my $InLine = $InLineParam{$TruePos+1};
            my $StrongConvert = 0;
            if($OverloadedInterface{$Interface})
            {
                if(not isExactValueAble($TypeName_Uncovered)
                and $TypeType ne "Enum") {
                    #$InLine = 0;
                    $StrongConvert = 1;
                }
            }
            $InLine = 0 if(uncover_typedefs($TypeName)=~/\&/);
            $InLine = 0 if(get_TypeType($FTypeId)!~/\A(Intrinsic|Enum)\Z/ and $Param_Name!~/\Ap\d+\Z/
                and not isCyclical(\@RecurTypeId, get_TypeStackId($TypeId)));
            my $NewKey = ($Param_Name)? (($Key)?$Key."_".$Param_Name:$Param_Name) : ($Key)?$Key."_".($TruePos+1):"p".$InvOrder{$Param_Pos+1};
            my $SpecTypeId = $InterfaceSpecType{$Interface}{"SpecParam"}{$TruePos};
            my $ParamValue = "no value";
            if(defined $ProxyValue{$Interface}
            and my $PValue = $ProxyValue{$Interface}{$TruePos}) {
                $ParamValue = $PValue;
            }
            # initialize parameter
            if(($Init_Desc{"OutParam"} ne "") and $Param_Pos==$Init_Desc{"OutParam"})
            {# initializing out-parameter
                $AccessToParam{$TruePos+1} = $Init_Desc{"OutVar"};
                $TargetAccessToParam{$TruePos+1} = $Init_Desc{"OutVar"};
                if($SpecTypeId and ($SpecType{$SpecTypeId}{"InitCode"}.$SpecType{$SpecTypeId}{"FinalCode"}.$SpecType{$SpecTypeId}{"PreCondition"}.$SpecType{$SpecTypeId}{"PostCondition"})=~/\$0/)
                {
                    if(is_equal_types(get_TypeName($TypeId), $SpecType{$SpecTypeId}{"DataType"}))
                    {
                        $AccessToParam{"0"} = $Init_Desc{"OutVar"};
                        $TargetAccessToParam{"0"} = $Init_Desc{"OutVar"};
                    }
                    else
                    {
                        my ($TargetCall, $Preamble)=
                        convert_familiar_types((
                            "InputTypeName"=>get_TypeName($TypeId),
                            "InputPointerLevel"=>get_PointerLevel($Tid_TDid{$TypeId}, $TypeId),
                            "OutputTypeId"=>get_TypeIdByName($SpecType{$SpecTypeId}{"DataType"}),
                            "Value"=>$Init_Desc{"OutVar"},
                            "Key"=>$NewKey,
                            "Destination"=>"Target",
                            "MustConvert"=>0));
                        $Params_Init{"Init"} .= $Preamble;
                        $AccessToParam{"0"} = $TargetCall;
                        $TargetAccessToParam{"0"} = $TargetCall;
                    }
                }
                my %Param_Init = initializeParameter((
                    "Interface" => $Interface,
                    "AccessToParam" => \%TargetAccessToParam,
                    "TypeId" => $TypeId,
                    "Key" => $NewKey,
                    "SpecType" => $SpecTypeId,
                    "Usage" => "OnlySpecType",
                    "ParamName" => $Param_Name,
                    "ParamPos" => $TruePos));
                $Params_Init{"Init"} .= $Param_Init{"Init"};
                $Params_Init{"Code"} .= $Param_Init{"Code"};
                $Params_Init{"FinalCode"} .= $Param_Init{"FinalCode"};
                $Params_Init{"PreCondition"} .= $Param_Init{"PreCondition"};
                $Params_Init{"PostCondition"} .= $Param_Init{"PostCondition"};
                $Interface_Init{"Headers"} = addHeaders($Param_Init{"Headers"}, $Interface_Init{"Headers"});
            }
            else
            {
                my $CreateChild = ($ShortName eq "operator="
                    and get_TypeName($FTypeId) eq get_TypeName($CompleteSignature{$Interface}{"Class"}) and $CompleteSignature{$Interface}{"Protected"});
                if($IsWrapperCall and $CompleteSignature{$Interface}{"Class"}) {
                    push(@RecurTypeId, $CompleteSignature{$Interface}{"Class"});
                }
                my %Param_Init = initializeParameter((
                    "Interface" => $Interface,
                    "AccessToParam" => \%TargetAccessToParam,
                    "TypeId" => $TypeId,
                    "Key" => $NewKey,
                    "InLine" => $InLine,
                    "Value" => $ParamValue,
                    "CreateChild" => $CreateChild,
                    "SpecType" => $SpecTypeId,
                    "Usage" => "Common",
                    "ParamName" => $Param_Name,
                    "ParamPos" => $TruePos,
                    "StrongConvert" => $StrongConvert,
                    "DoNotReuse" => $Init_Desc{"DoNotReuse"},
                    "ParamNameExt" => $Param_Name_Ext,
                    "MaxParamPos" => $MaxParamPos));
                if($IsWrapperCall
                and $CompleteSignature{$Interface}{"Class"}) {
                    pop(@RecurTypeId);
                }
                if(not $Param_Init{"IsCorrect"})
                {
                    foreach my $ClassId (keys(%{$SubClasses_Before})) {
                        $Create_SubClass{$ClassId} = 1;
                    }
                    return ();
                }
                my $RetParam = $Init_Desc{"RetParam"};
                if($Param_Init{"ByNull"} and ($Interface ne $TestedInterface)
                and (($ShortName=~/(\A|_)\Q$RetParam\E(\Z|_)/i and $ShortName!~/(\A|_)init(\Z|_)/i and $Param_Name!~/out|error/i)
                or is_transit_function($CompleteSignature{$Interface}{"ShortName"}))) {
                    return ();
                }
                if($Param_Init{"ByNull"}
                and $Param_Init{"InsertCall"}) {
                    return ();
                }
                $Params_Init{"Init"} .= $Param_Init{"Init"};
                $Params_Init{"Code"} .= $Param_Init{"Code"};
                $Params_Init{"Destructors"} .= $Param_Init{"Destructors"};
                $Params_Init{"FinalCode"} .= $Param_Init{"FinalCode"};
                $Params_Init{"PreCondition"} .= $Param_Init{"PreCondition"};
                $Params_Init{"PostCondition"} .= $Param_Init{"PostCondition"};
                $Interface_Init{"Headers"} = addHeaders($Param_Init{"Headers"}, $Interface_Init{"Headers"});
                $AccessToParam{$TruePos+1} = $Param_Init{"Call"};
                $TargetAccessToParam{$TruePos+1} = $Param_Init{"TargetCall"};
            }
        }
        foreach my $Param_Pos (sort {int($a)<=>int($b)} keys(%{$CompleteSignature{$Interface}{"Param"}}))
        {
            next if($Param_Pos eq "");
            my $Param_Call = $AccessToParam{$Param_Pos + 1};
            my $ParamType_Id = $CompleteSignature{$Interface}{"Param"}{$Param_Pos}{"type"};
            if((get_TypeName($ParamType_Id) ne "..." and not $CompleteSignature{$Interface}{"Param"}{$Param_Pos}{"default"})
            or $Param_Call ne "") {
                push(@ParamList, $Param_Call);
            }
        }
        my $LastParamPos = keys(%{$CompleteSignature{$Interface}{"Param"}})-1;
        my $LastTypeId = $CompleteSignature{$Interface}{"Param"}{$LastParamPos}{"type"};
        my $LastParamCall = $AccessToParam{$LastParamPos+1};
        if(get_TypeName($LastTypeId) eq "..." and $LastParamCall ne "0" and $LastParamCall ne "NULL")
        {# add sentinel to function call
         # http://www.linuxonly.nl/docs/2/2_GCC_4_warnings_about_sentinels.html
            push(@ParamList, "(char*)0");
        }
        my $Parameters_Call = "(".create_list(\@ParamList, "    ").")";
        if($IsWrapperCall)
        {
            $Interface_Init{"Call"} .= "()";
            $Wrappers{$Interface}{"Init"} = $Params_Init{"Init"};
            $Wrappers{$Interface}{"Code"} = $Params_Init{"Code"};
            $Wrappers{$Interface}{"Destructors"} = $Params_Init{"Destructors"};
            $Wrappers{$Interface}{"FinalCode"} = $Params_Init{"FinalCode"};
            $Wrappers{$Interface}{"PreCondition"} = $Params_Init{"PreCondition"};
            foreach my $PreCondition (keys(%{$Interface_PreCondition{$Interface}})) {
                $Wrappers{$Interface}{"PreCondition"} .= constraint_for_interface($Interface, "precondition", $PreCondition);
            }
            $Wrappers{$Interface}{"PostCondition"} = $Params_Init{"PostCondition"};
            foreach my $PostCondition (keys(%{$Interface_PostCondition{$Interface}})) {
                $Wrappers{$Interface}{"PostCondition"} .= constraint_for_interface($Interface, "postcondition", $PostCondition);
            }
            $Wrappers{$Interface}{"Parameters_Call"} = $Parameters_Call;
            foreach my $ClassId (keys(%Create_SubClass)) {
                $Wrappers_SubClasses{$Interface}{$ClassId} = 1;
            }
        }
        else
        {
            $Interface_Init{"Call"} .= $Parameters_Call;
            $Interface_Init{"Init"} .= $Params_Init{"Init"};
            $Interface_Init{"Code"} .= $Params_Init{"Code"};
            $Interface_Init{"Destructors"} .= $Params_Init{"Destructors"};
            $Interface_Init{"FinalCode"} .= $Params_Init{"FinalCode"};
            $Interface_Init{"PreCondition"} .= $Params_Init{"PreCondition"};
            foreach my $PreCondition (keys(%{$Interface_PreCondition{$Interface}})) {
                $Interface_Init{"PreCondition"} .= constraint_for_interface($Interface, "precondition", $PreCondition);
            }
            $Interface_Init{"PostCondition"} .= $Params_Init{"PostCondition"};
            foreach my $PostCondition (keys(%{$Interface_PostCondition{$Interface}})) {
                $Interface_Init{"PostCondition"} .= constraint_for_interface($Interface, "postcondition", $PostCondition);
            }
        }
    }
    elsif($CompleteSignature{$Interface}{"Data"})
    {
        if($IsWrapperCall) {
            $Interface_Init{"Call"} .= "()";
        }
    }
    else
    {
        $Interface_Init{"Call"} .= "()";
        $Wrappers{$Interface}{"Parameters_Call"} = "()";
    }
    if($IsWrapperCall)
    {
        foreach my $ClassId (keys(%{$SubClasses_Before})) {
            $Create_SubClass{$ClassId} = 1;
        }
    }
    #check requirement for return value
    my $SpecReturnType = $InterfaceSpecType{$Interface}{"SpecReturn"};
    if(not $SpecReturnType) {
        $SpecReturnType = chooseSpecType($CompleteSignature{$Interface}{"Return"}, "common_retval", $Interface);
    }
    $Interface_Init{"ReturnRequirement"} = requirementReturn($Interface, $CompleteSignature{$Interface}{"Return"}, $SpecReturnType, $ObjectCall);
    if($SpecReturnType)
    {
        if(my $ReturnInitCode = $SpecType{$SpecReturnType}{"InitCode"})
        {
            my %ParsedCode = parseCode($ReturnInitCode, "Code");
            if($ParsedCode{"IsCorrect"})
            {
                $Interface_Init{"Headers"} = addHeaders($ParsedCode{"Headers"}, $Interface_Init{"Headers"});
                $Interface_Init{"Code"} .= $ParsedCode{"NewGlobalCode"};
                $Interface_Init{"Init"} .= $ParsedCode{"Code"};
            }
        }
        if(my $ReturnFinalCode = $SpecType{$SpecReturnType}{"FinalCode"})
        {
            my %ParsedCode = ();
            if($Init_Desc{"RetParam"}) {
                my $LastId = pop(@RecurTypeId);
                # add temp $retval
                $ValueCollection{$CurrentBlock}{"\$retval"} = $CompleteSignature{$Interface}{"Return"};
                # parse code using temp $retval
                %ParsedCode = parseCode($ReturnFinalCode, "Code");
                # remove temp $retval
                delete($ValueCollection{$CurrentBlock}{"\$retval"});
                push(@RecurTypeId, $LastId);
            }
            else {
                %ParsedCode = parseCode($ReturnFinalCode, "Code");
            }
            if($ParsedCode{"IsCorrect"})
            {
                $Interface_Init{"Headers"} = addHeaders($ParsedCode{"Headers"}, $Interface_Init{"Headers"});
                $Interface_Init{"Code"} .= $ParsedCode{"NewGlobalCode"};
                $Interface_Init{"ReturnFinalCode"} = $ParsedCode{"Code"};
            }
            else {
                $Interface_Init{"ReturnFinalCode"} = "";
            }
        }
    }
    foreach my $ParamId (keys %AccessToParam)
    {
        if($TargetAccessToParam{$ParamId} and ($TargetAccessToParam{$ParamId} ne "no object"))
        {
            my $AccessValue = $TargetAccessToParam{$ParamId};
            foreach my $Attr (keys(%Interface_Init)) {
                $Interface_Init{$Attr}=~s/\$\Q$ParamId\E([^0-9]|\Z)/$AccessValue$1/g;
            }
        }
    }
    $Interface_Init{"IsCorrect"} = 1;
    return %Interface_Init;
}

sub parse_param_name($$)
{
    my ($String, $Place) = @_;
    if($String=~/(([a-z_]\w+)[ ]*\(.+\))/i)
    {
        my ($Call, $Interface_ShortName) = ($1, $2);
        my $Pos = 0;
        foreach my $Part (get_Signature_Parts($Call, 0))
        {
            $Part=~s/(\A\s+|\s+\Z)//g;
            if($Part eq $Place)
            {
                if($CompleteSignature{$Interface_ShortName}) {
                    return ($CompleteSignature{$Interface_ShortName}{"Param"}{$Pos}{"name"}, $Pos, $Interface_ShortName);
                }
                else {
                    return (0, 0, "");
                }
            }
            $Pos+=1;
        }
    }
    return (0, 0, "");
}

sub parseCode_m($$)
{
    my ($Code, $Mode) = @_;
    return ("IsCorrect"=>1) if(not $Code or not $Mode);
    my ($Bracket_Num, $Code_Inlined, $NotEnded) = (0, "", 0);
    foreach my $Line (split(/\n/, $Code))
    {
        foreach my $Pos (0 .. length($Line) - 1)
        {
            my $Symbol = substr($Line, $Pos, 1);
            $Bracket_Num += 1 if($Symbol eq "(");
            $Bracket_Num -= 1 if($Symbol eq ")");
        }
        if($NotEnded and $Bracket_Num!=0) {
            $Line=~s/\A\s+/ /g;
        }
        $Code_Inlined .= $Line;
        if($Bracket_Num==0) {
            $Code_Inlined .= "\n";
        }
        else {
            $NotEnded = 1;
        }
    }
    $Code = $Code_Inlined;
    my ($AllSubCode, $ParsedCode, $Headers) = ();
    $Block_InsNum{$CurrentBlock} = 1 if(not defined $Block_InsNum{$CurrentBlock});
    if($Mode eq "Value") {
        $Code=~s/\n//g;
    }
    foreach my $String (split(/\n/, $Code))
    {
        if($String=~/\#[ \t]*include[ \t]*\<[ \t]*([^ \t]+)[ \t]*\>/)
        {
            $Headers = addHeaders($Headers, [$1]);
            next;
        }
        my ($CodeBefore, $CodeAfter) = ();
        while($String=~/(\$\(([^\$\(\)]+)\))/)
        {#parsing $(Type) constructions
            my $Replace = $1;
            my $TypeName = $2;
            my $TypeId = get_TypeIdByName($TypeName);
            my $FTypeId = get_FoundationTypeId($TypeId);
            my $NewKey = "_var".$Block_InsNum{$CurrentBlock};
            my ($FuncParamName, $FuncParamPos, $InterfaceShortName) = parse_param_name($String, $Replace);
            if($FuncParamName) {
                $NewKey = $FuncParamName;
            }
            my $InLine = 1;
            $InLine = 0 if(uncover_typedefs($TypeName)=~/\&/);
            $InLine = 0 if(get_TypeType($FTypeId)!~/\A(Intrinsic|Enum)\Z/ and $FuncParamName and $FuncParamName!~/\Ap\d+\Z/
                and not isCyclical(\@RecurTypeId, get_TypeStackId($TypeId)));
            my %Param_Init = initializeParameter((
                "AccessToParam" => {"obj"=>"no object"},
                "TypeId" => $TypeId,
                "Key" => $NewKey,
                "InLine" => $InLine,
                "Value" => "no value",
                "CreateChild" => 0,
                "SpecType" => ($FuncParamName and $InterfaceShortName)?$InterfaceSpecType{$InterfaceShortName}{"SpecParam"}{$FuncParamPos}:0,
                "Usage" => "Common",
                "ParamName" => $NewKey,
                "Interface" => $InterfaceShortName));
            return () if(not $Param_Init{"IsCorrect"} or $Param_Init{"ByNull"});
            $Block_InsNum{$CurrentBlock} += 1 if(($Param_Init{"Init"}.$Param_Init{"FinalCode"}.$Param_Init{"Code"})=~/\Q$NewKey\E/);
            $Param_Init{"Init"} = alignCode($Param_Init{"Init"}, $String, 0);
            $Param_Init{"PreCondition"} = alignCode($Param_Init{"PreCondition"}, $String, 0);
            $Param_Init{"PostCondition"} = alignCode($Param_Init{"PostCondition"}, $String, 0);
            $Param_Init{"Call"} = alignCode($Param_Init{"Call"}, $String, 1);
            substr($String, index($String, $Replace), pos($Replace) + length($Replace)) = $Param_Init{"Call"};
            $String = clearSyntax($String);
            $AllSubCode .= $Param_Init{"Code"};
            $Headers = addHeaders($Param_Init{"Headers"}, $Headers);
            $CodeBefore .= $Param_Init{"Init"}.$Param_Init{"PreCondition"};
            $CodeAfter .= $Param_Init{"PostCondition"}.$Param_Init{"FinalCode"};
        }
        while($String=~/(\$\[([^\$\[\]]+)\])/)
        {# parsing $[Interface] constructions
            my $Replace = $1;
            my $InterfaceName = $2;
            my $RetvalName = "";
            if($InterfaceName=~/\A(.+):(\w+?)\Z/)
            {# $[al_create_display:allegro_display]
                ($InterfaceName, $RetvalName) = ($1, $2);
            }
            my $NewKey = "_var".$Block_InsNum{$CurrentBlock};
            my %Interface_Init = ();
            return () if(not $InterfaceName or not $CompleteSignature{$InterfaceName});
            if($InterfaceName eq $TestedInterface)
            {# recursive call of the target interface
                substr($String, index($String, $Replace), pos($Replace) + length($Replace)) = "";
                $String = "" if($String eq ";");
                next;
            }
            if($CompleteSignature{$InterfaceName}{"Constructor"})
            {
                push(@RecurTypeId, $CompleteSignature{$InterfaceName}{"Class"});
                %Interface_Init = callInterface((
                    "Interface"=>$InterfaceName, 
                    "Key"=>$NewKey));
                pop(@RecurTypeId);
            }
            else
            {
                if($RetvalName) {
                    push(@RecurTypeId, get_TypeStackId($CompleteSignature{$InterfaceName}{"Return"}));
                }
                %Interface_Init = callInterface((
                    "Interface"=>$InterfaceName, 
                    "Key"=>$NewKey,
                    "RetParam"=>$RetvalName));
                if($RetvalName)
                {
                    pop(@RecurTypeId);
                    $Interface_Init{"ReturnFinalCode"}=~s/\$retval/$RetvalName/;
                }
            }
            return () if(not $Interface_Init{"IsCorrect"});
            $Block_InsNum{$CurrentBlock} += 1 if(($Interface_Init{"Init"}.$Interface_Init{"FinalCode"}.$Interface_Init{"ReturnFinalCode"}.$Interface_Init{"Code"})=~/\Q$NewKey\E/);
            if(($CompleteSignature{$InterfaceName}{"Constructor"}) and (needToInherit($InterfaceName)))
            {# for constructors in abstract classes
                    my $ClassName = get_TypeName($CompleteSignature{$InterfaceName}{"Class"});
                    my $ClassNameChild = getSubClassName($ClassName);
                    if($Interface_Init{"Call"}=~/\A(\Q$ClassName\E([\n]*)\()/) {
                        substr($Interface_Init{"Call"}, index($Interface_Init{"Call"}, $1), pos($1) + length($1)) = $ClassNameChild.$2."(";
                    }
                    $UsedConstructors{$CompleteSignature{$InterfaceName}{"Class"}}{$InterfaceName} = 1;
                    $IntSubClass{$TestedInterface}{$CompleteSignature{$InterfaceName}{"Class"}} = 1;
                    $Create_SubClass{$CompleteSignature{$InterfaceName}{"Class"}} = 1;
            }
            $Interface_Init{"Init"} = alignCode($Interface_Init{"Init"}, $String, 0);
            $Interface_Init{"PreCondition"} = alignCode($Interface_Init{"PreCondition"}, $String, 0);
            $Interface_Init{"PostCondition"} = alignCode($Interface_Init{"PostCondition"}, $String, 0);
            $Interface_Init{"FinalCode"} = alignCode($Interface_Init{"FinalCode"}, $String, 0);
            $Interface_Init{"ReturnFinalCode"} = alignCode($Interface_Init{"ReturnFinalCode"}, $String, 0);
            $Interface_Init{"Call"} = alignCode($Interface_Init{"Call"}, $String, 1);
            if($RetvalName)
            {
                $Block_Variable{$CurrentBlock}{$RetvalName} = 1;
                $ValueCollection{$CurrentBlock}{$RetvalName} = $CompleteSignature{$InterfaceName}{"Return"};
                $UseVarEveryWhere{$CurrentBlock}{$RetvalName} = 1;
                $Interface_Init{"Call"} = get_TypeName($CompleteSignature{$InterfaceName}{"Return"})." $RetvalName = ".$Interface_Init{"Call"};
            }
            substr($String, index($String, $Replace), pos($Replace) + length($Replace)) = $Interface_Init{"Call"};
            $AllSubCode .= $Interface_Init{"Code"};
            $Headers = addHeaders($Interface_Init{"Headers"}, $Headers);
            $CodeBefore .= $Interface_Init{"Init"}.$Interface_Init{"PreCondition"};
            $CodeAfter .= $Interface_Init{"PostCondition"}.$Interface_Init{"FinalCode"}.$Interface_Init{"ReturnFinalCode"};
        }
        $ParsedCode .= $CodeBefore.$String."\n".$CodeAfter;
        if($Mode eq "Value")
        {
            return ("NewGlobalCode" => $AllSubCode,
            "Code" => $String,
            "CodeBefore" => $CodeBefore,
            "CodeAfter" => $CodeAfter,
            "Headers" => $Headers,
            "IsCorrect" => 1);
        }
    }
    return ("NewGlobalCode" => $AllSubCode, "Code" => clearSyntax($ParsedCode), "Headers" => $Headers, "IsCorrect" => 1);
}

sub callInterface_m(@)
{
    my %Init_Desc = @_;
    my ($Interface, $Key) = ($Init_Desc{"Interface"}, $Init_Desc{"Key"});
    my $SpecObjectType = $InterfaceSpecType{$Interface}{"SpecObject"};
    my $SpecReturnType = $InterfaceSpecType{$Interface}{"SpecReturn"};
    my %Interface_Init = ();
    my $ClassName = get_TypeName($CompleteSignature{$Interface}{"Class"});
    my ($CreateChild, $CallAsGlobalData, $MethodToInitObj) = (0, 0, "Common");
    
    if(needToInherit($Interface) and isInCharge($Interface))
    {# impossible testing
        return ();
    }
    if($CompleteSignature{$Interface}{"Protected"})
    {
        if(not $CompleteSignature{$Interface}{"Constructor"}) {
            $UsedProtectedMethods{$CompleteSignature{$Interface}{"Class"}}{$Interface} = 1;
        }
        $IntSubClass{$TestedInterface}{$CompleteSignature{$Interface}{"Class"}} = 1;
        $Create_SubClass{$CompleteSignature{$Interface}{"Class"}} = 1;
        $CreateChild = 1;
    }
    if(($CompleteSignature{$Interface}{"Static"}) and (not $CompleteSignature{$Interface}{"Protected"}))
    {
        $MethodToInitObj = "OnlySpecType";
        $CallAsGlobalData = 1;
    }
    if($SpecReturnType and not isCyclical(\@RecurSpecType, $SpecReturnType))
    {
        my $SpecReturnCode = $SpecType{$SpecReturnType}{"Code"};
        if($SpecReturnCode) {
            push(@RecurSpecType, $SpecReturnType);
        }
        my $PreviousBlock = $CurrentBlock;
        $CurrentBlock = $CurrentBlock."_code_".$SpecReturnType;
        my %ParsedCode = parseCode($SpecType{$SpecReturnType}{"Code"}, "Code");
        $CurrentBlock = $PreviousBlock;
        if(not $ParsedCode{"IsCorrect"})
        {
            if($SpecReturnCode) {
                pop(@RecurSpecType);
            }
            return ();
        }
        $SpecCode{$SpecReturnType} = 1 if($ParsedCode{"Code"});
        $Interface_Init{"Code"} .= $ParsedCode{"NewGlobalCode"}.$ParsedCode{"Code"};
        $Interface_Init{"Headers"} = addHeaders($ParsedCode{"Headers"}, $Interface_Init{"Headers"});
        if($SpecReturnCode) {
            pop(@RecurSpecType);
        }
    }
    if($CompleteSignature{$Interface}{"Class"}
    and not $CompleteSignature{$Interface}{"Constructor"})
    {
        #initialize object
        my $ParamName = select_obj_name($Key, $CompleteSignature{$Interface}{"Class"});
        my $NewKey = ($Key)?$Key."_".$ParamName:$ParamName;
        if(not $SpecObjectType) {
            $SpecObjectType = chooseSpecType($CompleteSignature{$Interface}{"Class"}, "common_param", $Init_Desc{"Interface"});
        }
        my %Obj_Init = (not $Init_Desc{"ObjectCall"})?initializeParameter((
            "ParamName" => $ParamName,
            "Interface" => $Interface,
            "AccessToParam" => {"obj"=>"create object"},
            "TypeId" => $CompleteSignature{$Interface}{"Class"},
            "Key" => $NewKey,
            "InLine" => 0,
            "Value" => "no value",
            "CreateChild" => $CreateChild,
            "SpecType" => $SpecObjectType,
            "Usage" => $MethodToInitObj,
            "ConvertToBase" => (not $CompleteSignature{$Interface}{"Protected"}),
            "ObjectInit" =>1 )):("IsCorrect"=>1, "Call"=>$Init_Desc{"ObjectCall"});
        if(not $Obj_Init{"IsCorrect"}) {
            if($DebugMode) {
                $DebugInfo{"Init_Class"}{get_TypeName($CompleteSignature{$Interface}{"Class"})} = 1;
            }
            return ();
        }
        $Obj_Init{"Call"} = "no object" if($CallAsGlobalData);
        #initialize parameters
        pop(@RecurInterface);
        $Init_Desc{"ObjectCall"} = $Obj_Init{"Call"} if(not $Init_Desc{"ObjectCall"});
        my %Params_Init = callInterfaceParameters(%Init_Desc);
        push(@RecurInterface, $Interface);
        return () if(not $Params_Init{"IsCorrect"});
        $Interface_Init{"ReturnRequirement"} .= $Params_Init{"ReturnRequirement"};
        $Interface_Init{"ReturnFinalCode"} .= $Params_Init{"ReturnFinalCode"};
        $Interface_Init{"Init"} .= $Obj_Init{"Init"}.$Params_Init{"Init"};
        $Interface_Init{"Destructors"} .= $Params_Init{"Destructors"}.$Obj_Init{"Destructors"};
        $Interface_Init{"Headers"} = addHeaders($Params_Init{"Headers"}, $Interface_Init{"Headers"});
        $Interface_Init{"Headers"} = addHeaders($Obj_Init{"Headers"}, $Interface_Init{"Headers"});
        $Interface_Init{"Code"} .= $Obj_Init{"Code"}.$Params_Init{"Code"};
        $Interface_Init{"PreCondition"} .= $Obj_Init{"PreCondition"}.$Params_Init{"PreCondition"};
        $Interface_Init{"PostCondition"} .= $Obj_Init{"PostCondition"}.$Params_Init{"PostCondition"};
        $Interface_Init{"FinalCode"} .= $Obj_Init{"FinalCode"}.$Params_Init{"FinalCode"};
        #target call
        if($CallAsGlobalData) {
            $Interface_Init{"Call"} = $ClassName."::".$Params_Init{"Call"};
        }
        else
        {
            if(($Obj_Init{"Call"}=~/\A\*/) or ($Obj_Init{"Call"}=~/\A\&/)) {
                $Obj_Init{"Call"} = "(".$Obj_Init{"Call"}.")";
            }
            $Interface_Init{"Call"} = $Obj_Init{"Call"}.".".$Params_Init{"Call"};
            $Interface_Init{"Call"}=~s/\(\*(\w+)\)\./$1\-\>/;
            $Interface_Init{"Call"}=~s/\(\&(\w+)\)\-\>/$1\./;
        }
        #simplify operators
        $Interface_Init{"Call"} = simplifyOperator($Interface_Init{"Call"});
        $Interface_Init{"IsCorrect"} = 1;
        return %Interface_Init;
    }
    else
    {
        pop(@RecurInterface);
        $Init_Desc{"ObjectCall"} = "no object";
        my %Params_Init = callInterfaceParameters(%Init_Desc);
        push(@RecurInterface, $Interface);
        return () if(not $Params_Init{"IsCorrect"});
        $Interface_Init{"ReturnRequirement"} .= $Params_Init{"ReturnRequirement"};
        $Interface_Init{"ReturnFinalCode"} .= $Params_Init{"ReturnFinalCode"};
        $Interface_Init{"Init"} .= $Params_Init{"Init"};
        $Interface_Init{"Destructors"} .= $Params_Init{"Destructors"};
        $Interface_Init{"Headers"} = addHeaders($Params_Init{"Headers"}, $Interface_Init{"Headers"});
        $Interface_Init{"Code"} .= $Params_Init{"Code"};
        $Interface_Init{"PreCondition"} .= $Params_Init{"PreCondition"};
        $Interface_Init{"PostCondition"} .= $Params_Init{"PostCondition"};
        $Interface_Init{"FinalCode"} .= $Params_Init{"FinalCode"};
        $Interface_Init{"Call"} = $Params_Init{"Call"};
        if($CompleteSignature{$Interface}{"NameSpace"}
        and not $CompleteSignature{$Interface}{"Class"}) {
            $Interface_Init{"Call"} = $CompleteSignature{$Interface}{"NameSpace"}."::".$Interface_Init{"Call"};
        }
        $Interface_Init{"IsCorrect"} = 1;
        return %Interface_Init;
    }
}

sub simplifyOperator($)
{
    my $String = $_[0];
    if($String!~/\.operator/) {
        return $String;
    }
    return $String if($String!~/(.*)\.operator[ ]*([^()]+)\((.*)\)/);
    my $Target = $1;
    my $Operator = $2;
    my $Params = $3;
    if($Params eq "")
    {
        #prefix operator
        if($Operator=~/[a-z]/i) {
            return $String;
        }
        else {
            return $Operator.$Target;
        }
    }
    else
    {
        #postfix operator
        if($Params!~/\,/)
        {
            $Params = "" if(($Operator eq "++") or ($Operator eq "--"));
            if($Operator eq "[]") {
                return $Target."[$Params]";
            }
            else {
                return $Target.$Operator."$Params";
            }
        }
        else {
            return $Target.$Operator."($Params)";
        }
    }
}

sub callInterface(@)
{
    my %Init_Desc = @_;
    my $Interface = $Init_Desc{"Interface"};
    return () if(not $Interface);
    return () if($SkipInterfaces{$Interface});
    foreach my $SkipPattern (keys(%SkipInterfaces_Pattern)) {
        return () if($Interface=~/$SkipPattern/);
    }
    if(defined $MakeIsolated and $Interface_Library{$Interface}
    and keys(%InterfacesList) and not $InterfacesList{$Interface}) {
        return ();
    }
    my $Global_State = save_state();
    return () if(isCyclical(\@RecurInterface, $Interface));
    push(@RecurInterface, $Interface);
    $UsedInterfaces{$Interface} = 1;
    my %Interface_Init = callInterface_m(%Init_Desc);
    if(not $Interface_Init{"IsCorrect"})
    {
        pop(@RecurInterface);
        restore_state($Global_State);
        return ();
    }
    pop(@RecurInterface);
    $Interface_Init{"ReturnTypeId"} = $CompleteSignature{$Interface}{"Return"};
    return %Interface_Init;
}

sub get_REQ_define($)
{
    my $Interface = $_[0];
    my $Code = "#define REQ(id, failure_comment, constraint) { \\\n";
    $Code .= "    if(!(constraint)) { \\\n";
    $Code .= "        printf(\"\%s: \%s\\n\", id, failure_comment); \\\n    } \\\n";
    $Code .= "}\n";
    $FuncNames{"REQ"} = 1;
    $Block_Variable{"REQ"}{"id"} = 1;
    $Block_Variable{"REQ"}{"failure_comment"} = 1;
    $Block_Variable{"REQ"}{"constraint"} = 1;
    return $Code;
}

sub get_REQva_define($)
{
    my $Interface = $_[0];
    my $Code = "#define REQva(id, constraint, failure_comment_fmt, ...) { \\\n";
    $Code .= "    if(!(constraint)) { \\\n";
    $Code .= "        printf(\"\%s: \"failure_comment_fmt\"\\n\", id, __VA_ARGS__); \\\n    } \\\n";
    $Code .= "}\n";
    $FuncNames{"REQva"} = 1;
    $Block_Variable{"REQva"}{"id"} = 1;
    $Block_Variable{"REQva"}{"failure_comment"} = 1;
    $Block_Variable{"REQva"}{"constraint"} = 1;
    return $Code;
}

sub add_line_numbers($$)
{
    my ($Code, $AdditionalLines) = @_;
    my @Lines = split(/\n/, $Code);
    my $NewCode_LineNumbers = "\@LT\@table\@SP\@class='l_num'\@SP\@border='0'\@SP\@cellpadding='0'\@SP\@cellspacing='0'\@GT\@\@NL\@";
    my $NewCode_Lines = "\@LT\@table\@SP\@class='code_lines'\@SP\@border='0'\@SP\@cellpadding='0'\@SP\@cellspacing='0'\@GT\@\@NL\@";
    my $NewCode = "\@LT\@table\@SP\@border='0'\@SP\@cellpadding='0'\@SP\@cellspacing='0'\@GT\@\@NL\@";
    my $MaxLineNum = 0;
    foreach my $LineNum (0 .. $#Lines)
    {
        my $Line = $Lines[$LineNum];
        $Line = "    " if(not $Line);
        $NewCode_LineNumbers .= "\@LT\@tr\@GT\@\@LT\@td\@GT\@".($LineNum+1)."\@LT\@/td\@GT\@\@LT\@/tr\@GT\@\@NL\@";
        $NewCode_Lines .= "\@LT\@tr\@GT\@\@LT\@td\@GT\@$Line\@LT\@/td\@GT\@\@LT\@/tr\@GT\@\@NL\@";
        $MaxLineNum = $LineNum;
    }
    foreach my $LineNum (1 .. $AdditionalLines) {
        $NewCode_LineNumbers .= "\@LT\@tr\@GT\@\@LT\@td\@GT\@".($MaxLineNum+$LineNum+1)."\@LT\@/td\@GT\@\@LT\@/tr\@GT\@\@NL\@";
    }
    $NewCode_LineNumbers .= "\@LT\@/table\@GT\@\@NL\@";
    $NewCode_Lines .= "\@LT\@/table\@GT\@\@NL\@";
    $NewCode .= "\@LT\@tr\@GT\@\@LT\@td\@SP\@valign='top'\@GT\@$NewCode_LineNumbers\@LT\@/td\@GT\@\@LT\@td\@SP\@valign='top'\@GT\@$NewCode_Lines\@LT\@/td\@GT\@\@LT\@/tr\@GT\@\@LT\@/table\@GT\@\@NL\@";
    return $NewCode;
}

sub parse_variables($)
{
    my $Code = $_[0];
    return () if(not $Code);
    my $Code_Copy = $Code;
    my (%Variables, %LocalFuncNames, %LocalMethodNames) = ();
    while($Code=~s/([a-z_]\w*)[ ]*\([^;{}]*\)[ \n]*\{//io) {
        $LocalFuncNames{$1} = 1;
    }
    $Code = $Code_Copy;
    while($Code=~s/\:\:([a-z_]\w*)[ ]*\([^;{}]*\)[ \n]*\{//io) {
        $LocalMethodNames{$1} = 1;
    }
    foreach my $Block (sort keys(%Block_Variable))
    {
        foreach my $Variable (sort {length($b)<=>length($a)} keys(%{$Block_Variable{$Block}}))
        {
            next if(not $Variable);
            if($Code_Copy=~/\W$Variable[ ]*(,|(\n[ ]*|)\))/) {
                $Variables{$Variable}=1;
            }
            else
            {
                next if(is_not_variable($Variable, $Code_Copy));
                next if($LocalFuncNames{$Variable} and ($Code_Copy=~/\W\Q$Variable\E[ ]*\(/ or $Code_Copy=~/\&\Q$Variable\E\W/));
                next if($LocalMethodNames{$Variable} and $Code_Copy=~/\W\Q$Variable\E[ ]*\(/);
                $Variables{$Variable}=1;
            }
        }
    }
    while($Code=~s/[ ]+([a-z_]\w*)([ ]*=|;)//io)
    {
        my $Variable = $1;
        next if(is_not_variable($Variable, $Code_Copy));
        next if($LocalFuncNames{$Variable} and ($Code_Copy=~/\W\Q$Variable\E[ ]*\(/ or $Code_Copy=~/\&\Q$Variable\E\W/));
        next if($LocalMethodNames{$Variable} and $Code_Copy=~/\W\Q$Variable\E[ ]*\(/);
        $Variables{$Variable}=1;
    }
    while($Code=~s/(\(|,)[ ]*([a-z_]\w*)[ ]*(\)|,)//io)
    {
        my $Variable = $2;
        next if(is_not_variable($Variable, $Code_Copy));
        next if($LocalFuncNames{$Variable} and ($Code_Copy=~/\W\Q$Variable\E[ ]*\(/ or $Code_Copy=~/\&\Q$Variable\E\W/));
        next if($LocalMethodNames{$Variable} and $Code_Copy=~/\W\Q$Variable\E[ ]*\(/);
        $Variables{$Variable}=1;
    }
    my @Variables = keys(%Variables);
    return @Variables;
}

sub is_not_variable($$)
{
    my ($Variable, $Code) = @_;
    return 1 if($Variable=~/\A[A-Z_]+\Z/);
    # FIXME: more appropriate constants check
    return 1 if($TName_Tid{$Variable});
    return 1 if($EnumMembers{$Variable});
    return 1 if($NameSpaces{$Variable}
    and ($Code=~/\W\Q$Variable\E\:\:/ or $Code=~/\s+namespace\s+\Q$Variable\E\s*;/));
    return 1 if($IsKeyword{$Variable} or $Variable=~/\A(\d+)\Z|_SubClass/);
    return 1 if($Constants{$Variable});
    return 1 if($GlobVarNames{$Variable});
    return 1 if($FuncNames{$Variable} and ($Code=~/\W\Q$Variable\E[ ]*\(/ or $Code=~/\&\Q$Variable\E\W/));
    return 1 if($MethodNames{$Variable} and $Code=~/\W\Q$Variable\E[ ]*\(/);
    return 1 if($Code=~/(\-\>|\.|\:\:)\Q$Variable\E[ ]*\(/);
    return 1 if($AllDefines{$Variable});
    return 0;
}

sub highlight_code($$)
{
    my ($Code, $Interface) = @_;
    my $Signature = get_Signature($Interface);
    my %Preprocessor = ();
    my $PreprocessorNum = 1;
    my @Lines = split(/\n/, $Code);
    foreach my $LineNum (0 .. $#Lines)
    {
        my $Line = $Lines[$LineNum];
        if($Line=~/\A[ \t]*(#.+)\Z/)
        {
            my $LineNum_Define = $LineNum;
            my $Define = $1;
            while($Define=~/\\[ \t]*\Z/)
            {
                $LineNum_Define+=1;
                $Define .= "\n".$Lines[$LineNum_Define];
            }
            if($Code=~s/\Q$Define\E/\@PREPROC_$PreprocessorNum\@/)
            {
                $Preprocessor{$PreprocessorNum} = $Define;
                $PreprocessorNum+=1;
            }
        }
    }
    my %Strings_DQ = ();
    my $StrNum_DQ = 1;
    while($Code=~s/((L|)"[^"]*")/\@STR_DQ_$StrNum_DQ\@/)
    {
        $Strings_DQ{$StrNum_DQ} = $1;
        $StrNum_DQ += 1;
    }
    my %Strings = ();
    my $StrNum = 1;
    while($Code=~s/((?<=\W)(L|)'[^']*')/\@STR_$StrNum\@/)
    {
        $Strings{$StrNum} = $1;
        $StrNum += 1;
    }
    my %Comments = ();
    my $CommentNum = 1;
    while($Code=~s/([^:]|\A)(\/\/[^\n]*)\n/$1\@COMMENT_$CommentNum\@\n/)
    {
        $Comments{$CommentNum} = $2;
        $CommentNum += 1;
    }
    if(my $ShortName = ($CompleteSignature{$Interface}{"Constructor"})?get_TypeName($CompleteSignature{$Interface}{"Class"}):$CompleteSignature{$Interface}{"ShortName"})
    {# target interface
        if($CompleteSignature{$Interface}{"Class"})
        {
            while($ShortName=~s/\A\w+\:\://g){ };
            if($CompleteSignature{$Interface}{"Constructor"}) {
                $Code=~s!(\:| new |\n    )(\Q$ShortName\E)([ \n]*\()!$1\@LT\@span\@SP\@class='targ'\@GT\@$2\@LT\@/span\@GT\@$3!g;
            }
            elsif($CompleteSignature{$Interface}{"Destructor"}) {
                $Code=~s!(\n    )(delete)([ \n]*\()!$1\@LT\@span\@SP\@class='targ'\@GT\@$2\@LT\@/span\@GT\@$3!g;
            }
            else {
                $Code=~s!(\-\>|\.|\:\:| new )(\Q$ShortName\E)([ \n]*\()!$1\@LT\@span\@SP\@class='targ'\@GT\@$2\@LT\@/span\@GT\@$3!g;
            }
        }
        else {
            $Code=~s!( )(\Q$ShortName\E)([ \n]*\()!$1\@LT\@span\@SP\@class='targ'\@GT\@$2\@LT\@/span\@GT\@$3!g;
        }
    }
    my %Variables = ();
    foreach my $Variable (parse_variables($Code))
    {
        if($Code=~s#(?<=[^\w\n.:])($Variable)(?=\W)#\@LT\@span\@SP\@class='var'\@GT\@$1\@LT\@/span\@GT\@#g) {
            $Variables{$Variable}=1;
        }
    }
    $Code=~s!(?<=[^.\w])(bool|_Bool|_Complex|complex|void|const|int|long|short|float|double|volatile|restrict|char|unsigned|signed)(?=[^\w\=])!\@LT\@span\@SP\@class='type'\@GT\@$1\@LT\@/span\@GT\@!g;
    $Code=~s!(?<=[^.\w])(false|true|namespace|return|struct|enum|union|public|protected|private|delete|typedef)(?=[^\w\=])!\@LT\@span\@SP\@class='keyw'\@GT\@$1\@LT\@/span\@GT\@!g;
    if(not $Variables{"class"}) {
        $Code=~s!(?<=[^.\w])(class)(?=[^\w\=])!\@LT\@span\@SP\@class='keyw'\@GT\@$1\@LT\@/span\@GT\@!g;
    }
    if(not $Variables{"new"}) {
        $Code=~s!(?<=[^.\w])(new)(?=[^\w\=])!\@LT\@span\@SP\@class='keyw'\@GT\@$1\@LT\@/span\@GT\@!g;
    }
    $Code=~s!(?<=[^.\w])(for|if|else if)([ \n]*\()(?=[^\w\=])!\@LT\@span\@SP\@class='keyw'\@GT\@$1\@LT\@/span\@GT\@$2!g;
    $Code=~s!(?<=[^.\w])else([ \n\{]+)(?=[^\w\=])!\@LT\@span\@SP\@class='keyw'\@GT\@else\@LT\@/span\@GT\@$1!g;
    $Code=~s!(?<=[^\w\@\$])(\d+(f|L|LL|)|NULL)(?=[^\w\@\$])!\@LT\@span\@SP\@class='num'\@GT\@$1\@LT\@/span\@GT\@!g;
    $Code=~s!(?<=[^\w\@\$])(0x[a-fA-F\d]{4})(?=[^\w\@\$])!\@LT\@span\@SP\@class='num'\@GT\@$1\@LT\@/span\@GT\@!g;
    foreach my $Num (keys(%Comments))
    {
        my $String = $Comments{$Num};
        $Code=~s!\@COMMENT_$Num\@!\@LT\@span\@SP\@class='comm'\@GT\@$String\@LT\@/span\@GT\@!g;
    }
    my $AdditionalLines = 0;
    foreach my $Num (keys(%Preprocessor))
    {
        my $Define = $Preprocessor{$Num};
        while($Define=~s/\n//) {
            $AdditionalLines+=1;
        }
    }
    $Code = add_line_numbers($Code, $AdditionalLines);
    #$Code=~s/\n/\@LT\@br\/\@GT\@\n/g;
    foreach my $Num (keys(%Preprocessor))
    {
        my $Define = $Preprocessor{$Num};
        $Code=~s!\@PREPROC_$Num\@!\@LT\@span\@SP\@class='prepr'\@GT\@$Define\@LT\@/span\@GT\@!g;
    }
    foreach my $Num (keys(%Strings_DQ))
    {
        my $String = $Strings_DQ{$Num};
        $Code=~s!\@STR_DQ_$Num\@!\@LT\@span\@SP\@class='str'\@GT\@$String\@LT\@/span\@GT\@!g;
    }
    foreach my $Num (keys(%Strings))
    {
        my $String = $Strings{$Num};
        $Code=~s!\@STR_$Num\@!\@LT\@span\@SP\@class='str'\@GT\@$String\@LT\@/span\@GT\@!g;
    }
    $Code =~ s!\[\]![\@LT\@span\@SP\@style='padding-left:2px;'\@GT\@]\@LT\@/span\@GT\@!g;
    $Code =~ s!\(\)!(\@LT\@span\@SP\@style='padding-left:2px;'\@GT\@)\@LT\@/span\@GT\@!g;
    return $Code;
}

sub is_process_running($) {
    my ($PID, $procname) = @_;
    if (!-e "/proc/$PID") {
        return 0;
    }
    open(FILE, "/proc/$PID/stat") or return 0;
    my $info = <FILE>;
    close(FILE);
    if ($info=~/^\d+\s+\((.*)\)\s+(\S)\s+[^\(\)]+$/) {
        return ($2 ne 'Z');
    }
    else {
        return 0;
    }
}

sub kill_all_childs($)
{
    my $root_pid = $_[0];
    return if(not $root_pid);
    # Build the list of processes to be killed.
    # Sub-tree of this particular process is excluded so that it could finish its work.
    my %children = ();
    my %parent = ();
    # Read list of all currently running processes
    if(!opendir(PROC_DIR, "/proc"))
    {
        kill(9, $root_pid);
        return;
    }
    my @all_pids = grep(/^\d+$/, readdir(PROC_DIR));
    closedir(PROC_DIR);
    # Build the parent-child tree and get command lines
    foreach my $pid (@all_pids) {
        if (open(PID_FILE, "/proc/$pid/stat")) {
            my $info = <PID_FILE>;
            close(PID_FILE);
            if ($info=~/^\d+\s+\((.*)\)\s+\S\s+(\d+)\s+[^\(\)]+$/) {
                my $ppid = $2;
                $parent{$pid} = $ppid;
                if (!defined($children{$ppid})) {
                    $children{$ppid} = [];
                }
                push @{$children{$ppid}}, $pid;
            }
        }
    }
    # Get the plain list of processes to kill (breadth-first tree-walk)
    my @kill_list = ($root_pid);
    for (my $i = 0; $i < scalar(@kill_list); ++$i) {
        my $pid = $kill_list[$i];
        if ($children{$pid}) {
            foreach (@{$children{$pid}}) {
                push @kill_list, $_;
            }
        }
    }
    # Send TERM signal to all processes
    foreach (@kill_list) {
        kill("SIGTERM", $_);
    }
    # Try 20 times, waiting 0.3 seconds each time, for all the processes to be really dead.
    my %death_check = map { $_ => 1 } @kill_list;
    for (my $i = 0; $i < 20; ++$i) {
        foreach (keys %death_check) {
            if (!is_process_running($_)) {
                delete $death_check{$_};
            }
        }
        if (scalar(keys %death_check) == 0) {
            last;
        }
        else {
            select(undef, undef, undef, 0.3);
        }
    }
}

sub filt_output($)
{
    my $Output = $_[0];
    return $Output if(not keys(%SkipWarnings) and not keys(%SkipWarnings_Pattern));
    my @NewOutput = ();
    foreach my $Line (split(/\n/, $Output))
    {
        my $IsMatched = 0;
        foreach my $Warning (keys(%SkipWarnings))
        {
            if($Line=~/\Q$Warning\E/) {
                $IsMatched = 1;
            }
        }
        foreach my $Warning (keys(%SkipWarnings_Pattern))
        {
            if($Line=~/$Warning/) {
                $IsMatched = 1;
            }
        }
        if(not $IsMatched) {
            push(@NewOutput, $Line);
        }
    }
    my $FinalOut = join("\n", @NewOutput);
    $FinalOut=~s/\A[\n]+//g;
    return $FinalOut;
}

sub createTestRunner()
{# C-utility for running tests under Windows
    rmtree("test_runner/");
    my $CL = get_CmdPath("cl");
    if(not $CL) {
        print STDERR "ERROR: can't find \"cl\" compiler\n";
        exit(1);
    }
    writeFile("test_runner/test_runner.cpp","
    #include <windows.h>
    #include <stdio.h>
    int main(int argc, char *argv[])
    {
        char* cmd = argv[1];
        char* directory = argv[2];
        char* res = argv[3];
        STARTUPINFO si;
        PROCESS_INFORMATION pi;
        ZeroMemory( &si, sizeof(STARTUPINFO));
        si.cb = sizeof(STARTUPINFO);
        ZeroMemory( &pi, sizeof(PROCESS_INFORMATION));
        if(CreateProcess(NULL, cmd, NULL, NULL, FALSE, DEBUG_PROCESS,
        NULL, directory, &si, &pi) == 0) {
            return 1;
        }
        FILE * result = fopen(res, \"w+\");
        if(result==NULL) {
            return 1;
        }
        DEBUG_EVENT de;
        DWORD ecode;
        int done = 0;
        for(;;)
        {
            if(WaitForDebugEvent(&de, INFINITE)==0)
                break;
            switch (de.dwDebugEventCode)
            {
                case EXCEPTION_DEBUG_EVENT:
                    ecode = de.u.Exception.ExceptionRecord.ExceptionCode;
                    if (ecode!=EXCEPTION_BREAKPOINT &&
                    ecode!=EXCEPTION_SINGLE_STEP)
                    {
                        fprintf(result, \"\%x;0\", ecode);
                        printf(\"\%x\\n\", ecode);
                        TerminateProcess(pi.hProcess, 0);
                        done = 1;
                    }
                    break;
                case EXIT_PROCESS_DEBUG_EVENT:
                    done = 1;
            }
            if(done==1)
                break;
            ContinueDebugEvent(de.dwProcessId, de.dwThreadId, DBG_CONTINUE);
        }
        fclose(result);
        return 0;
    }
    ");
    system("cd test_runner && $CL test_runner.cpp >build_log 2>&1");
    if($?) {
        print "ERROR: can't compile test runner\n";
        exit(1);
    }
}

my %WindowsExceptions=(
    "c0000005" => "ACCESS_VIOLATION",
    "c00002c5" => "DATATYPE_MISALIGNMENT",
    "c000008c" => "ARRAY_BOUNDS_EXCEEDED",
    "c000008d" => "FLOAT_DENORMAL_OPERAND",
    "c000008e" => "FLOAT_DIVIDE_BY_ZERO",
    "c000008f" => "FLOAT_INEXACT_RESULT",
    "c0000090" => "FLOAT_INVALID_OPERATION",
    "c0000091" => "FLOAT_OVERFLOW",
    "c0000092" => "FLOAT_STACK_CHECK",
    "c0000093" => "FLOAT_UNDERFLOW",
    "c0000094" => "INTEGER_DIVIDE_BY_ZERO",
    "c0000095" => "INTEGER_OVERFLOW",
    "c0000096" => "PRIVILEGED_INSTRUCTION",
    "c0000006" => "IN_PAGE_ERROR",
    "c000001d" => "ILLEGAL_INSTRUCTION",
    "c0000025" => "NONCONTINUABLE_EXCEPTION",
    "c00000fd" => "STACK_OVERFLOW",
    "c0000026" => "INVALID_DISPOSITION",
    "80000001" => "GUARD_PAGE_VIOLATION",
    "c0000008" => "INVALID_HANDLE",
    "c0000135" => "DLL_NOT_FOUND"
);

sub run_sanity_test($)
{
    my $Interface = $_[0];
    my $TestDir = $Interface_TestDir{$Interface};
    if(not $TestDir)
    {
        $ResultCounter{"Run"}{"Fail"} += 1;
        $RunResult{$Interface}{"IsCorrect"} = 0;
        $RunResult{$Interface}{"TestNotExists"} = 1;
        if($TargetInterfaceName) {
            print "fail\n";
            print STDERR "ERROR: test is not generated yet\n"
        }
        return 1;
    }
    elsif(not -f $TestDir."/test" and not -f $TestDir."/test.exe")
    {
        $ResultCounter{"Run"}{"Fail"} += 1;
        $RunResult{$Interface}{"IsCorrect"} = 0;
        $RunResult{$Interface}{"TestNotExists"} = 1;
        if($TargetInterfaceName) {
            print "fail\n";
            print STDERR "ERROR: test is not built yet\n"
        }
        return 1;
    }
    unlink($TestDir."/result");
    my $pid = fork();
    unless($pid)
    {
        if($OSgroup eq "windows")
        {
            my $ProcCmd = "test_runner/test_runner.exe \"".abs_path($TestDir)."/run_test.bat\" \"$TestDir\" \"".abs_path($TestDir)."/result\" >nul 2>&1";
            $ProcCmd=~s/[\/\\]/$SLASH/g;
            system($ProcCmd);
        }
        else
        {
            open(STDIN,"$TMP_DIR/null");
            open(STDOUT,"$TMP_DIR/null");
            open(STDERR,"$TMP_DIR/null");
            setsid(); # to remove signals printing on the terminal screen
            system("cd ".esc($TestDir)." && sh run_test.sh 2>stderr");
            writeFile("$TestDir/result", $?.";".$!);
        }
        exit(0);
    }
    my $Hang = 0;
    $SIG{ALRM} = sub {
        $Hang=1;
        if($OSgroup eq "windows") {
            kill(9, $pid);
        }
        else {
            kill_all_childs($pid);
        }
    };
    alarm $HANGED_EXECUTION_TIME;
    waitpid($pid, 0);
    alarm 0;
    my $Result = readFile("$TestDir/result");
    unlink($TestDir."/result");
    unlink("$TestDir/output") if(not readFile("$TestDir/output"));
    unlink("$TestDir/stderr") if(not readFile("$TestDir/stderr"));
    my ($R_1, $R_2) = split(";", $Result);
    my $ErrorOut = readFile("$TestDir/output");#checking test output
    $ErrorOut = filt_output($ErrorOut);
    if($ErrorOut)
    {# reduce length of the test output
        if(length($ErrorOut)>1200) {
            $ErrorOut = substr($ErrorOut, 0, 1200)." ...";
        }
    }
    if($Hang)
    {
        $ResultCounter{"Run"}{"Fail"} += 1;
        $RunResult{$Interface}{"IsCorrect"} = 0;
        $RunResult{$Interface}{"Type"} = "Hanged_Execution";
        $RunResult{$Interface}{"Info"} = "hanged execution (more than $HANGED_EXECUTION_TIME seconds)";
        $RunResult{$Interface}{"Info"} .= "\n".$ErrorOut if($ErrorOut);
    }
    elsif($R_1)
    {
        if($OSgroup eq "windows")
        {
            my $ExceptionName = $WindowsExceptions{$R_1};
            $RunResult{$Interface}{"Info"} = "received exception $ExceptionName\n";
            $RunResult{$Interface}{"Type"} = "Received_Exception";
            $RunResult{$Interface}{"Value"} = $ExceptionName;
        }
        else
        {
            if ($R_1 == -1) {
                $RunResult{$Interface}{"Info"} = "failed to execute: $R_2\n";
                $RunResult{$Interface}{"Type"} = "Other_Problems";
            }
            elsif (my $Signal_Num = ($R_1 & 127)) {
                my $Signal_Name = $SigName{$Signal_Num};
                $RunResult{$Interface}{"Info"} = "received signal $Signal_Name, ".(($R_1 & 128)?"with":"without")." coredump\n";
                $RunResult{$Interface}{"Type"} = "Received_Signal";
                $RunResult{$Interface}{"Value"} = ($R_1 & 127);
            }
            else {
                my $Signal_Num = ($R_1 >> 8)-128;
                my $Signal_Name = $SigName{$Signal_Num};
                if($Signal_Name)
                {
                    $RunResult{$Interface}{"Info"} = "received signal $Signal_Name\n";
                    $RunResult{$Interface}{"Type"} = "Received_Signal";
                    $RunResult{$Interface}{"Value"} = $Signal_Name;
                }
                else
                {
                    $RunResult{$Interface}{"Info"} = "exited with value ".($R_1 >> 8)."\n";
                    $RunResult{$Interface}{"Type"} = "Exited_With_Value";
                    $RunResult{$Interface}{"Value"} = ($R_1 >> 8);
                }
            }
        }
        $ResultCounter{"Run"}{"Fail"} += 1;
        $RunResult{$Interface}{"IsCorrect"} = 0;
        $RunResult{$Interface}{"Info"} .= "\n".$ErrorOut if($ErrorOut);
    }
    elsif(readFile($TestDir."/output")=~/(constraint|postcondition|precondition) for the (return value|object|environment|parameter) failed/i)
    {
        $ResultCounter{"Run"}{"Fail"} += 1;
        $RunResult{$Interface}{"IsCorrect"} = 0;
        $RunResult{$Interface}{"Type"} = "Requirement_Failed";
        $RunResult{$Interface}{"Info"} .= "\n".$ErrorOut if($ErrorOut);
    }
    elsif($ErrorOut)
    {
        $ResultCounter{"Run"}{"Fail"} += 1;
        $RunResult{$Interface}{"Unexpected_Output"} = $ErrorOut;
        $RunResult{$Interface}{"Type"} = "Unexpected_Output";
        $RunResult{$Interface}{"Info"} = $ErrorOut;
    }
    else
    {
        $ResultCounter{"Run"}{"Success"} += 1;
        $RunResult{$Interface}{"IsCorrect"} = 1;
    }
    if(not $RunResult{$Interface}{"IsCorrect"})
    {
        return 0 if(not -e $TestDir."/test.c" and not -e $TestDir."/test.cpp");
        my $ReadingStarted = 0;
        foreach my $Line (split(/\n/, readFile($TestDir."/view.html")))
        {
            if($ReadingStarted) {
                $RunResult{$Interface}{"Test"} .= $Line;
            }
            if($Line eq "<!--Test-->") {
                $ReadingStarted = 1;
            }
            if($Line eq "<!--Test_End-->") {
                last;
            }
        }
        my $Test_Info = readFile($TestDir."/info");
        foreach my $Str (split(/\n/, $Test_Info))
        {
            if($Str=~/\A[ ]*([^:]*?)[ ]*\:[ ]*(.*)[ ]*\Z/i)
            {
                my ($Attr, $Value) = ($1, $2);
                if(lc($Attr) eq "header") {
                    $RunResult{$Interface}{"Header"} = $Value;
                }
                elsif(lc($Attr) eq "shared object") {
                    $RunResult{$Interface}{"SharedObject"} = $Value;
                }
                elsif(lc($Attr) eq "interface") {
                    $RunResult{$Interface}{"Signature"} = $Value;
                }
                elsif(lc($Attr) eq "short name") {
                    $RunResult{$Interface}{"ShortName"} = $Value;
                }
                elsif(lc($Attr) eq "namespace") {
                    $RunResult{$Interface}{"NameSpace"} = $Value;
                }
            }
        }
        $RunResult{$Interface}{"ShortName"} = $Interface if(not $RunResult{$Interface}{"ShortName"});
        # filtering problems
        if($RunResult{$Interface}{"Type"} eq "Exited_With_Value")
        {
            if($RunResult{$Interface}{"ShortName"}=~/exit|die|assert/i) {
                skip_problem($Interface);
            }
            else {
                mark_as_warning($Interface);
            }
        }
        elsif($RunResult{$Interface}{"Type"} eq "Hanged_Execution")
        {
            if($RunResult{$Interface}{"ShortName"}=~/call|exec|acquire|start|run|loop|blocking|startblock|wait|time|show|suspend|pause/i
            or ($Interface=~/internal|private/ and $RunResult{$Interface}{"ShortName"}!~/private(.*)key/i)) {
                mark_as_warning($Interface);
            }
        }
        elsif($RunResult{$Interface}{"Type"} eq "Received_Signal")
        {
            if($RunResult{$Interface}{"ShortName"}=~/badalloc|bad_alloc|fatal|assert/i) {
                skip_problem($Interface);
            }
            elsif($Interface=~/internal|private/ and $RunResult{$Interface}{"ShortName"}!~/private(.*)key/i) {
                mark_as_warning($Interface);
            }
            elsif($RunResult{$Interface}{"Value"}!~/\A(SEGV|FPE|BUS|ILL|PIPE|SYS|XCPU|XFSZ)\Z/) {
                mark_as_warning($Interface);
            }
        }
        elsif($RunResult{$Interface}{"Type"} eq "Unexpected_Output")
        {
            if($Interface=~/print|debug|warn|message|error|fatal/i) {
                skip_problem($Interface);
            }
            else {
                mark_as_warning($Interface);
            }
        }
        elsif($RunResult{$Interface}{"Type"} eq "Other_Problems") {
            mark_as_warning($Interface);
        }
    }
    return 0;
}

sub mark_as_warning($)
{
    my $Interface = $_[0];
    $RunResult{$Interface}{"Warnings"} = 1;
    $ResultCounter{"Run"}{"Warnings"} += 1;
    $ResultCounter{"Run"}{"Fail"} -= 1;
    $ResultCounter{"Run"}{"Success"} += 1;
    $RunResult{$Interface}{"IsCorrect"} = 1;
}

sub skip_problem($)
{
    my $Interface = $_[0];
    $ResultCounter{"Run"}{"Fail"} -= 1;
    $ResultCounter{"Run"}{"Success"} += 1;
    delete($RunResult{$Interface});
    $RunResult{$Interface}{"IsCorrect"} = 1;
}

sub print_highlight($$)
{
    my ($Str, $Part) = @_;
    $ENV{"GREP_COLOR"}="01;36";
    system("echo \"$Str\"|grep \"$Part\" --color");
}

sub read_scenario()
{
    foreach my $TestCase (split(/\n/, readFile($TEST_SUITE_PATH."/scenario")))
    {
        if($TestCase=~/\A(.*);(.*)\Z/) {
            $Interface_TestDir{$1} = $2;
        }
    }
}

sub write_scenario()
{
    my $TestCases = "";
    foreach my $Interface (sort {lc($a) cmp lc($b)} keys(%Interface_TestDir)) {
        $TestCases .= $Interface.";".$Interface_TestDir{$Interface}."\n";
    }
    writeFile("$TEST_SUITE_PATH/scenario", $TestCases);
}

sub build_sanity_test($)
{
    my $Interface = $_[0];
    my $TestDir = $Interface_TestDir{$Interface};
    if(not $TestDir or not -f "$TestDir/Makefile")
    {
        $BuildResult{$Interface}{"TestNotExists"} = 1;
        if($TargetInterfaceName) {
            print "fail\n";
            print STDERR "ERROR: test is not generated yet\n"
        }
        return 0;
    }
    my $MakeCmdName = ($OSgroup eq "windows")?"nmake":"make";
    my $MakeCmd = get_CmdPath($MakeCmdName);
    if(not $MakeCmd) {
        print STDERR "ERROR: can't find \'$MakeCmdName\'\n";
        exit(1);
    }
    system("cd ".esc($TestDir)." && $MakeCmd clean -f Makefile 2>build_log >$TMP_DIR/null && $MakeCmd -f Makefile 2>build_log >$TMP_DIR/null");
    if($?) {
        $ResultCounter{"Build"}{"Fail"} += 1;
        $BuildResult{$Interface}{"IsCorrect"} = 0;
    }
    else {
        $ResultCounter{"Build"}{"Success"} += 1;
        $BuildResult{$Interface}{"IsCorrect"} = 1;
    }
    unlink("$TestDir/test.o");
    unlink("$TestDir/test.obj");
    if(not readFile("$TestDir/build_log")) {
        unlink("$TestDir/build_log");
    }
    elsif($BuildResult{$Interface}{"IsCorrect"}) {
        $BuildResult{$Interface}{"Warnings"} = 1;
    }
}

sub clean_sanity_test($)
{
    my $Interface = $_[0];
    my $TestDir = $Interface_TestDir{$Interface};
    if(not $TestDir or not -f "$TestDir/Makefile")
    {
        $BuildResult{$Interface}{"TestNotExists"} = 1;
        if($TargetInterfaceName) {
            print "fail\n";
            print STDERR "ERROR: test is not generated yet\n";
        }
        return 0;
    }
    unlink("$TestDir/test.o");
    unlink("$TestDir/test.obj");
    unlink("$TestDir/test");
    unlink("$TestDir/test.exe");
    unlink("$TestDir/build_log");
    unlink("$TestDir/output");
    unlink("$TestDir/stderr");
    rmtree("$TestDir/testdata");
    if($CleanSources)
    {
        foreach my $Path (cmd_find($TestDir,"f","",""))
        {
            if(get_filename($Path) ne "view.html") {
                unlink($Path);
            }
        }
    }
    return 1;
}

sub testForDestructor($)
{
    my $Interface = $_[0];
    my $ClassId = $CompleteSignature{$Interface}{"Class"};
    my $ClassName = get_TypeName($ClassId);
    my %Interface_Init = ();
    my $Var = select_obj_name("", $ClassId);
    $Block_Variable{$CurrentBlock}{$Var} = 1;
    if($Interface=~/D2/)
    {
        #push(@RecurTypeId, $ClassId);
        my %Obj_Init = findConstructor($ClassId, "");
        #pop(@RecurTypeId);
        return () if(not $Obj_Init{"IsCorrect"});
        my $ClassNameChild = getSubClassName($ClassName);
        if($Obj_Init{"Call"}=~/\A(\Q$ClassName\E([\n]*)\()/) {
            substr($Obj_Init{"Call"}, index($Obj_Init{"Call"}, $1), pos($1) + length($1)) = $ClassNameChild.$2."(";
        }
        $ClassName = $ClassNameChild;
        $UsedConstructors{$ClassId}{$Obj_Init{"Interface"}} = 1;
        $IntSubClass{$TestedInterface}{$ClassId} = 1;
        $Create_SubClass{$ClassId} = 1;
        $Interface_Init{"Init"} .= $Obj_Init{"Init"};
        #$Interface_Init{"Init"} .= "//parameter initialization\n";
        if($Obj_Init{"PreCondition"}) {
            $Interface_Init{"Init"} .= $Obj_Init{"PreCondition"};
        }
        $Interface_Init{"Init"} .= "$ClassName *$Var = new ".$Obj_Init{"Call"}.";\n";
        if($Obj_Init{"PostCondition"}) {
            $Interface_Init{"Init"} .= $Obj_Init{"PostCondition"};
        }
        if($Obj_Init{"ReturnRequirement"})
        {
            $Obj_Init{"ReturnRequirement"}=~s/(\$0|\$obj)/*$Var/gi;
            $Interface_Init{"Init"} .= $Obj_Init{"ReturnRequirement"};
        }
        if($Obj_Init{"FinalCode"})
        {
            $Interface_Init{"Init"} .= "//final code\n";
            $Interface_Init{"Init"} .= $Obj_Init{"FinalCode"}."\n";
        }
        $Interface_Init{"Headers"} = addHeaders($Obj_Init{"Headers"}, $Interface_Init{"Headers"});
        $Interface_Init{"Code"} .= $Obj_Init{"Code"};
        $Interface_Init{"Call"} = "delete($Var)";
    }
    elsif($Interface=~/D0/)
    {
        if(isAbstractClass($ClassId))
        {#Impossible to call in-charge-deleting (D0) destructor in abstract class
            return ();
        }
        if($CompleteSignature{$Interface}{"Protected"})
        {#Impossible to call protected in-charge-deleting (D0) destructor
            return ();
        }
        #push(@RecurTypeId, $ClassId);
        my %Obj_Init = findConstructor($ClassId, "");
        #pop(@RecurTypeId);
        return () if(not $Obj_Init{"IsCorrect"});
        if($CompleteSignature{$Obj_Init{"Interface"}}{"Protected"})
        {#Impossible to call in-charge-deleting (D0) destructor in class with protected constructor
            return ();
        }
        $Interface_Init{"Init"} .= $Obj_Init{"Init"};
        if($Obj_Init{"PreCondition"}) {
            $Interface_Init{"Init"} .= $Obj_Init{"PreCondition"};
        }
        #$Interface_Init{"Init"} .= "//parameter initialization\n";
        $Interface_Init{"Init"} .= $ClassName." *$Var = new ".$Obj_Init{"Call"}.";\n";
        if($Obj_Init{"PostCondition"}) {
            $Interface_Init{"Init"} .= $Obj_Init{"PostCondition"};
        }
        if($Obj_Init{"ReturnRequirement"})
        {
            $Obj_Init{"ReturnRequirement"}=~s/(\$0|\$obj)/*$Var/gi;
            $Interface_Init{"Init"} .= $Obj_Init{"ReturnRequirement"}
        }
        if($Obj_Init{"FinalCode"})
        {
            $Interface_Init{"Init"} .= "//final code\n";
            $Interface_Init{"Init"} .= $Obj_Init{"FinalCode"}."\n";
        }
        $Interface_Init{"Headers"} = addHeaders($Obj_Init{"Headers"}, $Interface_Init{"Headers"});
        $Interface_Init{"Code"} .= $Obj_Init{"Code"};
        $Interface_Init{"Call"} = "delete($Var)";
    }
    elsif($Interface=~/D1/)
    {
        if(isAbstractClass($ClassId))
        {#Impossible to call in-charge (D1) destructor in abstract class
            return ();
        }
        if($CompleteSignature{$Interface}{"Protected"})
        {#Impossible to call protected in-charge (D1) destructor
            return ();
        }
        #push(@RecurTypeId, $ClassId);
        my %Obj_Init = findConstructor($ClassId, "");
        #pop(@RecurTypeId);
        return () if(not $Obj_Init{"IsCorrect"});
        if($CompleteSignature{$Obj_Init{"Interface"}}{"Protected"})
        {#Impossible to call in-charge (D1) destructor in class with protected constructor
            return ();
        }
        $Interface_Init{"Init"} .= $Obj_Init{"Init"};
        #$Interface_Init{"Init"} .= "//parameter initialization\n";
        if($Obj_Init{"PreCondition"}) {
            $Interface_Init{"Init"} .= $Obj_Init{"PreCondition"};
        }
        $Interface_Init{"Init"} .= correct_init_stmt("$ClassName $Var = ".$Obj_Init{"Call"}.";\n", $ClassName, $Var);
        if($Obj_Init{"PostCondition"}) {
            $Interface_Init{"Init"} .= $Obj_Init{"PostCondition"};
        }
        if($Obj_Init{"ReturnRequirement"})
        {
            $Obj_Init{"ReturnRequirement"}=~s/(\$0|\$obj)/$Var/gi;
            $Interface_Init{"Init"} .= $Obj_Init{"ReturnRequirement"}
        }
        if($Obj_Init{"FinalCode"})
        {
            $Interface_Init{"Init"} .= "//final code\n";
            $Interface_Init{"Init"} .= $Obj_Init{"FinalCode"}."\n";
        }
        $Interface_Init{"Headers"} = addHeaders($Obj_Init{"Headers"}, $Interface_Init{"Headers"});
        $Interface_Init{"Code"} .= $Obj_Init{"Code"};
        $Interface_Init{"Call"} = "";#auto call after construction
    }
    $Interface_Init{"Headers"} = addHeaders([$CompleteSignature{$Interface}{"Header"}], $Interface_Init{"Headers"});
    $Interface_Init{"IsCorrect"} = 1;
    my $Typedef_Id = get_type_typedef($ClassId);
    if($Typedef_Id)
    {
        $Interface_Init{"Headers"} = addHeaders(getTypeHeaders($Typedef_Id), $Interface_Init{"Headers"});
        foreach my $Elem ("Call", "Init") {
            $Interface_Init{$Elem} = cover_by_typedef($Interface_Init{$Elem}, $ClassId, $Typedef_Id);
        }
    }
    else {
        $Interface_Init{"Headers"} = addHeaders(getTypeHeaders($ClassId), $Interface_Init{"Headers"});
    }
    return %Interface_Init;
}

sub testForConstructor($)
{
    my $Interface = $_[0];
    my $Ispecobjecttype = $InterfaceSpecType{$Interface}{"SpecObject"};
    my $PointerLevelTarget = get_PointerLevel($Tid_TDid{$SpecType{$Ispecobjecttype}{"TypeId"}}, $SpecType{$Ispecobjecttype}{"TypeId"});
    my $ClassId = $CompleteSignature{$Interface}{"Class"};
    my $ClassName = get_TypeName($ClassId);
    my $Var = select_obj_name("", $ClassId);
    $Block_Variable{$CurrentBlock}{$Var} = 1;
    if(isInCharge($Interface))
    {
        if(isAbstractClass($ClassId))
        {#Impossible to call in-charge constructor in abstract class
            return ();
        }
        if($CompleteSignature{$Interface}{"Protected"})
        {#Impossible to call in-charge protected constructor
            return ();
        }
    }
    my $HeapStack = ($SpecType{$Ispecobjecttype}{"TypeId"} and ($PointerLevelTarget eq 0))?"Stack":"Heap";
    my $ObjectCall = ($HeapStack eq "Stack")?$Var:"(*$Var)";
    my %Interface_Init = callInterfaceParameters((
            "Interface"=>$Interface,
            "Key"=>"",
            "ObjectCall"=>$ObjectCall));
    return () if(not $Interface_Init{"IsCorrect"});
    my $PreviousBlock = $CurrentBlock;
    $CurrentBlock = $CurrentBlock."_code_".$Ispecobjecttype;
    my %ParsedCode = parseCode($SpecType{$Ispecobjecttype}{"Code"}, "Code");
    $CurrentBlock = $PreviousBlock;
    return () if(not $ParsedCode{"IsCorrect"});
    $SpecCode{$Ispecobjecttype} = 1 if($ParsedCode{"Code"});
    $Interface_Init{"Code"} .= $ParsedCode{"NewGlobalCode"}.$ParsedCode{"Code"};
    $Interface_Init{"Headers"} = addHeaders($ParsedCode{"Headers"}, $Interface_Init{"Headers"});
    if(isAbstractClass($ClassId) or isNotInCharge($Interface) or ($CompleteSignature{$Interface}{"Protected"}))
    {
        my $ClassNameChild = getSubClassName($ClassName);
        if($Interface_Init{"Call"}=~/\A($ClassName([\n]*)\()/)
        {
            substr($Interface_Init{"Call"}, index($Interface_Init{"Call"}, $1), pos($1) + length($1)) = $ClassNameChild.$2."(";
        }
        $ClassName = $ClassNameChild;
        $UsedConstructors{$ClassId}{$Interface} = 1;
        $IntSubClass{$TestedInterface}{$ClassId} = 1;
        $Create_SubClass{$ClassId} = 1;
    }
    if($HeapStack eq "Stack") {
        $Interface_Init{"Call"} = correct_init_stmt($ClassName." $Var = ".$Interface_Init{"Call"}, $ClassName, $Var);
    }
    elsif($HeapStack eq "Heap") {
        $Interface_Init{"Call"} = $ClassName."* $Var = new ".$Interface_Init{"Call"};
    }
    if(my $Typedef_Id = get_type_typedef($ClassId))
    {
        $Interface_Init{"Headers"} = addHeaders(getTypeHeaders($Typedef_Id), $Interface_Init{"Headers"});
        foreach my $Elem ("Call", "Init") {
            $Interface_Init{$Elem} = cover_by_typedef($Interface_Init{$Elem}, $ClassId, $Typedef_Id);
        }
    }
    else {
        $Interface_Init{"Headers"} = addHeaders(getTypeHeaders($ClassId), $Interface_Init{"Headers"});
    }
    if($Ispecobjecttype and my $PostCondition = $SpecType{$Ispecobjecttype}{"PostCondition"}
    and $ObjectCall ne "" and (not defined $Template2Code or $Interface eq $TestedInterface))
    {# postcondition
        $PostCondition=~s/(\$0|\$obj)/$ObjectCall/gi;
        $PostCondition = clearSyntax($PostCondition);
        my $NormalResult = $PostCondition;
        while($PostCondition=~s/([^\\])"/$1\\\"/g){}
        $ConstraintNum{$Interface}+=1;
        my $ReqId = short_interface_name($Interface).".".normalize_num($ConstraintNum{$Interface});
        $RequirementsCatalog{$Interface}{$ConstraintNum{$Interface}} = "postcondition for the object: \'$PostCondition\'";
        my $Comment = "postcondition for the object failed: \'$PostCondition\'";
        $Interface_Init{"ReturnRequirement"} .= "REQ(\"$ReqId\",\n\"$Comment\",\n$NormalResult);\n";
        $TraceFunc{"REQ"}=1;
    }
    # init code
    my $InitCode = $SpecType{$Ispecobjecttype}{"InitCode"};
    $Interface_Init{"Init"} .= clearSyntax($InitCode);
    # final code
    my $ObjFinalCode = $SpecType{$Ispecobjecttype}{"FinalCode"};
    $ObjFinalCode=~s/(\$0|\$obj)/$ObjectCall/gi;
    $Interface_Init{"FinalCode"} .= clearSyntax($ObjFinalCode);
    return %Interface_Init;
}

sub add_VirtualTestData($$)
{
    my ($Code, $Path) = @_;
    my $RelPath = test_data_relpath("sample.txt");
    if($Code=~s/TG_TEST_DATA_(PLAIN|TEXT)_FILE/$RelPath/g)
    {#plain text files
        mkpath($Path);
        writeFile($Path."/sample.txt", "Where there's a will there's a way.");
    }
    $RelPath = test_data_abspath("sample", $Path);
    if($Code=~s/TG_TEST_DATA_ABS_FILE/$RelPath/g)
    {
        mkpath($Path);
        writeFile($Path."/sample", "Where there's a will there's a way.");
    }
    $RelPath = test_data_relpath("sample.xml");
    if($Code=~s/TG_TEST_DATA_XML_FILE/$RelPath/g)
    {
        mkpath($Path);
        writeFile($Path."/sample.xml", getXMLSample());
    }
    $RelPath = test_data_relpath("sample.html");
    if($Code=~s/TG_TEST_DATA_HTML_FILE/$RelPath/g)
    {
        mkpath($Path);
        writeFile($Path."/sample.html", getHTMLSample());
    }
    $RelPath = test_data_relpath("sample.dtd");
    if($Code=~s/TG_TEST_DATA_DTD_FILE/$RelPath/g)
    {
        mkpath($Path);
        writeFile($Path."/sample.dtd", getDTDSample());
    }
    $RelPath = test_data_relpath("sample.db");
    if($Code=~s/TG_TEST_DATA_DB/$RelPath/g)
    {
        mkpath($Path);
        writeFile($Path."/sample.db", "");
    }
    $RelPath = test_data_relpath("sample.audio");
    if($Code=~s/TG_TEST_DATA_AUDIO/$RelPath/g)
    {
        mkpath($Path);
        writeFile($Path."/sample.audio", "");
    }
    $RelPath = test_data_relpath("sample.asoundrc");
    if($Code=~s/TG_TEST_DATA_ASOUNDRC_FILE/$RelPath/g)
    {
        mkpath($Path);
        writeFile($Path."/sample.asoundrc", getASoundRCSample());
    }
    $RelPath = test_data_relpath("");
    if($Code=~s/TG_TEST_DATA_DIRECTORY/$RelPath/g)
    {
        mkpath($Path);
        writeFile($Path."/sample.txt", "Where there's a will there's a way.");
    }
    while($Code=~/TG_TEST_DATA_FILE_([A-Z]+)/)
    {
        my ($Type, $Ext) = ($1, lc($1));
        $RelPath = test_data_relpath("sample.$Ext");
        $Code=~s/TG_TEST_DATA_FILE_$Type/$RelPath/g;
        mkpath($Path);
        writeFile($Path."/sample.$Ext", "");
    }
    return $Code;
}

sub test_data_relpath($)
{
    my $File = $_[0];
    if(defined $Template2Code)
    {
        return "T2C_GET_DATA_PATH(\"$File\")";
    }
    else
    {
        return "\"testdata/$File\"";
    }
}

sub test_data_abspath($$)
{
    my ($File, $Path) = @_;
    if(defined $Template2Code)
    {
        return "T2C_GET_DATA_PATH(\"$File\")";
    }
    else
    {
        return "\"".abs_path("./")."/".$Path.$File."\"";
    }
}

sub getXMLSample()
{
    return "<?xml version=\"1.0\" encoding=\"UTF-8\"?>
<note>
    <to>Tove</to>
    <from>Jani</from>
    <heading>Reminder</heading>
    <body>Don't forget me this weekend!</body>
</note>";
}

sub getHTMLSample()
{
    return "<html>
<body>
Where there's a will there's a way.
</body>
</html>";
}

sub getDTDSample()
{
    return "<!ELEMENT note (to,from,heading,body)>
<!ELEMENT to (#PCDATA)>
<!ELEMENT from (#PCDATA)>
<!ELEMENT heading (#PCDATA)>
<!ELEMENT body (#PCDATA)>";
}

sub getASoundRCSample()
{
    if(my $Sample = readFile("/usr/share/alsa/alsa.conf"))
    {
        return $Sample;
    }
    elsif(my $Sample = readFile("/etc/asound-pulse.conf"))
    {
        return $Sample;
    }
    elsif(my $Sample = readFile("/etc/asound.conf"))
    {
        return $Sample;
    }
    else
    {
        return "pcm.card0 {
    type hw
    card 0
}
ctl.card0 {
    type hw
    card 0
}";
    }
}

sub add_TestData($$)
{
    my ($Code, $Path) = @_;
    my %CopiedFiles = ();
    if($Code=~/TEST_DATA_PATH/)
    {
        if(not $TestDataPath)
        {
            print STDERR "WARNING: test data directory is not specified\n";
            return $Code;
        }
    }
    while($Code=~s/TEST_DATA_PATH[ ]*\([ ]*"([^\(\)]+)"[ ]*\)/test_data_relpath($1)/ge)
    {
        my $FileName = $1;
        next if($CopiedFiles{$FileName});
        mkpath($Path);
        next if(not -e $TestDataPath."/".$FileName);
        copy($TestDataPath."/".$FileName, $Path);
        $CopiedFiles{$FileName} = 1;
    }
    return $Code;
}

sub constraint_for_environment($$$)
{
    my ($Interface, $ConditionType, $Condition) = @_;
    $ConstraintNum{$Interface}+=1;
    my $ReqId = short_interface_name($Interface).".".normalize_num($ConstraintNum{$Interface});
    $RequirementsCatalog{$Interface}{$ConstraintNum{$Interface}} = "$ConditionType for the environment: \'$Condition\'";
    my $Comment = "$ConditionType for the environment failed: \'$Condition\'";
    $TraceFunc{"REQ"}=1;
    return "REQ(\"$ReqId\",\n\"$Comment\",\n$Condition);\n";
}

sub get_env_conditions($$)
{
    my ($Interface, $SpecEnv_Id) = @_;
    my %Conditions = ();
    if(my $InitCode = $SpecType{$SpecEnv_Id}{"InitCode"}) {
        $Conditions{"Preamble"} .= $InitCode."\n";
    }
    if(my $FinalCode = $SpecType{$SpecEnv_Id}{"FinalCode"}) {
        $Conditions{"Finalization"} .= $FinalCode."\n";
    }
    if(my $GlobalCode = $SpecType{$SpecEnv_Id}{"GlobalCode"}) {
        $Conditions{"Env_CommonCode"} .= $GlobalCode."\n";
        $SpecCode{$SpecEnv_Id} = 1;
    }
    if(my $PreCondition = $SpecType{$SpecEnv_Id}{"PreCondition"}) {
        $Conditions{"Env_PreRequirements"} .= constraint_for_environment($Interface, "precondition", $PreCondition);
    }
    if(my $PostCondition = $SpecType{$SpecEnv_Id}{"PostCondition"}) {
        $Conditions{"Env_PostRequirements"} .= constraint_for_environment($Interface, "postcondition", $PostCondition);
    }
    foreach my $Lib (keys(%{$SpecType{$SpecEnv_Id}{"Libs"}})) {
        $SpecLibs{$Lib} = 1;
    }
    return %Conditions;
}

sub generate_sanity_test($)
{
    my %ResultComponents = ();
    my $Interface = $_[0];
    return () if(not $Interface);
    my $CommonCode = "";
    my %TestComponents = ();
    return () if(not interfaceFilter($Interface));
    $TestedInterface = $Interface;
    $CurrentBlock = "main";
    $ValueCollection{$CurrentBlock}{"argc"} = get_TypeIdByName("int");
    $Block_Param{$CurrentBlock}{"argc"} = get_TypeIdByName("int");
    $Block_Variable{$CurrentBlock}{"argc"} = 1;
    $ValueCollection{$CurrentBlock}{"argv"} = get_TypeIdByName("char**");
    $Block_Param{$CurrentBlock}{"argv"} = get_TypeIdByName("char**");
    $Block_Variable{$CurrentBlock}{"argv"} = 1;
    
    my ($CommonPreamble, $Preamble, $Finalization, $Env_CommonCode, $Env_PreRequirements, $Env_PostRequirements) = ();
    foreach my $SpecEnv_Id (sort {int($a)<=>int($b)} (keys(%Common_SpecEnv)))
    {# common environments
        next if($Common_SpecType_Exceptions{$Interface}{$SpecEnv_Id});
        my %Conditions = get_env_conditions($Interface, $SpecEnv_Id);
        $CommonPreamble .= $Conditions{"Preamble"};# in the direct order
        $Finalization = $Conditions{"Finalization"}.$Finalization;# in the backward order
        $Env_CommonCode .= $Conditions{"Env_CommonCode"};
        $Env_PreRequirements .= $Conditions{"Env_PreRequirements"};# in the direct order
        $Env_PostRequirements = $Conditions{"Env_PostRequirements"}.$Env_PostRequirements;# in the backward order
    }
    
    # parsing of common preamble code for using
    # created variables in the following test case
    my %CommonPreamble_Parsed = parseCode($CommonPreamble, "Code");
    $CommonPreamble = $CommonPreamble_Parsed{"Code"};
    $CommonCode = $CommonPreamble_Parsed{"NewGlobalCode"}.$CommonCode;
    $TestComponents{"Headers"} = addHeaders($CommonPreamble_Parsed{"Headers"}, $TestComponents{"Headers"});
    
    # creating test case
    if($CompleteSignature{$Interface}{"Constructor"})
    {
        %TestComponents = testForConstructor($Interface);
        $CommonCode .= $TestComponents{"Code"};
    }
    elsif($CompleteSignature{$Interface}{"Destructor"})
    {
        %TestComponents = testForDestructor($Interface);
        $CommonCode .= $TestComponents{"Code"};
    }
    else
    {
        %TestComponents = callInterface((
            "Interface"=>$Interface));
        $CommonCode .= $TestComponents{"Code"};
    }
    if(not $TestComponents{"IsCorrect"})
    {
        $ResultCounter{"Gen"}{"Fail"} += 1;
        $GenResult{$Interface}{"IsCorrect"} = 0;
        return ();
    }
    if($TraceFunc{"REQ"} and not defined $Template2Code) {
        $CommonCode = get_REQ_define($Interface)."\n".$CommonCode;
    }
    if($TraceFunc{"REQva"} and not defined $Template2Code) {
        $CommonCode = get_REQva_define($Interface)."\n".$CommonCode;
    }
    
    foreach my $SpecEnv_Id (sort {int($a)<=>int($b)} (keys(%SpecEnv)))
    {# environments used in the test case
        my %Conditions = get_env_conditions($Interface, $SpecEnv_Id);
        $Preamble .= $Conditions{"Preamble"};# in the direct order
        $Finalization = $Conditions{"Finalization"}.$Finalization;# in the backward order
        $Env_CommonCode .= $Conditions{"Env_CommonCode"};
        $Env_PreRequirements .= $Conditions{"Env_PreRequirements"};# in the direct order
        $Env_PostRequirements = $Conditions{"Env_PostRequirements"}.$Env_PostRequirements;# in the backward order
    }
    
    my %Preamble_Parsed = parseCode($Preamble, "Code");
    $Preamble = $Preamble_Parsed{"Code"};
    $CommonCode = $Preamble_Parsed{"NewGlobalCode"}.$CommonCode;
    $TestComponents{"Headers"} = addHeaders($Preamble_Parsed{"Headers"}, $TestComponents{"Headers"});
    
    my %Finalization_Parsed = parseCode($Finalization, "Code");
    $Finalization = $Finalization_Parsed{"Code"};
    $CommonCode = $Finalization_Parsed{"NewGlobalCode"}.$CommonCode;
    $TestComponents{"Headers"} = addHeaders($Finalization_Parsed{"Headers"}, $TestComponents{"Headers"});
    
    my %Env_ParsedCode = parseCode($Env_CommonCode, "Code");
    $CommonCode = $Env_ParsedCode{"NewGlobalCode"}.$Env_ParsedCode{"Code"}.$CommonCode;
    $TestComponents{"Headers"} = addHeaders($Env_ParsedCode{"Headers"}, $TestComponents{"Headers"});
    foreach my $Header (@{$Env_ParsedCode{"Headers"}}) {
        $SpecTypeHeaders{get_filename($Header)}=1;
    }
    # insert subclasses
    my ($SubClasses_Code, $SubClasses_Headers) = create_SubClasses(keys(%Create_SubClass));
    $TestComponents{"Headers"} = addHeaders($SubClasses_Headers, $TestComponents{"Headers"});
    $CommonCode = $SubClasses_Code.$CommonCode;
    # closing streams
    foreach my $Stream (keys(%{$OpenStreams{"main"}})) {
        $Finalization .= "fclose($Stream);\n";
    }
    # test assembling
    my ($SanityTest, $SanityTestMain, $SanityTestBody) = ();
    if($CommonPreamble.$Preamble)
    {
        $SanityTestMain .= "//preamble\n";
        $SanityTestMain .= $CommonPreamble.$Preamble."\n";
    }
    if($Env_PreRequirements) {
        $SanityTestMain .= $Env_PreRequirements."\n";
    }
    if($TestComponents{"Init"}) {
        $SanityTestBody .= $TestComponents{"Init"};
    }
    # precondition for parameters
    if($TestComponents{"PreCondition"}) {
        $SanityTestBody .= $TestComponents{"PreCondition"};
    }
    if($TestComponents{"Call"})
    {
        if($TestComponents{"ReturnRequirement"} and $CompleteSignature{$Interface}{"Return"})
        {#Call interface and check return value
            my $ReturnType_Id = $CompleteSignature{$Interface}{"Return"};
            my $ReturnType_Name = $TypeDescr{$Tid_TDid{$ReturnType_Id}}{$ReturnType_Id}{"Name"};
            my $ReturnType_PointerLevel = get_PointerLevel($Tid_TDid{$ReturnType_Id}, $ReturnType_Id);
            my $ReturnFType_Id = get_FoundationTypeId($ReturnType_Id);
            my $ReturnFType_Name = get_TypeName($ReturnFType_Id);
            if($ReturnFType_Name eq "void" and $ReturnType_PointerLevel==1)
            {
                my $RetVal = select_var_name("retval", "");
                $TestComponents{"ReturnRequirement"}=~s/(\$0|\$retval)/$RetVal/gi;
                $SanityTestBody .= "int* $RetVal = (int*)".$TestComponents{"Call"}."; //target call\n";
                $Block_Variable{$CurrentBlock}{$RetVal} = 1;
            }
            elsif($ReturnFType_Name eq "void" and $ReturnType_PointerLevel==0) {
                $SanityTestBody .= $TestComponents{"Call"}."; //target call\n";
            }
            else
            {
                my $RetVal = select_var_name("retval", "");
                $TestComponents{"ReturnRequirement"}=~s/(\$0|\$retval)/$RetVal/gi;
                my ($InitializedEType_Id, $Declarations, $Headers) = get_ExtTypeId($RetVal, $ReturnType_Id);
                my $InitializedType_Name = get_TypeName($InitializedEType_Id);
                $TestComponents{"Code"} .= $Declarations;
                $TestComponents{"Headers"} = addHeaders($Headers, $TestComponents{"Headers"});
                my $Break = ((length($InitializedType_Name)>20)?"\n":" ");
                my $InitializedFType_Id = get_FoundationTypeId($ReturnType_Id);
                if(($InitializedType_Name eq $ReturnType_Name)) {
                    $SanityTestBody .= $ReturnType_Name.$Break.$RetVal." = ".$TestComponents{"Call"}."; //target call\n";
                }
                else {
                    $SanityTestBody .= $InitializedType_Name.$Break.$RetVal." = "."(".$InitializedType_Name.")".$TestComponents{"Call"}."; //target call\n";
                }
                $Block_Variable{$CurrentBlock}{$RetVal} = 1;
                $TestComponents{"Headers"} = addHeaders(getTypeHeaders($InitializedFType_Id), $TestComponents{"Headers"});
            }
        }
        else {
            $SanityTestBody .= $TestComponents{"Call"}."; //target call\n";
        }
    }
    elsif($CompleteSignature{$Interface}{"Destructor"}) {
        $SanityTestBody .= "//target interface will be called at the end of main() function automatically\n";
    }
    if($TestComponents{"ReturnRequirement"}) {
        $SanityTestBody .= $TestComponents{"ReturnRequirement"}."\n";
    }
    # postcondition for parameters
    if($TestComponents{"PostCondition"}) {
        $SanityTestBody .= $TestComponents{"PostCondition"}."\n";
    }
    if($TestComponents{"FinalCode"})
    {
        $SanityTestBody .= "//final code\n";
        $SanityTestBody .= $TestComponents{"FinalCode"}."\n";
    }
    $SanityTestMain .= $SanityTestBody;
    if($Finalization)
    {
        $SanityTestMain .= "\n//finalization\n";
        $SanityTestMain .= $Finalization."\n";
    }
    if($Env_PostRequirements) {
        $SanityTestMain .= $Env_PostRequirements."\n";
    }
    if(my $AddDefines = $Descriptor{"Defines"})
    {
        $AddDefines=~s/\n\s+/\n/g;
        $SanityTest .= $AddDefines."\n";
    }
    # clear code syntax
    $SanityTestMain = alignCode($SanityTestMain, "    ", 0);
    if(keys(%ConstraintNum)>0)
    {
        if(getTestLang($Interface) eq "C++") {
            $TestComponents{"Headers"} = addHeaders(["iostream"], $TestComponents{"Headers"});
            $AuxHeaders{"iostream"}=1;
        }
        else {
            $TestComponents{"Headers"} = addHeaders(["stdio.h"], $TestComponents{"Headers"});
            $AuxHeaders{"stdio.h"}=1;
        }
    }
    foreach my $Header_Path (sort {int($Include_Preamble{$b}{"Position"})<=>int($Include_Preamble{$a}{"Position"})} keys(%Include_Preamble))
    {
        @{$TestComponents{"Headers"}} = ($Header_Path, @{$TestComponents{"Headers"}});
    }
    my %HeadersList = unique_headers_list(add_prefixes(create_headers_list(order_headers_list(@{$TestComponents{"Headers"}}))));
    $ResultComponents{"Headers"} = [];
    my $HList = "";
    foreach my $Pos (sort {int($a) <=> int($b)} keys(%HeadersList))
    {
        $HList .= "#include <".$HeadersList{$Pos}{"Inc"}.">\n";
        @{$ResultComponents{"Headers"}} = (@{$ResultComponents{"Headers"}}, $HeadersList{$Pos}{"Inc"});
        if($HeadersList{$Pos}{"Inc"}=~/\+\+(\.h|)\Z/) {
            $UsedInterfaces{"__gxx_personality"} = 1;
        }
    }
    if(getTestLang($Interface) eq "C"
    and $OSgroup eq "windows") {
        $SanityTest .= "extern \"C\" {\n$HList}\n";
    }
    else {
        $SanityTest .= $HList;
    }
    my %UsedNameSpaces = ();
    foreach my $NameSpace (add_namespaces(\$CommonCode), add_namespaces(\$SanityTestMain)) {
        $UsedNameSpaces{$NameSpace} = 1;
    }
    if(keys(%UsedNameSpaces))
    {
        $SanityTest .= "\n";
        foreach my $NameSpace (sort {get_depth_symbol($a,"::")<=>get_depth_symbol($b,"::")} keys(%UsedNameSpaces)) {
            $SanityTest .= "using namespace $NameSpace;\n";
        }
        $SanityTest .= "\n";
    }
    if($CommonCode)
    {
        $SanityTest .= "\n$CommonCode\n\n";
        $ResultComponents{"Code"} = $CommonCode;
    }
    $SanityTest .= "int main(int argc, char *argv[])\n";
    $SanityTest .= "{\n";
    $ResultComponents{"main"} = correct_spaces($SanityTestMain);
    $SanityTestMain .= "    return 0;\n";
    $SanityTest .= $SanityTestMain;
    $SanityTest .= "}\n";
    $SanityTest = correct_spaces($SanityTest); #cleaning code
    if(getTestLang($Interface) eq "C++" and getIntLang($Interface) eq "C")
    {# removing extended initializer lists
        $SanityTest=~s/({\s*|\s)\.[a-z_][a-z_\d]*\s*=\s*/$1  /ig;
    }
    if(defined $Standalone)
    {# creating stuff for building and running test
        my $TestFileName = (getTestLang($Interface) eq "C++" or $OSgroup eq "windows")?"test.cpp":"test.c";
        my $TestPath = getTestPath($Interface);
        if(-e $TestPath) {
            rmtree($TestPath);
        }
        mkpath($TestPath);
        $Interface_TestDir{$Interface} = $TestPath;
        $SanityTest = add_VirtualTestData($SanityTest, $TestPath."/testdata/");
        $SanityTest = add_TestData($SanityTest, $TestPath."/testdata/");
        writeFile("$TestPath/$TestFileName", $SanityTest);
        my $SharedObject = $Interface_Library{$Interface};
        $SharedObject = $NeededInterface_Library{$Interface} if(not $SharedObject);
        writeFile("$TestPath/info", "Library: $TargetLibraryName-".$Descriptor{"Version"}."\nInterface: ".get_Signature($Interface)."\nSymbol: $Interface".(($Interface=~/\A(_Z|\?)/)?"\nShort Name: ".$CompleteSignature{$Interface}{"ShortName"}:"")."\nHeader: ".$CompleteSignature{$Interface}{"Header"}.(($SharedObject ne "WithoutLib")?"\nShared Object: ".get_filename($SharedObject):"").(get_IntNameSpace($Interface)?"\nNamespace: ".get_IntNameSpace($Interface):""));
        my $Signature = get_Signature($Interface);
        my $NameSpace = get_IntNameSpace($Interface);
        if($NameSpace) {
            $Signature=~s/(\W|\A)\Q$NameSpace\E\:\:(\w)/$1$2/g;
        }
        my $TitleSignature = $Signature;
        $TitleSignature=~s/([^:]):[^:].+?\Z/$1/;
        my $Title = "Test for ".htmlSpecChars($TitleSignature);
        my $Keywords = $CompleteSignature{$Interface}{"ShortName"}.", unit test";
        my $FunctionMethod = $CompleteSignature{$Interface}{"Class"}?"method":"function";
        my $Description = "Sanity test for $FunctionMethod ".htmlSpecChars($TitleSignature);
        my $View = composeHTML_Head($Title, $Keywords, $Description, "
        <style type=\"text/css\">
        body {
            font-family:Arial;
            font-size:14px;
            text-align:left;
            padding-left:5px;
        }
        h1 {
            font-family:Verdana;
            font-size:18px;
            margin-bottom:3px;
            margin-top:3px;
        }
        span.int {
            font-weight:bold;
            font-size:16px;
            font-family:Arial;
            color:#003E69;
        }
        span.int_p {
            font-weight:normal;
        }
        hr {
            color:Black;
            background-color:Black;
            height:1px;
            border:0;
        }
        div.tinfo {
            padding-left:10px;
        }
        ".get_TestView_Style()."
            table.test_view {
            cursor:text;
            margin-top:7px;
            width:75%;
            font-family:Monaco, \"Courier New\", Courier;
            font-size:14px;
            padding:10px;
            border:1px solid #e0e8e5;
            color:#444444;
            background-color:#eff3f2;
            overflow:auto;
        }
        </style>");
        $View .= "<body>".show_navigation($TestPath)."<h1><span style='color:Black'>Unit Test</span></h1>\n";
        $View .= "<div class='tinfo'>\n";
        if($NameSpace) {
            $View .= "Namespace&nbsp;<span class='int'>$NameSpace</span><br/>\n";
        }
        $View .= ucfirst($FunctionMethod)."&nbsp;";
        $View .= "<span class='int'>".highLight_Signature_Italic_Color($Signature)."</span>\n";
        if($Interface=~/\A(_Z|\?)/) {
            $View .= "<br/>Symbol&nbsp;<span class='int'>$Interface</span>\n";
        }
        $View .= "<!--Test-->\n".get_TestView($SanityTest, $Interface)."<!--Test_End-->\n";
        $View .= "</div>";
        $View .= $TOOL_SIGNATURE."\n</body>\n</html>\n";
        writeFile("$TestPath/view.html", $View);
        writeFile("$TestPath/Makefile", get_Makefile($Interface, \%HeadersList));
        my $RunScript = ($OSgroup eq "windows")?"run_test.bat":"run_test.sh";
        writeFile("$TestPath/$RunScript", get_RunScript($Interface));
        chmod(0755, $TestPath."/$RunScript");
    }
    else
    { # t2c
    }
    $GenResult{$Interface}{"IsCorrect"} = 1;
    $ResultCounter{"Gen"}{"Success"} += 1;
    $ResultComponents{"IsCorrect"} = 1;
    return %ResultComponents;
}

sub getTestLang($)
{
    my $Interface = $_[0];
    my $Lang = getIntLang($Interface);
    foreach my $Interface (keys(%UsedInterfaces))
    {
        if($Interface=~/\A_Z(L|)\d/) {
            # mangled C functions and global data
            next;
        }
        if($CompleteSignature{$Interface}{"Constructor"}
        or $CompleteSignature{$Interface}{"Destructor"}
        or $Interface=~/\A(_Z|__gxx_)/
        or $CompleteSignature{$Interface}{"Header"}=~/\.(hh|hpp)\Z/i) {
            $Lang = "C++";
        }
    }
    return $Lang;
}

sub show_navigation($)
{
    my $TestPath = $_[0];
    my $BackPath = "";
    foreach (0 .. get_depth($TestPath)-3) {
        $BackPath .= "../";
    }
    my $TestSuite = $BackPath."view_tests.html";
    my $TestResults = "";
    if($TestPath=~/(.+?)\/(.+?)\/(.+?)\//) {
        $TestResults = "../../../".$BackPath."test_results/$2/$3/test_results.html";
    }
    return "<a href=\'$TestResults\'>Results</a> | <a href=\'$TestSuite\'>Back</a>";
}

sub add_namespaces($)
{
    my $CodeRef = $_[0];
    my @UsedNameSpaces = ();
    foreach my $NameSpace (sort {get_depth_symbol($b,"::")<=>get_depth_symbol($a,"::")} keys(%NestedNameSpaces))
    {
        next if($NameSpace eq "std");
        my $NameSpace_InCode = $NameSpace."::";
        if(${$CodeRef}=~s/(\W|\A)(\Q$NameSpace_InCode\E)(\w)/$1$3/g) {
            push(@UsedNameSpaces, $NameSpace);
        }
        my $NameSpace_InSubClass = getSubClassBaseName($NameSpace_InCode);
        if(${$CodeRef}=~s/(\W|\A)($NameSpace_InSubClass)(\w+_SubClass)/$1$3/g) {
            push(@UsedNameSpaces, $NameSpace);
        }
    }
    return @UsedNameSpaces;
}

sub correct_spaces($)
{
    my $Code = $_[0];
    $Code=~s/\n\n\n\n/\n\n/g;
    $Code=~s/\n\n\n/\n\n/g;
    $Code=~s/\n    \n    /\n\n    /g;
    $Code=~s/\n    \n\n/\n/g;
    $Code=~s/\n\n\};/\n};/g;
    return $Code;
}

sub add_prefixes(@)
{
    my %HeadersToInclude = remove_prefixes(@_);
    foreach my $Pos (sort {int($b) <=> int($a)} keys(%HeadersToInclude))
    {
        if(not get_dirname($HeadersToInclude{$Pos}{"Inc"}))
        {
            my $Prefix = get_filename(get_dirname($HeadersToInclude{$Pos}{"Path"}));
            if($Prefix!~/include/i) {
                $HeadersToInclude{$Pos}{"Inc"} = joinPath($Prefix,$HeadersToInclude{$Pos}{"Inc"});
            }
        }
    }
    return %HeadersToInclude;
}

sub remove_prefixes(@)
{
    my %HeadersToInclude = @_;
    my %IncDir = get_HeaderDeps_forList(%HeadersToInclude);
    foreach my $Pos (sort {int($a) <=> int($b)} keys(%HeadersToInclude))
    {
        if($HeadersToInclude{$Pos}{"Inc"}=~/\A(\/|\w+:[\/\\])/)
        {
            foreach my $Prefix (sort {length($b)<=>length($a)} keys(%IncDir)) {
                last if($HeadersToInclude{$Pos}{"Inc"}=~s/\A\Q$Prefix\E[\/\\]//);
            }
        }
    }
    return %HeadersToInclude;
}

sub get_HeaderDeps_forList(@)
{
    my %HeadersToInclude = @_;
    my %IncDir = ();
    foreach my $Pos (sort {int($a) <=> int($b)} keys(%HeadersToInclude))
    {
        foreach my $Dir (get_HeaderDeps($HeadersToInclude{$Pos}{"Path"})) {
            $IncDir{$Dir}=1;
        }
        if(my $DepDir = get_dirname($HeadersToInclude{$Pos}{"Path"}))
        {
            if(my $Prefix = get_dirname($HeadersToInclude{$Pos}{"Inc"})) {
                $DepDir=~s/[\/\\]+\Q$Prefix\E\Z//;
            }
            if(not is_default_include_dir($DepDir)
            and $DepDir ne "/usr/local/include") {
                $IncDir{$DepDir} = 1;
            }
        }
    }
    return %IncDir;
}

sub unique_headers_list(@)
{
    my %HeadersToInclude = @_;
    my (%FullPaths, %NewHeadersToInclude) = ();
    my $NewPos = 0;
    foreach my $Pos (sort {int($a) <=> int($b)} keys(%HeadersToInclude))
    {
        if(not $FullPaths{$HeadersToInclude{$Pos}{"Path"}})
        {
            $NewHeadersToInclude{$NewPos}{"Path"} = $HeadersToInclude{$Pos}{"Path"};
            $NewHeadersToInclude{$NewPos}{"Inc"} = $HeadersToInclude{$Pos}{"Inc"};
            $FullPaths{$HeadersToInclude{$Pos}{"Path"}} = 1;
            $NewPos+=1;
        }
    }
    return %NewHeadersToInclude;
}

sub order_headers_list(@)
{# ordering headers according to descriptor
    my @HeadersToInclude = @_;
    return @HeadersToInclude if(not keys(%Include_RevOrder));
    my @NewHeadersToInclude = ();
    my (%ElemNum, %Replace) = ();
    my $Num = 1;
    foreach my $Elem (@HeadersToInclude)
    {
        $ElemNum{$Elem} = $Num;
        $Num+=1;
    }
    foreach my $Elem (@HeadersToInclude)
    {
        if(my $Preamble = $Include_RevOrder{$Elem})
        {
            if(not $ElemNum{$Preamble})
            {
                push(@NewHeadersToInclude, $Preamble);
                push(@NewHeadersToInclude, $Elem);
            }
            elsif($ElemNum{$Preamble}>$ElemNum{$Elem})
            {
                push(@NewHeadersToInclude, $Preamble);
                $Replace{$Preamble} = $Elem;
            }
            else {
                push(@NewHeadersToInclude, $Elem);
            }
        }
        elsif($Replace{$Elem}) {
            push(@NewHeadersToInclude, $Replace{$Elem});
        }
        else {
            push(@NewHeadersToInclude, $Elem);
        }
    }
    return @NewHeadersToInclude;
}

sub create_headers_list(@)
{
    my @InputHeadersList = @_;
    my (%Already_Included_Header, %HeadersToInclude) = ();
    my $IPos=0;
    foreach my $Header_Name (@InputHeadersList)
    {
        next if($Already_Included_Header{$Header_Name});
        my ($Header_Inc, $Header_Path) = identify_header($Header_Name);
        next if(not $Header_Inc);
        detect_recursive_includes($Header_Path);
        @RecurHeader = ();
        if(my $RHeader_Path = $Header_ErrorRedirect{$Header_Path}{"Path"})
        {
            my $RHeader_Inc = $Header_ErrorRedirect{$Header_Path}{"Inc"};
            next if($Already_Included_Header{$RHeader_Inc});
            $Already_Included_Header{$RHeader_Inc} = 1;
            $HeadersToInclude{$IPos}{"Inc"} = $RHeader_Inc;
            $HeadersToInclude{$IPos}{"Path"} = $RHeader_Path;
        }
        elsif($Header_ShouldNotBeUsed{$Header_Path}) {
            next;
        }
        else
        {
            next if($Already_Included_Header{$Header_Inc});
            $Already_Included_Header{$Header_Inc} = 1;
            $HeadersToInclude{$IPos}{"Inc"} = $Header_Inc;
            $HeadersToInclude{$IPos}{"Path"} = $Header_Path;
        }
        $IPos+=1;
    }
    if(keys(%Target_Headers)==1)
    {
        my (%HeadersToInclude_New, %Included) = ();
        my $Pos = 0;
        foreach my $HeaderPath ((sort {int($Include_Preamble{$a}{"Position"})<=>int($Include_Preamble{$b}{"Position"})} keys(%Include_Preamble)),
        (sort {int($Target_Headers{$a}{"Position"})<=>int($Target_Headers{$b}{"Position"})} keys(%Target_Headers)))
        {
            my ($Header_Inc, $Header_Path) = identify_header($HeaderPath);
            next if($Included{$Header_Inc});
            $HeadersToInclude_New{$Pos}{"Inc"} = $Header_Inc;
            $HeadersToInclude_New{$Pos}{"Path"} = $Header_Path;
            $Included{$Header_Inc} = 1;
            $Pos+=1;
        }
        foreach my $IPos (sort {int($a)<=>int($b)} keys(%HeadersToInclude))
        {
            my ($Header_Inc, $Header_Path) = ($HeadersToInclude{$IPos}{"Inc"}, $HeadersToInclude{$IPos}{"Path"});
            if($AuxHeaders{get_filename($Header_Inc)} or $SpecTypeHeaders{get_filename($Header_Inc)})
            {
                next if($Included{$Header_Inc});
                $HeadersToInclude_New{$Pos}{"Inc"} = $Header_Inc;
                $HeadersToInclude_New{$Pos}{"Path"} = $Header_Path;
                $Included{$Header_Inc} = 1;
                $Pos+=1;
            }
        }
        %HeadersToInclude_New = optimize_includes(%HeadersToInclude_New);
        return %HeadersToInclude_New;
    }
    elsif($IsHeaderListSpecified)
    {
        my (%HeadersToInclude_New, %Included) = ();
        my $Pos = 0;
        foreach my $IPos (sort {int($a)<=>int($b)} keys(%HeadersToInclude))
        {
            my ($Header_Inc, $Header_Path) = ($HeadersToInclude{$IPos}{"Inc"}, $HeadersToInclude{$IPos}{"Path"});
            if(my ($TopHeader_Inc, $TopHeader_Path) = find_top_header($Header_Path))
            {
                next if($Included{$TopHeader_Inc});
                $HeadersToInclude_New{$Pos}{"Inc"} = $TopHeader_Inc;
                $HeadersToInclude_New{$Pos}{"Path"} = $TopHeader_Path;
                $Included{$TopHeader_Inc} = 1;
                $Included{$Header_Inc} = 1;
                $Pos+=1;
            }
            elsif($AuxHeaders{get_filename($Header_Inc)}
            or $SpecTypeHeaders{get_filename($Header_Inc)})
            {
                next if($Included{$Header_Inc});
                $HeadersToInclude_New{$Pos}{"Inc"} = $Header_Inc;
                $HeadersToInclude_New{$Pos}{"Path"} = $Header_Path;
                $Included{$Header_Inc} = 1;
                $Pos+=1;
            }
            else
            {
                next if($Included{$Header_Inc});
                $HeadersToInclude_New{$Pos}{"Inc"} = $Header_Inc;
                $HeadersToInclude_New{$Pos}{"Path"} = $Header_Path;
                $Included{$Header_Inc} = 1;
                $Pos+=1;
            }
        }
        return %HeadersToInclude_New;
    }
    else {
        %HeadersToInclude = optimize_includes(%HeadersToInclude);
        return %HeadersToInclude;
    }
}

sub detect_real_includes($)
{
    my $AbsPath = $_[0];
    return $Cache{"detect_real_includes"}{$AbsPath} if(defined $Cache{"detect_real_includes"}{$AbsPath});
    my $Path = callPreprocessor($AbsPath, "", getTestLang($TestedInterface), "#\ [0-9]*\ ");
    my %Includes = ();
    open(PREPROC, $Path);
    while(<PREPROC>)
    {
        if(/#\s+\d+\s+"([^"]+)"[\s\d]*\n/)
        {
            my $Include = $1;
            if($Include=~/\<(built\-in|internal|command\-line)\>|\A\./
            or $Include eq $AbsPath) {
                next;
            }
            if(my ($Header_Inc, $Header_Path) = identify_header($Include)) {
                $Includes{$Header_Path} = 1;
            }
        }
    }
    close(PREPROC);
    $Cache{"detect_real_includes"}{$AbsPath} = \%Includes;
    return \%Includes;
}

sub find_top_header($)
{
    my $AbsPath = $_[0];
    my @PreambleHeaders = keys(%Include_Preamble);
    @PreambleHeaders = sort {int($Include_Preamble{$a}{"Position"})<=>int($Include_Preamble{$b}{"Position"})} @PreambleHeaders;
    my @Headers = keys(%Target_Headers);
    @Headers = sort {int($Target_Headers{$a}{"Position"})<=>int($Target_Headers{$b}{"Position"})} @Headers;
    foreach my $Header_Path (@PreambleHeaders, @Headers)
    {
        my %Includes = %{detect_real_includes($Header_Path)};
        if($Includes{$AbsPath} or $Header_Path eq $AbsPath) {
            return identify_header($Header_Path);
        }
    }
    return ();
}

sub optimize_includes(@)
{
    my %HeadersToInclude = @_;
    %HeadersToInclude = optimize_recursive_includes(%HeadersToInclude);
    %HeadersToInclude = optimize_src_includes(%HeadersToInclude);
    %HeadersToInclude = optimize_recursive_includes(%HeadersToInclude);
    return %HeadersToInclude;
}

sub optimize_recursive_includes(@)
{
    my %HeadersToInclude = @_;
    my (%HeadersToInclude_New, %DeletedHeaders) = ();
    my $Pos=0;
    foreach my $Pos1 (sort {int($a)<=>int($b)} keys(%HeadersToInclude))
    {
        my $IsNeeded=1;
        my $Header_Inc = $HeadersToInclude{$Pos1}{"Inc"};
        my $Header_Path = $HeadersToInclude{$Pos1}{"Path"};
        if(not defined $Include_Preamble{$Header_Path}
        and not defined $Include_Order{$Header_Inc})
        {
            foreach my $Pos2 (sort {int($a)<=>int($b)} keys(%HeadersToInclude))
            {
                my $Candidate_Inc = $HeadersToInclude{$Pos2}{"Inc"};
                my $Candidate_Path = $HeadersToInclude{$Pos2}{"Path"};
                next if($DeletedHeaders{$Candidate_Inc});
                next if($Header_Inc eq $Candidate_Inc);
                my %Includes = %{detect_real_includes($Candidate_Path)};
                if($Includes{$Header_Path}) {
                    $IsNeeded=0;
                    $DeletedHeaders{$Header_Inc}=1;
                    last;
                }
            }
        }
        if($IsNeeded) {
            %{$HeadersToInclude_New{$Pos}} = %{$HeadersToInclude{$Pos1}};
            $Pos+=1;
        }
    }
    return %HeadersToInclude_New;
}

sub optimize_src_includes(@)
{
    my %HeadersToInclude = @_;
    my (%HeadersToInclude_New, %Included) = ();
    my $ResPos=0;
    foreach my $Pos (sort {int($a)<=>int($b)} keys(%HeadersToInclude))
    {
        my $Header_Inc = $HeadersToInclude{$Pos}{"Inc"};
        my $Header_Path = $HeadersToInclude{$Pos}{"Path"};
        my $DepDir = get_dirname($Header_Path);
        if(my $Prefix = get_dirname($Header_Inc)) {
            $DepDir=~s/[\/\\]+\Q$Prefix\E\Z//;
        }
        if(not $SystemPaths{"include"}{$DepDir} and not $DefaultGccPaths{$DepDir} and $Header_TopHeader{$Header_Path}
        and not defined $Include_Preamble{$Header_Path} and ((keys(%{$Header_Includes{$Header_Path}})<=$Header_MaxIncludes/5
        and keys(%{$Header_Includes{$Header_Path}})<keys(%{$Header_Includes{$Header_TopHeader{$Header_Path}}})) or $STDCXX_TESTING))
        {
            my %Includes = %{detect_real_includes($Header_TopHeader{$Header_Path})};
            if($Includes{$Header_Path})
            {# additional check
                my ($TopHeader_Inc, $TopHeader_Path) = identify_header($Header_TopHeader{$Header_Path});
                next if($Included{$TopHeader_Inc});
                $HeadersToInclude_New{$ResPos}{"Inc"} = $TopHeader_Inc;
                $HeadersToInclude_New{$ResPos}{"Path"} = $TopHeader_Path;
                $ResPos+=1;
                $Included{$TopHeader_Inc}=1;
                next;
            }
        }
        next if($Included{$Header_Inc});
        $HeadersToInclude_New{$ResPos}{"Inc"} = $Header_Inc;
        $HeadersToInclude_New{$ResPos}{"Path"} = $Header_Path;
        $ResPos+=1;
        $Included{$Header_Inc}=1;
    }
    return %HeadersToInclude_New;
}

sub cut_path_prefix($$)
{
    my ($Path, $Prefix) = @_;
    $Prefix=~s/[\/\\]+\Z//;
    $Path=~s/\A\Q$Prefix\E[\/\\]+//;
    return $Path;
}

sub identify_header($)
{
    my $Header = $_[0];
    return @{$Cache{"identify_header"}{$Header}} if(defined $Cache{"identify_header"}{$Header});
    my ($HInc, $HPath) = identify_header_m($Header);
    if(not $HPath and $OSgroup eq "macos" and $Header=~/\A([^\/]+)\/([^\/]+)/)
    {# search in frameworks: "OpenGL/gl.h" is "OpenGL.framework/Headers/gl.h"
        my ($HFramework, $HName) = ($1, $2);
        my $RelPath = "$HFramework\.framework\/Headers\/$HName";
        if(my $HeaderDir = find_in_dependencies($RelPath)) {
            ($HInc, $HPath) = ($Header, joinPath($HeaderDir, $RelPath));
        }
    }
    if($Header!~/\//){
    @{$Cache{"identify_header"}{$Header}} = ($HInc, $HPath);}
    return ($HInc, $HPath);
}

sub identify_header_m($)
{# search for header by absolute path, relative path or name
    my $Header = $_[0];
    $Header=~s/[\/\\]/$SLASH/g if($OSgroup eq "windows");
    if(not $Header
    or $Header=~/\.tcc\Z/) {
        return ("", "");
    }
    elsif(-f $Header)
    {
        if($Header!~/\A(\/|\w+:[\/\\])/) {
            $Header = abs_path($Header);
        }
        my $Prefix = get_filename(get_dirname($Header));
        if(my @Prefixes = sort {get_depth($b) <=> get_depth($a)}
        keys(%{$Include_Prefixes{$Header}})) {
            # use a standard prefix (<openbabel/math/align.h>)
            $Prefix = $Prefixes[0];
        }
        elsif(my @Prefixes = sort {get_depth($b) <=> get_depth($a)}
        keys(%{$IncDir_Prefixes{get_dirname($Header)}})) {
            # use a standard prefix (<openbabel/math/align.h>)
            $Prefix = $Prefixes[0];
        }
        my $HeaderDir = find_in_dependencies(joinPath($Prefix, get_filename($Header)));
        if($HeaderDir and $Prefix!~/include/i
        and $Header=~/\A\Q$HeaderDir\E([\/\\]|\Z)/) {
            $Header = cut_path_prefix($Header, $HeaderDir);
            return ($Header, joinPath($HeaderDir,$Header));
        }
        elsif(is_default_include_dir(get_dirname($Header))) {
            return (get_filename($Header), $Header);
        }
        else {
            return ($Header, $Header);
        }
    }
    elsif($GlibcHeader{$Header} and not $GLIBC_TESTING
    and my $HeaderDir = find_in_defaults($Header)) {
        return ($Header, joinPath($HeaderDir,$Header));
    }
    elsif(my $HeaderDir = find_in_dependencies($Header)) {
        return ($Header, joinPath($HeaderDir,$Header));
    }
    elsif($DependencyHeaders_All{$Header}) {
        return ($DependencyHeaders_All{$Header}, $DependencyHeaders_All_FullPath{$Header});
    }
    elsif($DefaultGccHeader{$Header}) {
        return ($DefaultGccHeader{$Header}{"Inc"}, $DefaultGccHeader{$Header}{"Path"});
    }
    elsif(my @Res = search_in_public_dirs($Header)) {
        return @Res;
    }
    elsif(my $HeaderDir = find_in_defaults($Header)) {
        return ($Header, joinPath($HeaderDir,$Header));
    }
    elsif($DefaultCppHeader{$Header}) {
        return ($DefaultCppHeader{$Header}{"Inc"}, $DefaultCppHeader{$Header}{"Path"});
    }
    elsif($Header_Prefix{$Header}
    and my $AnyPath = selectSystemHeader(joinPath($Header_Prefix{$Header},$Header)))
    {# gfile.h in glib-2.0/gio/ and poppler/goo
        return (joinPath($Header_Prefix{$Header},$Header), $AnyPath);
    }
    elsif(my $AnyPath = selectSystemHeader($Header)) {
        return (get_filename($AnyPath), $AnyPath);
    }
    else {
        return ("", "");
    }
}

sub search_in_public_dirs($)
{
    my $Header = $_[0];
    return () if(not $Header);
    my @DefaultDirs = ("sys", "netinet");
    foreach my $Dir (@DefaultDirs)
    {
        if(my $HeaderDir = find_in_defaults(joinPath($Dir,$Header))) {
            return (joinPath($Dir,$Header), joinPath(joinPath($HeaderDir,$Dir),$Header));
        }
    }
    return ();
}

sub cmp_paths($$)
{
    my ($Path1, $Path2) = @_;
    my @Parts1 = split(/[\/\\]/, $Path1);
    my @Parts2 = split(/[\/\\]/, $Path2);
    foreach my $Num (0 .. $#Parts1)
    {
        my $Part1 = $Parts1[$Num];
        my $Part2 = $Parts2[$Num];
        if($GlibcDir{$Part1}
        and not $GlibcDir{$Part2}) {
            return 1;
        }
        elsif($GlibcDir{$Part2}
        and not $GlibcDir{$Part1}) {
            return -1;
        }
        elsif($Part1=~/glib/
        and $Part2!~/glib/) {
            return 1;
        }
        elsif($Part1!~/glib/
        and $Part2=~/glib/) {
            return -1;
        }
        elsif(my $CmpRes = ($Part1 cmp $Part2)) {
            return $CmpRes;
        }
    }
    return 0;
}

sub selectSystemHeader($)
{
    my $FilePath = $_[0];
    return $FilePath if(-f $FilePath);
    return "" if($FilePath=~/\A(\/|\w+:[\/\\])/ and not -f $FilePath);
    return "" if($FilePath=~/\A(atomic|config|configure|build|conf|setup)\.h\Z/i);
    return "" if($OSgroup ne "windows" and get_filename($FilePath)=~/windows|win32|win64|\Ados.h/i);
    return $Cache{"selectSystemHeader"}{$FilePath} if(defined $Cache{"selectSystemHeader"}{$FilePath});
    if($FilePath!~/[\/\\]/)
    {
        foreach my $Path (keys(%{$SystemPaths{"include"}}))
        {# search in default paths
            if(-f $Path."/".$FilePath)
            {
                $Cache{"selectSystemHeader"}{$FilePath} = joinPath($Path,$FilePath);
                return $Cache{"selectSystemHeader"}{$FilePath};
            }
        }
    }
    detectSystemHeaders() if(not keys(%SystemHeaders));
    my $Arch = getArch();
    my $FileName = get_filename($FilePath);
    my $ShortName = $FileName;
    $ShortName=~s/\.\w+\Z//g;
    foreach my $Path (sort {get_depth($a)<=>get_depth($b)} sort {cmp_paths($b, $a)}
    keys(%{$SystemHeaders{$FileName}}))
    {
        if($Path=~/[\/\\]\Q$FilePath\E\Z/)
        {
            if($FilePath!~/[\/\\]/ and get_depth($Path)>=5 and $FilePath!~/config/
            and $Path!~/\/lib\d*\// and get_dirname($Path)!~/\Q$ShortName\E/)
            { # Disallow to search for "abstract" headers in too deep directories
              # Disallow to search for sstream.h or typeinfo.h in /usr/include/wx-2.9/wx/
              # Allow to search for glibconfig.h in /usr/lib/glib-2.0/include/
              # Allow to search for atk.h in /usr/include/atk-1.0/atk/
                next;
            }
            if($Path=~/[\/\\]asm-/ and $Path!~/[\/\\]asm-\Q$Arch\E/) {
                # skip ../asm-arm/ if using x86 architecture
                next;
            }
            if($Path=~/c\+\+[\/\\]\d+/ and $MAIN_CPP_DIR
            and $Path!~/\A\Q$MAIN_CPP_DIR\E/) {
                # skip ../c++/3.3.3/ if using ../c++/4.4.0/
                next;
            }
            if($FileName eq "parser.h" and $Path!~/\/libxml2\//) {
                # select parser.h from xml2 library
                next;
            }
            $Cache{"selectSystemHeader"}{$FilePath} = $Path;
            return $Path;
        }
    }
    $Cache{"selectSystemHeader"}{$FilePath} = "";
    return "";
}

sub detectSystemHeaders()
{
    foreach my $DevelPath (keys(%{$SystemPaths{"include"}}))
    {
        if(-d $DevelPath)
        {
            foreach my $Path (cmd_find($DevelPath,"f","",""))
            {
                $SystemHeaders{get_filename($Path)}{$Path}=1;
                if(get_depth($Path)>=3
                and my $Prefix = get_filename(get_dirname($Path))) {
                    $SystemHeaders{joinPath($Prefix,get_filename($Path))}{$Path}=1;
                }
            }
        }
    }
    foreach my $DevelPath (keys(%{$SystemPaths{"lib"}}))
    {
        if(-d $DevelPath)
        {
            foreach my $Path (cmd_find($DevelPath,"f","*.h",""))
            {
                $SystemHeaders{get_filename($Path)}{$Path}=1;
                if(get_depth($Path)>=3
                and my $Prefix = get_filename(get_dirname($Path))) {
                    $SystemHeaders{joinPath($Prefix,get_filename($Path))}{$Path}=1;
                }
            }
        }
    }
}

sub detectSystemObjects()
{
    foreach my $DevelPath (keys(%{$SystemPaths{"lib"}}))
    {
        if(-d $DevelPath)
        {
            foreach my $Path (cmd_find($DevelPath,"",".*\\.$LIB_EXT\[0-9.]*",""))
            {
                $SystemObjects{short_soname(get_filename($Path))}{$Path}=1;
                $SystemObjects{cut_lib_suffix(get_filename($Path))}{$Path}=1;
            }
        }
    }
}

sub alignSpaces($)
{
    my $Code = $_[0];
    my $Code_Copy = $Code;
    my ($MinParagraph, $Paragraph);
    while($Code=~s/\A([ ]+)//) {
        $MinParagraph = length($1) if(not defined $MinParagraph or $MinParagraph>length($1));
    }
    foreach (1 .. $MinParagraph) {
        $Paragraph .= " ";
    }
    $Code_Copy=~s/(\A|\n)$Paragraph/$1/g;
    return $Code_Copy;
}

sub alignCode($$$)
{
    my ($Code, $Code_Align, $Single) = @_;
    return "" if($Code eq "" or $Code_Align eq "");
    my $Paragraph = get_paragraph($Code_Align, 0);
    $Code=~s/\n([^\n])/\n$Paragraph$1/g;
    if(not $Single) {
        $Code=~s/\A/$Paragraph/g;
    }
    return $Code;
}

sub get_paragraph($$)
{
    my ($Code, $MaxMin) = @_;
    my ($MinParagraph_Length, $MinParagraph);
    while($Code=~s/\A([ ]+)//)
    {
        if(not defined $MinParagraph_Length or
        (($MaxMin)?$MinParagraph_Length<length($1):$MinParagraph_Length>length($1))) {
            $MinParagraph_Length = length($1);
        }
    }
    foreach (1 .. $MinParagraph_Length) {
        $MinParagraph .= " ";
    }
    return $MinParagraph;
}

sub appendFile($$)
{
    my ($Path, $Content) = @_;
    return if(not $Path);
    if(my $Dir = get_dirname($Path)) {
        mkpath($Dir);
    }
    open (FILE, ">>".$Path) || die ("can't open file \'$Path\': $!\n");
    print FILE $Content;
    close(FILE);
}

sub writeFile($$)
{
    my ($Path, $Content) = @_;
    return if(not $Path);
    if(my $Dir = get_dirname($Path)) {
        mkpath($Dir);
    }
    open (FILE, ">".$Path) || die ("can't open file \'$Path\': $!\n");
    print FILE $Content;
    close(FILE);
}

sub readFile($)
{
    my $Path = $_[0];
    return "" if(not $Path or not -f $Path);
    open (FILE, $Path);
    my $Content = join("", <FILE>);
    close(FILE);
    $Content=~s/\r//g;
    return $Content;
}

sub get_RunScript($)
{
    my $Interface = $_[0];
    my @ImpLibs = ();
    my %Dir = ();
    foreach my $Lib_Path (sort {length($a)<=>length($b)} keys(%SharedObjects))
    {
        my $Lib_Dir = get_dirname($Lib_Path);
        next if($Dir{$Lib_Dir} or $DefaultLibPaths{$Lib_Dir});
        push(@ImpLibs, $Lib_Dir);
        $Dir{$Lib_Dir} = 1;
    }
    if($OSgroup eq "windows")
    {
        if(@ImpLibs) {
            my $EnvSet = "\@set PATH=".join(";",@ImpLibs).";\%PATH\%";
            return $EnvSet."\ntest.exe arg1 arg2 arg3 >output 2>&1\n";
        }
        else {
            return "test.exe arg1 arg2 arg3 >output 2>&1\n";
        }
    }
    elsif($OSgroup eq "macos")
    {
        if(@ImpLibs) {
            my $EnvSet = "export DYLD_LIBRARY_PATH=\$DYLD_LIBRARY_PATH:\"".join(":", @ImpLibs)."\"";
            return "#!/bin/sh\n$EnvSet && ./test arg1 arg2 arg3 >output 2>&1\n";
        }
        else {
            return "#!/bin/sh\n./test arg1 arg2 arg3 >output 2>&1\n";
        }
    }
    else
    {
        if(@ImpLibs) {
            my $EnvSet = "export LD_LIBRARY_PATH=\$LD_LIBRARY_PATH:\"".join(":", @ImpLibs)."\"";
            return "#!/bin/sh\n$EnvSet && ./test arg1 arg2 arg3 >output 2>&1\n";
        }
        else {
            return "#!/bin/sh\n./test arg1 arg2 arg3 >output 2>&1\n";
        }
    }
}

sub get_HeaderDeps($)
{
    my $AbsPath = $_[0];
    return () if(not $AbsPath);
    return @{$Cache{"get_HeaderDeps"}{$AbsPath}} if(defined $Cache{"get_HeaderDeps"}{$AbsPath});
    my %IncDir = ();
    detect_recursive_includes($AbsPath);
    foreach my $HeaderPath (keys(%{$RecursiveIncludes{$AbsPath}}))
    {
        next if(not $HeaderPath);
        next if($MAIN_CPP_DIR and $HeaderPath=~/\A\Q$MAIN_CPP_DIR\E([\/\\]|\Z)/);
        my $Dir = get_dirname($HeaderPath);
        foreach my $Prefix (keys(%{$Header_Include_Prefix{$AbsPath}{$HeaderPath}}))
        {
            my $Dep = $Dir;
            if($Prefix)
            {
                if(not $Dep=~s/[\/\\]+\Q$Prefix\E\Z//g)
                {
                    if($OSgroup eq "macos" and $HeaderPath=~/(.+\.framework)\/Headers\/([^\/]+)/)
                    {# frameworks
                        my ($HFramework, $HName) = ($1, $2);
                        $Dep = $HFramework;
                    }
                    else
                    {# mismatch
                        next;
                    }
                }
            }
            next if(not $Dep or is_default_include_dir($Dep) or get_depth($Dep)==1);
            next if($DefaultIncPaths{get_dirname($Dep)} and $GlibcDir{get_filename($Dep)});
            $IncDir{$Dep}=1;
        }
    }
    $Cache{"get_HeaderDeps"}{$AbsPath} = sortIncPaths([keys(%IncDir)]);
    return @{$Cache{"get_HeaderDeps"}{$AbsPath}};
}

sub sortIncPaths($)
{
    my $ArrRef = $_[0];
    @{$ArrRef} =  sort {$Header_Dependency{$b}<=>$Header_Dependency{$a}}
      sort {get_depth($a)<=>get_depth($b)} sort {$b cmp $a} @{$ArrRef};
    return $ArrRef;
}

sub is_default_include_dir($)
{
    my $Dir = $_[0];
    $Dir=~s/[\/\\]+\Z//;
    return ($DefaultGccPaths{$Dir} or $DefaultCppPaths{$Dir} or $DefaultIncPaths{$Dir});
}

sub cut_lib_suffix($)
{
    my $Name = $_[0];
    return "" if(not $Name);
    $Name = short_soname($Name);
    $Name=~s/[\d-._]*\.$LIB_EXT\Z//g;
    return $Name;
}

sub short_soname($)
{
    my $Name = $_[0];
    $Name=~s/(?<=\.$LIB_EXT)\.[0-9.]+\Z//g;
    return $Name;
}

sub get_shared_object_deps($)
{
    my $Path = $_[0];
    return () if(not $Path or not -e $Path);
    my $LibName = get_filename($Path);
    return if(isCyclical(\@RecurLib, $LibName));
    push(@RecurLib, $LibName);
    my %Deps = ();
    foreach my $Dep (keys(%{$SystemObjects_Needed{$Path}}))
    {
        $Deps{$Dep}=1;
        foreach my $RecurDep (get_shared_object_deps($Dep)) {
            $Deps{$RecurDep}=1;
        }
    }
    pop(@RecurLib);
    return keys(%Deps);
}

sub get_library_pure_symlink($)
{
    my $Path = $_[0];
    return "" if(not $Path or not -e $Path);
    my ($Directory, $Name) = separate_path($Path);
    $Name=~s/[0-9.]+\Z//g;
    $Name=~s/[\-0-9.]+(\.$LIB_EXT)\Z/$1/g;
    my $Candidate = joinPath($Directory,$Name);
    if(-f $Candidate
    and resolve_symlink($Candidate) eq $Path) {
        return $Candidate;
    }
    else {
        return "";
    }
}

sub get_Makefile($$)
{
    my ($Interface, $HeadersList) = @_;
    my (%LibPaths, %LibPaths_All, %LibNames_All, %LibSuffixes, %SpecLib_Paths) = ();
    foreach my $SpecLib (keys(%SpecLibs))
    {
        if(my $SpecLib_Path = find_solib_path($SpecLib)) {
            $SpecLib_Paths{$SpecLib_Path} = 1;
        }
    }
    my (%UsedSharedObjects, %UsedSharedObjects_Needed) = ();
    foreach my $Interface (keys(%UsedInterfaces))
    {
        if(my $Path = $Interface_Library{$Interface}) {
            $UsedSharedObjects{$Path}=1;
        }
        elsif(my $Path = $NeededInterface_Library{$Interface}) {
            $UsedSharedObjects{$Path}=1;
        }
        else
        {
            foreach my $SoPath (find_symbol_libs($Interface)) {
                $UsedSharedObjects{$SoPath}=1;
            }
        }
    }
    if(not keys(%UsedSharedObjects)) {
        %UsedSharedObjects = %SharedObjects;
    }
    foreach my $Path (keys(%UsedSharedObjects))
    {
        foreach my $Dep (get_shared_object_deps($Path)) {
            $UsedSharedObjects_Needed{$Dep}=1;
        }
    }
    my $LIBS = "";
    foreach my $Path (sort (keys(%UsedSharedObjects), keys(%CompilerOptions_Libs), keys(%UsedSharedObjects_Needed), keys(%SpecLib_Paths)))
    {
        if(my $Link = get_library_pure_symlink($Path)) {
            $Path = $Link;
        }
        $LibPaths_All{"\"".get_dirname($Path)."\""} = 1;
        $LibNames_All{get_filename($Path)} = 1;
        if(($Path=~/\.$LIB_EXT\Z/ or -f short_soname($Path))
        and $Path=~/\A(.*)[\/\\]lib([^\/\\]+)\.$LIB_EXT[^\/\\]*\Z/)
        {
            $LibPaths{$1} = 1;
            $LibSuffixes{$2} = 1;
        }
        else {
            $LIBS .= " ".$Path;
        }
    }
    foreach my $Path (keys(%LibPaths))
    {
        next if(not $Path or $DefaultLibPaths{$Path});
        $LIBS .= " -L".$Path;
    }
    foreach my $Suffix (keys(%LibSuffixes)) {
        $LIBS .= " -l".$Suffix;
    }
    my ($IncludeString, $Style) = ("", "");
    if($OSgroup eq "windows") {
        $Style = "CL";
    }
    else {
        $Style = "GCC";
    }
    if($INC_PATH_AUTODETECT)
    { # try to ptimize list
        my %IncDir = get_HeaderDeps_forList(%{$HeadersList});
        $IncludeString = getIncString([keys(%IncDir)], $Style);
    }
    else
    { # user defined paths
        $IncludeString = getIncString(getIncPaths(), $Style);
    }
    if($OSgroup eq "windows")
    { # compiling using CL and NMake
        my $CL = get_CmdPath("cl");
        if(not $CL) {
            print STDERR "ERROR: can't find \"cl\" compiler\n";
            exit(1);
        }
        my $Makefile = "CC       = $CL".($IncludeString?"\nINCLUDES = $IncludeString":"").(keys(%LibNames_All)?"\nLIBS     = ".join(" ", keys(%LibNames_All)):"")."\n\nall: test.exe\n\n";
        $Makefile .= "test.exe: test.cpp\n\t".(keys(%LibNames_All)?"set LIB=".join(";", keys(%LibPaths_All)).";\$(LIB)\n\t":"")."\$(CC) ".($IncludeString?"\$(INCLUDES) ":"")."test.cpp".(keys(%LibNames_All)?" \$(LIBS)":"")."\n\n";
        $Makefile .= "clean:\n\tdel test.exe test\.obj\n";
        return $Makefile;
    }
    else
    { # compiling using GCC and Make
        if(getTestLang($Interface) eq "C++")
        {
            my $Makefile = "CXX      = $GPP_PATH\nCXXFLAGS = -Wall$CompilerOptions_Headers".($IncludeString?"\nINCLUDES = $IncludeString":"").($LIBS?"\nLIBS     = $LIBS":"")."\n\nall: test\n\n";
            $Makefile .= "test: test.cpp\n\t\$(CXX) \$(CXXFLAGS)".($IncludeString?" \$(INCLUDES)":"")." test.cpp -o test".($LIBS?" \$(LIBS)":"")."\n\n";
            $Makefile .= "clean:\n\trm -f test test\.o\n";
            return $Makefile;
        }
        else
        {
            my $Makefile = "CC       = $GCC_PATH\nCFLAGS   = -Wall$CompilerOptions_Headers".($IncludeString?"\nINCLUDES = $IncludeString":"").($LIBS?"\nLIBS     = $LIBS":"")."\n\nall: test\n\n";
            $Makefile .= "test: test.c\n\t\$(CC) \$(CFLAGS)".($IncludeString?" \$(INCLUDES)":"")." test.c -o test".($LIBS?" \$(LIBS)":"")."\n\n";
            $Makefile .= "clean:\n\trm -f test test\.o\n";
            return $Makefile;
        }
    }
}

sub get_one_step_title($$$$$)
{
    my ($Num, $All_Count, $Head, $Success, $Fail)  = @_;
    my $Title = "$Head: $Num/$All_Count [".cut_off_number($Num*100/$All_Count, 3)."%],";
    $Title .= " success/fail: $Success/$Fail";
    return $Title."    ";
}

sub addArrows($)
{
    my $Text = $_[0];
    $Text=~s/\-\>/&minus;&gt;/g;
    return $Text;
}

sub insertIDs($)
{
    my $Text = $_[0];
    
    while($Text=~/CONTENT_ID/)
    {
        if(int($Content_Counter)%2) {
            $ContentID -= 1;
        }
        $Text=~s/CONTENT_ID/c_$ContentID/;
        $ContentID += 1;
        $Content_Counter += 1;
    }
    return $Text;
}

sub cut_off_number($$)
{
    my ($num, $digs_to_cut) = @_;
    if($num!~/\./)
    {
        $num .= ".";
        foreach (1 .. $digs_to_cut-1) {
            $num .= "0";
        }
    }
    elsif($num=~/\.(.+)\Z/ and length($1)<$digs_to_cut-1)
    {
        foreach (1 .. $digs_to_cut - 1 - length($1)) {
            $num .= "0";
        }
    }
    elsif($num=~/\d+\.(\d){$digs_to_cut,}/) {
      $num=sprintf("%.".($digs_to_cut-1)."f", $num);
    }
    return $num;
}

sub generate_tests()
{
    rmtree($TEST_SUITE_PATH) if(-e $TEST_SUITE_PATH);
    mkpath($TEST_SUITE_PATH);
    ($ResultCounter{"Gen"}{"Success"}, $ResultCounter{"Gen"}{"Fail"}) = (0, 0);
    my %TargetInterfaces = ();
    if($TargetHeaderName)
    {# from the header file
        if(not $DependencyHeaders_All{$TargetHeaderName}) {
            print STDERR "ERROR: specified header \'$TargetHeaderName\' is not found\n";
            return;
        }
        foreach my $Interface (keys(%Interface_Library))
        {
            if($CompleteSignature{$Interface}{"Header"} eq $TargetHeaderName
            and interfaceFilter($Interface)) {
                $TargetInterfaces{$Interface} = 1;
            }
        }
    }
    elsif(keys(%InterfacesList))
    {# from the list
        foreach my $Interface (keys(%InterfacesList))
        {
            if(interfaceFilter($Interface)) {
                $TargetInterfaces{$Interface} = 1;
            }
        }
    }
    elsif($CheckHeadersOnly and keys(%CompleteSignature))
    {# from the headers
        foreach my $Interface (keys(%CompleteSignature))
        {
            next if($Interface=~/\A__/ or $CompleteSignature{$Interface}{"ShortName"}=~/\A__/);
            next if(not $DependencyHeaders_All{$CompleteSignature{$Interface}{"Header"}});
            if(interfaceFilter($Interface)) {
                $TargetInterfaces{$Interface} = 1;
            }
        }
    }
    else
    {# from the shared objects (default)
        if(not keys(%Interface_Library)) {
            print STDERR "ERROR: cannot generate tests because extracted list of symbols is empty\n";
            exit(1);
        }
        foreach my $Interface (keys(%Interface_Library))
        {
            if(interfaceFilter($Interface)) {
                $TargetInterfaces{$Interface} = 1;
            }
        }
        foreach my $Interface (keys(%NeededInterface_Library))
        {
            if($RegisteredHeaders_Short{$CompleteSignature{$Interface}{"Header"}}
            and interfaceFilter($Interface)) {
                $TargetInterfaces{$Interface} = 1;
            }
        }
        if(not keys(%TargetInterfaces)) {
            print STDERR "ERROR: cannot obtain enough information from header files to generate tests\n";
            exit(1);
        }
    }
    if(not keys(%TargetInterfaces)) {
        print STDERR "ERROR: specified information is not enough to generate tests\n";
        exit(1);
    }
    unlink($TEST_SUITE_PATH."/scenario");
    open(FAIL_LIST, ">$TEST_SUITE_PATH/gen_fail_list");
    if(defined $Template2Code)
    {
        if(keys(%LibGroups))
        {
            my %LibGroups_Filtered = ();
            my ($Test_Num, $All_Count) = (0, 0);
            foreach my $LibGroup (sort {lc($a) cmp lc($b)} keys(%LibGroups))
            {
                foreach my $Interface (keys(%{$LibGroups{$LibGroup}}))
                {
                    if($TargetInterfaces{$Interface}) {
                        $LibGroups_Filtered{$LibGroup}{$Interface} = 1;
                        $All_Count+=1;
                    }
                }
            }
            foreach my $LibGroup (sort {lc($a) cmp lc($b)} keys(%LibGroups_Filtered))
            {
                my @Ints = sort {lc($a) cmp lc($b)} keys(%{$LibGroups_Filtered{$LibGroup}});
                t2c_group_generation($LibGroup, "", \@Ints, 0, \$Test_Num, $All_Count);
            }
            print "\r".get_one_step_title($All_Count, $All_Count, "generating tests", $ResultCounter{"Gen"}{"Success"}, $ResultCounter{"Gen"}{"Fail"})."\n";
        }
        else
        {
            my $TwoComponets = 0;
            my %Header_Class_Interface = ();
            my ($Test_Num, $All_Count) = (0, int(keys(%TargetInterfaces)));
            foreach my $Interface (sort {lc($a) cmp lc($b)} keys(%TargetInterfaces))
            {
                my %Signature = %{$CompleteSignature{$Interface}};
                $Header_Class_Interface{$Signature{"Header"}}{get_type_short_name(get_TypeName($Signature{"Class"}))}{$Interface}=1;
                if($Signature{"Class"}) {
                    $TwoComponets=1;
                }
            }
            foreach my $Header (sort {lc($a) cmp lc($b)} keys(%Header_Class_Interface))
            {
                foreach my $ClassName (sort {lc($a) cmp lc($b)} keys(%{$Header_Class_Interface{$Header}}))
                {
                    my @Ints = sort {lc($a) cmp lc($b)} keys(%{$Header_Class_Interface{$Header}{$ClassName}});
                    t2c_group_generation($Header, $ClassName, \@Ints, $TwoComponets, \$Test_Num, $All_Count);
                }
            }
            print "\r".get_one_step_title($All_Count, $All_Count, "generating tests", $ResultCounter{"Gen"}{"Success"}, $ResultCounter{"Gen"}{"Fail"})."\n";
        }
        writeFile("$TEST_SUITE_PATH/$TargetLibraryName-t2c/$TargetLibraryName.cfg", "# Custom compiler options\nCOMPILER_FLAGS = -DCHECK_EXT_REQS `pkg-config --cflags ".search_pkgconfig_name($TargetLibraryName)."` -D_GNU_SOURCE\n\n# Custom linker options\nLINKER_FLAGS = `pkg-config --libs ".search_pkgconfig_name($TargetLibraryName)."`\n\n# Maximum time (in seconds) each test is allowed to run\nWAIT_TIME = $HANGED_EXECUTION_TIME\n\n# Copyright holder\nCOPYRIGHT_HOLDER = Linux Foundation\n");
    }
    else
    {#standalone
        my $Test_Num = 0;
        if(keys(%LibGroups))
        {
            foreach my $Interface (keys(%TargetInterfaces))
            {
                if(not $Interface_LibGroup{$Interface}) {
                    delete($TargetInterfaces{$Interface});
                }
            }
        }
        my $All_Count = keys(%TargetInterfaces);
        foreach my $Interface (sort {lc($a) cmp lc($b)} keys(%TargetInterfaces))
        {
            print "\r".get_one_step_title($Test_Num, $All_Count, "generating tests", $ResultCounter{"Gen"}{"Success"}, $ResultCounter{"Gen"}{"Fail"});
            #resetting global state
            restore_state(());
            @RecurInterface = ();
            @RecurTypeId = ();
            @RecurSpecType = ();
            %SubClass_Created = ();
            my %Result = generate_sanity_test($Interface);
            if(not $Result{"IsCorrect"}) {
                print FAIL_LIST $Interface."\n";
                if($StrictGen) {
                    print STDERR "ERROR: can't generate test for $Interface\n";
                    exit(1);
                }
            }
            $Test_Num += 1;
        }
        write_scenario();
        print "\r".get_one_step_title($All_Count, $All_Count, "generating tests", $ResultCounter{"Gen"}{"Success"}, $ResultCounter{"Gen"}{"Fail"})."\n";
        restore_state(());
    }
    close(FAIL_LIST);
    unlink($TEST_SUITE_PATH."/gen_fail_list") if(not readFile($TEST_SUITE_PATH."/gen_fail_list"));
}

sub get_pkgconfig_name($)
{
    my $Name = $_[0];
    if(my $PkgConfig = get_CmdPath("pkg-config"))
    {
        my %InstalledLibs = ();
        foreach my $Line (split(/\n/, `$PkgConfig --list-all`))
        {
            if($Line=~/\A([^ ]+)[ ]+(.+?)\Z/) {
                $InstalledLibs{$1}=$2;
            }
        }
        foreach my $Lib (sort {length($a)<=>length($b)} keys(%InstalledLibs))
        {
            if($Lib=~/([^a-z]|\A)$Name([^a-z]|\Z)/i) {
                return $Lib;
            }
        }
        return "";
    }
    else
    {
        foreach my $PkgPath (keys(%{$SystemPaths{"pkgconfig"}}))
        {
            if(-d $PkgPath)
            {
                foreach my $Path (sort {length($a)<=>length($b)} cmd_find($PkgPath,"f","*$Name*",""))
                {
                    my $LibName = get_filename($Path);
                    $LibName=~s/\.pc\Z//;
                    if($LibName=~/([^a-z]|\A)\Q$Name\E([^a-z]|\Z)/i) {
                        return $LibName;
                    }
                }
            }
        }
        return "";
    }
}

sub search_pkgconfig_name($)
{
    my $Name = lc($_[0]);
    my $PkgConfigName = get_pkgconfig_name($Name);
    return $PkgConfigName if($PkgConfigName);
    if($Name=~s/[0-9_-]+\Z//)
    {
        if($PkgConfigName = get_pkgconfig_name($Name)) {
            return $PkgConfigName;
        }
    }
    if($Name=~s/\Alib//i)
    {
        if($PkgConfigName = get_pkgconfig_name($Name)) {
            return $PkgConfigName;
        }
    }
    else
    {
        $Name = "lib".$Name;
        if($PkgConfigName = get_pkgconfig_name($Name)) {
            return $PkgConfigName;
        }
    }
    return lc($Name);
}

sub t2c_group_generation($$$$$$)
{
    my ($C1, $C2, $Interfaces, $TwoComponets, $Test_NumRef, $All_Count) = @_;
    my ($SuitePath, $MediumPath, $TestName) = getLibGroupPath($C1, $C2, $TwoComponets);
    my $MaxLength = 0;
    my $LibGroupName = getLibGroupName($C1, $C2);
    my %TestComponents = ();
    #resetting global state for section
    restore_state(());
    foreach my $Interface (@{$Interfaces})
    {
        print "\r".get_one_step_title(${$Test_NumRef}, $All_Count, "generating tests", $ResultCounter{"Gen"}{"Success"}, $ResultCounter{"Gen"}{"Fail"});
        restore_local_state(());
        %IntrinsicNum=(
        "Char"=>64,
        "Int"=>0,
        "Str"=>0,
        "Float"=>0);
        @RecurInterface = ();
        @RecurTypeId = ();
        @RecurSpecType = ();
        %SubClass_Created = ();
        my $Global_State = save_state();
        my %Result = generate_sanity_test($Interface);
        if(not $Result{"IsCorrect"})
        {
            restore_state($Global_State);
            print FAIL_LIST $Interface."\n";
        }
        ${$Test_NumRef} += 1;
        $TestComponents{"Headers"} = addHeaders($TestComponents{"Headers"}, $Result{"Headers"});
        $TestComponents{"Code"} .= $Result{"Code"};
        my ($DefinesList, $ValuesList) = list_t2c_defines();
        $TestComponents{"Blocks"} .= "##=========================================================================\n## ".get_Signature($Interface)."\n\n<BLOCK>\n\n<TARGETS>\n    ".$CompleteSignature{$Interface}{"ShortName"}."\n</TARGETS>\n\n".(($DefinesList)?"<DEFINE>\n".$DefinesList."</DEFINE>\n\n":"")."<CODE>\n".$Result{"main"}."</CODE>\n\n".(($ValuesList)?"<VALUES>\n".$ValuesList."</VALUES>\n\n":"")."</BLOCK>\n\n\n";
        $MaxLength = length($CompleteSignature{$Interface}{"ShortName"}) if($MaxLength<length($CompleteSignature{$Interface}{"ShortName"}));
    }
    #adding test data
    my $TestDataDir = $SuitePath."/testdata/".(($MediumPath)?"$MediumPath/":"")."$TestName/";
    mkpath($TestDataDir);
    $TestComponents{"Blocks"} = add_VirtualTestData($TestComponents{"Blocks"}, $TestDataDir);
    $TestComponents{"Blocks"} = add_TestData($TestComponents{"Blocks"}, $TestDataDir);
    my $Content = "#library $TargetLibraryName\n#libsection $LibGroupName\n\n<GLOBAL>\n\n// Tested here:\n";
    foreach my $Interface (@{$Interfaces})
    {#development progress
        $Content .= "// ".$CompleteSignature{$Interface}{"ShortName"};
        foreach (0 .. $MaxLength - length($CompleteSignature{$Interface}{"ShortName"}) + 2) {
            $Content .= " ";
        }
        $Content .= "DONE (shallow)\n";
    }
    $Content .= "\n";
    foreach my $Header (@{$TestComponents{"Headers"}})
    {#includes
        $Content .= "#include <$Header>\n";
    }
    $Content .= "\n".$TestComponents{"Code"}."\n" if($TestComponents{"Code"});
    $Content .= "\n</GLOBAL>\n\n".$TestComponents{"Blocks"};
    writeFile($SuitePath."/src/".(($MediumPath)?"$MediumPath/":"")."$TestName.t2c", $Content);
    writeFile($SuitePath."/reqs/".(($MediumPath)?"$MediumPath/":"")."$TestName.xml", get_requirements_catalog($Interfaces));
}

sub get_requirements_catalog($)
{
    my @Interfaces = @{$_[0]};
    my $Reqs = "";
    foreach my $Interface (@Interfaces)
    {
        foreach my $ReqId (sort {int($a)<=>int($b)} keys(%{$RequirementsCatalog{$Interface}}))
        {
            my $Req = $RequirementsCatalog{$Interface}{$ReqId};
            $Req=~s/&/&amp;/g;
            $Req=~s/>/&gt;/g;
            $Req=~s/</&lt;/g;
            $Reqs .= "<req id=\"".$CompleteSignature{$Interface}{"ShortName"}.".".normalize_num($ReqId)."\">\n    $Req\n</req>\n";
        }
    }
    if(not $Reqs) {
        $Reqs = "<req id=\"function.01\">\n    If ... then ...\n</req>\n";
    }
    return "<?xml version=\"1.0\"?>\n<requirements>\n".$Reqs."</requirements>\n";
}

sub list_t2c_defines()
{
    my (%Defines, $DefinesList, $ValuesList) = ();
    my $MaxLength = 0;
    foreach my $Define (sort keys(%Template2Code_Defines))
    {
        if($Define=~/\A(\d+):(.+)\Z/)
        {
            $Defines{$1}{"Name"} = $2;
            $Defines{$1}{"Value"} = $Template2Code_Defines{$Define};
            $MaxLength = length($2) if($MaxLength<length($2));
        }
    }
    foreach my $Pos (sort {int($a) <=> int($b)} keys(%Defines))
    {
        $DefinesList .= "#define ".$Defines{$Pos}{"Name"};
        foreach (0 .. $MaxLength - length($Defines{$Pos}{"Name"}) + 2) {
            $DefinesList .= " ";
        }
        $DefinesList .= "<%$Pos%>\n";
        $ValuesList .= "    ".$Defines{$Pos}{"Value"}."\n";
    }
    return ($DefinesList, $ValuesList);
}

sub cut_off($$)
{
    my ($String, $Num) = @_;
    if(length($String)<$Num) {
        return $String;
    }
    else {
        return substr($String, 0, $Num)."...";
    }
}

sub build_tests()
{
    if(-e $TEST_SUITE_PATH."/build_fail_list") {
        unlink($TEST_SUITE_PATH."/build_fail_list");
    }
    ($ResultCounter{"Build"}{"Success"}, $ResultCounter{"Build"}{"Fail"}) = (0, 0);
    read_scenario();
    return if(not keys(%Interface_TestDir));
    my $All_Count = keys(%Interface_TestDir);
    my $Test_Num = 0;
    open(FAIL_LIST, ">$TEST_SUITE_PATH/build_fail_list");
    foreach my $Interface (sort {lc($a) cmp lc($b)} keys(%Interface_TestDir))
    {#building tests
        print "\r".get_one_step_title($Test_Num, $All_Count, "building tests", $ResultCounter{"Build"}{"Success"}, $ResultCounter{"Build"}{"Fail"});
        build_sanity_test($Interface);
        if(not $BuildResult{$Interface}{"IsCorrect"}) {
            print FAIL_LIST $Interface_TestDir{$Interface}."\n";
            if($StrictBuild) {
                print STDERR "ERROR: can't build test for $Interface\n";
                exit(1);
            }
        }
        $Test_Num += 1;
    }
    close(FAIL_LIST);
    unlink($TEST_SUITE_PATH."/build_fail_list") if(not readFile($TEST_SUITE_PATH."/build_fail_list"));
    print "\r".get_one_step_title($All_Count, $All_Count, "building tests", $ResultCounter{"Build"}{"Success"}, $ResultCounter{"Build"}{"Fail"})."\n";
}

sub clean_tests()
{
    read_scenario();
    return if(not keys(%Interface_TestDir));
    my $All_Count = keys(%Interface_TestDir);
    my $Test_Num = 0;
    foreach my $Interface (sort {lc($a) cmp lc($b)} keys(%Interface_TestDir))
    {#cleaning tests
        print "\r".get_one_step_title($Test_Num, $All_Count, "cleaning tests", $Test_Num, 0);
        clean_sanity_test($Interface);
        $Test_Num += 1;
    }
    print "\r".get_one_step_title($All_Count, $All_Count, "cleaning tests", $All_Count, 0)."\n";
}

sub run_tests()
{
    if(-f $TEST_SUITE_PATH."/run_fail_list") {
        unlink($TEST_SUITE_PATH."/run_fail_list");
    }
    ($ResultCounter{"Run"}{"Success"}, $ResultCounter{"Run"}{"Fail"}) = (0, 0);
    read_scenario();
    if(not keys(%Interface_TestDir)) {
        print STDERR "ERROR: tests were not generated yet\n";
        return 1;
    }
    my %ForRunning = ();
    foreach my $Interface (keys(%Interface_TestDir))
    {
        if(-f $Interface_TestDir{$Interface}."/test"
        or -f $Interface_TestDir{$Interface}."/test.exe") {
            $ForRunning{$Interface} = 1;
        }
    }
    my $All_Count = keys(%ForRunning);
    if($All_Count==0) {
        print STDERR "ERROR: tests were not built yet\n";
        return 1;
    }
    my $Test_Num = 0;
    open(FAIL_LIST, ">$TEST_SUITE_PATH/run_fail_list");
    my $XvfbStarted = 0;
    $XvfbStarted = runXvfb() if($UseXvfb);
    foreach my $Interface (sort {lc($a) cmp lc($b)} keys(%ForRunning))
    {# running tests
        print "\r".get_one_step_title($Test_Num, $All_Count, "running tests", $ResultCounter{"Run"}{"Success"}, $ResultCounter{"Run"}{"Fail"});
        run_sanity_test($Interface);
        if(not $RunResult{$Interface}{"IsCorrect"}) {
            print FAIL_LIST $Interface_TestDir{$Interface}."\n";
            if($StrictRun) {
                print STDERR "ERROR: test run failed for $Interface\n";
                exit(1);
            }
        }
        $Test_Num += 1;
    }
    stopXvfb($XvfbStarted) if($UseXvfb);
    close(FAIL_LIST);
    unlink($TEST_SUITE_PATH."/run_fail_list") if(not readFile($TEST_SUITE_PATH."/run_fail_list"));
    print "\r".get_one_step_title($All_Count, $All_Count, "running tests", $ResultCounter{"Run"}{"Success"}, $ResultCounter{"Run"}{"Fail"})."\n";
    return 0;
}

sub detectPointerSize()
{
    return if(not $GCC_PATH);
    writeFile("$TMP_DIR/empty.h", "");
    my $Defines = `$GCC_PATH -E -dD $TMP_DIR/empty.h`;
    unlink("$TMP_DIR/empty.h");
    if($Defines=~/ __SIZEOF_POINTER__\s+(\d+)/)
    {# GCC 4
        $POINTER_SIZE = $1;
    }
    elsif($Defines=~/ __PTRDIFF_TYPE__\s+(\w+)/)
    {# GCC 3
        my $PTRDIFF = $1;
        if($PTRDIFF=~/long/) {
            $POINTER_SIZE = 8;
        }
        else {
            $POINTER_SIZE = 4;
        }
    }
    if(not int($POINTER_SIZE)) {
        print STDERR "ERROR: can't check size of pointer\n";
        exit(1);
    }
}

sub init_signals()
{
    return if(not defined $Config{"sig_name"}
    or not defined $Config{"sig_num"});
    my $No = 0;
    my @Numbers = split(/\s/, $Config{"sig_num"} );
    foreach my $Name (split(/\s/, $Config{"sig_name"})) 
    {
        if(not $SigName{$Numbers[$No]}
        or $Name=~/\A(SEGV|ABRT)\Z/)
        {
            $SigNo{$Name} = $Numbers[$No];
            $SigName{$Numbers[$No]} = $Name;
        }
        $No+=1;
    }
}

sub genDescriptorTemplate()
{
    writeFile("VERSION.xml", $Descriptor_Template."\n");
    print "XML-descriptor template ./VERSION.xml has been generated\n";
}

sub genSpecTypeTemplate()
{
    writeFile("SPECTYPES.xml", $SpecType_Template."\n");
    print "specialized type template ./SPECTYPES.xml has been generated\n";
}

sub testSystem_cpp()
{
    print "testing C++ library API\n";
    my ($DataDefs, $Sources)  = ();
    my $DeclSpec = ($OSgroup eq "windows")?"__declspec( dllexport )":"";
    
    #Simple parameters
    $DataDefs .= "
        $DeclSpec int func_simple_parameters(
            int a,
            float b,
            double c,
            long double d,
            long long e,
            char f,
            unsigned int g,
            const char* h,
            char* i,
            unsigned char* j,
            char** k,
            const char*& l,
            const char**& m,
            char const*const* n,
            unsigned int* offset
        );";
    $Sources .= "
        int func_simple_parameters(
            int a,
            float b,
            double c,
            long double d,
            long long e,
            char f,
            unsigned int g,
            const char* h,
            char* i,
            unsigned char* j,
            char** k,
            const char*& l,
            const char**& m,
            char const*const* n,
            unsigned int* offset ) {
            return 1;
        }";
    
    #Initialization by interface
    $DataDefs .= "
        struct simple_struct {
            int m;
        };
        $DeclSpec struct simple_struct simple_func(int a, int b);";
    $Sources .= "
        struct simple_struct simple_func(int a, int b)
        {
            struct simple_struct x = {1};
            return x;
        }";
    
    $DataDefs .= "
        $DeclSpec int func_init_param_by_interface(struct simple_struct p);";
    $Sources .= "
        int func_init_param_by_interface(struct simple_struct p) {
            return 1;
        }";
    
    #Private Interface
    $DataDefs .= "
        class $DeclSpec private_class {
        private:
            private_class(){};
            int a;
            float private_func(float p);
        };";
    $Sources .= "
        float private_class::private_func(float p) {
            return p;
        }";
    
    #Assembling structure
    $DataDefs .= "
        struct complex_struct {
            int a;
            float b;
            struct complex_struct* c;
        };";
    
    $DataDefs .= "
        $DeclSpec int func_assemble_param(struct complex_struct p);";
    $Sources .= "
        int func_assemble_param(struct complex_struct p) {
            return 1;
        }";
    
    #Abstract class
    $DataDefs .= "
        class $DeclSpec abstract_class {
        public:
            abstract_class(){};
            int a;
            virtual float virt_func(float p) = 0;
            float func(float p);
        };";
    $Sources .= "
        float abstract_class::func(float p) {
            return p;
        }";
    
    #Parameter FuncPtr
    $DataDefs .= "
        typedef int (*funcptr_type)(int a, int b);
        $DeclSpec funcptr_type func_return_funcptr(int a);
        $DeclSpec int func_param_funcptr(const funcptr_type** p);";
    $Sources .= "
        funcptr_type func_return_funcptr(int a) {
            return 0;
        }
        int func_param_funcptr(const funcptr_type** p) {
            return 0;
        }";

    #Parameter FuncPtr (2)
    $DataDefs .= "
        typedef int (*funcptr_type2)(int a, int b, float c);
        $DeclSpec int func_param_funcptr2(funcptr_type2 p);";
    $Sources .= "
        int func_param_funcptr2(funcptr_type2 p) {
            return 0;
        }";
    
    #Parameter Array
    $DataDefs .= "
        $DeclSpec int func_param_array(struct complex_struct const ** x);";
    $Sources .= "
        int func_param_array(struct complex_struct const ** x) {
            return 0;
        }";
    
    #Nested classes
    $DataDefs .= "//Nested classes
        class $DeclSpec A {
        public:
            virtual bool method1() {
                return false;
            };
        };

        class $DeclSpec B: public A { };

        class $DeclSpec C: public B {
        public:
            C() { };
            virtual bool method1();
            virtual bool method2() const;
        };";
    $Sources .= "//Nested classes
        bool C::method1() {
            return false;
        };

        bool C::method2() const {
            return false;
        };";
    
    #Throw class
    $DataDefs .= "
        class $DeclSpec Exception {
        public:
            Exception();
            int a;
        };";
    $Sources .= "
        Exception::Exception() { }";
    $DataDefs .= "
        class $DeclSpec throw_class {
        public:
            throw_class() { };
            int a;
            virtual float virt_func(float p) throw(Exception) = 0;
            float func(float p);
        };";
    $Sources .= "
        float throw_class::func(float p) {
            return p;
        }";

    # Should crash
    $DataDefs .= "
        $DeclSpec int func_should_crash();";
    $Sources .= "
        int func_should_crash()
        {
            int *x = 0x0;
            *x = 1;
            return 1;
        }";
    
    runTests("libsample_cpp", "C++", "namespace TestNS {\n$DataDefs\n}\n", "namespace TestNS {\n$Sources\n}\n", "type_test_opaque", "_ZN18type_test_internal5func1ES_");
}

sub testSystem_c()
{
    print "\ntesting C library API\n";
    my ($DataDefs, $Sources)  = ();
    my $DeclSpec = ($OSgroup eq "windows")?"__declspec( dllexport )":"";
    
    # Simple parameters
    $DataDefs .= "
        $DeclSpec int func_simple_parameters(
            int a,
            float b,
            double c,
            long double d,
            long long e,
            char f,
            unsigned int g,
            const char* h,
            char* i,
            unsigned char* j,
            char** k);";
    $Sources .= "
        int func_simple_parameters(
            int a,
            float b,
            double c,
            long double d,
            long long e,
            char f,
            unsigned int g,
            const char* h,
            char* i,
            unsigned char* j,
            char** k) {
            return 1;
        }";
    
    # Initialization by interface
    $DataDefs .= "
        struct simple_struct {
            int m;
        };
        $DeclSpec struct simple_struct simple_func(int a, int b);";
    $Sources .= "
        struct simple_struct simple_func(int a, int b)
        {
            struct simple_struct x = {1};
            return x;
        }";
    
    $DataDefs .= "
        $DeclSpec int func_init_param_by_interface(struct simple_struct p);";
    $Sources .= "
        int func_init_param_by_interface(struct simple_struct p) {
            return 1;
        }";
    
    # Assembling structure
    $DataDefs .= "
        typedef struct complex_struct {
            int a;
            float b;
            struct complex_struct* c;
        } complex_struct;";
    
    $DataDefs .= "
        $DeclSpec int func_assemble_param(struct complex_struct p);";
    $Sources .= "
        int func_assemble_param(struct complex_struct p) {
            return 1;
        }";
    
    # Initialization by out parameter
    $DataDefs .= "
        struct out_opaque_struct;
        $DeclSpec void create_out_param(struct out_opaque_struct** out);";
    $Sources .= "
        struct out_opaque_struct {
            const char* str;
        };
        $DeclSpec void create_out_param(struct out_opaque_struct** out) { }\n";
    
    $DataDefs .= "
        $DeclSpec int func_init_param_by_out_param(struct out_opaque_struct* p);";
    $Sources .= "
        int func_init_param_by_out_param(struct out_opaque_struct* p) {
            return 1;
        }";
    
    # Should crash
    $DataDefs .= "
        $DeclSpec int func_should_crash();";
    $Sources .= "
        int func_should_crash()
        {
            int *x = 0x0;
            *x = 1;
            return 1;
        }";
    
    # Function with out parameter
    $DataDefs .= "
        $DeclSpec int func_has_out_opaque_param(struct out_opaque_struct* out);";
    $Sources .= "
        int func_has_out_opaque_param(struct out_opaque_struct* out) {
            return 1;
        }";

    # C++ keywords
    $DataDefs .= "
        $DeclSpec int operator();";
    $Sources .= "
        int operator() {
            
            return 1;
        }";
    
    
    runTests("libsample_c", "C", $DataDefs, $Sources, "type_test_opaque", "func_test_internal");
}

sub runTests($$$$$$)
{
    my ($LibName, $Lang, $DataDefs, $Sources, $Opaque, $Private) = @_;
    my $Ext = ($Lang eq "C++")?"cpp":"c";
    # creating test suite
    rmtree($LibName);
    mkpath($LibName);
    writeFile("$LibName/version", "TEST_1.0 {\n};\nTEST_2.0 {\n};\n");
    writeFile("$LibName/libsample.h", $DataDefs."\n");
    writeFile("$LibName/libsample.$Ext", "#include \"libsample.h\"\n".$Sources."\n");
    writeFile("$LibName/descriptor.xml", "
        <version>
            1.0
        </version>

        <headers>
            ".abs_path($LibName)."
        </headers>

        <libs>
            ".abs_path($LibName)."
        </libs>

        <opaque_types>
            $Opaque
        </opaque_types>

        <skip_symbols>
            $Private
        </skip_symbols>\n");
    my $BuildCmd = "";
    if($OSgroup eq "windows")
    {
        my $CL = get_CmdPath("cl");
        if(not $CL) {
            print STDERR "ERROR: can't find \"cl\" compiler\n";
            exit(1);
        }
        $BuildCmd = "$CL /LD libsample.$Ext >build_out 2>&1";
    }
    elsif($OSgroup eq "linux")
    {
        writeFile("$LibName/version", "VERSION_1.0 {\n};\nVERSION_2.0 {\n};\n");
        $BuildCmd = "$GCC_PATH -Wl,--version-script version -shared -fkeep-inline-functions".($Lang eq "C++"?" -x c++":"")." libsample.$Ext -o libsample.$LIB_EXT";
        if(getArch()=~/\A(arm|x86_64)\Z/i)
        { # relocation R_X86_64_32S against `vtable for class' can not be used when making a shared object; recompile with -fPIC
            $BuildCmd .= " -fPIC";
        }
    }
    elsif($OSgroup eq "macos") {
        $BuildCmd = "$GCC_PATH -dynamiclib -fkeep-inline-functions".($Lang eq "C++"?" -x c++":"")." libsample.$Ext -o libsample.$LIB_EXT";
    }
    else {
        $BuildCmd = "$GCC_PATH -shared -fkeep-inline-functions".($Lang eq "C++"?" -x c++":"")." libsample.$Ext -o libsample.$LIB_EXT";
    }
    writeFile("$LibName/Makefile", "all:\n\t$BuildCmd\n");
    system("cd $LibName && $BuildCmd");
    if($?) {
        print STDERR "ERROR: can't compile \'$LibName/libsample.$Ext\'\n";
        return;
    }
    # running the tool
    system("perl $0 -l $LibName -d $LibName/descriptor.xml -gen -build -run -show-retval");
    my ($Total, $Passed, $Failed) = (0, 0, 0);
    if(my $FLine = readFirstLine("test_results/$LibName/1.0/test_results.html")) {
        if($FLine=~/total:(\d+)/) {
            $Total = $1;
        }
        if($FLine=~/passed:(\d+)/) {
            $Passed = $1;
        }
        if($FLine=~/failed:(\d+)/) {
            $Failed = $1;
        }
    }
    if($Total==($Passed+$Failed) and (($LibName eq "libsample_c" and $Total>5 and $Failed>=1)
    or ($LibName eq "libsample_cpp" and $Total>10 and $Failed>=1))) {
        print "result: SUCCESS ($Total test cases, $Passed passed, $Failed failed)\n\n";
    }
    else {
        print STDERR "result: FAILED ($Total test cases, $Passed passed, $Failed failed)\n\n";
    }
}

sub readFirstLine($)
{
    my $Path = $_[0];
    return "" if(not $Path or not -f $Path);
    open (FILE, $Path);
    my $FirstLine = <FILE>;
    close(FILE);
    return $FirstLine;
}

sub esc($)
{
    my $Str = $_[0];
    $Str=~s/([()\[\]{}$ &'"`;,<>\+])/\\$1/g;
    return $Str;
}

sub detect_default_paths()
{
    foreach my $Type (keys(%{$OS_AddPath{$OSgroup}}))
    {# additional search paths
        foreach my $Path (keys(%{$OS_AddPath{$OSgroup}{$Type}})) {
            next if(not -d $Path);
            $SystemPaths{$Type}{$Path} = $OS_AddPath{$OSgroup}{$Type}{$Path};
        }
    }
    if($OSgroup ne "windows")
    {
        foreach my $Type ("include", "lib", "bin")
        {# autodetecting system "devel" directories
            foreach my $Path (cmd_find("/","d","*$Type*",1)) {
                $SystemPaths{$Type}{$Path} = 1;
            }
            if(-d "/usr") {
                foreach my $Path (cmd_find("/usr","d","*$Type*",1)) {
                    $SystemPaths{$Type}{$Path} = 1;
                }
            }
        }
    }
    detect_bin_default_paths();
    foreach my $Path (keys(%DefaultBinPaths)) {
        $SystemPaths{"bin"}{$Path} = $DefaultBinPaths{$Path};
    }
    # check environment variables
    if($OSgroup eq "beos")
    {
        foreach my $Path (split(/:|;/, $ENV{"BEINCLUDES"}))
        {
            if($Path=~/\A(\/|\w+:[\/\\])/) {
                $DefaultIncPaths{$Path} = 1;
            }
        }
        foreach my $Path (split(/:|;/, $ENV{"BELIBRARIES"}), split(/:|;/, $ENV{"LIBRARY_PATH"}))
        {
            if($Path=~/\A(\/|\w+:[\/\\])/) {
                $DefaultLibPaths{$Path} = 1;
            }
        }
    }
    else
    {
        my $Sep = ($OSgroup eq "windows")?";":":|;";
        foreach my $Var (keys(%ENV))
        {
            if($Var=~/INCLUDE/i) {
                foreach my $Path (split(/$Sep/, $ENV{$Var})) {
                    if($Path=~/\A(\/|\w+:[\/\\])/) {
                        $SystemPaths{"include"}{$Path} = 1;
                    }
                }
            }
            elsif($Var=~/(\ALIB)|LIBRAR(Y|IES)/i) {
                foreach my $Path (split(/$Sep/, $Var)) {
                    if($Path=~/\A(\/|\w+:[\/\\])/) {
                        $SystemPaths{"lib"}{$Path} = 1;
                    }
                }
            }
        }
    }
    detect_lib_default_paths();
    foreach my $Path (keys(%DefaultLibPaths)) {
        $SystemPaths{"lib"}{$Path} = $DefaultLibPaths{$Path};
    }
    foreach my $Path (keys(%DefaultIncPaths)) {
        $SystemPaths{"include"}{$Path} = $DefaultIncPaths{$Path};
    }
}

sub detect_gcc()
{
    $GCC_PATH = get_CmdPath("gcc");
    if(not $GCC_PATH) {
        print STDERR "ERROR: can't find GCC >=3.0 in PATH\n";
        exit(1);
    }
    $GPP_PATH = get_CmdPath("g++");
    if(not $GPP_PATH) {
        print STDERR "ERROR: can't find G++ >=3.0 in PATH\n";
        exit(1);
    }
    $CPP_FILT = get_CmdPath("c++filt");
    if(not $CPP_FILT) {
        print STDERR "ERROR: can't find c++filt in PATH\n";
        exit(1);
    }
    if(not check_gcc_version($GCC_PATH, 4))
    {# warning if GCC 3 is selected
        print "Using GCC ".get_dumpversion($GCC_PATH)."\n";
    }
    detect_include_default_paths();
}

sub get_dumpversion($)
{
    my $Cmd = $_[0];
    return "" if(not $Cmd);
    my $Version = `$Cmd -dumpversion 2>$TMP_DIR/null`;
    chomp($Version);
    return $Version;
}

sub get_version($)
{
    my $Cmd = $_[0];
    return "" if(not $Cmd);
    my $Version = `$Cmd --version 2>$TMP_DIR/null`;
    return $Version;
}

sub check_gcc_version($$)
{
    my ($Cmd, $Req) = @_;
    return 0 if(not $Cmd or not $Req);
    my $GccVersion = get_dumpversion($Cmd);
    my ($MainVer, $MinorVer, $MicroVer) = split(/\./, $GccVersion);
    if(int($MainVer)>=int($Req)) {
        return $Cmd;
    }
    return "";
}

sub remove_option($$)
{
    my ($OptionsRef, $Option) = @_;
    return if(not $OptionsRef or not $Option);
    $Option=esc($Option);
    my @Result = ();
    foreach my $Arg (@{$OptionsRef})
    {
        if($Arg!~/\A[-]+$Option\Z/) {
            push(@Result, $Arg);
        }
    }
    @{$OptionsRef} = @Result;
}

sub get_RetValName($)
{
    my $Interface = $_[0];
    return "" if(not $Interface);
    if($Interface=~/\A(.+?)(_|)(init|initialize|initializer|install)\Z/) {
        return $1;
    }
    else {
        return getParamNameByTypeName($CompleteSignature{$Interface}{"Return"});
    }
}

sub add_LibraryPreambleAndFinalization()
{
    if(not keys(%LibraryInitFunc)
    or keys(%LibraryInitFunc)>1) {
        return;
    }
    my $AddedPreamble = 0;
    my $Pos = 0;
    foreach my $Interface (sort {$Library_Prefixes{getPrefix($b)} <=> $Library_Prefixes{getPrefix($a)}}
    sort {$b=~/init/i <=> $a=~/init/i} sort {lc($a) cmp lc($b)} keys(%LibraryInitFunc))
    {
        next if(not interfaceFilter($Interface));
        my $Prefix = getPrefix($Interface);
        next if($Library_Prefixes{$Prefix}<$LIBRARY_PREFIX_MAJORITY);
        next if($Interface=~/plugin/i);
        my $ReturnId = $CompleteSignature{$Interface}{"Return"};
        my $ReturnFId = get_FoundationTypeId($ReturnId);
        my $ReturnFTypeType = get_TypeType($ReturnFId);
        my $RPLevel = get_PointerLevel($Tid_TDid{$ReturnId}, $ReturnId);
        my $RetValName = get_RetValName($Interface);
        if(defined $CompleteSignature{$Interface}{"Param"}{0})
        {# should not have a complex parameter type
            my $PTypeId = $CompleteSignature{$Interface}{"Param"}{0}{"type"};
            next if(get_TypeType(get_FoundationTypeId($PTypeId))!~/\A(Enum|Intrinsic)\Z/ and get_PointerLevel($Tid_TDid{$PTypeId}, $PTypeId)!=0);
        }
        if(get_TypeName($ReturnId) eq "void"
        or ($ReturnFTypeType=~/\A(Enum|Intrinsic)\Z/ and $RPLevel==0)
        or ($ReturnFTypeType eq "Struct" and $RPLevel>=1))
        {# should return a simple type or structure pointer
            readSpecTypes("
            <spec_type>
                <name>
                    automatic preamble
                </name>
                <kind>
                    common_env
                </kind>
                <global_code>
                    #include <".$CompleteSignature{$Interface}{"Header"}.">
                </global_code>
                <init_code>
                    \$[$Interface".($ReturnFTypeType eq "Struct" and $RetValName?":$RetValName":"")."];
                </init_code>
                <libs>
                    ".get_filename($Interface_Library{$Interface})."
                </libs>
                <associating>
                    <except>
                        $Interface
                    </except>
                </associating>
            </spec_type>");
            $AddedPreamble = 1;
            $LibraryInitFunc{$Interface} = $Pos++;
        }
    }
    if(not $AddedPreamble
    or keys(%LibraryExitFunc)>1) {
        return;
    }
    foreach my $Interface (sort {lc($a) cmp lc($b)} keys(%LibraryExitFunc))
    {
        next if(not interfaceFilter($Interface));
        my $Prefix = getPrefix($Interface);
        next if($Library_Prefixes{$Prefix}<$LIBRARY_PREFIX_MAJORITY);
        next if($Interface=~/plugin/i);
        my $ReturnId = $CompleteSignature{$Interface}{"Return"};
        my $PTypeId = (defined $CompleteSignature{$Interface}{"Param"}{0})?$CompleteSignature{$Interface}{"Param"}{0}{"type"}:0;
        my $Interface_Pair = 0;
        foreach my $Interface_Init (keys(%LibraryInitFunc))
        {# search for a pair interface
            my $Prefix_Init = getPrefix($Interface_Init);
            my $ReturnId_Init = $CompleteSignature{$Interface_Init}{"Return"};
            my $PTypeId_Init = (defined $CompleteSignature{$Interface_Init}{"Param"}{0})?$CompleteSignature{$Interface_Init}{"Param"}{0}{"type"}:0;
            if($Prefix eq $Prefix_Init
            and ($PTypeId==0 or $PTypeId_Init==$ReturnId or $PTypeId==$ReturnId_Init or $PTypeId==$PTypeId_Init))
            {# libraw_init ( unsigned int flags ):libraw_data_t*
             # libraw_close ( libraw_data_t* p1 ):void
                $Interface_Pair = $Interface_Init;
                last;
            }
        }
        next if(not $Interface_Pair);
        if(get_TypeName($ReturnId) eq "void"
        or (get_TypeType(get_FoundationTypeId($ReturnId))=~/\A(Enum|Intrinsic)\Z/ and get_PointerLevel($Tid_TDid{$ReturnId}, $ReturnId)==0))
        {
            readSpecTypes("
            <spec_type>
                <name>
                    automatic finalization
                </name>
                <kind>
                    common_env
                </kind>
                <global_code>
                    #include <".$CompleteSignature{$Interface}{"Header"}.">
                </global_code>
                <final_code>
                    \$[$Interface];
                </final_code>
                <libs>
                    ".get_filename($Interface_Library{$Interface})."
                </libs>
                <associating>
                    <except>
                        $Interface
                    </except>
                </associating>
            </spec_type>");
        }
    }
}

sub writeDebugInfo()
{
    my $DLogPath = "debug/$TargetLibraryName/".$Descriptor{"Version"}."/log.txt";
    my $DEBUG_LOG = "";
    if(my @Interfaces = keys(%{$DebugInfo{"Init_InterfaceParams"}}))
    {
        $DEBUG_LOG .= "Failed to initialize parameters of these symbols:\n";
        foreach my $Interface (@Interfaces) {
            $DEBUG_LOG .= "  ".get_Signature($Interface)."\n";
        }
        delete($DebugInfo{"Init_InterfaceParams"});
    }
    if(my @Types = keys(%{$DebugInfo{"Init_Class"}}))
    {
        $DEBUG_LOG .= "Failed to create instances for these classes:\n";
        foreach my $Type (@Types) {
            $DEBUG_LOG .= "  $Type\n";
        }
        delete($DebugInfo{"Init_Class"});
    }
    if($DEBUG_LOG) {
        appendFile($DLogPath, $DEBUG_LOG."\n");
        print "DEBUG: log has written to \'$DLogPath\'\n";
    }
    else {
        print "DEBUG: log is empty\n";
    }
}

sub scenario()
{
    if(defined $Help) {
        HELP_MESSAGE();
        exit(0);
    }
    if(defined $InfoMsg) {
        INFO_MESSAGE();
        exit(0);
    }
    if(defined $ShowVersion) {
        print "API Sanity Checker (ASC) $API_SANITY_CHECKER_VERSION\nCopyright (C) 2011 ROSA Laboratory\nLicense: LGPL or GPL <http://www.gnu.org/licenses/>\nThis program is free software: you can redistribute it and/or modify it.\n\nWritten by Andrey Ponomarenko.\n";
        exit(0);
    }
    if(defined $DumpVersion) {
        print "$API_SANITY_CHECKER_VERSION\n";
        exit(0);
    }
    if(not defined $Template2Code) {
        $Standalone = 1;
    }
    if($GenerateDescriptorTemplate) {
        genDescriptorTemplate();
        exit(0);
    }
    if($GenerateSpecTypeTemplate) {
        genSpecTypeTemplate();
        exit(0);
    }
    detect_default_paths();
    if($OSgroup eq "windows")
    {
        if(not $ENV{"DevEnvDir"}
        or not $ENV{"LIB"}) {
            print "ERROR: can't start without VS environment (vsvars32.bat)";
            exit(1);
        }
    }
    if(defined $TestSystem)
    {
        detect_gcc();
        testSystem_c();
        testSystem_cpp();
        exit(0);
    }
    if(not defined $TargetLibraryName) {
        print STDERR "\nERROR: library name is not selected (option -l <name>)\n";
        exit(1);
    }
    else {
        # validate library name
        if($TargetLibraryName=~/[\*\/\\]/) {
            print STDERR "\nERROR: \"\\\", \"\/\" and \"*\" symbols are not allowed in the library name\n";
            exit(1);
        }
    }
    if($TestDataPath and not -d $TestDataPath) {
        print STDERR "\nERROR: can't access directory \'$TestDataPath\'\n";
        exit(1);
    }
    if($SpecTypes_PackagePath and not -f $SpecTypes_PackagePath) {
        print STDERR "\nERROR: can't access file \'$SpecTypes_PackagePath\'\n";
        exit(1);
    }
    if($InterfacesListPath)
    {
        if(-f $InterfacesListPath) {
            foreach my $Interface (split(/\n/, readFile($InterfacesListPath))) {
                $InterfacesList{$Interface} = 1;
            }
        }
        else {
            print STDERR "\nERROR: can't access file \'$InterfacesListPath\'\n";
            exit(1);
        }
    }
    if(not $Descriptor{"Path"}) {
        print STDERR "ERROR: library descriptor is not selected (option -d <path>)\n";
        exit(1);
    }
    if(not $GenerateTests and not $BuildTests
    and not $RunTests and not $CleanTests and not $CleanSources) {
        print "specify appropriate option: -gen, -build, -run, -clean\n";
        exit(1);
    }
    if(not $TargetLibraryFullName) {
        $TargetLibraryFullName = $TargetLibraryName;
    }
    $TOOL_SIGNATURE = "<hr/><div style='width:100%;font-family:Arial;font-size:11px;' align='right'><i>Generated on ".(localtime time)." for <span style='font-weight:bold'>$TargetLibraryFullName</span> by <a href='".$HomePage{"Dev"}."'>API Sanity Checker</a> $API_SANITY_CHECKER_VERSION &nbsp;<br/>Unit test generator for a shared C/C++ library API&nbsp;&nbsp;</i></div>";
    if($ParameterNamesFilePath)
    {
        if(-f $ParameterNamesFilePath)
        {
            foreach my $Line (split(/\n/, readFile($ParameterNamesFilePath)))
            {
                if($Line=~s/\A(\w+)\;//)
                {
                    my $Interface = $1;
                    if($Line=~/;(\d+);/) {
                        while($Line=~s/(\d+);(\w+)//) {
                            $AddIntParams{$Interface}{$1}=$2;
                        }
                    }
                    else
                    {
                        my $Num = 0;
                        foreach my $Name (split(/;/, $Line)) {
                            $AddIntParams{$Interface}{$Num}=$Name;
                            $Num+=1;
                        }
                    }
                }
            }
        }
        else {
            print STDERR "\nERROR: can't access file \'$ParameterNamesFilePath\'\n";
            exit(1);
        }
    }
    if($TargetInterfaceName and defined $Template2Code) {
        print STDERR "\nERROR: selecting of a symbol is not supported in the Template2Code format\n";
        exit(1);
    }
    if(($BuildTests or $RunTests or $CleanTests) and defined $Template2Code
    and not defined $GenerateTests) {
        print STDERR "\nERROR: see Template2Code technology documentation for building and running tests:\n       http://template2code.sourceforge.net/t2c-doc/index.html\n";
        exit(1);
    }
    if($Strict) {
        ($StrictGen, $StrictBuild, $StrictRun) = (1, 1, 1);
    }
    if($GenerateTests)
    {
        detect_gcc();
        checkVersionNum($Descriptor{"Path"});
        foreach my $Part (split(/\s*,\s*/, $Descriptor{"Path"})) {
            createDescriptor($Part);
        }
        $TEST_SUITE_PATH = ((defined $Template2Code)?"tests_t2c":"tests")."/$TargetLibraryName/".$Descriptor{"Version"};
        my $LOG_DIR = "logs/$TargetLibraryName/".$Descriptor{"Version"};
        rmtree($LOG_DIR);
        mkpath($LOG_DIR);
        $LOG_PATH = abs_path($LOG_DIR)."/log.txt";
        if(not $CheckHeadersOnly)
        {
            print "library(ies) analysis: [0.00%]";
            getSymbols();
            print "\rlibrary(ies) analysis: [100.00%]\n";
            translateSymbols(keys(%Interface_Library));
        }
        print "header(s) analysis: [0.00%]";
        searchForHeaders();
        print "\rheader(s) analysis: [20.00%]";
        detectPointerSize();
        parseHeaders();
        print "\rheader(s) analysis: [85.00%]";
        prepareInterfaces();
        print "\rheader(s) analysis: [95.00%]";
        if($CheckHeadersOnly)
        {#detecting language
            foreach my $Interface (keys(%CompleteSignature))
            {
                $Interface_Library{$Interface}="WithoutLib";
                if($Interface=~/\A(_Z|\?)/) {
                    $Language{"WithoutLib"}="C++";
                }
            }
            $Language{"WithoutLib"}="C" if(not $Language{"WithoutLib"});
        }
        print "\rheader(s) analysis: [100.00%]\n";
        add_os_spectypes();
        if($SplintAnnotations) {
            read_annotations_Splint();
        }
        if($SpecTypes_PackagePath) {
            readSpecTypes(readFile($SpecTypes_PackagePath));
        }
        setRegularities();
        markAbstractClasses();
        if(not keys(%Common_SpecEnv))
        {# automatic preamble and finalization
            add_LibraryPreambleAndFinalization();
        }
        if($TargetInterfaceName)
        {
            if(not $CompleteSignature{$TargetInterfaceName})
            {
                print STDERR "ERROR: specified symbol is not found\n";
                if($Func_ShortName_MangledName{$TargetInterfaceName})
                {
                    if(keys(%{$Func_ShortName_MangledName{$TargetInterfaceName}})==1) {
                        print STDERR "did you mean ".(keys(%{$Func_ShortName_MangledName{$TargetInterfaceName}}))[0]." ?\n";
                    }
                    else {
                        print STDERR "candidates are:\n ".join("\n ", keys(%{$Func_ShortName_MangledName{$TargetInterfaceName}}))."\n";
                    }
                }
                exit(1);
            }
            print "generating test for $TargetInterfaceName ... ";
            read_scenario();
            generate_sanity_test($TargetInterfaceName);
            write_scenario();
            if($GenResult{$TargetInterfaceName}{"IsCorrect"}) {
                print "success\n";
            }
            else {
                print "fail\n";
            }
            create_Index();
        }
        else {
            generate_tests();
            create_Index() if(not defined $Template2Code);
        }
        if($ResultCounter{"Gen"}{"Success"}>0)
        {
            if($TargetInterfaceName) {
                my $TestPath = getTestPath($TargetInterfaceName);
                print "see generated test in \'$TestPath/\'\n";
            }
            else
            {
                if($Template2Code) {
                    print "1. see generated test suite in the directory \'$TEST_SUITE_PATH/\'\n";
                    print "2. see Template2Code technology documentation for building and running tests:\nhttp://template2code.sourceforge.net/t2c-doc/index.html\n";
                }
                else {
                    print "1. see generated test suite in the directory \'$TEST_SUITE_PATH/\'\n";
                    print "2. for viewing the tests use \'$TEST_SUITE_PATH/view_tests.html\'\n";
                    print "3. use -build option for building tests\n";
                }
            }
        }
        if($DebugMode)
        {# write debug log
            writeDebugInfo();
        }
        remove_option(\@INPUT_OPTIONS, "gen");
        remove_option(\@INPUT_OPTIONS, "generate");
    }
    if($BuildTests and $GenerateTests and defined $Standalone)
    { # allocated memory for tests generation should be returned to OS
        system("perl", $0, @INPUT_OPTIONS);# build + run
        exit($?>>8);
    }
    elsif($BuildTests and defined $Standalone)
    {
        checkVersionNum($Descriptor{"Path"});
        foreach my $Part (split(/\s*,\s*/, $Descriptor{"Path"})) {
            createDescriptor($Part);
        }
        $TEST_SUITE_PATH = "tests/$TargetLibraryName/".$Descriptor{"Version"};
        if(not -e $TEST_SUITE_PATH) {
            print STDERR "\nERROR: tests were not generated yet\n";
            exit(1);
        }
        if($TargetInterfaceName)
        {
            print "building test for $TargetInterfaceName ... ";
            read_scenario();
            build_sanity_test($TargetInterfaceName);
            if($BuildResult{$TargetInterfaceName}{"IsCorrect"})
            {
                if($BuildResult{$TargetInterfaceName}{"Warnings"}) {
                    print "success (Warnings)\n";
                }
                else {
                    print "success\n";
                }
            }
            elsif(not $BuildResult{$TargetInterfaceName}{"TestNotExists"}) {
                print "fail\n";
            }
        }
        else {
            build_tests();
        }
        if($ResultCounter{"Build"}{"Success"}>0
        and not $TargetInterfaceName and not $RunTests) {
            print "use -run option for running tests\n";
        }
        remove_option(\@INPUT_OPTIONS, "build");
        remove_option(\@INPUT_OPTIONS, "make");
    }
    if(($CleanTests or $CleanSources) and defined $Standalone)
    {
        checkVersionNum($Descriptor{"Path"});
        foreach my $Part (split(/\s*,\s*/, $Descriptor{"Path"})) {
            createDescriptor($Part);
        }
        $TEST_SUITE_PATH = "tests/$TargetLibraryName/".$Descriptor{"Version"};
        if(not -e $TEST_SUITE_PATH) {
            print STDERR "\nERROR: tests were not generated yet\n";
            exit(1);
        }
        if($TargetInterfaceName)
        {
            print "cleaning test for $TargetInterfaceName ... ";
            read_scenario();
            clean_sanity_test($TargetInterfaceName);
            print "success\n";
        }
        else {
            clean_tests();
        }
        remove_option(\@INPUT_OPTIONS, "clean") if($CleanTests);
        remove_option(\@INPUT_OPTIONS, "view-only") if($CleanSources);
        
    }
    if($RunTests and $GenerateTests and defined $Standalone)
    {#tests running requires creation of two processes, so allocated memory must be returned to the system
        system("perl", $0, @INPUT_OPTIONS);
        exit($ResultCounter{"Build"}{"Fail"}!=0 or $?>>8);
    }
    elsif($RunTests and defined $Standalone)
    {
        init_signals();
        checkVersionNum($Descriptor{"Path"});
        foreach my $Part (split(/\s*,\s*/, $Descriptor{"Path"})) {
            createDescriptor($Part);
        }
        $TEST_SUITE_PATH = "tests/$TargetLibraryName/".$Descriptor{"Version"};
        $REPORT_PATH = "test_results/$TargetLibraryName/".$Descriptor{"Version"};
        if(not -e $TEST_SUITE_PATH) {
            print STDERR "\nERROR: tests were not generated yet\n";
            exit(1);
        }
        if($OSgroup eq "windows") {
            createTestRunner();
        }
        my $ErrCode = 0;
        if($TargetInterfaceName)
        {
            read_scenario();
            my $XvfbStarted = 0;
            if($UseXvfb and (-f $Interface_TestDir{$TargetInterfaceName}."/test"
            or -f $Interface_TestDir{$TargetInterfaceName}."/test.exe")) {
                $XvfbStarted = runXvfb();
            }
            print "running test for $TargetInterfaceName ... ";
            $ErrCode = run_sanity_test($TargetInterfaceName);
            stopXvfb($XvfbStarted) if($UseXvfb);
            if($RunResult{$TargetInterfaceName}{"IsCorrect"})
            {
                if($RunResult{$TargetInterfaceName}{"Warnings"}) {
                    print "success (Warnings)\n";
                }
                else {
                    print "success\n";
                }
            }
            elsif(not $RunResult{$TargetInterfaceName}{"TestNotExists"}) {
                print "fail (".get_problem_title($RunResult{$TargetInterfaceName}{"Type"}, $RunResult{$TargetInterfaceName}{"Value"}).")\n";
            }
        }
        else {
            $ErrCode = run_tests();
        }
        mkpath($REPORT_PATH);
        if((not $TargetInterfaceName or not $RunResult{$TargetInterfaceName}{"TestNotExists"})
        and keys(%Interface_TestDir) and not $ErrCode)
        {
            unlink($REPORT_PATH."/test_results.html");# removing old report
            print "creating report ...\n";
            translateSymbols(keys(%RunResult));
            create_HtmlReport();
            print "see test results in the file:\n  $REPORT_PATH/test_results.html\n";
        }
        exit($ResultCounter{"Run"}{"Fail"}!=0);
    }
    exit($ResultCounter{"Build"}{"Fail"}!=0);
}

scenario();
