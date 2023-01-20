
/* sched.c - getschedclass, setschedclass, getnextproc, getmaxgoodproc, updateprocgoodness, dequeuenextproc */

#include <kernel.h>
#include <q.h>
#include <proc.h>
#include <sched.h>

int currentSchedPolicy = 0;

int getschedclass()
{
    return currentSchedPolicy;
}

void setschedclass (int sched_class) 
{ 
    currentSchedPolicy = sched_class;
}

int getnextproc(int num)
{
    int next = q[rdytail].qprev, prev = next;

    for(; (num < q[next].qkey) && (next < NPROC); next = q[next].qprev){
        if(q[next].qkey != q[prev].qkey)
        {
            prev = next;
        }
    }

    return (next >= NPROC) ? 0 : prev;
}

void getmaxgoodproc(int *next, int *max)
{
    int i = q[rdytail].qprev;
    
    do
    {
        if(proctab[i].goodness > *max){
            *next = i;
            *max = proctab[i].goodness;
        }
        i = q[i].qprev;
    }while(i != rdyhead);
}

void updateprocgoodness()
{
    int pid = 0;
    struct pentry *proc;

    while(pid < NPROC && NPROC > 0)
    {
        proc = &proctab[pid];
        if(proc->pstate != PRFREE)
        {
            proc->counter = proc->pprio + (proc->counter) / 2;
            proc->goodness = proc->pprio + proc->counter;
        }
        ++pid;
    }

}

struct pentry* dequeuenextproc(int pid)
{
    currpid = pid;
    struct pentry *nptr = &proctab[pid];
    nptr->pstate = PRCURR;
    dequeue(currpid);

    return nptr;
}