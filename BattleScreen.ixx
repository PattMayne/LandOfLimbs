/**
* 
*  ____    _  _____ _____ _     _____
* | __ )  / \|_   _|_   _| |   | ____|
* |  _ \ / _ \ | |   | | | |   |  _|
* | |_) / ___ \| |   | | | |___| |___
* |____/_/___\_\_|_  |_|_|_____|_____|
* / ___| / ___|  _ \| ____| ____| \ | |
* \___ \| |   | |_) |  _| |  _| |  \| |
*  ___) | |___|  _ <| |___| |___| |\  |
* |____/ \____|_| \_\_____|_____|_| \_|
* 
* Battle Screen.
* 
* -- Move panels into BattleScreen object.
* 
* 
* BATTLE SYSTEM:
* 
* - When attacking a limb, some of the damage inevitably spreads to the connected limbs instead.
* ---- Higher "intelligence" means more accuracy.
* - Type of attack depend on dominance cycle.
* 
* 
* 
* TO DO:
* - Create Battle Class.
* - Save battle object to DB (from MapScreen).
* - Load battle oject from DB (based on battle_id saved to PLAYER character).
* - Display both opponents (full drawn limbs, no avatar).
* - Animate both opponents (constantly rotating one limbs or another... just one at a time?).
* - HUD of player stats.
* - HUD of opponent stats.
* - Menu of UNEQUIPPED player limbs (to throw)
* ---- Include dominance cycle
* - Select EQUIPPED opponent Limb to attack via right-hand button.
* --- selected limb flashes.
* - Player limb to HEAL (by consuming LIMB from unequipped limbs).
* - THROW unequipped Limb (only if we have arm or leg to throw or kick).
* ---- THROW is more accurate but less power.
* ---- KICK is less accurate but more power.
* - PassingMessage panel shows effects of latest attack.
* - DROPPED limbs are returned to the map ON the opponent's square and scattered around.
* 
*/



module;
export module BattleScreen;

import "include/json.hpp";
import "SDL.h";
import "SDL_image.h";
import <stdio.h>;
import <string>;
import <iostream>;
import "SDL_ttf.h";
import <vector>;
import <cstdlib>;
import <time.h>;
import <unordered_map>;

import TypeStorage;
import GameState;
import Resources;
import UI;
import CharacterClasses;
import BattleClasses;
import Database;

using namespace std;



bool intsContainInt(vector<int> ints, int myInt) {
	for (int thisInt : ints) {
		if (myInt == thisInt) {
			return true;
		}
	}
	return false;
}


void bringSumTo100(int& numA, int& numB) {
	if (numA + numB > 100) {
		--numA;
		bringSumTo100(numB, numA);
	}
	else if (numA + numB < 100) {
		++numA;
		bringSumTo100(numB, numA);
	}
}



/* Map Screen class: where we navigate worlds, dungeons, and buildings. */
export class BattleScreen {
public:
	/* constructor */
	BattleScreen() {
		GameState& gameState = GameState::getInstance();
		UI& ui = UI::getInstance();

		screenToLoadStruct = ScreenStruct(ScreenType::Menu, 0);

		id = gameState.getScreenStruct().id;
		screenType = ScreenType::Battle;
		battle = loadBattle(id);

		getBackgroundTexture(ui);
		createTitleTexture(ui);

		settingsPanel = ui.createSettingsPanel(ScreenType::Map);
		settingsPanel.setShow(false);

		showTitle = true;
		titleCountdown = 140;

		/* Make sure characters have avatars (for now... later will draw limbs). */
		Character& playerCharacter = battle.getPlayerCharacter();
		Character& npc = battle.getNpc();

		npc.buildDrawLimbList();
		playerCharacter.buildDrawLimbList();

		playerCharacter.setTexture(playerCharacter.createAvatar());
		npc.setTexture(npc.createAvatar());

		/* Get the draw start points (WILL CHANGE after we implement button panels.) */
		int playerAvatarWidth, playerAvatarHeight, npcAvatarWidth, npcAvatarHeight;
		SDL_QueryTexture(playerCharacter.getTexture(), NULL, NULL, &playerAvatarWidth, &playerAvatarHeight);
		SDL_QueryTexture(npc.getTexture(), NULL, NULL, &npcAvatarWidth, &npcAvatarHeight);

		int windowHeight, windowWidth;
		SDL_GetWindowSize(ui.getMainWindow(), &windowWidth, &windowHeight);

		SDL_Rect playerAnchorLimbDrawRect = playerCharacter.getAnchorLimb().getDrawRect();
		SDL_Rect npcAnchorLimbDrawRect = npc.getAnchorLimb().getDrawRect();

		int playerX = ((windowWidth / 2) - playerAvatarWidth + playerAnchorLimbDrawRect.x) - 100 ;
		int playerY = (windowHeight / 2) - (playerAvatarHeight / 2) + playerAnchorLimbDrawRect.y;

		int npcX = (windowWidth / 2) + npcAnchorLimbDrawRect.x + 100;
		int npcY = (windowHeight / 2) - (npcAvatarHeight / 2) + npcAnchorLimbDrawRect.y;

		playerDrawStartPoint = Point( playerX, playerY );
		npcDrawStartPoint = Point(npcX, npcY);

		setLimbIdList();
		rotationCountdown = 30;
		resetRotationAmount();
		limbToRotateId = 0;
		skyBG = SkyAndCloudsBackground(true);

		unrotatePlayer = false;
		unrotateNpc = false;

		bobbingMeter = 0;
		bobbingMax = 20;
		reverseBob = false;

		playerStatsPanel = ui.createHud(ScreenType::Battle, playerCharacter.getCharStatsData(), false);
		playerStatsPanel.setShow(true);

		npcStatsPanel = ui.createHud(ScreenType::Battle, npc.getCharStatsData());
		npcStatsPanel.setShow(true);

		playerAttackStructs = playerCharacter.getAttacks();
		npcAttackStructs = npc.getAttacks();

		playerTurnPanel = ui.createBattlePanel(playerAttackStructs);
		playerTurnPanel.setShow(true);

		confirmationPanel = ui.createConfirmationPanel("Ready for Battle!", ConfirmationButtonType::OkCancel, false);
		confirmationPanel.setShow(true);

		passingMessagePanel = ui.createPassingMessagePanel("", true, true);
		passingMessagePanel.setShow(false);

		createNpcLimbPanel();
		createPlayerLimbPanels();

		flashingLimbCountdown = 10;
		drawFlashingLimb = true;
		flashLimb = false;

		attackAdvancePlayer = 0;
		attackAdvanceNpc = 0;
		passingMessageCountdown = 0;

		attackAdvanceHitTarget = false;
	}

