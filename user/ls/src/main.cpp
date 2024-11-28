
#include <stacsos/console.h>
#include <stacsos/memops.h>
#include <stacsos/objects.h>

using namespace stacsos;

int main(const char *cmdline)
{
	console::get().write("NIHAOMAI\n");
	;
	if (!cmdline || memops::strlen(cmdline) == 0) {
		console::get().write("error: usage: ls <path>\n");
		return 1;
	}

	object *dir = object::opendir(cmdline);
	if (!dir) {
		console::get().writef("error: unable to open file '%s' for reading\n", cmdline);
		return 1;
	}
	console::get().write("WE ARE SOOO BACK\n");
	;
	return 0;
}
