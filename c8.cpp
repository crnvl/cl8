#include <iostream>
#include "Chip8.cpp"
#include <string>
#include <Windows.h>

int main()
{
	Chip8 chip8;
	chip8.LoadRom("./roms/pong.ch8");

	auto lastCycleTime = std::chrono::high_resolution_clock::now();

	int frameCount = 0;
	int renderEvery = 60;
	while (true)
	{
		auto currentTime = std::chrono::high_resolution_clock::now();
		float deltaTime = std::chrono::duration<float, std::chrono::milliseconds::period>(currentTime - lastCycleTime).count();
		lastCycleTime = currentTime;

		chip8.Cycle();

		auto vid = chip8.video;
		
		if (frameCount % renderEvery == 0) {
			// clear console 
			system("cls");
			
			// draw screen with upscale 
			int upscale = 1;
			for (int y = 0; y < 32 * upscale; y++) {
				std::string frameLine;
				for (int x = 0; x < 64 * upscale; x++) {
					frameLine += (vid[(y / upscale) * 64 + (x / upscale)] == 0 ? ' ' : (char)254u);
				}
				std::cout << frameLine << std::endl;
			}

			std::cout << std::dec << frameCount << " frames" << std::endl << "Operations: ";
		}
		frameCount++;

		chip8.keypad[0] = (GetKeyState('X') & 0x8000) == 0 ? 0 : 1;
		chip8.keypad[1] = (GetKeyState('1') & 0x8000) == 0 ? 0 : 1;
		chip8.keypad[2] = (GetKeyState('2') & 0x8000) == 0 ? 0 : 1;
		chip8.keypad[3] = (GetKeyState('3') & 0x8000) == 0 ? 0 : 1;
		chip8.keypad[4] = (GetKeyState('Q') & 0x8000) == 0 ? 0 : 1;
		chip8.keypad[5] = (GetKeyState('W') & 0x8000) == 0 ? 0 : 1;
		chip8.keypad[6] = (GetKeyState('E') & 0x8000) == 0 ? 0 : 1;
		chip8.keypad[7] = (GetKeyState('A') & 0x8000) == 0 ? 0 : 1;
		chip8.keypad[8] = (GetKeyState('S') & 0x8000) == 0 ? 0 : 1;
		chip8.keypad[9] = (GetKeyState('D') & 0x8000) == 0 ? 0 : 1;
		chip8.keypad[10] = (GetKeyState('Y') & 0x8000) == 0 ? 0 : 1;
		chip8.keypad[11] = (GetKeyState('C') & 0x8000) == 0 ? 0 : 1;
		chip8.keypad[12] = (GetKeyState('4') & 0x8000) == 0 ? 0 : 1;
		chip8.keypad[13] = (GetKeyState('R') & 0x8000) == 0 ? 0 : 1;
		chip8.keypad[14] = (GetKeyState('F') & 0x8000) == 0 ? 0 : 1;
		chip8.keypad[15] = (GetKeyState('V') & 0x8000) == 0 ? 0 : 1;
	}

	return 0;
}
