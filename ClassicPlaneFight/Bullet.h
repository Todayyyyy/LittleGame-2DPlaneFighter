#pragma once
#include "GameEntity.h"
#include "GameManager.h" 

class Bullet : public GameEntity
{
private:

public:
	Bullet(float startX, float startY)
		: GameEntity(startX, startY, 5, 5, 1, 0.1f)
	{
	}
    void Update() override
    {
        if (_isDead) return;
        this->y -= _speed;
        if (this->y < 0)
        {
            _isDead = true;
            SDL_Log("Bullet out of bounds, marked as dead.");
        }
    }

};