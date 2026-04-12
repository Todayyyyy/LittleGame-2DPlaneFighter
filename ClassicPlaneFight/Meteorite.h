#pragma once
#include "GameEntity.h"
#include "GameManager.h" 

class Meteorite : public GameEntity
{
private:
    float _scale; 

public:
    // ¹¹Ôìº¯Êý
    Meteorite(int hp, float startX, float scale)
        : GameEntity(startX, 10.0f * scale, 10.0f * scale, 10.0f * scale, hp, 0.01f)
    {
        _scale = scale;

        SDL_Log("Spawned a meteorite at X=%.2f Y=%.2f with scale=%.2f", this->x, this->y, _scale);
    }

    void Update() override
    {
        if (_isDead) return;
        this->y += _speed;
        if (this->y > GameManager::GetInstance().GetScreenHeight())
        {
            _isDead = true;
            SDL_Log("Meteorite out of bounds, marked as dead.");
        }
    }
};