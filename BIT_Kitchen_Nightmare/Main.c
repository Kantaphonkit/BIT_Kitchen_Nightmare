#define SDL_MAIN_HANDLED
#include <stdio.h>
#include "SDL.h"
#include "./constant.h"
#include "SDL_image.h"
#include "SDL_main.h"
#include "SDL_ttf.h"
#include "AudioManager.h"
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

//Define variable
int game_is_running = FALSE;
SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
SDL_Texture* map_texture = NULL;
SDL_Texture* start_button_texture = NULL;
SDL_Texture* setting_button_texture = NULL;
SDL_Texture* collection_button_texture = NULL;
SDL_Texture* shop_button_texture = NULL;
SDL_Texture* menu_background_texture = NULL;
SDL_Texture* game_over_texture = NULL;
SDL_Texture* congrates_texture = NULL;
SDL_Texture* retry_texture = NULL;
SDL_Texture* back_to_menu_texture = NULL;

//Pause menu
SDL_Texture* resume_button_texture = NULL;
SDL_Texture* exit_button_texture = NULL;

//Time
int last_frame_time = 0;
float  delta_time;
const Uint32 periodicInterval = 60000;
Uint32 lastPeriodicCall = 0;
int waveIndex = 0;

//Main_character
int move_up = FALSE;
int move_down = FALSE;
int move_left = FALSE;
int move_right = FALSE;
int facing_left = 0; // Add this global variable
int map[MAP_HEIGHT][MAP_WIDTH];
bool render_motion = true;
Uint32 last_switch_time = 0; // Tracks the last time we switched textures


//Player camera position
struct Camera {
	float x, y;
	int width, height;
} camera = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };

//Attack system
typedef struct {
	SDL_Rect area; // The attack area
	Uint32 cooldown; // Time in milliseconds between attacks
	Uint32 lastAttackTime; // Last time this attack was used
	Uint32 lastTimeRender;
	SDL_Texture* vfxTexture; // Texture for the attack's visual effect
	int damage; // Damage dealt by the attack
	int level; // Level of the attack, influencing damage, area, etc.
	bool isActive;// Is the attack currently active/enabled
	bool isRender;
	bool isHave;
} AutoAttack;

//Character stat
struct character {
	float x;
	float y;
	float width;
	float height;
	float movement_speed; // Pixels per second, adjust as needed
	float health;
	AutoAttack attacks[MAX_ATTACKS];
	int haveAttacks; // Number of attacks currently active
	int type;
	SDL_Texture* texture[2];
} Main_character;

//Each enemy type stat
typedef struct {
	float width;
	float height;
	float movement_speed;
	float health;
	float atk;
	SDL_Texture* texture;
} Enemy_type;

//Store each enemy in the game;
typedef struct {
	SDL_Rect rect; // Position and size
	int type; // Index to the Enemy_type
	float currentHealth; // Current health if it can decrease from the base
	int isActive; // 1 if active, 0 if not
	int last_damage_taken;
	Uint32 damage_display_timer;
} Enemy;

//Wave data
typedef struct {
	int Enemy_count[MAX_ENEMY_TYPE]; //index will shown amount of itype enemy
}Wave;

//stage data
typedef struct {
	Wave waves[MAX_WAVES];
} Stage;

//Define struct of enemy data
Enemy_type type[MAX_ENEMY_TYPE];
Stage stage1;
Enemy Enemies[MAX_ENEMIES_STAGE1];
int randPos;
int typeTexture;
int killed_enemy;


//8 spawn position
struct spawn_pos {
	float x;
	float y;
}spawn_position[8] = {
	{ -200, -200},
	{ 1920, -200},
	{ 4040, -200},
	{ 4040, 1080},
	{ 4040, 2360},
	{ 1920, 2360},
	{ -200, 2360},
	{ -200, 1080},
};

enum GameState {
	GAME_STATE_MAIN_MENU,
	GAME_STATE_GAMEPLAY,
	GAME_STATE_PAUSE_MENU,
	GAME_STATE_WIN,
	GAME_STATE_LOSE
};
enum GameState gameState = GAME_STATE_MAIN_MENU;

// Music track state
typedef enum {
	NONE_MUSIC = -1, //Indicate there is no music
	MAIN_MENU_MUSIC,
	GAMEPLAY_MUSIC
} MusicTrack;

MusicTrack currentMusicTrack = NONE_MUSIC;

// Initialization and setup functions
int initialize_window(void);
void gameplay_setup(void);
void setup(void);
void reset_game_state(void);                                                 

//Enemies function related
void initialize_enemies(SDL_Renderer* renderer);
void initialize_stage1_enemies();
void spawn_wave(int waveIndex);
void render_enemies(SDL_Renderer* renderer);
void update_enemies(float delta_time);

// Texture loading
SDL_Texture* load_texture(const char* filename, SDL_Renderer* renderer);

//combat mechanics
void check_collision_and_apply_damage(float delta_time);
void initialize_attacks(void);
void updated_attacks(Uint32 currentTime);
void render_attacks();
void apply_attack_damage_to_enemies();
void render_enemy_damage(SDL_Renderer* renderer);