	void createPlayerLimbPanels();
	void createNpcLimbPanel();
	void calculatePlayerDamageAttackStruct(int sourceLimbId, int targetLimbId);
	void calculateNpcDamageAttackStruct(int sourceLimbId, int targetLimbId);

	ScreenType getScreenType() { return screenType; }
	void run();

	bool applyPlayerAttackEffects();
	bool applyNpcAttackEffects();

private:
	ScreenType screenType;
	int id;
	ScreenStruct screenToLoadStruct;

	SDL_Texture* bgTexture = NULL;
	SDL_Rect bgSourceRect;
	SDL_Rect bgDestinationRect;

	SDL_Texture* titleTexture;
	SDL_Rect titleRect;

	void draw(UI& ui);
	void drawPlayer(UI& ui);
	void drawNpc(UI& ui);
	void setPlayerAttackAdvance();
	void setNpcAttackAdvance();

	void handleEvent(SDL_Event& e, bool& running, GameState& gameState);
	void checkMouseLocation(SDL_Event& e);
	void handlePlayerMove(ButtonClickStruct clickStruct);

	void launchNpcTurn();

	void getBackgroundTexture(UI& ui);
	void rebuildDisplay(Panel& settingsPanel);
	void createTitleTexture(UI& ui);

	void refreshNpcLimbsPanel();

	void raiseTitleRect() { --titleRect.y; }
	int getTitleBottomPosition() { return titleRect.y + titleRect.h; }

	bool showTitle;
	int titleCountdown;

	Point playerDrawStartPoint;
	Point npcDrawStartPoint;

	int rotationAmount;
	int rotationCountdown;
	int limbToRotateId;

	void resetRotationAmount();
	void setLimbIdList();
	void restartRotation();

	vector<int> limbIds;

	/* panels */
	Panel settingsPanel;
	Panel playerStatsPanel;
	Panel npcStatsPanel;
	Panel playerTurnPanel;
	Panel equippedLimbsPanel;
	Panel unequippedLimbsPanel;
	Panel npcLimbsPanel;
	Panel passingMessagePanel;
	Panel confirmationPanel;

	int passingMessageCountdown;

	Battle battle;

	SkyAndCloudsBackground skyBG;

	bool unrotatePlayer;
	bool unrotateNpc;

	int bobbingMeter;
	int bobbingMax;
	bool reverseBob;

	vector<AttackStruct> playerAttackStructs;
	vector<AttackStruct> npcAttackStructs;

	int flashingLimbCountdown;
	bool drawFlashingLimb; /* switches on and off WHILE flashLimb is true. */
	bool flashLimb;

	AttackStruct playerAttackLoaded;
	AttackStruct npcAttackLoaded;

	bool animateAttack;
	bool animateEffect;
	int animationCountdown;

	int attackAdvancePlayer;
	int attackAdvanceNpc;
	bool attackAdvanceHitTarget;

	vector<int> limbsIdsToSteal;
	vector<int> limbIdsToUpdate;

	bool running;
};



void BattleScreen::createNpcLimbPanel() {
	UI& ui = UI::getInstance();

	/* Destroy textures if they already exist. */
	npcLimbsPanel.destroyTextures();

	/* Build a vector of data structures so the UI can build the panel of Limb buttons. */
	vector<LimbButtonData> limbBtnDataStructs;
	vector<Limb>& limbs = battle.getNpc().getLimbs();

	for (int i = 0; i < limbs.size(); ++i) {
		Limb& thisLimb = limbs[i];
		if (thisLimb.isEquipped()) {
			limbBtnDataStructs.emplace_back(thisLimb.getTexturePath(), thisLimb.getName(), thisLimb.getId());
		}
	}

	npcLimbsPanel = ui.createChooseLimbModePanel(limbBtnDataStructs, true, "CHOOSE AN OPPONENT LIMB TO ATTACK:");
	npcLimbsPanel.setShow(false);
}


void BattleScreen::createPlayerLimbPanels() {
	UI& ui = UI::getInstance();

	/* Destroy textures if they already exist. */
	if (unequippedLimbsPanel.getButtons().size() > 0) {
		unequippedLimbsPanel.destroyTextures();
	}

	if (equippedLimbsPanel.getButtons().size() > 0) {
		equippedLimbsPanel.destroyTextures();
	}

	/* Build a vector of data structures so the UI can build the panel of Limb buttons. */
	vector<LimbButtonData> equippedLimbBtnDataStructs;
	vector<LimbButtonData> unequippedLimbBtnDataStructs;
	vector<Limb>& limbs = battle.getPlayerCharacter().getLimbs();

	for (int i = 0; i < limbs.size(); ++i) {
		Limb& thisLimb = limbs[i];
		if (thisLimb.isEquipped()) {
			equippedLimbBtnDataStructs.emplace_back(thisLimb.getTexturePath(), thisLimb.getName(), thisLimb.getId());
		}
		else {
			unequippedLimbBtnDataStructs.emplace_back(thisLimb.getTexturePath(), thisLimb.getName(), thisLimb.getId());
		}
	}

	unequippedLimbsPanel = ui.createChooseLimbModePanel(unequippedLimbBtnDataStructs, true, "NON-EQUIPPED LIMBS");
	equippedLimbsPanel = ui.createChooseLimbModePanel(equippedLimbBtnDataStructs, true, "EQUIPPED LIMBS");

	equippedLimbsPanel.setShow(false);
	unequippedLimbsPanel.setShow(false);
}




/* We need a prebuilt list of limb ids
* so we can randomly select one to rotate.
*/
void BattleScreen::setLimbIdList() {
	limbIds = {};

	for (Limb& limb : battle.getPlayerCharacter().getLimbs()) {
		if (limb.isEquipped()) {
			limbIds.push_back(limb.getId());
		}
	}

	for (Limb& limb : battle.getNpc().getLimbs()) {
		if (limb.isEquipped()) {
			limbIds.push_back(limb.getId());
		}
	}
}

void BattleScreen::resetRotationAmount() {
	rotationAmount = 2;

	if (rand() % 2 == 0) {
		rotationAmount *= -1;
	}
}


void BattleScreen::restartRotation() {

	/* Occaisionally reset the player and npc. */
	int resetCoin = rand() % 40;

	if (resetCoin == 0) {
		unrotatePlayer = true;
		return;
	} else if (resetCoin == 1) {
		unrotateNpc = true;
		return;
	}

	resetRotationAmount();

	if ((rand() % 4) > 0) {
		int index = rand() % limbIds.size();
		limbToRotateId = limbIds[index];
	}
	else {
		limbToRotateId = 0;
	}

	rotationCountdown = (rand() % 46) + 7;
}

