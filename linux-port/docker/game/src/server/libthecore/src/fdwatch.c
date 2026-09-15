#define __LIBTHECORE__
#include "stdafx.h"

#ifndef __USE_SELECT__

/* ### LINUX-BLOCK-BEGIN (fdwatch.c) ##################################### */
#if defined(__linux__)
/* ===========================================================================
 * Linux backend - epoll(7).
 *
 * This is a re-implementation of the kqueue backend that follows it further
 * down this file.  It must be behaviour-compatible with it, because game/ and
 * db/ drive their whole main loop through this API:
 *
 *     n = fdwatch(fdw, timeout);
 *     for (i = 0; i < n; ++i) {
 *         d = fdwatch_get_client_data(fdw, i);
 *         switch (fdwatch_check_event(fdw, d->socket, i)) {
 *             case FDW_READ: ... case FDW_WRITE: ... case FDW_EOF: ...
 *             default: close the connection;      <-- note this
 *         }
 *     }
 *
 * (game/src/main.cpp:1054-1122 and db/src/ClientManager.cpp:3200-3241.)
 * The "default: close the connection" arm makes the contract strict: every
 * index in [0, n) must answer with exactly one of FDW_READ / FDW_WRITE /
 * FDW_EOF for the descriptor it belongs to, or a live connection gets torn
 * down.  The four places where kqueue and epoll genuinely differ, and how
 * each is bridged, are flagged with "MAPPING:" comments below.
 *
 * MAPPING 1 - one event per filter vs. one event per descriptor.
 *   kqueue queues EVFILT_READ and EVFILT_WRITE separately, so a socket that
 *   is both readable and writable yields two struct kevents.  epoll_wait()
 *   returns a single struct epoll_event whose .events is a bitmask.  fdwatch()
 *   therefore expands each epoll_event into up to two FDWEVENTs (one per
 *   direction) in fdw->fdwrevents, which is the analogue of kqrevents.
 *
 * MAPPING 2 - per-descriptor user data.
 *   kqueue looks the client pointer up as fd_data[kevent.ident].  We keep
 *   exactly that and store the descriptor itself in epoll_event.data.fd, so
 *   no pointer is ever handed to the kernel and no stale pointer can come
 *   back after a descriptor is recycled.
 *
 * MAPPING 3 - EV_EOF / EV_ERROR.
 *   kqueue sets EV_EOF on the read filter once the peer has shut down its
 *   writing half, and fdwatch_check_event() answers FDW_EOF for it before it
 *   even looks at the filter.  EPOLLRDHUP is the exact counterpart, and
 *   EPOLLERR/EPOLLHUP correspond to EV_ERROR / a full hangup.
 *
 * MAPPING 4 - EV_ONESHOT.
 *   EPOLLONESHOT is *not* usable: it disarms the whole registration, i.e. the
 *   read side too, whereas kqueue's EV_ONESHOT only removes the EVFILT_WRITE
 *   knote.  fdwatch() drops EPOLLOUT by hand instead - see there.
 *
 * One deliberate structural difference: kqueue batches its change list into
 * fdw->kqevents and only flushes it on the next kevent() call, while
 * epoll_ctl() is its own syscall and is issued immediately.  That is
 * observationally equivalent for callers - the event array handed out by the
 * previous fdwatch() call is untouched either way (which is why
 * fdwatch_clear_event() still exists to neutralise entries mid-iteration) -
 * and it is strictly safer, since a descriptor that is closed disappears from
 * the epoll set on its own, whereas a queued EV_DELETE for a closed
 * descriptor turns into an error entry on FreeBSD.
 * ========================================================================= */

/* Translate the software watch state of one descriptor into an epoll mask. */
static unsigned int fdwatch_desired_mask(LPFDWATCH fdw, socket_t fd)
{
    unsigned int mask = 0;

    if (fdw->fd_rw[fd] & FDW_READ)
    {
	/* MAPPING 3: EPOLLRDHUP is the epoll counterpart of EV_EOF on
	 * EVFILT_READ.  Without asking for it a half-closed peer would only
	 * ever surface as "readable, recv() returns 0", which is not what the
	 * callers of this API were written against. */
	mask |= EPOLLIN | EPOLLRDHUP;
    }

    if (fdw->fd_rw[fd] & FDW_WRITE)
	mask |= EPOLLOUT;

    /* Note: EPOLLERR and EPOLLHUP are always reported by the kernel and must
     * not (and need not) be requested here. */
    return mask;
}

