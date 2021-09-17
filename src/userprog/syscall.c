#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

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
  if(syscall_no == SYS_EXIT){
	thread_exit();
	f->eax = *(int*)(f->esp + 4);
	printf("SYS_EXIT!\n");
  }
  else if(syscall_no == SYS_WRITE){
	unsigned size = *(unsigned*)( f->esp + 12);
	int buffer = *(int*)(f->esp + 8);
	int fd = *(int*)(f->esp + 4);

	if(fd == 1){
		putbuf((const char*)buffer,size);
	}
	
	printf("SYS_WRITE!\n");	
  }
  printf ("system call!\n");
  thread_exit ();
}
