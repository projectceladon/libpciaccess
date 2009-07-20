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
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <limits.h>

#include "pciaccess.h"
#include "pciaccess_private.h"

#define BUFSIZE 64

int
pci_device_vgaarb_init(void)
{
    if ((pci_sys->vgaarb_fd = open ("/dev/vga_arbiter", O_RDWR)) < 0) {
        fprintf(stderr, "device open failed");
        return errno;
    }

    return 0;
}

void
pci_device_vgaarb_fini(void)
{
    if (close(pci_sys->vgaarb_fd) != 0)
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

static int
parse_string_to_decodes_rsrc(char *input, int *vga_count)
{
    char *tok;
    char count[16];

    strncpy(count, input, 10);
    count[11] = 0;

    tok = strtok(count,":");
    if (!tok)
	goto fail;
    tok = strtok(NULL, "");
    if (!tok)
	goto fail;

    *vga_count = strtoul(tok, NULL, 10);
    if (*vga_count == LONG_MAX)
	goto fail;

#ifdef DEBUG
    fprintf(stderr,"vga count is %d\n", *vga_count);
#endif

    tok = strtok(input, ",");
    if (!tok)
	goto fail;

    tok = strtok(NULL, ",");
    if (!tok)
	goto fail;
    tok = strtok(tok, "=");
    if (!tok)
	goto fail;
    tok = strtok(NULL, "=");
    if (!tok)
	goto fail;

    if (!strncmp(tok, "io+mem", 6))
    	return VGA_ARB_RSRC_LEGACY_IO | VGA_ARB_RSRC_LEGACY_MEM;
    if (!strncmp(tok, "io", 2))
    	return VGA_ARB_RSRC_LEGACY_IO;
    if (!strncmp(tok, "mem", 3))
    	return VGA_ARB_RSRC_LEGACY_MEM;
fail:
    return VGA_ARB_RSRC_NONE;
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
    int ret;

    len = snprintf(buf, BUFSIZE, "target PCI:%d:%d:%d.%d",
                   dev->domain, dev->bus, dev->dev, dev->func);

    ret = vgaarb_write(pci_sys->vgaarb_fd, buf, len);
    if (ret)
	return ret;

    ret = read(pci_sys->vgaarb_fd, buf, BUFSIZE);
    if (ret <= 0)
	return -1;

    dev->vgaarb_rsrc = parse_string_to_decodes_rsrc(buf, &pci_sys->vga_count);
    return 0;
}

int
pci_device_vgaarb_decodes(struct pci_device *dev, int new_vgaarb_rsrc)
{
    int len;
    char buf[BUFSIZE];
    int ret;

    if (dev->vgaarb_rsrc == new_vgaarb_rsrc)
	return 0;

    len = snprintf(buf, BUFSIZE, "decodes %s", rsrc_to_str(dev->vgaarb_rsrc));
    ret = vgaarb_write(pci_sys->vgaarb_fd, buf, len);
    if (ret == 0)
        dev->vgaarb_rsrc = new_vgaarb_rsrc;
    return ret;
}

int
pci_device_vgaarb_lock(struct pci_device *dev)
{
    int len;
    char buf[BUFSIZE];

    if (dev->vgaarb_rsrc == 0 || pci_sys->vga_count == 1)
        return 0;

    len = snprintf(buf, BUFSIZE, "lock %s", rsrc_to_str(dev->vgaarb_rsrc));

    return vgaarb_write(pci_sys->vgaarb_fd, buf, len);
}

int
pci_device_vgaarb_trylock(struct pci_device *dev)
{
    int len;
    char buf[BUFSIZE];

    if (dev->vgaarb_rsrc == 0 || pci_sys->vga_count == 1)
        return 0;

    len = snprintf(buf, BUFSIZE, "trylock %s", rsrc_to_str(dev->vgaarb_rsrc));

    return vgaarb_write(pci_sys->vgaarb_fd, buf, len);
}

int
pci_device_vgaarb_unlock(struct pci_device *dev)
{
    int len;
    char buf[BUFSIZE];

    if (dev->vgaarb_rsrc == 0 || pci_sys->vga_count == 1)
        return 0;

    len = snprintf(buf, BUFSIZE, "unlock %s", rsrc_to_str(dev->vgaarb_rsrc));

    return vgaarb_write(pci_sys->vgaarb_fd, buf, len);
}
