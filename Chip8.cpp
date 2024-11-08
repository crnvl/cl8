#include <cstdint>
#include <fstream>
#include <chrono>
#include <random>
#include <iostream>

class Chip8 {
public:
	uint8_t cpu_registers[16]{}; // 16 8-bit registers
	uint8_t memory[4096]{}; // 4KB of memory
	uint16_t index_register{}; // 16-bit index register
	uint16_t program_counter{}; // 16-bit program counter
	uint16_t stack[16]{}; // 16 16-bit stack
	uint8_t stack_pointer{}; // 8-bit stack pointer
	uint8_t delay_timer{}; // 8-bit delay timer
	uint8_t sound_timer{}; // 8-bit sound timer
	uint8_t keypad[16]{}; // 16-key hexadecimal keypad
	uint32_t video[64 * 32]{}; // 64x32 monochrome display
	uint16_t opcode{}; // 16-bit opcode

	std::default_random_engine randGen;
	std::uniform_int_distribution<unsigned int> randByte;

	typedef void (Chip8::* Chip8Func)();
	Chip8Func table[0xF + 1];
	Chip8Func table0[0xE + 1];
	Chip8Func table8[0xE + 1];
	Chip8Func tableE[0xE + 1];
	Chip8Func tableF[0x65 + 1];

	const unsigned int START_ADDRESS = 0x200; // Start address of the program (See reference)

	const unsigned int FONTSET_SIZE = 80; // Size of the fontset (See reference)
	const unsigned int FONTSET_START_ADDRESS = 0x50; // Start address of the fontset (See reference)

	uint8_t fontset[80] = {
		// Fontset (See reference)
		0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
		0x20, 0x60, 0x20, 0x20, 0x70, // 1
		0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
		0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
		0x90, 0x90, 0xF0, 0x10, 0x10, // 4
		0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
		0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
		0xF0, 0x10, 0x20, 0x40, 0x40, // 7
		0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
		0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
		0xF0, 0x90, 0xF0, 0x90, 0x90, // A
		0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
		0xF0, 0x80, 0x80, 0x80, 0xF0, // C
		0xE0, 0x90, 0x90, 0x90, 0xE0, // D
		0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
		0xF0, 0x80, 0xF0, 0x80, 0x80  // F
	};

	Chip8()
		: randGen(std::chrono::system_clock::now().time_since_epoch().count())
	{
		program_counter = START_ADDRESS; // Set the program counter to the start address

		// Load the fontset into memory
		for (unsigned int i = 0; i < FONTSET_SIZE; ++i) {
			memory[FONTSET_START_ADDRESS + i] = fontset[i];
		}

		// Initialize the random number generator
		randByte = std::uniform_int_distribution<unsigned int>(0, 255u);

		// Set up function pointer table
		table[0x0] = &Chip8::Table0;
		table[0x1] = &Chip8::OP_1nnn;
		table[0x2] = &Chip8::OP_2nnn;
		table[0x3] = &Chip8::OP_3xkk;
		table[0x4] = &Chip8::OP_4xkk;
		table[0x5] = &Chip8::OP_5xy0;
		table[0x6] = &Chip8::OP_6xkk;
		table[0x7] = &Chip8::OP_7xkk;
		table[0x8] = &Chip8::Table8;
		table[0x9] = &Chip8::OP_9xy0;
		table[0xA] = &Chip8::OP_Annn;
		table[0xB] = &Chip8::OP_Bnnn;
		table[0xC] = &Chip8::OP_Cxkk;
		table[0xD] = &Chip8::OP_Dxyn;
		table[0xE] = &Chip8::TableE;
		table[0xF] = &Chip8::TableF;

		for (size_t i = 0; i <= 0xE; i++) {
			table0[i] = &Chip8::OP_NULL;
			table8[i] = &Chip8::OP_NULL;
			tableE[i] = &Chip8::OP_NULL;
		}

		table0[0x0] = &Chip8::OP_00E0;
		table0[0xE] = &Chip8::OP_00EE;

		table8[0x0] = &Chip8::OP_8xy0;
		table8[0x1] = &Chip8::OP_8xy1;
		table8[0x2] = &Chip8::OP_8xy2;
		table8[0x3] = &Chip8::OP_8xy3;
		table8[0x4] = &Chip8::OP_8xy4;
		table8[0x5] = &Chip8::OP_8xy5;
		table8[0x6] = &Chip8::OP_8xy6;
		table8[0x7] = &Chip8::OP_8xy7;
		table8[0xE] = &Chip8::OP_8xyE;

		tableE[0x1] = &Chip8::OP_ExA1;
		tableE[0xE] = &Chip8::OP_Ex9E;

		for (size_t i = 0; i <= 0x65; i++)
		{
			tableF[i] = &Chip8::OP_NULL;
		}

		tableF[0x07] = &Chip8::OP_Fx07;
		tableF[0x0A] = &Chip8::OP_Fx0A;
		tableF[0x15] = &Chip8::OP_Fx15;
		tableF[0x18] = &Chip8::OP_Fx18;
		tableF[0x1E] = &Chip8::OP_Fx1E;
		tableF[0x29] = &Chip8::OP_Fx29;
		tableF[0x33] = &Chip8::OP_Fx33;
		tableF[0x55] = &Chip8::OP_Fx55;
		tableF[0x65] = &Chip8::OP_Fx65;
	}

