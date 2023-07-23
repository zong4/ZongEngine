#include "UUID.hpp"

#include <random>

namespace zong
{
namespace core
{

static std::random_device                      RandomDevice;
static std::mt19937_64                         Engine(RandomDevice());
static std::uniform_int_distribution<uint64_t> UniformDistribution;

} // namespace core
} // namespace zong

zong::core::UUID::UUID() : _uuid(UniformDistribution(Engine))
{
}
