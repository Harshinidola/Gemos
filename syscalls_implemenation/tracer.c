#include<context.h>
#include<memory.h>
#include<lib.h>
#include<entry.h>
#include<file.h>
#include<tracer.h>


///////////////////////////////////////////////////////////////////////////
//// 		Start of Trace buffer functionality 		      /////
///////////////////////////////////////////////////////////////////////////

int is_valid_mem_range(unsigned long buff, u32 count, int access_bit) 
{
     struct exec_context* ctx = get_current_ctx();
	 int p;
     struct vm_area* vmarea=ctx->vm_area;
	 while(vmarea!=NULL){
	 if(buff>=vmarea->vm_start&&(buff+count-1)<=vmarea->vm_end-1)
	  {
		//printk("uu");
		p=vmarea->access_flags;
		p=p%4;
		//printk(" p is %x start %x end %x ",vmarea->access_flags,vmarea->vm_start,vmarea->vm_end);
		if(access_bit==0)//read call
		{
			if(p==0||p==1)
			{
				//printk("1");
				return 0;
			}
		else
		   {
				//printk("2");
				return 1;
			}
		}
		else //write call
		{
           if(p==0||p==2)
		  {
				//printk("3");
				return 0;
			}
		else
		 {
				//printk("4");
				return 1;
			}
		}
	  }
	  vmarea=vmarea->vm_next;
	 }
	struct mm_segment* ms=ctx->mms;
	for(int i=0;i<MAX_MM_SEGS;i++){
		if(i==MM_SEG_CODE){
			if(buff>=ms[i].start&&(buff+count-1)<=(ms[i].next_free-1))
			{
              if(access_bit==0)  
				{
				//printk("5");
				return 0;
			}
			  else 
			   {
				//printk("6");
				return 1;
			}
			
		}
		}
		else if(i==MM_SEG_RODATA){
			if(buff>=ms[i].start&&(buff+count-1)<=(ms[i].next_free-1))
			{
              if(access_bit==0)
			  {
				//printk("7");
				return 0;
			}
			  else 
			   {
				//printk("8");
				return 1;
			}
			}
		}
		else if(i==MM_SEG_DATA){
			if(buff>=ms[i].start&&(buff+count-1)<=(ms[i].next_free-1))
			 {
				//printk("9");
				return 1;
			}
		}
		else if(i==MM_SEG_STACK){
			if(buff>=ms[i].start&&(buff+count-1)<=ms[i].end)
			 {
				//printk("10");
				return 1;
			}
		}
		else
		{
			if(buff>=ms[i].start&&(buff+count-1)<=(ms[i].next_free-1))
			 {
		p=ms[i].access_flags;
		p=p%4;
		if(access_bit==0)//read call
		{
			if(p==0||p==1)
			{
				//printk("11");
				return 0;
			}
		else
		    return 1;
		}
		else //write call
		{
           if(p==0||p==2)
		 {
				//printk("12");
				return 0;
			}
		else
		 {
				//printk("13");
				return 1;
			}
		}
	  }
		}
	}
	//printk("14");
	return 0;
}



long trace_buffer_close(struct file *filep)
{
	os_page_free(USER_REG,filep->trace_buffer->data);
	if(filep->trace_buffer==NULL)return -EINVAL;
    else os_page_free(USER_REG, filep->trace_buffer);
	if(filep->fops==NULL)return -EINVAL;
    else os_page_free(USER_REG, filep->fops);
	if(filep==NULL)return -EINVAL;
    else os_page_free(USER_REG, filep);
	return 0;	
}



int trace_buffer_read(struct file *filep, char *buff, u32 count)
{
	//printk(" read started ");
	if(count<0)return -EINVAL;
    if(count==0)return 0;
	//if(is_valid_mem_range((unsigned long)buff,count,0)==0)return -EBADMEM;
  //printk(" writeoff %x readoff %x ",filep->trace_buffer->writeoffset,filep->trace_buffer->readoffset);
	if(filep->trace_buffer->num==0)return 0;
	int i=0,j;
    if(filep->trace_buffer->mode!=O_READ && filep->trace_buffer->mode!=O_RDWR)return -EINVAL;
	if(filep->trace_buffer->writeoffset>filep->trace_buffer->readoffset){
		//printk("1");
		for( j=filep->trace_buffer->readoffset;j<filep->trace_buffer->writeoffset;j++){
	     //printk("2");
			buff[i++]=filep->trace_buffer->data[j];
            filep->trace_buffer->space+=1;
            filep->trace_buffer->num-=1;
			if(i==count){
				filep->trace_buffer->readoffset=(j+1)%4096;
				//printk("3");
				return i;
			}
		}
		//printk("4");
		filep->trace_buffer->readoffset=(j+1)%4096;
	}
	else {
		for( j=filep->trace_buffer->readoffset;j<4096;j++){
		
		buff[i++]=filep->trace_buffer->data[j];
         filep->trace_buffer->space+=1;
            filep->trace_buffer->num-=1;
		if(i==count){
				filep->trace_buffer->readoffset=(j+1)%4096;
				return i;
			}
		}
		for( j=0;j<filep->trace_buffer->writeoffset;j++){
			
		buff[i++]=filep->trace_buffer->data[j];
         filep->trace_buffer->space+=1;
            filep->trace_buffer->num-=1;
		if(i==count){
				filep->trace_buffer->readoffset=(j+1)%4096;
				return i;
			}
		}
				filep->trace_buffer->readoffset=(j+1)%4096;
	}
	if(i>=0)
	return i;
else return -EINVAL;
	//return 0;
}


