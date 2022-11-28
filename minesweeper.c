#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

// virtual terminal sequences
#include <wchar.h>
#include <windows.h>

// Useful functions for manipulating cursor
#define ESC "\x1b"
void hide_cursor(void) { printf(ESC"[?25l"); }
void show_cursor(void) { printf(ESC"[?25h"); }
void save_cursor(void) { printf(ESC"[s"); }
void load_cursor(void) { printf(ESC"[u"); }
void down_n_lines(int n) { printf(ESC"[%iE", n); }
void up_n_lines(int n) { printf(ESC"[%iF", n); }
void right_n_chars(int n) { printf(ESC"[%iC", n); }
void left_n_chars(int n) { printf(ESC"[%iD", n); }
void cursor_to_start(void) { printf(ESC"[0G"); } // Puts cursor to start of current line
void clear_n_lines(int n) { printf(ESC"[%iM", n); } // Clear n lines below cursor
// Useful functions for changing terminal appearance
void set_default_colors(void) { printf(ESC"[0m"); }
void set_foreground_color(int r, int g, int b) { printf(ESC"[38;2;%i;%i;%im", r, g, b); }
void set_background_color(int r, int g, int b) { printf(ESC"[48;2;%i;%i;%im", r, g, b); }
// Print the same C-string n times
void print_n_times(const char* cstr, int n) { for (int i = 0; i < n; ++i) printf("%s", cstr); }

// Returns any error that might occur
int enable_virtual_terminal_sequences(void) {
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hOut == INVALID_HANDLE_VALUE)
		return GetLastError();

	DWORD dwMode = 0;
	if (!GetConsoleMode(hOut, &dwMode))
		return GetLastError();

	dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	if (!SetConsoleMode(hOut, dwMode))
		return GetLastError();

	return 0;
}

// Hardcode keybinds
#define KEYBOARD_INTERRUPT '\3'
#define KEYBOARD_ENTER '\r'
#define KEYBIND_UP 'w'
#define KEYBIND_DOWN 's'
#define KEYBIND_LEFT 'a'
#define KEYBIND_RIGHT 'd'
#define KEYBIND_OPEN ' '
#define KEYBIND_FLAG 'f'

// Default arguments
#define DEFAULT_WIDTH 20
#define DEFAULT_HEIGHT 15
#define DEFAULT_MINE_COUNT 15
#define DEFAULT_USE_MINE_PERCENTAGE true

// Colors
#define TO_RGB(hex) (((hex) >> 16) & 255), ((hex) >> 8 & 255), ((hex) >> 0 & 255)
#define COLOR_CLOSED 0x909090
#define COLOR_FLAG 0xF23607
#define COLOR_MINE 0xFF0000
#define COLOR_1 0x1B77D1
#define COLOR_2 0x388E3C
#define COLOR_3 0xFF6702
#define COLOR_4 0x7B1FA2
#define COLOR_5 0xFF8F00
#define COLOR_6 0x0097A7
#define COLOR_7 0x525252
#define COLOR_8 0xA29B93

#define COLOR_N(n) COLOR_NUMS[n - 1]
const int COLOR_NUMS[] = {
	COLOR_1,
	COLOR_2,
	COLOR_3,
	COLOR_4,
	COLOR_5,
	COLOR_6,
	COLOR_7,
	COLOR_8
};

#define THROW(...) { fprintf(stderr, __VA_ARGS__); return EXIT_FAILURE; }
#define THROW_VAL(err, ...) { fprintf(stderr, __VA_ARGS__); return err; }

typedef enum {
	PLAYING,
	WIN,
	LOSS
} Game_State;

typedef enum {
	CLOSED,
	OPEN,
	FLAGGED
} State;

typedef struct {
	bool is_mine;
	int neighbors;
	State state;
} Cell;

typedef struct {
	Cell* cells;
	int w, h, mine_count;
} Field;

// Sets mine_count to percentage of size rounded down
// Percentage is clamped withing [0, 100]
void set_mine_count_percent(Field* field, int percent) {
	if (percent < 0) percent = 0;
	else if (percent > 100) percent = 100;
	field->mine_count = field->w * field->h * percent / 100;
}

// Returns wether pos is within the field
bool in_field(Field* field, int x, int y) {
	return 0 <= x && 0 <= y && x < field->w&& y < field->h;
}

