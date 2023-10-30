/* Utils.cpp

   Copyright (C) 2014 - 2023 Marc Postema (mpostema09 -at- gmail.com)

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
   Or, point your browser to http://www.gnu.org/copyleft/gpl.html
 */
#include <Utils.h>
#include <base/ChildPIPEReader.h>

#include <iostream>
#include <fstream>
#include <algorithm>

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <execinfo.h>

#ifdef HAS_NO_BACKTRACE_FUNCTIONS
void Utils::createBackTrace(const char *) {
#warning Compiling with no backtrace!
#else
void Utils::createBackTrace(const char *file) {
	// DO NOT alloc memory on heap!!
	void *array[25];
	const size_t size = backtrace(array, 25);

	// Log all the frames to Backtrace File
	char path[256];
	snprintf(path, sizeof(path), "/tmp/%s.bt", file);
	int backtraceFile = ::open(path, O_CREAT|O_WRONLY|O_TRUNC, 0664);
	if (backtraceFile > 0) {
		backtrace_symbols_fd(array, size, backtraceFile);
		::close(backtraceFile);
	} else {
		backtrace_symbols_fd(array, size, STDOUT_FILENO);
	}
#endif
}

void Utils::annotateBackTrace(const char *app, const char *file) {
	std::ifstream bt(file, std::ios::in);
	int i = 0;
	for (std::string line; std::getline(bt, line); ++i) {
		const auto begin = line.find('[');
		const auto end = line.find(']');
		if (begin != std::string::npos && end != std::string::npos) {
			const std::string addr = line.substr(begin + 1, end - begin - 1);

			char addr2line[256];
			snprintf(addr2line, sizeof(addr2line),"addr2line %s -e %s", addr.data(), app);

			base::ChildPIPEReader exec;
			exec.open(addr2line);
			std::string code;
			char buffer[256];
			std::size_t s;
			while ((s = exec.read(reinterpret_cast<unsigned char *>(buffer), 255)) > 0) {
				code.append(buffer, s);
				code.erase(std::find(code.begin(), code.end(), '\n'));
			}

			std::cout << "[bt] #" << i << " " << code << " -- " << line << std::endl;
		}
	}
}
