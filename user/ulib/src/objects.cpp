/* SPDX-License-Identifier: MIT */

/* StACSOS - userspace standard library
 *
 * Copyright (c) University of St Andrews 2024
 * Tom Spink <tcs6@st-andrews.ac.uk>
 */
#include <stacsos/objects.h>
#include <stacsos/user-syscall.h>
#include <stacsos/console.h>

using namespace stacsos;

object *object::open(const char *path)
{
	auto result = syscalls::open(path);
	if (result.code != syscall_result_code::ok) {
		return nullptr;
	}

	return new object(result.id);
}

int object::opendir(const char *path)
{
	auto result = syscalls::opendir(path);
	if (result.code != syscall_result_code::ok) {
		return -1;
	}
	return result.id;
}

int object::readdir(u64 obj, const void * dirdata)
{
	auto result = syscalls::readdir(obj, dirdata);
	if (result.code != syscall_result_code::ok) {
		return -1;
	}
	return result.id;
}

int object::close(u64 obj) {
	auto result = syscalls::close(obj);
	return (int) result;
}

object::~object() { syscalls::close(handle_); }

size_t object::read(void *buffer, size_t length) { return syscalls::read(handle_, buffer, length).length; }
size_t object::write(const void *buffer, size_t length) { return syscalls::write(handle_, buffer, length).length; }
size_t object::pwrite(const void *buffer, size_t length, size_t offset) { return syscalls::pwrite(handle_, buffer, length, offset).length; }
size_t object::pread(void *buffer, size_t length, size_t offset) { return syscalls::pread(handle_, buffer, length, offset).length; }
u64 object::ioctl(u64 cmd, void *buffer, size_t length) { return syscalls::ioctl(handle_, cmd, buffer, length).length; }
