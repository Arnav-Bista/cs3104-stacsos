#pragma once 

#include <stacsos/kernel/fs/fs-node.h>

namespace stacsos::kernel::fs {

class directory {
public:
	directory(fs_node &node): node_(node) {}

private:
	fs_node &node_;
};
}