// Game loop functions
int menu_process_input(void);
int gameplay_process_input(void);
int pause_process_input(void);
void gameplay_update(float delta_time);
void menu_render(void);
void gameplay_render(void);
int game_lose_process_input(void);
void game_lose_state_render(void);
int game_win_process_input(void);
void game_win_state_render(void);
void pause_render(void);
void update_camera(void); 
void cap_framerate(int* last_frame_time, float* delta_time); //FPS


// Rendering helpers
void render_health_bar(SDL_Renderer* renderer, float health, float max_health, int x, int y, int width, int height);

// Cleanup
void destroy_window();


int main(int argc, char* argv[]) {

	printf("Game is running...\n");

	game_is_running = initialize_window();

	if(SDL_Init(SDL_INIT_AUDIO) < 0) {
		printf("SDL Audio could not initialize! SDL Audio Error: %s\n", SDL_GetError());
	}

	
	int MusicWasPaused = 0; //Keep track of the music state

	setup();
	lastPeriodicCall = SDL_GetTicks() - periodicInterval;

	
	

	while (game_is_running) {

		switch (gameState) {

		case GAME_STATE_MAIN_MENU:

			// Play the main menu music
			if (currentMusicTrack != MAIN_MENU_MUSIC) { // Check if the current music track is not the main menu music
				AudioManager_LoadAndPlayMusic("Assets/Background Musics/Demo1.mp3", -1); // Load and play the main menu music
			
			Mix_VolumeMusic(50); // Set the volume to 50%
			currentMusicTrack = MAIN_MENU_MUSIC; // Update the current music track
			}

				if(menu_process_input() == 1) gameState = GAME_STATE_GAMEPLAY;
				menu_render();
				
				break;

			case GAME_STATE_GAMEPLAY: {

				Uint32 currentTime = SDL_GetTicks();

				if (currentMusicTrack != GAMEPLAY_MUSIC) {
					AudioManager_LoadAndPlayMusic("Assets/Background Musics/Demo2.mp3", -1);
					currentMusicTrack = GAMEPLAY_MUSIC;
				}
				

				if (gameplay_process_input() == 2) { // Indicates a request to enter pause menu
					gameState = GAME_STATE_PAUSE_MENU;
					AudioManager_PauseMusic(); //Pause the music
					MusicWasPaused = 1; //Keep track of the music state
				}
				else {
					Uint32 currentTime = SDL_GetTicks();
					if (currentTime - lastPeriodicCall >= periodicInterval) {
						if (waveIndex <= 19) {
							spawn_wave(waveIndex);
							waveIndex++;
						}
						lastPeriodicCall += periodicInterval;
					}
					gameplay_update(delta_time);
					gameplay_render();
				}
				break;
			}

			case GAME_STATE_LOSE: {

				if (game_lose_process_input() == 1) {
					reset_game_state();
					gameState = GAME_STATE_MAIN_MENU;
				}
				game_lose_state_render();

				break;
			}

			case GAME_STATE_WIN:

				if (game_win_process_input() == 1) {
					reset_game_state();
					gameState = GAME_STATE_MAIN_MENU;
				}
				game_win_state_render();

				break;

			case GAME_STATE_PAUSE_MENU: {
				// Process pause menu input and render
				int pauseInputResult = pause_process_input();
				if (pauseInputResult == 1) { // Resume game
					gameState = GAME_STATE_GAMEPLAY;
					if(MusicWasPaused == 1){
						AudioManager_ResumeMusic(); // Resume the music
					}
				}
				else if (pauseInputResult == 2) { // Exit to main menu
					reset_game_state();
					gameState = GAME_STATE_MAIN_MENU;
					AudioManager_StopMusic(); // Stop the music
				}
				pause_render();
				break;
			}
		}

		cap_framerate(&last_frame_time, &delta_time);
	}

	destroy_window;
	return 0;
}

int initialize_window(void) {
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {

		fprintf(stderr, "Error initializing SDL.\n");
		return FALSE;
	}

	window = SDL_CreateWindow(
		NULL,
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		WINDOW_WIDTH,
		WINDOW_HEIGHT,
		SDL_WINDOW_FULLSCREEN
	);
	if (!window) {
		fprintf(stderr, "Error creating SDL Window.\n");
		return FALSE;
	}

	renderer = SDL_CreateRenderer(window, -1, 0);

	if (!renderer) {
		fprintf(stderr, "Error creating SDL Renderer.\n");
		return FALSE;
	}

	if (IMG_Init(IMG_INIT_PNG) && IMG_INIT_PNG != IMG_INIT_PNG) {
		fprintf(stderr, "Failed to initialize SDL_image: %s\n", IMG_GetError());
		return FALSE;
	}

	if (TTF_Init() == -1) {
		printf("SDL_ttf could not initialize! SDL_ttf Error: %s\n", TTF_GetError());
		// Handle the error, perhaps exit the program
	}

	initialize_enemies(renderer);
	initialize_stage1_enemies();
	initialize_attacks();
	//Initialize the audio system
	AudioManager_Init();

	return TRUE;
}

