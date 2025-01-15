#include <errno.h>
#include <fcntl.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

int set_interface_attribs(int fd, int speed)
{
    struct termios tty;

    if (tcgetattr(fd, &tty) < 0) {
        printf("Error from tcgetattr: %s\n", strerror(errno));
        return -1;
    }

    cfsetospeed(&tty, (speed_t)speed);
    cfsetispeed(&tty, (speed_t)speed);

    tty.c_cflag |= (CLOCAL | CREAD);    /* ignore modem controls */
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;         /* 8-bit characters */
    tty.c_cflag &= ~PARENB;     /* no parity bit */
    tty.c_cflag &= ~CSTOPB;     /* only need 1 stop bit */
    tty.c_cflag &= ~CRTSCTS;    /* no hardware flowcontrol */

    /* setup for non-canonical mode */
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    tty.c_oflag &= ~OPOST;

    /* fetch bytes as they become available */
    tty.c_cc[VMIN] = 1;
    tty.c_cc[VTIME] = 1;

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        printf("Error from tcsetattr: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

void set_mincount(int fd, int mcount)
{
    struct termios tty;

    if (tcgetattr(fd, &tty) < 0) {
        printf("Error tcgetattr: %s\n", strerror(errno));
        return;
    }

    tty.c_cc[VMIN] = mcount ? 1 : 0;
    tty.c_cc[VTIME] = 5;        /* half second timer */

    if (tcsetattr(fd, TCSANOW, &tty) < 0)
        printf("Error tcsetattr: %s\n", strerror(errno));
}

typedef enum {
    INITIAL,
    O_FOUND,
    N_FOUND,
    F_FOUND,
    ON_DECTECTED,
    OFF_DETECTED
} state_t;

int is_motion(char *portname)
{
    int fd;
    int wlen;
    char *xstr = "\xFD\xFC\xFB\xFA\x08\x00\x12\x00\x00\x00\x64\x00\x00\x00\x04\x03\x02\x01";
    int xlen = strlen(xstr);

    fd = open(portname, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
        printf("Error opening %s: %s\n", portname, strerror(errno));
        return -1;
    }
    /*baudrate 115200, 8 bits, no parity, 1 stop bit */
    set_interface_attribs(fd, B115200);
    //set_mincount(fd, 0);                /* set to pure timed read */

    /* simple output */
    wlen = write(fd, xstr, xlen);
    if (wlen != xlen) {
        printf("Error from write: %d, %d\n", wlen, errno);
    }
    tcdrain(fd);    /* delay for output */


    /* simple noncanonical input */
    state_t state = INITIAL;
    do {
        unsigned char buf[80];
        int rdlen;

        rdlen = read(fd, buf, sizeof(buf) - 1);
        if (rdlen > 0) {
            for (int i = 0; i < rdlen && state != ON_DECTECTED && state != OFF_DETECTED; i++) {
                unsigned char c = buf[i];
                switch(state) {
                    case INITIAL:
                        switch(c) {
                            case 'O': state = O_FOUND; break;                            
                            default: state = INITIAL; break;
                        }
                        break;
                    case O_FOUND:
                        switch(c) {
                            case 'N': state = ON_DECTECTED; break;
                            case 'F': state = F_FOUND; break;
                            default: state = INITIAL; break;
                        }
                        break;
                    case F_FOUND:
                        switch(c) {
                            case 'F': state = OFF_DETECTED; break;
                            default: state = INITIAL; break;
                        }
                    default: state = INITIAL; break;                            
                }
            }
            //buf[rdlen] = 0;
            //printf("Read %d: \"%s\"\n", rdlen, buf);

        } else if (rdlen < 0) {
            printf("Error from read: %d: %s\n", rdlen, strerror(errno));
        } else {  /* rdlen == 0 */
            printf("Timeout from read\n");
        }               
        if (state == ON_DECTECTED || state == OFF_DETECTED) {
            close(fd);
            return state == ON_DECTECTED;
        }
    } while (1);
}