/* Reconcile the kernel-side registration of fd with 'mask'. */
static void fdwatch_apply(LPFDWATCH fdw, socket_t fd, unsigned int mask)
{
    struct epoll_event ev;
    int op;

    if (mask == fdw->fd_mask[fd])
	return;

    memset(&ev, 0, sizeof(ev));
    ev.events = mask;
    ev.data.fd = fd;			/* MAPPING 2 */

    if (mask == 0)
	op = EPOLL_CTL_DEL;
    else if (fdw->fd_mask[fd] == 0)
	op = EPOLL_CTL_ADD;
    else
	op = EPOLL_CTL_MOD;

    if (epoll_ctl(fdw->ep, op, fd, (op == EPOLL_CTL_DEL) ? NULL : &ev) < 0)
    {
	if (errno == EEXIST)
	{
	    /* Our bookkeeping lost track of an existing registration (can
	     * happen if a descriptor number was recycled without
	     * fdwatch_del_fd()); repair it rather than leak it. */
	    epoll_ctl(fdw->ep, EPOLL_CTL_MOD, fd, &ev);
	}
	else if (errno != ENOENT && errno != EBADF)
	{
	    /* ENOENT / EBADF simply mean the descriptor is already gone - the
	     * kernel removes closed descriptors by itself. */
	    sys_err("epoll_ctl(op %d, fd %d): %s", op, fd, strerror(errno));
	}
    }

    fdw->fd_mask[fd] = mask;
}

/* kqueue's EVFILT_WRITE reports sbspace(), the room left in the socket send
 * buffer, in kevent.data; fdwatch_get_buffer_size() hands that straight to the
 * callers (game/src/desc.cpp:390, db/src/PeerBase.cpp:206), which use it to cap
 * how much they push into socket_write().  epoll carries no such number, so
 * derive an equivalent one.
 *
 * Deliberately conservative: reporting *more* room than the kernel really has
 * makes socket_write() (socket.c:111-142) spin - socket_write_tcp() returns 0
 * on EAGAIN, so "total -= 0" loops without progress - whereas reporting less
 * only delays a few bytes until the next writable event. */
static int fdwatch_sndbuf_left(socket_t fd)
{
    int sndbuf = 0;
    int queued = 0;
    socklen_t optlen = sizeof(sndbuf);

    if (getsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char *) &sndbuf, &optlen) < 0)
	return 0;

    /* SIOCOUTQ: payload bytes written but not yet acknowledged. */
    if (ioctl(fd, SIOCOUTQ, &queued) < 0)
	return 0;

    /* Linux reports SO_SNDBUF as twice the usable size, the second half being
     * kernel bookkeeping overhead.  Halving it gives a payload figure
     * comparable to what FreeBSD's sbspace() returns. */
    sndbuf /= 2;

    return (sndbuf > queued) ? (sndbuf - queued) : 0;
}

/* Append one expanded event; the analogue of a struct kevent landing in
 * kqrevents.  Capacity is nfiles*2, and epoll_wait() is capped at nfiles
 * events each expanding to at most two, so this cannot overflow - the guard is
 * belt and braces. */
static void fdwatch_push_event(LPFDWATCH fdw, socket_t fd, int filter, int flags, int data)
{
    int idx = fdw->nfdwrevents;

    if (idx >= fdw->nfiles * 2)
	return;

    fdw->fdwrevents[idx].ident  = fd;
    fdw->fdwrevents[idx].filter = filter;
    fdw->fdwrevents[idx].flags  = flags;
    fdw->fdwrevents[idx].data   = data;

    /* Same side table the kqueue branch builds: it lets
     * fdwatch_get_buffer_size(fdw, fd) find this descriptor's write event
     * without being told the index. */
    if (filter == FDW_WRITE)
	fdw->fd_event_idx[fd] = idx;

    ++fdw->nfdwrevents;
}

LPFDWATCH fdwatch_new(int nfiles)
{
    LPFDWATCH fdw;
    int ep;

    ep = epoll_create1(EPOLL_CLOEXEC);

    if (ep == -1)
    {
	sys_err("%s", strerror(errno));
	return NULL;
    }

    CREATE(fdw, FDWATCH, 1);

    fdw->ep = ep;
    fdw->nfiles = nfiles;
    fdw->nfdwrevents = 0;

    CREATE(fdw->epevents, struct epoll_event, nfiles);
    CREATE(fdw->fdwrevents, FDWEVENT, nfiles * 2);
    CREATE(fdw->fd_event_idx, int, nfiles);
    CREATE(fdw->fd_rw, int, nfiles);
    CREATE(fdw->fd_data, void*, nfiles);
    CREATE(fdw->fd_mask, unsigned int, nfiles);

    return (fdw);
}

