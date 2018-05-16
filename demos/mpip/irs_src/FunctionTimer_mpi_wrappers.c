
#include <stdio.h>
#include <stdlib.h>
#include "irs.h"
#include "Hash.h"
#include "FunctionTimer.h"

/*
 ***********************************************************************
 *  file:    trace_prepend.c
 *  authors: Terry Jones (LLNL) & Robert Blackmore (IBM)
 *  notes:   Please see README.tracing
 ***********************************************************************
 */

#ifdef TERRY_TRACE

#include <values.h>

char *ctl_file = "/dev/systrctl";
int  ctlfd;
int  loop_count=0, mytraceid, iam_leadtask=0, leadtask;

static int find_leadtask(int);

#endif

/*
 ***********************************************************************
 * End of Terry Jones Section of code
 ***********************************************************************
 */

#include <unistd.h>


char * ft_mpi_routine_names[MPI_num_routines] = {
   "MPI_Abort",      "MPI_Allgather",   "MPI_Allgatherv",
   "MPI_Allreduce",
   "MPI_Barrier",    "MPI_Bcast",       "MPI_Comm_create", 
   "MPI_Comm_group", "MPI_Comm_rank",   "MPI_Comm_size",  
   "MPI_Finalize",   "MPI_Gather",      "MPI_Gatherv", 
   "MPI_Group_incl", "MPI_Init",
   "MPI_Irecv",      "MPI_Isend",       "MPI_Recv",       
   "MPI_Reduce",     "MPI_Send",        "MPI_Waitall",     
   "MPI_Waitany",    "MPI_Wait",        "MPI_Wtick",      
   "MPI_Wtime",      "Total MPI Calls",
};
int ft_mpi_cntrs[MPI_num_routines] = {
   0, 0, 0, 
   0,
   0, 0, 0,
   0, 0, 0,
   0, 0, 0, 
   0, 0,
   0, 0, 0,
   0, 0, 0,
   0, 0, 0,
   0, 0 
};
#ifndef COMMPI
MPI_Request  MPI_REQUEST_NULL = 0;
MPI_Datatype MPI_DOUBLE       = 11;
MPI_Datatype MPI_CHAR         = 1;
MPI_Datatype MPI_INT          = 6;
MPI_Comm     MPI_COMM_WORLD   = 91;
MPI_Op       MPI_SUM          = 102 ;
MPI_Op       MPI_MIN          = 101 ;
MPI_Op       MPI_MAX          = 100 ;
#endif
static int reset_cycle = 0;
void mpi_reset_cntrs(int cycle)
{
  int i;
  for (i=0; i<MPI_num_routines; i++) { 
    ft_mpi_cntrs[i] = 0; 
  }
  reset_cycle = cycle;
}
int     MPI_Abort_Wrapper(MPI_Comm comm, int errorcode)
{
#ifdef COMMPI
  char *me = ft_mpi_routine_names[MPI_Abort_cntr];
  int ierr;
  FT_INITIALIZE(me, ft_global_ht)
  ft_mpi_cntrs[MPI_Total_cntr]++;
  ft_mpi_cntrs[MPI_Abort_cntr]++;
  ierr = MPI_Abort(comm, errorcode);
  FT_FINALIZE(me, ft_global_ht, 1)
  return(ierr);
#else
  return(0);
#endif
}
int 	MPI_Allgather_Wrapper(void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
{
#ifdef COMMPI
  char *me = ft_mpi_routine_names[MPI_Allgather_cntr];
  int ierr;
  FT_INITIALIZE(me, ft_global_ht)
  ft_mpi_cntrs[MPI_Total_cntr]++;
  ft_mpi_cntrs[MPI_Allgather_cntr]++;
  ierr = MPI_Allgather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
  FT_FINALIZE(me, ft_global_ht, 1)
  return(ierr);
#else
  return(0);
#endif
}
int 	MPI_Allgatherv_Wrapper(void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int *recvcount, int *recvdisp, MPI_Datatype recvtype, MPI_Comm comm)
{
#ifdef COMMPI
  char *me = ft_mpi_routine_names[MPI_Allgatherv_cntr];
  int ierr;
  FT_INITIALIZE(me, ft_global_ht)
  ft_mpi_cntrs[MPI_Total_cntr]++;
  ft_mpi_cntrs[MPI_Allgatherv_cntr]++;
  ierr = MPI_Allgatherv(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvdisp, recvtype, comm);
  FT_FINALIZE(me, ft_global_ht, 1)
  return(ierr);
#else
  return(0);
#endif
}

int 	MPI_Allreduce_Wrapper(void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
#ifdef COMMPI

  char *me = ft_mpi_routine_names[MPI_Allreduce_cntr];

  int ierr;

  FT_INITIALIZE(me, ft_global_ht)

  ft_mpi_cntrs[MPI_Total_cntr]++;
  ft_mpi_cntrs[MPI_Allreduce_cntr]++;

#ifdef TERRY_TRACE
  TERRY_MPI_Allreduce_cntr++;
  if (datatype == MPI_INT) { TRCHKGT(BEFORE_MPI_Allreduce, cycle, TERRY_MPI_Allreduce_cntr, (int)op, count, count*sizeof(int)); }
  else                     { TRCHKGT(BEFORE_MPI_Allreduce, cycle, TERRY_MPI_Allreduce_cntr, (int)op, count, count*sizeof(double)); }
#endif

  ierr = MPI_Allreduce(sendbuf, recvbuf, count, datatype, op, comm);

#ifdef TERRY_TRACE
  if (datatype == MPI_INT) { TRCHKGT(AFTER_MPI_Allreduce, cycle, TERRY_MPI_Allreduce_cntr, (int)op, count, count*sizeof(int)); }
  else                     { TRCHKGT(AFTER_MPI_Allreduce, cycle, TERRY_MPI_Allreduce_cntr, (int)op, count, count*sizeof(double)); }
#endif

  FT_FINALIZE(me, ft_global_ht, 1)

  return(ierr);

#else
  return(0);
#endif
}


int 	MPI_Barrier_Wrapper(MPI_Comm comm)
{
#ifdef COMMPI
  char *me = ft_mpi_routine_names[MPI_Barrier_cntr];
  int ierr;
  FT_INITIALIZE(me, ft_global_ht)
  ft_mpi_cntrs[MPI_Total_cntr]++;
  ft_mpi_cntrs[MPI_Barrier_cntr]++;

#ifdef TERRY_TRACE
  TERRY_MPI_Barrier_cntr++;
  TRCHKGT(BEFORE_MPI_Barrier, cycle, TERRY_MPI_Barrier_cntr, 0, 0, 0);
#endif

  ierr = MPI_Barrier(comm);

#ifdef TERRY_TRACE
  TRCHKGT(AFTER_MPI_Barrier, cycle, TERRY_MPI_Barrier_cntr, 0, 0, 0); 
#endif

  FT_FINALIZE(me, ft_global_ht, 1)
  return(ierr);
#else
  return(0);
#endif
}
int 	MPI_Bcast_Wrapper(void* buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm )
{
#ifdef COMMPI
  char *me = ft_mpi_routine_names[MPI_Bcast_cntr];
  int ierr;
  FT_INITIALIZE(me, ft_global_ht)
  ft_mpi_cntrs[MPI_Total_cntr]++;
  ft_mpi_cntrs[MPI_Bcast_cntr]++;
#ifdef TERRY_TRACE
  TERRY_MPI_Bcast_cntr++;
  if      (datatype == MPI_INT)    { TRCHKGT(BEFORE_MPI_Bcast, cycle, TERRY_MPI_Bcast_cntr, root, count, count*sizeof(int));    }
  else if (datatype == MPI_DOUBLE) { TRCHKGT(BEFORE_MPI_Bcast, cycle, TERRY_MPI_Bcast_cntr, root, count, count*sizeof(double)); }
  else                             { TRCHKGT(BEFORE_MPI_Bcast, cycle, TERRY_MPI_Bcast_cntr, root, count, count*sizeof(char));   }
#endif
  ierr = MPI_Bcast(buffer, count, datatype, root, comm);
#ifdef TERRY_TRACE
  if      (datatype == MPI_INT)    { TRCHKGT(AFTER_MPI_Bcast, cycle, TERRY_MPI_Bcast_cntr, root, count,  count*sizeof(int));     }
  else if (datatype == MPI_DOUBLE) { TRCHKGT(AFTER_MPI_Bcast, cycle, TERRY_MPI_Bcast_cntr, root, count,  count*sizeof(double));  }
  else                             { TRCHKGT(AFTER_MPI_Bcast, cycle, TERRY_MPI_Bcast_cntr, root, count,  count*sizeof(char));    }
#endif
  FT_FINALIZE(me, ft_global_ht, 1)
  return(ierr);
#else
  return(0);
#endif
}
int 	MPI_Comm_create_Wrapper(MPI_Comm comm, MPI_Group group, MPI_Comm *newcomm)
{
#ifdef COMMPI
  char *me = ft_mpi_routine_names[MPI_Comm_create_cntr];
  int ierr;
  FT_INITIALIZE(me, ft_global_ht)
  ft_mpi_cntrs[MPI_Total_cntr]++;
  ft_mpi_cntrs[MPI_Comm_create_cntr]++;
  ierr = MPI_Comm_create(comm, group, newcomm);
  FT_FINALIZE(me, ft_global_ht, 1)
  return(ierr);
#else
  return(0);
#endif
}
int 	MPI_Comm_group_Wrapper(MPI_Comm comm, MPI_Group *group)
{
#ifdef COMMPI
  char *me = ft_mpi_routine_names[MPI_Comm_group_cntr];
  int ierr;
  FT_INITIALIZE(me, ft_global_ht)
  ft_mpi_cntrs[MPI_Total_cntr]++;
  ft_mpi_cntrs[MPI_Comm_group_cntr]++;
  ierr = MPI_Comm_group(comm, group);
  FT_FINALIZE(me, ft_global_ht, 1)
  return(ierr);
#else
  return(0);
#endif
}
int 	MPI_Comm_rank_Wrapper(MPI_Comm comm, int *rank)
{
#ifdef COMMPI
  char *me = ft_mpi_routine_names[MPI_Comm_rank_cntr];
  int ierr;
  FT_INITIALIZE(me, ft_global_ht)
  ft_mpi_cntrs[MPI_Total_cntr]++;
  ft_mpi_cntrs[MPI_Comm_rank_cntr]++;
  ierr = MPI_Comm_rank(comm, rank);
  FT_FINALIZE(me, ft_global_ht, 1)
  return(ierr);
#else
  return(0);
#endif
}
int 	MPI_Comm_size_Wrapper(MPI_Comm comm, int *size)
{
#ifdef COMMPI
  char *me = ft_mpi_routine_names[MPI_Comm_size_cntr];
  int ierr;
  FT_INITIALIZE(me, ft_global_ht)
  ft_mpi_cntrs[MPI_Total_cntr]++;
  ft_mpi_cntrs[MPI_Comm_size_cntr]++;
  ierr = MPI_Comm_size(comm, size);
  FT_FINALIZE(me, ft_global_ht, 1)
  return(ierr);
#else
  return(0);
#endif
}
int 	MPI_Finalize_Wrapper(void)
{
#ifdef COMMPI
  char *me = ft_mpi_routine_names[MPI_Finalize_cntr];
  int ierr;
  FT_INITIALIZE(me, ft_global_ht)
  ft_mpi_cntrs[MPI_Total_cntr]++;
  ft_mpi_cntrs[MPI_Finalize_cntr]++;
  ierr = MPI_Finalize();
  FT_FINALIZE(me, ft_global_ht, 1)
  return(ierr);
#else
  return(0);
#endif
}
int 	MPI_Gather_Wrapper(void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
#ifdef COMMPI
  char *me = ft_mpi_routine_names[MPI_Gather_cntr];
  int ierr;
  FT_INITIALIZE(me, ft_global_ht)
  ft_mpi_cntrs[MPI_Total_cntr]++;
  ft_mpi_cntrs[MPI_Gather_cntr]++;                                                                
  ierr = MPI_Gather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm);
  FT_FINALIZE(me, ft_global_ht, 1)
  return(ierr);
#else
  return(0);
#endif
}
int 	MPI_Gatherv_Wrapper(void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int *recvcount, int *recvdisp, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
#ifdef COMMPI
  char *me = ft_mpi_routine_names[MPI_Gatherv_cntr];
  int ierr;
  FT_INITIALIZE(me, ft_global_ht)
  ft_mpi_cntrs[MPI_Total_cntr]++;
  ft_mpi_cntrs[MPI_Gatherv_cntr]++;                                                                
  ierr = MPI_Gatherv(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvdisp, recvtype, root, comm);
  FT_FINALIZE(me, ft_global_ht, 1)
  return(ierr);
#else
  return(0);
#endif
}
int 	MPI_Group_incl_Wrapper(MPI_Group group, int n, int *ranks, MPI_Group *newgroup)
{
#ifdef COMMPI
  char *me = ft_mpi_routine_names[MPI_Group_incl_cntr];
  int ierr;
  FT_INITIALIZE(me, ft_global_ht)
  ft_mpi_cntrs[MPI_Total_cntr]++;
  ft_mpi_cntrs[MPI_Group_incl_cntr]++;
  ierr = MPI_Group_incl(group, n, ranks, newgroup);
  FT_FINALIZE(me, ft_global_ht, 1)
  return(ierr);
#else
  return(0);
#endif
}
int	MPI_Init_Wrapper(int *argc, char ***argv) 
{
#ifdef COMMPI
  char *me = ft_mpi_routine_names[MPI_Init_cntr];
  int ierr;
  FT_INITIALIZE(me, ft_global_ht)
  ft_mpi_cntrs[MPI_Total_cntr]++;
  ft_mpi_cntrs[MPI_Init_cntr]++;
  ierr = MPI_Init(argc,argv);
  FT_FINALIZE(me, ft_global_ht, 1)
  return(ierr);
#else
  return(NO_MPI);
#endif
}
int 	MPI_Irecv_Wrapper(void* buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request *request)
{
#ifdef COMMPI
  char *me = ft_mpi_routine_names[MPI_Irecv_cntr];
  int ierr;
  FT_INITIALIZE(me, ft_global_ht)
  ft_mpi_cntrs[MPI_Total_cntr]++;
  ft_mpi_cntrs[MPI_Irecv_cntr]++;
#ifdef TERRY_TRACE
  TERRY_MPI_Irecv_cntr++;
  if      (datatype == MPI_INT)    { TRCHKGT(BEFORE_MPI_Irecv, cycle, TERRY_MPI_Irecv_cntr, source, count, count*sizeof(int)); 	}
  else if (datatype == MPI_DOUBLE) { TRCHKGT(BEFORE_MPI_Irecv, cycle, TERRY_MPI_Irecv_cntr, source, count, count*sizeof(double)); 	}
  else                             { TRCHKGT(BEFORE_MPI_Irecv, cycle, TERRY_MPI_Irecv_cntr, source, count, count*sizeof(char)); 	}
#endif
  ierr = MPI_Irecv(buf, count, datatype, source, tag, comm, request);
#ifdef TERRY_TRACE
  if      (datatype == MPI_INT)    { TRCHKGT(AFTER_MPI_Irecv, cycle, TERRY_MPI_Irecv_cntr, source, count, count*sizeof(int));	} 
  else if (datatype == MPI_DOUBLE) { TRCHKGT(AFTER_MPI_Irecv, cycle, TERRY_MPI_Irecv_cntr, source, count, count*sizeof(double)); 	}
  else                             { TRCHKGT(AFTER_MPI_Irecv, cycle, TERRY_MPI_Irecv_cntr, source, count, count*sizeof(char)); 	}
#endif
  FT_FINALIZE(me, ft_global_ht, 1)
  return(ierr);
#else
  return(0);
#endif
}
int 	MPI_Isend_Wrapper(void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
#ifdef COMMPI
  char *me = ft_mpi_routine_names[MPI_Isend_cntr];
  int ierr;
  FT_INITIALIZE(me, ft_global_ht)
  ft_mpi_cntrs[MPI_Total_cntr]++;
  ft_mpi_cntrs[MPI_Isend_cntr]++;
#ifdef TERRY_TRACE
  TERRY_MPI_Isend_cntr++;
  if      (datatype == MPI_INT)    { TRCHKGT(BEFORE_MPI_Isend, cycle, TERRY_MPI_Isend_cntr, dest, count, count*sizeof(int));	} 
  else if (datatype == MPI_DOUBLE) { TRCHKGT(BEFORE_MPI_Isend, cycle, TERRY_MPI_Isend_cntr, dest, count, count*sizeof(double)); 	}
  else                             { TRCHKGT(BEFORE_MPI_Isend, cycle, TERRY_MPI_Isend_cntr, dest, count, count*sizeof(char)); 	}
#endif
  ierr = MPI_Isend(buf, count, datatype, dest, tag, comm, request);
#ifdef TERRY_TRACE
  if      (datatype == MPI_INT)    { TRCHKGT(AFTER_MPI_Isend, cycle, TERRY_MPI_Isend_cntr, dest, count, count*sizeof(int)); 	}
  else if (datatype == MPI_DOUBLE) { TRCHKGT(AFTER_MPI_Isend, cycle, TERRY_MPI_Isend_cntr, dest, count, count*sizeof(double)); 	}
  else                             { TRCHKGT(AFTER_MPI_Isend, cycle, TERRY_MPI_Isend_cntr, dest, count, count*sizeof(char)); 	}
#endif
  FT_FINALIZE(me, ft_global_ht, 1)
  return(ierr);
#else
  return(0);
#endif
}
int 	MPI_Recv_Wrapper(void* buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status *status)
{
#ifdef COMMPI
  char *me = ft_mpi_routine_names[MPI_Recv_cntr];
  int ierr;
  FT_INITIALIZE(me, ft_global_ht)
  ft_mpi_cntrs[MPI_Total_cntr]++;
  ft_mpi_cntrs[MPI_Recv_cntr]++;
#ifdef TERRY_TRACE
  TERRY_MPI_Recv_cntr++;
  if      (datatype == MPI_INT)    { TRCHKGT(BEFORE_MPI_Recv, cycle, TERRY_MPI_Recv_cntr, source, count, count*sizeof(int));	}
  else if (datatype == MPI_DOUBLE) { TRCHKGT(BEFORE_MPI_Recv, cycle, TERRY_MPI_Recv_cntr, source, count, count*sizeof(double));	}
  else                             { TRCHKGT(BEFORE_MPI_Recv, cycle, TERRY_MPI_Recv_cntr, source, count, count*sizeof(char));	}
#endif
  ierr = MPI_Recv(buf, count, datatype, source, tag, comm, status);
#ifdef TERRY_TRACE
  if      (datatype == MPI_INT)    { TRCHKGT(AFTER_MPI_Recv, cycle, TERRY_MPI_Recv_cntr, source, count, count*sizeof(int)); 	}
  else if (datatype == MPI_DOUBLE) { TRCHKGT(AFTER_MPI_Recv, cycle, TERRY_MPI_Recv_cntr, source, count, count*sizeof(double)); 	}
  else                             { TRCHKGT(AFTER_MPI_Recv, cycle, TERRY_MPI_Recv_cntr, source, count, count*sizeof(char));	}
#endif
  FT_FINALIZE(me, ft_global_ht, 1)
  return(ierr);
#else
  return(0);
#endif
}
int 	MPI_Reduce_Wrapper(void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm)
{
#ifdef COMMPI
  char *me = ft_mpi_routine_names[MPI_Reduce_cntr];
  int ierr;
  FT_INITIALIZE(me, ft_global_ht)
  ft_mpi_cntrs[MPI_Total_cntr]++;
  ft_mpi_cntrs[MPI_Reduce_cntr]++;
  ierr = MPI_Reduce(sendbuf, recvbuf, count, datatype, op, root, comm);
  FT_FINALIZE(me, ft_global_ht, 1)
  return(ierr);
#else
  return(0);
#endif
}
int 	MPI_Send_Wrapper(void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)
{
#ifdef COMMPI
  char *me = ft_mpi_routine_names[MPI_Send_cntr];
  int ierr;
  FT_INITIALIZE(me, ft_global_ht)
  ft_mpi_cntrs[MPI_Total_cntr]++;
  ft_mpi_cntrs[MPI_Send_cntr]++;
#ifdef TERRY_TRACE
  TERRY_MPI_Send_cntr++;
  if      (datatype == MPI_INT)    { TRCHKGT(BEFORE_MPI_Send, cycle, TERRY_MPI_Send_cntr, dest, count, count*sizeof(int)); 	}
  else if (datatype == MPI_DOUBLE) { TRCHKGT(BEFORE_MPI_Send, cycle, TERRY_MPI_Send_cntr, dest, count, count*sizeof(double)); 	}
  else                             { TRCHKGT(BEFORE_MPI_Send, cycle, TERRY_MPI_Send_cntr, dest, count, count*sizeof(char)); 	}
#endif
  ierr = MPI_Send(buf, count, datatype, dest, tag, comm);
#ifdef TERRY_TRACE
  if      (datatype == MPI_INT)    { TRCHKGT(AFTER_MPI_Send, cycle, TERRY_MPI_Send_cntr, dest, count, count*sizeof(int)); 	}
  else if (datatype == MPI_DOUBLE) { TRCHKGT(AFTER_MPI_Send, cycle, TERRY_MPI_Send_cntr, dest, count, count*sizeof(double)); 	}
  else                             { TRCHKGT(AFTER_MPI_Send, cycle, TERRY_MPI_Send_cntr, dest, count, count*sizeof(char)); 	}
#endif
  FT_FINALIZE(me, ft_global_ht, 1)
  return(ierr);
#else
  return(0);
#endif
}
int 	MPI_Waitall_Wrapper(int count, MPI_Request *array_of_requests, MPI_Status *array_of_status)
{
#ifdef COMMPI
  char *me = ft_mpi_routine_names[MPI_Waitall_cntr];
  int ierr;
  FT_INITIALIZE(me, ft_global_ht)
  ft_mpi_cntrs[MPI_Total_cntr]++;
  ft_mpi_cntrs[MPI_Waitall_cntr]++;
#ifdef TERRY_TRACE
  TERRY_MPI_Waitall_cntr++;
  TRCHKGT(BEFORE_MPI_Waitall, cycle, TERRY_MPI_Waitall_cntr, 0, 0, 0); 
#endif
  ierr = MPI_Waitall(count, array_of_requests, array_of_status);
#ifdef TERRY_TRACE
  TRCHKGT(AFTER_MPI_Waitall, cycle, TERRY_MPI_Waitall_cntr, 0, 0, 0); 
#endif
  FT_FINALIZE(me, ft_global_ht, 1)
  return(ierr);
#else
  return(0);
#endif
}
int 	MPI_Waitany_Wrapper(int count, MPI_Request *array_of_requests, int *index, MPI_Status *status)
{
#ifdef COMMPI
  char *me = ft_mpi_routine_names[MPI_Waitany_cntr];
  int ierr;
  FT_INITIALIZE(me, ft_global_ht)
  ft_mpi_cntrs[MPI_Total_cntr]++;
  ft_mpi_cntrs[MPI_Waitany_cntr]++;
#ifdef TERRY_TRACE
  TERRY_MPI_Waitany_cntr++;
  TRCHKGT(BEFORE_MPI_Waitany, cycle, TERRY_MPI_Waitany_cntr, 0, 0, 0); 
#endif
  ierr = MPI_Waitany(count, array_of_requests, index, status);
#ifdef TERRY_TRACE
  TRCHKGT(AFTER_MPI_Waitany, cycle, TERRY_MPI_Waitany_cntr, 0, 0, 0); 
#endif
  FT_FINALIZE(me, ft_global_ht, 1)
  return(ierr);
#else
  return(0);
#endif
}
int 	MPI_Wait_Wrapper(MPI_Request *request, MPI_Status *status)
{
#ifdef COMMPI
  char *me = ft_mpi_routine_names[MPI_Wait_cntr];
  int ierr;
  FT_INITIALIZE(me, ft_global_ht)
  ft_mpi_cntrs[MPI_Total_cntr]++;
  ft_mpi_cntrs[MPI_Wait_cntr]++;
#ifdef TERRY_TRACE
  TERRY_MPI_Wait_cntr++;
  TRCHKGT(BEFORE_MPI_Wait, cycle, TERRY_MPI_Wait_cntr, 0, 0, 0); 
#endif
  ierr = MPI_Wait(request, status);
#ifdef TERRY_TRACE
  TRCHKGT(AFTER_MPI_Wait, cycle, TERRY_MPI_Wait_cntr, 0, 0, 0); 
#endif
  FT_FINALIZE(me, ft_global_ht, 1)
  return(ierr);
#else
  return(0);
#endif
}
double 	MPI_Wtick_Wrapper(void)
{
#ifdef COMMPI
  char *me = ft_mpi_routine_names[MPI_Wtick_cntr];
  double derr;
  ft_mpi_cntrs[MPI_Total_cntr]++;
  ft_mpi_cntrs[MPI_Wtick_cntr]++;
  derr = MPI_Wtick();
  return(derr);
#else
  return(0);
#endif
}
double 	MPI_Wtime_Wrapper(void)
{
#ifdef COMMPI
  char *me = ft_mpi_routine_names[MPI_Wtime_cntr];
  double derr;
  ft_mpi_cntrs[MPI_Total_cntr]++;
  ft_mpi_cntrs[MPI_Wtime_cntr]++;
  derr = MPI_Wtime();
  return(derr);
#else
  return(0);
#endif
}

