#include "Game/Stats.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Game/GameCommon.hpp"

Stats::Stats()
	: m_stats()
{

}

Stats Stats::CalculateRandomStatsInRange(const Stats& minStats, const Stats& maxStats)
{
	Stats newStats;

	newStats.m_stats[STAT_BRAVERY] = g_random.GetRandomIntInRange(minStats.m_stats[STAT_BRAVERY], maxStats.m_stats[STAT_BRAVERY]);
	newStats.m_stats[STAT_FAITH] = g_random.GetRandomIntInRange(minStats.m_stats[STAT_FAITH], maxStats.m_stats[STAT_FAITH]);
	newStats.m_stats[STAT_ATTACK] = g_random.GetRandomIntInRange(minStats.m_stats[STAT_ATTACK], maxStats.m_stats[STAT_ATTACK]);
	newStats.m_stats[STAT_SPEED] = g_random.GetRandomIntInRange(minStats.m_stats[STAT_SPEED], maxStats.m_stats[STAT_SPEED]);
	newStats.m_stats[STAT_EVASION] = g_random.GetRandomIntInRange(minStats.m_stats[STAT_EVASION], maxStats.m_stats[STAT_EVASION]);
	newStats.m_stats[STAT_MOVE] = g_random.GetRandomIntInRange(minStats.m_stats[STAT_MOVE], maxStats.m_stats[STAT_MOVE]);
	newStats.m_stats[STAT_JUMP] = g_random.GetRandomIntInRange(minStats.m_stats[STAT_JUMP], maxStats.m_stats[STAT_JUMP]);
	newStats.m_stats[STAT_MAX_HP] = g_random.GetRandomIntInRange(minStats.m_stats[STAT_MAX_HP], maxStats.m_stats[STAT_MAX_HP]);
	newStats.m_stats[STAT_MAX_MP] = g_random.GetRandomIntInRange(minStats.m_stats[STAT_MAX_MP], maxStats.m_stats[STAT_MAX_MP]);

	return newStats;
}
