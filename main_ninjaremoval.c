#include <stdio.h>
#include <stddef.h>

int main(int argc, char **argv)
{

	FILE* fr;
	if (argc < 2) {
		fr = stdin;
	}
	else if (fr = fopen(argv[1], "r"), fr == NULL) {
		fprintf(stderr, "Error reading file %s", argv[1]);
		return 2;
	}

	// Reading map name header
	int ch;
	do {
		ch = fgetc(fr);
		putchar(ch);
	} while (ch != '#');

	// reading 8 bytes of rubish + tiles data
	for (size_t i = 0; i < 8U + 1426U; i++) {
		ch = fgetc(fr);
		putchar(ch);
	}

	// Ninja object data: "0010" number of ninjas equals one
	putchar('0');
	putchar('0');
	putchar('1');
	putchar('0');

	unsigned long long ninja_pwr[] = { 256, 65536, 1, 16 };
	unsigned ninjas = 0;

	// Read ninja object count
	for (size_t i = 0; i < 4U; i++) {
		ch = fgetc(fr);

		int ch_value;
		if (isdigit(ch))
			ch_value = ch - '0';
		else {
			ch_value = 10 + ch - 'a';
		}
			 
		ninjas += ninja_pwr[i] * ch_value;
	}

	// Accept first ninja object placement data
	for (size_t i = 0; i < 4U; i++) {
		ch = fgetc(fr);
		putchar(ch);
	}

	ninjas -= 1;
	ninjas *= 4;

	// Disregard all other ninjas placement data
	while (ninjas--) {
		(void)fgetc(fr);
	}

	// Read all other object data
	while ((ch = fgetc(fr)) != EOF) {
		putchar(ch);
	}

	fclose(fr);
}