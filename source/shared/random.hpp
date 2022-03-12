#ifndef TALE_GLOBALS_RANDOM_H
#define TALE_GLOBALS_RANDOM_H

#include <random>
#include <vector>
namespace tattletale
{
    class Random
    {
    public:
        Random();
        Random(uint32_t seed);
        void Seed(uint32_t seed);
        int GetInt(int min, int max);
        uint32_t GetUInt(uint32_t min, uint32_t max);
        float GetFloat(float min, float max);
        size_t PickIndex(const std::vector<float> &probability_distribution, bool all_zeroes_means_equal_chance = false);

    private:
        std::mt19937 rng_;
    };
} // namespace tattletale
#endif // TALE_GLOBALS_RANDOM_H