int menu_process_input() {
	const Uint8* state = SDL_GetKeyboardState(NULL);
	SDL_Event event;

	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_QUIT) {
			game_is_running = FALSE;
		}

		if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
			game_is_running = FALSE;
		}

		if (event.type == SDL_MOUSEBUTTONDOWN) {
			int x, y;
			SDL_GetMouseState(&x, &y);
			if (x >= 780 && x <= 1140 && y >= 550 && y <= 648) { // If the mouse click is within the start button area
				return 1; // Start the game
			}
		}
		
	}

	return 0;
}

int gameplay_process_input() {
	const Uint8* state = SDL_GetKeyboardState(NULL);
	SDL_Event event;

	int mouseX, mouseY;
	SDL_GetMouseState(&mouseX, &mouseY);

	mouseX += camera.x;
	mouseY += camera.y;

	while (SDL_PollEvent(&event)) {
		
		if (event.type == SDL_QUIT) {
			game_is_running = FALSE;
			return 0;
		}else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
			return 2;
		}

		// Update facing direction based mouse direction
		if (mouseX > (Main_character.x + Main_character.width / 2)) {
			facing_left = 0; // Face right
		}
		else {
			facing_left = 1; // Face left
		}
	}

	// Update movement directions based on key states
	move_up = state[SDL_SCANCODE_W];
	move_down = state[SDL_SCANCODE_S];
	move_left = state[SDL_SCANCODE_A];
	move_right = state[SDL_SCANCODE_D];

	return 1;
}

int game_lose_process_input() {

	const Uint8* state = SDL_GetKeyboardState(NULL);
	SDL_Event event;

	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_QUIT) {
			game_is_running = FALSE;
		}

		if (event.type == SDL_MOUSEBUTTONDOWN) {
			int x, y;
			SDL_GetMouseState(&x, &y);
			if (x >= 630 && x <= 1331 && y >= 675 && y <= 763) { // If the mouse click is within menu button area
				return 1; // to menu
			}
		}

	}

	return 0;

}

int game_win_process_input() {

	const Uint8* state = SDL_GetKeyboardState(NULL);
	SDL_Event event;

	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_QUIT) {
			game_is_running = FALSE;
		}

		if (event.type == SDL_MOUSEBUTTONDOWN) {
			int x, y;
			SDL_GetMouseState(&x, &y);
			if (x >= 630 && x <= 1331 && y >= 600 && y <= 688) { // If the mouse click is within the start button area
				return 1; // Start the game
			}
		}

	}

	return 0;


}


int pause_process_input() {
	const Uint8* state = SDL_GetKeyboardState(NULL);
	SDL_Event event;

	

	while (SDL_PollEvent(&event)) {

		if (event.type == SDL_QUIT) {
			game_is_running = FALSE;
			return 0;
		}

			// Assuming these are the button positions and sizes
			SDL_Rect resume_button_rect = { 785, 320, 350, 150 }; // Placeholder positions
			SDL_Rect exit_button_rect = { 785, 600, 350, 150 }; // Placeholder positions

			int mouseX, mouseY;
			SDL_GetMouseState(&mouseX, &mouseY);

			if (event.type == SDL_MOUSEBUTTONDOWN) {

				if (mouseX >= resume_button_rect.x && mouseX <= resume_button_rect.x + resume_button_rect.w &&
					mouseY >= resume_button_rect.y && mouseY <= resume_button_rect.y + resume_button_rect.h) {
					// Resume button was clicked
					return 1;
				}else if (mouseX >= exit_button_rect.x && mouseX <= exit_button_rect.x + exit_button_rect.w &&
					mouseY >= exit_button_rect.y && mouseY <= exit_button_rect.y + exit_button_rect.h){
					// Exit button was clicked
					return 2;
					}	
			}
	}

	return 3;
}

SDL_Texture* load_texture(const char* filename, SDL_Renderer* renderer) {
	SDL_Surface* surface = IMG_Load(filename);
	if (!surface) {
		fprintf(stderr, "Failed to load image: %s\n", IMG_GetError());
		return NULL;
	}

	SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
	if (!texture) {
		fprintf(stderr, "Failed to create texture: %s\n", SDL_GetError());
	}
	else {
		// Set the blend mode for the texture
		SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
	}

	SDL_FreeSurface(surface); // No longer need the surface

	return texture;
}

