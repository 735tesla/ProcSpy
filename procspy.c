#define _GNU_SOURCE
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <dlfcn.h>
#include <sys/socket.h>
#include <arpa/inet.h>

typedef FILE* (*fopen_type)(const char *restrict, const char *restrict);
typedef int (*connect_type)(int, const struct sockaddr*, socklen_t);
typedef ssize_t (*send_type)(int, const void*, size_t, int);
typedef ssize_t (*sendto_type)(int, const void*, size_t, int, const struct sockaddr*, socklen_t);

static fopen_type orig_fopen = fopen;
static connect_type orig_connect = connect;
static send_type orig_send = send;
static sendto_type orig_sendto = sendto;

#ifdef __APPLE__
	#define DYLD_INTERPOSE(_evil, _good) __attribute__((used)) \
		static struct { \
			const void * good; \
			const void * evil; \
		} _interpose_##_good \
		__attribute__(\
			(section ("__DATA,__interpose") )\
			) = { \
				(const void*)(unsigned long)&_evil, \
				(const void*)(unsigned long)&_good\
			};
#endif

void hexsummary(char *desc, const void *buf, int len) {
	int i;
    unsigned char buff[17];
    unsigned char *pc = (unsigned char*)buf;
    if (desc != NULL)
        printf ("%s:\n", desc);
    for (i = 0; (i < len && i < 512); i++) {
        if ((i % 16) == 0) {
            if (i != 0)
                printf ("  %s\n", buff);
            printf ("  %04x ", i);
        }
        printf (" %02x", pc[i]);
        if ((pc[i] < 0x20) || (pc[i] > 0x7e))
            buff[i % 16] = '.';
        else
            buff[i % 16] = pc[i];
        buff[(i % 16) + 1] = '\0';
    }
    while ((i % 16) != 0) {
        printf ("   ");
        i++;
    }
    printf ("  %s\n", buff);
}

char *straddr(const struct sockaddr *addr) {
	static char str[INET6_ADDRSTRLEN];
	if (addr->sa_family == AF_INET6)
		inet_ntop(addr->sa_family, &((struct sockaddr_in6*)addr)->sin6_addr, str, sizeof(str));
	else if (addr->sa_family == AF_INET)
		inet_ntop(addr->sa_family, &((struct sockaddr_in*)addr)->sin_addr.s_addr, str, sizeof(str));
	return str;
}

#ifdef __linux
FILE * fopen(const char * filepath, const char *restrict mode) {
#elif __APPLE__
FILE * _new_fopen(const char * filepath, const char *restrict mode) {
#endif
    printf("[*] Opening filed \"%s\" with mode %s\n", filepath, mode);
    return orig_fopen(filepath, mode);
}

#ifdef __linux
int connect(int socket, const struct sockaddr *address, socklen_t address_len) {
#elif __APPLE__
int _new_connect(int socket, const struct sockaddr *address, socklen_t address_len) {
#endif
	if (address->sa_family == AF_INET6 || address->sa_family == AF_INET)
        printf("[*] Connect to %s:%d\n", straddr(address), ntohs(((struct sockaddr_in*)address)->sin_port));
    return orig_connect(socket, address, address_len);
}

#ifdef __linux
ssize_t send(int socket, const void *buffer, size_t length, int flags) {
#elif __APPLE__
ssize_t _new_send(int socket, const void *buffer, size_t length, int flags) {
#endif
	socklen_t addr_len = sizeof(struct sockaddr);
	struct sockaddr addr;
	getpeername(socket, &addr, &addr_len);
	if (addr.sa_family != AF_INET && addr.sa_family != AF_INET6)
		return orig_send(socket, buffer, length, flags);
	printf("send %s:%d :\n", straddr(&addr), ntohs(((struct sockaddr_in*)&addr)->sin_port));
	hexsummary(NULL, buffer, length);
	return orig_send(socket, buffer, length, flags);
}

#ifdef __linux
ssize_t sendto(int socket, const void *buffer, size_t length, int flags, const struct sockaddr *dest_addr, socklen_t dest_len) {
#elif __APPLE__
ssize_t _new_sendto(int socket, const void *buffer, size_t length, int flags, const struct sockaddr *dest_addr, socklen_t dest_len) {
#endif
	if (!dest_addr)
		return sendto(socket, buffer, length, flags, dest_addr, dest_len);
	printf("sendto %s:%d :\n", straddr(dest_addr), ntohs(((struct sockaddr_in*)dest_addr)->sin_port));
	hexsummary(NULL, buffer, length);
	return orig_sendto(socket, buffer, length, flags, dest_addr, dest_len);
}

#ifdef __linux
pid_t fork(void) {
#elif __APPLE__
pid_t _new_fork(void) {
#endif
	pid_t ret = fork();
	printf("[*] fork() == %d\n", ret);
	return ret;
}

#ifdef __APPLE__
	DYLD_INTERPOSE(_new_fopen, fopen);
	DYLD_INTERPOSE(_new_connect, connect);
	DYLD_INTERPOSE(_new_send, send);
	DYLD_INTERPOSE(_new_sendto, sendto);
	DYLD_INTERPOSE(_new_fork, fork);
#elif __linux
	__attribute__((constructor))
	void init(void) {
		orig_fopen = (fopen_type)dlsym(RTLD_NEXT, "fopen");
		orig_connect = (connect_type)dlsym(RTLD_NEXT, "connect");
		orig_send = (send_type)dlsym(RTLD_NEXT, "send");
		orig_sendto = (sendto_type)dlsym(RTLD_NEXT, "sendto");
	}
#endif