#ifdef TERRY_TRACE

void trace_finalize(void)
{

   TRCHKGT(EXITING_IRS_MAIN,cycle,IRS_MAIN_cntr,0,0,0);

   STOP_TRACING(iam_leadtask);

   HALT_TRACING(iam_leadtask);
}

void trace_init(void)
{
  /*
   ********************************************************************
   *  The following initializes some variables we'll need later
   *	  each layer will need to do these lines as one-time housekeeping
   ********************************************************************
   */

    MPI_Comm_rank(MPI_COMM_WORLD, &mytraceid);
    leadtask = find_leadtask(mytraceid);
    if(leadtask == myid ) { iam_leadtask=1; }

  /*
   ********************************************************************
   *  The following is some more one-time housekeeping for tracing
   ********************************************************************
   */

    ZAP_OLD_TRACE(iam_leadtask);

    INIT_TRACING(iam_leadtask);

    START_TRACING(iam_leadtask);

    MPI_Barrier(MPI_COMM_WORLD);  

    IRS_MAIN_cntr++;

    TRCHKGT(ENTERING_IRS_MAIN,cycle,IRS_MAIN_cntr,0,0,0);
}

static int find_leadtask(int mytask)
{
    char *common_tasks;
    char *child,*tmp,*save_str=NULL;
    int lead_task,common,i,count;
  
    tmp=getenv("MP_COMMON_TASKS");
    if(tmp!=NULL)  {
       lead_task= MAXINT;
       common_tasks = malloc(strlen(tmp) +1 );
       if(common_tasks == NULL){
          printf("Cannot malloc\n");
          exit(1);
       }
       strcpy(common_tasks,tmp);
       tmp= strtok_r(common_tasks,":",&save_str);
       if(tmp)
          count = atoi(tmp);
       else
          count = 0;
       for(i=0;i<count;i++) {
          tmp = strtok_r(NULL,":",&save_str);
          if(tmp) {
             common = atoi(tmp);
             if(common < lead_task)
                lead_task = common;
          }
       }
    } else {
       lead_task = mytask;
    }
    if(mytask < lead_task)
          lead_task = mytask;
    return lead_task;
}
#endif

