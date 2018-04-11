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
    int max_print = 10;
    //~ int max_print = NPROC;
    
    int i;
    for (i=0; i<max_print; i++){
		printf(1, "pid: %d, inuse: %d, priority: %d, hticks %d, lticks %d\n",
		 p->pid[i],
		 p->inuse[i],
		 p->priority[i],
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

void basic_test(){
	struct pstat procs;
	int ret = getpinfo(&procs);
	printf(1, "return val: %d\n", ret);
	print_pstat(&procs);
	
	setpri(2);
}

int main(){
	basic_test();
	
	//~ error_tests();
	
	exit();
}
