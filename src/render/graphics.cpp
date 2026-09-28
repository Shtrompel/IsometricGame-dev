#include "render/graphics.hpp"

#include <cmath>
#include <cstdlib>

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

sf::RenderStates RendererClass::get_render_states(sf::Sprite* sprite)
{
	sf::RenderStates rs;
	if (renderShaders)
	{
		rs.shader = &shader;
		sf::Vector2u texSize = sprite->getTexture().getSize();
		sf::Vector2f textureOffset =  sf::Vector2f(1.0f / texSize.x, 1.0f / texSize.y);
		textureOffset *= orientation.scale.x;
		shader.setUniform("texOffset", textureOffset);
	}
	return rs;
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
		sf::Vector2i(center.x - size.x / 2.f, center.y + size.y / 2.f)};

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

	sf::Sprite *grassSprite = assets->get_sprite(spriteGrass)->get(0);

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
FRect RendererClass::render_object(GameBody *body, bool bSearch, ImageAlphaGrid *gridOut)
{
	if (!body->visible || body->dead) 
        return {};

	if (!body->visible)
		return {};

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
		return {};
	}

	t_sprite spriteHolder;
	if (body->type == BodyType::BUILDING)
	{
		BuildingBody *building = dynamic_cast<BuildingBody *>(body);
		assert(building);

		spriteHolder = building->get_sprite(orientation.rotation);
	}
	else
	{
		spriteHolder = body->spriteHolder;
	}

	if (spriteHolder == -1)
	{
		return {};
	}

	AssetSprites *sprites = assets
								->get_sprite(spriteHolder);

	if (body->type == BodyType::ENTITY && !sprites)
		DEBUG("render_object: entity id=%zu spriteHolder=%d resolved to null AssetSprites", body->objectId, (int)spriteHolder);

	// Get sprite from the identifier
	sf::Sprite *sTile;
	size_t spriteId;
	// Unique behaviour from the bullet's direction
	if (body->type == BodyType::BULLET)
	{
		BulletBody *bullet;
		bullet = dynamic_cast<BulletBody *>(body);
		assert(bullet);

		spriteId = (size_t)bullet->get_direction_frame(orientation.rotation);
	}
	// Unique behabiour from a building's sprite state space
	else if (body->type == BodyType::BUILDING)
	{
		BuildingBody *buildingBody = dynamic_cast<BuildingBody *>(body);
		assert(buildingBody && buildingBody->base);
		spriteId = (size_t)buildingBody->base->get_sprite_frame_index();
	}
	else if (body->type == BodyType::ENTITY)
	{
		EntityBody *entity = dynamic_cast<EntityBody *>(body);
		assert(entity);
		int cols = std::max(sprites->animGrid.x, 1);
		int dirIndex = entity->get_direction_frame(orientation.rotation);

		// Code made by Claude Sonnet 5 - whether this plays back and forth
		// instead of looping is a property of the sprite sheet itself
		// (textures.json's "anim_mode"), not of the entity's current action
		int frameInSeq = sprites->fold_frame((int)body->animFrame, cols);

		spriteId = (size_t)(dirIndex * cols + frameInSeq);
	}
	else
	{
		spriteId = body->animFrame;
	}
	sTile = sprites->get(spriteId);
	if (body->type == BodyType::ENTITY && !sTile)
		DEBUG("render_object: entity id=%zu spriteId=%zu -> sprites->get() returned null", body->objectId, spriteId);

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
		sf::Color oldColor = sTile->getColor();

		if (halfWalls && body->type == BodyType::BUILDING)
		{
			BuildingBody *b = dynamic_cast<BuildingBody *>(body);
			if (b && b->base->buildType == (t_id)BuildingType::WALL)
			{
				sf::Color c = oldColor;
				c.a = 64;
				sTile->setColor(c);
			}
		}

		// Code made by Claude Sonnet 5 - berserk entities are drawn tinted red until they get their own sprite set
		if (body->type == BodyType::ENTITY)
		{
			EntityBody *e = dynamic_cast<EntityBody *>(body);
			if (e && e->isBerserk)
				sTile->setColor(sf::Color(255, 90, 90, oldColor.a));
		}

		posOut = sub_render(
			pos,
			orientation.scale,
			*sTile,
			texOrigin);

		// Reset the color
		sTile->setColor(oldColor);
	}

	if (body->type == BodyType::ENTITY && drawPath)
	{
		EntityBody *entity = dynamic_cast<EntityBody *>(body);
		assert(entity);
		sub_render_draw(entity->pathData, {1.f, 1.f});
	}

	if (bSearch)
	{
		FVec size = orientation.scale * sprites->spriteSize;
		return {
			(FVec)posOut,
			size};
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
		[&buildTree](sf::Vector2i &tilePos, sf::Vector2i &buildSize, float &radius, const sf::Vector2i &item)
		{
			buildSize = buildTree->size;
			radius = buildTree->effectRadius;
			tilePos = item;
		});
}

