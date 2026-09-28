#include "game/game_bullet.hpp"

#include "game/game_data.hpp"

BulletBody::BulletBody() :
	GameBody(nullptr, -1, 0, { 0.0f, 0.0f }, -1, BodyType::BULLET,
		ENTITY_COLLISION_SIZE)
{
}

BulletBody::BulletBody(
	GameData* context,
	size_t id,
	const sf::Vector2f& pos,
	const sf::Vector2f& dir,
	t_sprite spriteHolder,
	t_id alignment)
	: GameBody(
		context,
		-1,
		id,
		pos,
		spriteHolder,
		BodyType::BULLET,
		ENTITY_COLLISION_SIZE)
{
	this->dir = dir;
	this->alignment = alignment;
	this->z = 0.0f;
	this->radius = 0.1f;
}

int BulletBody::get_hp() const {
	return INT_MAX;
}

void BulletBody::apply_data(GameBodyConfig* config)
{
	if (static_cast<EntityStats*>(config) == nullptr)
	{
		WARNING(
			"In function %s invalid type of input function parameter, should be of type GameBodyConfig",
			__PRETTY_FUNCTION__);
		return;
	}

	auto& stats = *static_cast<BulletStats*>(config);

	this->alignment = stats.alignment;
	this->damage = stats.damage;
	this->lifeSpan = stats.lifeSpan;
	this->maxDist = stats.maxDist;
	this->maxSpeed = stats.maxSpeed;
	
	this->maxForce = stats.maxForce;
	this->spriteHolder = stats.sprite;
	this->radius = stats.radius;
	this->isLightSource = stats.isLightSource;

	this->props.append(stats.props);

	assert(config);
}

bool BulletBody::confirm_body(GameData* context)
{
	this->context = context;
	context->bodiesContext.bodies.push_back(dynamic_cast<GameBody*>(this));
	return true;
}

int BulletBody::get_direction_frame(int rotation)
{
	float angle = atan2f(dir.y, dir.x);
	angle += (float)M_PI / 2.f;
	angle -= (float)M_PI_2 / 8.f;
	angle = math_mod(angle, (float)M_2PI);
	angle = math_map(angle, 0.0f, (float)M_2PI, 0.f, 8.f);
	if (rotation % 2 == 1)
		angle += (float)M_PI_2;
	return math_mod((int)angle, 8);
}

void BulletBody::update(float delta)
{
	 if (dead)
		 return;

	if (lifeTimer->time_passed(context->get_time()) > lifeSpan)
	{
		dead = true;
		return;
	}


	struct BulletCondition
	{
		size_t alignment;
		BulletCondition(size_t alignment)
		{
			this->alignment = alignment;
		}

		inline bool operator()(const GameBody* b, const int, const int) const
		{
			bool ret = GameData::alignment_compare(b->alignment, alignment) ==
				ALIGNMENTS_ENEMIES;
			return ret;
		}
	};

	if (props.bool_is(BulletPropertyBools::HOMING))
	{
		if (!target)
		{
			size_t targetAlgn = GameData::alignment_opposite(this->alignment);
			if (targetAlgn != ALIGNMENT_NONE)
			{
				// Find a new target
				auto nearLst = context->nearest_bodies_quad(
					this->pos,
					1,
					{ ENUM_ALIGNMENT, targetAlgn },
					[=](const sf::Vector2i&, const GameBody* b, const int)
					{
						if (b->dead &&
							b->type == BodyType::BUILDING &&
							props.bool_is(BulletPropertyBools::SPECTRAL))
							return false;
						return true;
					}
				);
				if (nearLst.size())
				{
					set_target(nearLst.back());
				}
			}
		}
		// If target was found...
		if (target)
		{
			FVec dif = target.get()->pos - this->pos;
			if (dif != FVec{ 0.0f, 0.0f })
			{
				float ls = vec_normalize(dif);
				this->dir = dif;
				force = dif;
				vec_limit(force, maxForce);
				vel += force;
				vec_limit(vel, maxSpeed);
			}
			else
			{
				vel = { 0.0f, 0.0f };
			}
		}
	}
	else
	{
		vec_normalize(vel);
		vel *= maxSpeed;
	}

	FVec newPos = this->pos + vel * delta;
	this->pos = newPos;

	std::list<GameBody*> nearbyBodies = this->context->nearest_bodies_radius(
		this->pos,
		this->radius,
		1ull,
		BulletCondition{this->alignment}
	);

	if (nearbyBodies.empty())
		return;

	GameBody* gb = nearbyBodies.front();
	GameData::damage_body(gb, this->damage);
	this->dead = true;

	if (gb->get_hp() <= 0)
	{
		this->set_target(nullptr);
	}
}

json BulletBody::to_json() const
{
	json ret = GameBody::to_json();

	ret["lifeTimer"] = lifeTimer;

	return ret;
}

void BulletBody::from_json(const json &j)
{
	GameBody::from_json(j);

	if (!lifeTimer.get())
	{
		lifeTimer = t_time_manager::make_independent_timer(0.0f);
	}

	if (j.count("lifeTimer"))
		j.at("lifeTimer").get_to(lifeTimer);
}

void BulletBody::serialize_publish(const SerializeMap &map)
{
	GameBody::serialize_publish(map);
	
	GameData* g = map.get<GameData>(SERIALIZABLE_DATA, 0);
	assert(g);


	if (!lifeTimer.get())
	{
		lifeTimer = t_time_manager::make_independent_timer(g->get_time());
		lifeTimer->init(g->get_time());
	}
	g->add_timer(lifeTimer);

	map.apply(this->target);
}

void BulletBody::serialize_initialize(const SerializeMap& map)
{
	GameBody::serialize_initialize(map);

	GameData* g = map.get<GameData>(SERIALIZABLE_DATA, 0);
	assert(g);

	this->confirm_body(g);
}