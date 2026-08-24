#ifndef GAME_UTILS
#define GAME_UTILS

#include <cstdlib>
#include <iterator>
#include <functional>
#include <stdarg.h>
#include <sstream>
#include <string.h>
#include <vector>
#include <map>
#include <unordered_map>
#include <locale>

#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Vector2.hpp>

#include "globals.hpp"
#include "class/logger.hpp"

#ifdef __GNUC__

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#define _UNUSED __attribute__((unused))

#else

#define _UNUSED

#endif // __GNUC__

#if !defined(__PRETTY_FUNCTION__) && defined(_MSC_VER)
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

// Macros

#define LOOP(var, time) for (size_t var = 0; var < time; ++var)

#define GOT_HERE() printf("Got here %s %d\n", __FILE__, __LINE__)

#define COUNT_OF(arr) (sizeof(arr) / sizeof(*arr))

#define CSTR(x) (x.to_string().c_str())

#define VEC_CSTR(v) (vec_str((v)).data())

#define TO_CSTR(x) (std::to_string((x)).c_str())
// Panic / last resort debugging tools

#ifndef AA
#define AA printf("%d, ", __LINE__);
#endif

#ifndef AAAA
#define AAAA printf("%d\n", __LINE__);
#endif

#ifdef _MSC_VER
#define strtok_r strtok_s
#endif

namespace std
{
template <>
struct hash<sf::Vector2<int32_t>>
{
	std::size_t operator()(const sf::Vector2<int32_t> &k) const
	{
		const size_t x = *reinterpret_cast<const uint32_t *>(&k.x);
		const size_t y = *reinterpret_cast<const uint32_t *>(&k.y);
		uint64_t h = (uint64_t)x + ((uint64_t)y << 32);
		return (size_t)h;
	}
};
}; // namespace std

namespace std
{
template <>
struct hash<t_idpair>
{
	std::size_t operator()(const t_idpair &k) const
	{
		uint64_t h = k.group + (k.id << 32);
		return (size_t)h;
	}
};
}; // namespace std

template <typename T = int>
struct Range
{
	T mStart, mEnd, mDiff;

	Range(T mStart, T mEnd, T mDiff = 1)
		: mStart(mStart), mEnd(mEnd), mDiff(mDiff)
	{
		assert(mDiff);
	}

	void reverse()
	{
		T tmp = mStart;
		mStart = mEnd;
		mEnd = tmp;
		mDiff = -mDiff;
	}

	struct iterator
	{
		using iterator_category = std::random_access_iterator_tag;
		using difference_type = T;
		using value_type = T;
		using pointer = T *;
		using reference = T &;

		T mValue;
		T mDiff;

		iterator(T mValue, T mDiff)
			: mValue(mValue), mDiff(mDiff)
		{
		}

		reference operator*() { return mValue; }
		pointer operator->() { return &mValue; }

#define OPERATOR(op1, op2)     \
	iterator &operator op1()   \
	{                          \
		mValue op2;            \
		return *this;          \
	}                          \
	iterator operator op1(int) \
	{                          \
		iterator tmp = *this;  \
		op1(*this);            \
		return tmp;            \
	}

		OPERATOR(++, += mDiff);

		OPERATOR(--, -= mDiff);

		iterator &operator+=(T diff)
		{
			mValue += diff * mDiff;
			return *this;
		}

		iterator &operator-=(T diff)
		{
			mValue -= diff * mDiff;
			return *this;
		}

		friend bool operator==(const iterator &a, const iterator &b)
		{
			return a.mValue == b.mValue;
		};
		friend bool operator!=(const iterator &a, const iterator &b)
		{
			return a.mValue != b.mValue;
		}
	};

	iterator begin()
	{
		return iterator(mStart, mDiff);
	}

	iterator end()
	{
		int end = mDiff * ((mEnd - mStart) / mDiff + 1);
		end += mStart;

		return iterator(end, mDiff);
	}
};

template <template<class...> 
	class Container,
	class TypeFirst,
	class TypeSecond,
	class TypeFinal>
