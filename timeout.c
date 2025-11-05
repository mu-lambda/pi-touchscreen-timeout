/* timeout.c - a little program to blank the RPi touchscreen and unblank it
   on touch.  Original by https://github.com/timothyhollabaugh

   2018-04-16 - Joe Hartley, https://github.com/JoeHartley3
     Added command line parameters for input device and timeout in seconds
     Added nanosleep() to the loop to bring CPU usage from 100% on a single core
   to around 1%

   Note that when not running X Windows, the console framebuffer may blank and
   not return on touch. Use one of the following fixes:

   * Raspbian Jessie
     Add the following line to /etc/rc.local (on the line before the final exit
   0) and reboot: sh -c "TERM=linux setterm -blank 0 >/dev/tty0" Even though
   /dev/tty0 is used, this should propagate across all terminals.

   * Raspbian Wheezy
     Edit /etc/kbd/config and change the values for the variable shown below,
   then reboot: BLANK_TIME=0

   2018-04-23 - Moved nanosleep() outside of last if statement, fixed help
   screen to be consistent with binary name

   2018-08-22 - CJ Vaughter, https://github.com/cjvaughter
     Added support for multiple input devices
     Added external backlight change detection

   2021-12-04 - Tim H
     Include new headers (ctype.h, unistd.h) for isdigit and read, write, lseek
     Take backlight name from arguments

   2025-01-13 - MuLambda
     Use better values for bl_power.

*/

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <linux/input.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "motion.h"

const char UNBLANK = '0' + FB_BLANK_UNBLANK;
const char POWERDOWN = '0' + FB_BLANK_POWERDOWN;
static void usage() {
        printf("Usage: timeout <timeout_sec> <backlight> [--motion <path>] "
               "[<device> <device>...]\n");
        printf("    Backlights are in /sys/class/backlight/....\n");
        printf("    Use lsinput to see input devices.\n");
        printf("    Device to use is shown as /dev/input/....\n");
}

int main(int argc, char *argv[]) {
        if (argc < 4) {
                usage();
                exit(1);
        }
        // Parse arguments.
        int current_arg = 1;
        int timeout;
        {
                int tlen = strlen(argv[current_arg]);
                for (int i = 0; i < tlen; i++) {
                        if (!isdigit(argv[current_arg][i])) {
                                printf(
                                    "Entered timeout value is not a number\n");
                                exit(1);
                        }
                }
                timeout = atoi(argv[current_arg]);
        }
        current_arg++;

        char backlight_path[64] = "";
        {
                strcat(backlight_path, argv[current_arg]);
                strcat(backlight_path, "/bl_power");
        }
        current_arg++;

        char *motion_sensor = 0;
        if (strcmp("--motion", argv[current_arg]) == 0) {
                current_arg++;
                if (current_arg >= argc) {
                        usage();
                        exit(1);
                }
                motion_sensor = argv[current_arg];
                current_arg++;
        }

        int num_dev = argc - current_arg;
        int eventfd[num_dev];
        char *device[num_dev];
        {
                for (int i = 0; i < num_dev; i++) {
                        device[i] = argv[current_arg++];

                        int event_dev = open(device[i], O_RDONLY | O_NONBLOCK);
                        if (event_dev == -1) {
                                int err = errno;
                                printf("Error opening %s: %d\n", device[i],
                                       err);
                                fflush(stdout);
                                exit(1);
                        }
                        eventfd[i] = event_dev;
                }
        }
        // End argument parsing.

        if (motion_sensor != 0) {
                printf("Using motion sensor: %s\n", motion_sensor);
                fflush(stdout);
        }
        if (num_dev > 0) {
                printf("Using input device%s: ", (num_dev > 1) ? "s" : "");
                for (int i = 0; i < num_dev; i++) {
                        printf("%s ", device[i]);
                }
                printf("\n");
                fflush(stdout);
        }

        printf("Starting...\n");
        fflush(stdout);
        struct input_event event[64];
        int lightfd;
        int event_size;
        int light_size;
        int size = sizeof(struct input_event);

        int motion_state = -1;
        char read_on;
        char on;

        /* new sleep code to bring CPU usage down from 100% on a core */
        struct timespec sleepTime;
        sleepTime.tv_sec = 0;
        sleepTime.tv_nsec = 50000000L; /* 0.5 seconds - larger values may reduce
                                          load even more */

        lightfd = open(backlight_path, O_RDWR | O_NONBLOCK);

        if (lightfd == -1) {
                int err = errno;
                printf("Error opening backlight file: %d", err);
                exit(1);
        }

        light_size = read(lightfd, &read_on, sizeof(char));

        if (light_size < sizeof(char)) {
                int err = errno;
                printf("Error reading backlight file: %d", err);
                exit(1);
        }

        time_t now = time(NULL);
        time_t touch = now;
        on = read_on;

        while (1) {
                now = time(NULL);

                lseek(lightfd, 0, SEEK_SET);
                light_size = read(lightfd, &read_on, sizeof(char));
                if (light_size == sizeof(char) && read_on != on) {
                        if (read_on == UNBLANK) {
                                printf("Power enabled externally - Timeout "
                                       "reset\n");
                                fflush(stdout);
                                on = UNBLANK;
                                touch = now;
                        } else {
                                printf("Power disabled externally\n");
                                fflush(stdout);
                                on = read_on;
                        }
                }

                int event_detected = 0;

                // Motion detection.
                if (motion_sensor != 0) {
                        int current_motion_state = is_motion(motion_sensor);
                        if (current_motion_state) {
                                event_detected = 1;
                        }
                        if (motion_state != current_motion_state) {
                                motion_state = current_motion_state;
                                printf("Motion state changed to %s\n",
                                       (motion_state ? "on" : "off"));
                                fflush(stdout);
                        }
                }

                // Touch detection.
                for (int i = 0; i < num_dev; i++) {
                        event_size = read(eventfd[i], event, size * 64);
                        if (event_size != -1) {
                                printf("%s Value: %d, Code: %x\n", device[i],
                                       event[0].value, event[0].code);
                                fflush(stdout);
                                event_detected = 1;
                        }
                }
                if (event_detected) {
                        touch = now;

                        if (on != UNBLANK) {
                                printf("Turning On\n");
                                fflush(stdout);
                                on = UNBLANK;
                                write(lightfd, &on, sizeof(char));
                        }
                }

                if (difftime(now, touch) > timeout) {
                        if (on == UNBLANK) {
                                printf("Turning Off\n");
                                fflush(stdout);
                                on = POWERDOWN;
                                write(lightfd, &on, sizeof(char));
                        }
                }

                nanosleep(&sleepTime, NULL);
        }
}
