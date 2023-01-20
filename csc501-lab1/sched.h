#define EXPDISTSCHED 1
#define LINUXSCHED 2

int getschedclass();
void setschedclass (int sched_class);
int getnextproc(int num);
void getmaxgoodproc(int *next, int *max);
void updateprocgoodness();
struct pentry * dequeuenextproc(int pid);


extern int currentSchedPolicy;


