
#include <stacsos/console.h>
#include <stacsos/dirdata.h>
#include <stacsos/list.h>
#include <stacsos/memops.h>
#include <stacsos/objects.h>
#include <stacsos/string.h>

using namespace stacsos;

const int OPT_LONG = 1 << 0; // -l flag
const int OPT_RECURSIVE = 1 << 1; // -r flag
const int OPT_SORTED = 1 << 2; // -s flag

void ls(const char *path, int options)
{
	if (!path) {
		console::get().write("error: usage: ls <path>\n");
		return;
	}

	int dir = object::opendir(path);
	if (dir == -1) {
		console::get().writef("error: unable to open file '%s' for reading\n", path);
		return;
	}

	dirdata directory;

	auto dir_entry = object::readdir((u64)dir, &directory);
	while (dir_entry != -1) {
		if (options & OPT_LONG) {
			if (directory.type == 1) {
				console::get().writef("[D] %s\n", directory.name);
			} else {
				console::get().writef("[F] %s    %d\n", directory.name, directory.size);
			}
		} else {
			console::get().writef("%s\n", directory.name);
		}

		if (directory.type == 1 && (options & OPT_RECURSIVE)) {
			string *filename_string = new string(path);
			string *new_file = new string(directory.name);
			string new_name;
			// Special case for root directory
			if (filename_string->length() == 1 && (*filename_string)[0] == '/') {
				new_name = *filename_string + *new_file;
			} else if (*filename_string->end() == '/') {
				new_name = *filename_string + *new_file;
			} else {
				new_name = *filename_string + "/" + *new_file;
			}
			// recurse!
			// Perhaps doing this iteratively would be better,
			// but then I would need to store the list of directories
			ls(new_name.c_str(), options);
		}
		// flush name
		memops::memset(directory.name, 0, 255);
		dir_entry = object::readdir((u64)dir, &directory);
	}
}

int main(const char *cmdline)
{
	int options = 0;
	const char *filename = nullptr;
	string *str = new string(cmdline);
	list<string> args = str->split(' ', true);

	for (string arg : args) {
		const char *chars = arg.c_str();
		if (chars[0] == '-') {
			int i = 1;
			while (chars[i] != '\0') {
				switch (chars[i]) {
				case 'l':
					options |= OPT_LONG;
					break;
				case 'r':
					options |= OPT_RECURSIVE;
					break;
				case 's':
					options |= OPT_SORTED;
					break;
				}
				i++;
			}
		} else {
			filename = chars;
		}
	}

	ls(filename, options);

	return 0;
}