// Get cell at pos !! Not position safe !!
Cell* get_cell(Field* field, int x, int y) {
	return &field->cells[x + y * field->w];
}

// Set cell at pos !! Not position safe !!
void set_cell(Field* field, int x, int y, bool is_mine, State state) {
	Cell* cell = get_cell(field, x, y);
	cell->is_mine = is_mine;
	cell->state = state;
}

// Return pointer to random cell 
Cell* random_cell(Field* field) {
	return &field->cells[rand() % (field->w * field->h)];
}

// Gets random_cell and sets it to mine
// Returns previous mine state
bool set_random_cell_to_mine(Field* field) {
	Cell* cell = random_cell(field);
	bool was_mine = cell->is_mine;
	cell->is_mine = true;
	return was_mine;
}

// Toggle flag state if not open
// If given, updates flag_count accordingly
void flag_cell(Field* field, int x, int y, int* flag_count) {
	Cell* cell = get_cell(field, x, y);
	switch (cell->state) {
		case CLOSED:
			cell->state = FLAGGED;
			if (flag_count) ++(*flag_count);
			break;
		case FLAGGED:
			cell->state = CLOSED;
			if (flag_count) --(*flag_count);
			break;
		default:
			break;
	}
}

// Returns the number of neighboring mines
// Outside bounds are not counted as mines
int mine_neighbor_count(Field* field, int x, int y) {
	int count = 0;
	for (int dy = -1; dy <= 1; ++dy)
		for (int dx = -1; dx <= 1; ++dx)
			if ((dx != 0 || dy != 0) && in_field(field, x + dx, y + dy)
				&& get_cell(field, x + dx, y + dy)->is_mine)
				++count;

	return count;
}

// Returns the number of neighboring flags
// Outside bounds are not counted as flagged
int flagged_neighbor_count(Field* field, int x, int y) {
	int count = 0;
	for (int dy = -1; dy <= 1; ++dy)
		for (int dx = -1; dx <= 1; ++dx)
			if ((dx != 0 || dy != 0) && in_field(field, x + dx, y + dy)
				&& get_cell(field, x + dx, y + dy)->state == FLAGGED)
				++count;

	return count;
}

// Set closed cell to open
// If zero, open neighbors automatically
// Returns true if player hit a mine, false otherwise
bool open_closed_cell(Field*, int, int);

// Set closed cell to open
// If zero, open neighbors automatically
// If open and enough neighbors are flagged, open the other neighbors automatically
// Returns wether player hit a mine
bool open_cell(Field* field, int x, int y) {
	Cell* cell = get_cell(field, x, y);
	if (cell->state == FLAGGED) return false;
	if (cell->is_mine) return true;

	bool open_neighbors = cell->neighbors == 0 || (cell->state == OPEN && cell->neighbors == flagged_neighbor_count(field, x, y));
	cell->state = OPEN;

	bool hit_mine = false;
	if (open_neighbors) // Recursively open neighboring cells
		for (int dy = -1; dy <= 1; ++dy)
			for (int dx = -1; dx <= 1; ++dx)
				hit_mine |= (dx != 0 || dy != 0) && in_field(field, x + dx, y + dy)
				&& open_closed_cell(field, x + dx, y + dy);

	if (hit_mine) // Open cells regardless
		for (int dy = -1; dy <= 1; ++dy)
			for (int dx = -1; dx <= 1; ++dx)
				get_cell(field, x + dx, y + dy)->state = OPEN;

	return hit_mine;
}

bool open_closed_cell(Field* field, int x, int y) {
	if (get_cell(field, x, y)->state == CLOSED)
		return open_cell(field, x, y);
	return false;
}

// Set all cells according to mine_neighbor_count
void calculate_neighbors(Field* field) {
	for (int y = 0; y < field->h; ++y)
		for (int x = 0; x < field->w; ++x)
			get_cell(field, x, y)->neighbors = mine_neighbor_count(field, x, y);
}

// Returns number of cells of state in field
int state_count(Field* field, State state) {
	int count = 0;
	for (int y = 0; y < field->h; ++y)
		for (int x = 0; x < field->w; ++x)
			if (get_cell(field, x, y)->state == state)
				++count;

	return count;
}

