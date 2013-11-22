#include <stdarg.h>

#if defined (__cplusplus)
extern "C"
#endif
void verbose (int level, const char *format, ...);

extern int DebugLevel;