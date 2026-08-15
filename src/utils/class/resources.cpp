#include "utils/class/resources.hpp"

// Iterators
decltype(Resources::resCount)::iterator Resources::begin()
{
	return resCount.begin();
}

decltype(Resources::resCount)::iterator Resources::end()
{
	return resCount.end();
}

decltype(Resources::resCount)::const_iterator Resources::begin() const
{
	return resCount.begin();
}

decltype(Resources::resCount)::const_iterator Resources::end() const
{
	return resCount.end();
}

// Constructors
Resources::Resources(const Resources &x, t_weights *resWeight)
{
	resCount[ORE] = 0;
	resCount[GEMS] = 0;
	resCount[BODIES] = 0;
}

Resources::Resources(std::initializer_list<int> lst)
{
	auto i = 0;
	auto j = lst.begin();
	for (; j != lst.end(); j++)
		resCount[i++] = *j;
}

// Static factory methods
Resources Resources::all(int x, t_weights *resWeight)
{
	Resources ret;
	if (!resWeight)
		return ret;
	t_weights &weights = *resWeight;
	for (auto &w : weights)
	{
		auto &id = w.first;
		ret.resCount[id] = x;
	}
	return ret;
}

Resources Resources::res_inf(t_weights *resWeight)
{
	return all(-1, resWeight);
}

Resources Resources::res_empty(t_weights *resWeight)
{
	return all(0, resWeight);
}

Resources Resources::res_empty()
{
	return Resources({});
}

Resources Resources::res_true(t_weights *resWeight)
{
	return all(1, resWeight);
}

// Accessors
int &Resources::operator[](const size_t i)
{
	return resCount[i];
}

int Resources::get(const size_t i) const
{
	return resCount.count(i) ? resCount.at(i) : 0;
}

// Comparison operators
bool Resources::operator==(const Resources& other) const
{
	// First check if all resources in this object are 
	// available in the second
	for (const std::pair<const size_t, int>& resource : resCount)
	{
		if (resource.second == 0)
			continue;
		if (!other.resCount.count(resource.first))
			return false;
		if (other.resCount.at(resource.first) !=
			resource.second)
			return false;
		// continue
	}

	// Then do the opposite
	for (const std::pair<const size_t, int>& resource : other.resCount)
	{
		if (resource.second == 0)
			continue;
		if (!this->resCount.count(resource.first))
			return false;
		if (this->resCount.at(resource.first) !=
			resource.second)
			return false;
		// continue
	}

	return true;
}

bool Resources::operator!=(const Resources& other) const
{
	return ! ( *this == other );
}

Resources &Resources::operator=(const Resources &other)
{
	this->resCount = other.resCount;
	return *this;
}

bool Resources::operator<(const Resources &other) const
{
	for (auto o : other)
	{
		if (get(o.first) >= o.second)
			return false;
	}
	for (auto o : *this)
	{
		if (o.second >= other.get(o.first))
			return false;
	}
	return true;
}

bool Resources::operator<=(const Resources &other) const
{
	for (auto o : other)
	{
		if (get(o.first) > o.second)
			return false;
	}
	for (auto o : *this)
	{
		if (o.second > other.get(o.first))
			return false;
	}
	return true;
}

// Arithmetic operators
Resources Resources::operator+(const Resources &c) const
{
	Resources ret = *this;
	for (auto i : c)
	{
		ret[i.first] += i.second;
	}
	return ret;
}

Resources Resources::operator-(const Resources &c) const
{
	Resources ret = *this;
	for (auto i : c)
	{
		ret[i.first] -= i.second;
	}
	return ret;
}

Resources &Resources::operator+=(const Resources &c)
{
	for (auto &itr : c)
	{
		size_t i = itr.first;
		if (get(i) == -1 || c.get(i) == -1)
			continue;
		operator[](i) += c.get(i);
	}
	return *this;
}

Resources &Resources::operator-=(const Resources &c)
{
	for (auto &itr : c)
	{
		size_t i = itr.first;
		if (get(i) == -1 || c.get(i) == -1)
			continue;
		(*this)[i] -= c.get(i);
	}
	return *this;
}

// Percentage / capacity
float Resources::get_presentage_of(const Resources &c) const
{
	float sum = 0.0f;
	int counter = 0;
	for (auto &itr : c)
	{
		size_t i = itr.first;
		if (c.get(i) == 0)
			continue;
		++counter;
		sum += (float)get(i) / c.get(i);
	}
	return counter ? (sum / (float)counter) : 0.f;
}

float Resources::get_presentage_of(int cap, t_weights* weights) const
{
	assert(weights);
	
	int weightSum = 0;
	for (auto& itr : this->resCount)
	{
		size_t i = itr.first;
		if (get(i) == 0)
			continue;
		if (get(i) == -1)
			return -1.f;
		weightSum += weights->get(i) * get(i);
	}
	return (float)weightSum / cap;
}

