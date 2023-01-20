#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

int get_bs(bsd_t bs_id, unsigned int npages) {

  /* requests a new mapping of npages with ID map_id */
  STATWORD ps;
  disable(ps);

  if((npages <= 0 || npages > 256) && (bs_id < 0 || bs_id > 7) || (bsm_tab[bs_id].bs_freepages < npages)) {kprintf("\nBacking store does not have space"); restore(ps); return SYSERR; }
  
  if(bsm_tab[bs_id].bs_status == BSM_UNMAPPED)
  {
    bsm_tab[bs_id].bs_status = BSM_MAPPED;
    bsm_tab[bs_id].bs_pid = currpid;
    
    restore(ps);
    return npages;
  }
  else if(bsm_tab[bs_id].bs_status == BSM_MAPPED && bsm_tab[bs_id].bs_freepages > npages) { restore(ps); return bsm_tab[bs_id].bs_npages; }

  restore(ps);
  return SYSERR;
}