void fdwatch_delete(LPFDWATCH fdw)
{
    if (!fdw)
	return;

    if (fdw->ep != -1)
	close(fdw->ep);

    free(fdw->fd_data);
    free(fdw->fd_rw);
    free(fdw->fd_mask);
    free(fdw->epevents);
    free(fdw->fdwrevents);
    free(fdw->fd_event_idx);
    free(fdw);
}

int fdwatch(LPFDWATCH fdw, struct timeval *timeout)
{
    int i, r, timeout_ms;

    if (!timeout)
    {
	/* The kqueue branch passes a zeroed timespec here, i.e. "collect what
	 * is ready and return immediately". */
	timeout_ms = 0;
    }
    else
    {
	timeout_ms = (int) (timeout->tv_sec * 1000 + timeout->tv_usec / 1000);

	/* NOTE for the next reader: the kqueue branch assigns tv_usec straight
	 * into timespec.tv_nsec, so on FreeBSD the wait is actually 1000x
	 * shorter than the caller asked for.  We honour the documented meaning
	 * of struct timeval instead, but never turn a non-zero request into a
	 * busy poll. */
	if (timeout_ms == 0 && (timeout->tv_sec != 0 || timeout->tv_usec != 0))
	    timeout_ms = 1;
    }

    r = epoll_wait(fdw->ep, fdw->epevents, fdw->nfiles, timeout_ms);

    if (r == -1)
    {
	/* Includes EINTR, exactly like the kqueue branch: callers treat a
	 * negative return as "skip this iteration". */
	return -1;
    }

    memset(fdw->fd_event_idx, 0, sizeof(int) * fdw->nfiles);

    fdw->nfdwrevents = 0;

    for (i = 0; i < r; i++)
    {
	int fd = fdw->epevents[i].data.fd;
	unsigned int ev = fdw->epevents[i].events;
	int rw;

	if (fd < 0 || fd >= fdw->nfiles)
	{
	    sys_err("ident overflow %d nfiles: %d", fd, fdw->nfiles);
	    continue;
	}

	rw = fdw->fd_rw[fd];

	/* MAPPING 3: a hard error or a full hangup.  kqueue would deliver a
	 * single event with EV_ERROR / EV_EOF set and fdwatch_check_event()
	 * answers FDW_EOF for it without ever looking at the filter, so emit
	 * one such event and nothing else - the descriptor is dead either
	 * way. */
	if (ev & (EPOLLERR | EPOLLHUP))
	{
	    fdwatch_push_event(fdw, fd, FDW_NONE, FDW_EOF, 0);
	    continue;
	}

	/* MAPPING 1: expand the bitmask into per-direction events.  The "rw &"
	 * tests reproduce the guards kqueue's fdwatch_check_event() applies;
	 * filtering here as well means an index is never handed to the caller
	 * that would answer 0 and trip its "default:" close-the-connection
	 * arm. */
	if ((ev & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) && (rw & FDW_READ))
	{
	    /* MAPPING 3: EPOLLRDHUP == EV_EOF on EVFILT_READ.  Like FreeBSD,
	     * EOF wins over still-buffered readable data. */
	    fdwatch_push_event(fdw, fd, FDW_READ,
			       (ev & EPOLLRDHUP) ? FDW_EOF : FDW_NONE, 0);
	}

	if ((ev & EPOLLOUT) && (rw & FDW_WRITE))
	{
	    fdwatch_push_event(fdw, fd, FDW_WRITE, FDW_NONE,
			       fdwatch_sndbuf_left(fd));

	    /* MAPPING 4: one-shot writes.  EPOLLONESHOT would disarm the read
	     * side of the same registration too, so drop the write interest by
	     * hand.  This is not optional: EPOLLOUT is level-triggered, so
	     * leaving it armed makes every epoll_wait() return instantly for
	     * as long as the send buffer has room - a 100% CPU spin.
	     *
	     * Both halves of the transition happen HERE, together: the
	     * software bit and the kernel mask.  The kqueue branch instead
	     * clears FDW_WRITE later, inside fdwatch_check_event(), because on
	     * FreeBSD the kernel has already dropped the EV_ONESHOT knote by
	     * itself and the two can never disagree.  Under epoll they can,
	     * and splitting them is a real bug: if the caller handles a read
	     * event for this same descriptor before it gets to the write event
	     * (both are in this batch), it calls fdwatch_add_fd(FDW_WRITE)
	     * while fd_rw still claims FDW_WRITE, the add is skipped as
	     * redundant, and the descriptor is left armed for EPOLLOUT in the
	     * kernel with no software interest - the spin above.
	     *
	     * Keeping fd_mask == desired_mask(fd_rw) as a standing invariant
	     * removes that whole class of drift. */
	    if (rw & FDW_WRITE_ONESHOT)
	    {
		fdw->fd_rw[fd] &= ~FDW_WRITE;
		fdwatch_apply(fdw, fd, fdwatch_desired_mask(fdw, fd));
	    }
	}
    }

    return (fdw->nfdwrevents);
}

