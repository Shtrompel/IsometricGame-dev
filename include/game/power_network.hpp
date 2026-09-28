#ifndef _GAME_POWER_NETWORK
#define _GAME_POWER_NETWORK

#include "game_body.hpp"
#include "game_entity.hpp"

struct PowerNetwork : public Variant
{
	// All typedefed containers should be reimplemeted
	// for wfficency later. (aka Todo)
	typedef std::vector<VariantPtr<BuildingBase>> t_builds;
	t_builds output;
	t_builds input;
	t_builds station;

	int powerOut = 0;
	int powerStore = 0;
	int powerIn = 0;

	int powerValue = 0;
	int storeCount = 0;

	enum Use
	{
		IN,
		OUT,
		STATION
	};

	PowerNetwork(size_t id = 0);

	json to_json() const override;

	void from_json(const json &j) override;

	void serialize_post(const SerializeMap &map) override;

    void serialize_publish(const SerializeMap &map) override;

    void serialize_initialize(const SerializeMap &map) override;

	std::string to_string();

	t_builds &itr(Use use);

	int &value(Use use);

	int value(Use use) const;

	// Add a building of a specific utility. The Use enum designated 
	// whether the building is a producer, consumer or a storage of electricity.
	// changeValue tells to update or ignore the value of the designation
	void add(BuildingBase *base, Use use, bool changeValue = true);

	// Add a set of buildings
	void add(const t_builds &builds, Use use);

	// Add a new building for the power grid. Acount for it being a consuming
	// building, generator building and storage building.
	void add(BuildingBase *base);

	void remove(BuildingBase *base, Use use);

	void remove(BuildingBase *base);

	void clear();

	static int resource(BuildingBase *base, Use use);

	static void merge(PowerNetwork &a, PowerNetwork &b);

	static void merge(PowerNetwork *a, PowerNetwork *b);
};

#endif // _GAME_POWER_NETWORK