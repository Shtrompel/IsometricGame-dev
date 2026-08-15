#include "render/graphics.hpp"

// Static helper functions for JSON serialization
void to_json(json &j, const Orientation &v)
{
	j = {v.scale, v.tileSize, v.rotation, v.worldPos, v.zoomFactor};
}

void from_json(const json &j, Orientation &v)
{
	j.at(0).get_to(v.scale);
	j.at(1).get_to(v.tileSize);
	j.at(2).get_to(v.rotation);
	j.at(3).get_to(v.worldPos);
	j.at(4).get_to(v.zoomFactor);
}

// Orientation::to_string
std::string Orientation::to_string()
{
	std::stringstream ret;
	ret << "{";
	ret << "Scale : " << vec_str(scale);
	ret << ", ";
	ret << "Tile Pixel Size : " << vec_str(tileSize);
	ret << ", ";
	ret << "World rotation : " << rotation * 4 << " Degrees";
	ret << "}";
	return ret.str();
}

// RendererClass constructors
RendererClass::RendererClass()
{
}

RendererClass::RendererClass(
	sf::RenderWindow *window,
	sf::View *view,
	AssetManager *assets) : window(window), view(view), assets(assets)
{
}

// render_grid
void RendererClass::render_grid(Chunks *chunks)
{
	assert(window);
	assert(view);

	sf::Vector2f center = view->getCenter();
	sf::Vector2f size = view->getSize();

	// 1. Define the four corners of the visible screen
	sf::Vector2i corners[4] = {
		sf::Vector2i(center.x - size.x / 2.f, center.y - size.y / 2.f),
		sf::Vector2i(center.x + size.x / 2.f, center.y - size.y / 2.f),
		sf::Vector2i(center.x + size.x / 2.f, center.y + size.y / 2.f),
		sf::Vector2i(center.x - size.x / 2.f, center.y + size.y / 2.f)
	};

	int minTileX = INT_MAX, maxTileX = INT_MIN;
	int minTileY = INT_MAX, maxTileY = INT_MIN;

	// 2. Convert screen corners to tile coordinates to find the min/max bounds
	for (int i = 0; i < 4; ++i)
	{
		sf::Vector2i tilePos = screen_pos_to_tile_pos(corners[i], orientation);
		minTileX = std::min(minTileX, tilePos.x);
		maxTileX = std::max(maxTileX, tilePos.x);
		minTileY = std::min(minTileY, tilePos.y);
		maxTileY = std::max(maxTileY, tilePos.y);
	}

	// 3. Convert the tile bounds to chunk indices
	// A padding of 1 is added to prevent pop-in at the edges during camera movement
	int minChunkX = math_floordiv(minTileX, CHUNK_W) - 1;
	int maxChunkX = math_floordiv(maxTileX, CHUNK_W) + 1;
	int minChunkY = math_floordiv(minTileY, CHUNK_H) - 1;
	int maxChunkY = math_floordiv(maxTileY, CHUNK_H) + 1;

	sf::Sprite* grassSprite = assets->get_sprite(spriteGrass)->get(0);

	// 4. Iterate only through the visible chunks
	for (int x = minChunkX; x <= maxChunkX; ++x)
	{
		for (int y = minChunkY; y <= maxChunkY; ++y)
		{
			sub_render_grid(chunks, x, y, grassSprite);
		}
	}
}

// render_object
FRect RendererClass::render_object(GameBody* body, bool bSearch, ImageAlphaGrid* gridOut)
{
	
	if (!body->visible)
		return{};

	FVec bodyPos = body->pos;
	bodyPos.y -= body->z;
	
	sf::Vector2i pos = world_pos_to_screen_pos(
		bodyPos,
		orientation);

	if (!vec_inside<float>(
		(sf::Vector2f)pos,
		0,
		0,
		view->getSize().x,
		view->getSize().y))
	{
		return{};
	}
	
	t_sprite spriteHolder;
	if (body->type == BodyType::BUILDING)
	{
		BuildingBody* building = dynamic_cast<BuildingBody*>(body);
		assert(building);

		spriteHolder = building->get_sprite(orientation.rotation);
	}
	else
	{
		spriteHolder = body->spriteHolder;
	}

	if (spriteHolder == -1)
		return{};

	AssetSprites* sprites = assets
		->get_sprite(spriteHolder);

	sf::Sprite* sTile;
	size_t spriteId;
	if (body->type == BodyType::BULLET)
	{
		BulletBody* bullet;
		bullet = dynamic_cast<BulletBody*>(body);
		assert(bullet);

		spriteId = (size_t)bullet->get_direction_frame(orientation.rotation);
	}
	else
	{
		spriteId = body->animFrame;
	}
	sTile = sprites->get(spriteId);

	if (bSearch)
		*gridOut = sprites->getAlphaGrid(spriteId);

	// IVec textureSize = sprites->spriteSize;


	TextureOrigin texOrigin = TextureOrigin::BOTTOM;
	switch (body->type)
	{
	case BodyType::BULLET:
		texOrigin = TextureOrigin::CENTERED;
		break;
	default:
		break;
	}

	FVec posOut;
	if (sTile)
	{
		posOut = sub_render(
			pos,
			orientation.scale,
			*sTile,
			texOrigin);
	}

	if (body->type == BodyType::ENTITY && drawPath)
	{
		EntityBody* entity = dynamic_cast<EntityBody*>(body);
		assert(entity);
		sub_render_draw(entity->pathData, { 1.f, 1.f });
	}

	if (bSearch)
	{
		FVec size = orientation.scale * sprites->spriteSize;
		return{
			(FVec)posOut,
			size };
	}
	return {};

}

