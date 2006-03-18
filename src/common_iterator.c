/*
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
 * \file common_iterator.c
 * Platform independent iterator support routines.
 * 
 * \author Ian Romanick <idr@us.ibm.com>
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <regex.h>

#include "pciaccess.h"
#include "pciaccess_private.h"

/**
 * Track device iteration state
 * 
 * \private
 */
struct pci_device_iterator {
    unsigned next_index;
    regex_t   reg;
    int no_regex;
};



/**
 * Create an iterator based on a regular expression.
 * 
 * The set of devices to be iterated is selected by the regular expression
 * passed in \c regex.  The expression matches against an extended PCI bus
 * identifier string. The format of this string is
 * "domain:bus:slot.function:vendor:device_id:subvendor:subdevice_id:class".
 * Unlike classic X bus IDs, all values in the extened bus identifier string
 * are in hexadecimal.  To simplify the required regular expressions, all hex
 * digits greater than 9 will be lower-case. 
 *
 * To match all devices in domain 0, the expression "0:.+" would be used.  To
 * match all devices by ATI, the expression ".+:1002:.+". To match all devices
 * with a class of display, a class of multimedia and a subclass of video, or
 * a class of processor and a subclass of coprocessor, the expression
 * ".+:(03[[:hex:]]2|0400|0b40|0001)$" would be used.  Since this is a fully
 * function regular expression, arbitrarilly complex matches can be requested.
 * 
 * \param pci_sys  Handle for the PCI subsystem.
 * \param regex    Pointer to the regular expression to match against.  If
 *                 \c NULL is passed, all devices will be matched.
 * 
 * \return
 * A pointer to a fully initialized \c pci_device_iterator structure on
 * success, or \c NULL on failure.
 * 
 * \sa pci_device_next, pci_iterator_destroy
 */
struct pci_device_iterator *
pci_iterator_create( const char * re )
{
    struct pci_device_iterator * iter;
    
    if ( pci_sys == NULL ) {
	return NULL;
    }

    iter = malloc( sizeof( *iter ) );
    if ( iter != NULL ) {
	iter->next_index = 0;

	/* If the caller passed a NULL or empty expression, then we don't try
	 * to compile the expression.  Instead we set a flag that tells the
	 * iterator routine to iterate every device in the list.
	 */
	if ( (re != NULL) && (strlen( re ) > 0) ) {
	    int err = regcomp( & iter->reg, re, REG_EXTENDED | REG_NOSUB );
	    if ( err != 0 ) {
		free( iter );
		iter = NULL;
	    }

	    iter->no_regex = 0;
	}
	else {
	    iter->no_regex = 1;
	}
    }

    return iter;
}


/**
 * Destroy an iterator previously created with \c pci_iterator_create.
 * 
 * \param iter  Iterator to be destroyed.
 * 
 * \sa pci_device_next, pci_iterator_create
 */
void
pci_iterator_destroy( struct pci_device_iterator * iter )
{
    if ( iter != NULL ) {
	if ( ! iter->no_regex ) {
	    regfree( & iter->reg );
	}

	free( iter );
    }
}


static void
fill_device_string( struct pci_device_private * d )
{

    if ( d->device_string == NULL ) {
	char * const string = malloc( 40 );
	if ( string != NULL ) {
	    pci_device_probe( (struct pci_device *) d );
	    sprintf( string, "%04x:%02x:%02x.%u:%04x:%04x:%04x:%04x:%06x",
		     d->base.domain,
		     d->base.bus,
		     d->base.dev,
		     d->base.func,
		     d->base.vendor_id,
		     d->base.device_id,
		     d->base.subvendor_id,
		     d->base.subdevice_id,
		     d->base.device_class );

	    d->device_string = string;
	}
    }
}


/**
 * Iterate to the next PCI device.
 * 
 * \param iter  Device iterator returned by \c pci_device_iterate.
 * 
 * \return
 * A pointer to a \c pci_device, or \c NULL when all devices have been
 * iterated.
 *
 * \bug
 * The only time this routine should be able to return \c NULL is when the
 * end of the list is hit.  However, there is a memory allocation (via
 * \c fill_device_string) that can fail.  If this allocation fails, \c NULL
 * will be erroneously returned.  What should be done here?  Pre-fill the
 * device strings in \c pci_iterator_create?
 */
struct pci_device *
pci_device_next( struct pci_device_iterator * iter )
{
    struct pci_device_private * d = NULL;

    if ( iter->no_regex ) {
	if ( iter->next_index < pci_sys->num_devices ) {
	    d = & pci_sys->devices[ iter->next_index ];
	    iter->next_index++;
	}
    }
    else {
	while ( iter->next_index < pci_sys->num_devices ) {
	    struct pci_device_private * const temp = 
	      & pci_sys->devices[ iter->next_index ];

	    if ( temp->device_string == NULL ) {
		fill_device_string( temp );		
		if ( temp->device_string == NULL ) {
		    break;
		}
	    }

	    iter->next_index++;

	    if ( regexec( & iter->reg, temp->device_string, 0, NULL, 0 ) == 0 ) {
		d = temp;
		break;
	    }
	}
    }
    
    return (struct pci_device *) d;
}
