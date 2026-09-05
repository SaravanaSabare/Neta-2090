#pragma once

#include <cstdint>
#include <string>

namespace neta::entities {

using EntityId = std::uint32_t;

struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;
};

// Minimal entity base. Deliberately small: future systems (inventory,
// health, resources) will be added as components/data on top, not by
// bloating this class.
class Entity {
public:
    Entity(EntityId id, std::string name, Vec2 pos = {})
        : m_id(id)
        , m_name(std::move(name))
        , m_pos(pos) {}
    virtual ~Entity() = default;

    EntityId id() const { return m_id; }
    const std::string& name() const { return m_name; }
    Vec2 position() const { return m_pos; }
    void setPosition(Vec2 pos) { m_pos = pos; }

protected:
    EntityId m_id;
    std::string m_name;
    Vec2 m_pos;
};

}  // namespace neta::entities
