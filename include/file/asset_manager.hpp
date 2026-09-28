#ifndef _GAME_ASSET_MANAGER
#define _GAME_ASSET_MANAGER

#include <cstdio>
#include <cassert>
#include <functional>
#include <map>
#include <unordered_map>
#include <vector>
#include <sstream>

#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics/Image.hpp>

#include "utils/class/logger.hpp"
#include "utils/globals.hpp"
#include "utils/utils.hpp"
#include "utils/math.hpp"

constexpr bool ALPHAGRID_TRANSPARENT = 1;

// Converts image to a 2d array repsesenting whether each pixel
// is transparent or not.
struct ImageAlphaGrid
{
    ImageAlphaGrid();
    ImageAlphaGrid(ImageAlphaGrid&& other) noexcept;
    ImageAlphaGrid(const ImageAlphaGrid& other) noexcept;
    ImageAlphaGrid(const sf::Image& img);
    ImageAlphaGrid(bool** buffer, unsigned w, unsigned h);
    ~ImageAlphaGrid();

    ImageAlphaGrid& operator=(const ImageAlphaGrid& other);
    ImageAlphaGrid& operator=(ImageAlphaGrid&& other) noexcept;

    bool loadFromImage(const sf::Image& img);
    bool getPixel(int x, int y);
    ImageAlphaGrid subsection(unsigned x, unsigned y, unsigned w, unsigned h);

    bool** grid = nullptr;
    unsigned gridH = 0, gridW = 0;
};

struct AssetKeySprites
{
    std::string texName;
    int start, end;
    // For multisized buildings, every tile has a
    // different id.
    unsigned id = 0;

    bool operator==(const AssetKeySprites &other) const;

    std::string to_string() const;
};

struct AssetSprites
{
    // Frames
    std::vector<sf::Sprite> sprites;
    // For every sprite, hold a alpga grid
    ImageAlphaGrid* alphaGrids = nullptr;

    int spriteCount;
    IVec sectionCount = {1, 1};
    IVec frameCount = {1, 1};
    // i removed the comments CLAUDE they are annoying
    IVec animGrid = {1, 1};
    AssetKeySprites key;
    bool allowOverflow = true;

    // Code made by Claude Sonnet 5 - whether get()'s frame index should fold
    // back and forth (0,1,2,1,0,...) instead of looping (0,1,2,0,1,2,...);
    // set per-texture via textures.json's "anim_mode" and carried over here
    bool pingpong = false;

    IVec spriteSize = { 0, 0 };

    sf::Sprite *get(const size_t i);
    ImageAlphaGrid& getAlphaGrid(const size_t i);
    std::string to_string() const;

    // Code made by Claude Sonnet 5 - folds a raw, ever-increasing frame
    // counter into an index within [0, cols), respecting this sheet's
    // animation mode (pingpong vs the default loop)
    int fold_frame(int frame, int cols) const
    {
        if (cols <= 1)
            return 0;

        if (!pingpong)
        {
            int m = frame % cols;
            return m < 0 ? m + cols : m;
        }

        int period = 2 * (cols - 1);
        int m = frame % period;
        if (m < 0)
            m += period;
        return m < cols ? m : period - m;
    }
};

struct AssetTexture
{
    sf::Texture texture;
    ImageAlphaGrid alphaGrid;

    IVec frameSize;
    IVec frameCount = {1, 1};
    IVec divisions = {1, 1};

    // Code made by Claude Sonnet 5 - carried into every AssetSprites cut from
    // this texture; see AssetSprites::pingpong
    bool pingpong = false;

    std::string to_string() const;
};

namespace std
{
template <>
struct hash<AssetKeySprites>
{
    size_t operator()(AssetKeySprites const &key) const noexcept
    {
        size_t seed = std::hash<std::string>{}(key.texName);
        hash_combine(seed, key.start);
        hash_combine(seed, key.end);
        hash_combine(seed, key.id);
        return seed;
    }
};
} // namespace std

struct AssetManager
{
    const sf::Vector2i non = {-1, -1};

    std::vector<AssetTexture*> textures;
    std::vector<AssetSprites*> sprites;
    std::map<std::string, int> texturesHash;
    std::unordered_map<AssetKeySprites, int> spritesHash;
    std::string path;

    const AssetTexture nullTexture = {};

    typedef std::map<std::string, int>::iterator t_texhash_itr;

    AssetManager(const char *path);
    ~AssetManager();

    AssetSprites *get_sprite(int index);
    int generate_null_texture();
    int load_texture(
        const char *name,
        const char *subpath = "",
        const IVec &frameCount = {1, 1},
        const IVec &divisions = {1, 1},
        const char *type = "png",
        bool pingpong = false);
    int split_sprites(
        int index,
        const sf::Vector2i &framesStart,
        const sf::Vector2i &framesEnd,
        const sf::Vector2i &pos,
        const sf::Vector2i &buildSize);
    const AssetTexture &get_texture_from_name(const char *name);
    int load_sprites(
        const char* name,
        const sf::Vector2i& start,
        const sf::Vector2i& end,
        const sf::Vector2i& size = { 1, 1 });

    template <typename U>
    static inline bool vec_or_equals(const sf::Vector2<U> &a, const sf::Vector2<U> &b)
    {
        return a.x == b.x || a.y == b.y;
    }
};

#endif // _GAME_ASSET_MANAGER