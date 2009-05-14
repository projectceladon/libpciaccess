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

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "pciaccess.h"

#define BUFSIZE 32

int
pci_device_vgaarb_init(struct pci_device *dev)
{
    dev->vgaarb_rsrc = VGA_ARB_RSRC_NONE;

    if ((dev->vgaarb_fd = open ("/dev/vga_arbiter", O_RDWR)) < 0) {
        fprintf(stderr, "device open failed");
        return errno;
    }

    return 0;
}

void
pci_device_vgaarb_fini(struct pci_device *dev)
{
    if (close(dev->vgaarb_fd) != 0)
        fprintf(stderr, "device close failed");
}

/**
 * Writes message on vga device. The messages are defined by the kernel
 * implementation.
 *
 * \param fd    vga arbiter device.
 * \param buf   message itself.
 * \param len   message length.
 *
 * \return
 * Zero on success, 1 if something gets wrong and 2 if fd is busy (only for
 * 'trylock')
 */
static int
vgaarb_write(int fd, char *buf, int len)
{
    int ret;


    buf[len] = '\0';

    ret = write(fd, buf, len);
    if (ret == -1) {
        /* the user may have called "trylock" and didn't get the lock */
        if (errno == EBUSY)
            return 2;

        fprintf(stderr, "write error");
        return 1;
    }
    else if (ret != len) {
        /* it's need to receive the exactly amount required. */
        fprintf(stderr, "write error: wrote different than expected\n");
        return 1;
    }

#ifdef DEBUG
    fprintf(stderr, "%s: successfully wrote: '%s'\n", __FUNCTION__, buf);
#endif

    return 0;
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
    int len;
    char buf[BUFSIZE];

    len = snprintf(buf, BUFSIZE, "trylock %s", rsrc_to_str(dev->vgaarb_rsrc));

    return vgaarb_write(dev->vgaarb_fd, buf, len);
}

int
pci_device_vgaarb_unlock(struct pci_device *dev)
{
    int len;
    char buf[BUFSIZE];

    len = snprintf(buf, BUFSIZE, "unlock %s", rsrc_to_str(dev->vgaarb_rsrc));

    return vgaarb_write(dev->vgaarb_fd, buf, len);
}