// Set all cells to closed non-mines
void clear_field(Field* field) {
	for (int y = 0; y < field->h; ++y)
		for (int x = 0; x < field->w; ++x)
			set_cell(field, x, y, false, CLOSED);
}

// Set all cells mine-status to is_mine
void fill_mine_field(Field* field, bool is_mine) {
	for (int y = 0; y < field->h; ++y)
		for (int x = 0; x < field->w; ++x)
			get_cell(field, x, y)->is_mine = is_mine;
}

// Set all cells to open
void open_field(Field* field) {
	for (int y = 0; y < field->h; ++y)
		for (int x = 0; x < field->w; ++x)
			get_cell(field, x, y)->state = OPEN;
}

// Set all mines to open
// Unset all flags
void open_mines(Field* field) {
	Cell* cell;
	for (int y = 0; y < field->h; ++y)
		for (int x = 0; x < field->w; ++x) {
			cell = get_cell(field, x, y);
			if (cell->is_mine) cell->state = OPEN;
			else if (cell->state == FLAGGED) cell->state = CLOSED;
		}
}

// Initialize field and return pointer
Field* create_field(void) {
	Field* field = malloc(sizeof(Field));
	if (field) {
		field->w = -1;
		field->h = -1;
		field->mine_count = -1;
		field->cells = NULL;
	}

	return field;
}

// Frees and reallocates according to size
// Returns 1 if malloc fails, 0 otherwise
int resize_field(Field* field) {
	if (field->cells) free(field->cells);
	field->cells = malloc(field->w * field->h * sizeof(Cell));
	return field->cells == NULL;
}

// Sets size, frees and reallocates according to new size
// Returns 1 if malloc fails, 0 otherwise
int resize_field_to(Field* field, int w, int h) {
	field->w = w;
	field->h = h;
	return resize_field(field);
}

// Frees cells and field
void delete_field(Field* field) {
	if (field == NULL) return;
	if (field->cells) free(field->cells);
	free(field);
}

// Clears and randomizes new mine positions
// Calculates neighbors
void generate_field(Field* field) {
	fill_mine_field(field, false);

	int mines_left = min(field->mine_count, field->w * field->h);
	while (mines_left-- > 0) while (set_random_cell_to_mine(field));

	calculate_neighbors(field);
}

// Clears and randomizes new mine positions
// Calculates neighbors
// If possible cell at pos will be non-mine
// If possible cells neighboring pos will all be non-mine
void generate_field_with_zero(Field* field, int x, int y) {
	// Fills entire board without randomizing if no non-mines exists 
	if (field->mine_count >= field->w * field->h) {
		fill_mine_field(field, true);
		calculate_neighbors(field);
		return;
	}

	fill_mine_field(field, false);

	Cell* cell;
	int rand_x, rand_y, mines_left = field->mine_count;
	while (mines_left > 0) {
		// Get random cell with coordinates
		rand_x = rand() % field->w;
		rand_y = rand() % field->h;
		cell = get_cell(field, rand_x, rand_y);

		if (!cell->is_mine // Not already mine
			&& ((field->mine_count - mines_left >= field->w * field->h - 9 // Forced to put mine beside x, y
				&& (x != rand_x || y != rand_y)) // Don't set x, y to mine
				|| (abs(x - rand_x) > 1 || abs(y - rand_y) > 1)) // Default to keep x, y a 0
			) {
			// Set mine
			cell->is_mine = true;
			--mines_left;
		}
	};

	calculate_neighbors(field);
}

// Prints field
// Does not overwrite with blank spaces
// Returns cursor to saved position
void print_field(Field* field) {
	int foreground_color = -1;
	for (int y = 0; y < field->h; ++y) {
		for (int x = 0; x < field->w; ++x) {
			right_n_chars(1);

			Cell* cell = get_cell(field, x, y);
			switch (cell->state) {
				case CLOSED:
					// set_default_colors();
					if (foreground_color != COLOR_CLOSED) {
						foreground_color = COLOR_CLOSED;
						set_foreground_color(TO_RGB(foreground_color));
					}
					printf(".");
					break;

				case FLAGGED:
					if (foreground_color != COLOR_FLAG) {
						foreground_color = COLOR_FLAG;
						set_foreground_color(TO_RGB(foreground_color));
					}
					printf("F");
					break;

				case OPEN:
					if (cell->is_mine) {
						if (foreground_color != COLOR_MINE) {
							foreground_color = COLOR_MINE;
							set_foreground_color(TO_RGB(foreground_color));
						}
						printf("*");
					} else if (cell->neighbors == 0) {
						printf(" ");
					} else {
						if (foreground_color != COLOR_N(cell->neighbors)) {
							foreground_color = COLOR_N(cell->neighbors);
							set_foreground_color(TO_RGB(foreground_color));
						}
						printf("%c", cell->neighbors + '0');
					}

					break;
			}
		}
		down_n_lines(1);
	}
	set_default_colors();
	load_cursor();
}

