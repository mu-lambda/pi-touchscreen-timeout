# pi-touchscreen-timeout
Leaving the backlight on the Official Raspberry Pi touchscreen can quickly wear it out.
If you have a use that requires the pi to be on all the time, but does not require the
display on all the time, then turning off the backlight while not in use can dramatically
increase the life of the backlight.

pi-touchscreen-timeout will transparently turn off the display backlight after there 
has been no input for a specifed timeout, independent of anything using the display
at the moment. It will then turn the touchscreen back on when input is received. The
timeout period is set by a command-line argument.

**Note:** This does not stop the event from getting to whatever is running on the
display. Whatever is running will still receive an event, even if the display
is off.

The program will use a linux event device like `/dev/input/event0` to receive events
from the touchscreen, keyboard, mouse, etc., and `/sys/class/backlight/rpi-backlight/bl_power`
to turn the backlight on and off. The event device is a command-line parameter without the
/dev/input/ path specification.

This progam also supports waking up (and powering down) on motion detection using 
[HMMD sensor from WaveShark](https://www.waveshare.com/hmmd-mmwave-sensor.htm).


# Installation

Clone the repository and change directories:
```
git clone https://github.com/mu-lambda/pi-touchscreen-timeout.git
cd pi-touchscreen-timeout
```

Build and run it!
```
make backlight-timeout
sudo ./backlight-timeout 10 /sys/class/backlight/rpi_backlight /dev/input/event0
```
Your backlight might be named differently. To find your backlight name, use `ls /sys/class/backlight/` and replace `rpi_backlight` in the command above

Multiple devices may be specified.

To also add motion detection, use
```
sudo ./backlight-timeout 10 /sys/class/backlight/rpi_backlight --motion /dev/ttyAMA0 /dev/input/event0
```
Replace `/dev/ttyAMA0` with serial port of HMMD mmWave Sensor.

**Note:** It must be run as root or with `sudo` to be able to access the backlight.

It is recommended to make it run at startup, for example by putting a line in 
`/etc/rc.local`


### Conflict with console blanker

When running this program without X Windows running, such as when running a Kivy
program in the console at startup, you may run into a conflict with a console 
blanker.  In such an instance, the backlight will be turned on, but with the 
console blanked, it seems like the backlight has not come on.

In this case, follow one of these methods for disabling the console blanker:
   * Raspbian Jessie : 
     Add the following line to /etc/rc.local (on the line before the final exit 0) 
     and reboot:
```
  sh -c "TERM=linux setterm -blank 0 >/dev/tty0"
```

   Even though /dev/tty0 is used, this should propagate across all terminals.

   * Raspbian Wheezy :
     Edit /etc/kbd/config and change the values for the variable shown below, 
     then reboot:
```
  BLANK_TIME=0
```
