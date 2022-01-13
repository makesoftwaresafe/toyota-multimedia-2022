Soup-2.4.gir: Soup_2_4_gir_list 
	@-echo Generating $@...
	$(PYTHON) $(G_IR_SCANNER)	\
	--verbose -no-libtool	\
	--namespace=Soup	\
	--nsversion=2.4	\
		\
	--library=soup-2.4	\
		\
	--add-include-path=$(G_IR_INCLUDEDIR)	\
	--include=Gio-2.0	\
	--pkg-export=libsoup-2.4	\
	--cflags-begin	\
	-DG_LOG_DOMAIN="libsoup" -DLIBSOUP_COMPILATION -I../.. -DLIBSOUP_USE_UNSTABLE_REQUEST_API	\
	--cflags-end	\
	--c-include "libsoup/soup.h"	\
	--filelist=Soup_2_4_gir_list	\
	-o $@

Soup-2.4.typelib: Soup-2.4.gir
	@-echo Compiling $@...
	$(G_IR_COMPILER)	\
	--includedir=. --debug --verbose	\
	Soup-2.4.gir	\
	-o $@

SoupGNOME-2.4.gir: SoupGNOME_2_4_gir_list Soup-2.4.gir
	@-echo Generating $@...
	$(PYTHON) $(G_IR_SCANNER)	\
	--verbose -no-libtool	\
	--namespace=SoupGNOME	\
	--nsversion=2.4	\
		\
	--library=soup-gnome-2.4 --library=soup-2.4	\
		\
	--add-include-path=$(G_IR_INCLUDEDIR)	\
		\
	--pkg-export=libsoup-gnome-2.4	\
	--cflags-begin	\
	-DG_LOG_DOMAIN="libsoup" -DLIBSOUP_COMPILATION -I../..	\
	--cflags-end	\
	i --identifier-prefix=Soup --symbol-prefix=soup --c-include "libsoup/soup-gnome.h" --include-uninstalled=./Soup-2.4.gir	\
	--filelist=SoupGNOME_2_4_gir_list	\
	-o $@

SoupGNOME-2.4.typelib: SoupGNOME-2.4.gir
	@-echo Compiling $@...
	$(G_IR_COMPILER)	\
	--includedir=. --debug --verbose	\
	SoupGNOME-2.4.gir	\
	-o $@

