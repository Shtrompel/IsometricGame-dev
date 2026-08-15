#ifndef _GAME_BODY
#define _GAME_BODY

#include <SFML/System/Vector2.hpp>

#include "utils/globals.hpp"
#include "libs/FastNoiseLite.h"
#include "file/serialization.hpp"
#include "utils/class/timer.hpp"

#include <unordered_set>

struct GameData;

typedef typename std::shared_ptr<GameTimer<t_seconds>> t_body_timer;
typedef GameTimerManager<t_seconds> t_time_manager;

template <typename t_use, typename t_property>
struct PropertySet
{
	typedef typename std::unordered_set<t_use> t_bools;
	typedef typename std::map<t_property, double> t_nums;

	static_assert(
		!std::is_same<
			t_use,
			t_property>::value,
		"PropertySet types can be the same");

	t_bools uses;
	t_nums nums;

	struct BoolItr
	{
		typedef typename t_bools::iterator iterator;
		typedef typename t_bools::const_iterator const_iterator;

		t_bools *const bools = nullptr;

		iterator begin() { return bools->begin(); }
		iterator end() { return bools->end(); }
		const_iterator cbegin() const { return bools->cbegin(); }
		const_iterator cend() const { return bools->cend(); }
	};

	template <bool IsConst = false>
	struct NumItr
	{
		using Container = std::conditional_t<
			IsConst,
			t_nums const,
			t_nums>;

		using iterator = std::conditional_t<
			IsConst,
			typename Container::const_iterator,
			typename Container::iterator>;

		Container *nums = nullptr;

		iterator begin() { return nums->begin(); }
		iterator end() { return nums->end(); }
	};

	PropertySet() {}

	void append(const PropertySet<t_use, t_property> &other)
	{
		for (const auto &v : other.nums)
			this->nums.insert(v);
		for (const auto &v : other.uses)
			this->uses.insert(v);
	}

	// Uses
	inline void bool_reset() { uses.clear(); }
	bool bool_is(const t_use &x) const { return uses.count(x); }
	bool bool_get(const t_use &x) const { return uses.count(x); }
	bool bool_is(size_t i) const { return bool_is((t_use)i); }
	bool bool_get(size_t i) const { return bool_get((t_use)i); }
	inline void bool_set(t_use x, bool b) { uses.insert(x); }
	inline size_t bool_size() const { return uses.size(); }
	inline BoolItr bool_itr() { return BoolItr{&uses}; }

	// Properties
	inline void num_reset() { nums.clear(); }
	bool num_is(const t_property x) const { return nums.count(x); }
	inline double num_set(t_property x, double d) { return (nums[x] = d); }
	double num_get(t_property x) { return nums.at(x); }
	bool num_is(size_t i) const { return num_is((t_property)i); }
	bool num_get(size_t i) const { return bool_get((t_property)i); }
	size_t num_size() const { return nums.size(); }
	inline NumItr<false> nums_itr() { return NumItr<false>{&nums}; }

	inline NumItr<true> nums_citr() const
	{
		NumItr<true> ret;
		ret.nums = &this->nums;
		return ret;
	}
};

template <typename t_use, typename t_property>
static void to_json(
	json &j,
	const PropertySet<t_use, t_property> &v)
{
	j = {v.uses, v.nums};
}

template <typename t_use, typename t_property>
static void from_json(
	const json &j,
	PropertySet<t_use, t_property> &v)
{
	j.at(0).get_to(v.uses);
	j.at(1).get_to(v.nums);
}

struct GameBodyConfig
{
	virtual ~GameBodyConfig() = default;
	virtual std::string to_string();
};

struct BodyConfigManager
{
	typedef GameBodyConfig *t_type;
	typedef std::unordered_map<t_id, t_type> t_idt;
	typedef std::unordered_map<t_idpair, t_idt> t_pair_idt;

