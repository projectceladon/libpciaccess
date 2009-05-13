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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include "pciaccess.h"


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
#ifdef DEBUG
    fprintf(stderr, "%s: YOU'RE USING THE STUB FUNCTIONS!\n", __FUNCTION__);
#endif
    return 1;
}

void
vga_arb_fini(vga_arb_ptr vgaDev)
{
}
