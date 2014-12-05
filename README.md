# Example mDNS project

I want to be able to advertise a service on a network and be able to find a
headless device without knowing it's IP address or hostname, and no sane way to
get it.

For this the obvious tool for the job is mDNS. mDNS is multicast dns, and it's
the system behind the Bonjour service. It allows you to send a multicast dns
broadcast saying to the LAN "I am called foo, and I provide the service bar". It
also listens for things along the lines of "I'm looking for a foo, or something
providing bar"

The packets are all dns packets, so the standard is a nice thin layer of
multicast things, and some standardization of service type formats, etc.


## Bonjour

Bonjour is Apple's mDNS stack. It consists of an mDNS responder, and mDNS
service discovery platform. It's released under the Apache 2.0 license, and is
cross platform for UNIX, Posix, and Windows -- i.e. basically everything.

There is a backend daemon (or windows service) which is queried through their
API. The reason for having a backend, is that it can be a "polite citezen" of a
LAN and cache mDNS entries it sees flying about on multicast, and other similar
things to reduce network load. This is especially nice if you're scared the
local Cisco routers might not like you sending 100s of multicast packets every
time you open an application.


## Using the Apple mDNS Libraries.
### Install
The exact setup for the libraries will vary by platform. For me it wasn't too
bad, I downloaded the tar from Apple's website, built the daemon component with.
I'm going to do all shell commands relative to the top director of the Apple
mdns library from now on.

```shell
cd Clients
make os=linux all
```

Which helpfully also builds a cute tool for doing mdns queries in
`Clients/build/dns_sd`. It also builds the daemon and library to link to.

### Daemon
You will need to start the daemon on your machine to do any querying, so I
suggest you do that 1st.
```shell
sudo ./mDNSPosix/mdnsd.sh start
```

This is an `init.d` style script, which would be installed with `make install`
if I understand correctly. Depending on what you're doing, it might be sane to
do a `make install` and just make assumptions about the library's presence, but
I'm not keen on the idea, and prefer to keep tabs on it myself.

### Shared Library and Makefile
When the library was built earlier, a `.so` file was built, for linking to
applications calling the the mDNS library. This file is at
`mDNSPosix/build/prod/libdns_sd.so`

Adding this to your Make script isn't too hard. I have it set up such that the
Apple mdns source is inside my project directory, and have the following in my
project's Makefile to build the libs and copy the output .so to a local folder.
```makefile
lib_dns_sd:
    # You might want to comment out the make lines, or have some directives to
    # skip building these every time you rebuild your application.
    cd apple_mdns/mDNSPosix && $(MAKE) os=linux clean
    cd apple_mdns/mDNSPosix && $(MAKE) os=linux all
    cp apple_mdns/mDNSPosix/build/prod/libdns_sd.so lib/
```

### Making an mDNS call

The documentation on the API is very good for specific functions and what they
do, but goes into far less depth about how to fit them together. Much of this
code has been adapted from the `dns_sd` example application Apple include with
the libraries. The source of which can be found [here][dns_sd]

I did this in C, as I'm more comfortable with C than C++, but it should apply
fine to both.

You'll need to link your binary to the `libdns_sd.so` file which was built
earlier. I have a Makefile which looks like this.
```makefile
# ld flags to add the libs directory and then link the libdns_sd.so file
LIBS = -L./lib -ldns_sd

# Include the folder with the dns_sd.h header
include_dir = -I./apple_mdns/mDNSShared/

# Build flags: debug, warn all, gnu99, include dirs and link flags
build_flags = -g -Wall -std=gnu99 $(include_dir) $(LIBS)
## ...
c_query: lib_dns_sd main.o dns_common.o
    gcc $(build_flags) bin/main.o -o c_query
```

With the `libdns_sd.so` file in a local libs folder, you'll need to tell ld
where to find it, in order to run your application.

```shell
# Add the lib dir relative to the current directory to the LD_LIBARY_PATH
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:./lib
```

And now to write some code...

```c
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Configuration defines for the apple DNS libraries.
#include "apple_mdns_config.h"

// Apple mDNS Libraries & helper functions
#include <dns_sd.h>

#include "main.h"

DNSServiceRef client;

int main(int argc, char * argv[])
{
    DNSServiceErrorType err = 0;

    printf("c_query started.\n");

    // Run a query looking for workstations on the LAN.
    err = DNSServiceBrowse(&client, 0, 0, "_workstation._tcp", NULL, printDNSResults, NULL);
    if(err){
         fprintf(stderr, "DNSServiceBrowse failed %ld\n", (long int)err);
         fprintf(stderr, "The program will now exit.\n");
         exit(-1);
    }

    HandleEvents();

    printf("Done.\n");
    return err;
}
```
<!--- \_ fixes a bug in atom -->