void fdwatch_clear_fd(LPFDWATCH fdw, socket_t fd)
{
    if (fd < 0 || fd >= fdw->nfiles)
	return;

    fdw->fd_data[fd] = NULL;
    fdw->fd_rw[fd] = 0;

    /* The kqueue branch leaves the kernel state to fdwatch_del_fd(); under
     * epoll the registration has to go here too.  EPOLLIN is level-triggered,
     * so a descriptor still armed but with no software interest left would
     * wake epoll_wait() on every pass and expand to zero events - a silent
     * busy loop. */
    fdwatch_apply(fdw, fd, 0);
}

void fdwatch_add_fd(LPFDWATCH fdw, socket_t fd, void * client_data, int rw, int oneshot)
{
    if (fd < 0 || fd >= fdw->nfiles)
    {
	sys_err("fd overflow %d", fd);
	return;
    }

    if (fdw->fd_rw[fd] & rw)
    {
	/* Already watching this direction, nothing to do - same early-out as
	 * the kqueue branch.  This is only safe because fd_mask is kept equal
	 * to desired_mask(fd_rw) at every mutation point, so "fd_rw says it is
	 * armed" always means "the kernel has it armed". */
	return;
    }

    fdw->fd_rw[fd] |= rw;
    sys_log(2, "FDWATCH_fdw %p fd %d rw %d data %p", fdw, fd, rw, client_data);

    if (oneshot)
    {
	sys_log(2, "ADD ONESHOT fd_rw %d", fdw->fd_rw[fd]);
	fdw->fd_rw[fd] |= FDW_WRITE_ONESHOT;
    }

    fdw->fd_data[fd] = client_data;
    fdwatch_apply(fdw, fd, fdwatch_desired_mask(fdw, fd));
}

void fdwatch_del_fd(LPFDWATCH fdw, socket_t fd)
{
    /* fdwatch_clear_fd() drops both the software state and the epoll
     * registration, so unlike the kqueue branch there is no separate
     * change-list entry to queue up here. */
    fdwatch_clear_fd(fdw, fd);
}

void fdwatch_clear_event(LPFDWATCH fdw, socket_t fd, unsigned int event_idx)
{
    assert(event_idx < (unsigned int) (fdw->nfiles * 2));

    if (fdw->fdwrevents[event_idx].ident != fd)
	return;

    /* Same trick as the kqueue branch: zero the ident so every later lookup
     * for this index misses.  This is how a caller neutralises an event it has
     * already consumed (the listen socket after accept()) or one belonging to
     * a descriptor it just destroyed mid-iteration. */
    fdw->fdwrevents[event_idx].ident = 0;
}

int fdwatch_check_event(LPFDWATCH fdw, socket_t fd, unsigned int event_idx)
{
    assert(event_idx < (unsigned int) (fdw->nfiles * 2));

    if (fdw->fdwrevents[event_idx].ident != fd)
	return 0;

    if (fdw->fdwrevents[event_idx].flags & FDW_EOF)
	return FDW_EOF;

    if (fdw->fdwrevents[event_idx].filter == FDW_READ)
    {
	if (fdw->fd_rw[fd] & FDW_READ)
	    return FDW_READ;
    }
    else if (fdw->fdwrevents[event_idx].filter == FDW_WRITE)
    {
	/* No "fd_rw & FDW_WRITE" test here, unlike the kqueue branch: for a
	 * one-shot write that bit was already cleared by fdwatch() when the
	 * event was produced (see MAPPING 4 there), so testing it would report
	 * 0 for a perfectly good event - and the callers close the connection
	 * when this function returns 0.  fdwatch() only emits a write event
	 * after checking the interest itself, so the event record is
	 * authoritative.
	 *
	 * fd_rw is still consulted for one thing: it is zero only after
	 * fdwatch_clear_fd()/fdwatch_del_fd(), so this rejects events left
	 * over for a descriptor the caller destroyed earlier in the same
	 * iteration, which is exactly what the kqueue branch does. */
	if (fdw->fd_rw[fd] != 0)
	    return FDW_WRITE;
    }
    else
	sys_err("fdwatch_check_event: Unknown filter %d (descriptor %d)", fdw->fdwrevents[event_idx].filter, fd);

    return 0;
}

