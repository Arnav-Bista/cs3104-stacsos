#pragma once

#include <stacsos/kernel/fs/fs-node.h>
#include <stacsos/kernel/debug.h>

namespace stacsos::kernel::fs {

class directory {
public:
	directory(fs_node &node)
		: node_(node)
	{
	}

	fs_node &get_node() { return node_; }

private:
	fs_node &node_;
};
} // namespace stacsos::kernel::fs