struct NestedMap
{
	typedef Container<TypeSecond, TypeFinal> t_inner;
	typedef Container<TypeFirst, t_inner> t_outer;

	t_outer data;

	NestedMap()
	{
	}

	typename t_outer::iterator
		insert(
		const TypeFirst& tfirst,
		const TypeSecond& tsecond,
		const TypeFinal& type)
	{
		typename t_outer::iterator itrA;
		typename t_inner::iterator itrB;

		if ((itrA = data.find(tfirst)) == data.end())
			itrA = data.insert({ tfirst, t_inner{} }).first;
		t_inner& inner = (*itrA).second;

		if ((itrB = inner.find(tsecond)) == inner.end())
			itrB = inner.insert({ tsecond, type }).first;
		return itrA;
	}

	t_inner& at(const TypeFirst& first)
	{
		return data.at(first);
	}

	TypeFinal& at(const TypeFirst& first, const TypeSecond& second)
	{
		return data.at(first).at(second);
	}

	typename t_outer::iterator find(const TypeFirst& first)
	{
		return data.find(first);
	}

	typename t_inner::iterator find(const TypeFirst& first, const TypeSecond& second)
	{
		return data.at(first).find(second);
	}

	size_t count(const TypeFirst& first)
	{
		return data.count(first);
	}

	size_t count(const TypeFirst& first, const TypeFirst& second)
	{
		auto itr = data.find(first);
		if (itr == data.end())
			return (size_t)0;
		else
			return itr->second.count(second);
	}


	typename t_outer::iterator
		begin() { return data.begin(); }

	typename t_outer::iterator
		end() { return data.end(); }

	typename t_outer::const_iterator
		cbegin() const
	{
		return data.cbegin();
	}

	typename t_outer::const_iterator
		cend() const
	{
		return data.cend();
	}


	/*
	iterator
	begin() { return iterator{data.begin(), data.end()}; }

	iterator
	end() { return iterator{data.end(), data.end()}; }

	const_iterator
	cbegin() const
	{
		return const_iterator{data.cbegin(), data.cend()};
	}

	const_iterator
	cend() const
	{
		return const_iterator{data.cend(), data.cend()};
	}
	*/
};

template <size_t x>
constexpr size_t check_compile()
{
	return x;
}

template <typename Container>
constexpr void clear_containers(Container c)
{
	for (auto i = c.begin(); i != c.end(); i++)
	{
		delete *i;
	}
	c.clear();
}

static inline std::uint8_t *write_color(std::uint8_t *buf, sf::Color &c)
{
	*buf++ = c.r;
	*buf++ = c.g;
	*buf++ = c.b;
	*buf++ = c.a;
	return buf;
}

static inline std::vector<std::string> str_split(const std::string str, const std::string splt)
{
	std::vector<std::string> ret;
	char *cstr = const_cast<char *>(str.c_str());
	char *rest = cstr;
	char *token = strtok_r(cstr, splt.c_str(), &rest);
	while (token != nullptr)
	{
		std::string str = std::string(token);
		ret.push_back(str);
		token = strtok_r(nullptr, splt.c_str(), &rest);
	}
	return ret;
}

template <typename T>
static inline std::string vec_str(const sf::Vector2<T> &v)
{
	std::ostringstream out;
	out.precision(3);
	out << "{";
	out << std::fixed << v.x;
	out << ", ";
	out << std::fixed << v.y;
	out << "}";
	return out.str();
}

template <typename U>
static inline std::string array_to_string(const U *arr, const size_t arrSize)
{
	std::stringstream str;
	str << "{ ";
	for (size_t i = 0; i < arrSize; ++i)
	{
		str << arr[i] << (i == arrSize - 1 ? "" : ", ");
	}
	str << " }";
	return str.str();
}

template <typename T>
static inline std::ostream &operator<<(std::ostream &output, const sf::Vector2<T> &v)
{
	output << vec_str(v);
	return output;
}

