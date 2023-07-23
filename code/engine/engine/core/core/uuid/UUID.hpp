#pragma once

#include <cstddef>

namespace zong
{
namespace core
{

class UUID
{
private:
    uint64_t _uuid;

public:
    inline uint64_t uuid() const { return _uuid; }

public:
    UUID();
    UUID(uint64_t uuid) : _uuid(uuid) {}

    // inline operator uint64_t() const { return uuid(); }
};

} // namespace core
} // namespace zong

namespace std
{
template <typename T>
struct hash;

template <>
struct hash<zong::core::UUID>
{
    std::size_t operator()(zong::core::UUID const& uuid) const noexcept { return uuid.uuid(); }
};

} // namespace std
