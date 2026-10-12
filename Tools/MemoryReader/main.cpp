// MemoryReader - small READ-ONLY learning tool for understanding Base + Offset.
//
// It finds a running process, finds the base address of one of its modules and
// follows a pointer chain of offsets to print one value. It never writes to
// another process's memory.
//
// Usage:
//   memreader <process.exe> <module.dll> <type> <offset> [offset ...] [--watch <ms>]
//
// type:    int | uint | int64 | float | double | ptr
// offsets: hex (0x...) or decimal. All offsets but the last are followed as
//          pointers, the last one is added to the final address and read.
//
// Example (offsets are placeholders, find real ones yourself):
//   memreader game.exe game.dll int 0x1A2B30 0x340
//     -> addr = base + 0x1A2B30; addr = *(uint64*)addr; value = *(int*)(addr + 0x340)
//
// Only use this on your own programs or on games started offline/without
// anti-cheat (for CS2: private server with bots and the -insecure launch option).

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <tlhelp32.h>

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

namespace
{
	bool EqualsNoCase(const wchar_t* A, const wchar_t* B)
	{
		return _wcsicmp(A, B) == 0;
	}

	DWORD FindProcessId(const std::wstring& Name)
	{
		HANDLE Snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (Snap == INVALID_HANDLE_VALUE)
		{
			return 0;
		}
		PROCESSENTRY32W Entry{};
		Entry.dwSize = sizeof(Entry);
		DWORD Pid = 0;
		if (Process32FirstW(Snap, &Entry))
		{
			do
			{
				if (EqualsNoCase(Entry.szExeFile, Name.c_str()))
				{
					Pid = Entry.th32ProcessID;
					break;
				}
			} while (Process32NextW(Snap, &Entry));
		}
		CloseHandle(Snap);
		return Pid;
	}

	uintptr_t FindModuleBase(DWORD Pid, const std::wstring& Name)
	{
		HANDLE Snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, Pid);
		if (Snap == INVALID_HANDLE_VALUE)
		{
			return 0;
		}
		MODULEENTRY32W Entry{};
		Entry.dwSize = sizeof(Entry);
		uintptr_t Base = 0;
		if (Module32FirstW(Snap, &Entry))
		{
			do
			{
				if (EqualsNoCase(Entry.szModule, Name.c_str()))
				{
					Base = reinterpret_cast<uintptr_t>(Entry.modBaseAddr);
					break;
				}
			} while (Module32NextW(Snap, &Entry));
		}
		CloseHandle(Snap);
		return Base;
	}

	bool ReadBytes(HANDLE Process, uintptr_t Address, void* Out, size_t Size)
	{
		SIZE_T Read = 0;
		return ReadProcessMemory(Process, reinterpret_cast<LPCVOID>(Address), Out, Size, &Read) && Read == Size;
	}

	bool ParseOffset(const wchar_t* Text, uintptr_t& Out)
	{
		wchar_t* End = nullptr;
		const unsigned long long Value = wcstoull(Text, &End, 0); // base 0: "0x.." hex, else decimal
		if (End == Text || *End != L'\0')
		{
			return false;
		}
		Out = static_cast<uintptr_t>(Value);
		return true;
	}

	void Usage()
	{
		std::puts("Usage: memreader <process.exe> <module.dll> <type> <offset> [offset ...] [--watch <ms>]");
		std::puts("type: int | uint | int64 | float | double | ptr");
		std::puts("All offsets except the last are followed as pointers; the last is read as <type>.");
	}

	// Follows the chain and prints the final value. Returns false on any read error.
	bool ReadChain(HANDLE Process, uintptr_t Base, const std::vector<uintptr_t>& Offsets, const std::wstring& Type)
	{
		uintptr_t Address = Base;
		for (size_t I = 0; I + 1 < Offsets.size(); ++I)
		{
			uintptr_t Next = 0;
			if (!ReadBytes(Process, Address + Offsets[I], &Next, sizeof(Next)))
			{
				std::printf("read failed at step %zu (address 0x%llX)\n", I, static_cast<unsigned long long>(Address + Offsets[I]));
				return false;
			}
			Address = Next;
		}
		Address += Offsets.back();

		union
		{
			int32_t I32;
			uint32_t U32;
			int64_t I64;
			uintptr_t Ptr;
			float F;
			double D;
		} V{};
		V.I64 = 0;

		bool Ok = false;
		if (Type == L"int")         { Ok = ReadBytes(Process, Address, &V.I32, 4); if (Ok) std::printf("0x%llX = %d\n", (unsigned long long)Address, V.I32); }
		else if (Type == L"uint")   { Ok = ReadBytes(Process, Address, &V.U32, 4); if (Ok) std::printf("0x%llX = %u\n", (unsigned long long)Address, V.U32); }
		else if (Type == L"int64")  { Ok = ReadBytes(Process, Address, &V.I64, 8); if (Ok) std::printf("0x%llX = %lld\n", (unsigned long long)Address, (long long)V.I64); }
		else if (Type == L"float")  { Ok = ReadBytes(Process, Address, &V.F, 4);   if (Ok) std::printf("0x%llX = %f\n", (unsigned long long)Address, V.F); }
		else if (Type == L"double") { Ok = ReadBytes(Process, Address, &V.D, 8);   if (Ok) std::printf("0x%llX = %f\n", (unsigned long long)Address, V.D); }
		else if (Type == L"ptr")    { Ok = ReadBytes(Process, Address, &V.Ptr, sizeof(V.Ptr)); if (Ok) std::printf("0x%llX = 0x%llX\n", (unsigned long long)Address, (unsigned long long)V.Ptr); }
		else
		{
			std::puts("unknown type");
			return false;
		}
		if (!Ok)
		{
			std::printf("read failed at final address 0x%llX\n", static_cast<unsigned long long>(Address));
		}
		return Ok;
	}
}

