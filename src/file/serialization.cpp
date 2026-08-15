#include "file/serialization.hpp"

#include "utils/math.hpp"

Variant::Variant()
{
}

Variant::~Variant()
{
}

json Variant::to_ptr_json()
{
	json j;
	j["id"] = objectId;
	j["type"] = objectType;
	return j;
}

json Variant::to_json() const
{
	return json{};
}

void Variant::from_json(const json &j)
{
}

void Variant::serialize_post(const SerializeMap &map)
{
}

void Variant::serialize_publish(const SerializeMap &map)
{
}

void Variant::serialize_initialize(const SerializeMap &map)
{
}
/*
std::string Variant::to_string()
{
	return to_json().dump(4);
}
*/
SerializeMap::iterator::iterator(
	t_itr_type itrType,
	t_itr_id itrId,
	t_itr_type itrEnd)
	: itrType(itrType),
	  itrId(itrId),
	  itrEnd(itrEnd)
{
}

// Prefix increment
SerializeMap::iterator &SerializeMap::iterator::operator++()
{
	if (std::next(itrId, 1) == itrType->second.end())
	{
		++this->itrType;
		this->itrId = (*itrType).second.begin();
	}
	else
	{
		++this->itrId;
	}

	return *this;
}

// Postfix increment
SerializeMap::iterator SerializeMap::iterator::operator++(int)
{
	SerializeMap::iterator tmp = *this;
	++(*this);
	return tmp;
}

bool operator==(
	const SerializeMap::iterator &a,
	const SerializeMap::iterator &b)
{
	if (a.itrType == a.itrEnd &&
		b.itrType == b.itrEnd &&
		a.itrEnd == b.itrEnd)
		return true;
	return a.itrId == b.itrId &&
		   a.itrType == b.itrType;
};

bool operator!=(
	const SerializeMap::iterator &a,
	const SerializeMap::iterator &b)
{
	return !(a == b);
};

SerializeMap::const_iterator::const_iterator(
	t_itr_type itrType,
	t_itr_id itrId,
	t_itr_type itrEnd)
	: itrType(itrType),
	  itrId(itrId),
	  itrEnd(itrEnd)
{
}

// Prefix increment
SerializeMap::const_iterator &SerializeMap::const_iterator::operator++()
{
	if (std::next(itrId, 1) == itrType->second.end())
	{
		++this->itrType;
		this->itrId = (*itrType).second.begin();
	}
	else
	{
		++this->itrId;
	}

	return *this;
}

// Postfix increment
SerializeMap::const_iterator SerializeMap::const_iterator::operator++(int)
{
	SerializeMap::const_iterator tmp = *this;
	++(*this);
	return tmp;
}

bool operator==(
	const SerializeMap::const_iterator &a,
	const SerializeMap::const_iterator &b)
{
	if (a.itrType == a.itrEnd &&
		b.itrType == b.itrEnd &&
		a.itrEnd == b.itrEnd)
		return true;
	return a.itrId == b.itrId &&
		   a.itrType == b.itrType;
};

bool operator!=(
	const SerializeMap::const_iterator &a,
	const SerializeMap::const_iterator &b)
{
	return !(a == b);
};

SerializeMap::iterator SerializeMap::begin()
{
	return SerializeMap::iterator{
		data.begin(),
		data.begin()->second.begin(),
		data.end()};
}

SerializeMap::iterator SerializeMap::end()
{
	return SerializeMap::iterator{
		data.end(),
		{data.begin()->second.end()},
		data.end()};
}

SerializeMap::const_iterator SerializeMap::cbegin()
{
	return SerializeMap::const_iterator{
		data.begin(),
		data.begin()->second.begin(),
		data.end()};
}

SerializeMap::const_iterator SerializeMap::cend()
{
	return SerializeMap::const_iterator{
		data.end(),
		{},
		data.end()};
}

bool SerializeMap::has(const json &j) const
{
	size_t id = 0, type = 0;
	auto itrType = j.find("type");
	if (itrType == j.end())
		return false;
	id = itrType.value().get<size_t>();
	auto itrId = j.find("id");
	if (itrId == j.end())
		return false;
	type = itrId.value().get<size_t>();
	return has(type, id);
}

bool SerializeMap::has(size_t type, size_t id) const
{
	auto itra = data.find(type);
	if (itra == data.end())
		return false;
	return itra->second.count(id);
}

inline SerializeMap::t_map_id &SerializeMap::operator[](size_t i)
{
	return data[i];
}

VariantFactory::VariantFactory()
{
}

VariantFactory::~VariantFactory()
{
}

Variant *VariantFactory::create(SerializeMap &map, t_variant_id type, size_t id)
{
	auto itrVariant = mSwitchToVariant.find(type);
	if (itrVariant == mSwitchToVariant.end())
	{
		LOG_ERROR("Variant of type id %d in VariantFactory was not found.",(int) type);

		std::stringstream ss;
		for (auto& x : mSwitchToVariant)
			ss << std::to_string(x.first) << ", ";
		LOG("Available are: %s.", ss.str().c_str());
		
		assert(itrVariant != mSwitchToVariant.end());
		return nullptr;
	}

	auto itrCounter = mSwitchCounter.find(type);
	if (itrCounter == mSwitchCounter.end())
	{
		mSwitchCounter[type] = 0;
	}

	mSwitchCounter[type] = math_max(mSwitchCounter[type], id);
	
	t_switch_variant::const_iterator it;
	it = mSwitchToVariant.find(type);
	if (it == mSwitchToVariant.end())
		return nullptr;
	return it->second->create(map, type, id);
}

Variant *VariantFactory::create(SerializeMap &map, t_variant_id type)
{
	auto itrCounter = mSwitchCounter.find(type);
	if (itrCounter == mSwitchCounter.end())
	{
		mSwitchCounter[type] = 0;
		itrCounter = mSwitchCounter.find(type);
	}
	return create(map, type, (*itrCounter).second++);
}

void VariantFactory::clear_counters()
{
	mSwitchCounter.clear();
}

void VariantFactory::register_variant(
	t_variant_id type,
	std::unique_ptr<VariantinatorBase> &&creator,
	size_t typeHash)
{
	mSwitchToVariant[type] = std::move(creator);
	mSwitchToVariant[type]->typeId = typeHash;
}