#ifndef _GAME_SERIALIZABLE
#define _GAME_SERIALIZABLE

#include <cstdio>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <functional>

#include <fstream>

#include "nlohmann/json.hpp"


using namespace nlohmann;

#include "utils/class/resources.hpp"
#include "utils/class/timer.hpp"

#define PRINT_LINE printf("%d\n", (int)__LINE__)

typedef size_t t_id;
typedef size_t t_variant_id;

#ifdef __GNUC__

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"

#endif // __GNUC__

/*
Adding Variant:
	Add extend
	Define three functions
	Add counter and update
	Apply VariantPtr
	


*/

namespace sf
{
template <class T>
void to_json(json &j, const Vec<T> &v)
{
	j = json{v.x, v.y};
}

template <class T>
void from_json(const json &j, Vec<T> &p)
{
	j.at(0).get_to(p.x);
	j.at(1).get_to(p.y);
} 
}// namespace sf

static void to_json(json &j, const Resources &v)
{
	j = v.resCount;
}

static void from_json(const json &j, Resources &v)
{
	j.get_to(v.resCount);
}

template <class T>
static void to_json(json& j, const std::shared_ptr<GameTimer<T>>& v)
{
	j["length"] = v.get()->m_length;
	j["start"] = v.get()->m_start;
	j["init"] = v.get()->bInit;
	j["suspend"] = v.get()->m_suspend_start;
	j["saveTime"] = v.get()->m_saveTime;
}

template <class T>
static void from_json(const json& j, std::shared_ptr<GameTimer<T>>& v)
{
	j.at("length").get_to(v.get()->m_length);
	j.at("start").get_to(v.get()->m_start);
	j.at("init").get_to(v.get()->bInit);
	if (j.count("suspend"))
		j.at("suspend").get_to(v.get()->m_suspend_start);
	if (j.count("saveTime"))
		j.at("saveTime").get_to(v.get()->m_saveTime);
}

#ifdef __GNUC__

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"

#endif // __GNUC__

struct Variant;
struct SerializeMap;

struct Variant
{
	Variant();

	Variant(size_t objectType, size_t objectId)
		: objectId(objectId),
		  objectType(objectType)
	{
	}

	virtual ~Variant() = 0;

	virtual void variant_initialize()
	{
	}

	json to_ptr_json();

	virtual json to_json() const;

	/*
	* "from_json" should initialize every variable that is located inside the derivitive.
	* If the object is not being desirialized from json use variant_initialize, this
	* function uses default parameters that belong to the derivitive class.
	*/
	virtual void from_json(const json &j);

	/*
	 * Was originnaly used as serialize_publish and serialize_initialize
	 */
	[[deprecated]] virtual void serialize_post(const SerializeMap &map);
	
	/*
	* Post serialization assingments, after from_json was called and all variables initialized.
	* Publish all variables derivitive of "Variant" class to SerializeMap.
	* Use before "serialize_initialize".
	*/
	virtual void serialize_publish(const SerializeMap &map);
	
	/*
	 * Initialize stuff that requires the use of other variants.
	 * After "serialize_post" was called, use the "SerializeMap" object to intiailize every
	 * variable that depends on other "Variant".
	 * For example, a citizen who has a link for it's home. First the citizen and home are
	 * initialized so the citizen home  could be located from the id and assinged
	 */
	virtual void serialize_initialize(const SerializeMap &map);
	
	// virtual std::string to_string();
	t_id objectId = 0;
	t_group objectType = 0;
};

#ifdef __GNUC__

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"

#endif // __GNUC__

static void to_json(json &j, const Variant *v)
{
	if (v)
		j = v->to_json();
}

static void from_json(const json &j, Variant *v)
{
	if (v)
		v->from_json(j);
}

#ifdef __GNUC__

#pragma GCC diagnostic pop

#endif // __GNUC__

/*
 * Used only in desirialization from json, every json object represent every "Variant"
 * variable with a "VariantPtr" that stored it's unique id. After desirialization the object
 * can find the original object through the id and initialize it's pointer(?) value.
 */
template <typename T>
struct VariantPtr final
{
	VariantPtr()
	{
		static_assert((std::is_base_of<Variant, T>::value));
	}

	VariantPtr(T *t_) : t(t_)
	{
		static_assert((std::is_base_of<Variant, T>::value));
		// Is it neccesary? Must check!
		if (t) {
			this->objectId = t->objectId;
			this->objectType = t->objectType;
		}
	}

	~VariantPtr()
	{
	}

	operator T *() const
	{
		return t;
	}