int trace_buffer_write(struct file *filep, char *buff, u32 count)
{
	//printk(" write started ");
	if(count<0)return -EINVAL;
	//if(is_valid_mem_range((unsigned long)buff,count,1)==0)return -EBADMEM;
	//printk("1");
	if(count==0)return 0;
	int i=0,j;
    if(filep->trace_buffer->space==0)return 0;
    if(filep->trace_buffer->mode!=O_WRITE && filep->trace_buffer->mode!=O_RDWR)return -EINVAL;
	if(filep->trace_buffer->writeoffset>=filep->trace_buffer->readoffset){
		for( j=filep->trace_buffer->writeoffset;j<4096;j++){
			//printk("2");
			filep->trace_buffer->data[j]=buff[i++];
             filep->trace_buffer->space-=1;
            filep->trace_buffer->num+=1;
			if(i==count){
				filep->trace_buffer->writeoffset=(j+1)%4096;
             // printk(" 217 off %x ",filep->trace_buffer->writeoffset);
				return i;
			}
		}
		for( j=0;j<filep->trace_buffer->readoffset;j++){
			//printk("3");
			filep->trace_buffer->data[j]=buff[i++];
             filep->trace_buffer->space-=1;
            filep->trace_buffer->num+=1;
			if(i==count){
				filep->trace_buffer->writeoffset=(j+1)%4096;
				return i;
			}
		}
		filep->trace_buffer->writeoffset=(j+1)%4096;
	}
	else {
		for( j=filep->trace_buffer->writeoffset;j<filep->trace_buffer->readoffset;j++){
			//printk("4");
		filep->trace_buffer->data[j]=buff[i++];
         filep->trace_buffer->space-=1;
            filep->trace_buffer->num+=1;
		if(i==count){
				filep->trace_buffer->writeoffset=(j+1)%4096;
				return i;
			}
		}
		filep->trace_buffer->writeoffset=(j+1)%4096;
	}
 // printk(" %x %x write done ",i,filep->trace_buffer->writeoffset);
	if(i>=0)
	return i;
else return -EINVAL;
    	//return 0;
}

int sys_create_trace_buffer(struct exec_context *current, int mode)
{
	if(mode!=O_READ && mode!=O_WRITE && mode!=O_RDWR)return -EINVAL;
	int fd=-1;
	for(int i=0;i<MAX_OPEN_FILES;i++){
		if(current->files[i]==NULL){
			fd=i;
			break;
		}
	}
	if(fd==-1)return -EINVAL;
	struct file* fobj=(struct file*)os_page_alloc(USER_REG);
	current->files[fd]=fobj;
	struct trace_buffer_info* tbobj=(struct trace_buffer_info*)os_page_alloc(USER_REG);
	//mem allocation errors
	tbobj->readoffset=0;
	tbobj->writeoffset=0;
    tbobj->num=0;
    tbobj->space=4096;
	tbobj->data=(char *)os_page_alloc(USER_REG);
	tbobj->mode=mode;
	fobj->mode=mode;
	fobj->inode=NULL;
	fobj->trace_buffer=tbobj;
	fobj->type=TRACE_BUFFER;
	
	fobj->ref_count+=1;
	struct fileops* tbops=(struct fileops*)os_page_alloc(USER_REG);
	fobj->fops=tbops;
	tbops->read=trace_buffer_read;
	tbops->write=trace_buffer_write;
	return fd;

	//return 0;
}

///////////////////////////////////////////////////////////////////////////
//// 		Start of strace functionality 		      	      /////
///////////////////////////////////////////////////////////////////////////