// render_influence_area (overload for UpgradeTree)
void RendererClass::render_influence_area(
	UpgradeTree *buildTree,
	const std::set<sf::Vector2i, IVecCompare> &suggestionBuilds)
{
	render_influence_area_generic(
		suggestionBuilds,
		[&buildTree]( sf::Vector2i& tilePos, sf::Vector2i& buildSize, float& radius, const sf::Vector2i& item)
		{
			buildSize = buildTree->size;
			radius = buildTree->effectRadius;
			tilePos = item;
		}
	);
}

// render_influence_area (overload for selectedBuildings)
void RendererClass::render_influence_area(
	const std::unordered_map<size_t, BuildingBase*>& selectedBuildings)
{
	render_influence_area_generic(
		selectedBuildings,
		[]( sf::Vector2i& tilePos, sf::Vector2i& buildSize, float& radius, const std::pair<size_t, BuildingBase*>& item)
		{
			BuildingBase* build = item.second;
			radius = build->effectRadius;
			buildSize = build->tilesSize;
			tilePos = build->tilePos;
		}
	);
}

// render_suggestion
void RendererClass::render_suggestion(
	UpgradeTree *buildTree,
	Chunks *chunks,
	GameMode gameMode,
	std::set<sf::Vector2i, IVecCompare> &suggestionBuilds)
{
	assert(window);
	assert(view);

	// Show tiles that are suggested to be build over /
	// become empty.
	if (gameMode == GameMode::BUILD && buildTree)
	{
		render_influence_area(buildTree, suggestionBuilds);
		
		for (auto itr = suggestionBuilds.begin();
			 itr != suggestionBuilds.end();
			 ++itr)
		{
			auto &tilePos = *itr;

			for (int x = 0; x < buildTree->size.x; ++x)
			{
				for (int y = 0; y < buildTree->size.y; ++y)
				{
					sf::Vector2i tileId = sf::Vector2i{x, y};
					sf::Vector2f addition = sf::Vector2f{.1f, .1f};
					addition = vec_mult(
						addition,
						vec_90_rotate_axis(-orientation.rotation));
					sf::Vector2f pos = sf::Vector2f(tilePos + tileId) +
									   sf::Vector2f{.5f, .5f} -
									   addition;
					sf::Vector2i drawPos = world_pos_to_screen_pos<float>(pos, orientation);
					sf::Vector2f tileDrawSize = orientation.tileSize * orientation.scale;

					if (buildTree->spriteHolders.empty() ||
						!vec_inside<float>(
							(sf::Vector2f)drawPos,
							-tileDrawSize.x,
							-tileDrawSize.y,
							view->getSize().x + 2.f * tileDrawSize.x,
							view->getSize().y + 2.f * tileDrawSize.y))
						continue;

					if (!buildTree->spriteHolders.count(tileId))
						continue;

					t_sprite holder = buildTree->spriteHolders.at(tileId);
					sf::Sprite *sTile = nullptr;
					if ((size_t)holder < assets->sprites.size())
						sTile = assets->sprites.at(holder)->get(0);

					if (!sTile)
						continue;
					
					float fx = ((orientation.rotation % 2 == 0) ? 1.f : -1.f);
					sub_render(
						drawPos, 
						{fx * orientation.scale.x, orientation.scale.y},
						*sTile,
						TextureOrigin::BOTTOM);
				}
			}
		}
	}
	else if (gameMode == GameMode::DELETE ||
			 gameMode == GameMode::UPGRADE)
	{
		for (auto &tilePos : suggestionBuilds)
		{
			BuildingBody *b = chunks->get_build(tilePos.x, tilePos.y);
			sf::Vector2f pos = (sf::Vector2f)tilePos + sf::Vector2f{.5f, .5f};
			sf::Vector2i drawPos = world_pos_to_screen_pos<float>(pos, orientation);
			sf::Sprite *sIcon;

			if (b)
			{
				drawPos.y -= 20;
				sIcon = assets->sprites.at(spriteIconDelete)->get(0);
			}
			else
			{
				drawPos.y -= (int)(5.0f * orientation.scale.y);
				sIcon = assets->sprites.at(spriteTileSelected)->get(0);
			}
			sub_render(
				drawPos, 
				0.5f * orientation.scale,
				*sIcon, 
				TextureOrigin::BOTTOM);
		}
	}
}