	T *&get() { return t; }

	T *get() const { return t; }

	T *operator->() const { return t; }

	T *operator=(T *t) { 
        if (t) {
            this->objectId = t->objectId;
            this->objectType = t->objectType;
        } 
		else {
            this->objectId = 0;
            this->objectType = 0;
        }

        return this->t = t;
	}

	bool operator==(T *const other) const
	{
		return this->t == other;
	}

	bool operator==(const VariantPtr &other) const
	{
		return this->t == other.t;
	}

	operator bool() const 
	{ 
		return t != nullptr; 
	}

	bool is_null() const
	{
		return this->t == nullptr;
	}
	
	bool is_valid()
	{
		return objectType != 0;
	}

	/*
	* Unless the variable type is unknown, and it need to get initialized later.
	* Instead of using from_json_ptr() use get()->from_json()
	*/
	void from_json_ptr(const json &j)
	{
		this->objectId = j["id"].get<size_t>();
		this->objectType = j["type"].get<size_t>();
	}

	/*
	* Instead of using to_json_pre() use get()->to_json()
	*/
	json to_json_ptr(bool withVal = false) const
	{
		json j;
		
		// Don't use data frp, t since t can be dead memory
		j["id"] = this->objectId;
		j["type"] = this->objectType;
		if (withVal)
			j["value"] = (t ? t->to_json() : (json) nullptr);
		return j;
	}

	size_t objectId = 0;
	size_t objectType = 0;
	T *t = nullptr;
};

namespace std
{
template <class T>
struct hash<VariantPtr<T>>
{
	std::size_t operator()(const VariantPtr<T> &k) const {
		size_t hash = 17;
		hash = hash * 31 + std::hash<size_t>()(k.objectId);
		hash = hash * 31 + std::hash<size_t>()(k.objectType);
		return hash;
	}
};


} // namespace std

template <class T>
static void to_json(json &j, const VariantPtr<T> &v)
{
	//j = v->to_ptr_json();

	j = v.to_json_ptr(false);
}

template <class T>
static void from_json(const json &j, VariantPtr<T> &v)
{
	v.from_json_ptr(j);
}

/**
* Containts all serializable variants for use in variant initialization
* Most variants need access to each other when being desirialized,
* so to being initialized correctly, they get access to "SerializeMap".
*/
struct SerializeMap
{
	typedef std::map<size_t, Variant *> t_map_id;
	typedef std::map<t_serializable, t_map_id> t_map;

	t_map data;

	struct iterator
	{
		using iterator_category = std::forward_iterator_tag;
		using value_type = Variant *;
		using difference_type = std::ptrdiff_t;
		using pointer = value_type *;
		using reference = value_type &;

		typedef t_map_id::iterator t_itr_id;
		typedef t_map::iterator t_itr_type;

		reference operator*() const { return (*itrId).second; }
		pointer operator->() { return &(*itrId).second; }

		iterator(
			t_itr_type itrType,
			t_itr_id itrId,
			t_itr_type itrEnd);

		// Prefix increment
		iterator &operator++();

		// Postfix increment
		iterator operator++(int);

		friend bool operator==(const iterator &a, const iterator &b);

		friend bool operator!=(const iterator &a, const iterator &b);

		t_itr_type itrType;
		t_itr_id itrId;
		t_itr_type itrEnd;
	};

	struct const_iterator
	{
		using iterator_category = std::forward_iterator_tag;
		using value_type = Variant *;
		using difference_type = std::ptrdiff_t;
		using pointer = value_type *;
		using reference = const value_type &;

		typedef t_map_id::iterator t_itr_id;
		typedef t_map::iterator t_itr_type;

		reference operator*() const { return (*itrId).second; }
		pointer operator->() { return &(*itrId).second; }

		const_iterator(
			t_itr_type itrType,
			t_itr_id itrId,
			t_itr_type itrEnd);

		// Prefix increment
		const_iterator &operator++();

		// Postfix increment
		const_iterator operator++(int);

		friend bool operator==(
			const const_iterator &a,
			const const_iterator &b);

		friend bool operator!=(
			const const_iterator &a,
			const const_iterator &b);

		t_itr_type itrType;
		t_itr_id itrId;
		t_itr_type itrEnd;
	};

	iterator begin();

	iterator end();

	const_iterator cbegin();

	const_iterator cend();

	bool has(const json &j) const;

	bool has(size_t type, size_t id) const;

