// Minimal C library for tests: the string functions GCC and ported code
// (e.g. Kyber, Dilithium) call, and small printf-family functions.
//
// Formatting supports: %d %i %u %x %X %c %s %p %%, the flags '-' and '0', a
// field width, and the length modifiers l, ll and z.
// printf/fprintf/vprintf write through putchar(); stdout and stderr both go
// to the program output. snprintf/vsnprintf write to a buffer.

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/reent.h>

// newlib's stdio.h expands stdout/stderr to fields of *_impure_ptr. The
// streams are never used (fprintf ignores its stream), so a zeroed struct is
// enough to make code like fprintf(stderr, ...) link.
static struct _reent reent;
struct _reent *_impure_ptr = &reent;

// GCC can turn these loops into calls to the functions themselves (infinite
// recursion), so that optimization is switched off for them.
#define NO_LIBCALL __attribute__((optimize("no-tree-loop-distribute-patterns")))

NO_LIBCALL void *memcpy(void *dst, const void *src, size_t n) {
  uint8_t *d = dst;
  const uint8_t *s = src;
  while (n--) {
    *d++ = *s++;
  }
  return dst;
}

NO_LIBCALL void *memmove(void *dst, const void *src, size_t n) {
  uint8_t *d = dst;
  const uint8_t *s = src;
  if (d < s) {
    while (n--) {
      *d++ = *s++;
    }
  } else {
    while (n--) {
      d[n] = s[n];
    }
  }
  return dst;
}

NO_LIBCALL void *memset(void *dst, int c, size_t n) {
  uint8_t *d = dst;
  while (n--) {
    *d++ = (uint8_t)c;
  }
  return dst;
}

int memcmp(const void *a, const void *b, size_t n) {
  const uint8_t *x = a, *y = b;
  for (; n; n--, x++, y++) {
    if (*x != *y) {
      return *x - *y;
    }
  }
  return 0;
}

size_t strlen(const char *s) {
  size_t n = 0;
  while (s[n]) {
    n++;
  }
  return n;
}

// ------------------------------------------------------------ formatting ---
// Output goes to a sink: putchar() for the console, otherwise into buf (which
// may be NULL with size 0), keeping room for the terminating NUL. count is the full length, as returned
// by printf/snprintf (also when the buffer was too small).
typedef struct {
  int console;
  char *buf;
  size_t size;
  int count;
} sink_t;

static void out(sink_t *s, char c) {
  if (s->console) {
    putchar(c);
  } else if ((size_t)s->count + 1 < s->size) {
    s->buf[s->count] = c;
  }
  s->count++;
}

static void pad(sink_t *s, int n, char c) {
  for (int i = 0; i < n; i++) {
    out(s, c);
  }
}

// Writes str (len chars) in a field of width, padded with fill (left or right)
static void put_field(sink_t *s, const char *str, int len, int width, int left,
                      char fill) {
  if (!left) {
    pad(s, width - len, fill);
  }
  for (int i = 0; i < len; i++) {
    out(s, str[i]);
  }
  if (left) {
    pad(s, width - len, ' ');
  }
}

static void put_number(sink_t *s, unsigned long long v, int neg, unsigned base,
                       int upper, int width, int left, int zero) {
  char buf[24];
  int i = sizeof(buf);
  const char *digits = upper ? "0123456789ABCDEF" : "0123456789abcdef";
  do {
    buf[--i] = digits[v % base];
    v /= base;
  } while (v);
  int len = sizeof(buf) - i;
  if (neg) {
    if (zero && !left) {  // sign before the zero padding: -0042
      out(s, '-');
      width--;
    } else {
      buf[--i] = '-';
      len++;
    }
  }
  put_field(s, &buf[i], len, width, left, zero && !left ? '0' : ' ');
}

static void format(sink_t *s, const char *fmt, va_list ap) {
  for (; *fmt; fmt++) {
    if (*fmt != '%') {
      out(s, *fmt);
      continue;
    }
    fmt++;
    int left = 0, zero = 0, width = 0, lng = 0;
    for (;; fmt++) {
      if (*fmt == '-') {
        left = 1;
      } else if (*fmt == '0') {
        zero = 1;
      } else {
        break;
      }
    }
    while (*fmt >= '0' && *fmt <= '9') {
      width = width * 10 + (*fmt++ - '0');
    }
    while (*fmt == 'l' || *fmt == 'z') {
      lng += (*fmt == 'z') ? (sizeof(size_t) == sizeof(long long) ? 2 : 1) : 1;
      fmt++;
    }
    switch (*fmt) {
      case 'd':
      case 'i': {
        long long v = lng >= 2 ? va_arg(ap, long long)
                    : lng == 1 ? va_arg(ap, long)
                               : va_arg(ap, int);
        unsigned long long u = v < 0 ? -(unsigned long long)v : (unsigned long long)v;
        put_number(s, u, v < 0, 10, 0, width, left, zero);
        break;
      }
      case 'u':
      case 'x':
      case 'X': {
        unsigned long long v = lng >= 2 ? va_arg(ap, unsigned long long)
                             : lng == 1 ? va_arg(ap, unsigned long)
                                        : va_arg(ap, unsigned int);
        put_number(s, v, 0, *fmt == 'u' ? 10 : 16, *fmt == 'X', width, left, zero);
        break;
      }
      case 'p':
        put_field(s, "0x", 2, 0, 0, ' ');
        put_number(s, (uintptr_t)va_arg(ap, void *), 0, 16, 0, 8, 0, 1);
        break;
      case 'c': {
        char c = (char)va_arg(ap, int);
        put_field(s, &c, 1, width, left, ' ');
        break;
      }
      case 's': {
        const char *str = va_arg(ap, const char *);
        if (!str) {
          str = "(null)";
        }
        put_field(s, str, strlen(str), width, left, ' ');
        break;
      }
      case '%':
        out(s, '%');
        break;
      case '\0':
        return;
      default:  // unsupported: print it as is
        out(s, '%');
        out(s, *fmt);
        break;
    }
  }
}

int vprintf(const char *fmt, va_list ap) {
  sink_t s = {1, NULL, 0, 0};
  format(&s, fmt, ap);
  return s.count;
}

int printf(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int n = vprintf(fmt, ap);
  va_end(ap);
  return n;
}

// The stream is ignored: stdout and stderr both go to the program output
int vfprintf(FILE *stream, const char *fmt, va_list ap) {
  (void)stream;
  return vprintf(fmt, ap);
}

int fprintf(FILE *stream, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int n = vfprintf(stream, fmt, ap);
  va_end(ap);
  return n;
}

int vsnprintf(char *buf, size_t size, const char *fmt, va_list ap) {
  sink_t s = {0, buf, size, 0};
  format(&s, fmt, ap);
  if (size) {
    buf[(size_t)s.count < size ? (size_t)s.count : size - 1] = '\0';
  }
  return s.count;
}

int snprintf(char *buf, size_t size, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int n = vsnprintf(buf, size, fmt, ap);
  va_end(ap);
  return n;
}
