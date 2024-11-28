
#include <stacsos/console.h>
#include <stacsos/memops.h>
#include <stacsos/objects.h>
#include <stacsos/dirdata.h>

using namespace stacsos;

int main(const char *cmdline)
{
	if (!cmdline || memops::strlen(cmdline) == 0) {
		console::get().write("error: usage: ls <path>\n");
		return 1;
	}

	int dir = object::opendir(cmdline);
	if (!dir) {
		console::get().writef("error: unable to open file '%s' for reading\n", cmdline);
		return 1;
	}

	console::get().writef("got %d as dir\n", dir);


	u64 size; 
	int type;
	char name[255];

	console::get().write("WE ARE SOOO BACK\n");
	auto a = object::readdir((u64)dir, &name[0], &size, &type);
	console::get().writef("%d awca\n", a);

	console::get().write("DONE!");

	console::get().writef("[%s] %s\t%d\n", type == 1 ? "D" : "F", name, size);
	// object::close(dir);
	
	return 0;
}