// Prints C-string at pos
// Does not overwrite anything else
// Returns cursor to saved position
void print_at(int x, int y, const char* cstr, ...) {
	if (y > 0) down_n_lines(y);
	if (x > 0) right_n_chars(x);

	va_list argptr;
	va_start(argptr, cstr);
	vprintf(cstr, argptr);
	va_end(argptr);

	// printf("%s", cstr);
	load_cursor();
}

// Prints cursor around position
// Does not overwrite anything else
// Returns cursor to original position
void print_cursor(int x, int y) {
	print_at(x * 2, y, "[ ]");
}

// Overwrites cursor with spaces
// Does not overwrite anything else
// Returns cursor to original position
void unprint_cursor(int x, int y) {
	print_at(x * 2, y, "   ");
}

// Reads argc arguments from argv and assigns to field accordingly
// Accepted arguments are hardcoded
// Returns 0 if successful, 1 otherwise
int get_args(int argc, char** argv, Field* field) {
	bool use_percent = DEFAULT_USE_MINE_PERCENTAGE;

	field->w = DEFAULT_WIDTH;
	field->h = DEFAULT_HEIGHT;
	field->mine_count = DEFAULT_MINE_COUNT;

	// Skip path argument
	for (int i = 1; i < argc; ++i) {
		char* arg = argv[i];

		// Check for flags
		if (strcmp(arg, "--help") == 0) {
			printf(
				"  w=<n>          Sets field width to n\n"
				"  h=<n>          Sets field height to n\n"
				"  dim=<n>        Sets field width and height to n\n"
				"  dim=<n>,<m>    Sets field width to n and height to m\n"
				"\n"
				"  mc=<n>         Sets field mine count to n\n"
				"  mp=<n>         Sets field mine count to n percent of cell count\n"
				"\n"
				"  --help         Display this information\n"
				"  --keybinds     List all keybinds\n"
			);

			exit(EXIT_SUCCESS);

		} else if (strcmp(arg, "--version") == 0) {
			printf(
				"Minesweeper 1.0.2\n"
				"Find the latest update at: https://github.com/TheArchSeal/Minesweeper\n"
			);

			exit(EXIT_SUCCESS);

		} else if (strcmp(arg, "--keybinds") == 0) {
			printf(
				"  w           Move cursor up\n"
				"  s           Move cursor down\n"
				"  a           Move cursor left\n"
				"  d           Move cursor right\n"
				"\n"
				"  f           Flag / Unflag cell\n"
				"  Space       Open cell\n"
				"  Enter       Exit game after playing\n"
				"  Ctrl + c    Exit game\n"
				"\n"
				"  <any>       Update timer and redraw\n"
			);

			exit(EXIT_SUCCESS);
		}


		// Find argument
		char* eq_pos = strchr(arg, '=');
		if (eq_pos) {
			*(eq_pos++) = '\0';
			int val = atoi(eq_pos);

			// Check for arguments
			if (strcmp(arg, "w") == 0) field->w = val;
			else if (strcmp(arg, "h") == 0) field->h = val;
			else if (strcmp(arg, "mc") == 0) {
				use_percent = false;
				field->mine_count = val;
			} else if (strcmp(arg, "mp") == 0) {
				use_percent = true;
				field->mine_count = val;
			} else if (strcmp(arg, "dim") == 0) {
				char* seperator_pos = strchr(eq_pos + 1, ',');

				// Separate val1 from val2
				if (seperator_pos) {
					*(seperator_pos++) = '\0';
					int val1 = atoi(eq_pos), val2 = atoi(seperator_pos);

					field->w = val1;
					field->h = val2;

					// Seperator not found, set both to same
				} else field->w = field->h = val;

				// Argument does not mach anything
			} else THROW("Argument %i: '%s' not recognized\n", i, arg);
		} else THROW("Flag %i: '%s' not recognized\n", i, arg);
	}

	// Clamp bounds
	field->w = max(field->w, 1);
	field->h = max(field->h, 1);

	if (use_percent) set_mine_count_percent(field, field->mine_count);
	field->mine_count = max(field->mine_count, 0);
	field->mine_count = min(field->mine_count, field->w * field->h);

	return EXIT_SUCCESS;
}

