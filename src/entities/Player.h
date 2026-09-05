#pragma once

#include "entities/Entity.h"

namespace neta::entities {

// The player avatar. Positions live in abstract "world units"; the renderer
// maps them to screen space, so gameplay code never depends on resolution.
class Player : public Entity {
public:
    Player();

    // Moves along dir for dt seconds. dir does not need to be normalized.
    void move(Vec2 dir, float dt);

    float speed() const { return m_speed; }

private:
    float m_speed = 20.0f;  // world units per second
};

}  // namespace neta::entities
