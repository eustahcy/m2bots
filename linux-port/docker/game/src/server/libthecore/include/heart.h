#ifndef __INC_LIBTHECORE_HEART_H__
#define __INC_LIBTHECORE_HEART_H__

typedef struct heart	HEART;
typedef struct heart *	LPHEART;

typedef void (*HEARTFUNC) (LPHEART heart, int pulse);

struct heart
{
    HEARTFUNC		func;

    struct timeval	before_sleep;
    struct timeval	opt_time;
    struct timeval	last_time;

    int			passes_per_sec;
    int			pulse;
};

extern LPHEART	heart_new(int opt_usec, HEARTFUNC func);
extern void	heart_delete(LPHEART ht);
extern int	heart_idle(LPHEART ht);
extern void	heart_beat(LPHEART ht, int pulses);

#endif
//martysama0134's 4e4e75d8b719b9240e033009cf4d7b0f
