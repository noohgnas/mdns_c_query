#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Configuration defines for the apple DNS libraries.
#include "apple_mdns_config.h"

// Apple mDNS Libraries & helper functions
#include <dns_sd.h>

#include "main.h"


#define LONG_TIME 180
static volatile int timeOut = LONG_TIME;
static volatile uint8_t stopNow = 0;

// Service clients, one for discovery one for resolution
DNSServiceRef client_disc, client_resl;

int main(int argc, char * argv[])
{
	signal(SIGINT, sig_int);
	DNSServiceErrorType err = 0;

	printf("c_query started.\n");

	err = DNSServiceBrowse(&client_disc, 0, 0, "_xtag._tcp", NULL, printDNSResults, NULL);
	
	if (err) {
		fprintf(stderr, "DNSServiceBrowse failed %ld\n", (long int)err); 	
		fprintf(stderr, "The program will now exit.\n");
		exit(err);
	}

	HandleEvents();
	
	printf("Done.\n");
	return err;
}

void sig_int(int sig) 
{
  stopNow = 1;
}

static void HandleEvents()
{
	fd_set readfds;
	struct timeval tv;
	int result;
	

	while (stopNow == 0)
	{
		// If the client is set, grab the socket FD from it, else set the fd to -1
		int dns_sd_fd  = client_disc ? DNSServiceRefSockFD(client_disc) : -1;
		int dns_sd_fd2 = client_resl ? DNSServiceRefSockFD(client_resl) : -1;
		int nfds = dns_sd_fd + 1;
		if (dns_sd_fd2 > dns_sd_fd) nfds = dns_sd_fd2 + 1;
		
		// 1. Set up the fd_set as usual here.
		// This example client has no file descriptors of its own,
		// but a real application would call FD_SET to add them to the set here
		FD_ZERO(&readfds);

		// 2. Add the fd for our client(s) to the fd_set
		if (client_disc) FD_SET(dns_sd_fd , &readfds);
		if (client_resl) FD_SET(dns_sd_fd2, &readfds);

		// 3. Set up the timeout.
		tv.tv_sec  = timeOut;
		tv.tv_usec = 0;

		result = select(nfds, &readfds, (fd_set*)NULL, (fd_set*)NULL, &tv);
		if (result > 0)
		{
			DNSServiceErrorType err = kDNSServiceErr_NoError;
			if (client_disc && FD_ISSET(dns_sd_fd , &readfds)) err = DNSServiceProcessResult(client_disc);
			if (err) goto Fail;

			if (client_resl && FD_ISSET(dns_sd_fd2, &readfds)) err = DNSServiceProcessResult(client_resl);
			if (err) goto Fail;
			else continue;

			Fail:
			// Something failed, print the err and bail out
			fprintf(stderr, "DNSServiceProcessResult returned %d\n", err); 
			stopNow = 1; 
		}
		else
		{
			// The socket select failed, print the err and bail out
			printf("select() returned %d errno %d %s\n", result, errno, strerror(errno));
			if (errno != EINTR) stopNow = 1;
		}
	}
}

void printDNSResults(DNSServiceRef sdRef, DNSServiceFlags flags,
                     uint32_t interfaceIndex, DNSServiceErrorType errorCode,
                     const char *serviceName, const char *regtype,
                     const char *replyDomain, void *context)
{
	printf("Name: %-45s    regtype: %-20s    replyDomain: %s\n", 
		serviceName, regtype, replyDomain);
	DNSServiceResolve(&client_resl, 0, interfaceIndex, serviceName, regtype, replyDomain, resolve_reply, NULL);
}

void resolve_reply(DNSServiceRef sdRef, const DNSServiceFlags flags, uint32_t ifIndex, DNSServiceErrorType errorCode,
	const char *fullname, const char *hosttarget, uint16_t opaqueport, uint16_t txtLen, const unsigned char *txtRecord, void *context)
{
	union { uint16_t s; u_char b[2]; } port = { opaqueport };
	uint16_t PortAsNumber = ((uint16_t)port.b[0]) << 8 | port.b[1];

	(void)sdRef;        // Unused
	(void)context;      // Unused

	if (errorCode)
		printf("Error code %d\n", errorCode);
	else
	{
		printf("%s can be reached at %s:%u (interface %d) and has %d TXT records", 
			fullname, hosttarget, PortAsNumber, ifIndex, txtLen);
		if (flags) printf(" Flags: %X", flags);
		// Don't show degenerate TXT records containing nothing but a single empty string
		if (txtLen > 1) { printf("\n"); ShowTXTRecord(txtLen, txtRecord); }
		printf("\n\n");
		fflush(stdout);
	}

	if (!(flags & kDNSServiceFlagsMoreComing)) fflush(stdout);
	DNSServiceRefDeallocate(sdRef);
}

static void ShowTXTRecord(uint16_t txtLen, const unsigned char *txtRecord)
{
	const unsigned char *ptr = txtRecord;
	const unsigned char *max = txtRecord + txtLen;
	while (ptr < max)
	{
		const unsigned char *const end = ptr + 1 + ptr[0];
		if (end > max) { printf("<< invalid data >>"); break; }
		if (++ptr < end) printf(" ");   // As long as string is non-empty, begin with a space
		while (ptr<end)
		{
			if (strchr(" &;`'\"|*?~<>^()[]{}$", *ptr)) printf("\\");
			if      (*ptr == '\\') printf("\\\\\\\\");
			else if (*ptr >= ' ' ) printf("%c",        *ptr);
			else                   printf("\\\\x%02X", *ptr);
			ptr++;
		}
	}
}