int wmain(int Argc, wchar_t** Argv)
{
	if (Argc < 5)
	{
		Usage();
		return 1;
	}

	const std::wstring ProcessName = Argv[1];
	const std::wstring ModuleName = Argv[2];
	const std::wstring Type = Argv[3];
	std::vector<uintptr_t> Offsets;
	int WatchMs = 0;

	for (int I = 4; I < Argc; ++I)
	{
		if (wcscmp(Argv[I], L"--watch") == 0 && I + 1 < Argc)
		{
			WatchMs = _wtoi(Argv[++I]);
			continue;
		}
		uintptr_t Offset = 0;
		if (!ParseOffset(Argv[I], Offset))
		{
			std::wprintf(L"invalid offset: %ls\n", Argv[I]);
			return 1;
		}
		Offsets.push_back(Offset);
	}
	if (Offsets.empty())
	{
		Usage();
		return 1;
	}

	const DWORD Pid = FindProcessId(ProcessName);
	if (Pid == 0)
	{
		std::wprintf(L"process not found: %ls\n", ProcessName.c_str());
		return 2;
	}

	const uintptr_t Base = FindModuleBase(Pid, ModuleName);
	if (Base == 0)
	{
		std::wprintf(L"module not found: %ls (try running as the same user / administrator)\n", ModuleName.c_str());
		return 3;
	}

	// Read-only access: no PROCESS_VM_WRITE / PROCESS_VM_OPERATION requested.
	HANDLE Process = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, Pid);
	if (!Process)
	{
		std::printf("OpenProcess failed (error %lu)\n", GetLastError());
		return 4;
	}

	std::wprintf(L"pid %lu, %ls base = 0x%llX\n", Pid, ModuleName.c_str(), static_cast<unsigned long long>(Base));

	int Result = 0;
	if (WatchMs > 0)
	{
		for (;;)
		{
			ReadChain(Process, Base, Offsets, Type);
			std::this_thread::sleep_for(std::chrono::milliseconds(WatchMs));
		}
	}
	else if (!ReadChain(Process, Base, Offsets, Type))
	{
		Result = 5;
	}

	CloseHandle(Process);
	return Result;
}
