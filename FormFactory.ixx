/*
* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* ~  _____ ___  ____  __  __						  ~
* ~ |  ___/ _ \|  _ \|  \/  |						  ~
* ~ | |_ | | | | |_) | |\/| |						  ~
* ~ |  _|| |_| |  _ <| |  | |						  ~
* ~ |_|   \___/|_| \_\_|  |_|						  ~
* ~  _____ _    ____ _____ ___  ____  ___ _____ ____  ~
* ~ |  ___/ \  / ___|_   _/ _ \|  _ \|_ _| ____/ ___| ~
* ~ | |_ / _ \| |     | || | | | |_) || ||  _| \___ \ ~
* ~ |  _/ ___ \ |___  | || |_| |  _ < | || |___ ___) |~
* ~ |_|/_/   \_\____| |_| \___/|_| \_\___|_____|____/ ~
* ~                                                   ~
* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* 
* 
* 
* 
* Specific, basic forms of maps (levels), limbs, and suits (pure unscrambled characters) will be hardcoded and defined here.
* Map includes Blocks and Landmarks.
* Blocks will ALWAYS come from the DB (no need for BlockForm struct).
* Landmarks never change, except for their position in the map.
* Later if I want to use JSON instead (to let non-programmers make levels and characters) then I can just plug the JSON (or whatever) into this file.
* The rest of the program won't have to care where this Factory gets its info.
* Hard-coding for now lets me develop quicker. But it also guarantees type safety.
* 
* Whenever we actually load a Map (with its landmarks, blocks, limbs, and suits) we will FIRST load the BASE/VANILLA version, and then OVERRIDE it with DB data (if exists).
*/

/*
* TO DO:
* 1 - STRUCTs defining data of LIMB, CHARACTER, and MAP. (does this make base classes useless? MAYBE... maybe not... we want the SAVE function... but structs can have that?)
* 2 - DEFINITIONS of specific LIMBs, SUITs, and MAP.
* 3 - Load these into the Character Creation Screen.
* 
*/



export module FormFactory;

import "SDL.h";
import "SDL_image.h";
import <string>;
import <iostream>;
import <vector>;
import <unordered_map>;

import CharacterClasses;
import LimbFormMasterList;
import TypeStorage;
import UI;

using namespace std;

/*

This is the big, major push.
The entire app changes now. I currently am using a default, vanilla, static "map" to just represent basic functionality.
But now I'm setting up Dynamic Maps. I need to pull from basic Forms first, hard-coded yet dynamic (because each map is a level,
each Limb has a type) and then override them with Database information.


FIRST GOAL:

-- make a FOREST MAP form, complete with textures, use it to populate the actual Map Screen map.
^^DONE^^

SECOND GOAL:

-- add two SUITS
-- make SHRINES from those SUITS
-- scatter LIMBS around

For now we won't save anything to any Database.
We'll just make everything a one-off, based on FORM DEFINITIONS.

Right now the maps are all SQUARE.
Expand this later to lat them be RECTANGLES.


*/

/*
*
* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* ~  _____ ___  ____  __  __                            ~
* ~ |  ___/ _ \|  _ \|  \/  |                           ~
* ~ | |_ | | | | |_) | |\/| |                           ~
* ~ |  _|| |_| |  _ <| |  | |                           ~
* ~ |_|_ _\___/|_|_\_\_|_ |_|                           ~
* ~  ___ _   _ ____ _____  _    _   _  ____ _____ ____  ~
* ~ |_ _| \ | / ___|_   _|/ \  | \ | |/ ___| ____/ ___| ~
* ~  | ||  \| \___ \ | | / _ \ |  \| | |   |  _| \___ \ ~
* ~  | || |\  |___) || |/ ___ \| |\  | |___| |___ ___) |~
* ~ |___|_| \_|____/ |_/_/   \_\_| \_|\____|_____|____/ ~
* ~                                                     ~
* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* 
* The functions which create the actual FORM structs for the levels of the game.
* Sub-maps (dungeons and buildings) will not have suits or their own limbs.
* Their native limbs will draw from their parent map (MapLevel).
* Sub-maps CAN have their own NPCs and landmarks though.
* 
*/


/*
*			FOREST MAP STRUCTS
* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* ~  _____ ___  ____  _____ ____ _____        ~
* ~ |  ___/ _ \|  _ \| ____/ ___|_   _|       ~
* ~ | |_ | | | | |_) |  _| \___ \ | |         ~
* ~ |  _|| |_| |  _ <| |___ ___) || |         ~
* ~ |_|  _\___/|_| \_\_____|____/ |_|         ~
* ~ |  \/  |  / \  |  _ \                     ~
* ~ | |\/| | / _ \ | |_) |                    ~
* ~ | |  | |/ ___ \|  __/                     ~
* ~ |_|__|_/_/_ _\_\_|_   _  ____ _____ ____  ~
* ~ / ___|_   _|  _ \| | | |/ ___|_   _/ ___| ~
* ~ \___ \ | | | |_) | | | | |     | | \___ \ ~
* ~  ___) || | |  _ <| |_| | |___  | |  ___) |~
* ~ |____/ |_| |_| \_\\___/ \____| |_| |____/ ~
* ~                                           ~
* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/


MapForm forestMap() {
	UI& ui = UI::getInstance();
	MapForm forestMap;
	forestMap.mapLevel = MapLevel::Forest;
	forestMap.name = "Enchanted Forest";
	forestMap.slug = "forest";
	forestMap.blocksWidth = 100;
	forestMap.blocksHeight = 100;

	/* create the TEXTURES */

	SDL_Surface* wallSurface = IMG_Load("data/maps/forest/wall_001.png");
	SDL_Surface* floorSurface = IMG_Load("data/maps/forest/floor_001.png");

	/* DO ERROR CHECKS */

	forestMap.wallTexture = SDL_CreateTextureFromSurface(ui.getMainRenderer(), wallSurface);
	forestMap.floorTexture = SDL_CreateTextureFromSurface(ui.getMainRenderer(), floorSurface);

	SDL_FreeSurface(wallSurface);
	SDL_FreeSurface(floorSurface);

	forestMap.nativeLimbs = getMapLimbs(forestMap.mapLevel);

	// SUITS will come later.

	return forestMap;
}

/* Get the basic Map data struct based on slug ID parameter value. */
export MapForm getMapFormFromSlug(string slug) {
	if (slug == "forest") {
		cout << "string check worked\n";
		return forestMap();
	}
	cout << "string check FAILED\n";
	// Temporary DEFAULT map... deal with error (faulty slug) somehow instead...
	return forestMap();
}


/*
* 
FUNCTIONS TO CREATE:

export SDL_Texture buildAvatarFromLimbs() {}

each map definition will have it collection of Suits like:

vector<Character> suits = {
	getSuit(slugName1),
	getSuit(slugName2),
	getSuit(slugName3)
}

*/

/* 
* TAKES a slug identifier and returns a limb object or struct DEFINED IN THE FUNCTION.
* OR : should each MAP be a function which contains all its objects, and can return EITHER the full map OR just its pieces ?
*/