template <typename Iterator>
static inline std::string iterator_to_string(Iterator begin, Iterator end, const std::string &sep = ", ")
{
	std::stringstream str;
	//str << "{ ";
	for (auto itr = begin; itr != end; ++itr)
	{
		str << *itr;
		if (std::next(itr) != end)
			str << sep;
	}
	//str << " }";
	return str.str();
}

// 2 * O^1 Efficiency
template <typename Iterator>
inline std::string iterator_to_string_linear(Iterator start, Iterator end, const std::string &sep = ", ")
{
	size_t bufferSize = 0;
	size_t dist = 0;
	for (auto i = start; i != end; ++i)
	{
		bufferSize += (*i).size();
		++dist;
	}
	if (!bufferSize)
	  return "";
	bufferSize += (dist - 1) * sep.size();
	std::string ret;
	ret.reserve(bufferSize);
	for (auto i = start; i != end; ++i)
	{
		ret += (*i);
		if (std::next(i) != end)
			ret += sep;
	}
	return ret;
}

template <typename Container>
static inline std::string container_to_string(const Container &container, const std::string &sep = ", ")
{
	return iterator_to_string(
		container.begin(),
		container.end(),
		sep);
}

template <typename U>
static inline void merge_vectors(std::vector<U> &a, const std::vector<U> &b)
{
	a.reserve(a.size() + b.size());
	a.insert(a.begin(), b.cbegin(), b.cend());
}

// Non‑template functions declared here, defined in utils.cpp
std::string str_is_integer(std::string& str);
std::string str_replace(std::string& str, const std::string& from, const std::string& to);
std::string str_unfold(const std::string &str);
int str_to_int(const std::string &str);
sf::Vector2i string_to_ivec(const std::string &str);
std::string str_enclose(const std::string &str);
std::string str_unenclose(const std::string &str);

// Inline small helpers
inline bool str_is_num(const std::string &str)
{
	return std::all_of(str.begin(), str.end(), ::isdigit);
}

inline bool str_is_alpha(const std::string &str)
{
	return std::all_of(str.begin(), str.end(), ::isalpha);
}

inline std::string str_lowercase(const std::string &str)
{
	std::string ret;
	ret = str;
	std::transform(str.cbegin(), str.cend(), ret.begin(), ::tolower);
	return ret;
}

inline std::string str_uppercase(const std::string &str)
{
	std::string ret;
	ret = str;
	std::transform(str.cbegin(), str.cend(), ret.begin(), ::toupper);
	return ret;
}

inline std::string str_left_trim(const std::string& str)
{
	std::string s = str;
	auto it = std::find_if(s.begin(), s.end(),
		[](char c) {
			return !std::isspace<char>(c, std::locale::classic());
		});
	s.erase(s.begin(), it);
	return s;
}

inline std::string str_right_trim(const std::string& str)
{
	std::string s = str;
	auto it = std::find_if(s.rbegin(), s.rend(),
		[](char c) {
			return !std::isspace<char>(c, std::locale::classic());
		});
	s.erase(it.base(), s.end());
	return s;
}

inline std::string str_trim(const std::string &s)
{
	return str_left_trim(str_right_trim(s));
}

template <size_t U = 32>
static inline std::string string_format(
	const std::string &dest,
	const std::string &source)
{
	auto a = str_split(dest, "_");
	auto b = str_split(source, "_");

	size_t maxSize = a.size() > b.size()
						 ? a.size()
						 : b.size();
	char buffer[U] = { 0 };
	char *buf = buffer;
	//size_t bufCounter = 0;
	for (size_t i = 0; i != maxSize; ++i)
	{
		std::string str1 = ((i >= a.size())
								? ""
								: str_unenclose(a[i]));
		std::string str2 = ((i >= b.size())
								? ""
								: str_unenclose(b[i]));
		std::string sel = "";

		if (str2 != "")
			sel = str2;
		else if (str1 != "")
			sel = str1;
		if (sel.size() && sel.front() == '{' && sel.back() == '}')
		{
			sel.erase(sel.begin());
			sel.erase(sel.end() - 1);
		}

		if (sel != "")
		{
			for (auto itr = sel.begin(); itr != sel.end(); ++itr)
			{
				*buf++ = *itr;
				if (buf - buffer >= U)
					break;
			}
			*buf++ = '_';
			if (buf - buffer >= U)
				break;
		}
		else
		{
			if (buf - buffer + 3 >= U)
				break;
			*buf++ = '{';
			*buf++ = '}';
			*buf++ = '_';
			if (buf - buffer >= U)
				break;
		}
	}
	*(buf - 1) = '\0';
	return std::string(buffer);
}