void BattleScreen::getBackgroundTexture(UI& ui) {
	int windowHeight, windowWidth;
	SDL_GetWindowSize(ui.getMainWindow(), &windowWidth, &windowHeight);
	bgSourceRect = { 0, 0, windowWidth, windowHeight };
	bgDestinationRect = { 0, 0, windowWidth, windowHeight };
	bgTexture = ui.createBackgroundTexture();
}

export void BattleScreen::run() {
	/* singletons */
	GameState& gameState = GameState::getInstance();
	UI& ui = UI::getInstance();
	
	/* Timeout data */
	const int TARGET_FPS = 60;
	const int FRAME_DELAY = 600 / TARGET_FPS; // milliseconds per frame
	Uint32 frameStartTime; // Tick count when this particular frame began
	int frameTimeElapsed; // how much time has elapsed during this frame

	/* loop and event control */
	SDL_Event e;
	running = true;

	if (gameState.getScreenStruct().id < 1) {
		/* Invalid battle id. Give a notice to the user on the way out? Or after getting back?
		* Maybe GameState deserves a message string to display.
		*/
		running = false;
	}

	while (running) {
		/* Get the total running time(tick count) at the beginning of the frame, for the frame timeout at the end */
		frameStartTime = SDL_GetTicks();

		/* Check for events in queue, and handle them(really just checking for X close now */
		while (SDL_PollEvent(&e) != 0) {
			if (e.type == SDL_MOUSEBUTTONDOWN) {
				handleEvent(e, running, gameState);
			}
		}

		/* Deal with blinking loaded limb */
		if (flashLimb) {
			--flashingLimbCountdown;
			if (flashingLimbCountdown < 1) {
				drawFlashingLimb = !drawFlashingLimb;
				flashingLimbCountdown = 10;
			}
		}

		/* Deal with the Passing Message Panel. */
		if (passingMessagePanel.getShow()) {
			if (passingMessageCountdown > 0) {
				--passingMessageCountdown;
			}
			else {
				/* If we JUST hit zero: */
				passingMessagePanel.setShow(false);
			}
		}
		

		/* Deal with animation. */
		if (animationCountdown > 0) {
			--animationCountdown;
		}
		else {
			/* End of ANY animation. */

			if (animateEffect) {
				/* LAST FRAME. End of animateEffect animations (for either character). */
				animateEffect = !animateEffect;

				if (battle.isPlayerTurn()) {
					/* End of Player's animateEffect animation. */
					flashLimb = false;

					cout << "FINISHED the EFFECT animation\n";

					/* NOW actually change the limbs, clear the queue, and make it the  */

					applyPlayerAttackEffects();

					playerAttackLoaded = AttackStruct();
					limbsIdsToSteal = {};
					limbIdsToUpdate = {};

					/* RESETTING PLAYER TURN.
					* We will switch to NPC turn LATER.
					* After it's a smooth  cycle.
					*/

					playerTurnPanel.setShow(true);
					npcLimbsPanel.setShow(false);
					playerStatsPanel.setShow(true);
					npcStatsPanel.setShow(true);

				}
				else {
					/* End of NPC's animateEffect animation. */
					flashLimb = false;

					applyNpcAttackEffects();

					npcAttackLoaded = AttackStruct();
					limbsIdsToSteal = {};
					limbIdsToUpdate = {};
					
					playerTurnPanel.setShow(true);
					npcLimbsPanel.setShow(false);
					playerStatsPanel.setShow(true);
					npcStatsPanel.setShow(true);

					battle.setPlayerTurn(true);
				}

			}

		}

		/* Initiate NPC turn. */
		if (!battle.isPlayerTurn() && animationCountdown < 1 && npcAttackLoaded.attackType == AttackType::NoAttack) {
			cout << "NPC TURN!\n";
			launchNpcTurn();
		}


		/* Deal with showing the title. */
		if (showTitle) {
			if (titleCountdown > 0) {
				--titleCountdown; }
			else {
				raiseTitleRect(); }

			if (getTitleBottomPosition() < -1) {
				showTitle = false; }
		}

		checkMouseLocation(e);
		draw(ui);

		/* Delay so the app doesn't just crash */
		frameTimeElapsed = SDL_GetTicks() - frameStartTime; // Calculate how long the frame took to process
		/* Delay loop */
		if (frameTimeElapsed < FRAME_DELAY) {
			SDL_Delay(FRAME_DELAY - frameTimeElapsed);
		}

		if (rotationCountdown < 1) {
			restartRotation();
		}

		--rotationCountdown;

		if(reverseBob) {
			--bobbingMeter;
			if (bobbingMeter < (bobbingMax * -1)) {
				reverseBob = !reverseBob;
			}			
		}
		else {
			++bobbingMeter;
			if (bobbingMeter > bobbingMax) {
				reverseBob = !reverseBob;
			}
		}
	}

	/* set the next screen to load */
	gameState.setScreenStruct(screenToLoadStruct);
}

void BattleScreen::drawNpc(UI& ui) {
	Character& npc = battle.getNpc();
	vector<Limb>& limbs = npc.getLimbs();
	bool limbsLeftToUnrotate = false;
	SDL_Rect limbRect = { 0, 0, 0, 0 };

	setNpcAttackAdvance();

	for (int i = 0; i < limbs.size(); ++i) {
		Limb& limb = limbs[i];
		if (limb.isEquipped()) {
			bool drawLimb = !animateEffect || drawFlashingLimb || !intsContainInt(limbIdsToUpdate, limb.getId());

			SDL_Point rotationPoint = limb.getRotationPointSDL();
			limbRect = limb.getDrawRect();
			limbRect.x += npcDrawStartPoint.x + attackAdvanceNpc;
			limbRect.y += npcDrawStartPoint.y - bobbingMeter;

			int limbRotationAngle = limb.getRotationAngle();

			if (drawLimb) {
				SDL_RenderCopyEx(
					ui.getMainRenderer(),
					limb.getTexture(),
					NULL,
					&limbRect,
					limbRotationAngle,
					&rotationPoint,
					SDL_FLIP_NONE);
			}			

			/* Rotate limbs to make the NPC dance. */

			if (limbToRotateId == limb.getId()) {
				limb.rotate(rotationAmount);
				npc.setChildLimbDrawRects(limb, ui);
			}

			/* Player rotation logic. */
			if (!unrotateNpc) {
				if (limbToRotateId == limb.getId()) {
					limb.rotate(rotationAmount);
					npc.setChildLimbDrawRects(limb, ui);
				}
			}
			else {
				if (limbRotationAngle > 2) {
					if (limbRotationAngle < 180) {
						limb.rotate(-2);
						npc.setChildLimbDrawRects(limb, ui);
						limbsLeftToUnrotate = true;
					}
					else {
						limb.rotate(2);
						npc.setChildLimbDrawRects(limb, ui);
						limbsLeftToUnrotate = true;
					}					
				}
				else  if (limbRotationAngle < -2) {
					limb.rotate(2);
					npc.setChildLimbDrawRects(limb, ui);
					limbsLeftToUnrotate = true;
				}
			}
		}
	}

	if (unrotateNpc && !limbsLeftToUnrotate) {
		unrotateNpc = false;
	}
}


