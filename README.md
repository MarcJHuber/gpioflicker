# GPIOFLICKER

This is my original OpenBSD gpioflicker code, relocated to Github ...

````
GPIOFLICKER(8)                                                  GPIOFLICKER(8)



NAME
       gpioflicker - link GPIO pin state to network input

SYNOPSIS
       gpioflicker [ -frP ] [ -nrP ] [ -d device ] [ -p pin ] [ -i interface ]
       [ -t milliseconds ] [ -B basename ] [ -I initial_state ]

DESCRIPTION
       gpioflicker changes the state of a GPIO pin on startup, on reception of
       a signal, or whenever a packet arrives on a network interface.  A popu‐
       lar application is to flash GPIO-connected  LEDs  on  firewall  systems
       when packets are blocked.

       The default values are suitable for a Soekris Net4501.

       The  -f  (foreground) option keeps gpioflicker from daemonizing itself.
       Without this option, it will silently detach itself from  the  control‐
       ling terminal and run in the background.

       The  -n  (non-blocking)  option keeps gpioflicker from monopolizing the
       GPIO device. Standard behavior is to keep the device open  permanently,
       but with this option it will only be opened to perform one single ioctl
       call, and then closed immediately. This gives other programs  a  chance
       to  do  their  own GPIO operations, but may result in inconsistent GPIO
       states, because gpioflicker ignores both modifications done  by  others
       and failures opening the GPIO device.

       The  -r  (reverse)  option  inverts the GPIO pin behavior.  Use this on
       systems where enabling a GPIO pin disables the connected LED (and  dis‐
       abling the pin enables it).

       When  the  -P  (pidfile)  option  is  used,  gpioflicker will write its
       process ID to /var/run/gpioflicker.pid. The gpioflicker  part  of  that
       filename  may  be  set by using the -B (basename) option (which implies
       -P).

       The -d and  -p  options  allow  for  selecting  GPIO  device  (default:
       /dev/gpio0) and pin (default: 9).

       The  -t  option  sets the number of milliseconds the GPIO pin should be
       kept active. This defaults to 100.

       The -I option sets the initial pin state. A value of 0 deactivates  the
       pin, a value of 1 activates it.

SIGNALS
       USR1 activates the GPIO pin, USR2 deacticates it.

SEE ALSO
       gpioctl(8).
````
