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
	};

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

	bool ReadChain(const State& S)
	{
		uintptr_t Address = S.Base;
		for (size_t I = 0; I + 1 < S.Offsets.size(); ++I)
		{
			uintptr_t Next = 0;
			if (!ReadBytes(S.Pid, Address + S.Offsets[I], &Next, sizeof(Next)))
			{
				std::printf("  read failed at step %zu (0x%lX)\n", I, (unsigned long)(Address + S.Offsets[I]));
				return false;
			}
			std::printf("  [0x%lX] -> 0x%lX\n", (unsigned long)(Address + S.Offsets[I]), (unsigned long)Next);
			Address = Next;
		}
		Address += S.Offsets.back();

		bool Ok = false;
		if (S.Type == "int")        { int32_t V;  if ((Ok = ReadBytes(S.Pid, Address, &V, 4))) std::printf("  0x%lX = %d\n", (unsigned long)Address, V); }
		else if (S.Type == "uint")  { uint32_t V; if ((Ok = ReadBytes(S.Pid, Address, &V, 4))) std::printf("  0x%lX = %u\n", (unsigned long)Address, V); }
		else if (S.Type == "int64") { int64_t V;  if ((Ok = ReadBytes(S.Pid, Address, &V, 8))) std::printf("  0x%lX = %lld\n", (unsigned long)Address, (long long)V); }
		else if (S.Type == "float") { float V;    if ((Ok = ReadBytes(S.Pid, Address, &V, 4))) std::printf("  0x%lX = %f\n", (unsigned long)Address, V); }
		else if (S.Type == "double"){ double V;   if ((Ok = ReadBytes(S.Pid, Address, &V, 8))) std::printf("  0x%lX = %f\n", (unsigned long)Address, V); }
		else if (S.Type == "ptr")   { uintptr_t V;if ((Ok = ReadBytes(S.Pid, Address, &V, 8))) std::printf("  0x%lX = 0x%lX\n", (unsigned long)Address, (unsigned long)V); }
		if (!Ok) std::printf("  read failed at final address 0x%lX (permissions? see header comment)\n", (unsigned long)Address);
		return Ok;
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
		            "4) set offsets (hex, space separated; last one is read)\n5) read once\n6) watch (Ctrl+C to stop)\n0) quit\n");
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
		else if (Choice == "5" || Choice == "6")
		{
			if (!S.Pid || !S.Base || S.Offsets.empty()) { std::puts("need process, module and offsets first"); continue; }
			if (Choice == "5") ReadChain(S);
			else for (;;)
			{
				if (!ReadChain(S)) break;
				std::this_thread::sleep_for(std::chrono::milliseconds(500));
			}
		}
	}
	return 0;
}
