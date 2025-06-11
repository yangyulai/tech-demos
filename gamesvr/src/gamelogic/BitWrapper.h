// BitWrapper.h
#pragma once
#include <bitset>
#include <cstdint>
#include <optional>
#include <unordered_map>

class BitWrapper {
public:
    BitWrapper() : bits_() {}

    void set(uint32_t pos, bool v = true) {
        bits_.set(pos, v);
    }
    void reset(uint32_t pos) {
        bits_.reset(pos);
    }
    void flip(uint32_t pos) {
        bits_.flip(pos);
    }
    bool get(uint32_t pos) const {
        return bits_.test(pos);
    }
    uint64_t value() const {
        return bits_.to_ullong();
    }
private:
    std::bitset<64> bits_;
};

class BitManager
{
public:
    BitManager() = default;
	void setBit(const std::string& name,uint32_t pos, bool v = true) {
		bits_[name]->set(pos, v);
	}
	bool getBit(const std::string& name, uint32_t pos) const {
		if (auto bit = FindBits(name))
		{
			bit->get(pos);
		}
		return false;
	}
	uint64_t value(const std::string& name) const {
		if (auto bit = FindBits(name))
		{
			bit->value();
		}
		return 0;
	}
private:
	BitWrapper* FindBits(const std::string& name) const
    {
		if (auto it = bits_.find(name);it!=bits_.end())
		{
			return it->second;
		}
		return nullptr;
	}
    std::unordered_map<std::string, BitWrapper*> bits_;
};