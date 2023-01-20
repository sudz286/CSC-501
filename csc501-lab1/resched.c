/* resched.c  -  resched */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <math.h>
#include <sched.h>
unsigned long currSP;	/* REAL sp of current process */
extern int ctxsw(int, int, int, int);
/*-----------------------------------------------------------------------
 * resched  --  reschedule processor to highest priority ready process
 *
 * Notes:	Upon entry, currpid gives current process id.
 *		Proctab[currpid].pstate gives correct NEXT state for
 *			current process if other than PRREADY.
 *------------------------------------------------------------------------
 */
int resched()
{
	register struct	pentry	*optr;	/* pointer to old process entry */
	register struct	pentry	*nptr;	/* pointer to new process entry */
	
	int currentSchedPolicy = getschedclass();
	optr = &proctab[currpid];

	if(currentSchedPolicy == EXPDISTSCHED)
	{
		int expval = (int)expdev();

		if(optr->pstate == PRCURR)
		{
			optr->pstate = PRREADY;
			insert(currpid, rdyhead, optr->pprio);
		}
		
		int next = getnextproc(expval);
		
		if(next) currpid = dequeue(next);
		else currpid = NULLPROC;

		nptr = &proctab[currpid];

		nptr->pstate = PRCURR;

        #ifdef RTCLOCK  
			preempt = QUANTUM;
        #endif
	}
	else if(currentSchedPolicy == LINUXSCHED)
	{
		int m = 0, next = 0;

		optr->goodness -= optr->counter - preempt;

		getmaxgoodproc(&next, &m);

		if(currpid != NULLPROC)	optr->counter = preempt;
		else optr->counter = 0;

		if (currpid == NULLPROC || !preempt) optr->goodness = 0;
		

		if(!m && (optr->pstate != PRCURR || !optr->counter))
		{
			updateprocgoodness();
			
			if (!m && currpid != NULLPROC) 
			{
				if (optr->pstate == PRCURR) 
				{
                    optr->pstate = PRREADY;
                    insert(currpid, rdyhead, optr->pprio);
                }

				nptr = dequeuenextproc(NULLPROC);

				#ifdef  RTCLOCK
					preempt = QUANTUM;
				#endif
				ctxsw((int) &optr->pesp, (int) optr->pirmask, (int) &nptr->pesp, (int) nptr->pirmask);
				return(OK);
			}
		} 
		else if (optr->pstate == PRCURR && optr->goodness >= m && optr->goodness) return(OK);
		else if (m > 0 && (!optr->counter || optr->pstate < m || optr->counter != PRCURR)) 
		{
			if (optr->pstate == PRCURR) 
			{
				optr->pstate = PRREADY;
				insert(currpid, rdyhead, optr->pprio);
			}

			nptr = dequeuenextproc(next);
			preempt = nptr->counter;
		}
	}
	else
	{
		// DEFAULT SCHEDULER
		/* no switch needed if current process priority higher than next*/
		if ( ( optr->pstate == PRCURR) &&
		(lastkey(rdytail)<optr->pprio)) {
			return(OK);
		}
		
		/* force context switch */

		if (optr->pstate == PRCURR) {
			optr->pstate = PRREADY;
			insert(currpid,rdyhead,optr->pprio);
		}

		/* remove highest priority process at end of ready list */

		nptr = &proctab[ (currpid = getlast(rdytail)) ];
		nptr->pstate = PRCURR;		
		#ifdef	RTCLOCK
			preempt = QUANTUM;
		#endif
	}
	ctxsw((int)&optr->pesp, (int)optr->pirmask, (int)&nptr->pesp, (int)nptr->pirmask);
	return OK;
}