void gf(char* a,u64 num){
	for(int i=0;i<8;i++)a[i]=(num>>(8*i)) & 0xFF;
}
void bf(char* b,u64* num){
	*num=0;
	for(int i=0;i<8;i++)*num|=(u64)b[i]<<(8*i);
}
int perform_tracing(u64 syscall_num, u64 param1, u64 param2, u64 param3, u64 param4)
{
	 int j,i;
	struct exec_context* ctx = get_current_ctx();
  if(ctx->st_md_base==NULL)return 0;
  if(syscall_num==38||syscall_num==37)return 0;
 if(ctx->st_md_base->is_traced==0){
       return -EINVAL;
    }
	if(ctx->st_md_base->tracing_mode==FILTERED_TRACING){
		struct strace_info* head=ctx->st_md_base->next;
		int k=0;
		while(head!=NULL){
       if(head->syscall_num==syscall_num){
		k=1;
		break;
	   }
	   else {
		head=head->next;
	   }
		}
		if(k==0)return 0;
	}
	int fd=ctx->st_md_base->strace_fd;
	// char* a=(char *)os_page_alloc(USER_REG);
	  //ctx->files[fd]->trace_buffer->data=a;
	if(!((syscall_num>=1&&syscall_num<=30&&syscall_num!=3&&syscall_num!=26)||(syscall_num>=35&&syscall_num<=41)||syscall_num==61))
	  return -EINVAL;
  int p;
	 if(syscall_num==2||syscall_num==10||syscall_num==11||syscall_num==13||syscall_num==15||syscall_num==20||syscall_num==21||syscall_num==22||syscall_num==38||syscall_num==61)
	 p=0;
	else if(syscall_num==1||syscall_num==6||syscall_num==7||syscall_num==12||syscall_num==14||syscall_num==19||syscall_num==27||syscall_num==29||syscall_num==36)
	p=1;
    else if(syscall_num==4||syscall_num==8||syscall_num==9||syscall_num==17||syscall_num==23||syscall_num==28||syscall_num==37||syscall_num==40)
	p=2;
    else if(syscall_num==5||syscall_num==18||syscall_num==24||syscall_num==25||syscall_num==30||syscall_num==39||syscall_num==41)
	p=3;
    else if(syscall_num==16||syscall_num==35)
	p=4;
else
return -EINVAL;
char c[8];
    if(p==0)
	{
      gf(c,syscall_num);
		trace_buffer_write(ctx->files[fd],c,8);
	}
    else if(p==1){
		gf(c,syscall_num);
		trace_buffer_write(ctx->files[fd],c,8);
		gf(c,param1);
		trace_buffer_write(ctx->files[fd],c,8);
	}
	else if(p==2){
		gf(c,syscall_num);
		trace_buffer_write(ctx->files[fd],c,8);
		gf(c,param1);
		trace_buffer_write(ctx->files[fd],c,8);
		gf(c,param2);
		trace_buffer_write(ctx->files[fd],c,8);
	}
	else if(p==3){
		gf(c,syscall_num);
		trace_buffer_write(ctx->files[fd],c,8);
		gf(c,param1);
		trace_buffer_write(ctx->files[fd],c,8);
		gf(c,param2);
		trace_buffer_write(ctx->files[fd],c,8);
		gf(c,param3);
		trace_buffer_write(ctx->files[fd],c,8);
	}
	else if(p==4){
		gf(c,syscall_num);
		trace_buffer_write(ctx->files[fd],c,8);
		gf(c,param1);
		trace_buffer_write(ctx->files[fd],c,8);
		gf(c,param2);
		trace_buffer_write(ctx->files[fd],c,8);
		gf(c,param3);
		trace_buffer_write(ctx->files[fd],c,8);
		gf(c,param4);
		trace_buffer_write(ctx->files[fd],c,8);
	}
    //  for(i=0;i<=p;i++){
	// 	if(i==0) {
	// 		for(int m=0;m<8;m++)b[m]=syscall_num>>(8-1-i)*8;
	// 	}
	// 	if(i==1)a[j]=param1;
	// 	else if(i==2)a[j]=param2;
	// 	else if(i==3)a[j]=param3;
	// 	else if(i==4)a[j]=param4;
	// 	j=(j+1)%4096;
	//  }
	//  ctx->files[fd]->trace_buffer->writeoffset=j;
//   printk(" 345 ");
//   printk(" p %x c %x ",p,c);
	//if(c<(p+1))return -EINVAL;
 // printk(" perform ended ");
    return 0;
}


