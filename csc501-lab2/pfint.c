/* pfint.c - pfint */

#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include<proc.h>


/*-------------------------------------------------------------------------
 * pfint - paging fault ISR
 *-------------------------------------------------------------------------
 */
SYSCALL pfint()
{
  STATWORD 	ps;
	disable(ps);  

  pd_t *pd_tab;
  pt_t *pt_tab;

  virt_addr_t *addr;

  int paged_offset;
  int paget_offset;
  unsigned long addr_read=read_cr2();

  addr= (virt_addr_t*)&addr_read;

  paged_offset= addr->pd_offset;

  unsigned long pdbr = proctab[currpid].pdbr;

  pd_tab = pdbr + paged_offset*4; // size of one pde = 32 bits

  int frame_id=0;
  int j=0;

  if(pd_tab->pd_pres!=1)
  {
    //create a page table
    get_frm(&frame_id);
    pt_tab=((1024 + frame_id)*NBPG);

    frm_tab[frame_id].fr_status=FRM_MAPPED;
    frm_tab[frame_id].fr_pid=currpid;
    frm_tab[frame_id].fr_type=FR_TBL;

    for( j=0;j<1024;j++)	
		{

      pt_tab[j].pt_global = 1;
      pt_tab[j].pt_avail = 0;
      pt_tab[j].pt_base = 0;
		}

    pd_tab->pd_pres=1;
    pd_tab->pd_write=1;
    pd_tab->pd_base=1024 +frame_id;


}
paget_offset=addr->pt_offset;

pt_tab=(pd_tab->pd_base*NBPG + paget_offset*4);

if(pt_tab->pt_pres!=1)
{
      get_frm(&frame_id);

      if(grpolicy()==SC)
      {
        add_frame_sc(frame_id);
      }
      else
      {
        add_frame_aging(frame_id);
      }
        



    frm_tab[frame_id].fr_status=FRM_MAPPED;
    frm_tab[frame_id].fr_pid=currpid;
    frm_tab[frame_id].fr_type=FR_DIR;
    frm_tab[frame_id].fr_vpno= addr_read/NBPG;

    pt_tab->pt_pres=1;
    pt_tab->pt_write=1;
    pt_tab->pt_base=1024+frame_id;

    //to increment reference count

    int frame_no=pd_tab->pd_base-1024;
    frm_tab[frame_no].fr_refcnt+=1;

    //read from backing store
    int bs_id=0;
    int page=0;

    bsm_lookup(currpid,addr_read,&bs_id,&page);

    read_bs( (char*)((1024 + frame_id) * NBPG), bs_id, page);

}

write_cr3(pdbr);
restore(ps);



  return OK;
}





