/*
*	Land of Limbs
* 
* A puzzle/adventure/RPG game where you build a modular character from LIMBs.
*
*/

/*
* NEXT:
* - refactor drawing panels, so that the UI object draws the panel (takes a panel as a parameter) (really? I'm not sure... have to think about this)
* - make Screen module
*	- enum of screen types
*	- 
* 
* DESGIN PATTERS TO USE:
*	- Singleton (for GameState and UI)
*	- Factory (for Screens, Panels, and Buttons) (nope, turned out to be a bad idea) (but maybe later for characters, etc)
*	- Observer (allow resizing of screens and propagating the size down to Panels and Buttons)
* 
*/


#include "SDL.h"
#include "SDL_image.h"
#include <stdio.h>
#include <string>
#include <iostream>
#include "SDL_ttf.h"
#include <cmath>
#include <vector>
#include <cstdlib>
#include <time.h>

using namespace std;

//import GameState;
import UI;
import GameState;
import Screen;
import ScreenType;

const bool DEBUG = false;

using std::string;
using std::cout;
using std::cin;
using std::vector;
using std::to_string;


// GLOBAL CONSTANTS - many of these will be stored in the UI module

void draw(Panel& panel);
void exit(SDL_Surface* surface, SDL_Window* window);


int main(int argc, char* args[]) {
	cout << "Hello new world";

	Panel menuPanel = UI::getInstance().createMainMenuPanel();

	// This loop will be in the menu screen

	// Timeout data
	const int TARGET_FPS = 60;
	const int FRAME_DELAY = 600 / TARGET_FPS; // milliseconds per frame
	Uint32 frameStartTime; // Tick count when this particular frame began
	int frameTimeElapsed; // how much time has elapsed during this frame

	bool running = true;
	SDL_Event e;

	// Game loop
	while (running)
	{
		// Get the total running time (tick count) at the beginning of the frame, for the frame timeout at the end
		frameStartTime = SDL_GetTicks();

		// Check for events in queue, and handle them (really just checking for X close now
		while (SDL_PollEvent(&e) != 0)
		{
			// User pressed X to close
			if (e.type == SDL_QUIT)
			{
				running = false;
			}

			// check event for mouse or keyboard action
		}

		draw(menuPanel);

		// Delay so the app doesn't just crash
		frameTimeElapsed = SDL_GetTicks() - frameStartTime; // Calculate how long the frame took to process
		// Delay loop
		if (frameTimeElapsed < FRAME_DELAY) {
			SDL_Delay(FRAME_DELAY - frameTimeElapsed);
		}
	}

	exit(UI::getInstance().getWindowSurface(), UI::getInstance().getMainWindow());
	return 0;
}

// This will go in the Menu Screen loop
void draw(Panel& panel) {
	// draw panel ( make this a function of the UI object which takes a panel as a parameter )

	SDL_SetRenderDrawColor(UI::getInstance().getMainRenderer(), 145, 145, 154, 1);
	SDL_RenderClear(UI::getInstance().getMainRenderer());

	SDL_SetRenderDrawColor(UI::getInstance().getMainRenderer(), 95, 77, 227, 1);

	vector<Button> buttons = panel.getButtons();

	for (int i = 0; i < buttons.size(); ++i) {
		// get the rect, send it a reference (to be converted to a pointer)
		SDL_Rect rect = buttons[i].getRect();
		SDL_Rect textRect = buttons[i].getTextRect();
		SDL_RenderFillRect(UI::getInstance().getMainRenderer(), &rect);

		// now draw the text
		SDL_Surface* buttonTextSurface = TTF_RenderText_Blended(UI::getInstance().getButtonFont(), buttons[i].getText().c_str(), UI::getInstance().getTextColor());
		SDL_Texture* buttonTextTexture = SDL_CreateTextureFromSurface(UI::getInstance().getMainRenderer(), buttonTextSurface);
		SDL_FreeSurface(buttonTextSurface);
		SDL_RenderCopyEx(UI::getInstance().getMainRenderer(), buttonTextTexture, NULL, &textRect, 0, NULL, SDL_FLIP_NONE);
	}

	// Update window
	SDL_RenderPresent(UI::getInstance().getMainRenderer());
}


void exit(SDL_Surface* surface, SDL_Window* window)
{
	SDL_FreeSurface(surface);
	surface = NULL;

	SDL_DestroyWindow(window);
	window = NULL;

	SDL_Quit();
}