int sys_strace(struct exec_context *current, int syscall_num, int action)
{
	if(action!=ADD_STRACE && action!=REMOVE_STRACE)return -EINVAL;
	struct strace_head* stmdbase=current->st_md_base;
	struct strace_info* head=stmdbase->next;
	if(action==ADD_STRACE){
        int k=0;
        while(head!=NULL){
            if(head->syscall_num==syscall_num){
                k=1;
                break;
            }
            else
            head=head->next;
        }
        if(k==0)return 0;
		if(stmdbase->last==NULL){
      stmdbase->next=(struct strace_info*)os_page_alloc(USER_REG);
	  stmdbase->next->syscall_num=syscall_num;
	  stmdbase->next->next=NULL;
	  stmdbase->last=stmdbase->next;
		}
		else {
		stmdbase->last->next=(struct strace_info*)os_page_alloc(USER_REG);
		if(stmdbase->last->next==NULL)return -EINVAL;
		stmdbase->last=stmdbase->last->next;
		stmdbase->last->syscall_num=syscall_num;
		stmdbase->last->next=NULL;
		}
	}
	else {
			struct strace_info* head=current->st_md_base->next;
		int k=0;
		while(head!=NULL){
       if(head->syscall_num==syscall_num){
		k=1;
		break;
	   }
		}
		if(k==0)return -EINVAL;
		if(head==NULL)return -EINVAL;
		struct strace_info* node=NULL;
		while(head!=NULL){
			if(head->syscall_num==syscall_num){
				if(node!=NULL)
				node->next=head->next;
			else
			stmdbase->next=head->next;
		  return 0;
			}
			else {
            node=head;
			head=head->next;
			}
		}
	}
	
	return 0;
}

int sys_read_strace(struct file *filep, char *buff, u64 count)
{
	if(filep==NULL)return -EINVAL;
	char m[8];
	int ans=0;
	char* data=filep->trace_buffer->data;
    for(int i=0;i<count;i++){
		//printk(" writeoff %x readoff %x ",filep->trace_buffer->writeoffset,filep->trace_buffer->readoffset);
		if(filep->trace_buffer->num!=0){
			u32 r=filep->trace_buffer->readoffset;
		   for(int j=0;j<8;j++){
			m[j]=data[(r+j)%4096];
		   }
		   u64 syscall_num;
		   bf(m,&syscall_num);
		   int p;
	 if(syscall_num==2||syscall_num==10||syscall_num==11||syscall_num==13||syscall_num==15||syscall_num==20||syscall_num==21||syscall_num==22||syscall_num==38||syscall_num==61)
	 p=0;
	else if(syscall_num==1||syscall_num==6||syscall_num==7||syscall_num==12||syscall_num==14||syscall_num==19||syscall_num==27||syscall_num==29||syscall_num==36)
	p=1;
    else if(syscall_num==4||syscall_num==8||syscall_num==9||syscall_num==17||syscall_num==23||syscall_num==28||syscall_num==37||syscall_num==40)
	p=2;
    else if(syscall_num==5||syscall_num==18||syscall_num==24||syscall_num==25||syscall_num==30||syscall_num==39||syscall_num==41)
	p=3;
    else if(syscall_num==16||syscall_num==35)
	p=4;
    if(p==0)
	trace_buffer_read(filep,buff,8);
    else if(p==1)
     trace_buffer_read(filep,buff,16);
    else if(p==2)
     trace_buffer_read(filep,buff,24);
    else if(p==3)
     trace_buffer_read(filep,buff,32);
	else if(p==4)
     trace_buffer_read(filep,buff,40);
    //  for(u32 j=r;j<=(r+p);j++){
	// 	*buff=data[j];
	// 	buff=buff+8;
	//  }
	// trace_buffer_read(filep,m,p+1);
	
     ans=ans+8*(p+1);
	// printk(" ans %x ",ans);
		}
		
	}
	// printk(" ansout %x ",ans);
	return ans;
}

int sys_start_strace(struct exec_context *current, int fd, int tracing_mode)
{
	//printk(" strace started ");
	if(tracing_mode!=FULL_TRACING&&tracing_mode!=FILTERED_TRACING)return -EINVAL;
    struct strace_head* stmdbase=(struct strace_head*)os_page_alloc(USER_REG);
	if(stmdbase==NULL)return -EINVAL;
	current->st_md_base=stmdbase;
	struct trace_buffer_info* tb=current->files[fd]->trace_buffer;
    stmdbase->count=0;
	stmdbase->is_traced=1;
	stmdbase->strace_fd=fd;
	stmdbase->tracing_mode=tracing_mode;
	// stmdbase->next=NULL;
	//  stmdbase->last=NULL;
	//printk(" strace ended ");
	return 0;
}

int sys_end_strace(struct exec_context *current)
{  
	//printk(" endstrace started ");  
	if(current->st_md_base==NULL)return -EINVAL;
	os_page_free(USER_REG,current->st_md_base);
	current->st_md_base=NULL;
	//printk(" endstrace ended ");
	return 0;
}



///////////////////////////////////////////////////////////////////////////
//// 		Start of ftrace functionality 		      	      /////
///////////////////////////////////////////////////////////////////////////


long do_ftrace(struct exec_context *ctx, unsigned long faddr, long action, long nargs, int fd_trace_buffer)
{
    return 0;
}

//Fault handler
long handle_ftrace_fault(struct user_regs *regs)
{
        return 0;
}


int sys_read_ftrace(struct file *filep, char *buff, u64 count)
{
    return 0;
}


