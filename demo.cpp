#pragma comment(lib,"xinput.lib")
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Xinput.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include "JoystickTools.h"

HANDLE hConsole;
CONSOLE_SCREEN_BUFFER_INFO screenInfo;

void drawDpad(SHORT x, SHORT y, char * title, WORD rawButtons, WORD processedButtons) {
	SetConsoleCursorPosition(hConsole, { x,y });
	printf("+%s+",title);
	SetConsoleCursorPosition(hConsole, { x,y+1});
	printf("|  %s  |", (rawButtons& XINPUT_GAMEPAD_DPAD_UP)?"^":" ");
	SetConsoleCursorPosition(hConsole, { x,y+2 });
	printf("|  %s  |", (processedButtons & XINPUT_GAMEPAD_DPAD_UP) ? "^" : " ");
	SetConsoleCursorPosition(hConsole, { x,y+3 });
	printf("|%s", (rawButtons & XINPUT_GAMEPAD_DPAD_LEFT) ? "<" : " ");
	printf("%s ", (processedButtons & XINPUT_GAMEPAD_DPAD_LEFT) ? "<" : " ");
	printf("%s", (processedButtons & XINPUT_GAMEPAD_DPAD_RIGHT) ? ">" : " ");
	printf("%s|", (rawButtons & XINPUT_GAMEPAD_DPAD_RIGHT) ? ">" : " ");
	SetConsoleCursorPosition(hConsole, { x,y+4 });
	printf("|  %s  |", (processedButtons & XINPUT_GAMEPAD_DPAD_DOWN) ? "v" : " ");
	SetConsoleCursorPosition(hConsole, { x,y+5});
	printf("|  %s  |", (rawButtons& XINPUT_GAMEPAD_DPAD_DOWN) ? "v" : " ");
	SetConsoleCursorPosition(hConsole, { x,y+6});
	printf("+-----+");
}

void printCoords(SHORT x, SHORT y, char * message, SHORT tX, SHORT tY) {
	SetConsoleCursorPosition(hConsole, { x,y });
	printf("%s:\t%6hd,%6hd", message, tX, tY);
}

int main(int argc, char **argv) {
	DWORD result;
	CONSOLE_CURSOR_INFO oldCursor;
	CONSOLE_CURSOR_INFO newCursor = { 100,0 };

	XInputEnable(true);

	// Get a handle to the STDOUT screen buffer to copy from and 
	// create a new screen buffer to copy to. 
	hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

	GetConsoleCursorInfo(hConsole, &oldCursor);
	SetConsoleCursorInfo(hConsole, &newCursor);

	GetConsoleScreenBufferInfo(hConsole, &screenInfo);
	FillConsoleOutputCharacter(hConsole, ' ', screenInfo.dwSize.X*screenInfo.dwSize.Y, { 0,0 }, &result);
	
	bool running = true;
	XINPUT_STATE state;
	ZeroMemory(&state, sizeof(XINPUT_STATE));
	do{
		XInputGetState(0, &state);

		drawDpad(1, 1, "D-Pad", state.Gamepad.wButtons,0);
		drawDpad(3, 8, "LStik", dpadFromAnalog(state.Gamepad.sThumbLX,state.Gamepad.sThumbLY), 0);
		drawDpad(2, 16,"RStik", 0, 0);
		Sleep(16);

		printCoords(15, 15, "ThumbL", state.Gamepad.sThumbLX, state.Gamepad.sThumbLY);
		printCoords(15, 16, "ThumbR", state.Gamepad.sThumbRX, state.Gamepad.sThumbRY);
		if (_kbhit()) {
			switch (_getch()) {
			case 27:
				running = false;
			default:
				break;
			}
		}
	} while (running);
	SetConsoleCursorInfo(hConsole, &oldCursor);
	return 0;
}