void BattleScreen::drawPlayer(UI& ui) {
	Character& playerCharacter = battle.getPlayerCharacter();
	vector<Limb>& limbs = playerCharacter.getLimbs();

	setPlayerAttackAdvance();

	SDL_Rect limbRect = { 0, 0, 0, 0 };
	bool limbsLeftToUnrotate = false;

	for (int i = 0; i < limbs.size(); ++i) {
		Limb& limb = limbs[i];
		if (limb.isEquipped()) {

			bool drawLimb = !animateEffect || drawFlashingLimb || !intsContainInt(limbIdsToUpdate, limb.getId());

			SDL_Point rotationPoint = limb.getRotationPointSDL();
			limbRect = limb.getDrawRect();
			limbRect.x += playerDrawStartPoint.x + attackAdvancePlayer;
			limbRect.y += playerDrawStartPoint.y + bobbingMeter;

			int limbRotationAngle = limb.getRotationAngle();

			if (drawLimb) {
				SDL_RenderCopyEx(
					ui.getMainRenderer(),
					limb.getTexture(),
					NULL,
					&limbRect,
					limbRotationAngle,
					&rotationPoint,
					SDL_FLIP_NONE);
			}
						

			/* Player rotation logic. */
			if (!unrotatePlayer) {
				if (limbToRotateId == limb.getId()) {
					limb.rotate(rotationAmount);
					playerCharacter.setChildLimbDrawRects(limb, ui);
				}
			}
			else {
				if (limbRotationAngle > 1) {
					if (limbRotationAngle < 180) {
						limb.rotate(-2);
						playerCharacter.setChildLimbDrawRects(limb, ui);
						limbsLeftToUnrotate = true;
					}
					else {
						limb.rotate(2);
						playerCharacter.setChildLimbDrawRects(limb, ui);
						limbsLeftToUnrotate = true;
					}
				}
				else  if (limbRotationAngle < -1) {
					limb.rotate(2);
					playerCharacter.setChildLimbDrawRects(limb, ui);
					limbsLeftToUnrotate = true;
				}
			}
		}
	}

	if (unrotatePlayer && !limbsLeftToUnrotate) {
		unrotatePlayer = false;
	}
}


void BattleScreen::draw(UI& ui) {
	/* draw panel(make this a function of the UI object which takes a panel as a parameter) */
	SDL_SetRenderDrawColor(ui.getMainRenderer(), 0, 0, 0, 1);
	SDL_RenderClear(ui.getMainRenderer());

	/* draw BG for now */
	//SDL_RenderCopyEx(ui.getMainRenderer(), bgTexture, &bgSourceRect, &bgDestinationRect, 0, NULL, SDL_FLIP_NONE);
	skyBG.draw();

	/* Draw the characters. */
	if (battle.isPlayerTurn()) {
		drawNpc(ui);
		drawPlayer(ui);
	}
	else {
		drawPlayer(ui);
		drawNpc(ui);
	}
	
	
	/* draw the title */
	if (showTitle) {
		SDL_RenderCopyEx(ui.getMainRenderer(), titleTexture, NULL, &titleRect, 0, NULL, SDL_FLIP_NONE);
	}

	settingsPanel.draw(ui);
	playerStatsPanel.draw(ui);
	npcStatsPanel.draw(ui);
	playerTurnPanel.draw(ui);
	npcLimbsPanel.draw(ui);
	confirmationPanel.draw(ui);
	passingMessagePanel.draw(ui);


	SDL_RenderPresent(ui.getMainRenderer()); /* update window */
}


/* Create the texture with the name of the game */
void BattleScreen::createTitleTexture(UI& ui) {
	string titleString = "BATTLE!";
	auto [incomingTitleTexture, incomingTitleRect] = ui.createTitleTexture(titleString);
	titleTexture = incomingTitleTexture;
	titleRect = incomingTitleRect;
}


/* Screen has been resized. Rebuild! */
void BattleScreen::rebuildDisplay(Panel& settingsPanel) {
	UI& ui = UI::getInstance();
	ui.rebuildSettingsPanel(settingsPanel, ScreenType::Map);
	getBackgroundTexture(ui);
	createTitleTexture(ui);
}


