
EXTRA_DIST = \
	COPYRIGHT \
	HISTORY \
	lua.hpp \
	local/lom.lua \
	local/llocal.lua

AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-I$(srcdir)/local \
	-I$(builddir)/local \
	@WITH_PCRE_CPPFLAGS@
if USE_LUA_CHKCONFIG
AM_CPPFLAGS += \
	-I$(srcdir)/chkconfig \
	-I$(builddir)/chkconfig \
	$(WITH_POPT_CPPFLAGS)
endif
if USE_LUA_SHADOW
AM_CPPFLAGS += \
	-I$(srcdir)/shadow \
	-I$(builddir)/shadow
endif
AM_CPPFLAGS += \
	-I$(srcdir)

pkgbindir =	@USRLIBRPM@/bin
pkgbin_PROGRAMS =	rpmlua rpmluac
noinst_PROGRAMS =	bin2c

pkglibdir =	@USRLIBRPM@/lib
pkglib_LTLIBRARIES =	liblua.la

rpmlua_SOURCES = lua.c
rpmlua_CFLAGS =
rpmlua_LDADD = liblua.la
rpmluac_SOURCES = luac.c
rpmluac_CFLAGS =
rpmluac_LDADD = liblua.la

# --- bin2c doesn't need anything but a compiler
bin2c$(EXEEXT):  bin2c.c
	$(CC) -o $@ $<

liblua_la_SOURCES =
liblua_la_CFLAGS = @WITH_LUA_SUBDIR_DEF@

if USE_LUA_CRYPTO
liblua_la_CFLAGS += \
	-DCRYPTO_OPENSSL
liblua_la_SOURCES += \
	local/lcrypto.c \
	local/lcrypto.h \
	local/lxplib.c \
	local/lxplib.h
else
EXTRA_DIST += \
	local/lcrypto.c \
	local/lcrypto.h \
	local/lxplib.c \
	local/lxplib.h
endif

if USE_LUA_SOCKET
liblua_la_SOURCES += \
	socket/auxiliar.c \
	socket/auxiliar.h \
	socket/buffer.c \
	socket/buffer.h \
	socket/except.c \
	socket/except.h \
	socket/ftp.lua \
	socket/http.lua \
	socket/inet.c \
	socket/inet.h \
	socket/io.c \
	socket/io.h \
	socket/ltn12.lua \
	socket/luasocket.c \
	socket/luasocket.h \
	socket/mime.c \
	socket/mime.h \
	socket/mime.lua \
	socket/options.c \
	socket/options.h \
	socket/select.c \
	socket/select.h \
	socket/smtp.lua \
	socket/socket.h \
	socket/socket.lua \
	socket/tcp.c \
	socket/tcp.h \
	socket/timeout.c \
	socket/timeout.h \
	socket/tp.lua \
	socket/udp.c \
	socket/udp.h \
	socket/unix.c \
	socket/unix.h \
	socket/url.lua \
	socket/usocket.c \
	socket/usocket.h \
	socket/wsocket.c \
	socket/wsocket.h
else
EXTRA_DIST += \
	socket/auxiliar.c \
	socket/auxiliar.h \
	socket/buffer.c \
	socket/buffer.h \
	socket/except.c \
	socket/except.h \
	socket/ftp.lua \
	socket/http.lua \
	socket/inet.c \
	socket/inet.h \
	socket/io.c \
	socket/io.h \
	socket/ltn12.lua \
	socket/luasocket.c \
	socket/luasocket.h \
	socket/mime.c \
	socket/mime.h \
	socket/mime.lua \
	socket/options.c \
	socket/options.h \
	socket/select.c \
	socket/select.h \
	socket/smtp.lua \
	socket/socket.h \
	socket/socket.lua \
	socket/tcp.c \
	socket/tcp.h \
	socket/timeout.c \
	socket/timeout.h \
	socket/tp.lua \
	socket/udp.c \
	socket/udp.h \
	socket/unix.c \
	socket/unix.h \
	socket/url.lua \
	socket/usocket.c \
	socket/usocket.h \
	socket/wsocket.c \
	socket/wsocket.h
endif

if USE_LUA_SHADOW
liblua_la_CFLAGS += \
	-DUSE_LUA_SHADOW
liblua_la_SOURCES += \
	shadow/chkname.h \
	shadow/commonio.h \
	shadow/commonio.c \
	shadow/shadow_config.h \
	shadow/copydir.c \
	shadow/defines.h \
	shadow/faillog.h \
	shadow/fputsx.c \
	shadow/getdef.h \
	shadow/getdef.c \
	shadow/groupadd.c \
	shadow/groupio.h \
	shadow/groupio.c \
	shadow/gshadow_.h \
	shadow/gshadow.c \
	shadow/list.c \
	shadow/nscd.h \
	shadow/prototypes.h \
	shadow/pwauth.h \
	shadow/pwio.h \
	shadow/pwio.c \
	shadow/sgroupio.h \
	shadow/sgroupio.c \
	shadow/sgetgrent.c \
	shadow/sgetpwent.c \
	shadow/shadowio.h \
	shadow/shadowio.c \
	shadow/strtoday.c \
	shadow/useradd.c
