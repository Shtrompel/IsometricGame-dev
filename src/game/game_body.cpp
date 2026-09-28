#include "game/game_body.hpp"
#include "game/game_data.hpp"
#include "utils/class/logger.hpp"
#include <sstream>
#include <cassert>

// -------------------------
// GameBodyConfig
// -------------------------

std::string GameBodyConfig::to_string()
{
    return "to_string() unimplemented";
}

// -------------------------
// BodyConfigManager
// -------------------------

BodyConfigManager::BodyConfigManager()
{
}

BodyConfigManager::iterator BodyConfigManager::insert(const t_idpair &pair, const t_type &type)
{
    t_pair_idt::iterator itrA;
    t_idt::iterator itrB;
    if ((itrA = data.find(pair)) == data.end())
        itrA = data.insert({pair, t_idt{}}).first;
    t_idt &idt = (*itrA).second;
    if ((itrB = idt.find((t_id)0)) == idt.end())
        itrB = idt.insert({0, type}).first;
    return iterator{&data, itrA, &idt, itrB};
}

BodyConfigManager::iterator BodyConfigManager::insert(const t_idpair &pair, const t_id id, const t_type &type)
{
    t_pair_idt::iterator itrA;
    t_idt::iterator itrB;
    if ((itrA = data.find(pair)) == data.end())
        itrA = data.insert({pair, t_idt{}}).first;
    t_idt &idt = (*itrA).second;
    if ((itrB = idt.find(id)) == idt.end())
        itrB = idt.insert({id, type}).first;
    return iterator{&data, itrA, &idt, itrB};
}

BodyConfigManager::t_type BodyConfigManager::at(const t_idpair &pair)
{
    return data.at(pair).at(0);
}

BodyConfigManager::t_type BodyConfigManager::at(const t_idpair &pair, const t_id id)
{
    return data.at(pair).at(id);
}

BodyConfigManager::iterator BodyConfigManager::begin()
{
    return iterator{
        &data,
        data.begin(),
        &(*data.begin()).second,
        (*data.begin()).second.begin()};
}

BodyConfigManager::iterator BodyConfigManager::end()
{
    auto last = std::next(data.begin(), data.size()-1);
    return iterator{
        &data,
        last,
        &(*last).second,
        (*last).second.end()};
}

// -------------------------
// AbstractCanTarget
// -------------------------

GameBody* AbstractCanTarget::get_target() 
{
    return nullptr;
}

void AbstractCanTarget::set_target(GameBody*) 
{
}

// -------------------------
// GameBody
// -------------------------

GameBody::GameBody()
{
    this->target = nullptr;
    this->followers.clear();
}

GameBody::GameBody(
    GameData *context,
    size_t e,
    size_t id,
    const sf::Vector2f &pos,
    const t_sprite &sprite,
    const BodyType &type,
    const sf::Vector2f &rect)
    : Variant(e, id)
{
    this->context = context;
    this->pos = pos;
    this->rectSize = rect;
    this->type = type;
    this->spriteHolder = sprite;

    if (!isNoiseInit)
    {
        noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
        noise.SetFrequency(1.f);
        isNoiseInit = true;
    }
}

std::string GameBody::get_ptr_str() const
{
    std::ostringstream oss;
    oss << (void*)this;
    std::string s(oss.str());
    return s;
}

int GameBody::get_hp() const 
{ 
    assert(0); 
    return -1; 
}

void GameBody::set_hp(int value)
{
    assert(0);
}

GameBody* GameBody::get_target()
{
    return target;
}

FVec GameBody::get_pos() 
{ 
    return this->pos;  
}

void GameBody::apply_data(GameBodyConfig *config) 
{
}

void GameBody::update() 
{
}

void GameBody::update(float delta) 
{
}

bool GameBody::operator<(const GameBody &other) const
{
	// Depth sorting for rendering
	return this->depth() < other.depth();
}

void GameBody::change_sprite(t_sprite sprite)
{
    if (sprite != (t_sprite)-1 && sprite != 0)
        this->spriteHolder = sprite;
}

