include_dir = -I./apple_mdns/mDNSShared/ 

ifneq "$(wildcard /usr/lib/libSystem.dylib)" ""
else
LIBS = -L./lib -ldns_sd
endif

build_flags = -g -Wall -pedantic -std=gnu99 $(include_dir) $(LIBS)

all: clean c_query

build: c_query

c_query: lib_dns_sd main.o dns_common.o
	gcc $(build_flags) bin/main.o -o c_query

main.o: src/main.c 
	gcc $(build_flags) -c src/main.c -o bin/main.o

dns_common.o: apple_mdns/mDNSCore/DNSCommon.c
	gcc $(build_flags) -c apple_mdns/mDNSCore/DNSCommon.c -o bin/dns_common.o

lib_dns_sd: 
	# cd apple_mdns/mDNSPosix && $(MAKE) os=linux clean
	# cd apple_mdns/mDNSPosix && $(MAKE) os=linux all
	cp apple_mdns/mDNSPosix/build/prod/libdns_sd.so lib/

clean:
	rm -rf bin/*.o c_query
