#pragma once


enum StatID
{
	STAT_BRAVERY,
	STAT_FAITH,
	STAT_MOVE,
	STAT_JUMP,
	STAT_SPEED,
	STAT_ATTACK,
	STAT_EVASION,
	STAT_MAX_HP,
	STAT_MAX_MP,
	NUM_STATS
};

class Stats
{
public:
	Stats();

	int m_stats[NUM_STATS];

	static Stats CalculateRandomStatsInRange(const Stats& minStats, const Stats& maxStats);

	Stats& operator=(const Stats& statsToAssign);
	Stats operator+(const Stats& statsToAdd);
	Stats& operator+=(const Stats& statsToAdd);
	int& operator [](StatID statIndex);
	int operator [](StatID statIndex) const;
};



inline Stats& Stats::operator=(const Stats& statsToAssign)
{
	m_stats[STAT_BRAVERY] = statsToAssign[STAT_BRAVERY];
	m_stats[STAT_FAITH] = statsToAssign[STAT_FAITH];
	m_stats[STAT_ATTACK] = statsToAssign[STAT_ATTACK];
	m_stats[STAT_EVASION] = statsToAssign[STAT_EVASION];
	m_stats[STAT_SPEED] = statsToAssign[STAT_SPEED];
	m_stats[STAT_MOVE] = statsToAssign[STAT_MOVE];
	m_stats[STAT_JUMP] = statsToAssign[STAT_JUMP];

	m_stats[STAT_MAX_HP] = statsToAssign[STAT_MAX_HP];
	m_stats[STAT_MAX_MP] = statsToAssign[STAT_MAX_MP];

	return *this;
}

inline Stats Stats::operator+(const Stats& statsToAdd)
{
	Stats outputStats = *this;

	outputStats[STAT_BRAVERY] += statsToAdd[STAT_BRAVERY];
	outputStats[STAT_FAITH] += statsToAdd[STAT_FAITH];
	outputStats[STAT_ATTACK] += statsToAdd[STAT_ATTACK];
	outputStats[STAT_EVASION] += statsToAdd[STAT_EVASION];
	outputStats[STAT_SPEED] += statsToAdd[STAT_SPEED];
	outputStats[STAT_MOVE] += statsToAdd[STAT_MOVE];
	outputStats[STAT_JUMP] += statsToAdd[STAT_JUMP];

	outputStats[STAT_MAX_HP] += statsToAdd[STAT_MAX_HP];
	outputStats[STAT_MAX_MP] += statsToAdd[STAT_MAX_MP];

	return outputStats;
}

inline Stats& Stats::operator+=(const Stats& statsToAdd)
{
	m_stats[STAT_BRAVERY] += statsToAdd[STAT_BRAVERY];
	m_stats[STAT_FAITH] += statsToAdd[STAT_FAITH];
	m_stats[STAT_ATTACK] += statsToAdd[STAT_ATTACK];
	m_stats[STAT_EVASION] += statsToAdd[STAT_EVASION];
	m_stats[STAT_SPEED] += statsToAdd[STAT_SPEED];
	m_stats[STAT_MOVE] += statsToAdd[STAT_MOVE];
	m_stats[STAT_JUMP] += statsToAdd[STAT_JUMP];

	m_stats[STAT_MAX_HP] += statsToAdd[STAT_MAX_HP];
	m_stats[STAT_MAX_MP] += statsToAdd[STAT_MAX_MP];

	return *this;
}

inline int& Stats::operator [](StatID statIndex)
{
	return m_stats[statIndex];
}

inline int Stats::operator [](StatID statIndex) const
{
	return m_stats[statIndex];
}
