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
  else if(syscall_no == SYS_CREATE){
	if(!is_user_vaddr(f->esp+4) || !is_user_vaddr(f->esp + 8))
		exit(-1);

	  f->eax = create(*(const char**)(f->esp+4),*(unsigned*)(f->esp+8));
  }
  else if(syscall_no == SYS_REMOVE){
	  if(!is_user_vaddr(f->esp + 4))
		  exit(-1);
	  f->eax = remove(*(const char**)(f->esp+4));
  }
  else if(syscall_no == SYS_OPEN){
	  if(!is_user_vaddr(f->esp + 4))
		  exit(-1);
	  f->eax = open(*(const char**)(f->esp+4));
  }
  else if(syscall_no == SYS_FILESIZE){
	  if(!is_user_vaddr(f->esp + 4))
		  exit(-1);
	  f->eax = filesize(*(int*)(f->esp+4));
  }
  else if(syscall_no == SYS_SEEK){
	  if(!is_user_vaddr(f->esp + 4)||!is_user_vaddr(f->esp+8))
		  exit(-1);
	  seek(*(int*)(f->esp+4),*(int*)(f->esp+8));
  }
  else if(syscall_no == SYS_TELL){
	  if(!is_user_vaddr(f->esp+4))
		 exit(-1);
	  f->eax = tell(*(int*)(f->esp+4));
  }
  else if(syscall_no == SYS_CLOSE){
	  if(!is_user_vaddr(f->esp + 4))
		  exit(-1); 
	  close(*(int*)(f->esp+4));
  }
  //thread_exit ();
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
	if(!is_user_vaddr(buffer))
		exit(-1);
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
	else{
		struct list_elem* elem;
		struct list_item* item;
		for(elem = list_begin(&(thread_current()->file_list));
			elem != list_end(&(thread_current()->file_list)); elem = list_next(elem)){
			if((item = list_entry(elem,struct list_item,elem))->fd == fd)
				return file_read(item->f,buffer,size);
		}
		return 0;
	}
}
int write(int fd,int *buffer,unsigned size){
	if(!is_user_vaddr(buffer))
		exit(-1);
	if(fd == 1){
		putbuf((const char*)buffer,size);
		return size;
	}
	else{
		struct list_elem* elem;
		struct list_item* item;
		unsigned w_size = 0;
		for(elem = list_begin(&(thread_current()->file_list));
			elem != list_end(&(thread_current()->file_list)); elem = list_next(elem)){
			if((item = list_entry(elem,struct list_item,elem))->fd == fd)
				file_allow_write(item->f);
				w_size = file_write(item->f,buffer,size);
				return w_size;
		}
		return 0;
	}
	
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
int create(const char* file, unsigned initial_size){
	if(file == NULL)
		exit(-1);
	return filesys_create(file,initial_size);
}
int remove(const char* file){
	return filesys_remove(file);
}
int open(const char* file){
	if(file == NULL)
		return -1;
	struct file* f = filesys_open(file);
	int fd = -1;
	if(f == NULL)
		return -1;
	
	if(thread_current()->file_bitmap == NULL){
		thread_current()->file_bitmap = bitmap_create(64);
		bitmap_set(thread_current()->file_bitmap,0,true);
		bitmap_set(thread_current()->file_bitmap,1,true);
	}
	
	for(int i=2; i<=63; i++){
		if(bitmap_count(thread_current()->file_bitmap,i,1,false) == 1){
			fd = i;
			bitmap_set(thread_current()->file_bitmap,i,true);
			break;
		}
	}
	struct list_item* item = (struct list_item*)malloc(sizeof(struct list_item));
	item->fd = fd;
	item->f = f;
	list_push_back(&(thread_current()->file_list),&(item->elem));
	file_deny_write(item->f);
	return fd;
}
int filesize(int fd){
	if(bitmap_count(thread_current()->file_bitmap,fd,1,false) == 1)
		return 0;
	struct list_elem* elem;
	struct list_item* item;
	for(elem = list_begin(&(thread_current()->file_list));
		elem != list_end(&(thread_current()->file_list)); elem = list_next(elem)){
		if((item = list_entry(elem,struct list_item,elem))->fd == fd)
			return file_length(item->f);
	}
	return 0;
}
void seek(int fd, unsigned position){
	if(bitmap_count(thread_current()->file_bitmap,fd,1,false) == 1)
		return ;
	struct list_elem* elem;
	struct list_item* item;
	for(elem = list_begin(&(thread_current()->file_list));
		elem != list_end(&(thread_current()->file_list)); elem = list_next(elem)){
		if((item = list_entry(elem,struct list_item,elem))->fd == fd){
			file_seek(item->f,position);
			break;
		}
	}

	return;
}
unsigned tell(int fd){
	if(bitmap_count(thread_current()->file_bitmap,fd,1,false) == 1)
		return 0;
	struct list_elem* elem;
	struct list_item* item;
	for(elem = list_begin(&(thread_current()->file_list));
		elem != list_end(&(thread_current()->file_list)); elem = list_next(elem)){
		if((item = list_entry(elem,struct list_item,elem))->fd == fd){
			return file_tell(item->f);
		}
	}
	return 0;
}
void close(int fd){
	if(thread_current()->file_bitmap == NULL){
		thread_current()->file_bitmap = bitmap_create(64);
		bitmap_set(thread_current()->file_bitmap,0,true);
		bitmap_set(thread_current()->file_bitmap,1,true);
	}

	//temporary code in project 2
	if(fd == 0 || fd == 1){
		bitmap_set(thread_current()->file_bitmap,fd,false);
	}
	//
	struct list_elem* elem;
	struct list_item* item;
	for(elem = list_begin(&(thread_current()->file_list));
		elem != list_end(&(thread_current()->file_list)); elem = list_next(elem)){
		if((item = list_entry(elem,struct list_item,elem))->fd == fd){
			file_close(item->f);
			bitmap_set(thread_current()->file_bitmap,item->fd,false);
			list_remove(elem);
			break;
		}
	}
	return;
}
