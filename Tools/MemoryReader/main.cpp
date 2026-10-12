// MemoryReader - READ-ONLY Linux learning tool for understanding Base + Offset.
//
// Finds a running process, finds the load address of one of its modules
// (from /proc/<pid>/maps) and follows a chain of offsets (via /proc/<pid>/mem)
// to print one value. It never writes to another process.
//
// Only use it on your own programs (see practice_target.cpp) or on games run
// offline without anti-cheat (CS2: private server with bots, -insecure).
//
// Reading another process may need: same user + `sudo sysctl kernel.yama.ptrace_scope=0`,
// or simply running with sudo.

#include <dirent.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace
{
	struct State
	{
		pid_t Pid = 0;
		std::string ProcessName;
		std::string ModuleName;
		uintptr_t Base = 0;
		std::string Type = "int";
		std::vector<uintptr_t> Offsets;
		bool Labels = true; // annotate every offset with its role (toggle: Insert menu)
	};

	volatile sig_atomic_t gStop = 0;
	void OnSigInt(int) { gStop = 1; }

	std::string Basename(const std::string& Path)
	{
		const size_t Slash = Path.find_last_of('/');
		return Slash == std::string::npos ? Path : Path.substr(Slash + 1);
	}

	bool IsNumber(const char* S)
	{
		if (!*S) return false;
		for (; *S; ++S) if (*S < '0' || *S > '9') return false;
		return true;
	}

	// Matches against the executable name (/proc/<pid>/exe) and /proc/<pid>/comm.
	pid_t FindPid(const std::string& Name)
	{
		DIR* Dir = opendir("/proc");
		if (!Dir) return 0;
		pid_t Found = 0;
		while (dirent* E = readdir(Dir))
		{
			if (!IsNumber(E->d_name)) continue;
			const std::string Base = std::string("/proc/") + E->d_name;
			std::string Comm;
			std::ifstream(Base + "/comm") >> Comm;
			char Exe[4096];
			const ssize_t N = readlink((Base + "/exe").c_str(), Exe, sizeof(Exe) - 1);
			const std::string ExeName = N > 0 ? Basename(std::string(Exe, N)) : "";
			if (Comm == Name || ExeName == Name)
			{
				Found = static_cast<pid_t>(atoi(E->d_name));
				break;
			}
		}
		closedir(Dir);
		return Found;
	}

	// Lowest mapped address of a file whose basename equals ModuleName.
	uintptr_t FindModuleBase(pid_t Pid, const std::string& ModuleName)
	{
		std::ifstream Maps("/proc/" + std::to_string(Pid) + "/maps");
		std::string Line;
		uintptr_t Lowest = 0;
		while (std::getline(Maps, Line))
		{
			std::istringstream Ss(Line);
			std::string Range, Perms, Off, Dev, Inode, Path;
			Ss >> Range >> Perms >> Off >> Dev >> Inode;
			std::getline(Ss, Path);
			const size_t Start = Path.find_first_not_of(' ');
			if (Start == std::string::npos) continue;
			if (Basename(Path.substr(Start)) != ModuleName) continue;
			const uintptr_t From = static_cast<uintptr_t>(strtoull(Range.c_str(), nullptr, 16));
			if (Lowest == 0 || From < Lowest) Lowest = From;
		}
		return Lowest;
	}

	bool ReadBytes(pid_t Pid, uintptr_t Address, void* Out, size_t Size)
	{
		const int Fd = open(("/proc/" + std::to_string(Pid) + "/mem").c_str(), O_RDONLY); // read-only
		if (Fd < 0) return false;
		const ssize_t N = pread(Fd, Out, Size, static_cast<off_t>(Address));
		close(Fd);
		return N == static_cast<ssize_t>(Size);
	}

	std::string FormatValue(pid_t Pid, uintptr_t Address, const std::string& Type, bool& Ok)
	{
		char Buf[64] = {};
		Ok = false;
		if (Type == "int")         { int32_t V;   if ((Ok = ReadBytes(Pid, Address, &V, 4))) std::snprintf(Buf, sizeof(Buf), "%d", V); }
		else if (Type == "uint")   { uint32_t V;  if ((Ok = ReadBytes(Pid, Address, &V, 4))) std::snprintf(Buf, sizeof(Buf), "%u", V); }
		else if (Type == "int64")  { int64_t V;   if ((Ok = ReadBytes(Pid, Address, &V, 8))) std::snprintf(Buf, sizeof(Buf), "%lld", (long long)V); }
		else if (Type == "float")  { float V;     if ((Ok = ReadBytes(Pid, Address, &V, 4))) std::snprintf(Buf, sizeof(Buf), "%f", V); }
		else if (Type == "double") { double V;    if ((Ok = ReadBytes(Pid, Address, &V, 8))) std::snprintf(Buf, sizeof(Buf), "%f", V); }
		else if (Type == "ptr")    { uintptr_t V; if ((Ok = ReadBytes(Pid, Address, &V, 8))) std::snprintf(Buf, sizeof(Buf), "0x%lX", (unsigned long)V); }
		return Buf;
	}

	// Labels ON : every offset is marked with its role (base / pointer step / value).
	// Labels OFF: only the final value is printed.
	bool ReadChain(const State& S)
	{
		uintptr_t Address = S.Base;
		if (S.Labels) std::printf("  base    %s = 0x%lX\n", S.ModuleName.c_str(), (unsigned long)S.Base);
		for (size_t I = 0; I + 1 < S.Offsets.size(); ++I)
		{
			uintptr_t Next = 0;
			if (!ReadBytes(S.Pid, Address + S.Offsets[I], &Next, sizeof(Next)))
			{
				std::printf("  read failed at step %zu (0x%lX)\n", I + 1, (unsigned long)(Address + S.Offsets[I]));
				return false;
			}
			if (S.Labels)
				std::printf("  ptr %zu   [%s + 0x%lX] -> 0x%lX\n", I + 1, I == 0 ? S.ModuleName.c_str() : "ptr", (unsigned long)S.Offsets[I], (unsigned long)Next);
			Address = Next;
		}
		Address += S.Offsets.back();

		bool Ok = false;
		const std::string Value = FormatValue(S.Pid, Address, S.Type, Ok);
		if (!Ok)
		{
			std::printf("  read failed at final address 0x%lX (permissions? see header comment)\n", (unsigned long)Address);
			return false;
		}
		if (S.Labels)
			std::printf("  value   [%s + 0x%lX] (%s) at 0x%lX = %s\n", S.Offsets.size() == 1 ? S.ModuleName.c_str() : "ptr",
			            (unsigned long)S.Offsets.back(), S.Type.c_str(), (unsigned long)Address, Value.c_str());
		else
			std::printf("  %s\n", Value.c_str());
		return true;
	}

	// Terminal without line buffering/echo so single key presses (Insert) arrive immediately.
	struct RawMode
	{
		termios Old{};
		bool Active = false;
		void Enter()
		{
			if (tcgetattr(STDIN_FILENO, &Old) != 0) return;
			termios Raw = Old;
			Raw.c_lflag &= ~static_cast<tcflag_t>(ICANON | ECHO);
			tcsetattr(STDIN_FILENO, TCSANOW, &Raw);
			Active = true;
		}
		~RawMode() { if (Active) tcsetattr(STDIN_FILENO, TCSANOW, &Old); }
	};

	// Live view. Insert opens/closes the menu; in the menu: 1 = labels on/off, q = stop.
	void Watch(State& S)
	{
		struct sigaction Sa{};
		Sa.sa_handler = OnSigInt; // no SA_RESTART: select() returns on Ctrl+C
		sigaction(SIGINT, &Sa, nullptr);
		gStop = 0;

		RawMode Raw;
		Raw.Enter();
		bool MenuOpen = false;

		while (!gStop)
		{
			std::printf("\033[H\033[J");
			std::printf("Watching %s (pid %d)   [Insert] menu   [q] stop\n\n", S.ProcessName.c_str(), S.Pid);
			ReadChain(S);
			if (MenuOpen)
			{
				std::printf("\n+--------- MENU (Insert to close) ---------+\n");
				std::printf("| 1) Offset labels: %-22s |\n", S.Labels ? "ON" : "OFF");
				std::printf("| q) stop watching                         |\n");
				std::printf("+------------------------------------------+\n");
			}
			std::fflush(stdout);

			fd_set Fds;
			FD_ZERO(&Fds);
			FD_SET(STDIN_FILENO, &Fds);
			timeval Timeout{0, 500000};
			if (select(STDIN_FILENO + 1, &Fds, nullptr, nullptr, &Timeout) <= 0) continue; // timeout -> refresh

			char Buf[16];
			const ssize_t N = read(STDIN_FILENO, Buf, sizeof(Buf));
			if (N <= 0) break; // stdin closed
			const std::string Input(Buf, static_cast<size_t>(N));

			if (Input.find("\033[2~") != std::string::npos) // Insert key
				MenuOpen = !MenuOpen;
			else if (Input == "\033")                      // plain Esc
				MenuOpen = false;
			else if (Input[0] != '\033')
				for (char C : Input)
				{
					if (C == 'q') gStop = 1;
					else if (C == '1' && MenuOpen) S.Labels = !S.Labels;
				}
		}
		gStop = 0;
		std::printf("\n");
	}

	std::string Prompt(const char* Text)
	{
		std::printf("%s", Text);
		std::fflush(stdout);
		std::string Line;
		std::getline(std::cin, Line);
		return Line;
	}

	void ShowStatus(const State& S)
	{
		std::printf("\n=== MemoryReader (read-only) ===\n");
		std::printf("process : %s%s\n", S.ProcessName.empty() ? "(none)" : S.ProcessName.c_str(), S.Pid ? "" : "");
		if (S.Pid) std::printf("pid     : %d\n", S.Pid);
		std::printf("module  : %s", S.ModuleName.empty() ? "(none)" : S.ModuleName.c_str());
		if (S.Base) std::printf("  base = 0x%lX", (unsigned long)S.Base);
		std::printf("\ntype    : %s\noffsets :", S.Type.c_str());
		for (uintptr_t O : S.Offsets) std::printf(" 0x%lX", (unsigned long)O);
		std::printf("\nlabels  : %s", S.Labels ? "ON" : "OFF");
		std::printf("\n");
	}
}