	// CPU Cycle
	void Cycle() {
		// Fetch
		opcode = (memory[program_counter] << 8u) | memory[program_counter + 1];

		// Increment the program counter before we execute anything
		program_counter += 2;

		// Decode and Execute
		std::cout << "0x" << std::hex << opcode << " ";
		((*this).*(table[(opcode & 0xF000u) >> 12u]))();

		// Decrement the delay timer if it's been set
		if (delay_timer > 0) {
			--delay_timer;
		}

		// Decrement the sound timer if it's been set
		if (sound_timer > 0) {
			--sound_timer;
		}
	}

	// Function pointer table
	void Table0() {
		((*this).*(table0[opcode & 0x000Fu]))();
	}

	void Table8() {
		((*this).*(table8[opcode & 0x000Fu]))();
	}

	void TableE() {
		((*this).*(tableE[opcode & 0x000Fu]))();
	}

	void TableF() {
		((*this).*(tableF[opcode & 0x00FFu]))();
	}

	void OP_NULL() {
		// invalid opcode
		std::cout << "Invalid opcode: " << opcode << std::endl;
	}

	// Instructions
	// CLS: Clear the display
	void OP_00E0() {
		memset(video, 0, sizeof(video));
	}

	// RET: Return from a subroutine
	void OP_00EE() {
		--stack_pointer;
		program_counter = stack[stack_pointer];
	}

	// JP addr: Jump to address
	void OP_1nnn() {
		uint16_t address = opcode & 0x0FFFu;
		program_counter = address;
	}

	// CALL addr: Call subroutine at address
	void OP_2nnn() {
		uint16_t address = opcode & 0x0FFFu;
		stack[stack_pointer] = program_counter;
		++stack_pointer;
		program_counter = address;
	}

	// SE Vx, byte: Skip next instruction if Vx == byte
	void OP_3xkk() {
		uint8_t Vx = (opcode & 0x0F00u) >> 8u;
		uint8_t byte = opcode & 0x00FFu;

		if (cpu_registers[Vx] == byte) {
			program_counter += 2;
		}
	}

	// SNE Vx, byte: Skip next instruction if Vx != byte
	void OP_4xkk() {
		uint8_t Vx = (opcode & 0x0F00u) >> 8u;
		uint8_t byte = opcode & 0x00FFu;

		if (cpu_registers[Vx] != byte) {
			program_counter += 2;
		}
	}

	// SE Vx, Vy: Skip next instruction if Vx == Vy
	void OP_5xy0() {
		uint8_t Vx = (opcode & 0x0F00u) >> 8u;
		uint8_t Vy = (opcode & 0x00F0u) >> 4u;

		if (cpu_registers[Vx] == cpu_registers[Vy]) {
			program_counter += 2;
		}
	}

	// LD Vx, byte: Set Vx = byte
	void OP_6xkk() {
		uint8_t Vx = (opcode & 0x0F00u) >> 8u;
		uint8_t byte = opcode & 0x00FFu;

		cpu_registers[Vx] = byte;
	}

	// ADD Vx, byte: Set Vx = Vx + byte
	void OP_7xkk() {
		uint8_t Vx = (opcode & 0x0F00u) >> 8u;
		uint8_t byte = opcode & 0x00FFu;

		cpu_registers[Vx] += byte;
	}

	// LD Vx, Vy: Set Vx = Vy
	void OP_8xy0() {
		uint8_t Vx = (opcode & 0x0F00u) >> 8u;
		uint8_t Vy = (opcode & 0x00F0u) >> 4u;

		cpu_registers[Vx] = cpu_registers[Vy];
	}

	// OR Vx, Vy: Set Vx = Vx OR Vy
	void OP_8xy1() {
		uint8_t Vx = (opcode & 0x0F00u) >> 8u;
		uint8_t Vy = (opcode & 0x00F0u) >> 4u;

		cpu_registers[Vx] |= cpu_registers[Vy];
	}

