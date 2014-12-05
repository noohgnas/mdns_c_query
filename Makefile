include_dir = -I./apple_mdns/mDNSShared/ 

ifneq "$(wildcard /usr/lib/libSystem.dylib)" ""
else
LIBS = -L./lib -ldns_sd
endif

build_flags = -g -Wall -pedantic -std=gnu99 $(include_dir) $(LIBS)

all: get_deps clean c_query

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
	cd apple_mdns/mDNSPosix && $(MAKE) os=linux all
	cp apple_mdns/mDNSPosix/build/prod/libdns_sd.so lib/