int main()
{
	State S;
	for (;;)
	{
		ShowStatus(S);
		std::printf("\n1) select process\n2) select module\n3) set type (int/uint/int64/float/double/ptr)\n"
		            "4) set offsets (hex, space separated; last one is read)\n5) read once\n6) live view (press Insert there for the menu)\n7) offset labels on/off\n0) quit\n");
		const std::string Choice = Prompt("> ");

		if (Choice == "0") break;
		else if (Choice == "1")
		{
			S.ProcessName = Prompt("process name (e.g. cs2): ");
			S.Pid = FindPid(S.ProcessName);
			S.Base = 0;
			std::printf(S.Pid ? "found pid %d\n" : "not found\n", S.Pid);
		}
		else if (Choice == "2")
		{
			if (!S.Pid) { std::puts("select a process first"); continue; }
			S.ModuleName = Prompt("module name (e.g. libclient.so): ");
			S.Base = FindModuleBase(S.Pid, S.ModuleName);
			if (S.Base) std::printf("base = 0x%lX\n", (unsigned long)S.Base);
			else std::puts("module not found in /proc/<pid>/maps");
		}
		else if (Choice == "3")
		{
			const std::string T = Prompt("type: ");
			if (T == "int" || T == "uint" || T == "int64" || T == "float" || T == "double" || T == "ptr") S.Type = T;
			else std::puts("unknown type");
		}
		else if (Choice == "4")
		{
			std::istringstream Ss(Prompt("offsets: "));
			std::vector<uintptr_t> New;
			std::string Tok;
			bool Valid = true;
			while (Ss >> Tok)
			{
				char* End = nullptr;
				const unsigned long long V = strtoull(Tok.c_str(), &End, 0); // 0x.. hex, else decimal
				if (*End) { Valid = false; break; }
				New.push_back(static_cast<uintptr_t>(V));
			}
			if (Valid && !New.empty()) S.Offsets = New;
			else std::puts("invalid offsets, unchanged");
		}
		else if (Choice == "7") S.Labels = !S.Labels;
		else if (Choice == "5" || Choice == "6")
		{
			if (!S.Pid || !S.Base || S.Offsets.empty()) { std::puts("need process, module and offsets first"); continue; }
			if (Choice == "5") ReadChain(S);
			else Watch(S);
		}
	}
	return 0;
}