	// AND Vx, Vy: Set Vx = Vx AND Vy
	void OP_8xy2() {
		uint8_t Vx = (opcode & 0x0F00u) >> 8u;
		uint8_t Vy = (opcode & 0x00F0u) >> 4u;

		cpu_registers[Vx] &= cpu_registers[Vy];
	}

	// XOR Vx, Vy: Set Vx = Vx XOR Vy
	void OP_8xy3() {
		uint8_t Vx = (opcode & 0x0F00u) >> 8u;
		uint8_t Vy = (opcode & 0x00F0u) >> 4u;

		cpu_registers[Vx] ^= cpu_registers[Vy];
	}

	// ADD Vx, Vy: Set Vx = Vx + Vy, set VF = carry
	void OP_8xy4() {
		uint8_t Vx = (opcode & 0x0F00u) >> 8u;
		uint8_t Vy = (opcode & 0x00F0u) >> 4u;

		uint16_t sum = cpu_registers[Vx] + cpu_registers[Vy];

		if (sum > 255U) {
			cpu_registers[0xF] = 1;
		}
		else {
			cpu_registers[0xF] = 0;
		}

		cpu_registers[Vx] = sum & 0xFFu;
	}

	// SUB Vx, Vy: Set Vx = Vx - Vy, set VF = NOT borrow
	void OP_8xy5() {
		uint8_t Vx = (opcode & 0x0F00u) >> 8u;
		uint8_t Vy = (opcode & 0x00F0u) >> 4u;

		if (cpu_registers[Vx] > cpu_registers[Vy]) {
			cpu_registers[0xF] = 1;
		}
		else {
			cpu_registers[0xF] = 0;
		}

		cpu_registers[Vx] -= cpu_registers[Vy];
	}

	// SHR Vx {, Vy}: Set Vx = Vx SHR 1
	void OP_8xy6() {
		uint8_t Vx = (opcode & 0x0F00u) >> 8u;

		cpu_registers[0xF] = cpu_registers[Vx] & 0x1u;
		cpu_registers[Vx] >>= 1;
	}

	// SUBN Vx, Vy: Set Vx = Vy - Vx, set VF = NOT borrow
	void OP_8xy7() {
		uint8_t Vx = (opcode & 0x0F00u) >> 8u;
		uint8_t Vy = (opcode & 0x00F0u) >> 4u;

		if (cpu_registers[Vy] > cpu_registers[Vx]) {
			cpu_registers[0xF] = 1;
		}
		else {
			cpu_registers[0xF] = 0;
		}

		cpu_registers[Vx] = cpu_registers[Vy] - cpu_registers[Vx];
	}

	// SHL Vx {, Vy}: Set Vx = Vx SHL 1
	void OP_8xyE() {
		uint8_t Vx = (opcode & 0x0F00u) >> 8u;

		cpu_registers[0xF] = (cpu_registers[Vx] & 0x80u) >> 7u;
		cpu_registers[Vx] <<= 1;
	}

	// SNE Vx, Vy: Skip next instruction if Vx != Vy
	void OP_9xy0() {
		uint8_t Vx = (opcode & 0x0F00u) >> 8u;
		uint8_t Vy = (opcode & 0x00F0u) >> 4u;

		if (cpu_registers[Vx] != cpu_registers[Vy]) {
			program_counter += 2;
		}
	}

	// LD I, addr: Set I = addr
	void OP_Annn() {
		uint16_t address = opcode & 0x0FFFu;
		index_register = address;
	}

	// JP V0, addr: Jump to location V0 + addr
	void OP_Bnnn() {
		uint16_t address = opcode & 0x0FFFu;
		program_counter = cpu_registers[0] + address;
	}

	// RND Vx, byte: Set Vx = random byte AND byte
	void OP_Cxkk() {
		uint8_t Vx = (opcode & 0x0F00u) >> 8u;
		uint8_t byte = opcode & 0x00FFu;

		cpu_registers[Vx] = randByte(randGen) & byte;
	}

	// DRW Vx, Vy, nibble: Display n-byte sprite starting at memory location I at (Vx, Vy), set VF = collision
	void OP_Dxyn() {
		uint8_t Vx = (opcode & 0x0F00u) >> 8u;
		uint8_t Vy = (opcode & 0x00F0u) >> 4u;
		uint8_t height = opcode & 0x000Fu;

		// Wrap if going beyond screen boundaries
		uint8_t xPos = cpu_registers[Vx] % 64;
		uint8_t yPos = cpu_registers[Vy] % 32;

		cpu_registers[0xF] = 0;

		for (unsigned int row = 0; row < height; ++row) {
			uint8_t spriteByte = memory[index_register + row];

			for (unsigned int col = 0; col < 8; ++col) {
				uint8_t spritePixel = spriteByte & (0x80u >> col);
				uint32_t* screenPixel = &video[(yPos + row) * 64 + (xPos + col)];

				// Sprite pixel is on
				if (spritePixel) {
					// Screen pixel also on - collision
					if (*screenPixel == 0xFFFFFFFF) {
						cpu_registers[0xF] = 1;
					}

					// Effectively XOR with the sprite pixel
					*screenPixel ^= 0xFFFFFFFF;
				}
			}
		}

	}

