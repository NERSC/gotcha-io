#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include "gotcha/gotcha.h"
#include <mpi.h>
#include <hdf5.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "my_amqps_sendstring.h"
#include "main.h"

typedef hid_t (*H5Fcreate_fptr)(const char * name, unsigned flags, hid_t fcpl_id, hid_t fapl_id);
typedef hid_t (*H5Fopen_fptr)(const char *name, unsigned flags, hid_t fapl_id);
typedef hid_t (*H5Fclose_fptr)(hid_t file_id);
typedef herr_t (*H5Dread_fptr)(void *dset, hid_t mem_type_id, hid_t mem_space_id,
	                        hid_t file_space_id, hid_t plist_id, void *buf);
typedef herr_t (*H5Dwrite_fptr)(void *dset, hid_t mem_type_id, hid_t mem_space_id,
				hid_t file_space_id, hid_t plist_id, void *buf);

static gotcha_wrappee_handle_t H5Fcreate_handle;
static gotcha_wrappee_handle_t H5Fopen_handle;
static gotcha_wrappee_handle_t H5Fclose_handle;
static gotcha_wrappee_handle_t H5Dread_handle;
static gotcha_wrappee_handle_t H5Dwrite_handle;

struct log_info job_log; 
int read_count = 0;
int create_count = 0;
int close_count = 0;
int dread_count = 0;
int dwrite_count = 0;
double tot_time = 0; 


static void printtime()
{
  fprintf(stderr, "Total overhead gotcha time %f sec\n", tot_time);
  return ;
}


static hbool_t is_mpi(hid_t fapl_id)
{
  hbool_t have_mpi = false;
  hid_t driver_id;
  // If its the default fapl then we know its serial
  if (fapl_id == H5P_DEFAULT){
    have_mpi = false;
  }
  else if((driver_id = H5Pget_driver(fapl_id)) > 0 && driver_id == H5FD_MPIO){
    have_mpi = true;
  }
  return have_mpi;
}


static void wrt_log(hid_t result, const char *file_op, const int ismpi)
{
  static int count = 0;
  //compute first time this enters
  if (!count){
     
    job_log.ismpi = ismpi;
    job_log.uid = getuid(); 
    unsigned long long ts = (unsigned long)time(NULL);
    time_t rtime;
    time(&rtime);
    struct tm * info = localtime(&rtime);
    job_log.first_hdf5api_time = asctime(info); 
    job_log.host = getenv("NERSC_HOST");
    job_log.user = getenv("USER");
    job_log.hostname = getenv("hostname");
    //job_log.slurm_job_id = ()
    job_log.slurm_job_num_nodes = getenv("SLURM_JOB_NUM_NODES");
    job_log.slurm_job_account = getenv("SLURM_JOB_ACCOUNT");
    //This is part to get the nodetype
    job_log.nodetype="None";  
    if (strcmp(job_log.host, "cori")==0){
      FILE *fptr = fopen("/proc/cpuinfo", "r");
      if (fptr == NULL)
        fprintf(stderr, "failed to open /proc/cpuinfo\n");
      char* line=NULL;
      size_t len;
      ssize_t read;
      while ((read=getline(&line, &len, fptr))!=-1){
        char tempstr[10];
        strncpy(tempstr, line, 10);
        if (strcmp(tempstr, "model name")==0){
	  if (strstr(line, "Phi")!=NULL)
	    job_log.nodetype = "KNL";
	  else {
	    job_log.nodetype = "Haswell";
	  }
	  break; 
        }
      }
      fclose(fptr);
    }
  //atexit(printtime);
  }/*
    if (strcmp(job_log.hostname, "nid")==0)
      job_log.is_compute = 1;
    else
      job_log.is_compute = 0; */
    //fprintf(stderr, "H5Fopen(%s, %u, %0llx) = %0llx at ts=%s uid=%d\n",
    //        name, flags, fapl_id, result, asctime(info), uid);
    //send_to_mods(file_op, ismpi, asctime(info), uid, job_log, count);
    //atexit(send_to_mods);
  count ++;
  return ;
}


