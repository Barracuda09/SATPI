/* ChildPIPEReader.h

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
#ifndef BASE_CHILD_PIPE_READER
#define BASE_CHILD_PIPE_READER BASE_CHILD_PIPE_READER

#include <Log.h>
#include <Utils.h>
#include <StringConverter.h>
#include <base/CharPointerArray.h>

#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>

namespace base {

class ChildPIPEReader {
		// =====================================================================
		//  -- Constructors and destructor -------------------------------------
		// =====================================================================
	public:

		ChildPIPEReader()  {}

		virtual ~ChildPIPEReader() {
			close();
		}

		// =====================================================================
		//  -- Other member functions ------------------------------------------
		// =====================================================================
	public:

		void open(const std::string &cmd) {
			popen2(cmd);
		}

		void close() {
			if (_open) {
				CLOSE_FD(_stdout);
				::kill(_pid, SIGKILL);
				_open = false;
			}
		}

		bool isOpen() const {
			return _open;
		}

		std::size_t read(unsigned char *buffer, std::size_t size) {
			return ::read(_stdout, buffer, size);
		}

	private:

		void popen2(const std::string &cmd) {
			int pipefd[2] = { -1, -1 };
			if (::pipe2(pipefd, 0) != 0) {
				return;
			}
			StringVector argumentVector = StringConverter::parseCommandArgumentString(cmd);
			const CharPointerArray charPtrArray(argumentVector);
			char * const* argv = charPtrArray.getData();

			// Prevent children going into 'zombie' mode
			signal(SIGCHLD, SIG_IGN);

			_pid = ::fork();

			if (_pid < 0) {
				// Something wrong here
				return;
			} else if (_pid == 0) {
				// This is the child process
				CLOSE_FD(pipefd[READ]);
				::dup2(pipefd[WRITE], WRITE);

				::execvp(argv[0], argv);
				SI_LOG_PERROR("execvp");
				exit(1);
			}
			// This is the parent process
			CLOSE_FD(pipefd[WRITE]);
			_stdout = pipefd[READ];
			_open = true;
		}

		// =====================================================================
		// -- Data members -----------------------------------------------------
		// =====================================================================
	private:

		static constexpr int READ = 0;
		static constexpr int WRITE = 1;
		int _stdout = -1;
		pid_t _pid = -1;
		bool _open = false;
};

} // namespace base

#endif // BASE_CHILD_PIPE_READER
