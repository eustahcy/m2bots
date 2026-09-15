#ifndef __INC_LIBTHECORE_SIGNAL_H__
#define __INC_LIBTHECORE_SIGNAL_H__

/* Linux: glibc's <sys/signal.h> is "#include <signal.h>", which the
 * -I../include on every compile resolves to this file. Resume the search
 * past this directory so the real one is read; _SIGNAL_H is its guard. */
#if defined(__linux__) && !defined(_SIGNAL_H)
#include_next <signal.h>
#endif

#ifdef __cplusplus
extern "C"
{
#endif
    extern void signal_setup();
    extern void signal_timer_disable();
    extern void signal_timer_enable(int timeout_seconds);

#ifdef __cplusplus
}
#endif

#endif
//martysama0134's 4e4e75d8b719b9240e033009cf4d7b0f
