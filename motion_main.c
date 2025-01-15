#include <stdio.h>
#include "motion.h"

int main(int argc, char *argv[]) {
    if(is_motion(argv[1])) {
        printf("Motion!\n");
    } else 
    {
        printf("No motion...\n");
    }
}
