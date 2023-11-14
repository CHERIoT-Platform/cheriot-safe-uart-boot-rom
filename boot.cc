#define CHERIOT_NO_AMBIENT_MALLOC
#define CHERIOT_NO_NEW_DELETE

#include <cheri.hh>
#include <platform-uart.hh>

using namespace CHERI;

namespace
{
	/**
	 * Write a string to the UART.
	 */
	void puts(volatile Uart *uart, const char *msg, bool newline = true)
	{
		while (char c = (*msg++))
		{
			uart->blocking_write(c);
		}
		if (newline)
		{
			uart->blocking_write('\n');
		}
	}

	/**
	 * Convert a hex character to a 4-bit value.
	 */
	uint8_t hex_to_nybble(char c)
	{
		switch (c)
		{
			case '0' ... '9':
				return c - '0';
			case 'a' ... 'f':
				return c - 'a' + 10;
			case 'A' ... 'F':
				return c - 'A' + 10;
		}
		return 0;
	}

	/**
	 * Convert a pair of hex characters to a byte.
	 */
	uint8_t hex_to_byte(char c0, char c1)
	{
		uint8_t byte = hex_to_nybble(c0) << 4;
		byte += hex_to_nybble(c1);
		return byte;
	}

	/**
	 * Print a word to the UART.
	 */
	void print_word(volatile Uart *uart, uint32_t word, bool newline = true)
	{
		// Buffer to hold the output.
		char buffer[9];
		// Convert a 4-bit value to a hex digit.
		auto nybble_to_digit = [](int nybble) {
			if (nybble < 10)
			{
				return nybble + '0';
			}
			else
			{
				return nybble - 10 + 'A';
			}
		};
		// Convert each byte to a pair of hex digits
		for (int i = 0; i < 4; i++)
		{
			int digit           = (word >> ((3 - i) * 8)) & 0xff;
			buffer[(i * 2)]     = nybble_to_digit(digit >> 4);
			buffer[(i * 2) + 1] = nybble_to_digit(digit & 0xf);
		}
		// Null terminator
		buffer[8] = 0;
		// Print the generated string.
		puts(uart, buffer, newline);
	}

	/**
	 * Load firmware from the specified UART, storing it via the capability in
	 * iram.
	 */
	uint32_t load_firmware(volatile Uart *uart, uint32_t *iram)
	{
		auto read_byte = [&]() {
			return hex_to_byte(uart->blocking_read(), uart->blocking_read());
		};
		uint32_t words = 0;
		puts(uart, "Ready to load firmware");
		uint32_t word;
		while (true)
		{
			char c = uart->blocking_read();
			if (c == '\n')
			{
				puts(uart, "\nFinished loading.  Last word was: ", false);
				print_word(uart, word);
				puts(uart, "Number of words loaded to IRAM: ", false);
				print_word(uart, words);
				return Capability{iram}.address();
			}
			// Read a little-endian word
			word = hex_to_byte(c, uart->blocking_read()) << 24;
			word += read_byte() << 16;
			word += read_byte() << 8;
			word += read_byte();
			// Store the word in memory
			*(iram++) = word;
			// Read the newline.
			c = uart->blocking_read();
			// Print the first word that we read for some sanity checking.
			if (words == 0)
			{
				puts(uart, "Starting loading.  First word was: ", false);
				print_word(uart, word);
			}
			// Print a . for every KiB (256 4-byte words)
			if ((words & 0xff) == 0)
			{
				uart->blocking_write('.');
			}
			words++;
			// If this wasn't a newline, give up.  We'll try again in the outer
			// loop.
			if (c != '\n')
			{
				puts(uart,
				     "Character that was not a newline read after the end of a "
				     "word.  Giving up.");
				return 0;
			}
		}
	}
} // namespace

/**
 * C++ entry point for the loader.  This is called from assembly, with the
 * read-write root in the first argument.
 */
extern "C" uint32_t rom_loader_entry(void *rwRoot)
{
	Capability<void> root{rwRoot};
	// Create a bounded capability to the UART
	Capability<volatile Uart> uart = root.cast<volatile Uart>();
	uart.address()                 = 0x8f00b000;
	uart.bounds()                  = 0x100;
	// Initialise the UART.
	uart->init();
	// Create a bounded capability to IRAM
	Capability<uint32_t> iram = root.cast<uint32_t>();
	iram.address()            = 0x20040000;
	// The actual bounds are 0x40000, but carve off 1 KiB for our stack.  We
	// don't actually use anything like that much, but a typical firmware image
	// isn't going to be 255 KiB either.
	iram.bounds()             = 0x3fc00;
	uint32_t end;
	// Make messages from the loader red
	puts(uart, "\033[31;1m");
	// Try loading until we have successfully loaded some data and not see an
	// error condition.
	while ((end = load_firmware(uart, iram)) == 0) {}
	// Return the end location.  The assembly code will zero the memory, which
	// includes our stack.
	puts(uart, "Loaded firmware, jumping to IRAM.");
	// Reset the colour
	puts(uart, "\033[0m");
	return end;
}
