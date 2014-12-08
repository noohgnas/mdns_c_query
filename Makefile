include_dir = -I./apple_mdns/mDNSShared/ 

ifneq "$(wildcard /usr/lib/libSystem.dylib)" ""
else
LIBS = -L./lib -ldns_sd
endif

build_flags = -g -Wall -pedantic -std=gnu99 $(include_dir) $(LIBS)
# There's a bunch of harmless warns in the apple MDNS code, but it muddys the
# build terminal, so I'm turning them off =3
dep_build_flags = -Wno-unused-but-set-variable -Wno-unused-value -Wno-unused-function -Wno-array-bounds -Wno-unused-label

# all: get_deps clean c_query
all: clean c_query

build: c_query

c_query: build_deps quick

quick: main.o dns_common.o
	gcc $(build_flags) bin/main.o -o c_query

main.o: src/main.c 
	gcc $(build_flags) -c src/main.c -o bin/main.o

dns_common.o: apple_mdns/mDNSCore/DNSCommon.c
	gcc $(build_flags) -c apple_mdns/mDNSCore/DNSCommon.c -o bin/dns_common.o

clean:
	rm -rf bin/*.o c_query mDNSResponder*.tar.gz

# 
#  Get dependancies from the apple open souce server, and build them 
#
get_deps:
	wget http://opensource.apple.com/tarballs/mDNSResponder/mDNSResponder-320.10.80.tar.gz -O mDNSResponder.tar.gz
	tar -xvzf mDNSResponder.tar.gz mDNSResponder-320.10.80/
	rm -rf apple_mdns
	mv mDNSResponder-320.10.80 apple_mdns

build_deps: 
	cd apple_mdns/mDNSPosix && $(MAKE) os=linux clean
	cd apple_mdns/mDNSPosix && $(MAKE) os=linux all CC="@cc $(dep_build_flags)"
	cp apple_mdns/mDNSPosix/build/prod/libdns_sd.so lib/