bool Resources::capped(const Resources &cap) const
{
	for (auto &itr : *this)
	{
		size_t i = itr.first;
		if (cap.get(i) != -1 && get(i) == -1 && get(i) > cap.get(i))
			return false;
	}
	return true;
}

bool Resources::affordable(const Resources &cost) const
{
	for (auto &itr : cost)
	{
		size_t i = itr.first;
		if ((cost.get(i) == CONST_INF) || get(i) < cost.get(i))
			return false;
	}
	return true;
}

bool Resources::empty() const
{
	for (auto &itr : *this)
	{
		size_t i = itr.first;
		if (get(i) != 0)
			return false;
	}
	return true;
}

bool Resources::is_infinity() const
{
	for (auto &itr : *this)
	{
		size_t i = itr.first;
		if (get(i) != -1)
			return false;
	}
	return true;
}

void Resources::clear()
{
	for (auto &itr : *this)
	{
		size_t i = itr.first;
		(*this)[i] = 0;
	}
}

Resources Resources::to_bool() const
{
	Resources ret;
	for (auto &itr : *this)
	{
		size_t i = itr.first;
		ret[i] = get(i) ? 1 : 0;
	}
	return ret;
}

Resources Resources::reverse_bool(t_weights* weights) const
{
	Resources ret;
	for (auto &itr : *weights)
	{
		size_t i = itr.first;
		ret[i] = get(i) ? false : true;
	}
	return ret;
}

Resources Resources::reverse_bool(t_weights& weights) const
{
	return reverse_bool(&weights);
}

std::string Resources::to_string_weights(t_weights&)
{
	char out[100];
	snprintf(out, sizeof(out), "TODO %d %s", __LINE__, __FILE__);

	return std::string(out);
}

std::string Resources::to_string() const
{
	std::string str = "{";
	for (auto &itr : *this)
	{
		str += std::to_string(itr.first);
		str += ": ";
		str += std::to_string(itr.second);
		str += ", ";
	}
	str += "}";
	return str;
}

std::string Resources::to_string(t_weights* weights) const
{
	std::string str = "{";
	for (auto &itr : *this)
	{
		str += weights->get_str(itr.first);
		str += ":";
		str += std::to_string(itr.second);
		str += " ";
	}
	str += "}";
	return str;
}

int Resources::weight(t_weights *resWeight) const
{
	int ret = 0;
	for (auto &itr : *this)
	{
		size_t i = itr.first;
		if (get(i) > 0)
			ret += resWeight->get(i) * get(i);
	}
	return ret;
}

bool Resources::is_full(t_weights *resWeight, const int weightLimit) const
{
	if (weightLimit == -1)
		return false;
	int sum = 0;
	for (auto &itr : *this)
	{
		size_t i = itr.first;
		if (get(i) > 0)
			sum += get(i) * resWeight->get(i);
	}
	return sum >= weightLimit;
}

bool Resources::is_full(const Resources &cap) const
{
	for (auto &itr : *this)
	{
		size_t i = itr.first;
		if (get(i) < cap.get(i) || cap.get(i) == -1)
			return false;
	}
	return true;
}

bool Resources::has_bool(const Resources &b) const
{
	for (auto &itr : *this)
	{
		size_t i = itr.first;
		if (get(i) && b.get(i))
			return true;
	}
	return false;
}

bool Resources::has_bool_all(const Resources& b) const
{
	if (b.empty())
		return true;
	for (auto& itr : *this)
	{
		size_t i = itr.first;
		if (b.get(i))
		{
			if (!get(i))
				return false;
		}
	}
	return true;
}

// Static use
bool Resources::use(Resources &src, Resources &cost)
{
	for (auto &itr : cost)
	{
		size_t i = itr.first;
		if (src.get(i) == 0 || cost.get(i) == 0)
			continue;
		ASSERT_ERROR(src.get(i) != -1 || cost.get(i) != -1, "Cant remove infinity from infinity");

		int dif = cost.get(i);
		dif = math_min(dif, src[i]);
		dif = INF(src, i) ? 0 : dif;
		if (INF(cost, i))
		{
			src[i] = 0;
		}
		else
		{
			src[i] -= dif;
			cost[i] -= dif;
		}
	}
	return cost.empty();
}

// fill
bool Resources::fill(Resources &in, const Resources &limit)
{
	bool ret = true;
	for (auto &itr : *this)
	{
		size_t i = itr.first;

		(*this)[i] += in.get(i);
		if (get(i) > limit.get(i))
		{
			in[i] = get(i) - limit.get(i);
			(*this)[i] = limit.get(i);
			ret = false;
		}
		else
		{
			in[i] = 0u;
		}
	}

	return ret;
}

