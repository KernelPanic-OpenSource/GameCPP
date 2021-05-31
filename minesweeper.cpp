/*
	ANSI terminal Minesweeper game.
	Written by Lê Duy Quang (leduyquang753@gmail.com).
	Command line arguments: <width> <height> <mines>.
	If those are not specified, defaults to a 9.9 board with 10 mines
	(beginner difficulty).
	Controls:
		Arrow keys to move.
		Enter to open the cell under the caret.
		' (apostrophe) to mark the cell as has a bomb (!) or suspicious (?).
		Ctrl + C to force quit.
	Requires a terminal with ANSI escape code support to display properly.
	Have fun.
*/

#include <conio.h>

#include <iostream>
#include <queue>
#include <random>
#include <string>
#include <vector>

using namespace std::string_literals;
	
const std::string
// If you want to decipher this ANSI code madness, you can refer to
// https://en.wikipedia.org/wiki/ANSI_escape_code
	escape = "\033["s,
	restoreCursor = "\033[u"s,
	backCursor = "\033[1D"s,
	nextCursor = "\033[1C"s,
	mineDisplay = "\033[38;2;255;0;0mX\033[0m"s,
	displayStrings[] {
		"\033[48;2;64;64;64m.\033[0m"s,
		"\033[7m\033[38;2;255;255;0m!\033[0m"s,
		"\033[7m\033[38;2;224;152;203m?\033[0m"s
	},
	numberStrings[] {
		"\033[38;2;100;100;100m0\033[0m"s,
		"\033[38;2;149;253;141m1\033[0m"s,
		"\033[38;2;193;253;141m2\033[0m"s,
		"\033[38;2;253;252;141m3\033[0m"s,
		"\033[38;2;253;211;141m4\033[0m"s,
		"\033[38;2;253;165;141m5\033[0m"s,
		"\033[38;2;253;141;176m6\033[0m"s,
		"\033[38;2;253;141;220m7\033[0m"s,
		"\033[38;2;237;141;253m8\033[0m"s
	};

void generateBoard(
	std::vector<char> &board, const int &size, const int &mines
) {
	std::vector<int> shuffler;
	shuffler.reserve(size);
	for (int i = 0; i < size; i++) shuffler.push_back(i);
	std::default_random_engine randomEngine((std::random_device())());
	int cap = size - mines - 1;
	for (int last = size-1; last > cap; last--) {
		std::uniform_int_distribution<int> distribution(0, last);
		int index = distribution(randomEngine);
		board[shuffler[index]] = 1;
		shuffler[index] = shuffler[last];
	}
}

char getMines(
	std::vector<char> &board,
	const int &width, const int &height,
	const int &row, const int &col
) {
	int rb = row-1, re = row+2, cb = col-1, ce = col+2, count = 0;
	for (int r = rb; r < re; r++)
		if (r != -1 && r != height)
			for (int c = cb; c < ce; c++)
				if (c != -1 && c != width && board[r*width + c])
					count++;
	return count;
}

std::string moveCursor(int &cursorX, int &cursorY, int x, int y) {
	std::string s;
	s += escape;
	if (y > cursorY) {
		s += std::to_string(y - cursorY);
		s += 'B';
	} else if (y < cursorY) {
		s += std::to_string(cursorY - y);
		s += 'A';
	}
	s += escape;
	s += std::to_string(x << 1 | 1);
	s += 'G';
	cursorX = x;
	cursorY = y;
	return s;
}

struct Coords {
	int row, col;
	Coords(int row, int col): row{row}, col{col} {}
};

void floodFill(
	std::vector<char> &board, std::vector<char> &display,
	const int &width, const int &height,
	const int &row, const int &col,
	int &opened,
	std::string &updateString
) {
	int cursorX = col, cursorY = row;
	std::queue<Coords> queue;
	queue.emplace(row, col);
	while (!queue.empty()) {
		Coords &coords = queue.front();
		int rb = coords.row-1, re = coords.row+2,
		    cb = coords.col-1, ce = coords.col+2;
		for (int r = rb; r < re; r++)
			if (r != -1 && r != height)
				for (int c = cb; c < ce; c++)
					if (
						c != -1 && c != height &&
						display[r*width + c] == 0
					) {
						int mines = getMines(
							board, width, height, r, c
						);
						display[r*width + c] = mines + 3;
						updateString += moveCursor(cursorX, cursorY, c, r);
						updateString += numberStrings[mines];
						updateString += backCursor;
						opened++;
						if (mines == 0) queue.emplace(r, c);
					}
		queue.pop();
	}
	updateString += moveCursor(cursorX, cursorY, col, row);
}

void updateStatusBar(
	const int &opened, const int &winCount,
	const int &flagged, const int &mines,
	const int &height,
	const int &cursorX, const int &cursorY,
	std::string &updateString
) {
	updateString += restoreCursor;
	updateString += "\033[1K\033[38;2;138;244;119m"s;
	updateString += std::to_string(opened);
	updateString += " / "s;
	updateString += std::to_string(winCount);
	updateString += "    \033[38;2;244;204;119m"s;
	updateString += std::to_string(flagged);
	updateString += " / "s;
	updateString += std::to_string(mines);
	updateString += "\033[0m\033["s;
	updateString += std::to_string(height - cursorY);
	updateString += "A\033["s;
	updateString += std::to_string(cursorX << 1 | 1);
	updateString += 'G';
}

