#
# Makefile
#

all:
	python certdata2pem.py

clean:
	-rm -f *.crt

install:
	for p in *.crt; do \
	 install -m 644 $$p $(CERTSDIR)/$$p ; \
	done