void GameBody::variant_initialize() 
{
    isNoiseInit = 0;
    z = 0.0f;
    type = BodyType::NONE;
    source = nullptr;
    shape = ShapeType::POINT;
    visible = true;
    alignment = ALIGNMENT_NONE;
    start = { -1, -1 };
    end = { -1, -1 };
    spriteHolder = -1;
    animFrame = 0u;
}

json GameBody::to_json() const 
{
    json j;
    
    // AI FIX: Explicitly create a JSON object to bypass the json_ref dangling pointer bug
    j["body"] = json::object(); 
    
    j["body"]["pos"] = pos;
    j["body"]["rectSize"] = rectSize;
    j["body"]["type"] = type;
    j["body"]["visible"] = visible;
    j["body"]["start"] = start;
    j["body"]["end"] = end;
    j["body"]["spriteHolder"] = spriteHolder;
    j["body"]["animFrame"] = animFrame;
    j["body"]["radius"] = radius;
    j["body"]["alignment"] = alignment;
    j["body"]["arrUseEnums"] = arrUseEnums;
    j["body"]["propertyVals"] = propertyVals;
    j["body"]["dead"] = this->dead;
    j["body"]["vel"] = this->vel;

    // AI FIX: Check objectType instead of .get() so unloaded pointers aren't wiped out
    if (target.is_valid()) {
        j["body"]["target"] = target.to_json_ptr();
        DEBUG("to_json: body id=%zu type=%zu writing OWN target id=%zu type=%zu",
            this->objectId, this->objectType, target.objectId, target.objectType);
    } else {
        j["body"]["target"] = nullptr;
    }

    if (!followers.empty()) {
        json fArr = json::array();
        for (const auto& f : followers) {
            // Safely push only the IDs
            if (f.is_valid()) {
                fArr.push_back(f.to_json_ptr());
                
                // Debugging
                DEBUG("to_json: body id=%zu type=%zu writing follower id=%zu type=%zu (follower->target id=%zu type=%zu valid=%d)",
                    this->objectId, this->objectType,
                    f.objectId, f.objectType,
                    f.get() ? f.get()->target.objectId : (size_t)-1,
                    f.get() ? f.get()->target.objectType : (size_t)-1,
                    f.get() ? f.get()->target.is_valid() : -1);
            } else {
                DEBUG("to_json: body id=%zu type=%zu SKIPPING an invalid follower entry (id=%zu type=%zu)",
                    this->objectId, this->objectType, f.objectId, f.objectType);
            }
        }
        j["body"]["followers"] = fArr;
    } else {
        j["body"]["followers"] = nullptr;
    }

    return j;
}

void GameBody::from_json(const json &j0) 
{
    auto &j = j0.at("body");

    this->followers.clear();
    this->target = nullptr;
    
    j.at("pos").get_to(this->pos);
    j.at("rectSize").get_to(this->rectSize);
    j.at("type").get_to(this->type);
    j.at("visible").get_to(this->visible);
    j.at("start").get_to(this->start);
    j.at("end").get_to(this->end);
    j.at("spriteHolder").get_to(this->spriteHolder);
    j.at("animFrame").get_to(this->animFrame);
    j.at("radius").get_to(this->radius);
    j.at("alignment").get_to(this->alignment);

    this->followers.clear();
    this->followers.shrink_to_fit();
    if (j.contains("followers") && !j.at("followers").is_null())
        j.at("followers").get_to(this->followers);

    this->target = nullptr;
    if (j.contains("target") && !j.at("target").is_null())
    {
        j.at("target").get_to(this->target);
        //DEBUG("from_json: body id=%zu type=%zu (raw json target: %s) read OWN target id=%zu type=%zu",
        //    this->objectId, this->objectType, j.at("target").dump().c_str(),
        //    this->target.objectId, this->target.objectType);
    }

    j.at("arrUseEnums").get_to(this->arrUseEnums);
    j.at("propertyVals").get_to(this->propertyVals);
    j.at("dead").get_to(this->dead);
    j.at("vel").get_to(this->vel);
}

