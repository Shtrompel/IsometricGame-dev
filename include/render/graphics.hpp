#ifndef _GAME_GRAPHICS
#define _GAME_GRAPHICS

#include <cstdio>
#include <set>
#include <unordered_map>

#include "game/game_data.hpp"
#include "utils/class/logger.hpp"
#include "utils/math.hpp"
#include "utils/utils.hpp"
#include "file/asset_manager.hpp"
#include "render/gui.hpp"

struct Orientation
{
	// World scale, scale of each tile compared to it's original size
	sf::Vector2f scale = {1.f, 1.f};
	// The tile size in pixels
	sf::Vector2i tileSize = DEFAULT_TILE_SIZE;
	// Times the world rotated in 90 degrees
	int rotation = ROTATION_INIT;

	sf::Vector2i worldPos = {0, 0};

	int zoomFactor = 0;

	std::string to_string();
};

// to_json and from_json are static helpers, defined in the .cpp
void to_json(json &j, const Orientation &v);
void from_json(const json &j, Orientation &v);

static sf::Vector2f screen_pos_to_world_pos_src(const sf::Vector2i &posIn, const Orientation &gridDraw)
{
	sf::Vector2i pos = posIn - gridDraw.worldPos;
	sf::Vector2f tileDrawSize = gridDraw.tileSize * gridDraw.scale;

	float a = 0.5f * tileDrawSize.x;
	float b = -a;
	float c = 0.5f * tileDrawSize.y;
	float d = c;

	sf::Vector2f posf = {
		(float)(d * pos.x - b * pos.y),
		(float)(-c * pos.x + a * pos.y)};
	//posf *= 1.0f / (a * d - b * c);
	posf = posf / (a * d - b * c);

	return posf;
}

inline sf::Vector2f screen_pos_to_world_pos(const sf::Vector2i &posIn, const Orientation &gridDraw)
{
	return vec_90_rotate(
		screen_pos_to_world_pos_src(posIn, gridDraw),
		gridDraw.rotation);
}

inline sf::Vector2i screen_pos_to_tile_pos(const sf::Vector2i &posIn, const Orientation &gridDraw)
{
	sf::Vector2f posf = screen_pos_to_world_pos_src(posIn, gridDraw);
	sf::Vector2i posi;

	posf = vec_90_rotate(
		posf,
		gridDraw.rotation);
	posi = sf::Vector2i{(int)math_floor(posf.x), (int)math_floor(posf.y)};
	return posi;
}

template <typename U = int>
sf::Vector2i world_pos_to_screen_pos(const sf::Vector2<U> &tilePos, const Orientation &gridDraw)
{
	auto fixedTilePos = tilePos;

	sf::Vector2<U> tileDrawPos = {
		fixedTilePos.x - fixedTilePos.y,
		fixedTilePos.x + fixedTilePos.y};
	sf::Vector2f tileDrawSize = gridDraw.tileSize * gridDraw.scale;

	tileDrawPos = vec_90_rotate(
		tileDrawPos,
		((int)gridDraw.rotation + 2) % 2);

	// Trial and error test, i don't understand it myself
	if (1 <= gridDraw.rotation &&
		gridDraw.rotation <= 2)
		tileDrawPos = tileDrawPos * (U)-1;

	tileDrawPos = (sf::Vector2<U>)(tileDrawPos * tileDrawSize);
	tileDrawPos /= (U)2;
	tileDrawPos += (sf::Vector2<U>)gridDraw.worldPos;

	return (sf::Vector2i)tileDrawPos;
}

struct IVecCompare
{
	inline bool operator()(const IVec &a, const IVec &b) const
	{
		return vec_compare(a, b);
	}
};

struct RendererClass
{
	sf::RenderWindow *window = nullptr;
	sf::View *view = nullptr;

	// Rendering
	Orientation orientation;
	bool zooming = false;
	bool isBtnPressed = 0;

	bool drawPath = true;