/* Process user input */
void BattleScreen::handleEvent(SDL_Event& e, bool& running, GameState& gameState) {
	/* User pressed X to close */
	if (e.type == SDL_QUIT) {
		cout << "\nQUIT\n";
		running = false;
		return;
	}
	else {
		// user clicked
		if (e.type == SDL_MOUSEBUTTONDOWN) {
			int mouseX, mouseY;
			SDL_GetMouseState(&mouseX, &mouseY);

			/* Check if confirmation panel is shown first (should deactivate other panels. */

			if (confirmationPanel.getShow()) {

				if (confirmationPanel.isInPanel(mouseX, mouseY)) {
					/* YES/NO CONFIRMATION PANEL. */
					ButtonClickStruct clickStruct = confirmationPanel.checkButtonClick(mouseX, mouseY);
					cout << "\n\nCLICK CONFIRMATION PANEL \n\n";

					if (clickStruct.buttonOption == ButtonOption::Agree) {
						cout << "OK!\n";
						confirmationPanel.setShow(false);
					}
				}				

			}
			else {
				cout << "user clicked mouse\n";
				// These events might change the value of screenToLoad
				

				if (settingsPanel.getShow() && settingsPanel.isInPanel(mouseX, mouseY)) {
					/* SETTINGS PANEL. */

					/* panel has a function to return which ButtonOption was clicked, and an ID(in the ButtonClickStruct). */
					ButtonClickStruct clickStruct = settingsPanel.checkButtonClick(mouseX, mouseY);
					UI& ui = UI::getInstance();
					/* see what button might have been clicked : */
					switch (clickStruct.buttonOption) {
					case ButtonOption::Mobile:
						ui.resizeWindow(WindowResType::Mobile);
						rebuildDisplay(settingsPanel);
						break;
					case ButtonOption::Tablet:
						ui.resizeWindow(WindowResType::Tablet);
						rebuildDisplay(settingsPanel);
						break;
					case ButtonOption::Desktop:
						ui.resizeWindow(WindowResType::Desktop);
						rebuildDisplay(settingsPanel);
						break;
					case ButtonOption::Fullscreen:
						ui.resizeWindow(WindowResType::Fullscreen);
						rebuildDisplay(settingsPanel);
						break;
					case ButtonOption::Back:
						settingsPanel.setShow(false);
						playerTurnPanel.setShow(true);
						break;
					case ButtonOption::Exit:
						/* back to menu screen */
						running = false;
						break;
					default:
						cout << "ERROR\n";
					}
				}
				else if (playerTurnPanel.getShow() && playerTurnPanel.isInPanel(mouseX, mouseY)) {
					/* BATTLE MENU. */

					cout << "\n\nCLICKED BATTLE MENU \n\n";

					ButtonClickStruct clickStruct = playerTurnPanel.checkButtonClick(mouseX, mouseY);

					UI& ui = UI::getInstance();
					/* see what button might have been clicked : */
					switch (clickStruct.buttonOption) {
					case ButtonOption::BattleMove:
						cout << "\nBATTLE MOVE!\n";

						handlePlayerMove(clickStruct);

						break;
					case ButtonOption::Settings:
						settingsPanel.setShow(true);
						playerTurnPanel.setShow(false);
						break;
					case ButtonOption::Exit:
						/* back to menu screen */
						running = false;
						break;
					default:
						cout << "ERROR\n";

					}
				}
				else if (npcLimbsPanel.getShow() && npcLimbsPanel.isInPanel(mouseX, mouseY)) {
					/* NPC LIMBS PANEL. */

					cout << "\n\nCLICK NPC LIMB MENU \n\n";
					ButtonClickStruct clickStruct = npcLimbsPanel.checkButtonClick(mouseX, mouseY);


					/* It might be the "back" button (but usually won't be so deal with that first). */
					if (clickStruct.buttonOption != ButtonOption::Back) {
						/* Do the animation.
						* When it counts down we'll launch the calculations.
						* After the NEXT animation we'll execute the results.
						*/
						UI& ui = UI::getInstance();
						playerAttackLoaded.targetLimbId = clickStruct.itemID;
						animateAttack = true;
						animationCountdown = 1000;
						attackAdvanceHitTarget = false;

						string attackMessage = "Player uses " + playerAttackLoaded.name + "!";
						passingMessageCountdown = 250;
						passingMessagePanel = ui.getNewPassingMessagePanel(attackMessage, passingMessagePanel, true, true);
						passingMessagePanel.setShow(true);
					}
					else {
						/* unload attack, reset panels. */
						playerTurnPanel.setShow(true);
						npcLimbsPanel.setShow(false);
						playerStatsPanel.setShow(true);
						npcStatsPanel.setShow(true);
						playerAttackLoaded = AttackStruct();

						return;
					}
				}
			}
		}
	}
}


void BattleScreen::checkMouseLocation(SDL_Event& e) {
	/* check for mouse over(for button hover) */
	int mouseX, mouseY;
	SDL_GetMouseState(&mouseX, &mouseY);

	/* send the x and y to the panel and its buttons to change the color */	
	if (confirmationPanel.getShow()) {
		confirmationPanel.checkMouseOver(mouseX, mouseY);
	}
	else {
		if (settingsPanel.getShow()) { settingsPanel.checkMouseOver(mouseX, mouseY); }
		if (playerTurnPanel.getShow()) { playerTurnPanel.checkMouseOver(mouseX, mouseY); }
		if (npcLimbsPanel.getShow()) { npcLimbsPanel.checkMouseOver(mouseX, mouseY); }
	}
}


/*
* 
* 
* 
* 
* 
* 
* 
* 
* 
* 
* 
* 
* 
* 
* BATTLE MOVE FUNCTIONS
* 
* 
* 
* 
* 
* 
* 
* 
* 
* 
* 
* 
* 
* 
*/




/*
* This interprets the user's input and sets up the Battle Move Sequence.
*/
void BattleScreen::handlePlayerMove(ButtonClickStruct clickStruct) {
	int clickID = clickStruct.itemID;

	UI& ui = UI::getInstance();
	cout << "\nBATTLE MOVE!\n";

	/* Check which attack. */
	for (AttackStruct aStruct : playerAttackStructs) {
		if (aStruct.attackType == clickID) {
			cout << "Attack Type: " << aStruct.name << "\n";

			/* Now we know that ATTACK TYPE matches existing attack options,
			* Go through those options and do the attack.
			*/

			if (aStruct.attackType == AttackType::Attack ||
				aStruct.attackType == AttackType::Punch ||
				aStruct.attackType == AttackType::DoublePunch ||
				aStruct.attackType == AttackType::Kick ||
				aStruct.attackType == AttackType::BodySlam
				) {
				/* REGULAR HP-DAMAGE ATTACKS */

				/*
				* If we do BODY SLAM then it should automatically choose anchoredLimb.
				*/


				/* Load attack
				* Open opponent Limb List
				* Select Opponent Limb
				* Do Attack
				* Animate Attack (flashing limb)
				* Show message panel with effects of attack.
				*/

				playerAttackLoaded = aStruct;

				/* Open the NPC Limb panel and user chooses targetId. */
				playerTurnPanel.setShow(false);
				npcLimbsPanel.setShow(true);
				playerStatsPanel.setShow(false);
				npcStatsPanel.setShow(false);

			}
			else if (aStruct.attackType == AttackType::BrainDrain) {

				/* This attack is general (all limbs), but is extra effective on Head body parts. */

				cout << "BRAIN DRAIN!\n";

			}
			else if (aStruct.attackType == AttackType::Steal) {

				/* This attack is general (all limbs), but is LESS effective on Torso body parts. */

				cout << "STEALIMG LIMB!\n";

			}
			else if (aStruct.attackType == AttackType::Throw) {

				/* This attack is general (all limbs), but is LESS effective on Torso body parts. */

				cout << "THROW!\n";

			}
			else if (aStruct.attackType == AttackType::Heal) {

				/* This attack is general (all limbs), but is LESS effective on Torso body parts. */

				cout << "HEAL!\n";

			}
		}
	}
}



