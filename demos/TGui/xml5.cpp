
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>


int frame3 ( void )
{
  static int count = 0;
  count++;

  sleep (1);
  int *a = (int*)malloc(10 * sizeof(int));

  // bad address;
  int n = a[10+count];

  if (count == 2)
        n = a[11];

  else if (count == 3)
        n = a[12];

  for (int i = 0; i < 3; i++)
  {
    // undefined condition  
    if (a[5] == 42) {
      printf("hello from frame3().  The answer is 42.\n");
    } else {
      printf("hello from frame3().  The answer is not 42.\n");
    }
  }

  sleep (1);

  // undefined address (careful ..)
  n = a[  a[0] & 7  ];


  // invalid free, the second time
  free(a);
  free(a);
  sleep (1);

  // more invalid frees
  free(&n);

  // leak (definite loss)
  int* a1 = (int*)malloc(101 * sizeof(int));
  a1 = NULL;
  // leak (possible)
  int* a2 = (int*)malloc(102 * sizeof(int));
  a2++;
  // leak (indirect)
  int** a3 = (int**)malloc(103 * sizeof(int*));
  a3[50] = (int*)malloc(10 * sizeof(int));
  free(a3);
  // leak (still reachable)
  static int* a4 = (int*)malloc(104 * sizeof(int));

  // mismatch
  int* foo = new int[99];
  delete foo;

  // pass garbage to the exit syscall
  return n;
}

int frame2 ( void )
{
  return frame3() - 1;
}

int frame1 ( void )
{
  return frame2() + 1;
}

int main ( void )
{
  int ret = 0;
  for (int i=0; i < 5; ++i)
	ret += frame1();
  return ret - 5;
}