// render_influence_area (overload for selectedBuildings)
void RendererClass::render_influence_area(
	const std::unordered_map<size_t, BuildingBase *> &selectedBuildings)
{
	render_influence_area_generic(
		selectedBuildings,
		[](sf::Vector2i &tilePos, sf::Vector2i &buildSize, float &radius, const std::pair<size_t, BuildingBase *> &item)
		{
			BuildingBase *build = item.second;
			radius = build->effectRadius;
			buildSize = build->tilesSize;
			tilePos = build->tilePos;
		});
}

// render_suggestion
void RendererClass::render_suggestion(
	UpgradeTree *buildTree,
	Chunks *chunks,
	GameMode gameMode,
	std::set<sf::Vector2i, IVecCompare> &suggestionBuilds,
	int affordableCount)
{
	assert(window);
	assert(view);

	// Show tiles that are suggested to be build over /
	// become empty.
	if (gameMode == GameMode::BUILD && buildTree)
	{
		render_influence_area(buildTree, suggestionBuilds);

		int count = 0;
		for (auto itr = suggestionBuilds.begin();
			 itr != suggestionBuilds.end();
			 ++itr, ++count)
		{
			auto &tilePos = *itr;
			bool affordable = (affordableCount == -1 || count < affordableCount);

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

					if (!affordable) {
						sTile->setColor(sf::Color(255, 100, 100, 120));
					} else {
						sTile->setColor(sf::Color(255, 255, 255, 100));
					}

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

void RendererClass::prepare_lighting(const std::vector<GameBody *> &lightEmittingBodies)
{
	std::vector<sf::Vector2f> positions;
    std::vector<sf::Vector3f> colors;
    std::vector<float> radii;

    shadowPathtrace.lights.clear();

    int lightCount = 0;
    for (GameBody* body : lightEmittingBodies)
    {
        if (lightCount >= 16) break; // Max lights defined in shader

		//if (body->lightRadius <= 0 || (body->lightColor) == sf::Vector3f{0.0f, 0.0f, 0.0f})
		{
		//	continue;
		}

        // Convert the object's 3D isometric world position to 2D screen coordinates
        FVec adjustedPos = body->pos;
        adjustedPos.y -= body->z;
        sf::Vector2i screenPos = world_pos_to_screen_pos(adjustedPos, orientation);
        
        // Important: gl_FragCoord treats the bottom-left of the window as (0,0). 
        // SFML treats top-left as (0,0). We must invert the Y axis for the shader.
        float invertedY = window->getSize().y - screenPos.y;
		
        positions.push_back(sf::Vector2f(screenPos.x-50, invertedY+50));
        colors.push_back(body->lightColor);
        radii.push_back(body->lightRadius * orientation.scale.x);

        // Code made by Claude Sonnet 5 - remember the light in top-down screen coordinates for the sheared shadow pass
        shadowPathtrace.lights.push_back({
            sf::Vector2f(screenPos.x - 50.f, screenPos.y - 50.f),
            body->lightColor,
            body->lightRadius * orientation.scale.x});
        
        lightCount++;
    }

    // Bind to the SFML Shader
    shader.setUniform("numLights", lightCount);
    if (lightCount > 0)
    {
        shader.setUniformArray("lightPositions", positions.data(), lightCount);
        shader.setUniformArray("lightColors", colors.data(), lightCount);
        shader.setUniformArray("lightRadii", radii.data(), lightCount);
    }

}

// Code made by Claude Sonnet 5 - allocates the ping-ponged shadow render targets and loads the pathtrace/accumulate shaders
void RendererClass::init_shadow_targets(unsigned width, unsigned height)
{
	(void)shadowPathtrace.occluderTarget.resize({width, height});
	(void)shadowPathtrace.sampleTarget.resize({width, height});
	(void)shadowPathtrace.accum[0].resize({width, height});
	(void)shadowPathtrace.accum[1].resize({width, height});

	shadowPathtrace.accum[0].clear(sf::Color::White);
	shadowPathtrace.accum[0].display();
	shadowPathtrace.accum[1].clear(sf::Color::White);
	shadowPathtrace.accum[1].display();

	bool ok = shadowPathtrace.lightShader.loadFromFile(
		"assets/shaders/testVert.vert",
		"assets/shaders/shadow_light.frag");
	ok = shadowPathtrace.castShader.loadFromFile(
		"assets/shaders/testVert.vert",
		"assets/shaders/shadow_cast.frag") && ok;
	ok = shadowPathtrace.accumulateShader.loadFromFile(
		"assets/shaders/testVert.vert",
		"assets/shaders/shadow_accumulate.frag") && ok;
	ok = shadowPathtrace.occluderCutoutShader.loadFromFile(
		"assets/shaders/testVert.vert",
		"assets/shaders/occluder_cutout.frag") && ok;

	if (ok)
	{
		shadowPathtrace.castShader.setUniform("u_texture", sf::Shader::CurrentTexture);
		shadowPathtrace.occluderCutoutShader.setUniform("u_texture", sf::Shader::CurrentTexture);
	}

	shadowPathtrace.ready = ok;
}

// Code made by Claude Sonnet 5 - gathers the barrier sprites standing on the ground (with a margin, since their shadows reach into view) for the shadow pass
void RendererClass::collect_shadow_casters(Chunks *chunks)
{
	shadowPathtrace.casters.clear();

	if (!shadowPathtrace.ready)
		return;

	bool cameraChanged =
		!shadowPathtrace.haveLastOrientation ||
		orientation.worldPos != shadowPathtrace.lastWorldPos ||
		orientation.rotation != shadowPathtrace.lastRotation ||
		orientation.scale != shadowPathtrace.lastScale;

	if (cameraChanged)
	{
		shadowPathtrace.justReset = true;

		shadowPathtrace.haveLastOrientation = true;
		shadowPathtrace.lastWorldPos = orientation.worldPos;
		shadowPathtrace.lastRotation = orientation.rotation;
		shadowPathtrace.lastScale = orientation.scale;
	}

	sf::Vector2f center = view->getCenter();
	sf::Vector2f size = view->getSize();

	sf::Vector2i corners[4] = {
		sf::Vector2i(center.x - size.x / 2.f, center.y - size.y / 2.f),
		sf::Vector2i(center.x + size.x / 2.f, center.y - size.y / 2.f),
		sf::Vector2i(center.x + size.x / 2.f, center.y + size.y / 2.f),
		sf::Vector2i(center.x - size.x / 2.f, center.y + size.y / 2.f)};

	int minTileX = INT_MAX, maxTileX = INT_MIN;
	int minTileY = INT_MAX, maxTileY = INT_MIN;
	for (int i = 0; i < 4; ++i)
	{
		sf::Vector2i tilePos = screen_pos_to_tile_pos(corners[i], orientation);
		minTileX = std::min(minTileX, tilePos.x);
		maxTileX = std::max(maxTileX, tilePos.x);
		minTileY = std::min(minTileY, tilePos.y);
		maxTileY = std::max(maxTileY, tilePos.y);
	}

	int margin = shadowPathtrace.tileMargin;
	for (int x = minTileX - margin; x <= maxTileX + margin; ++x)
	{
		for (int y = minTileY - margin; y <= maxTileY + margin; ++y)
		{
			const Tile *tile = chunks->get_tile_safe(x, y);
			if (!tile || !tile->is_barrier())
				continue;

			BuildingBody *building = tile->building;
			if (!building)
				continue;

			t_sprite spriteHolder = building->get_sprite(orientation.rotation);
			if (spriteHolder == -1)
				continue;

			AssetSprites *sprites = assets->get_sprite(spriteHolder);
			if (!sprites)
				continue;

			size_t spriteId = (size_t)building->base->get_sprite_frame_index();
			sf::Sprite *sTile = sprites->get(spriteId);
			if (!sTile)
				continue;

			FVec bodyPos = building->pos;
			bodyPos.y -= building->z;
			sf::Vector2i screenPos = world_pos_to_screen_pos(bodyPos, orientation);

			shadowPathtrace.casters.push_back({sTile, (sf::Vector2f)screenPos});
		}
	}
}

// Code made by Claude Sonnet 5 - per light: shear every caster's alpha-masked sprite away from the light onto the ground plane (base pinned, higher texels thrown farther), whiten the casters themselves, then add the light through that visibility mask
void RendererClass::render_shadow_lights()
{
	if (!shadowPathtrace.ready)
		return;

	ShadowPathtraceState &st = shadowPathtrace;

	sf::Vector2u texSize = st.occluderTarget.getSize();
	sf::Vector2f resolution((float)texSize.x, (float)texSize.y);

	sf::Vector2f scale = orientation.scale;
	float heightPx = st.lightHeight * scale.y;
	float zMax = heightPx * st.maxHeightRatio;
	float softness = st.softness * scale.x;
	float tileHalfH = orientation.tileSize.y / 2.f;

	sf::RenderStates castStates;
	castStates.shader = &st.castShader;
	castStates.blendMode = sf::BlendAlpha;

	sf::RenderStates cutoutStates;
	cutoutStates.shader = &st.occluderCutoutShader;

	sf::RenderStates lightStates;
	lightStates.shader = &st.lightShader;
	lightStates.blendMode = sf::BlendAdd;

	sf::RectangleShape fullscreen(resolution);

	st.sampleTarget.clear(sf::Color(89, 89, 89));

	std::vector<sf::Vertex> verts;

	for (const ShadowLight &light : st.lights)
	{
		float angle = ((float)std::rand() / (float)RAND_MAX) * 6.2831853f;
		float radius = std::sqrt((float)std::rand() / (float)RAND_MAX) * softness;
		sf::Vector2f lightScreen = light.screenPos + sf::Vector2f(std::cos(angle), std::sin(angle)) * radius;
		sf::Vector2f ground(lightScreen.x, lightScreen.y + heightPx);

		st.occluderTarget.clear(sf::Color::White);

		for (const ShadowCaster &caster : st.casters)
		{
			const sf::IntRect rect = caster.sprite->getTextureRect();
			float ox = (float)rect.size.x;
			float oy = (float)rect.size.y;

			sf::Vector2f toLight = caster.base - ground;
			if (std::sqrt(toLight.x * toLight.x + toLight.y * toLight.y) > light.radius + ox * scale.x)
				continue;

			float originX = ox / 2.f;
			float originY = oy - tileHalfH;

			verts.clear();
			for (int i = 0; i <= st.strips; ++i)
			{
				float v = oy * (float)i / (float)st.strips;
				float z = std::min(std::max(0.f, (originY - v) * scale.y), zMax);
				float k = z / (heightPx - z);
				std::uint8_t alpha = (std::uint8_t)(255.f * (1.f - st.fade * z / zMax));

				for (int side = 0; side < 2; ++side)
				{
					float u = side == 0 ? 0.f : ox;
					sf::Vector2f pg(caster.base.x + (u - originX) * scale.x, caster.base.y);
					sf::Vector2f pos = pg + (pg - ground) * k;

					verts.push_back(sf::Vertex{
						pos,
						sf::Color(255, 255, 255, alpha),
						sf::Vector2f((float)rect.position.x + u, (float)rect.position.y + v)});
				}
			}

			castStates.texture = &caster.sprite->getTexture();
			st.occluderTarget.draw(verts.data(), verts.size(), sf::PrimitiveType::TriangleStrip, castStates);
		}

		for (const ShadowCaster &caster : st.casters)
		{
			sf::Sprite *sTile = caster.sprite;
			float ox = (float)sTile->getTextureRect().size.x;
			float oy = (float)sTile->getTextureRect().size.y;

			sf::Color oldColor = sTile->getColor();
			sTile->setColor(sf::Color::White);
			sTile->setOrigin({ox / 2.f, oy - tileHalfH});
			sTile->setPosition(caster.base);
			sTile->setScale(scale);

			st.occluderTarget.draw(*sTile, cutoutStates);

			sTile->setColor(oldColor);
		}

		st.occluderTarget.display();

		st.lightShader.setUniform("maskTex", st.occluderTarget.getTexture());
		st.lightShader.setUniform("resolution", resolution);
		st.lightShader.setUniform("lightPos", sf::Vector2f(light.screenPos.x, resolution.y - light.screenPos.y));
		st.lightShader.setUniform("lightColor", light.color);
		st.lightShader.setUniform("lightRadius", light.radius);

		st.sampleTarget.draw(fullscreen, lightStates);
	}

	st.sampleTarget.display();
}

// Code made by Claude Sonnet 5 - blends the new noisy sample into the previous accumulation buffer and swaps which target is "current"
void RendererClass::accumulate_shadows()
{
	if (!shadowPathtrace.ready)
		return;

	int prevIndex = shadowPathtrace.accumIndex;
	int nextIndex = 1 - shadowPathtrace.accumIndex;

	float blendFactor = shadowPathtrace.justReset ? 1.0f : 0.3f;
	shadowPathtrace.justReset = false;

	shadowPathtrace.accumulateShader.setUniform(
		"newSample", shadowPathtrace.sampleTarget.getTexture());
	shadowPathtrace.accumulateShader.setUniform(
		"prevAccum", shadowPathtrace.accum[prevIndex].getTexture());
	shadowPathtrace.accumulateShader.setUniform("blendFactor", blendFactor);

	sf::Sprite fullscreen(shadowPathtrace.sampleTarget.getTexture());

	sf::RenderStates states;
	states.shader = &shadowPathtrace.accumulateShader;

	shadowPathtrace.accum[nextIndex].clear();
	shadowPathtrace.accum[nextIndex].draw(fullscreen, states);
	shadowPathtrace.accum[nextIndex].display();

	shadowPathtrace.accumIndex = nextIndex;
}

// Code made by Claude Sonnet 5 - multiplies the accumulated shadow buffer over the already-drawn scene
void RendererClass::composite_shadows()
{
	if (!shadowPathtrace.ready)
		return;

	sf::Sprite overlay(shadowPathtrace.accum[shadowPathtrace.accumIndex].getTexture());

	sf::RenderStates states;
	states.blendMode = sf::BlendMultiply;

	window->draw(overlay, states);
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