void setup() {

	srand(time(NULL));
	last_switch_time = SDL_GetTicks(); // Initialize with the current time
	
	//Lobby
	start_button_texture = load_texture("Assets/Lobby/Button1-Play.png", renderer);
	setting_button_texture = load_texture("Assets/Lobby/Button2-Setting.png", renderer);
	collection_button_texture = load_texture("Assets/Lobby/Button3-Collection.png", renderer);
	shop_button_texture = load_texture("Assets/Lobby/Button4-Shop.png", renderer);
	menu_background_texture = load_texture("Assets/Lobby/main_menu_background.png", renderer);

	//Pasue menu
	resume_button_texture = load_texture("Assets/Pause_menu/Resume_button.png", renderer);
	exit_button_texture = load_texture("Assets/Pause_menu/Exit_button.png", renderer);

	//Result
	game_over_texture = load_texture("Assets/Result/game_over/game_over.png", renderer);
	congrates_texture = load_texture("Assets/Result/congrates/congrates.png", renderer);
	retry_texture = load_texture("Assets/Result/game_over/retry.png", renderer);
	back_to_menu_texture = load_texture("Assets/Result/game_over/back_to_menu.png", renderer);

	gameplay_setup();
}

void gameplay_setup() {

	//Set up Main_character stat
	Main_character.x = MAP_WIDTH / 2;
	Main_character.y = MAP_HEIGHT / 2;
	Main_character.width = 90;
	Main_character.height = 90;
	Main_character.movement_speed = 300.0f;
	Main_character.health = 100.0f; // maximum health is 100
	Main_character.type = 1; // Boy or girl

	//Load texture.
	if (Main_character.type == 1) {

		Main_character.texture[0] = load_texture("Assets/Main_character/Female_chef_1.png", renderer);
		Main_character.texture[1] = load_texture("Assets/Main_character/Female_chef_2.png", renderer);

	}
	else {

		Main_character.texture[0] = load_texture("Assets/Main_character/Male_chef_1.png", renderer);
		Main_character.texture[1] = load_texture("Assets/Main_character/Male_chef_2.png", renderer);

	}


	map_texture = load_texture("Assets/Map/Map1.png", renderer);

	camera.x = Main_character.x - WINDOW_WIDTH / 2;
	camera.y = Main_character.y - WINDOW_HEIGHT / 2;
}

void reset_game_state() {

	last_frame_time = 0;
	delta_time = 0.0f;
	lastPeriodicCall = SDL_GetTicks() - periodicInterval;
	waveIndex = 0;

	// Reset player state
	move_up = FALSE;
	move_down = FALSE;
	move_left = FALSE;
	move_right = FALSE;
	facing_left = 0;
	render_motion = true;
	last_switch_time = 0;

	// Reset enemies, stages, and attacks
	// Make sure to properly deallocate any dynamic memory if necessary before resetting

	for(int i = 0; i < MAX_ENEMIES_STAGE1; ++i) {
		// Free any dynamically allocated memory (if any) here
		// For example, if Enemies[i].name is a dynamically allocated string:
		// free(Enemies[i].name);

		memset(&Enemies[i], 0, sizeof(Enemies[i]));
	}
	gameplay_setup();
}

//Each type of Enemy stat initialize
void initialize_enemies(SDL_Renderer* renderer) {
	type[0].width = 50;
	type[0].height = 50;
	type[0].movement_speed = 90;
	type[0].health = 20;
	type[0].atk = 10;
	type[0].texture = load_texture("Assets/Enemy/Enemy_egg.png", renderer);
	
	type[1].width = 50;
	type[1].height = 50;
	type[1].movement_speed = 150;
	type[1].health = 150;
	type[1].atk = 30;
	type[1].texture = load_texture("Assets/Enemy/Enemy_chicken.png", renderer);

	//---------------------------------------
	type[2].width = 50;
	type[2].height = 50;
	type[2].movement_speed = 150;
	type[2].health = 100;
	type[2].atk = 20;
	type[2].texture = load_texture("Assets/Enemy/Enemy_bell_pepper.png", renderer);
	//---------------------------------------------
	type[3].width = 300;
	type[3].height = 300;
	type[3].movement_speed = 100;
	type[3].health = 300;
	type[3].atk = 40;
	type[3].texture = load_texture("Assets/Enemy/Miniboss_chicken.png", renderer);
	
}

//initialize enemy per wave.
void initialize_stage1_enemies() {

	stage1.waves[0].Enemy_count[0] = 50; 
	stage1.waves[0].Enemy_count[1] = 20;
	stage1.waves[0].Enemy_count[2] = 30;

	stage1.waves[1].Enemy_count[0] = 70;
	stage1.waves[1].Enemy_count[1] = 40;
	stage1.waves[1].Enemy_count[2] = 40;

	stage1.waves[2].Enemy_count[0] = 70;
	stage1.waves[2].Enemy_count[1] = 30;
	stage1.waves[2].Enemy_count[3] = 1;

	stage1.waves[3].Enemy_count[0] = 90;
	stage1.waves[3].Enemy_count[1] = 50;
	stage1.waves[3].Enemy_count[2] = 30;

	stage1.waves[4].Enemy_count[0] = 90;
	stage1.waves[4].Enemy_count[1] = 50;
	stage1.waves[4].Enemy_count[2] = 40;

}

