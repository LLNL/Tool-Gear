/* Demonstrate some of Valgrind's ability to find memory errors and leaks */
/* Compile with 'mpiicc -g mpi_vg_demo2.c -o mpi_vg_demo2' */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    int  numtasks, rank, rc, i, *null_ptr; 
    char *buf, ch1;
    MPI_Status status;
    MPI_Request request;
    int sum, sum2;
    char *send_buf, *recv_buf;
    int send_to, recv_from;
    
    rc = MPI_Init(&argc,&argv);
    if (rc != MPI_SUCCESS) {
        printf ("Error starting MPI program. Terminating.\n");
        MPI_Abort(MPI_COMM_WORLD, rc);
    }
    
    MPI_Comm_size(MPI_COMM_WORLD,&numtasks);
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);
    
    /* Send and recv data from someone else if possible */
    send_to = rank + 1;
    if (send_to >= numtasks)
        send_to -= numtasks;
    recv_from = rank - 1;
    if (recv_from < 0)
        recv_from += numtasks;
    
    printf ("Number of tasks= %d   My rank= %d   Send to %d   Recv from %d\n", 
            numtasks,rank, send_to, recv_from);
    
    /* Demonstrate some Memcheck features */
    buf = (char *) malloc (100);
    
    ch1 = buf[2];  /* Uninitialized memory read */
    
    buf[100] = 0;  /* Write past malloced size */
    
    ch1 = buf[100]; /* Read past malloced size */
    
    buf = (char*) malloc (200); /* Lose pointer to original buffer */
    
    free (buf);
    
    char *freed_buf = buf;
    
    /* Not freed but pointer exists, so Memcheck doesn't report as leak */
    buf = (char *) malloc (40); 
    
    /* Allocate buffers for MPI test */
    send_buf = (char *)malloc (20); 
    
    /* Initialize only the first 15 characters */
    for (i=0; i < 15; i++)
       send_buf[i] = i;
    
    recv_buf = (char *)malloc (30);
    
    free (freed_buf);   /* Free twice */
    
    /* Send more data than allocated, shows as BFCP inside MPI_ISend */
    /* Wrappers should also detect uninitialized use also */
    MPI_Isend (send_buf, 25, MPI_CHAR, send_to, 0, MPI_COMM_WORLD,
               &request);
    
    /* Receive data sent above into bigger buffer */
    MPI_Recv (recv_buf, 30, MPI_CHAR, recv_from, 0, MPI_COMM_WORLD, &status);
    
    /* Free up Send's request */
    MPI_Wait (&request, &status);
    
    /* Sum data that was actually initialized and sent */
    sum = 0;
    for (i=0; i < 15; i++)
        sum += recv_buf[i];  
    
    /* Valgrind should not complain here with MPI wrappers */
    printf ("Sum over initialized data for rank %i is %i\n", rank, sum);
    
    /* Sum more data than was actually received */
    sum2 = 0;
    for (i=0; i < 30; i++)
        sum2 += recv_buf[i];  /* Last 5 bytes uninitialized */
    
    /* Valgrind should complain here, even with MPI wrappers */
    printf ("Sum over uninitialized data for rank %i is %i\n", rank, sum2);
    
    free (&rank); /* Free of unallocated memory */
    
    MPI_Finalize();
}
