#pragma once

struct LvlAbilityKey {
    int16_t dwLevel;  // 等级
    int16_t dwJob;    // 职业
    bool operator==(LvlAbilityKey const& o) const noexcept {
        return dwLevel == o.dwLevel
            && dwJob == o.dwJob;
    }
    LvlAbilityKey(int16_t lvl, int16_t job):dwLevel(lvl),dwJob(job){}
};
struct LvlAbilityKeyHash {
    size_t operator()(LvlAbilityKey const& key) const noexcept {
        uint32_t v = (static_cast<uint32_t>(
            static_cast<uint16_t>(key.dwLevel)) << 16)
            | static_cast<uint16_t>(key.dwJob);
        return std::hash<uint32_t>{}(v);
    }
};
struct MapGateKey {
	MapGateKey(int map_id, int16_t x, int16_t y)
		: mapId(map_id),
		  x(x),
		  y(y)
	{
	}

	int mapId;
    int16_t x, y;

    bool operator==(const MapGateKey& other) const {
        return mapId == other.mapId && x == other.x && y == other.y;
    }
};

template <>
struct std::hash<MapGateKey> {
    size_t operator()(MapGateKey const& k) const noexcept {
        uint64_t v = (static_cast<uint64_t>(static_cast<uint32_t>(k.mapId)) << 32)
            | (static_cast<uint64_t>(static_cast<uint16_t>(k.x)) << 16)
            | static_cast<uint16_t>(k.y);
        return std::hash<uint64_t>{}(v);
    }
};

struct BuffKey {
    uint32_t id;
    uint8_t level;
    bool operator==(const BuffKey& other) const {
        return id == other.id && level == other.level;
    }
    BuffKey(uint32_t id, uint8_t level) :id(id), level(level) {}
};

struct BuffKeyHash {
    size_t operator()(const BuffKey& k) const {
        return (k.id << 8) | k.level;
    }
};