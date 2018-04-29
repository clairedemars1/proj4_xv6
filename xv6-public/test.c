#include "types.h"
#include "pstat.h"
#include "user.h"
#include "fcntl.h"

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

void helper_get_and_print_pstat(){
	struct pstat procs;
	getpinfo(&procs);
	print_pstat(&procs);
}

void high_priority_runs_first(){
	int pid = fork();
	if (pid == 0){ // child
		setpri(2);
		sleep(5);  // parent should wait, since parent is lower priority
		printf(1, "high priority done (should happen first)\n");
		exit();
	} else { // parent
		sleep(2); // sleep, to give child a chance to be given higher priority
		//~ helper_get_and_print_pstat();
		printf(1, "LOW PRIORITY DONE (SHOULD HAPPEN SECOND) \n");
		wait();
	}
	
	// failing test case: parent and child take turns running, implying scheduling did not happen
}

void make_file(){
	int fd = open("foo.txt", O_CREATE );
	char* msg = "Hello";
	int ret;
	if ( ret = write(fd, msg, 6) != 6 ){
		printf(1, "write failed: %d\n", ret);
	}
	char out[6];
	read(fd, out, 6);
	printf(1, "i read %s", out);
	close(fd);
}

char buf[8192];
int stdout=1;
void
writetest1(void)
{
  int i, fd, n;

  printf(stdout, "big files test\n");

  fd = open("big", O_CREATE|O_RDWR);
  if(fd < 0){
    printf(stdout, "error: creat big failed!\n");
    exit();
  }
  int size = 10;
  for(i = 0; i < size; i++){
    ((int*)buf)[0] = i;
    if(write(fd, buf, 512) != 512){
      printf(stdout, "error: write big file failed\n", i);
      exit();
    }
  }

  close(fd);

  fd = open("big", O_RDONLY);
  if(fd < 0){
    printf(stdout, "error: open big failed!\n");
    exit();
  }

  n = 0;
  for(;;){
    i = read(fd, buf, 512);
    if(i == 0){
      if(n == size - 1){
        printf(stdout, "read only %d blocks from big", n);
        exit();
      }
      break;
    } else if(i != 512){
      printf(stdout, "read failed %d\n", i);
      exit();
    }
    if(((int*)buf)[0] != n){
      printf(stdout, "read content of block %d is %d\n",
             n, ((int*)buf)[0]);
      exit();
    }
    n++;
  }
  close(fd);
  if(unlink("big") < 0){
    printf(stdout, "unlink big failed\n");
    exit();
  }
  printf(stdout, "big files ok\n");
}

int main(){
	// testing scheduler
	//~ return_vals();
	//~ set_priority_and_track_lticks_hticks();
	//~ set_priority_to_value_it_already_had();
	//~ high_priority_runs_first();

	// testing integrity
	//~ make_file(); // couldn't get to work, even on an old project
	writetest1(); 
	exit();
}
