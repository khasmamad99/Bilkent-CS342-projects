

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include "sbmem.h"

int main()
{
    int i, ret; 
    char *p;  	

    printf("STARTED\n"); 


    ret = sbmem_open(); 
    if (ret == -1)
	exit (1); 
    
    p = sbmem_alloc (256); // allocate space to hold 1024 characters
    printf("%p\n", p);
    for (i = 0; i < 256; ++i)
	p[i] = 'a'; // init all chars to ‘a’
    sbmem_free (p);

    sbmem_close();

    printf("ENDED\n"); 
    
    return (0); 
}