int fdwatch_get_ident(LPFDWATCH fdw, unsigned int event_idx)
{
    assert(event_idx < (unsigned int) (fdw->nfiles * 2));
    return fdw->fdwrevents[event_idx].ident;
}

int fdwatch_get_buffer_size(LPFDWATCH fdw, socket_t fd)
{
    int event_idx;

    if (fd < 0 || fd >= fdw->nfiles)
	return 0;

    event_idx = fdw->fd_event_idx[fd];

    /* The extra ident check closes a hole the kqueue branch has: fd_event_idx
     * is memset to 0 every pass, so a descriptor with no write event this
     * round would otherwise read slot 0, which may belong to someone else. */
    if (event_idx < fdw->nfdwrevents &&
	fdw->fdwrevents[event_idx].ident == fd &&
	fdw->fdwrevents[event_idx].filter == FDW_WRITE)
    {
	return fdw->fdwrevents[event_idx].data;
    }

    /* No write event for this descriptor in the current round - ask the kernel
     * instead of answering 0.
     *
     * This is not a refinement, it is required for correctness.  kqueue's
     * kqrevents[] is not cleared between passes, so on FreeBSD the last
     * EVFILT_WRITE record for a descriptor survives and this function keeps
     * answering a positive sbspace() long after that event was consumed.  The
     * epoll branch rebuilds fdwrevents[] every pass, so without this fallback
     * it answers 0 for any descriptor that is not writable *this* round.
     *
     * Callers treat 0 as "the socket cannot take anything right now" and skip
     * the write entirely - DESC::ProcessOutput() (game/src/desc.cpp) returns
     * at its "buffer_left <= 0" guard.  That silently breaks the deliberate
     * mid-phase flushes, the one that matters being
     * CInputHandshake::Analyze()'s
     *
     *     d->SendKeyAgreementCompleted();
     *     // Flush socket output before going encrypted
     *     d->ProcessOutput();
     *
     * (game/src/input.cpp).  Those flushes always run while handling a *read*
     * event, i.e. exactly when no write event exists, so on Linux they became
     * no-ops: HEADER_GC_KEY_AGREEMENT_COMPLETED stayed queued and left the
     * server coalesced with the first ciphertext that followed it.  The client
     * decrypts whole socket reads at a time, so it took the ciphertext in
     * while its cipher was still inactive, kept it as plaintext, and stalled
     * forever on the garbage that produced.
     *
     * fdwatch_sndbuf_left() is the same conservative figure already reported
     * in write events above, so nothing downstream sees a value it would not
     * otherwise have been handed. */
    return fdwatch_sndbuf_left(fd);
}

void * fdwatch_get_client_data(LPFDWATCH fdw, unsigned int event_idx)
{
    int fd;

    assert(event_idx < (unsigned int) (fdw->nfiles * 2));

    fd = fdw->fdwrevents[event_idx].ident;	/* MAPPING 2 */

    if (fd < 0 || fd >= fdw->nfiles)
	return NULL;

    return (fdw->fd_data[fd]);
}

#else	/* !__linux__ : FreeBSD kqueue backend (unchanged) */
/* ### LINUX-BLOCK-END (fdwatch.c) ####################################### */

LPFDWATCH fdwatch_new(int nfiles)
{
    LPFDWATCH fdw;
    int kq;

    kq = kqueue();

    if (kq == -1)
    {
	sys_err("%s", strerror(errno));
	return NULL;
    }

    CREATE(fdw, FDWATCH, 1);

    fdw->kq = kq;
    fdw->nfiles = nfiles;
    fdw->nkqevents = 0;

    CREATE(fdw->kqevents, KEVENT, nfiles * 2);
    CREATE(fdw->kqrevents, KEVENT, nfiles * 2);
    CREATE(fdw->fd_event_idx, int, nfiles);
    CREATE(fdw->fd_rw, int, nfiles);
    CREATE(fdw->fd_data, void*, nfiles);

    return (fdw);
}

