/* SPDX-License-Identifier: MIT */

/* StACSOS - Kernel
 *
 * Copyright (c) University of St Andrews 2024
 * Tom Spink <tcs6@st-andrews.ac.uk>
 */
#include <stacsos/kernel/debug.h>
#include <stacsos/kernel/fs/vfs.h>

using namespace stacsos::kernel;
using namespace stacsos::kernel::fs;

void vfs::init()
{
	//
}

fs_node *vfs::lookup(const char *path)
{
	// dprintf("vfs: lookup '%s'\n", path);
	if (path[0] != '/') {
		// Probably a better way to do string concat...
		string *pre = new string("usr/");
		auto total = *pre + *(new string(path));
		return rootfs_.root().lookup(total.c_str());
	}

	return rootfs_.root().lookup(&path[1]);
}
