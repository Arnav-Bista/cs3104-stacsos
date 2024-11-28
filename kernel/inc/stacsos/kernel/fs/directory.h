#pragma once

#include <stacsos/kernel/fs/fs-node.h>

namespace stacsos::kernel::fs {

class directory {
public:
	directory(fs_node &node)
		: node_(node)
	{
	}

	virtual size_t pread(void *buffer, size_t offset, size_t length)
	{
		fs_node **pointer = (fs_node **)buffer;
		*pointer = &node_;
		return sizeof(fs_node*);
	};

	fs_node &get_node() { return node_; }

private:
	fs_node &node_;
};
} // namespace stacsos::kernel::fs