//Spawn a wave of enemy
void spawn_wave(int waveIndex) {
	Wave wave = stage1.waves[waveIndex];
	for (int typeIndex = 0; typeIndex < MAX_ENEMY_TYPE; ++typeIndex) { //loop for amount of type
		int count = wave.Enemy_count[typeIndex];
		for (int i = 0; i < count; ++i) { //loop for amount of each type
			// Find a free spot in the enemies array
			for (int j = 0; j < MAX_ENEMIES_STAGE1; ++j) {
				if (!Enemies[j].isActive) {
					Enemies[j].type = typeIndex;
					Enemies[j].isActive = 1;
					Enemies[j].currentHealth = type[typeIndex].health;
					
					randPos = rand() % 8; //Random postion for enemy's spawn
						
					Enemies[j].rect.x = spawn_position[randPos].x;
					Enemies[j].rect.y = spawn_position[randPos].y;
					Enemies[j].rect.w = type[typeIndex].width;
					Enemies[j].rect.h = type[typeIndex].height;
					break; // Break the loop after setting up an enemy
				}
			}
		}
	}
}


void update_enemies(float delta_time) {
	for (int i = 0; i < MAX_ENEMIES_STAGE1; ++i) {
		if (Enemies[i].isActive) {
			float dx = Main_character.x - (Enemies[i].rect.x - Enemies[i].rect.w / 2);
			float dy = Main_character.y - (Enemies[i].rect.y - Enemies[i].rect.h / 2);
			float distance = sqrt(dx * dx + dy * dy);

			// Avoid division by zero
			if (distance == 0) continue;

			float dx_normalized = dx / distance;
			float dy_normalized = dy / distance;

			// Get the enemy's movement speed
			float speed = type[Enemies[i].type].movement_speed;

			// Update enemy position
			Enemies[i].rect.x += dx_normalized * speed * delta_time;
			Enemies[i].rect.y += dy_normalized * speed * delta_time;

			for (int j = 0; j < MAX_ENEMIES_STAGE1; ++j) {
				if (i != j && Enemies[j].isActive) {
					SDL_Rect rect1 = Enemies[i].rect;
					SDL_Rect rect2 = Enemies[j].rect;

					// Calculate the intersection rectangle
					SDL_Rect intersection;
					if (SDL_IntersectRect(&rect1, &rect2, &intersection)) {
						int overlapWidth = intersection.w;
						int overlapHeight = intersection.h;

						// Calculate 50% of each enemy's size
						int allowedOverlapWidth = rect1.w / 2;
						int allowedOverlapHeight = rect1.h / 2;

						// Check if the overlap exceeds 50% of the size
						if (overlapWidth > allowedOverlapWidth || overlapHeight > allowedOverlapHeight) {
							// Resolve the overlap by adjusting positions
							// This is a simple resolution strategy and may need refinement
							if (rect1.x < rect2.x) {
								Enemies[i].rect.x -= overlapWidth / 20;
								Enemies[j].rect.x += overlapWidth / 20;
							} else {
								Enemies[i].rect.x += overlapWidth / 20;
								Enemies[j].rect.x -= overlapWidth / 20;
							}

							if (rect1.y < rect2.y) {
								Enemies[i].rect.y -= overlapHeight / 20;
								Enemies[j].rect.y += overlapHeight / 20;
							} else {
								Enemies[i].rect.y += overlapHeight / 20;
								Enemies[j].rect.y -= overlapHeight / 20;
							}
						}
					}
				}
			}
		}
	}
}

void render_enemies(SDL_Renderer* renderer) {
	for (int i = 0; i < MAX_ENEMIES_STAGE1; ++i) {
		if (Enemies[i].isActive) {

			SDL_Rect screen_rect = {
				Enemies[i].rect.x - (int)camera.x,
				Enemies[i].rect.y - (int)camera.y,
				Enemies[i].rect.w,
				Enemies[i].rect.h
			};
			SDL_RenderCopy(renderer, type[Enemies[i].type].texture, NULL, &screen_rect);
			//For debug
			//SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255); // Blue for enemy hitbox
			//SDL_RenderDrawRect(renderer, &screen_rect);

		}
	}
}

void check_collision_and_apply_damage(float delta_time) {
	// Assuming the Main_character's rect is set up like this
	SDL_Rect Main_characterRect = { Main_character.x, Main_character.y, Main_character.width, Main_character.height };

	for (int i = 0; i < MAX_ENEMIES_STAGE1; ++i) {
		if (Enemies[i].isActive) {
			// Check for collision between Main_character and the active enemy
			if (SDL_HasIntersection(&Main_characterRect, &Enemies[i].rect)) {
				// Collision detected, Main_character takes damage from enemy's attack power
				float damage = type[Enemies[i].type].atk * delta_time;
				Main_character.health -= damage;
				if (Main_character.health < 0) Main_character.health = 0; // Prevent health from dropping below zero

				if (Main_character.health == 0) {
					gameState = GAME_STATE_LOSE;
				}

			}
		}
	}
}

