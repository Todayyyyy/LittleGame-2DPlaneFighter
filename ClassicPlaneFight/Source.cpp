/*
ClassicPlaneFight is a simple 2D game, developed by SDL3 and C++.
*/


#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */
#include <vector>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include "Gamemanager.h"
#include "GameEntity.h"
#include "PlayerPlane.h"
#include "Meteorite.h"
#include "Bullet.h"	
#include "CollisionCheck.h"


//初始化窗口和渲染器。
static SDL_Window* window = NULL;
static SDL_Renderer* renderer = NULL;
static SDL_Texture* texture = NULL;
static int texture_width = 0;
static int texture_height = 0;
static SDL_Gamepad* gamepad = NULL;
static PlayerPlane* player_plane = nullptr;

static const char* text = "Plug in a gamepad, please.";
//初始化游戏参数管理器
static GameManager& gameManager = GameManager::GetInstance();
// 记录上一次生成陨石的时间戳
Uint64 lastSpawnTime = 0;
// 生成间隔：2000毫秒（2秒）（可以加入动态抖动优化生成间隔）
const Uint64 SPAWN_INTERVAL_MS = 2000;
//初始化敌人和子弹容器
std::vector<Meteorite*> MeteoritesLists;
std::vector<Bullet*> BulletsLists;

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[])
{
	SDL_Surface* surface = NULL;
	char* png_path = NULL;

	// 初始化 SDL 的视频子系统 和 手柄子系统
	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
		SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}
	//创建窗口和渲染器
	if (!SDL_CreateWindowAndRenderer("examples/renderer/streaming-textures", gameManager.GetScreenWidth(), gameManager.GetScreenHeight(), SDL_WINDOW_RESIZABLE, &window, &renderer)) {
		SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}
	SDL_SetRenderLogicalPresentation(renderer, gameManager.GetScreenWidth(), gameManager.GetScreenHeight(), SDL_LOGICAL_PRESENTATION_LETTERBOX);

	//创建飞机的纹理
	SDL_asprintf(&png_path, "%splan.png", SDL_GetBasePath());  /* allocate a string of the full file path */
	surface = SDL_LoadPNG(png_path);
	if (!surface) {
		SDL_Log("Couldn't load png: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}
	SDL_free(png_path);
	texture_width = surface->w;
	texture_height = surface->h;
	texture = SDL_CreateTextureFromSurface(renderer, surface);
	if (!texture) {
		SDL_Log("Couldn't create static texture: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}
	SDL_DestroySurface(surface);

	//初始化玩家
	float x = (gameManager.GetScreenWidth() / 2.0f) - 25;
	float y = gameManager.GetScreenHeight() - 50.0f;
	float w = (float)texture_width;
	float h = (float)texture_height;
	player_plane = new PlayerPlane(
		x, y, w, h
	);

	//初始化时间
	lastSpawnTime = SDL_GetTicks();
	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
	//如果点击关闭窗口，结束程序。
	if (event->type == SDL_EVENT_QUIT) {
		return SDL_APP_SUCCESS;
	}
	else if (event->type == SDL_EVENT_GAMEPAD_ADDED) {
		/* this event is sent for each hotplugged gamepad, but also each already-connected gamepad during SDL_Init(). */
		if (gamepad == NULL) {  /* we don't have a stick yet and one was added, open it! */
			gamepad = SDL_OpenGamepad(event->gdevice.which);
			if (!gamepad) {
				SDL_Log("Failed to open gamepad ID %u: %s", (unsigned int)event->gdevice.which, SDL_GetError());
			}
		}
	}
	else if (event->type == SDL_EVENT_GAMEPAD_REMOVED) {
		if (gamepad && (SDL_GetGamepadID(gamepad) == event->gdevice.which)) {
			SDL_CloseGamepad(gamepad);  /* our controller was unplugged. */
			gamepad = NULL;
		}
	}
	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate)
{
	//----------------------------
	//一、初始化
	//----------------------------

	//陨石比例随时间变大，最大是2倍。
	const Uint64 now = SDL_GetTicks();
	float Scale = 1.0f + (now / 100000.0f); // 100秒钟增长到2倍
	if (Scale < 1.5f) {
		Scale = 1.0f;
	}
	else if (Scale >= 1.5f && Scale < 2.0f) {
		Scale = 1.5f;
	}
	else if (Scale >= 2.0f) {
		Scale = 2.0f;
	}

	//----------------------------
	// 二、运算、生成
	// ----------------------------

	//1.陨石

	//生成陨石。
	if (now - lastSpawnTime > SPAWN_INTERVAL_MS + SDL_rand(1000)) {
		// 算出一个随机的 X 坐标 (保证在屏幕宽度范围内)
		float randomX = SDL_rand(gameManager.GetScreenWidth() - 10) * 1.0f; // 10 是陨石的宽度，确保它不会生成在屏幕外面
		// 造一颗新陨石，推入数组
		MeteoritesLists.push_back(new Meteorite(1, randomX, Scale));
		// 重置计时器，开始等下一个 2 秒
		lastSpawnTime = now;
	}
	//陨石更新逻辑
	for (auto* meteor : MeteoritesLists) {
		meteor->Update();
	}

	//2.玩家
	Bullet* newBullet = player_plane->PlayerUpdate(gamepad, now);

	//3.子弹
	if (newBullet) {
		BulletsLists.push_back(newBullet);
		// 【老兵提醒】：这里是触发音频的最佳时机！
		// AudioEngine::GetInstance().PostEvent("Play_Laser_SFX");
	}

	for (auto* bullet : BulletsLists) {
		bullet->Update();
	}

	//---------------------------
	// 3.碰撞检测
	//---------------------------

	//玩家和陨石
	for (auto& meteor : MeteoritesLists) {
		if (!meteor->IsDead() && CheckCollision_AABB(meteor, player_plane)) {
			player_plane->TakeDamage();
			meteor->TakeDamage();
		}
	}

	for (auto& bullet : BulletsLists) {
		if (bullet->IsDead()) continue;
		for (auto& meteor : MeteoritesLists) {
			if (meteor->IsDead()) continue;
			if (CheckCollision_AABB(bullet, meteor)) {
				bullet->TakeDamage(); 
				meteor->TakeDamage(); 
			}
		}
	}


	//----------------------------
	// 三、渲染
	//----------------------------

	//1.背景
	SDL_SetRenderDrawColor(renderer, 0, 0, 20, 255);
	SDL_RenderClear(renderer);
	if (!gamepad) {
		SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);  /* white, full alpha */
		SDL_RenderDebugText(renderer, gameManager.GetScreenWidth() / 2, gameManager.GetScreenHeight() / 2, "Please plug in a gamepad."); /* draw the text in the middle of the screen. */
	}

	//2.陨石
	SDL_SetRenderDrawColor(renderer, 139, 139, 139, 255);
	for (GameEntity* meteorite : MeteoritesLists) {

		if (!meteorite->IsDead()) {
			SDL_RenderFillRect(renderer, (const SDL_FRect*)meteorite);
		}
	}

	//3.玩家
	if (!player_plane->IsDead()) {
		SDL_RenderTexture(renderer, texture, NULL, player_plane);
	}


	//4.子弹
	SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255); // 黄色子弹
	for (auto* b : BulletsLists) {
		if (!b->IsDead()) {
			SDL_RenderFillRect(renderer, (const SDL_FRect*)b);
		}
	}

	SDL_RenderPresent(renderer);

	//----------------------------
	// 四、内存回收
	//----------------------------

	//1.陨石
	for (int i = (int)MeteoritesLists.size() - 1; i >= 0; --i) {
		if (MeteoritesLists[i]->IsDead()) {
			delete MeteoritesLists[i];
			MeteoritesLists.erase(MeteoritesLists.begin() + i);
		}
	}
	//2.子弹
	for (int i = (int)BulletsLists.size() - 1; i >= 0; --i) {
		if (BulletsLists[i]->IsDead()) {
			delete BulletsLists[i];
			BulletsLists.erase(BulletsLists.begin() + i);
		}
	}

	return SDL_APP_CONTINUE;

}

void SDL_AppQuit(void* appstate, SDL_AppResult result)
{
	SDL_DestroyTexture(texture);
	/* SDL will clean up the window/renderer for us. */
}