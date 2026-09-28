
#include "game/game_data.hpp"
#include "game/power_network.hpp"
#include "game/game_buildings.hpp"


PowerNetwork::PowerNetwork(size_t id)
	: Variant(SERIALIZABLE_NETWORK, id)
{
}

json PowerNetwork::to_json() const
{
	json j;
	j["builds"] = {output, input, station};
	j["vals"] = {powerOut, powerIn, powerStore,
				 powerValue, storeCount};
	return j;
}

void PowerNetwork::from_json(const json &j)
{
	try
	{
		j.at("builds").at(0).get_to(output);
		j.at("builds").at(1).get_to(input);
		j.at("builds").at(2).get_to(station);

		j.at("vals").at(0).get_to(powerOut);
		j.at("vals").at(1).get_to(powerIn);
		j.at("vals").at(2).get_to(powerStore);
		j.at("vals").at(3).get_to(powerValue);
		j.at("vals").at(4).get_to(storeCount);
	}
	catch (json::exception &e)
	{
		LOG_ERROR("Can't desirialize PowerNetwork: %s", e.what());
	}
}

void PowerNetwork::serialize_publish(const SerializeMap &map)
{
    Variant::serialize_publish(map);
    
	for (auto &v : output)
		map.apply(v);
	for (auto &v : input)
		map.apply(v);
	for (auto &v : station)
		map.apply(v);
    
    GameData *g = map.get<GameData>(SERIALIZABLE_DATA, 0);
	assert(g);
    // Add the network to the active game context
    g->resourceContext.networks.push_back(this);
}

void PowerNetwork::serialize_initialize(const SerializeMap &map)
{
}

std::string PowerNetwork::to_string()
{
	std::stringstream ss("");
	ss << "{ ";
	for (int i = 0; i < 3; ++i)
	{
		static const std::string NAMES[3] = {"In", "Out", "Store"};
		ss << NAMES[i] << ": { ";
		ss << "Count: " << itr((Use)i).size();
		ss << ", Value: " << value((Use)i);
		ss << " }" << ((i == 2) ? ("") : (", "));
	}
	ss << " }";
	return ss.str();
}

PowerNetwork::t_builds &PowerNetwork::itr(Use use)
{
	switch (use)
	{
	case IN:
		return input;
	case OUT:
		return output;
	case STATION:
		return station;
	default:
		return input;
	}
}

int &PowerNetwork::value(Use use)
{
	switch (use)
	{
	case IN:
		return powerIn;
	case OUT:
		return powerOut;
	case STATION:
		return powerStore;
	default:
		return powerIn;
	}
}

int PowerNetwork::value(Use use) const
{
    switch (use)
	{
	case IN:
		return powerIn;
	case OUT:
		return powerOut;
	case STATION:
		return powerStore;
	default:
		return powerIn;
	}
}

void PowerNetwork::add(BuildingBase *base, Use use, bool changeValue)
{
	base->network = this;
	if (changeValue)
	{
		value(use) += resource(base, use);
	}

	auto &vec = itr(use);
	auto itr = std::find(vec.begin(), vec.end(), base);
	if (itr == vec.end())
	{
		auto pos = std::upper_bound(vec.begin(), vec.end(), base);
		vec.insert(pos, base);
	}
}

void PowerNetwork::add(const t_builds &builds, Use use)
{
	for (auto &b : builds)
		add(b, use);
}

void PowerNetwork::add(BuildingBase *base)
{
	for (int i = 0; i < 3; ++i)
	{
		auto u = (PowerNetwork::Use)i;
		if (resource(base, u))
			add(base, u);
	}
}

void PowerNetwork::remove(BuildingBase *base, Use use)
{
	/*
	base->network = nullptr;
	value(use) -= resource(base, use);
	*/
	auto &vec = itr(use);
	vec.erase(
		std::remove(
			std::begin(vec),
			std::end(vec),
			base),
		std::end(vec));
}

void PowerNetwork::remove(BuildingBase *base)
{
	for (int i = 0; i < 3; ++i)
	{
		auto u = (PowerNetwork::Use)i;
		if (resource(base, u))
			remove(base, u);
	}
}

void PowerNetwork::clear()
{
	for (int i = 0; i < 3; ++i)
	{
		auto u = (PowerNetwork::Use)i;
		itr(u).clear();
	}
}

int PowerNetwork::resource(BuildingBase *base, Use use)
{
	switch (use)
	{
	case IN:
		return base->powerIn;
	case OUT:
		return base->powerOut;
	case STATION:
		return base->powerStore;
	default:
		return base->powerIn;
	}
}

void PowerNetwork::merge(PowerNetwork &a, PowerNetwork &b)
{
	for (int i = 0; i < 3; ++i)
	{
		auto u = (PowerNetwork::Use)i;
		a.add(b.itr(u), u);
	}
	b.clear();
}

void PowerNetwork::merge(PowerNetwork *a, PowerNetwork *b)
{
	for (int i = 0; i < 3; ++i)
	{
		auto u = (PowerNetwork::Use)i;
		a->add(b->itr(u), u);
	}
	b->clear();
}

void PowerNetwork::serialize_post(const SerializeMap& map) {
    DEBUG("TODO Implement");
}