/* add.c
 * a simple C program
 */
  
#include <stdio.h>
#define LAST 10

#if 0
!@!#
#endif 

int main()
{
    // comment
    int i, sum = 0;
   
    for ( i = 1; i <= LAST; i++ ) {
      sum += i;
    } /*-for-*/
    printf("sum = %d\n", sum);

    return 0;
}