// Play a simple game of minesweeper
int main(int argc, char** argv) {
	time_t start_time;
	srand(time(&start_time)); // Set random seed

	// Initialize everything
	Field* field = create_field();
	if (field == NULL) THROW("Unable to allocate field\n");

	int err;
	if ((err = get_args(argc, argv, field))) return err;
	if ((err = enable_virtual_terminal_sequences()))
		THROW_VAL(err, "Unable to enable virtual terminal sequences\n");

	if (resize_field(field)) THROW("Unable to allocate cells\n"); // If unable to malloc, quit
	clear_field(field);

	int cursor_x = (field->w + 1) / 2 - 1;
	int cursor_y = (field->h + 1) / 2 - 1;
	int flag_count = 0;
	bool started = false;
	Game_State game_state = PLAYING;

	// Disables auto-flushing
	// Allows buffer_size chars to be printed at once
	size_t buffer_size = max(BUFSIZ, (field->w * field->h + 8) * 32);
	char* buffer = malloc(buffer_size);
	if (buffer == NULL) THROW("Unable to allocate print buffer\n"); // If unable to malloc, quit
	setvbuf(stdout, buffer, _IOFBF, buffer_size);

	hide_cursor();
	save_cursor();
	fflush(stdout);

	// Game loop
	char c = '\0';
	do {
		// Swich on all keybinds
		switch (c) {
			case KEYBIND_UP:
				if (cursor_y > 0) cursor_y--;
				break;
			case KEYBIND_DOWN:
				if (cursor_y < field->h - 1) ++cursor_y;
				break;
			case KEYBIND_LEFT:
				if (cursor_x > 0) cursor_x--;
				break;
			case KEYBIND_RIGHT:
				if (cursor_x < field->w - 1) ++cursor_x;
				break;

			case KEYBIND_OPEN:
				if (!started) {
					generate_field_with_zero(field, cursor_x, cursor_y);
					started = true;
				}

				if (open_cell(field, cursor_x, cursor_y))
					game_state = LOSS; // Lose when opening mine
				else if (state_count(field, OPEN) == field->w * field->h - field->mine_count)
					game_state = WIN; // Win when all non-mines are opened

				break;
			case KEYBIND_FLAG:
				flag_cell(field, cursor_x, cursor_y, &flag_count);
				break;
		}

		// Print game-state
		clear_n_lines(field->h);
		print_cursor(cursor_x, cursor_y);
		print_field(field);
		print_at((field->w + 1) * 2, 0, "Mines left: %i", field->mine_count - flag_count);
		print_at((field->w + 1) * 2, 1, "Time taken: %llus", time(started ? NULL : &start_time) - start_time);
		fflush(stdout);

		// Get new input and quit at keyboard interrupt (CTRL+c)
	} while (game_state == PLAYING && (c = getch()) != KEYBOARD_INTERRUPT);

	// Determining game result
	time_t end_time = time(NULL);
	if (game_state != PLAYING) {
		if (game_state == LOSS)
			open_mines(field);

		// Show final field
		unprint_cursor(cursor_x, cursor_y);
		print_field(field);
		print_at((field->w + 1) * 2, 3, game_state == WIN ? "Game Won!" : "Game Lost!");
		fflush(stdout);

		// Wait for user to continue
		while ((c = getch()) != KEYBOARD_INTERRUPT && c != KEYBOARD_ENTER);
	}

	// Return terminal to original state
	load_cursor();
	clear_n_lines(field->h);

	// Print game result
	if (game_state != PLAYING)
		printf("%s a game of minesweeper in %llu seconds\n", game_state == WIN ? "Won" : "Lost", end_time - start_time);

	show_cursor();
	fflush(stdout);

	// Clear up memory
	free(buffer);
	delete_field(field);

	return EXIT_SUCCESS;
}
