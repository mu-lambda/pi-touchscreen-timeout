#include "motion.h"
#include <stdio.h>
#include <time.h>

int main(int argc, char *argv[]) {
        struct timespec sleepTime;
        sleepTime.tv_sec = 0;
        sleepTime.tv_nsec = 500000000L; /* 5 seconds */

        while (1) {
                if (is_motion(argv[1])) {
                        printf("Motion!\n");
                } else {
                        printf("No motion...\n");
                }

                nanosleep(&sleepTime, NULL);
        }
}
