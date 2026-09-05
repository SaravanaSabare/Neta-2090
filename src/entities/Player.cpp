#include "entities/Player.h"

#include <cmath>

namespace neta::entities {

Player::Player()
    : Entity(0, "Player", {50.0f, 28.0f}) {}

void Player::move(Vec2 dir, float dt) {
    const float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
    if (len < 1e-6f || dt <= 0.0f) {
        return;
    }
    // Normalize only oversized input so analog-scale speeds still work later.
    const float norm = len > 1.0f ? len : 1.0f;
    m_pos.x += (dir.x / norm) * m_speed * dt;
    m_pos.y += (dir.y / norm) * m_speed * dt;
    // NOTE: world bounds are enforced by Simulation, not here, so entities
    // stay independent of world geometry.
}

}  // namespace neta::entities