//Attack
void initialize_attacks(void) {
	for (int i = 0; i < MAX_ATTACKS; ++i) {
		Main_character.attacks[i].isHave = false; // Initially, most attacks are inactive
		// Initialize other fields as needed...
	}

	// Setup the first attack
	Main_character.attacks[0].isActive = false;
	Main_character.attacks[0].isRender = false;
	Main_character.attacks[0].isHave = true;
	Main_character.attacks[0].cooldown = 2000; // Example: 2 seconds
	Main_character.attacks[0].lastAttackTime = 0;
	Main_character.attacks[0].lastTimeRender = 0;
	Main_character.attacks[0].damage = 50; // Example damage
	Main_character.attacks[0].area = (SDL_Rect){0, 0, 200, 300}; // Set size, position is dynamic
	Main_character.attacks[0].vfxTexture = load_texture("Assets/Attack_VFX/Attack3.png", renderer);

	Main_character.haveAttacks = 1; // Main_character starts with one active attack
}

void updated_attacks(Uint32 currentTime) {
	
	for (int i = 0; i < Main_character.haveAttacks; ++i) {
		AutoAttack* attack = &Main_character.attacks[i];
		if (attack->isHave && !attack->isActive && (currentTime - attack->lastAttackTime >= attack->cooldown)) {
			// Set the attack as active
			attack->isActive = true;
			attack->isRender = true;

			// Record the current time as the last attack time
			attack->lastAttackTime = currentTime;

			// Determine the attack area's X position based on facing direction
			if (facing_left) {
				// Attack spawns to the left of the Main_character, aligning its right edge with Main_character's center
				attack->area.x = Main_character.x - (attack->area.w - (Main_character.width / 2));
			}
			else {
				// Attack spawns to the right of the Main_character, starting from Main_character's center
				attack->area.x = Main_character.x + (Main_character.width / 2);
			}

			// Vertically center the attack area relative to the Main_character
			attack->area.y = Main_character.y - (attack->area.h - Main_character.height) / 2;

		}

		// Optionally, deactivate the attack after a short duration
		// This is useful if your attack should "expire" even if it hasn't hit anything
		if (attack->isActive && (currentTime - attack->lastAttackTime > ATTACK_DURATION)) {
			attack->isActive = false;
		}
		if (currentTime - attack->lastAttackTime > RENDER_DURATION) {
			attack->isRender = false;
		}
	}
}

void render_attacks() {

	Uint32 currentTime = SDL_GetTicks();

	/*For debug------------------------------------------------------------------------ -
	if (Main_character.attacks[0].isActive && (currentTime - Main_character.attacks[0].lastAttackTime <= 100)) {
		// Adjust the position of the debug visual for the attack area relative to the camera
		SDL_Rect debugAttackArea = {
			Main_character.attacks[0].area.x - camera.x, // Adjust X position relative to camera
			Main_character.attacks[0].area.y - camera.y, // Adjust Y position relative to camera
			Main_character.attacks[0].area.w, // Width of the attack area
			Main_character.attacks[0].area.h  // Height of the attack area
		};

		// Set the draw color to red for high visibility
		SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);

		//Draw the debug visual as a solid rectangle
		SDL_RenderFillRect(renderer, &debugAttackArea);
	}
	*/
	

	if (Main_character.attacks[0].isRender && (currentTime - Main_character.attacks[0].lastAttackTime <= 200)) { // Display VFX for a short duration
		// Adjust the attack area position relative to the camera
		SDL_Rect vfxPosition = {
			Main_character.attacks[0].area.x - camera.x, // Adjust X position relative to camera
			Main_character.attacks[0].area.y - camera.y, // Adjust Y position relative to camera
			Main_character.attacks[0].area.w, // Width of the VFX
			Main_character.attacks[0].area.h  // Height of the VFX
		};

		// Determine the flip state based on the character's facing direction
		SDL_RendererFlip flipType = facing_left ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;

		// Render the VFX texture within the adjusted position, with possible horizontal flip
		SDL_RenderCopyEx(renderer, Main_character.attacks[0].vfxTexture, NULL, &vfxPosition, 0.0, NULL, flipType);


	}

}

void apply_attack_damage_to_enemies() {
	
	Uint32 currentTime = SDL_GetTicks();

	for (int attackIndex = 0; attackIndex < Main_character.haveAttacks; ++attackIndex) {
		AutoAttack* attack = &Main_character.attacks[attackIndex];

		// Skip if the attack is not active or the character doesn't have this attack
		if (!attack->isHave || !attack->isActive) {
			continue;
		}

		// Check if the attack's active duration has passed (assuming a fixed short duration for demonstration)
		if (currentTime - attack->lastAttackTime > 100) { // 100 ms for example
			attack->isActive = false;
			continue;
		}

		// Assuming you have a function or logic here to determine if enemies are within the attack area
		// This could involve iterating over all enemies and checking collision with the attack->area
		for (int enemyIndex = 0; enemyIndex < MAX_ENEMIES_STAGE1; ++enemyIndex) {
			Enemy* enemy = &Enemies[enemyIndex];

			if (!enemy->isActive) continue; // Skip inactive enemies

			SDL_Rect enemyHitbox = { enemy->rect.x, enemy->rect.y, enemy->rect.w, enemy->rect.h };
			SDL_Rect attackArea = { attack->area.x, attack->area.y, attack->area.w, attack->area.h };

			// Check if the attack area intersects with the enemy hitbox
			if (SDL_HasIntersection(&attackArea, &enemyHitbox)) {
				// Apply damage to the enemy
				int damage = attack->damage;
				enemy->currentHealth -= damage;
				enemy->last_damage_taken = damage;
				enemy->damage_display_timer = SDL_GetTicks() + 500;

				// Log or handle enemy defeat if health drops below zero
				if (enemy->currentHealth <= 0) {
					enemy->isActive = false; // Enemy defeated
					// Additional logic for handling defeated enemy (e.g., scoring)
					killed_enemy++;
					if (enemy->type == 3 && enemy -> isActive == 0) {
						gameState = GAME_STATE_WIN;
					}
				}

				
			}
		}
	}
}

