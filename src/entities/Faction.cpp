#include "entities/Faction.h"

#include <utility>

namespace neta::entities {

Faction::Faction(int id, std::string name, std::string agenda)
    : m_id(id)
    , m_name(std::move(name))
    , m_agenda(std::move(agenda)) {}

}  // namespace neta::entities