	t_sprite spriteGrass = -1;
	t_sprite spriteTileSelected = -1;
	t_sprite spriteEntity = -1;
	t_sprite spriteConstruction = -1;
	t_sprite spriteIconBuild = -1;
	t_sprite spriteIconDelete = -1;
	t_sprite spriteEnemy = -1;
	t_sprite spriteTmpBullet = -1;

	size_t renderFrame = 0;

	AssetManager *assets = nullptr;

	RendererClass();
	RendererClass(sf::RenderWindow *window, sf::View *view, AssetManager *assets);

	void render_grid(Chunks *chunks);
	FRect render_object(GameBody* body, bool bSearch, ImageAlphaGrid* gridOut = nullptr);

	template <class T> 
	GameBody* render_objects(
		T& bodies, 
		bool searchEntity, 
		FVec searchEntityPos,
		uint32_t entityFilter = -1)
	{
		assert(window);
		assert(view);

		GameBody* out = nullptr;
		for (auto bodyItr = bodies.begin(); bodyItr != bodies.end(); ++bodyItr)
		{
			GameBody* body = *bodyItr;
			assert(body);

			ImageAlphaGrid alphaGrid;
			ImageAlphaGrid* alphaGridPtr = &alphaGrid;
			FRect rect = render_object(body, searchEntity, &alphaGrid);

			IVec iSearchPos = (IVec)( (searchEntityPos - rect.pos) / orientation.scale.x);

			if (searchEntity)
			{
				// Check if the mask permits this object to be selected
				bool maskPass = entityFilter >> body->objectId & 0x1;
				/*
				DEBUG("Rect: %s %s, %d %d %d", 
					VEC_CSTR(rect.pos), 
					VEC_CSTR(rect.size),
					maskPass,
					vec_inside(searchEntityPos, rect.pos, rect.size),
					!alphaGrid.getPixel(iSearchPos.x, iSearchPos.y));
					*/

				if (rect.size == FVec{ 0.f, 0.f })
					continue;


				// Return this object if: 
				// Mask enables the selection, 
				// mouse inside the sprite rectangle,
				// pixel is not transparent.
				if (maskPass &&
					vec_inside(searchEntityPos, rect.pos, rect.size) &&
					alphaGrid.getPixel(iSearchPos.x, iSearchPos.y) != ALPHAGRID_TRANSPARENT)
				{
					out = body;
				}
			}
		}

		++renderFrame;
		return out;
	}

	template <typename Container, typename Extractor>
	void render_influence_area_generic(
		const Container& container, Extractor extract)
	{
		sf::Sprite *sHighlight = assets->sprites.at(spriteTileSelected)->get(0);
		sHighlight->setColor(sf::Color(100, 200, 255, 128));

		std::set<sf::Vector2i, IVecCompare> influenceTiles;

		for (const auto& item : container)
		{
			sf::Vector2i tilePos;
			sf::Vector2i buildSize;
			float effectRadius;
			extract(tilePos, buildSize, effectRadius, item);
			int r = (int)std::ceil(effectRadius);
			if (effectRadius <= 0 || r <= 0)
				continue;
			
			sf::Vector2f buildCenter = sf::Vector2f(tilePos) + sf::Vector2f(buildSize) / 2.f;
			for (int x = -r; x <= r + buildSize.x; ++x)
			{
				for (int y = -r; y <= r + buildSize.y; ++y)
				{
					sf::Vector2i currentPos = tilePos + sf::Vector2i{x, y};
					sf::Vector2f tileCenter = sf::Vector2f(currentPos) + sf::Vector2f{0.5f, 0.5f};

					if (vec_dist(buildCenter, tileCenter) <= effectRadius)
					{
						influenceTiles.insert(currentPos);
					}
				}
			}
		}

		for (const auto &pos : influenceTiles)
		{
			sf::Vector2f fpos = sf::Vector2f(pos) + sf::Vector2f{0.5f, 0.5f};
			sf::Vector2i drawPos = world_pos_to_screen_pos<float>(fpos, orientation);
			
			sub_render(
				drawPos,
				orientation.scale,
				*sHighlight,
				TextureOrigin::BOTTOM);
		}

		sHighlight->setColor(sf::Color::White);
	}