void fdwatch_delete(LPFDWATCH fdw)
{
    free(fdw->fd_data);
    free(fdw->fd_rw);
    free(fdw->kqevents);
    free(fdw->kqrevents);
    free(fdw->fd_event_idx);
    free(fdw);
}

int fdwatch(LPFDWATCH fdw, struct timeval *timeout)
{
    int	i, r;
    struct timespec ts;

    if (fdw->nkqevents)
	sys_log(2, "fdwatch: nkqevents %d", fdw->nkqevents);

    if (!timeout)
    {
	ts.tv_sec = 0;
	ts.tv_nsec = 0;

	r = kevent(fdw->kq, fdw->kqevents, fdw->nkqevents, fdw->kqrevents, fdw->nfiles, &ts);
    }
    else
    {
	ts.tv_sec = timeout->tv_sec;
	ts.tv_nsec = timeout->tv_usec;

	r = kevent(fdw->kq, fdw->kqevents, fdw->nkqevents, fdw->kqrevents, fdw->nfiles, &ts);
    }

    fdw->nkqevents = 0;

    if (r == -1)
	return -1;

    memset(fdw->fd_event_idx, 0, sizeof(int) * fdw->nfiles);

    for (i = 0; i < r; i++)
    {
	int fd = fdw->kqrevents[i].ident;

	if (fd >= fdw->nfiles)
	    sys_err("ident overflow %d nfiles: %d", fdw->kqrevents[i].ident, fdw->nfiles);
	else
	{
	    if (fdw->kqrevents[i].filter == EVFILT_WRITE)
		fdw->fd_event_idx[fd] = i;
	}
    }

    return (r);
}

void fdwatch_register(LPFDWATCH fdw, int flag, int fd, int rw)
{
    if (flag == EV_DELETE)
    {
	if (fdw->fd_rw[fd] & FDW_READ)
	{
	    fdw->kqevents[fdw->nkqevents].ident = fd;
	    fdw->kqevents[fdw->nkqevents].flags = flag;
	    fdw->kqevents[fdw->nkqevents].filter = EVFILT_READ;
	    ++fdw->nkqevents;
	}

	if (fdw->fd_rw[fd] & FDW_WRITE)
	{
	    fdw->kqevents[fdw->nkqevents].ident = fd;
	    fdw->kqevents[fdw->nkqevents].flags = flag;
	    fdw->kqevents[fdw->nkqevents].filter = EVFILT_WRITE;
	    ++fdw->nkqevents;
	}
    }
    else
    {
	fdw->kqevents[fdw->nkqevents].ident = fd;
	fdw->kqevents[fdw->nkqevents].flags = flag;
	fdw->kqevents[fdw->nkqevents].filter = (rw == FDW_READ) ? EVFILT_READ : EVFILT_WRITE; 

	++fdw->nkqevents;
    }
}

void fdwatch_clear_fd(LPFDWATCH fdw, socket_t fd)
{
    fdw->fd_data[fd] = NULL;
    fdw->fd_rw[fd] = 0;
}

void fdwatch_add_fd(LPFDWATCH fdw, socket_t fd, void * client_data, int rw, int oneshot)
{
	int flag;

	if (fd >= fdw->nfiles)
	{
		sys_err("fd overflow %d", fd);
		return;
	}

	if (fdw->fd_rw[fd] & rw)
		return;

	fdw->fd_rw[fd] |= rw;
	sys_log(2, "FDWATCH_fdw %p fd %d rw %d data %p", fdw, fd, rw, client_data);

	if (!oneshot)
		flag = EV_ADD;
	else
	{
		sys_log(2, "ADD ONESHOT fd_rw %d", fdw->fd_rw[fd]);
		flag = EV_ADD | EV_ONESHOT;
		fdw->fd_rw[fd] |= FDW_WRITE_ONESHOT;
	}

	fdw->fd_data[fd] = client_data;
	fdwatch_register(fdw, flag, fd, rw);
}

void fdwatch_del_fd(LPFDWATCH fdw, socket_t fd)
{
    fdwatch_register(fdw, EV_DELETE, fd, 0);
    fdwatch_clear_fd(fdw, fd);
}

void fdwatch_clear_event(LPFDWATCH fdw, socket_t fd, unsigned int event_idx)
{
    assert(event_idx < fdw->nfiles * 2);

    if (fdw->kqrevents[event_idx].ident != fd)
	return;

    fdw->kqrevents[event_idx].ident = 0;
}