This code fragment isn't quite complete. The `DNSServiceBrowse()` call is the
key thing to note. It communicates with the daemon, and provides it with a
pointer to a function to call with each result it finds which matches the query.

My implementation of printDNSResults is nice and simple:
```c
void printDNSResults(DNSServiceRef sdRef, DNSServiceFlags flags,
                     uint32_t interfaceIndex, DNSServiceErrorType errorCode,
                     const char * serviceName, const char * regtype,
                     const char * replyDomain, void * context)
{
    printf("Name: %-45s    regtype: %-20s    replyDomain: %s\n",
                  serviceName,      regtype,              replyDomain);
}
```

The full documentation for this specific call is a bit easier to follow that
what I have to say, so see [here][DNSServiceBrowse].

You might have also spotted the `HandleEvents()` call. The apple dns library
doesn't call our callback function for us (which struck me odd, having done lots
of callback-things with C#.NET). To deal with this, we're going to write the
`HandleEvents` function.

```c
static void HandleEvents()
{
    // If the client is set, grab the socket FD from it, else set the fd to -1
    int dns_sd_fd  = client ? DNSServiceRefSockFD(client) : -1;
    int nfds = dns_sd_fd + 1;
    fd_set readfds;
    struct timeval tv;
    int result;


    while (stopNow == 0)
    {
        // 1. Set up the fd_set as usual here.
        // This example client has no file descriptors of its own,
        // but a real application would call FD_SET to add them to the set here
        FD_ZERO(&readfds);

        // 2. Add the fd for our client(s) to the fd_set
        if (client)
            FD_SET(dns_sd_fd, &readfds);

        // 3. Set up the timeout.
        tv.tv_sec  = timeOut;
        tv.tv_usec = 0;

        result = select(nfds, &readfds, (fd_set*)NULL, (fd_set*)NULL, &tv);
        if (result > 0)
        {
            DNSServiceErrorType err = kDNSServiceErr_NoError;
            if (client && FD_ISSET(dns_sd_fd , &readfds)) {
                // This will call our callback function
                err = DNSServiceProcessResult(client);
            }
            if (err) {
                // Something failed, print the err and bail out
                fprintf(stderr, "DNSServiceProcessResult returned %d\n", err);
                stopNow = 1;
            }
        }
        else
        {
            // The socket select failed, print the err and bail out
            printf("select() returned %d errno %d %s\n", result, errno, strerror(errno));
            if (errno != EINTR) stopNow = 1;
        }
    }
}
```
There's nothing too fancy going on here. Lots of C defensive coding. We select
on a socket (which has the effect of blocking until the daemon finds something
matching our query), then call `DNSServiceProcessResult(client);` to run our
callback function on the found service.

With a bit more glue, which should be moderately obvious, we have an application
which outputs things like this:

```
$ ./c_query
c_query started.
Name: plague [00:1e:0b:bc:80:28]          regtype: _workstation._tcp.    replyDomain: local.
Name: camille [00:22:19:8a:c5:22]         regtype: _workstation._tcp.    replyDomain: local.
Name: fawkes [9c:b6:54:f4:03:07]          regtype: _workstation._tcp.    replyDomain: local.
Name: ghost [18:03:73:bc:2c:80]           regtype: _workstation._tcp.    replyDomain: local.
Name: frankenstein [00:1f:5b:33:2b:d8]    regtype: _workstation._tcp.    replyDomain: local.
Name: plasma [18:03:73:ba:2d:39]          regtype: _workstation._tcp.    replyDomain: local.
Name: madbeggar [a4:1f:72:5b:4b:fe]       regtype: _workstation._tcp.    replyDomain: local.
Name: cataclysm [34:17:eb:dc:fe:ec]       regtype: _workstation._tcp.    replyDomain: local.
```


<!--- References: -->
[dns_sd]: http://opensource.apple.com/source/mDNSResponder/mDNSResponder-333.10/Clients/dns-sd.c
[DNSServiceBrowse]: https://developer.apple.com/library/mac/documentation/Networking/Reference/DNSServiceDiscovery_CRef/index.html#//apple_ref/doc/c_ref/DNSServiceBrowse "Apple Documentation on DNSServiceBrowse()"