/*
* BASIC ATTACK LOGIC is contained in this function.
* I may later move attack logic to dedicated functions, in a dedicated module.
* This is separate from the animations.
* Attack animation happens, then this function happens (and stores data), then effect animation happens (based on stored data).
* These calculations provide info for the next animation, but we don't execute on the calculations until after the last animation.
*/
void BattleScreen::calculatePlayerDamageAttackStruct(int sourceLimbId, int targetLimbId) {


	/* This should happen AFTER the animation.*/

	AttackType attackType = playerAttackLoaded.attackType;
	Character& playerCharacter = battle.getPlayerCharacter();
	Character& npc = battle.getNpc();
	vector<Limb>& npcLimbs = npc.getLimbs();
	vector<Limb>& playerLimbs = playerCharacter.getLimbs();

	npcLimbsPanel.setShow(false);

	/*
	* Calculate how much of which attributes to take from which limbs.
	* When a limb dies, figure out what to do with limbs attached to it.
	* What are the rules?
	* -- 0 hp limbs are stolen.
	* -- If it's the anchored limb, find another limb to be an anchored limb (prefer ones with their own child limbs).
	* sourceLimb to fly spinning at targetLimb -- calculated to do full 360s and return upright as normal... wants to share x/y with oppo
	*
	*
	* CREATE ANIMATION STRUCT.
	* BattleAnimationStruct
	* EffectAnimationStruct
	*/

	/* Calculate the attack. */
	int attack = 0;
	for (Limb& limb : playerCharacter.getLimbs()) {
		if (limb.isEquipped()) {
			attack += limb.getStrength() + 1;
		}
	}

	attack = attack / 10; // add some RANDOMNESS.
	int spreadAttack = 0;

	/* Get the target limb and make a vector of ids for spread attack. */

	vector<int> connectedLimbIds;
	int totalDamage = 0;

	int precision = playerAttackLoaded.precision;
	int intensity = playerAttackLoaded.intensity;
	bringSumTo100(precision, intensity);

	if (precision < 90) {

		spreadAttack = attack / (precision / 10);
		attack = attack - spreadAttack;

		/* attack must be higher. */
		if (spreadAttack > attack) {
			int tempNumber = spreadAttack;
			spreadAttack = attack;
			attack = tempNumber;
		}

		/* First get all touching limbs. */
		for (int i = npcLimbs.size() - 1; i >= 0; --i) {
			Limb& limb = npcLimbs[i];

			for (Joint& joint : limb.getJoints()) {
				int connectedLimbId = joint.getConnectedLimbId();

				if (connectedLimbId == targetLimbId) {
					connectedLimbIds.push_back(limb.getId());
				}
				else if (limb.getId() == targetLimbId) {
					if (connectedLimbId >= 0) {
						connectedLimbIds.push_back(connectedLimbId);
					}
				}
			}
		}
	}


	/* Now get the actual target limb, while also attacking the other limbs. */
	for (int i = npcLimbs.size() - 1; i >= 0; --i) {
		Limb& limb = npcLimbs[i];


		/* Attack the target limb. */
		if (limb.getId() == targetLimbId) {
			limb.modifyHP(-attack);
			totalDamage -= attack;
			limbIdsToUpdate.push_back(limb.getId());
		}
		else {
			if (precision < 90) {
				/* Attack the spread limbs. */
				for (int connectedLimbId : connectedLimbIds) {
					if (limb.getId() == connectedLimbId) {
						limb.modifyHP(-spreadAttack);
						totalDamage -= spreadAttack;
						limbIdsToUpdate.push_back(limb.getId());
					}
				}
			}
		}
	}

	int damageDisplayNumber = totalDamage * -1;
	UI& ui = UI::getInstance();
	cout << "Player does " << damageDisplayNumber << " total damage\n";

	string attackMessage = "Player does " + to_string(damageDisplayNumber) + " damage!";
	passingMessageCountdown = 250;
	passingMessagePanel = ui.getNewPassingMessagePanel(attackMessage, passingMessagePanel, true, true);
	passingMessagePanel.setShow(true);


	/* Attack is calculated and saved to BattleScreen variables, to be used after Effect animation. */

}

/*
* BASIC ATTACK LOGIC is contained in this function.
* I may later move attack logic to dedicated functions, in a dedicated module.
* This is separate from the animations.
* Attack animation happens, then this function happens (and stores data), then effect animation happens (based on stored data).
* These calculations provide info for the next animation, but we don't execute on the calculations until after the last animation.
*/
void BattleScreen::calculateNpcDamageAttackStruct(int sourceLimbId, int targetLimbId) {


	/* This should happen AFTER the animation.*/

	AttackType attackType = npcAttackLoaded.attackType;
	Character& playerCharacter = battle.getPlayerCharacter();
	Character& npc = battle.getNpc();
	vector<Limb>& npcLimbs = npc.getLimbs();
	vector<Limb>& playerLimbs = playerCharacter.getLimbs();


	/*
	* Calculate how much of which attributes to take from which limbs.
	* When a limb dies, figure out what to do with limbs attached to it.
	* What are the rules?
	* -- 0 hp limbs are stolen.
	* -- If it's the anchored limb, find another limb to be an anchored limb (prefer ones with their own child limbs).
	* sourceLimb to fly spinning at targetLimb -- calculated to do full 360s and return upright as normal... wants to share x/y with oppo
	*
	*
	* CREATE ANIMATION STRUCT.
	* BattleAnimationStruct
	* EffectAnimationStruct
	*/

	/* Calculate the attack. */
	int attack = 0;
	for (Limb& limb : npcLimbs) {
		if (limb.isEquipped()) {
			attack += limb.getStrength() + 1;
		}
	}

	attack = attack / 10; // add some RANDOMNESS.
	int spreadAttack = 0;

	/* Get the target limb and make a vector of ids for spread attack. */

	vector<int> connectedLimbIds;
	int totalDamage = 0;

	int precision = npcAttackLoaded.precision;
	int intensity = npcAttackLoaded.intensity;
	bringSumTo100(precision, intensity);

	if (precision < 90) {

		spreadAttack = attack / (precision / 10);
		attack = attack - spreadAttack;

		/* attack must be higher. */
		if (spreadAttack > attack) {
			int tempNumber = spreadAttack;
			spreadAttack = attack;
			attack = tempNumber;
		}

		/* First get all touching limbs. */
		for (int i = playerLimbs.size() - 1; i >= 0; --i) {
			Limb& limb = playerLimbs[i];

			for (Joint& joint : limb.getJoints()) {
				int connectedLimbId = joint.getConnectedLimbId();

				if (connectedLimbId == targetLimbId) {
					connectedLimbIds.push_back(limb.getId());
				}
				else if (limb.getId() == targetLimbId) {
					if (connectedLimbId >= 0) {
						connectedLimbIds.push_back(connectedLimbId);
					}
				}
			}
		}
	}

	/* Now get the actual target limb, while also attacking the other limbs. */
	for (int i = playerLimbs.size() - 1; i >= 0; --i) {
		Limb& limb = playerLimbs[i];

		/* Attack the target limb. */
		if (limb.getId() == targetLimbId) {
			limb.modifyHP(-attack);
			totalDamage -= attack;
			limbIdsToUpdate.push_back(limb.getId());
		}
		else {
			if (precision < 90) {
				/* Attack the spread limbs. */
				for (int connectedLimbId : connectedLimbIds) {
					if (limb.getId() == connectedLimbId) {
						limb.modifyHP(-spreadAttack);
						totalDamage -= spreadAttack;
						limbIdsToUpdate.push_back(limb.getId());
					}
				}
			}
		}
	}

	int damageDisplayNumber = totalDamage * -1;
	UI& ui = UI::getInstance();
	cout << npc.getName() + " does " << damageDisplayNumber << " total damage\n";

	string attackMessage = npc.getName() + " does " + to_string(damageDisplayNumber) + " damage!";
	passingMessageCountdown = 250;
	passingMessagePanel = ui.getNewPassingMessagePanel(attackMessage, passingMessagePanel, true, true);
	passingMessagePanel.setShow(true);

	/* Attack is calculated and saved to BattleScreen variables, to be used after Effect animation. */

}


