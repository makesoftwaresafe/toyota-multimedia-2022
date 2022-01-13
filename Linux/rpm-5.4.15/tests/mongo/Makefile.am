## Process this file with automake to produce Makefile.in.

AUTOMAKE_OPTIONS = 1.4 foreign

EXTRA_DIST =	tutorial.js

EXTRA_PROGRAMS = \
	all_types \
	bcon \
	benchmark \
	json \
	pair \
	replica_set \
	replset

noinst_PROGRAMS = \
	auth \
	bson \
	bson_alloc \
	bson_subobject \
	commands \
	connect \
	count_delete \
	cursors \
	endian_swap \
	env_posix \
	errors \
	examples \
	functions \
	gridfs \
	helpers \
	oid \
	resize \
	simple \
	sizes \
	unix_socket \
	update \
	validate \
	write_concern

AM_CPPFLAGS = \
	-I$(srcdir) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/rpmdb \
	-I$(top_srcdir)/rpmio

RPMIO_LDADD = \
	$(top_builddir)/rpmio/librpmio.la \
	$(top_builddir)/misc/librpmmisc.la \
	@LTLIBINTL@

#RPMDB_LDADD = \
#	$(top_builddir)/rpmdb/librpmdb.la \
#	$(RPMIO_LDADD)

#RPM_LDADD = \
#	$(top_builddir)/build/librpmbuild.la \
#	$(top_builddir)/lib/librpm.la \
#	$(RPMDB_LDADD)

mongo =			@__MONGO@
mongoserver =		198.49.244.159	# XXX mongo.rpm5.org?

all_types_SOURCES =	test.h all_types.c
all_types_LDADD =	$(RPMIO_LDADD)

auth_SOURCES =		test.h auth.c
auth_LDADD =		$(RPMIO_LDADD)

bcon_SOURCES =		test.h bcon.c
bcon_LDADD =		$(RPMIO_LDADD)

benchmark_SOURCES =	test.h benchmark.c
benchmark_LDADD =	$(RPMIO_LDADD)

bson_SOURCES =		test.h bson.c
bson_LDADD =		$(RPMIO_LDADD)

bson_alloc_SOURCES =	test.h bson_alloc.c
bson_alloc_LDADD =	$(RPMIO_LDADD)

bson_subobject_SOURCES = test.h bson_subobject.c
bson_subobject_LDADD =	$(RPMIO_LDADD)

commands_SOURCES =	test.h commands.c
commands_LDADD =	$(RPMIO_LDADD)

connect_SOURCES =	test.h connect.c
connect_LDADD =		$(RPMIO_LDADD)

count_delete_SOURCES =	test.h count_delete.c
count_delete_LDADD =	$(RPMIO_LDADD)

cursors_SOURCES =	test.h cursors.c
cursors_LDADD =		$(RPMIO_LDADD)

endian_swap_SOURCES =	test.h endian_swap.c
endian_swap_LDADD =	$(RPMIO_LDADD)

env_posix_SOURCES =	test.h env_posix.c
env_posix_LDADD =	$(RPMIO_LDADD)

errors_SOURCES =	test.h errors.c
errors_LDADD =		$(RPMIO_LDADD)

examples_SOURCES =	test.h examples.c
examples_LDADD =	$(RPMIO_LDADD)

functions_SOURCES =	test.h functions.c
functions_LDADD =	$(RPMIO_LDADD)

gridfs_SOURCES =	test.h gridfs.h gridfs.c
gridfs_LDADD =		$(RPMIO_LDADD)

helpers_SOURCES =	test.h helpers.c
helpers_LDADD =		$(RPMIO_LDADD)

json_SOURCES =		test.h json.c
json_LDADD =		$(RPMIO_LDADD)

oid_SOURCES =		test.h oid.c
oid_LDADD =		$(RPMIO_LDADD)

pair_SOURCES =		test.h pair.c
pair_LDADD =		$(RPMIO_LDADD)

replica_set_SOURCES =	test.h replica_set.c
replica_set_LDADD =	$(RPMIO_LDADD)

replset_SOURCES =	test.h replset.c
replset_LDADD =		$(RPMIO_LDADD)

resize_SOURCES =	test.h resize.c
resize_LDADD =		$(RPMIO_LDADD)

simple_SOURCES =	test.h simple.c
simple_LDADD =		$(RPMIO_LDADD)

sizes_SOURCES =		test.h sizes.c
sizes_LDADD =		$(RPMIO_LDADD)

unix_socket_SOURCES =	test.h unix_socket.c
unix_socket_LDADD =	$(RPMIO_LDADD)

update_SOURCES =	test.h update.c
update_LDADD =		$(RPMIO_LDADD)

validate_SOURCES =	test.h validate.c
validate_LDADD =	$(RPMIO_LDADD)

write_concern_SOURCES =	test.h write_concern.c
write_concern_LDADD =	$(RPMIO_LDADD)

check-shell:
	@echo "=== mongo-shell ==="
	@-${mongo} ${mongoserver} < tutorial.js

check-local:	check-shell
	@echo "=== mongo-c-driver ==="
#	@echo "--> all_types"
#	@-./all_types
	@echo "--> auth"
	@-./auth $(mongoserver)
#	@echo "--> bcon"
#	@-./bcon $(mongoserver)
#	@echo "--> benchmark"
#	@-./benchmark $(mongoserver)
	@echo "--> bson"
	@-./bson $(mongoserver)
	@echo "--> bson_alloc"
	@-./bson_alloc $(mongoserver)
	@echo "--> bson_subobject"
	@-./bson_subobject $(mongoserver)
	@echo "--> commands"
	@-./commands $(mongoserver)
	@echo "--> connect"
	@-./connect $(mongoserver)
	@echo "--> count_delete"
	@-./count_delete $(mongoserver)
	@echo "--> cursors"
	@-./cursors $(mongoserver)
	@echo "--> endian_swap"
	@-./endian_swap
	@echo "--> env_posix"
	@-./env_posix
	@echo "--> errors"
	@-./errors $(mongoserver)
	@echo "--> examples"
	@-./examples $(mongoserver)
	@echo "--> functions"
	@-./functions $(mongoserver)
	@echo "--> gridfs"
	@-./gridfs $(mongoserver)
	@echo "--> helpers"
	@-./helpers $(mongoserver)
#	@echo "--> json"
#	@-./json
	@echo "--> oid"
	@-./oid $(mongoserver)
#	@echo "--> pair"
#	@-./pair $(mongoserver)
#	@echo "--> replica_set"
#	@-./replica_set $(mongoserver)
#	@echo "--> replset"
#	@-./replset
	@echo "--> resize"
	@-./resize
	@echo "--> simple"
	@-./simple $(mongoserver)
	@echo "--> sizes"
	@-./sizes
	@echo "--> unix_socket"
	@-./unix_socket $(mongoserver)
	@echo "--> update"
	@-./update $(mongoserver)
	@echo "--> validate"
	@-./validate $(mongoserver)
	@echo "--> write_concern"
	@-./write_concern $(mongoserver)

distclean-local:
	rm -f input-file output