static void do_log(const char *name, unsigned flags, hid_t fapl_id, hid_t result,
 							     const char *file_op)
{
  // Check if its MPI
  if (is_mpi(fapl_id)){
    //fprintf(stderr, "Using MPI\n");
    MPI_Comm mpi_comm = MPI_COMM_NULL;
    MPI_Info mpi_info = MPI_COMM_NULL;
    H5Pget_fapl_mpio(fapl_id, &mpi_comm, &mpi_info);
    
    // Get world size and rank
    int world_size, world_rank;
    //MPI_Comm_size(mpi_comm, &world_size);
    MPI_Comm_rank(mpi_comm, &world_rank);
    // fprintf(stderr, "Total communicators %d\n", world_size);
    // fprintf(stderr, "My rank %d\n", world_rank); 
    // Write from only one rank per communicator
    if (world_rank == 0)
    	wrt_log(result, file_op, 1);
  }
  else{
    wrt_log(result, file_op, 0);
  }
  return ;
}


static hid_t H5Fcreate_wrapper(const char * name, unsigned flags, hid_t fcpl_id, 
								hid_t fapl_id) 
{
  clock_t t = clock();
  H5Fcreate_fptr H5Fcreate_wrappee = (H5Fcreate_fptr) gotcha_get_wrappee(H5Fcreate_handle);
  t = clock()-t;
  tot_time += ((double)t)/CLOCKS_PER_SEC;
  hid_t result = H5Fcreate_wrappee(name, flags, fcpl_id, fapl_id);
  t = clock();
  create_count++;
  do_log(name, flags, fapl_id, result, "create");
  t = clock()-t;
  tot_time += ((double)t)/CLOCKS_PER_SEC;
  return result;
}


static hid_t H5Fopen_wrapper(const char *name, unsigned flags, hid_t fapl_id) 
{
  clock_t t = clock();
  H5Fopen_fptr H5Fopen_wrappee = (H5Fopen_fptr) gotcha_get_wrappee(H5Fopen_handle);
  t = clock()-t;
  tot_time += ((double)t)/CLOCKS_PER_SEC;
  hid_t result = H5Fopen_wrappee(name, flags, fapl_id);
  t = clock();
  open_count++;
  do_log(name, flags, fapl_id, result, "open");
  t = clock()-t;
  tot_time += ((double)t)/CLOCKS_PER_SEC; 
  return result;
}


static hid_t H5Fclose_wrapper(hid_t file_id) 
{
  clock_t t = clock();
  H5Fclose_fptr H5Fclose_wrappee = (H5Fclose_fptr) gotcha_get_wrappee(H5Fclose_handle);
  t = clock()-t;
  tot_time +=((double)t)/CLOCKS_PER_SEC;
  hid_t result = H5Fclose_wrappee(file_id);
  //fprintf(stderr, "H5Fclose(%0llx) = %0llx\n",
  //        file_id,result);
  t = clock();
  close_count++;
  wrt_log(result, "close", 0);
  t = clock()-t;
  tot_time += ((double)t)/CLOCKS_PER_SEC;
  return result;
}


static herr_t H5Dread_wrapper(void *dset, hid_t mem_type_id, hid_t mem_space_id,
                      hid_t file_space_id, hid_t plist_id, void *buf)
{ 
  H5Dread_fptr H5Dread_wrappee = (H5Dread_fptr) gotcha_get_wrappee(H5Dread_handle);
  herr_t ret_val = H5Dread_wrappee(dset, mem_type_id, mem_space_id, file_space_id, 
							plist_id, buf );
  dread_count++;
  wrt_log(ret_val, "DRead", 0);
  return ret_val;
}


static herr_t H5Dwrite_wrapper(void *dset, hid_t mem_type_id, hid_t mem_space_id,
			hid_t file_space_id, hid_t plist_id, void *buf)
{
  H5Dwrite_fptr H5Dwrite_wrappee = (H5Dwrite_fptr) gotcha_get_wrappee(H5Dwrite_handle);
  herr_t ret_val = H5Dwrite_wrappee(dset, mem_type_id, mem_space_id, file_space_id,
							plist_id, buf);
  dwrite_count++;
  wrt_log(ret_val, "DWrite", 0);
  return ret_val;
}


static gotcha_binding_t h5_file_func_bindings[]={
   {"H5Fcreate", H5Fcreate_wrapper, &H5Fcreate_handle},
   {"H5Fopen", H5Fopen_wrapper, &H5Fopen_handle},
   {"H5Fclose", H5Fclose_wrapper, &H5Fclose_handle},
   {"H5Dread", H5Dread_wrapper, &H5Dread_handle},
   {"H5Dwrite", H5Dwrite_wrapper, &H5Dwrite_handle}
};


void init_gotcha_h5_file_func(){
 gotcha_wrap(h5_file_func_bindings, 5, "h5gotcha");
}


__attribute__((constructor)) void construct_me(){
    init_gotcha_h5_file_func();
}


