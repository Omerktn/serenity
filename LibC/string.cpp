#include "string.h"
#include "errno.h"
#include "stdio.h"

extern "C" {

void* memset(void* dest, int c, size_t n)
{
    byte* bdest = (byte*)dest;
    for (; n; --n)
        *(bdest++) = c;
    return dest;
}

size_t strspn(const char* s, const char* accept)
{
    const char* p = s;
cont:
    char ch = *p++;
    char ac;
    for (const char* ap = accept; (ac = *ap++) != '\0';) {
        if (ac == ch)
            goto cont;
    }
    return p - 1 - s;
}

size_t strcspn(const char* s, const char* reject)
{
    for (auto* p = s;;) {
        char c = *p++;
        auto* rp = reject;
        char rc;
        do {
            if ((rc = *rp++) == c)
                return p - 1 - s;
        } while(rc);
    }
}

size_t strlen(const char* str)
{
    size_t len = 0;
    while (*(str++))
        ++len;
    return len;
}

int strcmp(const char* s1, const char* s2)
{
    for (; *s1 == *s2; ++s1, ++s2) {
        if (*s1 == 0)
            return 0;
    }
    return *(const unsigned char*)s1 < *(const unsigned char*)s2 ? -1 : 1;
}

int memcmp(const void* v1, const void* v2, size_t n)
{
    auto* s1 = (const byte*)v1;
    auto* s2 = (const byte*)v2;
    while (n-- > 0) {
        if (*s1++ != *s2++)
            return s1[-1] < s2[-1] ? -1 : 1;
    }
    return 0;
}

void memcpy(void* dest, const void* src, size_t n)
{
    auto* bdest = (unsigned char*)dest;
    auto* bsrc = (const unsigned char*)src;
    for (; n; --n)
        *(bdest++) = *(bsrc++);
}

char* strcpy(char* dest, const char *src)
{
    char* originalDest = dest;
    while ((*dest++ = *src++) != '\0');
    return originalDest;
}

char* strncpy(char* dest, const char* src, size_t n)
{
    size_t i;
    for (i = 0; i < n && src[i] != '\0'; ++i)
        dest[i] = src[i];
    for ( ; i < n; ++i)
        dest[i] = '\0';
    return dest;
}

char* strchr(const char* str, int c)
{
    if (!str)
        return nullptr;
    char* ptr = (char*)str;
    while (*ptr && *ptr != c)
        ++ptr;
    return ptr;
}

char* strrchr(const char* str, int ch)
{
    char *last = nullptr;
    char c;
    for (; (c = *str); ++str) {
        if (c == ch)
            last = (char*)str;
    }
    return last;
}

char* strcat(char *dest, const char *src)
{
    size_t destLength = strlen(dest);
    size_t i;
    for (i = 0 ; src[i] != '\0' ; i++)
        dest[destLength + i] = src[i];
    dest[destLength + i] = '\0';
    return dest;
}

char* strncat(char *dest, const char *src, size_t n)
{
    size_t destLength = strlen(dest);
    size_t i;
    for (i = 0 ; i < n && src[i] != '\0' ; i++)
        dest[destLength + i] = src[i];
    dest[destLength + i] = '\0';
    return dest;
}

const char* strerror(int errnum)
{
    switch (errnum) {
    case 0: return "No error";
    case EPERM: return "Operation not permitted";
    case ENOENT: return "No such file or directory";
    case ESRCH: return "No such process";
    case EINTR: return "Interrupted syscall";
    case EIO: return "I/O error";
    case ENXIO: return "No such device/address";
    case E2BIG: return "Argument list too long";
    case ENOEXEC: return "Exec format error";
    case EBADF: return "Bad fd number";
    case ECHILD: return "No child processes";
    case EAGAIN: return "Try again";
    case ENOMEM: return "Out of memory";
    case EACCES: return "Access denied";
    case EFAULT: return "Bad address";
    case ENOTBLK: return "Not a block device";
    case EBUSY: return "Resource busy";
    case EEXIST: return "File already exists";
    case EXDEV: return "Cross-device link";
    case ENODEV: return "No such device";
    case ENOTDIR: return "Not a directory";
    case EISDIR: return "Is a directory";
    case EINVAL: return "Invalid argument";
    case ENFILE: return "File table overflow";
    case EMFILE: return "Too many open files";
    case ENOTTY: return "Not a TTY";
    case ETXTBSY: return "Text file busy";
    case EFBIG: return "File too big";
    case ENOSPC: return "No space left";
    case ESPIPE: return "Illegal seek";
    case EROFS: return "File system is read-only";
    case EMLINK: return "Too many links";
    case EPIPE: return "Broken pipe";
    case EDOM: return "Math argument out of domain";
    case ERANGE: return "Math result not representable";
    case ENAMETOOLONG: return "Name too long";
    case EOVERFLOW: return "Value too large for data type";
    case ENOTIMPL: return "Not implemented";
    }
    printf("strerror() missing string for errnum=%d\n", errnum);
    return "Unknown error";
}

}

