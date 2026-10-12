// Practice target for memreader: a tiny "game" you can safely read.
// Build: g++ -std=c++17 practice_target.cpp -o practice_target
//
// It prints the offsets you need, so you can verify your chain works:
//   gPlayer is a global pointer -> struct Player { int Health; float X; }
#include <unistd.h>

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>

struct Player
{
	int Pad = 0;
	int Health = 100;
	float X = 12.5f;
};

Player* gPlayer = nullptr;

static unsigned long ExeBase()
{
	std::ifstream Maps("/proc/self/maps");
	std::string Line;
	std::getline(Maps, Line); // first mapping of the executable = lowest address
	return strtoul(Line.c_str(), nullptr, 16);
}

int main()
{
	setvbuf(stdout, nullptr, _IOLBF, 0);
	gPlayer = new Player();
	const unsigned long Base = ExeBase();
	std::printf("process: practice_target   module: practice_target\n");
	std::printf("offsets for memreader (type int):  0x%lX 0x%zX\n", (unsigned long)(reinterpret_cast<unsigned long>(&gPlayer) - Base), offsetof(Player, Health));
	std::printf("offsets for memreader (type float): 0x%lX 0x%zX\n", (unsigned long)(reinterpret_cast<unsigned long>(&gPlayer) - Base), offsetof(Player, X));
	std::printf("Health drops by 1 every second. Ctrl+C to quit.\n");
	for (;;)
	{
		sleep(1);
		--gPlayer->Health;
		gPlayer->X += 0.5f;
	}
}