// Taken from boost::hash_combine
template <class T>
inline void hash_combine(std::size_t &seed, const T &v)
{
	std::hash<T> hasher;
	seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

// Custom containers

#define SET_NNULL(a, b) \
	{                   \
		if (a)          \
			*a = b;     \
	}

/**
  * Containes two sided map represented by 
  * the maps with opposite key-value template
  */
template <
	typename _A,
	typename _B,
	template <typename...> class _Map = std::unordered_map>
struct BiMap
{
	_A nullA;
	_B nullB;
	_Map<_A, _B> mapAtB;
	_Map<_B, _A> mapBtA;

	void clear()
	{
		mapAtB.clear();
		mapBtA.clear();
	}

	void add(_A &&a, _B &&b)
	{
		mapAtB[a] = b;
		mapBtA[b] = a;
	}

	void add(const _A &a, const _B &b)
	{
		mapAtB[a] = b;
		mapBtA[b] = a;
	}

	_A get_first(const _B &b, bool *success = nullptr) const
	{
		if (success)
			*success = true;
		auto itr = mapBtA.find(b);
		if (itr == mapBtA.end())
		{
			if (success)
				*success = false;
			return nullA;
		}
		return (*itr).second;
	}

	_A &get_first(const _B &b, bool *success = nullptr)
	{
		if (success)
			*success = true;
		auto itr = mapBtA.find(b);
		if (itr == mapBtA.end())
		{
			if (success)
				*success = false;
			return nullA;
		}
		return (*itr).second;
	}

	_B &get_second(const _A &a, bool *success = nullptr)
	{
		if (success)
			*success = true;
		auto itr = mapAtB.find(a);
		if (itr == mapAtB.end())
		{
			if (success)
				*success = false;
			return nullB;
		}
		return (*itr).second;
	}

	_B get_second(const _A &a, bool *success = nullptr) const
	{
		if (success)
			*success = true;
		auto itr = mapAtB.find(a);
		if (itr == mapAtB.end())
		{
			if (success)
				*success = false;
			return nullB;
		}
		return (*itr).second;
	}

	bool has_first(const _B &b) const
	{
		auto itr = mapBtA.find(b);
		if (itr == mapBtA.end())
			return false;
		return true;
	}

	bool has_second(const _A &a) const
	{
		auto itr = mapAtB.find(a);
		if (itr == mapAtB.end())
			return false;
		return true;
	}

	typename _Map<_A, _B>::iterator
	begin() { return mapAtB.begin(); }

	typename _Map<_A, _B>::iterator
	end() { return mapAtB.end(); }

	typename _Map<_A, _B>::const_iterator
	begin() const { return mapAtB.begin(); }

	typename _Map<_A, _B>::const_iterator
	end() const { return mapAtB.end(); }
};

template <
	typename T,
	template <typename...> class _Map = std::unordered_map>
struct IdStrTMap
{
	// Id to class type map
	std::map<size_t, T> idTMap;
	// Two sided map linking between both object id and object name
	BiMap<size_t, std::string, _Map> biMap;
	T null;

	typedef typename decltype(idTMap)::iterator iterator;
	typedef typename decltype(idTMap)::const_iterator const_iterator;

	/**
	  * Adds a new type with a name and id
	  */
	void add(const size_t id, const std::string &name, const T &t)
	{
		biMap.add(id, name);
		idTMap[id] = t;
	}

	void clear()
	{
		idTMap.clear();
	}

	/**
	  * Adds a new type where only the name is available, meaning a new id
	  * has to be created.
	  */
	void append(const std::string &name, const T &t)
	{
		auto m = biMap.mapAtB;
		size_t id = 0;
		for (auto &p : m)
			id = (id < p.first ? p.first : id);
		id += 1;
		biMap.add(id, name);
		idTMap[id] = t;
	}

	size_t next_key()
	{
		if (!idTMap.count(0))
			return 0;
		size_t prev = 0, next = 0;
		for (auto &p : idTMap)
		{
			next = p.first;
			if (next != 0 && next - prev > 1)
			{
				return prev + 1;
			}
			prev = next;
		}
		return next + 1;
	}

	size_t push_and_move(
		size_t i,
		const std::string &str,
		const T &t)
	{
		size_t ret;
		auto itr = idTMap.find(i);
		if (itr == idTMap.end())
		{
			ret = i;
			add(i, str, t);
		}
		else
		{
			T &&tmp = std::move((*itr).second);
			std::string &&str = std::move(biMap.get_second(i));

			(*itr).second = t;
			biMap.add(i, str);

			ret = next_key();
			idTMap[ret] = tmp;
			biMap.add(ret, str);
		}
		return ret;
	}

	T &get(const size_t id, bool *success = nullptr)
	{
		if (success)
			*success = true;
		auto itr = idTMap.find(id);
		if (itr == idTMap.end())
		{
			if (success)
				*success = false;
			return null;
		}
		return (*itr).second;
	}

	T get(const size_t id, bool *success = nullptr)
		const
	{
		if (success)
			*success = true;
		auto itr = idTMap.find(id);
		if (itr == idTMap.end())
		{
			if (success)
				*success = false;
			return null;
		}
		return (*itr).second;
	}

	T &get(const std::string &name, bool *success = nullptr)
	{
		SET_NNULL(success, true);
		bool s;
		size_t ret = biMap.get_first(name, &s);
		if (!s)
		{
			SET_NNULL(success, false);
			return null;
		}
		return get(ret, success);
	}

	T get(const std::string &name, bool *success = nullptr)
		const
	{
		SET_NNULL(success, true);
		bool s;
		size_t ret = biMap.get_first(name, &s);
		if (!s)
		{
			SET_NNULL(success, false);
			return null;
		}
		return get(ret, success);
	}

	std::string get_str(const size_t id, bool *success = nullptr)
		const
	{
		bool s{};
		std::string ret = biMap.get_second(id, &s);
		SET_NNULL(success, s);
		return s ? ret : "Uknown";
	}

	inline size_t get_id(const std::string &str, bool *success = nullptr)
		const
	{
		return biMap.get_first(str, success);
	}

	inline bool has(const size_t id) const
	{
		return biMap.has_second(id);
	}

	inline bool has(const std::string &name) const
	{
		return biMap.has_first(name);
	}

	iterator begin() { return idTMap.begin(); }

	iterator end() { return idTMap.end(); }

	const_iterator begin() const { return idTMap.cbegin(); }

	const_iterator end() const { return idTMap.cend(); }
};

template <typename T, size_t _Size>
struct EnumManager
{
	typedef IdStrTMap<T> Group;

	Group groups[_Size];
	Group nullGroup;
	T null;
	BiMap<size_t, std::string> biMap;

	EnumManager()
	{
	}

	std::string to_string() 
	{
		std::stringstream ss("");
		for (auto itr = biMap.mapAtB.cbegin();
			 itr != biMap.mapAtB.cend();
			 ++itr)
		{
			ss << "Id: " << itr->second << " \"" << itr->first << "\"\n";
			Group &g = get_group(itr->second);

			for (auto itr2 = g.biMap.mapAtB.cbegin();
				 itr2 != g.biMap.mapAtB.cend();
				 ++itr2)
			{
				ss << '\t';
				ss << itr2->first << ": " << itr2->second << '\n';
			}
		}
		return ss.str();
	}

	// Group related

	void add_group(size_t id, const std::string &name)
	{
		this->biMap.add(id, str_uppercase(name));
	}

	bool has_group(size_t group) const
	{
		return group < _Size;
	}

	Group &get_group(size_t group, bool *success = nullptr)
	{
		if (success)
			*success = true;
		if (!has_group(group))
		{
			if (success)
				*success = false;
			return nullGroup;
		}
		return groups[group];
	}

	Group &get_group(const std::string &title, bool *success = nullptr) 
	{
		SET_NNULL(success, true);
		bool b;
		t_group group = biMap.get_first(title, &b);
		if (!b)
		{
			SET_NNULL(success, false);
			return nullGroup;
		}
		if (!has_group(group))
		{
			SET_NNULL(success, false);
			return nullGroup;
		}
		return groups[group];
	}

	// String related
	
	// Get full string
	std::string get_str(size_t group, size_t id) 
	{
		std::string str;
		std::string a = get_group_str(group);
		std::string b = get_id_str(group, id);
		str.reserve(a.size() + b.size() + 2u);
		str += a;
		str += "::";
		str += b;
		return str;
	}

	// Get full string
	std::string get_str(const t_idpair& idpair)
	{
		std::string str;
		std::string a = get_group_str(idpair.group);
		std::string b = get_id_str(idpair.group, idpair.id);
		str += a;
		str += "::";
		str += b;
		return str;
	}
	
	// ???
	std::string get_id_str(size_t group, size_t id)
	{
		bool s;
		auto g = get_group(group, &s);
		std::string ret;
		if (s)
			ret = g.get_str(id, &s);
		return s ? ret : "Unknown";
	}
	
	// Get tree id from string
	void get_id(
		size_t &group,
		size_t &id,
		const std::string &name,
		bool *success = nullptr)
	{
		bool b;
		SET_NNULL(success, true);
		auto v = str_split(str_uppercase(name), "::");
		if (v.size() == 0)
		{
			SET_NNULL(success, false);
			return;
		}
		else if (v.size() == 1)
		{
			bool any = false;
			for (int i = 0; i < _Size; ++i)
			{
				Group &g = groups[i];
				id = g.get_id(str_uppercase(v[0]), &b);
				if (b)
				{
					group = i;
					return;
				}
			}
			if (!any)
			{
				SET_NNULL(success, false);
			}
		}
		else
		{
			size_t g = biMap.get_first(str_uppercase(v[0]), &b);
			if (!b)
			{
				SET_NNULL(success, false);
				return;
			}
			group = g;
			id = groups[g].get_id(str_uppercase(v[1]), success);
		}
	}
	
	t_idpair get_id(
		const std::string &name,
		bool *success = nullptr)
	{
		t_group group = 0;
		t_id id = 0;
		get_id(group, id, name, success);
		return t_idpair{group, id};
	}

	std::string get_group_str(size_t group) 
	{
		std::string a;
		if (has_group(group) && biMap.has_second(group))
			a = biMap.get_second(group);
		else
			a = "Unknown";
		return a;
	}

	// Get T Variations

	T &get(size_t group, size_t id, bool *success = nullptr) 
	{
		SET_NNULL(success, true);
		if (!has_group(group))
		{
			SET_NNULL(success, false);
			return null;
		}
		return groups[group].get(id, success);
	}

	T &get(size_t group,
		   const std::string &name,
		   bool *success = nullptr) 
	{
		SET_NNULL(success, true);
		if (group >= _Size)
		{
			SET_NNULL(success, false);
			return null;
		}
		return groups[group].get(str_uppercase(name), success);
	}

	T &get(const std::string &name, bool *success = nullptr) 
	{
		SET_NNULL(success, true);
		size_t group{}, id{};
		bool b{};
		get_id(group, id, name, &b);
		if (!b)
		{
			SET_NNULL(success, false);
			return null;
		}
		return groups[group].get(id);
	}
};

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif // __GNUC__

#endif // GAME_UTILS