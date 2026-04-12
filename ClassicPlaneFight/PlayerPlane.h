#pragma once
#include "GameEntity.h"
#include "Gamemanager.h"
#include "Bullet.h"

class PlayerPlane : public GameEntity
{
private:
	Uint64 _lastFireTime = 0;
	Uint64 _fireInterval = 150;
public:
	// 构造函数：把参数传给父类 GameEntity 去初始化
	PlayerPlane(float startX, float startY, float width, float height)
		: GameEntity(startX, startY, width, height, 2, 0.05f)
	{
	}

	Bullet* PlayerUpdate(SDL_Gamepad* gamepad, const Uint64 now)
	{
		if (!gamepad || _isDead) return nullptr;

		//1.移动
		Sint16 axis_x = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTX);
		Sint16 axis_y = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTY);
		if (SDL_abs(axis_x) > 2000 || SDL_abs(axis_y) > 2000)
		{
			float normalized_x = axis_x / 32767.0f;
			float normalized_y = axis_y / 32767.0f;
			this->x += normalized_x * _speed;
			this->y += normalized_y * _speed;
			this->x = std::clamp(this->x, 0.0f, GameManager::GetInstance().GetScreenWidth() - this->w);
			this->y = std::clamp(this->y, 0.0f, GameManager::GetInstance().GetScreenHeight() - this->h);
		}

		//2.射击
		Sint16 triggerValue = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER);
		if (triggerValue > 10000 && (now - _lastFireTime >= _fireInterval)) {
			_lastFireTime = now;
			return new Bullet(this->x + (this->w / 2.0f), this->y + 5.0f);
		}
		return nullptr;
	}
};