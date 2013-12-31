/*
 * Copyright (c) 2011 Vojtech Horky, Jan Vesely
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/** @addtogroup drvusbuhci
 * @{
 */
/** @file
 * @brief UHCI driver initialization
 */

#include <assert.h>
#include <ddf/driver.h>
#include <devman.h>
#include <errno.h>
#include <io/log.h>
#include <pci_dev_iface.h>
#include <stdio.h>
#include <str_error.h>
#include <usb/debug.h>

#include "uhci.h"

#define NAME "uhci"

static int uhci_dev_add(ddf_dev_t *device);

static driver_ops_t uhci_driver_ops = {
	.dev_add = uhci_dev_add,
};

static driver_t uhci_driver = {
	.name = NAME,
	.driver_ops = &uhci_driver_ops
};

/** Call the PCI driver with a request to clear legacy support register
 *
 * @param[in] device Device asking to disable interrupts
 * @return Error code.
 */
static int disable_legacy(ddf_dev_t *device)
{
	assert(device);

	async_sess_t *parent_sess = devman_parent_device_connect(
	    EXCHANGE_SERIALIZE, ddf_dev_get_handle(device), IPC_FLAG_BLOCKING);
	if (!parent_sess)
		return ENOMEM;

	/* See UHCI design guide page 45 for these values.
	 * Write all WC bits in USB legacy register */
	const int rc = pci_config_space_write_16(parent_sess, 0xc0, 0xaf00);

	async_hangup(parent_sess);
	return rc;
}

/** Initialize a new ddf driver instance for uhci hc and hub.
 *
 * @param[in] device DDF instance of the device to initialize.
 * @return Error code.
 */
int uhci_dev_add(ddf_dev_t *device)
{
	usb_log_debug2("uhci_dev_add() called\n");
	assert(device);

	int ret = disable_legacy(device);
	if (ret != EOK) {
		usb_log_error("Failed to disable legacy USB: %s.\n",
		    str_error(ret));
		return ret;
	}


	ret = device_setup_uhci(device);
	if (ret != EOK) {
		usb_log_error("Failed to initialize UHCI driver: %s.\n",
		    str_error(ret));
	} else {
		usb_log_info("Controlling new UHCI device '%s'.\n",
		    ddf_dev_get_name(device));
	}

	return ret;
}

/** Initialize global driver structures (NONE).
 *
 * @param[in] argc Number of arguments in argv vector (ignored).
 * @param[in] argv Cmdline argument vector (ignored).
 * @return Error code.
 *
 * Driver debug level is set here.
 */
int main(int argc, char *argv[])
{
	printf(NAME ": HelenOS UHCI driver.\n");
	log_init(NAME);

	return ddf_driver_main(&uhci_driver);
}
/**
 * @}
 */
