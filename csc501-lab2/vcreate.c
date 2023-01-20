/* vcreate.c - vcreate */
    
#include <conf.h>
#include <i386.h>
#include <kernel.h>
#include <proc.h>
#include <sem.h>
#include <mem.h>
#include <io.h>
#include <paging.h>

/*
static unsigned long esp;
*/

LOCAL	newpid();
/*------------------------------------------------------------------------
 *  create  -  create a process to start running a procedure
 *------------------------------------------------------------------------
 */
SYSCALL vcreate(procaddr,ssize,hsize,priority,name,nargs,args)
	int	*procaddr;		/* procedure address		*/
	int	ssize;			/* stack size in words		*/
	int	hsize;			/* virtual heap size in pages	*/
	int	priority;		/* process priority > 0		*/
	char	*name;			/* name (for debugging)		*/
	int	nargs;			/* number of args that follow	*/
	long	args;			/* arguments (treated like an	*/
					/* array in the code)		*/
{
	STATWORD ps;
	disable(ps);

	struct mblock *bsaddr;

	int bs_id_to_pid=0;
	int pid =create(procaddr,ssize,priority,name,nargs,args);

	// If get_bsm fails, there are no free bs with space for vheap
	if(get_bsm(&bs_id_to_pid) == SYSERR){restore(ps); return SYSERR;}

	if(bsm_tab[bs_id_to_pid].bs_freepages<hsize)
		bs_id_to_pid+=1;

	bsm_map(pid,VAddr_Base,bs_id_to_pid,hsize);
	bs_heap[bs_id_to_pid]=pid;

	bsaddr=BACKING_STORE_BASE +(bs_id_to_pid *BACKING_STORE_UNIT_SIZE)+(256-bsm_tab[bs_id_to_pid].bs_freepages)*NBPG;
	bsaddr -> mlen = hsize * NBPG;

	proctab[pid].vhpno=256-bsm_tab[bs_id_to_pid].bs_freepages+1;
	proctab[pid].vhpnpages=hsize;

	proctab[pid].vmemlist->mnext = VAddr_Base * NBPG;

	restore(ps);

	return pid;
}

/*------------------------------------------------------------------------
 * newpid  --  obtain a new (free) process id
 *------------------------------------------------------------------------
 */
LOCAL	newpid()
{
	int	pid;			/* process id to return		*/
	int	i;

	for (i=0 ; i<NPROC ; i++) {	/* check all NPROC slots	*/
		if ( (pid=nextproc--) <= 0)
			nextproc = NPROC-1;
		if (proctab[pid].pstate == PRFREE)
			return(pid);
	}
	return(SYSERR);
}