	// SKP Vx: Skip next instruction if key with the value of Vx is pressed
	void OP_Ex9E() {
		uint8_t Vx = (opcode & 0x0F00u) >> 8u;
		uint8_t key = cpu_registers[Vx];

		if (keypad[key]) {
			program_counter += 2;
		}
	}

	// SKNP Vx: Skip next instruction if key with the value of Vx is not pressed
	void OP_ExA1() {
		uint8_t Vx = (opcode & 0x0F00u) >> 8u;
		uint8_t key = cpu_registers[Vx];

		if (!keypad[key]) {
			program_counter += 2;
		}
	}

	// LD Vx, DT: Set Vx = delay timer value
	void OP_Fx07() {
		uint8_t Vx = (opcode & 0x0F00u) >> 8u;
		cpu_registers[Vx] = delay_timer;
	}

	// LD Vx, K: Wait for a key press, store the value of the key in Vx
	void OP_Fx0A() {
		uint8_t Vx = (opcode & 0x0F00u) >> 8u;

		for (uint8_t i = 0; i < 16; ++i) {
			if (keypad[i]) {
				cpu_registers[Vx] = i;
				return;
			}
		}

		// Repeat the instruction if no key was pressed
		program_counter -= 2;
	}

	// LD DT, Vx: Set delay timer = Vx
	void OP_Fx15() {
		uint8_t Vx = (opcode & 0x0F00u) >> 8u;
		delay_timer = cpu_registers[Vx];
	}

	// LD ST, Vx: Set sound timer = Vx
	void OP_Fx18() {
		uint8_t Vx = (opcode & 0x0F00u) >> 8u;
		sound_timer = cpu_registers[Vx];
	}

	// ADD I, Vx: Set I = I + Vx
	void OP_Fx1E() {
		uint8_t Vx = (opcode & 0x0F00u) >> 8u;
		index_register += cpu_registers[Vx];
	}

	// LD F, Vx: Set I = location of sprite for digit Vx
	void OP_Fx29() {
		uint8_t Vx = (opcode & 0x0F00u) >> 8u;
		uint8_t digit = cpu_registers[Vx];

		index_register = FONTSET_START_ADDRESS + (5 * digit);
	}

	// LD B, Vx: Store BCD representation of Vx in memory locations I, I+1, and I+2
	void OP_Fx33() {
		uint8_t Vx = (opcode & 0x0F00u) >> 8u;
		uint8_t value = cpu_registers[Vx];

		// Ones-place
		memory[index_register + 2] = value % 10;
		value /= 10;

		// Tens-place
		memory[index_register + 1] = value % 10;
		value /= 10;

		// Hundreds-place
		memory[index_register] = value % 10;
	}

	// LD [I], Vx: Store registers V0 through Vx in memory starting at location I
	void OP_Fx55() {
		uint8_t Vx = (opcode & 0x0F00u) >> 8u;

		for (uint8_t i = 0; i <= Vx; ++i) {
			memory[index_register + i] = cpu_registers[i];
		}
	}

	// LD Vx, [I]: Read registers V0 through Vx from memory starting at location I
	void OP_Fx65() {
		uint8_t Vx = (opcode & 0x0F00u) >> 8u;

		for (uint8_t i = 0; i <= Vx; ++i) {
			cpu_registers[i] = memory[index_register + i];
		}
	}

	void LoadRom(char const* filename) {
		std::ifstream file(filename, std::ios::binary | std::ios::ate); // Open file in binary mode and seek to the end

		if (file.is_open()) {
			std::streampos size = file.tellg(); // Get the size of the file
			char* buffer = new char[size]; // Allocate a buffer to hold the file's contents

			file.seekg(0, std::ios::beg); // Seek back to the beginning of the file
			file.read(buffer, size); // Read the file into the buffer
			file.close(); // Close the file

			for (long i = 0; i < size; ++i) {
				memory[START_ADDRESS + i] = buffer[i]; // Load the program into memory starting at the start address
			}

			delete[] buffer; // Free the buffer
		}
	}
};
