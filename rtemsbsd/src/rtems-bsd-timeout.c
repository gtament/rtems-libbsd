/**
 * @file
 *
 * @ingroup rtems_bsd_rtems
 *
 * @brief TODO.
 */

/*
 *  COPYRIGHT (c) 1989-2012.
 *  On-Line Applications Research Corporation (OAR).
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rtems.com/license/LICENSE.
 */

#include <freebsd/machine/rtems-bsd-config.h>
#include <freebsd/sys/cdefs.h>
__FBSDID("$FreeBSD$");

#include <freebsd/sys/param.h>
#include <freebsd/sys/systm.h>
#include <freebsd/sys/bus.h>
#include <freebsd/sys/callout.h>
#include <freebsd/sys/condvar.h>
#include <freebsd/sys/interrupt.h>
#include <freebsd/sys/kernel.h>
#include <freebsd/sys/ktr.h>
#include <freebsd/sys/lock.h>
#include <freebsd/sys/malloc.h>
#include <freebsd/sys/mutex.h>
#include <freebsd/sys/proc.h>
#include <freebsd/sys/sdt.h>

static int timeout_cpu;
/*
 * There is one struct callout_cpu per cpu, holding all relevant
 * state for the callout processing thread on the individual CPU.
 * In particular:
 *	cc_ticks is incremented once per tick in callout_cpu().
 *	It tracks the global 'ticks' but in a way that the individual
 *	threads should not worry about races in the order in which
 *	hardclock() and hardclock_cpu() run on the various CPUs.
 *	cc_softclock is advanced in callout_cpu() to point to the
 *	first entry in cc_callwheel that may need handling. In turn,
 *	a softclock() is scheduled so it can serve the various entries i
 *	such that cc_softclock <= i <= cc_ticks .
 *	XXX maybe cc_softclock and cc_ticks should be volatile ?
 *
 *	cc_ticks is also used in callout_reset_cpu() to determine
 *	when the callout should be served.
 */
struct callout_cpu {
	struct mtx		cc_lock;
	struct callout		*cc_callout;
	struct callout_tailq	*cc_callwheel;
	struct callout_list	cc_callfree;
	struct callout		*cc_next;
	struct callout		*cc_curr;
	void			*cc_cookie;
	int 			cc_ticks;
	int 			cc_softticks;
	int			cc_cancel;
	int			cc_waiting;
};

/*
 * timeout --
 *	Execute a function after a specified length of time.
 *
 * untimeout --
 *	Cancel previous timeout function call.
 *
 * callout_handle_init --
 *	Initialize a handle so that using it with untimeout is benign.
 *
 *	See AT&T BCI Driver Reference Manual for specification.  This
 *	implementation differs from that one in that although an 
 *	identification value is returned from timeout, the original
 *	arguments to timeout as well as the identifier are used to
 *	identify entries for untimeout.
 */

struct callout_handle
timeout(ftn, arg, to_ticks)
	timeout_t *ftn;
	void *arg;
	int to_ticks;
{
	struct callout_cpu *cc;
	struct callout *new;
	struct callout_handle handle;

#if 0
	cc = CC_CPU(timeout_cpu);
	CC_LOCK(cc);
	/* Fill in the next free callout structure. */
	new = SLIST_FIRST(&cc->cc_callfree);
	if (new == NULL)
		/* XXX Attempt to malloc first */
		panic("timeout table full");
	SLIST_REMOVE_HEAD(&cc->cc_callfree, c_links.sle);
	callout_reset(new, to_ticks, ftn, arg);
	handle.callout = new;
	CC_UNLOCK(cc);
#endif
	return (handle);
}

