#include "types.h"
#include "pstat.h"
#include "user.h"

void assert(int claim){
	if( !claim ){
		printf(1, "Assertion error\n");
	}
}

void print_pstat(struct pstat* p){
    int max_print = 6; // just print the ones you're using
    
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

void return_vals(){
	// getpinfo
	//~ assert( getpinfo(0) == -1 ); // messes up
	//~ int i = 3;
	//~ assert( getpinfo( (struct pstat*) &i) == -1); // works fine, but maybe shouldn't work
	struct pstat procs;
	assert( getpinfo(&procs) == 0);
	
	//setpri
	assert( setpri(3) == -1 );
	assert( setpri(0) == -1 );
	assert( setpri(2) == 0 );
	assert( setpri(1) == 0 ); // back to 1
}

void set_priority_and_track_lticks_hticks(){
	struct pstat procs;
	int ret = getpinfo(&procs);
	printf(1, "return val: %d\n", ret);
	print_pstat(&procs);
	
	setpri(2);
	
	ret = getpinfo(&procs);
	printf(1, "return val: %d\n", ret);
	print_pstat(&procs);
	
	setpri(1); // clean slate for other tests
}

void set_priority_to_value_it_already_had(){
	setpri(1);
	setpri(1);
	
	struct pstat procs;
	int ret = getpinfo(&procs);
	printf(1, "return val: %d\n", ret);
	printf(1, "priority should be 1\n");
	print_pstat(&procs);
}

void high_priority_runs_first(){
	int pid = fork();
	if (pid == 0){ // child
		setpri(2);
		printf(1, "I am high priority: I'm going to to sleep, and low priority has to wait.\n");
		sleep(5);
		printf(1, "high priority done\n");
		exit();
	} else { // parent
		printf(1, "LOW PRIORITY DONE\n");
	}
	
	// failing test case: parent runs before child, and exits, and we have a zombie process
}

int main(){
	return_vals();
	set_priority_and_track_lticks_hticks();
	set_priority_to_value_it_already_had();
	high_priority_runs_first();

	exit();
}
