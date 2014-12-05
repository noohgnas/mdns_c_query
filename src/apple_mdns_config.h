#ifndef __apple_mdns_config_h_
#define __apple_mdns_config_h_

#define NOT_HAVE_SA_LEN
#define TEST_NEW_CLIENTSTUB 0
#define SA_LEN(addr) (((addr)->sa_family == AF_INET6)? sizeof(struct sockaddr_in6) : sizeof(struct sockaddr_in))
#define __APPLE_API_PRIVATE 1
#define HAS_NAT_PMP_API 1
#define HAS_ADDRINFO_API 1
// #define kDNSServiceFlagsReturnIntermediates 0

#endif // __apple_mdns_config_h_
