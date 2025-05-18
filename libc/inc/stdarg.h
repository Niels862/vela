#ifndef __VEMU_STDARG_H__
#define __VEMU_STDARG_H__

typedef __builtin_va_list va_list;

#define va_start(ap, last_arg) __builtin_va_start(ap, last_arg)
#define va_arg(ap, type)       __builtin_va_arg(ap, type)
#define va_end(ap)             __builtin_va_end(ap)
#define va_copy(dest, src)     __builtin_va_copy(dest, src)

#endif