void GameBody::serialize_publish(const SerializeMap &map) 
{
	for (VariantPtr<GameBody>& b : followers)
	{
		if (b.is_valid())
		{
			size_t wantId = b.objectId, wantType = b.objectType;
			bool linked = map.apply(b);
			
            // Debugging
            DEBUG("serialize_publish: body id=%zu type=%zu linking follower id=%zu type=%zu -> %s (resolved target on linked body: id=%zu type=%zu valid=%d)",
				this->objectId, this->objectType, wantId, wantType,
				linked ? "OK" : "FAILED",
				linked && b.get() ? b.get()->target.objectId : (size_t)-1,
				linked && b.get() ? b.get()->target.objectType : (size_t)-1,
				linked && b.get() ? b.get()->target.is_valid() : -1);
		}
	}
	
	if (target.is_valid()) {
        size_t wantId = target.objectId, wantType = target.objectType;
        bool linked = map.apply(target);
        DEBUG("serialize_publish: body id=%zu type=%zu linking OWN target id=%zu type=%zu -> %s (target resolved to address=%p)",
            this->objectId, this->objectType, wantId, wantType,
            linked ? "OK" : "FAILED", (void*)target.get());
    }
}

void GameBody::serialize_initialize(const SerializeMap &map) 
{
}

bool GameBody::confirm_body(GameData* context)
{
    return true;
}

std::string GameBody::test_get_str() const
{
	std::stringstream ret;
	ret << "{ ";
	ret << context->enum_get_str(this);
	ret << ", ";
	ret << this->alignment;
	ret << ", ";
	ret << (void const*)this;
	ret << " }";
	return ret.str();

}

void GameBody::set_target(GameBody* newTarget, bool skipOld)
{
	// An object can't make inself it's own target
	assert(newTarget != this);
	assert(target != this);

	if (target == newTarget)
		return;

	GameBody* oldTarget = this->target;

	// Remove this object as a follower from the old target
	if (!skipOld && this->target != nullptr)
	{
		std::vector<VariantPtr<GameBody>>& followersRef =
			this->target->followers;

		if (this->target != newTarget && followersRef.size())
		{
			for (auto itr = followersRef.begin(); itr != followersRef.end();)
			{
				VariantPtr<GameBody> bodyPtr = *itr;
				if (bodyPtr == this)
				{
					itr = followersRef.erase(itr);
				}
				else
				{
					++itr;
				}
			}
		}
	}

	// Add this object as a follower to the new target
	if (newTarget != nullptr)
	{
		std::vector<VariantPtr<GameBody>>& followersRef =
			newTarget->followers;

		bool addFollower = true;
		if (followersRef.size())
		{

			auto itr = std::find_if(
				followersRef.begin(),
				followersRef.end(),
				[this](const VariantPtr<GameBody>& ptr)
				{
					return ptr == this;
				});
			addFollower = itr == followersRef.end();
		}

		if (addFollower)
		{
			followersRef.push_back(this);
		}
	}

	// Set the new target
	this->target = newTarget;

	// Invariant: this must now be linked from exactly the right side(s).
	// In debug this halts immediately at the point of corruption; in
	// release it logs and repairs the drift instead of leaving it to be
	// discovered later by GameData::validate_state().
	if (newTarget != nullptr)
	{
		bool linked = std::find(
			newTarget->followers.begin(),
			newTarget->followers.end(),
			this) != newTarget->followers.end();
		ASSERT_ERROR(linked, "set_target: this not found in newTarget->followers after linking");
#ifdef NDEBUG
		if (!linked)
			newTarget->followers.push_back(this);
#endif
	}

	if (!skipOld && oldTarget != nullptr && oldTarget != newTarget)
	{
		bool unlinked = std::find(
			oldTarget->followers.begin(),
			oldTarget->followers.end(),
			this) == oldTarget->followers.end();
		ASSERT_ERROR(unlinked, "set_target: this still found in oldTarget->followers after unlinking");
#ifdef NDEBUG
		if (!unlinked)
		{
			auto& followersRef = oldTarget->followers;
			followersRef.erase(
				std::remove(followersRef.begin(), followersRef.end(), this),
				followersRef.end());
		}
#endif
	}
}