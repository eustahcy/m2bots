#ifndef __INC_LIBTHECORE_FDWATCH_H__
#define __INC_LIBTHECORE_FDWATCH_H__

#ifndef __WIN32__

/* ### LINUX-BLOCK-BEGIN (fdwatch.h) ###################################### */
#if defined(__linux__)

    /* --------------------------------------------------------------------
     * Linux backend: epoll(7).
     *
     * kqueue reports one event per (descriptor, filter) pair, so a socket
     * that is both readable and writable produces *two* struct kevents.
     * epoll reports one struct epoll_event per descriptor carrying a
     * bitmask of every ready condition.  Every caller of this API was
     * written against the kqueue event stream (it walks the returned events
     * by index and asks fdwatch_check_event() for a single FDW_* answer per
     * index), so the Linux backend expands each epoll_event back into up to
     * two FDWEVENTs.  FDWEVENT is therefore the exact analogue of struct
     * kevent, and "fdwrevents" below is the analogue of "kqrevents".
     * ------------------------------------------------------------------ */

    typedef struct fdwatch	FDWATCH;
    typedef struct fdwatch *	LPFDWATCH;

    enum EFdwatch
    {
	FDW_NONE		= 0,
	FDW_READ		= 1,
	FDW_WRITE		= 2,
	FDW_WRITE_ONESHOT	= 4,
	FDW_EOF			= 8,
    };

    typedef struct fdwevent
    {
	int	ident;		/* kevent.ident  : the file descriptor        */
	int	filter;		/* kevent.filter : FDW_READ / FDW_WRITE       */
	int	flags;		/* kevent.flags  : carries FDW_EOF only       */
	int	data;		/* kevent.data   : free send-buffer space     */
    } FDWEVENT;

    typedef FDWEVENT *		LPFDWEVENT;
    typedef int			EPOLLFD;

    struct fdwatch
    {
	EPOLLFD		ep;		/* epoll instance; mirrors kqueue's kq */

	int		nfiles;

	struct epoll_event *	epevents;	/* epoll_wait() output buffer  */

	LPFDWEVENT	fdwrevents;	/* expanded events; mirrors kqrevents  */
	int		nfdwrevents;	/* number of valid entries above       */

	int *		fd_event_idx;

	void **		fd_data;
	int *		fd_rw;

	unsigned int *	fd_mask;	/* epoll mask currently armed per fd   */
    };

#else	/* !__linux__ : FreeBSD and other BSDs - kqueue backend (unchanged) */
/* ### LINUX-BLOCK-END (fdwatch.h) ######################################## */

    typedef struct fdwatch	FDWATCH;
    typedef struct fdwatch *	LPFDWATCH;

    enum EFdwatch
    {
	FDW_NONE		= 0,
	FDW_READ		= 1,
	FDW_WRITE		= 2,
	FDW_WRITE_ONESHOT	= 4,
	FDW_EOF			= 8,
    };

    typedef struct kevent	KEVENT;
    typedef struct kevent *	LPKEVENT;
    typedef int			KQUEUE;

    struct fdwatch
    {
	KQUEUE		kq;

	int		nfiles;

	LPKEVENT	kqevents;
	int		nkqevents;

	LPKEVENT	kqrevents;
	int *		fd_event_idx;

	void **		fd_data;
	int *		fd_rw;
    };

/* ### LINUX-BLOCK-BEGIN (fdwatch.h tail) ################################# */
#endif	/* __linux__ */
/* ### LINUX-BLOCK-END (fdwatch.h tail) ################################### */

#else

    typedef struct fdwatch	FDWATCH;
    typedef struct fdwatch *	LPFDWATCH;

    enum EFdwatch
    {
	FDW_NONE		= 0,
	FDW_READ		= 1,
	FDW_WRITE		= 2,
	FDW_WRITE_ONESHOT	= 4,
	FDW_EOF			= 8,
    };

    struct fdwatch
    {
	fd_set rfd_set;
	fd_set wfd_set;

	socket_t* select_fds;
	int* select_rfdidx;

	int nselect_fds;

	fd_set working_rfd_set;
	fd_set working_wfd_set;

	int nfiles;

	void** fd_data;
	int* fd_rw;
    };

#endif // WIN32

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    extern LPFDWATCH	fdwatch_new(int nfiles);
    extern void		fdwatch_clear_fd(LPFDWATCH fdw, socket_t fd);
    extern void		fdwatch_delete(LPFDWATCH fdw);
	extern int fdwatch_check_fd(LPFDWATCH fdw, socket_t fd);
    extern int		fdwatch_check_event(LPFDWATCH fdw, socket_t fd, unsigned int event_idx);
    extern void		fdwatch_clear_event(LPFDWATCH fdw, socket_t fd, unsigned int event_idx);
    extern void		fdwatch_add_fd(LPFDWATCH fdw, socket_t fd, void* client_data, int rw, int oneshot);
    extern int		fdwatch(LPFDWATCH fdw, struct timeval *timeout);
    extern void *	fdwatch_get_client_data(LPFDWATCH fdw, unsigned int event_idx);
    extern void		fdwatch_del_fd(LPFDWATCH fdw, socket_t fd);
    extern int		fdwatch_get_buffer_size(LPFDWATCH fdw, socket_t fd);
    extern int		fdwatch_get_ident(LPFDWATCH fdw, unsigned int event_idx);

#ifdef __cplusplus
}
#endif

#endif
