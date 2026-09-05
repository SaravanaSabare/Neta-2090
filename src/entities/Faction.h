#pragma once

#include <string>
#include <unordered_map>

namespace neta::entities {

// Placeholder faction. Holds identity + a stub agenda string + a pairwise
// attitude table. Future faction simulation (agendas as data, diplomacy,
// resource pursuit) will extend this class, not replace it.
class Faction {
public:
    Faction(int id, std::string name, std::string agenda);

    int id() const { return m_id; }
    const std::string& name() const { return m_name; }
    const std::string& agenda() const { return m_agenda; }

    // Attitude toward another faction in [-100, 100]. Default 0 (neutral).
    void setRelation(int otherFactionId, int score) { m_relations[otherFactionId] = score; }
    int relation(int otherFactionId) const {
        const auto it = m_relations.find(otherFactionId);
        return it != m_relations.end() ? it->second : 0;
    }
    const std::unordered_map<int, int>& relations() const { return m_relations; }

private:
    int m_id;
    std::string m_name;
    std::string m_agenda;
    std::unordered_map<int, int> m_relations;
};

}  // namespace neta::entities