	void render_influence_area(
		UpgradeTree *buildTree,
		const std::set<sf::Vector2i, IVecCompare> &suggestionBuilds);

	void render_influence_area(
		const std::unordered_map<size_t, BuildingBase*>& selectedBuildings);

	void render_suggestion(
		UpgradeTree *buildTree,
		Chunks *chunks,
		GameMode gameMode,
		std::set<sf::Vector2i, IVecCompare> &suggestionBuilds);

	void render_gui(GameData &data, float fps);

	void sub_render_grid(Chunks *chunks, int idx, int idy, sf::Sprite *tileSprite);

	template <typename U>
	void sub_render_tile(
		sf::Vector2<U> pos,
		sf::Vector2f scale,
		sf::Sprite &s)
	{
		float x = (float)pos.x;
		float y = (float)pos.y;
		float ox = (float)s.getTextureRect().size.x;
		float oy = (float)s.getTextureRect().size.y;

		s.setScale(scale);
		s.setPosition({x, y});
		float originX, originY;


		float ORIGINS[4][2] = 
		{
			{ox / 2.f, oy - orientation.tileSize.y},
			{ox, oy - orientation.tileSize.y / 2.f},
			{ox / 2.f, oy},
			{0, oy - orientation.tileSize.y * 0.5f}
		};

		originX = ORIGINS[orientation.rotation % 4][0];
		originY =  ORIGINS[orientation.rotation % 4][1];

		s.setOrigin({originX, originY});

		window->draw(s);
	}

	// Return the position of the rop left corner of where 
	// the sprites start to being drawn
	template <typename U>
	FVec sub_render(
		const sf::Vector2<U>& pos,
		const sf::Vector2f& scale,
		sf::Sprite& s,
		TextureOrigin origin,
		IVec originPoint = {},
		bool drawBorders = false)
	{
		float x = (float)pos.x;
		float y = (float)pos.y;
		float ox = (float)s.getTextureRect().size.x;
		float oy = (float)s.getTextureRect().size.y;

		const sf::Vector2i& tileWorldSize = orientation.tileSize;

		switch (origin)
		{
		case TextureOrigin::BOTTOM:
			s.setOrigin({ox / 2.f, oy - tileWorldSize.y / 2.f});
			break;
		case TextureOrigin::CENTERED:
			s.setOrigin({ox / 2.f, oy / 2.f});
			break;
		case TextureOrigin::TOP_LEFT:
			s.setOrigin({0, 0});
			break;
		case TextureOrigin::POINT:
			s.setOrigin({
				(float)originPoint.x,
				(float)originPoint.y});
			break;
		default:
			break;
		}
		s.setPosition({x, y});
		s.setScale(scale);

		window->draw(s);

		// Draw rect representing the bounds of the sprite
		if (drawBorders)
		{
			sf::RectangleShape rectShape;
			rectShape.setPosition({
				s.getGlobalBounds().position.x,
				s.getGlobalBounds().position.y});
			rectShape.setSize(
				{ s.getGlobalBounds().size.x,
				s.getGlobalBounds().size.y });
			rectShape.setFillColor(sf::Color::Transparent);
			rectShape.setOutlineColor(sf::Color::Black);
			rectShape.setOutlineThickness(4);
			window->draw(rectShape);
		}

		#ifdef DEBUG_SUBRENDER_CENTER
		{
			sf::CircleShape circle;
			circle.setRadius(4.f);
			circle.setOrigin(circle.getRadius(), circle.getRadius());
			circle.setFillColor(sf::Color::Yellow);
			circle.setOutlineColor(sf::Color::Black);

			circle.setPosition(x, y);

			window->draw(circle);
		}
		#endif // DEBUG_SUBRENDER_CENTER

		return {
			s.getGlobalBounds().position.x,
			s.getGlobalBounds().position.y };
	}

	void sub_render_draw(
		PathData &pathData,
		sf::Vector2f scale,
		bool relevantSegment = true);
};

#endif // _GAME_GRAPHICS