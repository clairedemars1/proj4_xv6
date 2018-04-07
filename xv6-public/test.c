#include "types.h"
#include "pstat.h"
#include "user.h"

void assert(int claim){
	if( !claim ){
		printf(1, "Assertion error\n");
		exit();
	}
}

void print_pstat(struct pstat* p){
	//~ int inuse[NPROC];  // whether this slot of the process process table is in use (1 or 0)
    //~ int pid[NPROC];    // the PID of each process
    //~ int hticks[NPROC]; // the number of ticks each process has accumulated at priority 2
    //~ int lticks[NPROC]; // the number of ticks each process has accumulated at priority 1
    int max_print = 10;
    //~ int max_print = NPROC;
    
    int i;
    for (i=0; i<max_print; i++){
		printf(1, "inuse: %d, pid: %d, hticks %d, lticks %d\n",
		 p->inuse[i], 
		 p->pid[i],
		 p->hticks[i],
		 p->lticks[i]);
	}
	printf(1, "\n");
}

void error_tests(){
	//~ assert( getpinfo(0) == -1 ); // messes up
	 
	//~ int i = 3;
	//~ assert( getpinfo( (struct pstat*) &i) == -1); // it does not catch it
	assert( setpri(3) == -1 );
	assert( setpri(0) == -1 );
}

int main(){
	struct pstat procs;
	int ret = getpinfo(&procs);
	printf(1, "return val: %d\n", ret);
	print_pstat(&procs);
	
	error_tests();
	
	exit();
}