int fdwatch_check_event(LPFDWATCH fdw, socket_t fd, unsigned int event_idx)
{
    assert(event_idx < fdw->nfiles * 2);

    if (fdw->kqrevents[event_idx].ident != fd)
	return 0;

    if (fdw->kqrevents[event_idx].flags & EV_ERROR)
	return FDW_EOF;

    if (fdw->kqrevents[event_idx].flags & EV_EOF)
	return FDW_EOF;

    if (fdw->kqrevents[event_idx].filter == EVFILT_READ)
    {
	if (fdw->fd_rw[fd] & FDW_READ)
	    return FDW_READ;
    }
    else if (fdw->kqrevents[event_idx].filter == EVFILT_WRITE)
    {   
	if (fdw->fd_rw[fd] & FDW_WRITE)
	{ 
	    if (fdw->fd_rw[fd] & FDW_WRITE_ONESHOT)
		fdw->fd_rw[fd] &= ~FDW_WRITE;

	    return FDW_WRITE;
	}
    }
    else
	sys_err("fdwatch_check_event: Unknown filter %d (descriptor %d)", fdw->kqrevents[event_idx].filter, fd);

    return 0;
}

int fdwatch_get_ident(LPFDWATCH fdw, unsigned int event_idx)
{
    assert(event_idx < fdw->nfiles * 2);
    return fdw->kqrevents[event_idx].ident;
}

int fdwatch_get_buffer_size(LPFDWATCH fdw, socket_t fd)
{
    int event_idx = fdw->fd_event_idx[fd];

    if (fdw->kqrevents[event_idx].filter == EVFILT_WRITE)
	return fdw->kqrevents[event_idx].data;

    return 0;
}

void * fdwatch_get_client_data(LPFDWATCH fdw, unsigned int event_idx)
{
    int fd;

    assert(event_idx < fdw->nfiles * 2);

    fd = fdw->kqrevents[event_idx].ident;

    if (fd >= fdw->nfiles)
	return NULL;

    return (fdw->fd_data[fd]);
}

/* ### LINUX-BLOCK-BEGIN (fdwatch.c tail) ################################ */
#endif	/* __linux__ */
/* ### LINUX-BLOCK-END (fdwatch.c tail) ################################## */

#else	// ifndef __USE_SELECT__

#ifdef __WIN32__
static int win32_init_refcount = 0;

static bool win32_init()
{
    if (win32_init_refcount > 0)
    {
	win32_init_refcount++;
	return true;
    }

    WORD wVersion = MAKEWORD(2, 0);
    WSADATA wsaData;

    if (WSAStartup(wVersion, &wsaData) != 0)
	return false;

    win32_init_refcount++;
    return true;
}

static void win32_deinit()
{
    if (--win32_init_refcount <= 0)
	WSACleanup();
}
#endif

LPFDWATCH fdwatch_new(int nfiles)
{
    LPFDWATCH fdw;

#ifdef __WIN32__
    if (!win32_init())
	return NULL;
#endif
	// nfiles value is limited to FD_SETSIZE (64)
    CREATE(fdw, FDWATCH, 1);
	fdw->nfiles = MIN(nfiles, FD_SETSIZE);

    FD_ZERO(&fdw->rfd_set);
    FD_ZERO(&fdw->wfd_set);

    CREATE(fdw->select_fds, socket_t, nfiles);
    CREATE(fdw->select_rfdidx, int, nfiles);

	fdw->nselect_fds = 0;

    CREATE(fdw->fd_rw, int, nfiles);
    CREATE(fdw->fd_data, void*, nfiles);

    return (fdw);
}

void fdwatch_delete(LPFDWATCH fdw)
{
    free(fdw->fd_data);
    free(fdw->fd_rw);
    free(fdw->select_fds);
    free(fdw->select_rfdidx);
    free(fdw);

#ifdef __WIN32__
    win32_deinit();
#endif
}

static int fdwatch_get_fdidx(LPFDWATCH fdw, socket_t fd) {
	int i;
	for (i = 0; i < fdw->nselect_fds; ++i) {
		if (fdw->select_fds[i] == fd) {
			return i;
		}
	}
	return -1;
}

