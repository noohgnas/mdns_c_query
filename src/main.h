#ifndef __main_h_
#define __main_h_

#define TypeBufferSize 80

enum ERROR_CODE {
	NO_ERROR = 0
}; 

int main(int argc, char * argv[]);
void sig_int(int sig);
static void HandleEvents(void);
void printDNSResults(DNSServiceRef sdRef, DNSServiceFlags flags,
                     uint32_t interfaceIndex, DNSServiceErrorType errorCode,
                     const char *serviceName, const char *regtype,
                     const char *replyDomain, void *context);
void resolve_reply(DNSServiceRef sdref, const DNSServiceFlags flags, 
                   uint32_t ifIndex, DNSServiceErrorType errorCode, 
                   const char *fullname, const char *hosttarget, 
                   uint16_t opaqueport, uint16_t txtLen, 
                   const unsigned char *txtRecord, void *context);
static void ShowTXTRecord(uint16_t txtLen, const unsigned char *txtRecord);

#endif // __main_h_