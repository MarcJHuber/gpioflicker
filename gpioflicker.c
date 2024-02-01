/*
 * Copyright (C) 2004-2005 Marc Huber <Marc.Huber@web.de>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * gpioflicker
 *
 * Default behaviour: Enables the error LED on a Soekris Net4501 whenever
 * a packet is logged to pflog0.
 *
 * $Id: gpioflicker.c,v 1.2 2005/07/30 20:58:43 root Exp root $
 *
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <sys/gpio.h>
#include <net/if.h>
#include <net/bpf.h>
#include <errno.h>
#include <err.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sysexits.h>
#include <util.h>

#define SNAPLEN 64

static struct gpio_pin_op op;
static int devfd = -1;
static int led = 0;
static int led_on_val = GPIO_PIN_HIGH;
static int led_off_val = GPIO_PIN_LOW;
static char *device = "/dev/gpio0";

extern char *__progname;

static void usage(char *device, char *ifname, int to_ms)
{
    fprintf(stderr, "\nUsage: %s [options]\n"
	    " Options:\n"
	    "  -f                foreground [off]\n"
	    "  -d device         gpio device [%s]\n"
	    "  -p pin            gpio pin [%d]\n"
	    "  -t milliseconds   minimum period to keep LED active [%d]\n"
	    "  -i interface      network interface [%s]\n"
	    "  -n                don't monopolize gpio device\n"
	    "  -r                invert LED behavior\n"
	    "  -P                write pidfile [off]\n"
	    "  -B basename       pidfile basename (implies -P) [%s]\n"
	    "  -I {0|1}          initial state of gpio pin\n"
	    " Signals:\n"
	    "  USR1 activates gpio pin\n"
	    "  USR2 deactivates gpio pin\n"
	    " Version: 0.3\n",
	    __progname, device, op.gp_pin, to_ms, ifname, __progname);

    exit(EX_USAGE);
}

#define LED_STD 1
#define LED_SIG	2

static void led_on(int);
static void led_off(int);

static void sig_led_on(int unused)
{
    led_on(LED_SIG);
}

static void sig_led_off(int unused)
{
    led_off(LED_SIG);
}

static int (*gpio_ioctl)(int, unsigned long, ...) = NULL;

static int my_ioctl(int d, unsigned long request, ...)
{
    int res = d = open(device, O_RDWR);
    if (d > -1)
    {
	va_list ap;
	va_start(ap, request);
	res = ioctl(d, request, va_arg(ap, struct gpio_pin_op *));
	close (d);
	va_end(ap);
    }
    return res;
}

static void led_on(int mode)
{
    if (!led) {
	op.gp_value = led_on_val;
	gpio_ioctl(devfd, GPIOPINWRITE, &op);
    }
    led |= mode;
    signal(SIGUSR1, sig_led_on);
}

static void led_off(int mode)
{
    if (!(led & ~mode)) {
	op.gp_value = led_off_val;
	gpio_ioctl(devfd, GPIOPINWRITE, &op);
    }
    led &= ~mode;
    signal(SIGUSR2, sig_led_off);
}

static void bye(int unused)
{
    led_off(LED_STD);
    exit(-1);
}

static int bpf_open(char *ifn)
{
    char device[sizeof("/dev/bpf0000000000")];
    struct ifreq ifr;
    int fd;
    u_int i = 0;

    do {
	snprintf(device, sizeof(device), "/dev/bpf%d", i++);
	fd = open(device, O_RDONLY);
    } while (fd < 0 && errno == EBUSY);

    if (fd < 0)
	err(EX_IOERR, "%s", device);

    strlcpy(ifr.ifr_name, ifn, IFNAMSIZ);

    if ((i = 1, ioctl(fd, BIOCIMMEDIATE, &i) < 0)
	|| (i = SNAPLEN, ioctl(fd, BIOCSBLEN, &i) < 0)
	|| (ioctl(fd, BIOCSETIF, &ifr) < 0)
	|| (ioctl(fd, BIOCLOCK) < 0)) {
	close(fd);
	err(EX_IOERR, "ioctl");
    }
    return (fd);
}

int main(int argc, char **argv)
{
    char *ifname = "pflog0";
    char *pid_base = __progname;
    char buf[SNAPLEN];
    int ch, r;
    int foreground = 0;
    int opt_ms = 100;
    int opt_pidfile = 0;
    int initial = -1;
    struct pollfd pfd;

    gpio_ioctl = ioctl;

    memset(&op, 0, sizeof(op));
    op.gp_pin = 9;		/* suitable for Soekris Net4501 */

    while ((ch = getopt(argc, argv, "d:fhi:p:t:PB:I:rn")) != -1) {
	switch (ch) {
	case 'd':
	    device = optarg;
	    break;
	case 'f':
	    foreground = 1;
	    break;
	case 'i':
	    ifname = optarg;
	    break;
	case 'p':
	    op.gp_pin = atoi(optarg);
	    break;
	case 't':
	    opt_ms = atoi(optarg);
	    break;
	case 'I':
	    initial = atoi(optarg);
	    break;
	case 'B':
	    pid_base = optarg;
	case 'P':
	    opt_pidfile = 1;
	    break;
	case 'n':
	    gpio_ioctl = my_ioctl;
	    break;
	case 'r':
	    led_on_val = GPIO_PIN_LOW;
	    led_off_val = GPIO_PIN_HIGH;
	    break;
	default:
	    usage(device, ifname, opt_ms);
	}
    }

    if (argc > optind)
	usage(device, ifname, opt_ms);

    if ((gpio_ioctl == ioctl) && (devfd = open(device, O_RDWR)) < 0)
	err(EX_IOERR, "%s", device);

    if ((pfd.fd = bpf_open(ifname)) < 0)
	err(EX_IOERR, "%s", ifname);

    if (fcntl(pfd.fd, F_SETFL, O_NONBLOCK) < 0)
	err(EX_IOERR, "fcntl");

    if (!foreground)
	daemon(1, 0);

    if (opt_pidfile)
	pidfile(pid_base);

    signal(SIGQUIT, bye);
    signal(SIGHUP, bye);
    signal(SIGTERM, bye);
    signal(SIGUSR1, sig_led_on);
    signal(SIGUSR2, sig_led_off);

    switch (initial) {
    case 0:
	led_off(LED_SIG);
	break;
    case 1:
	led_on(LED_SIG);
	break;
    default:
	;
    }

    pfd.events = POLLIN;
    while (((r = poll(&pfd, 1, led ? opt_ms : -1)) > -1) ||
	   (errno == EINTR)) {
	if (r > 0) {
	    led_on(LED_STD);
	    while (0 < read(pfd.fd, buf, sizeof(buf)));
	} else if (led)
	    led_off(LED_STD);
    }

    led_on(LED_SIG);

    close(pfd.fd);
    if (devfd > -1)
	close(devfd);

    exit(EX_IOERR);
}
