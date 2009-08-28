/*
 * Copyright (c) 2009 Tiago Vignatti
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
#include "pciaccess.h"

int
pci_device_vgaarb_init(struct pci_device *dev)
{
#ifdef DEBUG
    fprintf(stderr, "%s: You're using VGA arbiter stub functions!\n",
                                                            __FUNCTION__);
#endif
    return 0;
}

void
pci_device_vgaarb_fini(struct pci_device *dev)
{
}

int
pci_device_vgaarb_set_target(struct pci_device *dev)
{
    return 0;
}

int
pci_device_vgaarb_decodes(struct pci_device *dev)
{
    return 0;
}

int
pci_device_vgaarb_lock(struct pci_device *dev)
{
    return 0;
}

int
pci_device_vgaarb_trylock(struct pci_device *dev)
{
    return 0;
}

int
pci_device_vgaarb_unlock(struct pci_device *dev)
{
    return 0;
}