/*
* Apply the effects that we just calculated and animated.
* If player loses anchor limb, send them to Characetr Creation screen.
*/
bool BattleScreen::applyNpcAttackEffects() {
	Character& npc = battle.getNpc();
	Character& playerCharacter = battle.getPlayerCharacter();
	vector<Limb>& playerLimbs = playerCharacter.getLimbs();
	vector<Limb>& npcLimbs = npc.getLimbs();
	int playerId = playerCharacter.getId();
	int npcId = npc.getId();
	UI& ui = UI::getInstance();

	sqlite3* db = startTransaction();

	/*
	* 1. Check for <1 hp and give those limbs to NPC.
	*   ---> remove them from player object and vector.
	* 2. Save all limbs and both characters.
	* 
	*/

	bool defeatedAnchorLimb = false;
	bool defeatedAnyLimb = false;

	for (int i = playerLimbs.size() - 1; i <= 0; --i) {
		Limb& limb = playerLimbs[i];
		if (limb.isEquipped() && limb.getHP() < 1) {
			/* Equipped limb is defeated. Give it to NPC. */
			defeatedAnyLimb = true;

			if (playerCharacter.getAnchorLimbId() == limb.getId()) {
				defeatedAnchorLimb = true; }

			playerCharacter.unEquipLimb(limb.getId());
			limb.setCharacterId(npcId);

			npc.getLimbs().push_back(limb);

			playerLimbs.erase(playerLimbs.begin() + i);
		}
	}

	if (defeatedAnyLimb && !defeatedAnchorLimb) {
		playerCharacter.buildDrawLimbList();
	}
	else if (defeatedAnchorLimb) {
		cout << "DEFEATED ANCHOR LIMB.\n Sending to Character Creation Screen.\n";
		playerCharacter.clearSuit();
		screenToLoadStruct.screenType == ScreenType::CharacterCreation;
		running = false;
		updateCharacterAnchorIdInTrans(playerId, playerCharacter.getAnchorLimbId(), db);
	}

	

	/* Save all the limbs. */
	
	for (Limb& limb : playerLimbs) {
		updateLimbBattleEffectsInTransaction(limb, db);
	
	}

	if (defeatedAnyLimb) {
		for (Limb& limb : npcLimbs) {
			updateLimbBattleEffectsInTransaction(limb, db);

		}
	}

	commitTransactionAndCloseDatabase(db);
	createPlayerLimbPanels();
	playerStatsPanel.destroyTextures();
	playerStatsPanel = ui.createHud(ScreenType::Battle, playerCharacter.getCharStatsData(), false);

	npcAttackLoaded = AttackStruct();
	limbIdsToUpdate = {};
	
	return true;
}



/*
* Apply the effects that we just calculated and animated.
* If npc loses any limbs, rebuild character with already-equipped limbs ONLY.
*/
bool BattleScreen::applyPlayerAttackEffects() {
	Character& npc = battle.getNpc();
	int playerId = battle.getPlayerCharacter().getId();
	int npcId = npc.getId();
	UI& ui = UI::getInstance();

	sqlite3* db = startTransaction();

	vector<Limb>& npcLimbs = npc.getLimbs();
	bool erasedLimb = false;

	vector<int> limbIdsToEquip;
	for (int thisId : npc.getDrawLimbIDs()) {
		limbIdsToEquip.emplace_back(thisId);
	}

	/* Damage has already been done (to attributes). Now save those changes. */
	for (int i = npc.getLimbs().size() - 1; i >= 0; --i) {
		Limb& limb = npc.getLimbs()[i];
		bool erasedThisLimb = false;

		/* For MODIFIED limbs (attacked, healed, whatever). */
		for (int limbId : limbIdsToUpdate) {
			if (limbId == limb.getId()) {
				/* Damage already happened in the calculations function. Now we just save them. */
				updateLimbBattleEffectsInTransaction(limb, db);
				break;
			}
		}

		if (limb.getHP() < 1) {
			npc.unEquipLimb(limb.getId());
			limb.setCharacterId(playerId);
			limb.unEquip();

			erasedLimb = true;
			erasedThisLimb = true;

			int limbId = limb.getId();
			for (int k = limbIdsToEquip.size() - 1; k >= 0; --k) {
				if (limbIdsToEquip[k] == limbId) {
					limbIdsToEquip.erase(limbIdsToEquip.begin() + k);
				}
			}

			updateLimbBattleEffectsInTransaction(limb, db);
			npc.getLimbs().erase(npc.getLimbs().begin() + i);
			continue;
		}

	}

	bool npcDefeated = false;

	if (erasedLimb) {
		cout << "\nLIMB ERASED\n";

		/* REBUILD CHARACTER.
		* We need a function to REBUILD CHARACTER after losing a limb.
		* But we can only rebuild with limbs that are ALREADY equipped.
		* Don't make the player fight through the entire catalog of limbs.
		*
		* ALSO check for VICTORY CONDITIONS.
		*/
		npc.clearSuit();
		for (Limb& limb : npc.getLimbs()) {
			npc.unEquipLimb(limb.getId());
			updateLimbBattleEffectsInTransaction(limb, db);
		}

		if (limbIdsToEquip.size() > 0) {
			for (Limb& limb : npc.getLimbs()) {

				for (int limbIdToEquip : limbIdsToEquip) {
					if (limb.getId() == limbIdToEquip) {
						bool equipSuccess = npc.equipLimb(limbIdToEquip);
					}
				}

				/* Update them all because some may have been cut out. */
				npc.setTexture(npc.createAvatar());
				updateLimbBattleEffectsInTransaction(limb, db);
			}

			npc.buildDrawLimbList();
			updateCharacterAnchorIdInTrans(npc.getId(), npc.getAnchorLimbId(), db);
			setLimbIdList();
		}
		else {
			npcDefeated = true;
		}
	}

	/* After all this, make sure that the npc has equipped limbs. */
	int numberOfEquippedLimbs = 0;
	for (Limb& limb : npc.getLimbs()) {
		if (limb.isEquipped()) {
			npcDefeated = false;
			++numberOfEquippedLimbs;
		}
	}

	cout << numberOfEquippedLimbs << " equippedLIMBS\n";

	if (numberOfEquippedLimbs < 1 or npc.getAnchorLimbId() < 1) {
		npcDefeated = true;
	}

	if (npcDefeated) {
		cout << "\n NPC DEFEATED\n\n";
		/* VICTORY CONDITIONS.
	* Player won the battle.
	*
	* Destroy NPC.
	* Spread limbs
	* ---> Player gets previously-equipped limbs.
	* ---> Non-equipped limbs move to the NPCs block.
	*/
		for (Limb& limb : npc.getLimbs()) {
			npc.unEquipLimb(limb.getId());
			limb.unEquip();

			bool limbWasGonnaEquip = false;
			for (int limbIdToEquip : limbIdsToEquip) {
				if (limb.getId() == limbIdToEquip) {
					limbWasGonnaEquip = true;
				}
			}

			if (!limbWasGonnaEquip) {
				limb.setCharacterId(-1);
				limb.setPosition(npc.getPosition());
			}
			else {
				limb.setCharacterId(playerId);
			}

			updateLimbBattleEffectsInTransaction(limb, db);
		}

		npc.buildDrawLimbList();
		/* Now destroy the NPC from the database. */
		deleteCharacterInTrans(npc.getId(), db);
		setLimbIdList();

		/* Victory Condition gives you a popup message, and then go back to the map. */
	}
	else {
		cout << "NPC NOT DEFEATED YET!!! \n";
	}

	commitTransactionAndCloseDatabase(db);
	createNpcLimbPanel();
	npcStatsPanel.destroyTextures();
	npcStatsPanel = ui.createHud(ScreenType::Battle, npc.getCharStatsData());

	playerAttackLoaded = AttackStruct();
	limbIdsToUpdate = {};

	battle.switchTurn();

	return true;
}


