#define __LIBTHECORE__
#include "stdafx.h"

#ifdef __WIN32__
void signal_setup() {}
void signal_timer_disable() {}
void signal_timer_enable(int timeout_seconds) {}
#elif defined(__FreeBSD__) || defined(__linux__)
#define RETSIGTYPE void

#ifdef __linux__
#include <execinfo.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static void crashsig(int sig)
{
    void* frames[64];
    char head[160];
    int n = backtrace(frames, 64);
    int len = snprintf(head, sizeof(head), "=== fatal signal %d (%s), %d frames, pid %d ===\n",
                       sig, strsignal(sig), n, (int) getpid());
    int fd = open("crash.txt", O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd >= 0)
    {
        if (write(fd, head, len) < 0) {}
        backtrace_symbols_fd(frames, n, fd);
        close(fd);
    }
    if (write(2, head, len) < 0) {}
    backtrace_symbols_fd(frames, n, 2);
    signal(sig, SIG_DFL);
    raise(sig);
}
#endif

RETSIGTYPE reap(int sig)
{
    while (waitpid(-1, NULL, WNOHANG) > 0);
    signal(SIGCHLD, reap);
}


RETSIGTYPE checkpointing(int sig)
{
    if (!tics)
    {
        sys_err("CHECKPOINT shutdown: tics did not updated.");
        if (bCheckpointCheck)
            abort();
    }
    else
		tics = 0;
}


RETSIGTYPE hupsig(int sig)
{
    shutdowned = TRUE;
    sys_log(0, "SIGHUP, SIGINT, SIGTERM signal has been received. shutting down."); // @warme012
}

RETSIGTYPE usrsig(int sig)
{
    core_dump();
}

void signal_timer_disable(void)
{
    struct itimerval itime;
    struct timeval interval;

    interval.tv_sec	= 0;
    interval.tv_usec	= 0;

    itime.it_interval = interval;
    itime.it_value = interval;

    setitimer(ITIMER_VIRTUAL, &itime, NULL);
}

void signal_timer_enable(int sec)
{
    struct itimerval itime;
    struct timeval interval;

    interval.tv_sec	= sec;
    interval.tv_usec	= 0;

    itime.it_interval = interval;
    itime.it_value = interval;

    setitimer(ITIMER_VIRTUAL, &itime, NULL);
}

void signal_setup(void)
{
    signal_timer_enable(30);

    signal(SIGVTALRM, checkpointing);

    /* just to be on the safe side: */
    signal(SIGHUP, hupsig);
    signal(SIGCHLD, reap);
    signal(SIGINT, hupsig);
    signal(SIGTERM, hupsig);
    signal(SIGPIPE, SIG_IGN);
    signal(SIGALRM, SIG_IGN);
    signal(SIGUSR1, usrsig);
#ifdef __linux__
    signal(SIGSEGV, crashsig);
    signal(SIGBUS, crashsig);
    signal(SIGFPE, crashsig);
    signal(SIGILL, crashsig);
    signal(SIGABRT, crashsig);
#endif
}

#endif
//martysama0134's 4e4e75d8b719b9240e033009cf4d7b0f