void fdwatch_add_fd(LPFDWATCH fdw, socket_t fd, void* client_data, int rw, int oneshot)
{
	int idx = fdwatch_get_fdidx(fdw, fd);
	if (idx < 0) {
		if (fdw->nselect_fds >= fdw->nfiles) {
			return;
		}
		idx = fdw->nselect_fds;
		fdw->select_fds[fdw->nselect_fds++] = fd;
		fdw->fd_rw[idx] = rw;
	} else {
		fdw->fd_rw[idx] |= rw;
	}
	fdw->fd_data[idx] = client_data;

    if (rw & FDW_READ)
	FD_SET(fd, &fdw->rfd_set);

    if (rw & FDW_WRITE)
	FD_SET(fd, &fdw->wfd_set);
}

void fdwatch_del_fd(LPFDWATCH fdw, socket_t fd)
{
	if (fdw->nselect_fds <= 0) {
		return;
	}
    int idx = fdwatch_get_fdidx(fdw, fd);
	if (idx < 0) {
		return;
	}

	--fdw->nselect_fds;

	fdw->select_fds[idx] = fdw->select_fds[fdw->nselect_fds];
    fdw->fd_data[idx] = fdw->fd_data[fdw->nselect_fds];
    fdw->fd_rw[idx] = fdw->fd_rw[fdw->nselect_fds];

    FD_CLR(fd, &fdw->rfd_set);
    FD_CLR(fd, &fdw->wfd_set);
}

int fdwatch(LPFDWATCH fdw, struct timeval *timeout)
{
    int r, i, event_idx;
    struct timeval tv;

    fdw->working_rfd_set = fdw->rfd_set;
    fdw->working_wfd_set = fdw->wfd_set;

    if (!timeout)
    {
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	r = select(0, &fdw->working_rfd_set, &fdw->working_wfd_set, (fd_set*) 0, &tv);
    }
    else
    {
	tv = *timeout;
	r = select(0, &fdw->working_rfd_set, &fdw->working_wfd_set, (fd_set*) 0, &tv);
    }

    if (r == -1)
	return -1;

    event_idx = 0;

    for (i = 0; i < fdw->nselect_fds; ++i)
    {
		if (fdwatch_check_fd(fdw, fdw->select_fds[i]))
			fdw->select_rfdidx[event_idx++] = i;
    }

    return event_idx;
}

int fdwatch_check_fd(LPFDWATCH fdw, socket_t fd)
{
    int idx = fdwatch_get_fdidx(fdw, fd);
	if (idx < 0) {
		return 0;
	}
	int result = 0;
	if ((fdw->fd_rw[idx] & FDW_READ) && FD_ISSET(fd, &fdw->working_rfd_set)) {
		result |= FDW_READ;
	}
	if ((fdw->fd_rw[idx] & FDW_WRITE) && FD_ISSET(fd, &fdw->working_wfd_set)) {
		result |= FDW_WRITE;
	}
    return result;
}

void * fdwatch_get_client_data(LPFDWATCH fdw, unsigned int event_idx)
{
	int idx = fdw->select_rfdidx[event_idx];
	if (idx < 0 || fdw->nfiles <= idx) {
		return NULL;
	}
    return fdw->fd_data[idx];
}

int fdwatch_get_ident(LPFDWATCH fdw, unsigned int event_idx)
{
	int idx = fdw->select_rfdidx[event_idx];
	if (idx < 0 || fdw->nfiles <= idx) {
		return 0;
	}
	return (int)fdw->select_fds[idx];
}

void fdwatch_clear_event(LPFDWATCH fdw, socket_t fd, unsigned int event_idx)
{
	int idx = fdw->select_rfdidx[event_idx];
	if (idx < 0 || fdw->nfiles <= idx) {
		return;
	}
	socket_t rfd = fdw->select_fds[idx];
	if (fd != rfd) {
		return;
	}
    FD_CLR(fd, &fdw->working_rfd_set);
    FD_CLR(fd, &fdw->working_wfd_set);
}

int fdwatch_check_event(LPFDWATCH fdw, socket_t fd, unsigned int event_idx)
{
	int idx = fdw->select_rfdidx[event_idx];
	if (idx < 0 || fdw->nfiles <= idx) {
		return 0;
	}
	socket_t rfd = fdw->select_fds[idx];
	if (fd != rfd) {
		return 0;
	}
	int result = fdwatch_check_fd(fdw, fd);
	if (result & FDW_READ) {
		return FDW_READ;
	} else if (result & FDW_WRITE) {
		return FDW_WRITE;
	}
	return 0;
}

int fdwatch_get_buffer_size(LPFDWATCH fdw, socket_t fd)
{
    return INT_MAX; // XXX TODO
}

#endif
