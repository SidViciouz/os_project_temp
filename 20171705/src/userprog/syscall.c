#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f) 
{
  int syscall_no = *(int*)(f->esp);

  if(!is_user_vaddr(f->esp))
	  exit(-1);
  if(syscall_no == SYS_HALT){
	  halt();
  }
  else if(syscall_no == SYS_EXIT){
	  if(!is_user_vaddr(f->esp + 4))
		  exit(-1);
	exit(*(int*)(f->esp + 4));
  }
  else if(syscall_no == SYS_EXEC){
	  if(!is_user_vaddr(f->esp + 4))
		  exit(-1);
	 f->eax = exec(*(int*)(f->esp + 4));
  }
  else if(syscall_no == SYS_WAIT){
	  if(!is_user_vaddr(f->esp + 4))
		  exit(-1);
	f->eax = wait(*(int*)(f->esp + 4));
  }
  else if(syscall_no == SYS_READ){
	  if(!is_user_vaddr(f->esp + 4) || !is_user_vaddr(f->esp + 8) || !is_user_vaddr(f->esp + 12))
		  exit(-1);
	f->eax = read(*(int*)(f->esp+4),*(int*)(f->esp+8),*(unsigned*)(f->esp+12));
  }
  else if(syscall_no == SYS_WRITE){
	  if(!is_user_vaddr(f->esp + 4) || !is_user_vaddr(f->esp + 8) || !is_user_vaddr(f->esp + 12))
		  exit(-1);
	f->eax = write(*(int*)(f->esp + 4),*(int*)(f->esp+8),*(unsigned*)(f->esp+12));
  }
  else if(syscall_no == SYS_FIBONACCI){
	if(!is_user_vaddr(f->esp + 4))
		exit(-1);
	f->eax = fibonacci(*(int*)(f->esp + 4));

  }
  else if(syscall_no == SYS_MAXOFFOURINT){
	  if(!is_user_vaddr(f->esp + 4) || !is_user_vaddr(f->esp + 8) || !is_user_vaddr(f->esp + 12) ||
			  !is_user_vaddr(f->esp + 16))
		  exit(-1);
	f->eax = max_of_four_int(*(int*)(f->esp+4),*(int*)(f->esp+8),*(int*)(f->esp+12),*(int*)(f->esp+16));
  }

  //thread_exit ();
}

void halt(){
  shutdown_power_off();
}
void exit(int exit_number){

  thread_current()->exit_number = exit_number;
  printf("%s: exit(%d)\n",thread_current()->name,exit_number);
  thread_exit();

}
int exec(const char* filename){
	 return  process_execute(filename);
}
int wait(int pid){
	return  process_wait(pid);
}
int read(int fd,int *buffer,unsigned size){

	if(fd == 0){
		for(int i=0; i<size; i++){
			*buffer = input_getc();
			if(*buffer != '\0')
				buffer++;
			else{ 
				size = i;
				break;
			}
		}
		return size; 
	}
	else
		return -1;
}
int write(int fd,int *buffer,unsigned size){
	if(fd == 1){
		putbuf((const char*)buffer,size);
		return size;
	}
	else
		return -1;
}
int fibonacci(int n){
	if( n <= 0)
		return -1;
	else if(n <= 2)
		return 1;
	else 
		return fibonacci(n-1) + fibonacci(n-2);
}

int max_of_four_int(int a,int b,int c,int d){
	int retval = a;
	if(retval < b)
		retval = b;
	if(retval < c)
		retval = c;
	if(retval < d)
		retval = d;

	return retval;
}
