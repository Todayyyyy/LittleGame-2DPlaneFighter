#pragma once
#include <SDL3/SDL.h>

#pragma once
#include <SDL3/SDL.h>

class GameManager
{
private:
    // 隐藏构造函数，不允许随便 new 出来第二个
    GameManager()
    {
        _screenWidth = 640;
        _screenHeight = 960;
        _isGameOver = false;
        _textureSize = 150;
        _meteoriteScale = 1.0f;
    }

    // 禁用拷贝和赋值，确保只有一个
    GameManager(const GameManager&) = delete;
    GameManager& operator=(const GameManager&) = delete;

    // 全局数据存放在这里
    int _screenWidth;
    int _screenHeight;
    bool _isGameOver;
    int _textureSize; 
	float _meteoriteScale;

public:
    // 获取管家的唯一入口
    static GameManager& GetInstance()
    {
        // 静态局部变量：只会在第一次调用时被创建，且生命周期直到游戏关闭才结束
        static GameManager instance;
        return instance;
    }

    // --- 暴露给外面查询的接口 ---
    int GetScreenWidth() const { return _screenWidth; }
    int GetScreenHeight() const { return _screenHeight; }
    bool IsGameOver() const { return _isGameOver; }

    void SetGameOver(bool state) { _isGameOver = state; }
};