else
EXTRA_DIST += \
	shadow/chkname.h \
	shadow/commonio.c \
	shadow/commonio.h \
	shadow/copydir.c \
	shadow/defines.h \
	shadow/faillog.h \
	shadow/fputsx.c \
	shadow/getdef.c \
	shadow/getdef.h \
	shadow/groupadd.c \
	shadow/groupio.c \
	shadow/groupio.h \
	shadow/gshadow.c \
	shadow/gshadow_.h \
	shadow/list.c \
	shadow/nscd.h \
	shadow/prototypes.h \
	shadow/pwauth.h \
	shadow/pwio.c \
	shadow/pwio.h \
	shadow/sgetgrent.c \
	shadow/sgetpwent.c \
	shadow/sgroupio.c \
	shadow/sgroupio.h \
	shadow/shadow_config.h \
	shadow/shadowio.c \
	shadow/shadowio.h \
	shadow/strtoday.c \
	shadow/useradd.c
endif
if USE_LUA_CHKCONFIG
liblua_la_CFLAGS += \
	-DUSE_LUA_CHKCONFIG
liblua_la_SOURCES += \
	chkconfig/chkconfig.c \
	chkconfig/leveldb.h \
	chkconfig/leveldb.c
else
EXTRA_DIST += \
	chkconfig/chkconfig.c \
	chkconfig/leveldb.c \
	chkconfig/leveldb.h
endif

#	local/lshadow.h
#	local/lshadow.c
#	local/lwrs.h
#	local/lwrs.c

liblua_la_LIBADD = @WITH_LUA_SUBDIR_LIB@ -lm -lexpat
liblua_la_SOURCES += \
	local/llocal.c \
	local/llocal.h \
	local/lposix.c \
	local/lposix.h \
	local/lua52compat.h \
	local/lrexlib.h \
	local/lrexlib_algo.h \
	local/lrexlib_common.h \
	local/lrexlib_common.c \
	local/lrexlib_lpcre.c \
	local/lrexlib_lpcre_f.c \
	local/lrexlib_lposix.c \
	local/luuid.c \
	local/luuid.h

liblua_la_SOURCES += \
	linit.c \
	lauxlib.c \
	lauxlib.h \
	lua.h \
	luaconf.h \
	lualib.h \
	lbaselib.c \
	lbitlib.c \
	lcorolib.c \
	lctype.c \
	lctype.h \
	ldblib.c \
	liolib.c \
	loslib.c \
	lmathlib.c \
	loadlib.c \
	lstrlib.c \
	ltablib.c \
	lapi.c \
	lapi.h \
	lcode.c \
	lcode.h \
	ldebug.c \
	ldebug.h \
	ldo.c \
	ldo.h \
	ldump.c \
	lfunc.c \
	lfunc.h \
	lgc.c \
	lgc.h \
	llex.c \
	llex.h \
	llimits.h \
	lmem.c \
	lmem.h \
	lobject.c \
	lobject.h \
	lopcodes.c \
	lopcodes.h \
	lparser.c \
	lparser.h \
	lstate.c \
	lstate.h \
	lstring.c \
	lstring.h \
	ltable.c \
	ltable.h \
	ltm.c \
	ltm.h \
	lundump.c \
	lundump.h \
	lvm.c \
	lvm.h \
	lzio.c \
	lzio.h
liblua.la: $(liblua_la_OBJECTS) $(liblua_la_DEPENDENCIES) 
	$(liblua_la_LINK) $(liblua_la_OBJECTS) $(liblua_la_LIBADD)
	-ln -sf ../liblua.la .libs/liblua.lai		#XXX HACK ALERT

$(builddir)/local/llocal.c: $(builddir)/local/llocal_lua.c

if USE_LUA_CRYPTO
$(builddir)/local/llocal_lua.c: $(builddir)/bin2c $(srcdir)/local/llocal.lua $(srcdir)/local/lom.lua
	mkdir -p $(builddir)/local
	$(builddir)/bin2c $(srcdir)/local/llocal.lua $(srcdir)/local/lom.lua > $(builddir)/local/llocal_lua.c
else
$(builddir)/local/llocal_lua.c: $(builddir)/bin2c $(srcdir)/local/llocal.lua
	mkdir -p $(builddir)/local
	$(builddir)/bin2c $(srcdir)/local/llocal.lua > $(builddir)/local/llocal_lua.c
endif

clean-local:
	-rm -f $(builddir)/local/llocal_lua.c

# XXX Build & install as rpmlua/rpmluac with hardlinks to lua/luac post install.
# XXX CVS has lua/luac sub-directories in the Attic that collide with the
# XXX lua/luac executable names when using cvs update.
install-exec-hook:	install-pkgbinPROGRAMS
	mkdir -p $(DESTDIR)$(pkgbindir)
	$(__RM) -f $(DESTDIR)$(pkgbindir)/lua
	$(__LN) $(DESTDIR)$(pkgbindir)/rpmlua $(DESTDIR)$(pkgbindir)/lua
	$(__RM) -f $(DESTDIR)$(pkgbindir)/luac
	$(__LN) $(DESTDIR)$(pkgbindir)/rpmluac $(DESTDIR)$(pkgbindir)/luac