void revealCell(
	std::vector<char> &board, std::vector<char> &display,
	const int &index, std::string &updateString
) {
	char mine = board[index], state = display[index];
	updateString +=
		state == 1
			? mine
				? "\033[7m\033[38;2;149;253;141m!\033[0m"s
				: "\033[7m\033[38;2;156;156;156m!\033[0m"s
			: mine
				? mineDisplay
				: nextCursor;
}

int main(int argc, char **argv) {
	int width = 9, height = 9, mines = 10, size = width*height;
	if (argc > 3) try {
		width = std::stoi(argv[1]);
		height = std::stoi(argv[2]);
		mines = std::stoi(argv[3]);
		size = width * height;
		if (width < 2 || height < 2 || mines < 1 || mines >= size) {
			std::cerr << "Invalid arguments.\n"s;
			return 1;
		}
	} catch (const std::exception &e) {
		std::cerr << "Invalid arguments.\n"s;
		return 1;
	}
	/*
		Board codes:
		0: No flag.
		1: Flag.
		2: Question.
		3 ÷ 11: Opened, number of mines = code – 3.
	*/
	std::vector<char> display(size, 0);
	std::vector<char> board(size, 0);
	generateBoard(board, size, mines);
	int cursorX = 0, cursorY = 0, opened = 0, winCount = size-mines, flagged = 0;
	
	std::string updateString;
	for (int row = 0; row < height; row++) {
		updateString += displayStrings[0];
		for (int col = 1; col < height; col++) {
			updateString += ' ';
			updateString += displayStrings[0];
		}
		updateString += '\n';
	}
	updateString += "\033[s\033[1G\033["s;
	updateString += std::to_string(height);
	updateString += 'A';
	updateStatusBar(
		opened, winCount, flagged, mines,
		height, cursorX, cursorY, updateString
	);
	std::cout << updateString;

	bool arrow = false, firstMove = true;
	while (true) {
		updateString.clear();
		if (arrow) {
			switch (_getch()) {
				case 72: { // Up.
					if (cursorY == 0) {
						cursorY = height-1;
						updateString += escape;
						updateString += std::to_string(cursorY);
						updateString += 'B';
					} else {
						cursorY--;
						updateString += "\033[1A"s;
					}
					break;
				}
				case 80: { // Down.
					if (cursorY == height-1) {
						cursorY = 0;
						updateString += escape;
						updateString += std::to_string(height-1);
						updateString += 'A';
					} else {
						cursorY++;
						updateString += "\033[1B"s;
					}
					break;
				}
				case 75: { // Left.
					cursorX = (cursorX + width - 1) % width;
					updateString += escape;
					updateString += std::to_string(cursorX << 1 | 1);
					updateString += 'G';
					break;
				}
				case 77: { // Right.
					cursorX = (cursorX + 1) % width;
					updateString += escape;
					updateString += std::to_string(cursorX << 1 | 1);
					updateString += 'G';
					break;
				}
			}
			arrow = false;
		} else switch (_getch()) {
			case 224: {
				arrow = true;
				break;
			}
			case 39: { // Toggle flag/suspicious.
				char &cell = display[cursorY*width + cursorX];
				if (cell < 3) {
					cell = (cell + 1) % 3;
					updateString += displayStrings[cell];
					updateString += backCursor;
					switch (cell) {
						case 1:
							flagged++;
							break;
						case 2:
							flagged--;
							break;
					}
					updateStatusBar(
						opened, winCount, flagged, mines,
						height, cursorX, cursorY, updateString
					);
				}
				break;
			}
			case 13: { // Open.
				int index = cursorY*width + cursorX;
				char &cell = display[index];
				if (cell == 0) {
					if (firstMove) { // First move is always safe.
						if (board[index]) {
							int pos = 0;
							while (board[pos]) pos++;
							board[pos] = 1;
							board[index] = 0;
						}
						firstMove = false;
					}
					if (board[index]) { // It's a mine. Boom and game over.
						updateString += escape;
						if (cursorY != 0) {
							updateString += std::to_string(cursorY);
							updateString += 'A';
						}
						updateString += "\033[1G"s;
						for (int row = 0; row < height; row++) {
							revealCell(board, display, row*width, updateString);
							for (int col = 1; col < height; col++) {
								updateString += nextCursor;
								revealCell(
									board, display, row*width+col, updateString
								);
							}
							updateString += "\033[1B\033[1G"s;
						}
						updateString += escape;
						updateString += std::to_string(height - cursorY);
						updateString += 'A';
						updateString += escape;
						updateString += std::to_string(cursorX << 1 | 1);
						updateString += "G\033[7m";
						updateString += mineDisplay;
						updateString += restoreCursor;
						updateString += "You detonated a bomb. "
						                "Better luck next time.\n"s;
						std::cout << updateString;
						return 0;
					} else { // Safe.
						int minesHere = getMines(
							board, width, height, cursorY, cursorX
						);
						cell = 3 + minesHere;
						updateString += numberStrings[minesHere];
						updateString += backCursor;
						opened++;
						if (minesHere == 0) floodFill(
							board, display,
							width, height, cursorY, cursorX,
							opened, updateString
						);
						if (opened == winCount) {
							updateString += restoreCursor;
							updateString += "\033[0K"
							                "You win. All the safe cells "
							                "have been opened.\n"s;
							std::cout << updateString;
							return 0;
						} else {
							updateStatusBar(
								opened, winCount, flagged, mines,
								height, cursorX, cursorY, updateString
							);
						}
					}
				}
				break;
			}
			case 3: {
				updateString += restoreCursor;
				updateString += '\n';
				std::cout << updateString;
				return 0;
				break;
			}
		}
		std::cout << updateString;
	}
}
