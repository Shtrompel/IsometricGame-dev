#ifndef _GAME_BULLET
#define _GAME_BULLET

#include "game_body.hpp"
#include "../file/serialization.hpp"

struct BulletStats : GameBodyConfig
{
	int id = -1;
	t_idpair type;

	t_alignment alignment = ALIGNMENT_NONE;
	float damage = 0.f;
	float lifeSpan = 1.0;
	float maxDist = 0;
	float maxSpeed = 0.0;
	float maxForce = 0.0;
	float radius = 0.0f;
	t_sprite sprite = 0;


	PropertySet<BulletPropertyBools, BulletPropertyNums> props;

	nlohmann::json to_json() const
	{
		nlohmann::json ret;
		
		ret["id"] = id;
		ret["type"] = { type.group, type.id };
		ret["alignment"] = alignment;
		ret["damage"] = damage;
		ret["lifeSpan"] = lifeSpan;
		ret["maxDist"] = maxDist;
		ret["maxSpeed"] = maxSpeed;
		ret["maxForce"] = maxForce;
		ret["radius"] = radius;
		ret["sprite"] = sprite;
		ret["props"] = props;

		return ret;
	}

	std::string to_string()
	{
		return to_json().dump(1).c_str();
	}

};

struct BulletBody : public GameBody
{
	BulletBody();
	
	BulletBody(
		GameData *context,
		size_t id,
		const sf::Vector2f &pos,
		const sf::Vector2f &vel,
		t_sprite spriteHolder,
		t_id alignment);

	int get_hp() const override;

	void apply_data(GameBodyConfig* config) override;

	bool confirm_body(GameData* context) override;

	int get_direction_frame(int rotation = 0);

	virtual void update(float delta) override;

	void logic_reset() {}

	json to_json() const override;

	void from_json(const json &j) override;

	void serialize_publish(const SerializeMap &map) override;
	
	void serialize_initialize(const SerializeMap &map) override;
	
public:

	t_bodies_itr<VariantPtr<BulletBody>> objItrBullet;
	
	FVec force = { 0.f, 0.f };
	FVec dir;

	float damage = 10;
	float lifeSpan = 0;
	float maxDist = 0;
	float maxSpeed = 0;
	float maxForce = 0;

	t_body_timer lifeTimer;


	//std::vector<VariantPtr<GameBody>> nearbyObjects;
	PropertySet<BulletPropertyBools, BulletPropertyNums> props;

};

#endif // _GAME_BULLET