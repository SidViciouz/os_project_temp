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
	  kernel_exit(-1);
  if(syscall_no == SYS_HALT){
	  shutdown_power_off();
  }
  else if(syscall_no == SYS_EXIT){
	  if(!is_user_vaddr(f->esp + 4))
		  kernel_exit(-1);
	kernel_exit(*(int*)(f->esp + 4));
  }
  else if(syscall_no == SYS_EXEC){
	  if(!is_user_vaddr(f->esp + 4))
		  kernel_exit(-1);
	 f->eax = process_execute(*(int*)(f->esp + 4));
  }
  else if(syscall_no == SYS_WAIT){
	  if(!is_user_vaddr(f->esp + 4))
		  kernel_exit(-1);
	f->eax = process_wait(*(int*)(f->esp + 4));
  }
  else if(syscall_no == SYS_READ){
	  if(!is_user_vaddr(f->esp + 4) || !is_user_vaddr(f->esp + 4) || !is_user_vaddr(f->esp + 4))
		  kernel_exit(-1);
	unsigned size = *(unsigned*)(f->esp + 12);
	int* buffer = *(int*)(f->esp + 8);
	int fd = *(int*)(f->esp + 4);
	
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
		f->eax = size; 
	}
	else
		f->eax = -1;

  }
  else if(syscall_no == SYS_WRITE){
	  if(!is_user_vaddr(f->esp + 4) || !is_user_vaddr(f->esp + 4) || !is_user_vaddr(f->esp + 4))
		  kernel_exit(-1);
	unsigned size = *(unsigned*)( f->esp + 12);
	int buffer = *(int*)(f->esp + 8);
	int fd = *(int*)(f->esp + 4);

	if(fd == 1){
		putbuf((const char*)buffer,size);
		f->eax = size;
	}
	else
		f->eax = -1;
  }
  //thread_exit ();
}

void kernel_exit(int exit_number){

  thread_current()->exit_number = exit_number;
  printf("%s: exit(%d)\n",thread_current()->name,exit_number);
  thread_exit();

}