	void debug_print() const
	{
		for (auto& a : data)
		{
			for (auto& b : a.second)
			{
				LOG("%d %d", a.first, b.first, b.second->objectType, b.second->objectId);
			}
		}
	}

	template <class T = Variant>
	T *get(size_t type, size_t id) const
	{
		auto itra = data.find(type);
		if (itra == data.end())
		{
			return nullptr;
		}
		auto itrb = (*itra).second.find(id);
		if (itrb == (*itra).second.end())
		{
			return nullptr;
		}
		return dynamic_cast<T *>(itrb->second);
	}

	// In the deserialization stage all "VariantPtr" don't have a pointer value, just id.
	// Apply will seach for the id and apply a pointer.
	// Should be in serialize_publish
	template <class T = Variant>
	bool apply(VariantPtr<T> &j) const
	{
		T* resolvedPtr = get<T>(j.objectType, j.objectId);
		
		if (!resolvedPtr && (j.objectType != 0 || j.objectId != 0)) {
			DEBUG("SerializeMap::apply FAILED to resolve Type: %zu, ID: %zu\n", j.objectType, j.objectId);
		} else if (resolvedPtr) {
			// Uncomment this to trace every single successful link
			DEBUG("Linked Type: %zu, ID: %zu to Address: %p\n", j.objectType, j.objectId, (void*)resolvedPtr);
		}

		j = resolvedPtr;
		return j.get() != nullptr;
	}

	template <class T = Variant>
	void add(t_serializable type, size_t id, T *t)
	{
		data[type][id] = dynamic_cast<Variant *>(t);
	}

	inline t_map_id &operator[](size_t i);
};

////////
// Variantinator
////////

class VariantinatorBase
{
  public:
	VariantinatorBase() {}
	virtual ~VariantinatorBase() {}
	virtual Variant *create(SerializeMap &, size_t, size_t) = 0;

	size_t typeId = 0;
};

template <class T>
class Variantinator : public VariantinatorBase
{
  public:
	Variantinator() {}
	virtual ~Variantinator() {}
	virtual Variant *create(
		SerializeMap &map,
		size_t type,
		size_t id)
	{
		T *t = new T{};
		t->objectId = id;
		t->objectType = type;
		map.add<T>(t->objectType, t->objectId, t);
		return dynamic_cast<Variant *>(t);
	}
};

class VariantFactory
{
  public:
	VariantFactory();

	~VariantFactory();

	template <size_t type, typename T>
	void register_variant()
	{
		register_variant(
			type,
			std::make_unique<Variantinator<T>>(),
			typeid(T).hash_code());
	}

	Variant *create(SerializeMap &map, t_variant_id type, size_t id);

	Variant *create(SerializeMap &map, t_variant_id type);

	template <typename T>
	inline  t_variant_id variant_type_to_id() const
	{
		return variant_hash_to_id(typeid(T).hash_code());
	}

	std::vector<int> list_types()
	{
		std::vector<int> out;
		std::stringstream ss{};
		for (auto& x : mSwitchToVariant)
			out.push_back((int)x.first);
		return out;
	}

	void clear_counters();
	

  private:
	void register_variant(
		t_variant_id type,
		std::unique_ptr<VariantinatorBase> &&creator,
		size_t typeHash);

	inline t_variant_id variant_hash_to_id(size_t typeHash) const
	{
		auto itr = mSwitchToVariant.cbegin();
		while (itr != mSwitchToVariant.cend())
		{
			if ((*itr).second->typeId == typeHash)
				return (*itr).first;
			++itr;
		}
		return 0;
	}

	typedef std::map<
		t_variant_id,
		std::unique_ptr<VariantinatorBase>>
		t_switch_variant;
	std::map<t_variant_id, size_t> mSwitchCounter;
	t_switch_variant mSwitchToVariant;
};


struct ClassIdCounter
{
	ClassIdCounter(VariantFactory *factory)
	{
		this->factory = factory;
	}

	template <class T>
	inline size_t advance()
	{
		assert(factory);
		t_variant_id id = factory->variant_type_to_id<T>();
		auto itr = counterMap.find(id);
		if (itr == counterMap.end())
		{
			counterMap[id] = 0;
			return 0;
		}
		return ++itr->second;
	}

	template <class T>
	inline void set(size_t x)
	{
		t_variant_id id = typeid(T).hash_code;
		counterMap[id] = 0;
	}

	void clear()
	{
		for (auto &x : counterMap)
			x.second = 0;
	}

	VariantFactory *factory = nullptr;
	std::map<t_variant_id, size_t> counterMap{};
};

#endif // _GAME_SERIALIZABLE
