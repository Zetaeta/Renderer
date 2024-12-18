#include <random>

namespace Random
{

thread_local static std::mt19937 sGenerator(std::random_device{}());

float Range(float start, float end)
{
	return std::uniform_real_distribution<float>(start, end)(sGenerator);
}

float Normal(float mean, float stddev)
{
	return std::normal_distribution(mean, stddev)(sGenerator);
}

}
