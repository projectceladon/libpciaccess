/*
 * Copyright (c) 2007 Paulo R. Zanoni, Tiago Vignatti
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

/* vgaaccess.c */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "vgaaccess.h"

/* ALL messages *should* fit in this buffer */
#define BUFSIZE 128

#define ARBITER_DEVICE "/dev/vga_arbiter"

/*
 * Writes the message on the device.
 *
 * Returns: 0 if something went wrong
 *      1 if everything is ok
 *      2 if the device returned EBUSY (used ONLY by trylock)
 */
static int
vga_arb_write(int fd, char *buf, int len)
{
    int ret;
   
    /* Just to make sure... */
    buf[len] = '\0';

    ret = write(fd, buf, len);

    if (ret == -1) {
    /* Check for EBUSY: the user may have called "trylock" and didn't get
     * the lock. */
    if (errno == EBUSY)
        return 2;
        perror("[libvgaaccess] write error");
    return 0;
    }
    else if (ret != len) {
    /* The VGA arbiter implementation shouldn't recive less than one
     * single message. It also shouldn't recive more. */
    fprintf(stderr, "[libvgaaccess] write error: "
            "wrote less than expected!\n");
    return 0;
    }

#ifdef DEBUG
    fprintf(stderr, "[libvgaaccess] successfully wrote: '%s'.\n", buf);
#endif

    return 1;
}

/* Convert "integer rsrc" in "string rsrc" */
static const char *
rsrc_to_str(VgaArbRsrcType iostate)
{
        switch (iostate) {
        case VGA_ARB_RSRC_LEGACY_IO | VGA_ARB_RSRC_LEGACY_MEM:
                return "io+mem";
        case VGA_ARB_RSRC_LEGACY_IO:
                return "io";
        case VGA_ARB_RSRC_LEGACY_MEM:
                return "mem";
        }
        return "none";
}

#ifndef STUB
int
vga_arb_read(vga_arb_ptr vgaDev)
{
    int ret;
    char buf[BUFSIZE];


    ret = read (vgaDev->fd, buf, BUFSIZE);

    /* Just to make sure... */
    buf[ret]='\0';

    if (ret == 0) {
    /* It always has something to be read. */
    fprintf(stderr, "[libvgaaccess] error: there is nothing to read!\n");
    return 0;
    }
    else if (ret == -1) {
    perror("[libvgaaccess] read error");
    return 0;
    }

#ifdef DEBUG    
    fprintf(stderr, "[libvgaaccess]: sucessfully read: '%s'.\n", buf);
#endif
    return 1;
}

int
vga_arb_set_target(vga_arb_ptr vgaDev, unsigned int domain, unsigned int bus,
               unsigned int dev, unsigned int fn)
{
    int len;
    char buf[BUFSIZE];

    len = snprintf(buf, BUFSIZE, "target PCI:%d:%d:%d.%d",
                   domain, bus, dev, fn);

    return vga_arb_write(vgaDev->fd, buf, len);
}

int
vga_arb_lock(vga_arb_ptr vgaDev)
{
    int len;
    char buf[BUFSIZE];

    len = snprintf(buf, BUFSIZE, "lock %s", rsrc_to_str(vgaDev->rsrc));

    return vga_arb_write(vgaDev->fd, buf, len);
}

int
vga_arb_trylock(vga_arb_ptr vgaDev)
{
    int len, write_ret;
    char buf[BUFSIZE];

    len = snprintf(buf, BUFSIZE, "trylock %s", rsrc_to_str(vgaDev->rsrc));

    write_ret = vga_arb_write(vgaDev->fd, buf, len);

    if (write_ret == 0)
        return -1;
    else if (write_ret == 1)
        return 1;
    else
        /* write_ret == 2 and the lock failed */
        return 0;
}

int
vga_arb_unlock(vga_arb_ptr vgaDev)
{
    int len;
    char buf[BUFSIZE];

    len = snprintf(buf, BUFSIZE, "unlock %s", rsrc_to_str(vgaDev->rsrc));

    return vga_arb_write(vgaDev->fd, buf, len);
}

int
vga_arb_decodes(vga_arb_ptr vgaDev)
{
    int len;
    char buf[BUFSIZE];

    len = snprintf(buf, BUFSIZE, "decodes %s", rsrc_to_str(vgaDev->rsrc));

    return vga_arb_write(vgaDev->fd, buf, len);
}

int
vga_arb_init(vga_arb_ptr *vgaDev)
{
    *vgaDev = malloc (sizeof(vga_arb_ptr *));
    if (vgaDev == NULL) {
    fprintf(stderr, "[libvgaaccess] malloc: couldn't allocate memory!\n");
    return 0;
    }

    (*vgaDev)->rsrc = 0;

    if (((*vgaDev)->fd = open (ARBITER_DEVICE, O_RDWR)) < 0) {
        perror("[libvgaaccess] device open failed");
        return 0;
    }

    return (*vgaDev)->fd;
}

void
vga_arb_fini(vga_arb_ptr vgaDev)
{
    if (close(vgaDev->fd) == -1)
    perror("[libvgaaccess] device close failed");
}

#else /* STUB */
int
vga_arb_read(vga_arb_ptr vgaDev)
{
    return 1;
}

int
vga_arb_set_target(vga_arb_ptr vgaDev, unsigned int domain, unsigned int bus,
                   unsigned int dev,    unsigned int fn)
{
    return 1;
}

int
vga_arb_lock(vga_arb_ptr vgaDev)
{
    return 1;
}

int
vga_arb_trylock(vga_arb_ptr vgaDev)
{
    return 1;
}

int
vga_arb_unlock(vga_arb_ptr vgaDev)
{
    return 1;
}

int
vga_arb_decodes(vga_arb_ptr vgaDev)
{
    return 1;
}

int
vga_arb_init(vga_arb_ptr *vgaDev)
{
    fprintf(stderr, "[libvgaaccess] YOU'RE USING THE STUB FUNCTIONS!\n");
    return 1;
}

void
vga_arb_fini(vga_arb_ptr vgaDev)
{
}
#endif /* END OF STUB */