// transfer overloads
bool Resources::transfer(
	Resources &from,
	Resources &to,
	t_weights *weights,
	const int weightTransfer)
{
	return transfer(
		from, to, weights, weightTransfer,
		res_inf(weights),
		-1,
		res_true(weights));
}

bool Resources::transfer(
	Resources &from,
	Resources &to,
	t_weights *weights,
	const int weightTransfer,
	const Resources &toLimit,
	const int toLimitWeight)
{
	return transfer(
		from, to, weights, weightTransfer,
		toLimit,
		toLimitWeight,
		res_true(weights));
}

bool Resources::transfer(
	Resources &from,
	Resources &to,
	t_weights *weights,
	const int weightTransfer,
	const Resources &toLimit,
	const int toLimitWeight,
	const Resources &boolResource)
{
	int weightMin = (weightTransfer == -1 ? INT32_MAX : weightTransfer);
	if (toLimitWeight >= 0)
		weightMin = math_min(weightMin, toLimitWeight - to.weight(weights));

	bool anything = false;

	// Go throught every input resource
	for (auto &itr : from)
	{
		// Get id of resource
		size_t i = itr.first;

		// Skip is bool resource is false for current
		if (!boolResource.get(i))
			continue;
		// Try to transfer all from source
		int amount = from[i];
		// If there's is a weight limit, transfer the minimum possible
		amount = weightMin / weights->get(i);
		amount = math_min(amount, from[i]);
		// If there's a hard limit, transfer the minimum
		if (toLimit.get(i) >= 0)
			amount = math_min(amount, toLimit.get(i) - to[i]);
		// If there's anything to transfer, do it
		if (amount > 0)
		{
			to[i] += amount;
			from[i] -= amount;
			anything = true;
			int sub = amount * weights->get(i);
			if (weightMin > 0 && weightMin - sub <= 0)
				break;
			weightMin -= sub;
		}
	}

	return !from.empty() && anything;
}

Resources Resources::bool_pass(const Resources &other) const
{
	Resources ret;
	for (auto &itr : other)
	{
		size_t i = itr.first;
		ret[i] = (other.get(i) ? this->get(i) : 0);
	}
	return ret;
}

// can_transfer_weight
bool Resources::can_transfer_weight(
	const Resources &from,
	const Resources &to,
	t_weights *weights,
	const int toLimitWeight)
{
	return can_transfer_weight(
		from, to, weights, toLimitWeight,
		res_true(weights));
}

bool Resources::can_transfer_weight(
	const Resources &from,
	const Resources &to,
	t_weights *weights,
	const int toLimitWeight,
	const Resources &boolResource)
{
	if (toLimitWeight == -1)
		return true;
	int delta = toLimitWeight - to.bool_pass(boolResource).weight(weights);
	if (delta <= 0)
		return false;

	for (auto &itr : from)
	{
		size_t i = itr.first;

		if (boolResource.get(i))
		{
			assert(from.get(i) != -1 && to.get(i) != -1);
			if (from.get(i) > 0 &&
				math_min(1, from.get(i)) * weights->get(i) <= delta)
				return true;
		}
	}

	return false;
}

// can_transfer_limit
bool Resources::can_transfer_limit(
	const Resources &from,
	const Resources &to,
	const Resources &toLimit,
	t_weights *weights)
{
	return can_transfer_limit(
		from, to, toLimit,
		res_true(weights));
}

bool Resources::can_transfer_limit(
	const Resources &from,
	const Resources &to,
	const Resources &toLimit,
	const Resources &boolResource)
{
	Resources dif = toLimit - to;
	for (auto &itr : from)
	{
		size_t i = itr.first;

		assert(from.get(i) != -1 && to.get(i) != -1);
		if (!boolResource.get(i))
			continue;
		if (from.get(i) > 0 && (dif.get(i) > 0 || INF(toLimit, i)))
			return true;
	}
	return false;
}

// can_transfer
bool Resources::can_transfer(
	const Resources &from,
	const Resources &to,
	t_weights *weights,
	const int toLimitWeight,
	const Resources &toLimit)
{
	return can_transfer(
		from, to, weights, toLimitWeight,
		toLimit,
		Resources::res_true(weights));
}

bool Resources::can_transfer(
	const Resources &from,
	const Resources &to,
	t_weights *weights,
	const int toLimitWeight,
	const Resources &toLimit,
	const Resources &boolResource)
{
	for (auto &itr : from)
	{
		size_t i = itr.first;
		ASSERT_ERROR(from.get(i) != -1, "Can't transfer from infinite storage storage");
	}
	return can_transfer_weight(from, to, weights, toLimitWeight, boolResource) &&
		   can_transfer_limit(from, to, toLimit, boolResource);
}