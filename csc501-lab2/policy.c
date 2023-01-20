/* policy.c = srpolicy*/

#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <proc.h>


extern int page_replace_policy;
/*-------------------------------------------------------------------------
 * srpolicy - set page replace policy 
 *-------------------------------------------------------------------------
 */
SYSCALL srpolicy(int policy)
{
  STATWORD ps;
  disable(ps);
  /* sanity check ! */
  if (policy!= 4 || policy !=3)
    return SYSERR;
  
  page_replace_policy=policy;

  restore(ps);

  return OK;
}

/*-------------------------------------------------------------------------
 * grpolicy - get page replace policy 
 *-------------------------------------------------------------------------
 */
SYSCALL grpolicy()
{
  return page_replace_policy;
}

void srpolicy_init()
{
  int i=0;
 

  for(i=0;i<NFRAMES;i++)
  {
    sc_frames[i].frame_id=i;
    sc_frames[i].next=-1;
  }
}
void agingpolicy_init()
{
  int i=0;
  for(i=0;i<NFRAMES;i++)
  {
    aging_frames[i].frame_id=i;
    aging_frames[i].next=-1;
    aging_frames[i].age=0;
  }
}

void add_frame_sc(int frame_id)
{
  int curr_frame=qhead_sc;
  if (qhead_sc==-1)
  {  
    qhead_sc=frame_id;
    return OK;

  }
  else
  {
    while(sc_frames[curr_frame].next!=-1)
    {
      curr_frame=sc_frames[curr_frame].next;
    }

    sc_frames[curr_frame].next=frame_id;
    sc_frames[frame_id].next= -1;
    return OK;

  }
  
}

void add_frame_aging(int frame_id)
{
  int curr_frame=qhead_aging;

  if (qhead_aging==-1)
  {
    qhead_aging=frame_id;
    return OK;

  }
  else
  {
    while(aging_frames[curr_frame].next!=-1)
    {
      curr_frame=aging_frames[curr_frame].next;
    }
    aging_frames[curr_frame].next=frame_id;
    aging_frames[frame_id].next=-1;

  }
}

int sc_policy_replacement()
{
  pd_t * pde;
  pt_t *pte;
  int curr_frame= qhead_sc;

  int prev = -1;
  virt_addr_t *addr;
  //unsigned long prev;

  while(curr_frame!=-1)
  {
    
    unsigned long pdbr= proctab[currpid].pdbr;
    unsigned long addr_obtd=frm_tab[curr_frame].fr_vpno * NBPG;

    addr=(virt_addr_t*)(&addr_obtd);

    unsigned long paget_offset=addr->pt_offset;
    unsigned long paged_offset=addr ->pd_offset;
    
    pde = pdbr + paged_offset * 4;

    pte=(pde->pd_base*NBPG + paget_offset*4);


    //temp = qhead_sc;
    if(!pte->pt_acc)
    {
     
      if(qhead_sc == curr_frame)
      {
        qhead_sc=sc_frames[curr_frame].next;
        sc_frames[curr_frame].next = -1;
        return curr_frame;
      }
      else
      {
        sc_frames[prev].next = sc_frames[curr_frame].next;
        sc_frames[curr_frame].next=-1;
        return curr_frame;
      }
    

    }
    else
      pte->pt_acc=0;

    
    prev=curr_frame;


    curr_frame=sc_frames[curr_frame].next;
  
  }

  //no free frame found yet
  curr_frame =qhead_sc;
  qhead_sc=sc_frames[qhead_sc].next;
  sc_frames[curr_frame].next=-1;
  return curr_frame;
}

int aging_policy_replacement()
{
  pd_t * pde;
  pt_t *pte;
  int curr_frame= qhead_aging;

  int prev = -1;
  virt_addr_t *addr;
  int min_age = 255;
  int prev_element=-1;
  int i;

  int temp = qhead_aging;
  for(i=qhead_aging;i<NFRAMES;i++)
  {
    aging_frames[i].age=aging_frames[i].age/2;
  }
  while(curr_frame!=-1)
  {
    
    unsigned long pdbr= proctab[currpid].pdbr;
    unsigned long addr_obtd=frm_tab[curr_frame].fr_vpno * NBPG;

    addr=(virt_addr_t*)(&addr_obtd);

    unsigned long paget_offset=addr->pt_offset;
    unsigned long paged_offset=addr->pd_offset;
    
    pde = pdbr + paged_offset * 4;

    pte=(pde->pd_base*NBPG + paget_offset*4);

    if(pte->pt_acc)
    {
      if(aging_frames[curr_frame].age+128<255)
        aging_frames[curr_frame].age+=128;
      else
        aging_frames[curr_frame].age=255;
    }

    if(aging_frames[curr_frame].age < aging_frames[temp].age )
    {
      prev_element=prev;
      temp=curr_frame;
    }
    prev=curr_frame;
    curr_frame=aging_frames[curr_frame].next;
    }
    if(qhead_aging==temp)
    {
      curr_frame=qhead_aging;
      qhead_aging=aging_frames[curr_frame].next;
      aging_frames[curr_frame].next=-1;
      return curr_frame;
    }
    else
    {
      aging_frames[prev_element].next=aging_frames[curr_frame].next;
      aging_frames[curr_frame].next=-1;
      return curr_frame;

    }
}