void render_enemy_damage(SDL_Renderer* renderer) {

	TTF_Font* font = TTF_OpenFont("Assets/Font/PixeloidSans.ttf", 24);
	SDL_Color color = { 200, 0, 0 };

	if (!font) {
		printf("Failed to load font: %s\n", TTF_GetError());
		// Handle error (e.g., use a fallback font or exit)
	}

	for (int i = 0; i < MAX_ENEMIES_STAGE1; ++i) {
		Enemy* enemy = &Enemies[i];
		if (!enemy->isActive || SDL_GetTicks() > enemy->damage_display_timer) {
			continue;
		}

		char damageText[12];
		sprintf_s(damageText, sizeof(damageText), "%d", enemy->last_damage_taken);

		SDL_Surface* surface = TTF_RenderText_Solid(font, damageText, color);

		if (surface) {
			SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
			int textWidth = surface->w;
			int textHeight = surface->h;

			SDL_Rect renderQuad = { enemy->rect.x - camera.x, enemy->rect.y - camera.y - 20, textWidth, textHeight }; // Position above the enemy
			SDL_RenderCopy(renderer, texture, NULL, &renderQuad);

			SDL_FreeSurface(surface);
			SDL_DestroyTexture(texture);

		}
		else {
			fprintf(stderr, "Failed to create surface: %s\n", TTF_GetError());
		}
	}

	TTF_CloseFont(font);
}


void update_camera() {
	camera.x = Main_character.x - WINDOW_WIDTH / 2;
	camera.y = Main_character.y - WINDOW_HEIGHT / 2;

	// Clamp camera to the edges of the map
	if (camera.x < 0) camera.x = 0;
	if (camera.y < 0) camera.y = 0;
	if (camera.x > MAP_WIDTH - camera.width) camera.x = MAP_WIDTH - camera.width;
	if (camera.y > MAP_HEIGHT - camera.height) camera.y = MAP_HEIGHT - camera.height;
}

void gameplay_update(float delta_time) {

	Uint32 currentTime = SDL_GetTicks();

	// Process auto-attack trigger
	updated_attacks(currentTime);
	update_enemies(delta_time);
	updated_attacks(currentTime);
	apply_attack_damage_to_enemies();
	check_collision_and_apply_damage(delta_time);

	//Main_character movement
	if (move_up) Main_character.y -= Main_character.movement_speed * delta_time;
	if (move_down) Main_character.y += Main_character.movement_speed * delta_time;
	if (move_left) Main_character.x -= Main_character.movement_speed * delta_time;
	if (move_right) Main_character.x += Main_character.movement_speed * delta_time;

	// Updated boundary checks for a 1920x1080 map
	if (Main_character.x < 0) Main_character.x = 0;
	if (Main_character.y < 0) Main_character.y = 0;
	if (Main_character.x + Main_character.width > MAP_WIDTH) Main_character.x = MAP_WIDTH - Main_character.width; // Use 1920 for the new map width
	if (Main_character.y + Main_character.height > MAP_HEIGHT) Main_character.y = MAP_HEIGHT - Main_character.height; // Use 1080 for the new map height


	// Update camera position to follow the ball
	update_camera();

}

void render_health_bar(SDL_Renderer* renderer, float health, float max_health, int x, int y, int width, int height) {
	// Background of the health bar (usually red or black)
	SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Red
	SDL_Rect bg_rect = { x, y, width, height };
	SDL_RenderFillRect(renderer, &bg_rect);

	// Foreground of the health bar showing current health
	float health_percentage = health / max_health;
	SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); // Green
	SDL_Rect fg_rect = { x, y, (int)(width * health_percentage), height };
	SDL_RenderFillRect(renderer, &fg_rect);
}

