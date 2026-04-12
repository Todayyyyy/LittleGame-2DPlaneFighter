#pragma once
#include <SDL3/SDL.h>
#include <algorithm>

// 这是一个所有游戏活动对象的“基类”
class GameEntity : public SDL_FRect
{
protected: // protected：外部无法直接访问，但继承它的飞机、敌人可以直接修改
    int _hp;
    float _speed;
    float _centerPoint[2];
    bool _isDead;
public:
    // 带参数的构造函数，强迫子类出生时必须汇报这些数据
    GameEntity(float startX, float startY, float width, float height, int Hp, float speed)
    {
        this->x = startX;
        this->y = startY;
        this->w = width;
        this->h = height;
        _hp = Hp;
        _speed = speed;
        _isDead = false;
    }

    //拥有虚函数的基类，必须有一个虚析构函数，否则会导致内存泄漏！
    virtual ~GameEntity() = default;

    // ---------------- 通用接口 ----------------
    int GetHp() const { return _hp; }
    bool IsDead() const { return _isDead; }

    // 通用逻辑：受伤
    virtual void TakeDamage()
    {
        _hp --;
        if (_hp <= 0) {
            _hp = 0;
            _isDead = true;
        }
    }

    // 通用逻辑：AABB 碰撞检测
    bool CheckCollision(const GameEntity* other) const
    {
        if (!other || _isDead || other->_isDead) return false;
        return SDL_HasRectIntersectionFloat(this, other); // SDL3 自带的高效碰撞
    }

	// 核心功能实现：移动等等，子类可以重写这个函数来实现自己的更新逻辑
    virtual void Update()
    {

    }
};