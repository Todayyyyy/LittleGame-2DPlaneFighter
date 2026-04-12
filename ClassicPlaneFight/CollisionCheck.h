#pragma once
#include <SDL3/SDL.h>

bool CheckCollision_AABB(const SDL_FRect* a, const SDL_FRect* b)
{
	return SDL_HasRectIntersectionFloat(a, b);
}