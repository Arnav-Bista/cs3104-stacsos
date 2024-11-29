
#include <stacsos/console.h>
#include <stacsos/dirdata.h>
#include <stacsos/list.h>
#include <stacsos/memops.h>
#include <stacsos/objects.h>
#include <stacsos/string.h>

using namespace stacsos;

int main(const char *cmdline)
{
	bool long_listing = false;
	bool recursive = false;

	const char *filename = nullptr;
	string *str = new string(cmdline);
	list<string> args = str->split(' ', true);

	for (string arg : args) {
		const char *chars = arg.c_str();
		if (chars[0] == '-') {
			int i = 1;
			while (chars[i] != '\0') {
				if (chars[i] == 'l') {
					long_listing = true;
				} else if (chars[i] == 'r') {
					recursive = true;
				}
				i++;
			}
		} else {
			filename = chars;
		}
	}

	if (filename == nullptr) {
		console::get().write("error: usage: ls <path>\n");
		return 1;
	}

	int dir = object::opendir(filename);
	if (dir == -1) {
		console::get().writef("error: unable to open file '%s' for reading\n", cmdline);
		return 1;
	}

	dirdata dirr;

	auto dir_entry = object::readdir((u64)dir, &dirr);
	while (dir_entry != -1) {
		if (long_listing) {
			console::get().writef("[%s] %s    %d\n", dirr.type == 1 ? "D" : "F", dirr.name, dirr.size);
		} else {
			console::get().writef("%s\n", dirr.name);
		}

		if (dirr.type == 1 && recursive) {
			string *filename_string = new string(filename);
			string *new_file = new string(dirr.name);
			string new_name = *filename_string + "/" + *new_file;
			if (recursive) {
				new_name += " -r";
			}
			if (long_listing) {
				new_name += " -l";
			}
			main(new_name.c_str());
		}
		// flush name
		memops::memset(dirr.name, 0, 255);
		dir_entry = object::readdir((u64)dir, &dirr);
	}

	return 0;
}
