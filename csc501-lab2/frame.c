/* frame.c - manage physical frames */
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>


/*-------------------------------------------------------------------------
 * init_frm - initialize frm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL init_frm()
{
   fr_map_t frm_tab[1024];

  STATWORD 	ps;
	disable(ps);
  int i;
  
  for(i=0; i<NFRAMES;i++)
  {
    frm_tab[i].fr_status=FRM_UNMAPPED;
    frm_tab[i].fr_pid=-1;
    frm_tab[i].fr_vpno=0;
    frm_tab[i].fr_refcnt=0;
    frm_tab[i].fr_type=FR_PAGE;  
    frm_tab[i].fr_dirty=0;

  }
  restore(ps);
  return OK;

}

/*-------------------------------------------------------------------------
 * get_frm - get a free frame according page replacement policy
 *-------------------------------------------------------------------------
 */
SYSCALL get_frm(int* avail)
{
  STATWORD 	ps;
	disable(ps);
  int i;
  for( i=0;i<NFRAMES;i++) // 1024 - 2048 frames 
  {
    if(frm_tab[i].fr_status==FRM_UNMAPPED)
      {
        *avail=i;
        restore(ps);
        return OK;
      }
  }

  // if every frame is already mapped
  int frame_id=0;
  if(grpolicy()==SC)
    frame_id=sc_policy_replacement();
  else
    frame_id=aging_policy_replacement();

  
  free_frm(frame_id);
  *avail=frame_id;


  
  restore(ps);
  return OK;
}

/*-------------------------------------------------------------------------
 * free_frm - free a frame 
 *-------------------------------------------------------------------------
 */
SYSCALL free_frm(int i)
{

  STATWORD ps;
  disable(ps);

  virt_addr_t *addr;
  
  pd_t *pde;
  pt_t *pte;
  unsigned long paget_offset,paged_offset;


  if (frm_tab[i].fr_type==FR_TBL || frm_tab[i].fr_type==FR_DIR)
    {
      restore(ps);
      return SYSERR;

    }
    int pid=frm_tab[i].fr_pid;

    int store_id=proctab[pid].store;
    int num_of_pages= frm_tab[i].fr_vpno-proctab[pid].vhpno;
    
    write_bs((i + 1024) * NBPG,store_id,num_of_pages);

    unsigned long pdbr= proctab[pid].pdbr;
    unsigned long addr_obtd=frm_tab[i].fr_vpno * NBPG;

    addr=(virt_addr_t*)(&addr_obtd);

    paget_offset=addr->pt_offset;
    paged_offset=addr ->pd_offset;
    
    pde = pdbr + paged_offset*4;

    pte=(pde->pd_base*NBPG + paget_offset*4);

    pte -> pt_pres = 0;

    int frame_no=pde->pd_base-1024;
    frm_tab[frame_no].fr_refcnt-=1;

    int ref_cnt=frm_tab[frame_no].fr_refcnt;

    if(ref_cnt<=0)
    {
        frm_tab[frame_no].fr_pid = -1;
				frm_tab[frame_no].fr_status = FRM_UNMAPPED;
				frm_tab[frame_no].fr_type = FR_PAGE;
				frm_tab[frame_no].fr_vpno = VAddr_Base;
				pde->pd_pres = 0;
    }

    restore(ps);



  return OK;
}

