/*
*	Land of Limbs
* 
* A puzzle/adventure/RPG game where you build a modular character from LIMBs.
*
*/

/*
* NEXT:
* - refactor drawing panels, so that the UI object draws the panel (takes a panel as a parameter)
* - buttonTextTexture should be saved in the button object
* - make Screen module
*	- enum of screen types
*	- 
* 
* 
* 
* DESGIN PATTERS TO USE:
*	- Singleton (for GameState)
*	- Factory (for Screens, Panels, and Buttons)
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

const bool DEBUG = false;

using std::string;
using std::cout;
using std::cin;
using std::vector;
using std::to_string;


// GLOBAL CONSTANTS - many of these will be stored in the UI module

/*
* There will be different sub-map screens.
* These will inherit from the map class... or they will just be internal enums.
* This will be stored in the GameState module.
*/
enum class ScreenType {
	Menu, Map, Battle, CharacterCreation
};

// This will be stored in the Map module, with a value held in each
enum class MapType {
	World, Building, Dungeon
};

void draw(UI& ui, Panel& panel);
void exit(SDL_Surface* surface, SDL_Window* window);


int main(int argc, char* args[]) {
	cout << "Hello new world";

	UI ui = UI();

	Panel menuPanel = ui.createMainMenuPanel();

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

		draw(ui, menuPanel);

		// Delay so the app doesn't just crash
		frameTimeElapsed = SDL_GetTicks() - frameStartTime; // Calculate how long the frame took to process
		// Delay loop
		if (frameTimeElapsed < FRAME_DELAY) {
			SDL_Delay(FRAME_DELAY - frameTimeElapsed);
		}
	}

	exit(ui.getWindowSurface(), ui.getMainWindow());
	return 0;
}

// This will go in the Menu Screen loop
void draw(UI& ui, Panel& panel) {
	// draw panel ( make this a function of the UI object which takes a panel as a parameter )

	SDL_SetRenderDrawColor(ui.getMainRenderer(), 145, 145, 154, 1);
	SDL_RenderClear(ui.getMainRenderer());

	SDL_SetRenderDrawColor(ui.getMainRenderer(), 95, 77, 227, 1);

	vector<Button> buttons = panel.getButtons();

	for (int i = 0; i < buttons.size(); ++i) {
		// get the rect, send it a reference (to be converted to a pointer)
		SDL_Rect rect = buttons[i].getRect();
		SDL_Rect textRect = buttons[i].getTextRect();
		SDL_RenderFillRect(ui.getMainRenderer(), &rect);

		// now draw the text
		SDL_Surface* buttonTextSurface = TTF_RenderText_Blended(ui.getButtonFont(), buttons[i].getText().c_str(), ui.getTextColor());
		SDL_Texture* buttonTextTexture = SDL_CreateTextureFromSurface(ui.getMainRenderer(), buttonTextSurface);
		SDL_FreeSurface(buttonTextSurface);
		SDL_RenderCopyEx(ui.getMainRenderer(), buttonTextTexture, NULL, &textRect, 0, NULL, SDL_FLIP_NONE);
	}

	// Update window
	SDL_RenderPresent(ui.getMainRenderer());
}


void exit(SDL_Surface* surface, SDL_Window* window)
{
	SDL_FreeSurface(surface);
	surface = NULL;

	SDL_DestroyWindow(window);
	window = NULL;

	SDL_Quit();
}