#ifdef SAMPLE_CODE_DONT_COMPILE
  /*
   ********************************************************************
   *  The following lines demonstrate tracing an EVENT
   *	  Bracket each MPI_Allreduce and MPI_Barrier callsite which
   *      is heavily used.
   * 
   *  usage example:
   *
   *       TRCHKGT(BEFORE_ALLREDUCE_1,i,j,k,l,m);
   *
   *  where:
   *      BEFORE_ALLREDUCE_1 is one of the constants in trace_prepend.c
   *	  i is any interesting int in your logic (e.g. innermost loop index)
   *	  j is any interesting int in your logic (e.g. middle loop index)
   *	  k is any interesting int in your logic (e.g. outermost loop index)
   *	  l is any interesting int in your logic (e.g. count)
   *	  m is any interesting int in your logic (e.g. bytes being allreduced)
   ********************************************************************
   */
    /* when we're entering an intesting code segment, start the tracing */
    START_TRACING(iam_leadtask);

    /* the following lines will typically be inside a loop */

    	TRCHKGT(BEFORE_ALLREDUCE_1,i,j,k, count, count*sizeof(n(1)));

    	/* do some MPI_Allreduce */

    	TRCHKGT(AFTER_ALLREDUCE_1,i,j,k, count, count*sizeof(n(1)));

    /* when we've finished the intesting code segment, stop the tracing */
    STOP_TRACING(iam_leadtask);
  /*
   ********************************************************************
   *  Include the HALT_TRACING macro before MPI_Finalize
   ********************************************************************
   */
    HALT_TRACING(iam_leadtask);
    MPI_Finalize();
  /*
   ********************************************************************
   *  Include the following line when entering "interesting" subroutines
   ********************************************************************
   */
  TRCHKGT(ENTERING_IRS_ROUTINE, irs_routine_id, 0, 0, 0, 0);

  /*
   ********************************************************************
   *  Include the following line when exiting "interesting" subroutines
   ********************************************************************
   */
  TRCHKGT(EXITING_IRS_ROUTINE, irs_routine_id, 0, 0, 0, 0);

#endif

