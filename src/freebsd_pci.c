/*
 * (C) Copyright Eric Anholt 2006
 * (C) Copyright IBM Corporation 2006
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * IBM AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/**
 * \file freebsd_pci.c
 *
 * Access the kernel PCI support using /dev/pci's ioctl and mmap interface.
 *
 * \author Eric Anholt <eric@anholt.net>
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/pciio.h>
#include <sys/mman.h>

#include "pciaccess.h"
#include "pciaccess_private.h"

/**
 * FreeBSD private pci_system structure that extends the base pci_system
 * structure.
 *
 * It is initialized once and used as a global, just as pci_system is used.
 */
struct freebsd_pci_system {
    struct pci_system pci_sys;

    int pcidev; /**< fd for /dev/pci */
} *freebsd_pci_sys;

/**
 * Map a memory region for a device using /dev/mem.
 *
 * \param dev          Device whose memory region is to be mapped.
 * \param region       Region, on the range [0, 5], that is to be mapped.
 * \param write_enable Map for writing (non-zero).
 *
 * \return
 * Zero on success or an \c errno value on failure.
 */
static int
pci_device_freebsd_map( struct pci_device *dev, unsigned region,
			int write_enable )
{
    int fd, err = 0, prot;

    fd = open( "/dev/mem", write_enable ? O_RDWR : O_RDONLY );
    if ( fd == -1 )
	return errno;

    prot = write_enable ? PROT_READ : (PROT_READ | PROT_WRITE);
    dev->regions[ region ].memory = mmap( NULL, dev->regions[ region ].size,
					  prot, MAP_SHARED, fd, 0 );

    if ( dev->regions[ region ].memory == MAP_FAILED ) {
	dev->regions[ region ].memory = NULL;
	err = errno;
    }

    close( fd );

    return err;
}

/**
 * Unmap the specified region.
 *
 * \param dev          Device whose memory region is to be unmapped.
 * \param region       Region, on the range [0, 5], that is to be unmapped.
 *
 * \return
 * Zero on success or an \c errno value on failure.
 */
static int
pci_device_freebsd_unmap( struct pci_device * dev, unsigned region )
{
    int err = 0;

    if ( munmap( dev->regions[ region ].memory,
		 dev->regions[ region ].size ) == -1) {
	err = errno;
    }

    dev->regions[ region ].memory = NULL;

    return err;
}

static int
pci_device_freebsd_read( struct pci_device * dev, void * data,
			 pciaddr_t offset, pciaddr_t size,
			 pciaddr_t * bytes_read )
{
    struct pci_io io;

    io.pi_sel.pc_bus = dev->bus;
    io.pi_sel.pc_dev = dev->dev;
    io.pi_sel.pc_func = dev->func;

    *bytes_read = 0;
    while ( size > 0 ) {
	int toread = (size < 4) ? size : 4;

	io.pi_reg = offset;
	io.pi_width = toread;

	if ( ioctl( freebsd_pci_sys->pcidev, PCIOCREAD, &io ) < 0 ) 
	    return errno;

	memcpy(data, &io.pi_data, toread );

	offset += toread;
	data += toread;
	size -= toread;
	*bytes_read += toread;
    }
}


static int
pci_device_freebsd_write( struct pci_device * dev, const void * data,
			  pciaddr_t offset, pciaddr_t size,
			  pciaddr_t * bytes_written )
{
    struct pci_io io;

    io.pi_sel.pc_bus = dev->bus;
    io.pi_sel.pc_dev = dev->dev;
    io.pi_sel.pc_func = dev->func;

    *bytes_written = 0;
    while ( size > 0 ) {
	int towrite = (size < 4 ? size : 4);

	io.pi_reg = offset;
	io.pi_width = towrite;
	memcpy( &io.pi_data, data, towrite );

	if ( ioctl( freebsd_pci_sys->pcidev, PCIOCWRITE, &io ) < 0 ) 
	    return errno;

	offset += towrite;
	data += towrite;
	size -= towrite;
	*bytes_written += towrite;
    }
}

static const struct pci_system_methods freebsd_pci_methods = {
    .destroy = NULL, /* XXX: free memory */
    .destroy_device = NULL,
    .read_rom = NULL, /* XXX: Fill me in */
    .probe = NULL, /* XXX: Fill me in */
    .map = pci_device_freebsd_map,
    .unmap = pci_device_freebsd_unmap,
    .read = pci_device_freebsd_read,
    .write = pci_device_freebsd_write,
    .fill_capabilities = pci_fill_capabilities_generic,
};

/**
 * Attempt to access the FreeBSD PCI interface.
 */
int
pci_system_freebsd_create( void )
{
    struct pci_conf_io pciconfio;
    struct pci_conf pciconf[255];
    struct freebsd_pci_system *freebsd_pci_sys;
    int pcidev;
    int i;

    /* Try to open the PCI device */
    pcidev = open( "/dev/pci", O_RDONLY );
    if ( pcidev == -1 )
	return ENXIO;

    freebsd_pci_sys = calloc( 1, sizeof( struct freebsd_pci_system ) );
    if ( freebsd_pci_sys == NULL ) {
	close( pcidev );
	return ENOMEM;
    }
    pci_sys = &freebsd_pci_sys->pci_sys;

    pci_sys->methods = & freebsd_pci_methods;
    freebsd_pci_sys->pcidev = pcidev;

    /* Probe the list of devices known by the system */
    bzero( &pciconf, sizeof( struct pci_conf_io ) );
    pciconfio.match_buf_len = sizeof(pciconf);
    pciconfio.matches = pciconf;

    if ( ioctl( pcidev, PCIOCGETCONF, &pciconfio ) == -1) {
	free( pci_sys );
	close( pcidev );
	return errno;
    }

    if (pciconfio.status == PCI_GETCONF_ERROR ) {
	free( pci_sys );
	close( pcidev );
	return EINVAL;
    }

    /* Translate the list of devices into pciaccess's format. */
    pci_sys->num_devices = pciconfio.num_matches;
    pci_sys->devices = calloc( pciconfio.num_matches,
			       sizeof( struct pci_device_private ) );

    for ( i = 0; i < pciconfio.num_matches; i++ ) {
	struct pci_conf *p = &pciconf[ i ];

	pci_sys->devices[ i ].base.domain = 0; /* XXX */
	pci_sys->devices[ i ].base.bus = p->pc_sel.pc_bus;
	pci_sys->devices[ i ].base.dev = p->pc_sel.pc_dev;
	pci_sys->devices[ i ].base.func = p->pc_sel.pc_func;
    }

    return 0;
}