/* When the npc attacks the player, they must theatrically "advance" on their position.
* This function incrementally moves the NPC toward the player's position, and then bounces it back.
*/
void BattleScreen::setNpcAttackAdvance() {
	if (!animateAttack || battle.isPlayerTurn()) {
		attackAdvanceNpc = 0;
		return;
	}

	if (!attackAdvanceHitTarget && npcDrawStartPoint.x + attackAdvanceNpc > playerDrawStartPoint.x) {
		attackAdvanceNpc = attackAdvanceNpc - 20;
	}
	else if (!attackAdvanceHitTarget) {
		attackAdvanceHitTarget = true;
		attackAdvanceNpc = attackAdvanceNpc + 20;
		cout << "Hit target. Why no flash?\n";
		calculateNpcDamageAttackStruct(-1, npcAttackLoaded.targetLimbId);
		animateEffect = true;
		animationCountdown = 100;
		flashingLimbCountdown = 10;
		drawFlashingLimb = false;
		flashLimb = true;

		/* We deal with end of Effect Animation in the run() function. */
	}
	else if (attackAdvanceNpc < 0) {
		attackAdvanceNpc = attackAdvanceNpc + 20;
	}
	else {
		animateAttack = !animateAttack;
		attackAdvanceNpc = 0;
	}
}


/* When the player attacks the npc, they must theatrically "advance" on their position.
* This function incrementally moves the player toward the NPC's position, and then bounces it back.
*/
void BattleScreen::setPlayerAttackAdvance() {
	if (!animateAttack || !battle.isPlayerTurn()) {
		attackAdvancePlayer = 0;
		return;
	}

	if (!attackAdvanceHitTarget && playerDrawStartPoint.x + attackAdvancePlayer < npcDrawStartPoint.x) {
		attackAdvancePlayer = attackAdvancePlayer + 20;
	}
	else if (!attackAdvanceHitTarget) {
		attackAdvanceHitTarget = true;
		attackAdvancePlayer = attackAdvancePlayer - 20;

		calculatePlayerDamageAttackStruct(-1, playerAttackLoaded.targetLimbId);
		animateEffect = true;
		animationCountdown = 100;
		flashingLimbCountdown = 10;
		drawFlashingLimb = false;
		flashLimb = true;

		/* We deal with end of Effect Animation in the run() function. */
	}
	else if (attackAdvancePlayer > 0) {
		attackAdvancePlayer = attackAdvancePlayer - 20;
	}
	else {
		animateAttack = !animateAttack;
		attackAdvancePlayer = 0;
	}
}



/*
* 
* 
* 
* 
* 
* 
* 
* 
* 
* 
*		NPC BATTLE MOVE FUNCTIONS
* 
* 
* 
* 
* 
* 
* 
* 
* 
*/



void BattleScreen::launchNpcTurn() {
	cout << "Launching NPC turn\n";
	Character& npc = battle.getNpc();
	Character& playerCharacter = battle.getPlayerCharacter();

	/* Get the ATTACKS. */

	vector<AttackStruct> attackStructs = npc.getAttacks();

	if (attackStructs.size() < 1 && !battle.isPlayerTurn()) {
		battle.switchTurn();
		return;
	}

	int attackStructIndex = rand() % attackStructs.size();

	AttackStruct attackStruct = attackStructs[attackStructIndex];
	npcAttackLoaded = attackStruct;

	/* Make sure the player has limbs to attack. */
	if (playerCharacter.getLimbs().size() < 1) {

		cout << "PLAYER HAS NO LIMBS\n";
		// LOSING CONDITIONS.
	}

	/* Get the TARGET LIMB ID. */

	vector<int> targetLimbIds;

	for (Limb& limb : playerCharacter.getLimbs()) {
		if (limb.isEquipped()) {
			targetLimbIds.push_back(limb.getId());
		}
	}

	if (targetLimbIds.size() < 1) {
		cout << "WE CANNOT FIGHT because the player has no limbs equipped\n";

		/* 
		* SEND the PLAYER to the CC screen...
		* unless they have no limbs at all... in that case player loses.
		*/
	}

	/* Load the targetLimbId, start the animation. */

	npcAttackLoaded.targetLimbId = targetLimbIds[rand() % targetLimbIds.size()];

	attackAdvanceHitTarget = false;
	animateAttack = true;

	/* START THE ANIMATION. */


	/*
	* 1. get available attacks.
	* 2. randomly pick an attack and a targetLimb.
	* 3. copy the PlayerAttack functions to make the NPC attack the player.
	* 4. Initiate the animation, calculations, and executions.
	*/
}