// render_gui
void RendererClass::render_gui(
	GameData &data,
	float fps)
{
	assert(window);
	assert(view);

	sf::CircleShape cs;
	cs.setFillColor(sf::Color::Green);
	cs.setRadius(16.f);
	cs.setOrigin({16, 16});
	cs.setPosition(view->getCenter());
	window->draw(cs);

	static char infoRes[60] = "";
	sprintf(
		infoRes,
		"Power: %d, Ore: %d, Gems: %d, Bodies: %d",
		data.resourceContext.power,
		data.resourceContext.resources[Resources::ORE],
		data.resourceContext.resources[Resources::GEMS],
		data.resourceContext.resources[Resources::BODIES]);
	static char fpsCStr[8];
	sprintf(fpsCStr, "%.3f", fps);
}

// sub_render_grid
void RendererClass::sub_render_grid(Chunks *chunks, int idx, int idy, sf::Sprite *tileSprite)
{
	sf::Sprite *sprite = nullptr;
	Grid *grid = chunks->get(idx, idy);
	if (!grid)
		return;

	for (int x = 0; x < grid->cx; x++)
	{
		for (int y = 0; y < grid->cy; y++)
		{
			sf::Vector2i tilePos = {x, y};

			tilePos += {
				grid->idx * CHUNK_W,
				grid->idy * CHUNK_H};

			sf::Vector2i drawPos = world_pos_to_screen_pos(tilePos, orientation);
			sf::Vector2f tileDrawSize = orientation.tileSize * orientation.scale;
			bool doRender = (*grid)(x, y).visible;
			doRender &= vec_inside<float>(
				(sf::Vector2f)drawPos,
				-tileDrawSize.x,
				-tileDrawSize.y,
				view->getSize().x + 2 * tileDrawSize.x,
				view->getSize().y + 2 * tileDrawSize.y);

			if (!doRender)
				continue;

			Tile &tile = (*grid)(x, y);
			BuildingBody *building = tile.building;

			// gridDraw2.rotation = WorldRotation::ROT0;
			bool bMouseTile = tilePos == screen_pos_to_tile_pos(
												 (sf::Vector2i)view->getCenter(),
												 orientation);
			if (bMouseTile)
				sprite = assets->get_sprite(spriteTileSelected)->get(0);
			else if ((building && building->base->buildType == (t_id)BuildingType::ROAD))
				sprite = assets->get_sprite(building->spriteHolder)->get(0);
			else
				sprite = tileSprite;

			if (tileSprite)
			{
				sub_render_tile(drawPos, orientation.scale, *sprite);
			}
		}
	}
}

// sub_render_draw
void RendererClass::sub_render_draw(
	PathData &pathData,
	sf::Vector2f scale,
	bool relevantSegment)
{
	if (!pathData.valid())
		return;
	sf::CircleShape circle;
	circle.setRadius(10.f);
	circle.setOrigin({circle.getRadius(), circle.getRadius()});
	circle.setFillColor(sf::Color::Yellow);
	circle.setOutlineColor(sf::Color::Black);

	auto &path = pathData.path;
	sf::Vector2f a, b;
	bool first = 0;
	auto itr = path.cbegin();
	if (relevantSegment)
		itr = pathData.follower;
	for (; itr != path.end(); ++itr)
	{
		a = (sf::Vector2f)(*itr).value + sf::Vector2f{.5f, .5f};
		if (first)
		{
			sf::Vector2f pos1 = a;
			sf::Vector2f pos2 = b;
			pos1 = (sf::Vector2f)world_pos_to_screen_pos(pos1, orientation);
			pos2 = (sf::Vector2f)world_pos_to_screen_pos(pos2, orientation);
			sf::Vertex line[2];
			line[0].position = pos1;
			line[1].position = pos2;
			window->draw(line, 2, sf::PrimitiveType::Lines);
		}
		{
			auto pos = world_pos_to_screen_pos(a, orientation);
			circle.setPosition((sf::Vector2f)pos);
			window->draw(circle);
		}

		first = 1;
		b = a;
	}
}