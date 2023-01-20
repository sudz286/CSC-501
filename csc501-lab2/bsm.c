/* bsm.c - manage the backing store mapping*/

#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <proc.h>

/*-------------------------------------------------------------------------
 * init_bsm- initialize bsm_tab
 *-------------------------------------------------------------------------
 */
//bs_map_t bsm_tab[];
SYSCALL init_bsm()
{
    STATWORD ps;
    disable(ps);

    int i = 0;
    for(i = 0;i < 8;i++)
    {
        bsm_tab[i].bs_status = 0; // 0 means unmapped
        bsm_tab[i].bs_pid = -1;
        bsm_tab[i].bs_vpno = 4096;
        bsm_tab[i].bs_npages = 0;
        bsm_tab[i].bs_freepages=256;

        bs_heap[i] = 0;
    }

    restore(ps);
    return OK;
}

/*-------------------------------------------------------------------------
 * get_bsm - get a free entry from bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL get_bsm(int* avail)
{
    STATWORD ps;
    disable(ps);

    int i = 0;
    for(i = 0; i < 8;i++)
    {
        if(bsm_tab[i].bs_status == BSM_UNMAPPED)
        {
            *avail = i;
            restore(ps);
            return (OK);
        }
    }

    restore(ps);
    return SYSERR;
}


/*-------------------------------------------------------------------------
 * free_bsm - free an entry from bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL free_bsm(int i)
{
    if(i < 0 || i > 7 && bsm_tab[i].bs_status == BSM_UNMAPPED){ return SYSERR;}

    bsm_tab[i].bs_status = BSM_UNMAPPED; // 0 means unmapped
    bsm_tab[i].bs_pid = -1;
    bsm_tab[i].bs_vpno = 4096;
    bsm_tab[i].bs_npages = 0;
    bsm_tab[i].bs_freepages = 256;
    bs_heap[i] = 0;

    return OK;
}

/*-------------------------------------------------------------------------
 * bsm_lookup - lookup bsm_tab and find the corresponding entry
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_lookup(int pid, long vaddr, int* store, int* pageth)
{
    int i = 0;
    STATWORD ps;
    disable(ps);

    for(i = 0; i < 8; i++)
    {
        if(bsm_tab[i].bs_pid == pid)
        {
            if(bsm_tab[i].bs_status == BSM_UNMAPPED){ restore(ps); return SYSERR;}

            *store = i;
            *pageth = (vaddr/NBPG) - bsm_tab[i].bs_vpno;

            restore(ps);
            return (OK);
         }
    }
    restore(ps);
    return SYSERR;
}


/*-------------------------------------------------------------------------
 * bsm_map - add an mapping into bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_map(int pid, int vpno, int source, int npages)
{
    STATWORD ps;
    disable(ps);

    int i = 0;
    if((pid > NPROC || pid < 0 )|| (source > 7 || source < 0)||(bs_heap[i] ==1 && bsm_tab[source].bs_pid != pid)) {restore(ps); return SYSERR;}

    if(bsm_tab[source].bs_pid==pid)
    {
        for(i = 0; i < 7; i++)
        {
            if(i == source)
            {
                bsm_tab[i].bs_pid = pid;
                bsm_tab[i].bs_vpno = vpno;
                bsm_tab[i].bs_npages = npages;
                bsm_tab[i].bs_status = BSM_MAPPED;
                bsm_tab[i].bs_freepages = 256 - npages;

                proctab[currpid].vhpno = vpno;
                proctab[currpid].store = source;

                restore(ps);
                return OK;
            }
        }
    }
    restore(ps);
    return SYSERR;
}



/*-------------------------------------------------------------------------
 * bsm_unmap - delete an mapping from bsm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_unmap(int pid, int vpno)
{
    STATWORD ps;
    disable(ps);
    if((pid > NPROC || pid < 0 )) {restore(ps); return SYSERR; }
    int i = 0;
    int bs_id_tbf, num_of_pages;
    if(bsm_lookup(pid, vpno, &bs_id_tbf, &num_of_pages) == SYSERR){ restore(ps); }
    for(i = 0; i < 8; i++)
    {
        if(bsm_tab[i].bs_pid == pid)
        {
            bsm_tab[i].bs_status = 0; // 0 means unmapped
            bsm_tab[i].bs_pid = 0;
            bsm_tab[i].bs_vpno = 4096;
            bsm_tab[i].bs_freepages = 256;
            bsm_tab[i].bs_npages = 0;
            bs_heap[i] = 0;
            
        }
    }

    restore(ps);
    return OK;
}