void menu_render() {

	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Optional, depending on if your map covers the whole screen
	SDL_RenderClear(renderer);


		SDL_Rect menu_background_rect = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
		SDL_RenderCopy(renderer, menu_background_texture, NULL, &menu_background_rect);

		SDL_Rect start_button_rect = { 780, 550, 360, 98 };
		SDL_RenderCopy(renderer, start_button_texture, NULL, &start_button_rect);

		SDL_Rect setting_button_rect = { 784, 675, 353, 88 };
		SDL_RenderCopy(renderer, setting_button_texture, NULL, &setting_button_rect);

		SDL_Rect collection_button_rect = { 784, 788, 353, 88 };
		SDL_RenderCopy(renderer, collection_button_texture, NULL, &collection_button_rect);

		SDL_Rect shop_button_rect = { 784, 901, 353, 88 };
		SDL_RenderCopy(renderer, shop_button_texture, NULL, &shop_button_rect);

	
	SDL_RenderPresent(renderer);

}

void gameplay_render() {
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Optional, depending on if your map covers the whole screen
	SDL_RenderClear(renderer);

	

		if (map_texture) {
			SDL_Rect srcRect = {
				(int)camera.x,
				(int)camera.y,
				camera.width,
				camera.height
			};
			SDL_Rect destRect = {
				0, 0, camera.width, camera.height
			};

			// Render the visible portion of the map
			SDL_RenderCopy(renderer, map_texture, &srcRect, &destRect);
		}

		Uint32 current_time = SDL_GetTicks();
		if (current_time - last_switch_time >= 250) { // Check if 0.5 seconds have passed
			render_motion = !render_motion; // Toggle the motion flag
			last_switch_time = current_time; // Reset the switch time
		}

		// Decide which texture to use based on render_motion
		SDL_Texture* texture_to_render = render_motion ? Main_character.texture[0] : Main_character.texture[1];

		SDL_Rect Main_character_rect = {
			(int)(Main_character.x - camera.x),
			(int)(Main_character.y - camera.y),
			(int)Main_character.width,
			(int)Main_character.height
		};
		SDL_RendererFlip flip = facing_left ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
		SDL_RenderCopyEx(renderer, texture_to_render, NULL, &Main_character_rect, 0, NULL, flip);


		//Render enemies
		render_enemies(renderer);

		//render damage
		render_enemy_damage(renderer);

		// Render the attack VFX if active and within the display window
		render_attacks();



		int health_bar_width = 1920;
		int health_bar_height = 40;
		int health_bar_x = 0; // Top left corner of the health bar
		int health_bar_y = 0; // Adjusted to be at the bottom of the window
		render_health_bar(renderer, Main_character.health, 100.0f, health_bar_x, health_bar_y, health_bar_width, health_bar_height);

	SDL_RenderPresent(renderer);
}

void game_lose_state_render() {

	
	SDL_Rect game_over_rect = { 135, 200, 1690, 451 };
	SDL_RenderCopy(renderer, game_over_texture, NULL, &game_over_rect);

	SDL_Rect back_to_menu_button_rect = { 630, 675, 701, 88 };
	SDL_RenderCopy(renderer, back_to_menu_texture, NULL, &back_to_menu_button_rect);

	SDL_RenderPresent(renderer);
}

void game_win_state_render() {

	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 5);// 128 is the alpha value for semi-transparent
	SDL_Rect rect = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
	SDL_RenderFillRect(renderer, &rect);

	SDL_Rect congrate_rect = { 58, 200, 1843, 296 };
	SDL_RenderCopy(renderer, congrates_texture, NULL, &congrate_rect);

	SDL_Rect back_to_menu_button_rect = { 630, 600, 701, 88 };
	SDL_RenderCopy(renderer, back_to_menu_texture, NULL, &back_to_menu_button_rect);
	
	SDL_RenderPresent(renderer);
}

void pause_render() {
	// Don't clear the screen with SDL_RenderClear(renderer);
	
   // Render a semi-transparent black rectangle over the entire screen
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 5);// 128 is the alpha value for semi-transparent
	SDL_Rect rect = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
	SDL_RenderFillRect(renderer, &rect);

	// Render pause menu elements here (if any), such as "Resume" and "Exit" buttons
	// Adjust position and sizes as needed
	SDL_Rect resume_button_rect = { 785, 320, 350, 150 }; // Placeholder positions
	SDL_Rect exit_button_rect = { 785, 600, 350, 150 }; // Placeholder positions

	SDL_RenderCopy(renderer, resume_button_texture, NULL, &resume_button_rect);
	SDL_RenderCopy(renderer, exit_button_texture, NULL, &exit_button_rect);



	SDL_RenderPresent(renderer);
}

//Control framerate( 60 FPS )
void cap_framerate(int* last_frame_time, float* delta_time) {

	int time_to_wait = FRAME_TARGET_TIME - (SDL_GetTicks() - *last_frame_time);
	if (time_to_wait > 0 && time_to_wait <= FRAME_TARGET_TIME) {
		SDL_Delay(time_to_wait);
	}

	*delta_time = (SDL_GetTicks() - *last_frame_time) / 1000.0f;
	*last_frame_time = SDL_GetTicks();
}

void destroy_window() {
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	if (map_texture) {
		SDL_DestroyTexture(map_texture);
	}
	SDL_DestroyTexture(resume_button_texture);
	SDL_DestroyTexture(exit_button_texture);

	AudioManager_Cleanup();
	SDL_Quit();
}