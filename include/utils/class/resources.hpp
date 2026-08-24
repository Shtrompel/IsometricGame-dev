#ifndef _GAME_RESOURCES
#define _GAME_RESOURCES

#include <array>
#include <cassert>
#include <iterator>
#include <map>
#include <string>

#include "utils/class/logger.hpp"
#include "utils/globals.hpp"
#include "utils/math.hpp"
#include "utils/utils.hpp"

typedef IdStrTMap<int, std::map> t_weights;

struct Resources
{
	static const int ORE = 0;
	static const int GEMS = 1;
	static const int BODIES = 2;
	static const int COUNT = 3;

	static const int CONST_INF = -1;

	std::map<size_t, int> resCount;

#define INF(res, indx) (res.get(indx) == -1)

	decltype(resCount)::iterator begin();
	decltype(resCount)::iterator end();
	decltype(resCount)::const_iterator begin() const;
	decltype(resCount)::const_iterator end() const;

	Resources(const Resources &x, t_weights *resWeight);
	Resources(std::initializer_list<int> lst);
	Resources() = default;

	static Resources all(int x, t_weights *resWeight);
	static Resources res_inf(t_weights *resWeight);
	static Resources res_empty(t_weights *resWeight);
	static Resources res_empty();
	static Resources res_true(t_weights *resWeight);

	int &operator[](const size_t i);
	int get(const size_t i) const;

	bool operator==(const Resources& other) const;
	bool operator!=(const Resources& other) const;
	Resources &operator=(const Resources &other);
	bool operator<(const Resources &other) const;
	bool operator<=(const Resources &other) const;
	Resources operator+(const Resources &c) const;
	Resources operator-(const Resources &c) const;

	template <typename U>
	Resources operator*(const U x) const
	{
		Resources ret;
		for (auto &itr : *this)
		{
			size_t i = itr.first;
			if (itr.second >= 0)
				ret[i] = (int)((U)itr.second * x);
			else
				ret[i] = -1;
		}
		return ret;
	}

	Resources &operator+=(const Resources &c);
		
	template <typename U>
	Resources &operator*=(const U x)
	{
		for (auto &itr : *this)
		{
			size_t i = itr.first;
			if (get(i) == -1)
				continue;
			operator[](i) = (int)((U)get(i) * x);
		}
		return *this;
	}

	Resources &operator-=(const Resources &c);

	float get_presentage_of(const Resources &c) const;
	float get_presentage_of(int cap, t_weights* weights) const;

	bool capped(const Resources &cap) const;
	bool affordable(const Resources &cost) const;
	bool empty() const;
	bool is_infinity() const;
	void clear();
	Resources to_bool() const;
	Resources reverse_bool(t_weights* weights) const;
	Resources reverse_bool(t_weights& weights) const;

	std::string to_string_weights(t_weights&);
	std::string to_string() const;
	std::string to_string(t_weights* weights) const;

	int weight(t_weights *resWeight) const;
	bool is_full(t_weights *resWeight, const int weightLimit) const;
	bool is_full(const Resources &cap) const;
	bool has_bool(const Resources &b) const;
	bool has_bool_all(const Resources& b) const;

	static bool use(Resources &src, Resources &cost);
	bool fill(Resources &in, const Resources &limit);

	static bool transfer(
		Resources &from,
		Resources &to,
		t_weights *weights,
		const int weightTransfer);

	static bool transfer(
		Resources &from,
		Resources &to,
		t_weights *weights,
		const int weightTransfer,
		const Resources &toLimit,
		const int toLimitWeight);

	static bool transfer(
		Resources &from,
		Resources &to,
		t_weights *weights,
		const int weightTransfer,
		const Resources &toLimit,
		const int toLimitWeight,
		const Resources &boolResource);

	Resources bool_pass(const Resources &other) const;

	static bool can_transfer_weight(
		const Resources &from,
		const Resources &to,
		t_weights *weights,
		const int toLimitWeight);

	static bool can_transfer_weight(
		const Resources &from,
		const Resources &to,
		t_weights *weights,
		const int toLimitWeight,
		const Resources &boolResource);

	static bool can_transfer_limit(
		const Resources &from,
		const Resources &to,
		const Resources &toLimit,
		t_weights *weights);

	static bool can_transfer_limit(
		const Resources &from,
		const Resources &to,
		const Resources &toLimit,
		const Resources &boolResource);

	static bool can_transfer(
		const Resources &from,
		const Resources &to,
		t_weights *weights,
		const int toLimitWeight,
		const Resources &toLimit);

	static bool can_transfer(
		const Resources &from,
		const Resources &to,
		t_weights *weights,
		const int toLimitWeight,
		const Resources &toLimit,
		const Resources &boolResource);
};

static const Resources NULL_RES = Resources();


#endif