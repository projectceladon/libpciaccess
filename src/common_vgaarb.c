/*
 * Copyright (c) 2007 Paulo R. Zanoni, Tiago Vignatti
 *               2009 Tiago Vignatti
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

#include "pciaccess.h"

/* ALL messages *should* fit in this buffer */
#define BUFSIZE 128
#define VGAARB_DEV "/dev/vga_arbiter"

int
pci_device_vgaarb_init(struct pci_device *dev)
{
    dev->vgaarb_rsrc = VGA_ARB_RSRC_NONE;

    if ((dev->vgaarb_fd = open (VGAARB_DEV, O_RDWR)) < 0) {
        perror("device open failed");
        return 1;
    }

    return 0;
}

void
pci_device_vgaarb_fini(struct pci_device *dev)
{
    if (close(dev->vgaarb_fd) == -1)
        perror("device close failed");
}

/*
 * Writes the message on the device.
 *
 * Returns: 0 if something went wrong
 *      1 if everything is ok
 *      2 if the device returned EBUSY (used ONLY by trylock)
 */
static int
vgaarb_write(int fd, char *buf, int len)
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

        perror("write error");
        return 0;
    }
    else if (ret != len) {
        /* The VGA arbiter implementation shouldn't receive less than one
         * single message. It also shouldn't receive more. */
        fprintf(stderr, "%s: write error: "
                "wrote less than expected\n", __FUNCTION__);
        return 0;
    }

#ifdef DEBUG
    fprintf(stderr, "%s: successfully wrote: '%s'\n", __FUNCTION__, buf);
#endif

    return 1;
}

static const char *
rsrc_to_str(int iostate)
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
#if 0
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
    fprintf(stderr, "%s: error: there is nothing to read\n", __FUNCTION__);
    return 0;
    }
    else if (ret == -1) {
    perror("read error");
    return 0;
    }

#ifdef DEBUG
    fprintf(stderr, "%s: sucessfully read: '%s'\n", __FUCNTION__, buf);
#endif
    return 1;
}
#endif
int
pci_device_vgaarb_set_target(struct pci_device *dev)
{
    int len;
    char buf[BUFSIZE];

    len = snprintf(buf, BUFSIZE, "target PCI:%d:%d:%d.%d",
                   dev->domain, dev->bus, dev->dev, dev->func);

    return vgaarb_write(dev->vgaarb_fd, buf, len);
}

int
pci_device_vgaarb_decodes(struct pci_device *dev)
{
    int len;
    char buf[BUFSIZE];

    len = snprintf(buf, BUFSIZE, "decodes %s", rsrc_to_str(dev->vgaarb_rsrc));

    return vgaarb_write(dev->vgaarb_fd, buf, len);
}

int
pci_device_vgaarb_lock(struct pci_device *dev)
{
    int len;
    char buf[BUFSIZE];

    len = snprintf(buf, BUFSIZE, "lock %s", rsrc_to_str(dev->vgaarb_rsrc));

    return vgaarb_write(dev->vgaarb_fd, buf, len);
}

int
pci_device_vgaarb_trylock(struct pci_device *dev)
{
    int len, write_ret;
    char buf[BUFSIZE];

    len = snprintf(buf, BUFSIZE, "trylock %s", rsrc_to_str(dev->vgaarb_rsrc));

    write_ret = vgaarb_write(dev->vgaarb_fd, buf, len);

    if (write_ret == 0)
        return -1;
    else if (write_ret == 1)
        return 1;
    else
        /* write_ret == 2 and the lock failed */
        return 0;
}

int
pci_device_vgaarb_unlock(struct pci_device *dev)
{
    int len;
    char buf[BUFSIZE];

    len = snprintf(buf, BUFSIZE, "unlock %s", rsrc_to_str(dev->vgaarb_rsrc));

    return vgaarb_write(dev->vgaarb_fd, buf, len);
}