	struct iterator
	{
		t_pair_idt *setA = nullptr;
		t_pair_idt::iterator itrA;
		t_idt *setB = nullptr;
		t_idt::iterator itrB;

		iterator(
			t_pair_idt *setA,
			t_pair_idt::iterator itrA,
			t_idt *setB,
			t_idt::iterator itrB)
			: setA(setA),
			  itrA(itrA),
			  setB(setB),
			  itrB(itrB)
		{
		}

		iterator &operator++()
		{
			itrB++;
			if (itrB == setB->end())
			{
				++itrA;
				if (itrA != setA->end())
					itrB = (*itrA).second.begin();
			}
			return *this;
		}

		iterator operator++(int)
		{
			iterator old = *this;
			operator++();
			return old;
		}

		t_type operator*()
		{
			return (*itrB).second;
		}
	};

	t_pair_idt data;

	BodyConfigManager();

	iterator insert(const t_idpair &pair, const t_type &type);
	iterator insert(const t_idpair &pair, const t_id id, const t_type &type);
	t_type at(const t_idpair &pair);
	t_type at(const t_idpair &pair, const t_id id);
	iterator begin();
	iterator end();
};

struct GameBody;

class AbstractCanTarget
{
	virtual GameBody* get_target();
	virtual void set_target(GameBody*);
};

struct GameBody : Variant, AbstractCanTarget
{
	bool isNoiseInit = 0;
	FastNoiseLite noise;

	GameData *context = nullptr;

	// Game logic
	FVec pos;
	FVec vel;
	float z = 0.0f;
	// AABB game rectangle width and height
	sf::Vector2f rectSize;
	// Radius for entities
	float radius = 0.0f;
	/* In the list of body types as seen in enums.json(with id of 1)
	 * what type of body is this object?
	 */
	BodyType type = BodyType::NONE;
	// What spawned this object
	GameBody *source = nullptr;

	std::vector<VariantPtr<GameBody>> followers{};

	VariantPtr<GameBody> target{ nullptr };

	ShapeType shape = ShapeType::POINT;
	bool visible = true;
	t_alignment alignment = ALIGNMENT_NONE;

	std::array<bool, COUNT_PROPERTY_BOOL>
		arrUseEnums = {false};
	std::map<PropertyNum, double>
		propertyVals;

	static inline int tmpHp = -1; // Null get_hp placeholder
	bool dead = false; // Add to json copression

	// Rendering
	sf::Vector2i start = {-1, -1};
	sf::Vector2i end = {-1, -1};
	t_sprite spriteHolder = -1;
	size_t animFrame = 0u;

	GameBody();
	GameBody(
		GameData *context,
		size_t e,
		size_t id,
		const sf::Vector2f &pos,
		const t_sprite &sprite,
		const BodyType &type,
		const sf::Vector2f &rect);

	virtual ~GameBody() = default;

	std::string get_ptr_str() const;

	virtual int get_hp() const;
	virtual int& get_hp();
	virtual GameBody* get_target() override;

#define EPIC_TEST3()\
	for (VariantPtr<GameBody>& x : this->followers)\
	assert(x.get() != this);

	std::string test_get_str() const;

	void set_target(GameBody* newTarget, bool skipOld = false);

	virtual FVec get_pos();
	virtual void apply_data(GameBodyConfig *config);
	virtual void update();
	virtual void update(float delta);

	inline float depth() const
	{
		// Isometric world depth
		return pos.x + pos.y;
	}

	bool operator<(const GameBody &other) const
	{
		// Depth sorting for rendering
		return this->depth() < other.depth();
	}

	void change_sprite(t_sprite sprite);

	virtual void variant_initialize() override;
	virtual json to_json() const override;
	virtual void from_json(const json &j0) override;
	virtual void serialize_publish(const SerializeMap &map) override;
	virtual void serialize_initialize(const SerializeMap &map) override;
	virtual bool confirm_body(GameData* context);
};

#endif // _GAME_BODY