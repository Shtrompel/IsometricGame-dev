#include "file/asset_manager.hpp"

// ImageAlphaGrid implementations

ImageAlphaGrid::ImageAlphaGrid() = default;

ImageAlphaGrid::ImageAlphaGrid(ImageAlphaGrid&& other) noexcept
{
    this->operator=(std::forward<ImageAlphaGrid>(other));
}

ImageAlphaGrid::ImageAlphaGrid(const ImageAlphaGrid& other) noexcept
{
    this->operator=(other);
}

ImageAlphaGrid::ImageAlphaGrid(const sf::Image& img)
{
    loadFromImage(img);
}

ImageAlphaGrid::ImageAlphaGrid(bool** buffer, unsigned w, unsigned h)
{
    this->grid = buffer;
    this->gridW = w;
    this->gridH = h;
}

ImageAlphaGrid::~ImageAlphaGrid()
{
    if (grid == nullptr)
        return;

    for (unsigned y = 0; y < gridH; ++y)
        delete[] grid[y];
    delete[] grid;
}

ImageAlphaGrid& ImageAlphaGrid::operator=(const ImageAlphaGrid& other)
{
    this->gridW = other.gridW;
    this->gridH = other.gridH;
    this->grid = new bool* [gridH];
    for (unsigned y = 0; y < gridH; ++y)
    {
        this->grid[y] = new bool[gridW];
        for (unsigned x = 0; x < gridW; ++x)
        {
            this->grid[y][x] = other.grid[y][x];
        }
    }

    return *this;
}

ImageAlphaGrid& ImageAlphaGrid::operator=(ImageAlphaGrid&& other) noexcept
{
    this->grid = other.grid;
    this->gridW = other.gridW;
    this->gridH = other.gridH;
    other.grid = nullptr;

    return *this;
}

bool ImageAlphaGrid::loadFromImage(const sf::Image& img)
{
    const std::uint8_t* pixels = img.getPixelsPtr();

    gridW = img.getSize().x;
    gridH = img.getSize().y;

    grid = new bool* [gridH];
    for (unsigned y = 0; y < gridH; ++y)
        grid[y] = new bool[gridW];

    bool* gridPtr = *grid;

    if (pixels == nullptr)
        return false;

    for (unsigned y = 0; y < gridH; ++y)
    {
        gridPtr = grid[y];
        for (unsigned x = 0; x < gridW; ++x)
        {
            unsigned isAlpha = 0;
            if (img.getPixel({x, y}).a == sf::Color::Transparent.a)
                isAlpha = 1;
            *gridPtr++ = isAlpha;
        }
    }

    return true;
}

bool ImageAlphaGrid::getPixel(int x, int y)
{
    if (grid == nullptr)
        return true;

    if (0 <= x && x < (int)gridW &&
        0 <= y && y < (int)gridH)
    {
        return  grid[y][x];
    }

    return true;
}

ImageAlphaGrid ImageAlphaGrid::subsection(unsigned x, unsigned y, unsigned w, unsigned h)
{
    if (gridH == 0 || gridW == 0)
        return {};

    if (!grid)
        return {};

    unsigned newGridW = w;
    unsigned newGridH = h;

    bool** newGrid = new bool* [newGridH];
    for (unsigned y = 0; y < newGridH; ++y)
        newGrid[y] = new bool[newGridW];

    bool* newGridPtr = *newGrid;
    bool* gridPtr = *grid;
    for (unsigned iy = y; iy < y + h; ++iy)
    {
        bool* newGridPtr = newGrid[iy - y];
        
        bool* gridPtr = (iy < gridH) ? grid[iy] : nullptr; 

        for (unsigned ix = x; ix < x + w; ++ix)
        {
            bool bitSource = true;
            
            if (gridPtr && ix < gridW) 
                bitSource = gridPtr[ix];
                
            *newGridPtr++ = bitSource;
        }
    }

    return ImageAlphaGrid{ newGrid, newGridW, newGridH };
}

// AssetKeySprites implementations

bool AssetKeySprites::operator==(const AssetKeySprites &other) const
{
    return this->texName == other.texName &&
           this->start == other.start &&
           this->end == other.end &&
           this->id == other.id;
}

std::string AssetKeySprites::to_string() const
{
    std::stringstream ret;
    ret << "{ ";
    ret << "Texture name: ";
    ret << texName << ", ";
    ret << "Start: ";
    ret << start << ", ";
    ret << "End: ";
    ret << end;
    ret << " }";
    return ret.str();
}

// AssetSprites implementations

sf::Sprite *AssetSprites::get(const size_t i)
{
    if (spriteCount == 0)
    {
        WARNING(
            "AssetSprites with texture: %s is empty and can't pass sprites.",
            key.texName.c_str());
        assert(0);
        return nullptr;
    }
    if (i >= spriteCount && !allowOverflow)
    {
        WARNING(
            "AssetSprites with texture: %s and sprite count: %zu was asked to get sprite at index: %zu.",
            key.texName.c_str(),
            spriteCount,
            i);
        assert(0);
        return nullptr;
    }
    if (sprites.empty())
    {
        WARNING(
            "\"sprites\" is null.",
            key.texName.c_str(),
            spriteCount,
            i);
        return nullptr;
    }
    return &sprites[i % spriteCount];
}

ImageAlphaGrid& AssetSprites::getAlphaGrid(const size_t i)
{
    if (spriteCount == 0)
    {
        WARNING(
            "ImageAlphaGrid with texture: %s is empty and can't pass sprites.",
            key.texName.c_str());
        assert(0);
    }
    if (i >= spriteCount && !allowOverflow)
    {
        WARNING(
            "ImageAlphaGrid with texture: %s and sprite count: %zu was asked to get sprite at index: %zu.",
            key.texName.c_str(),
            spriteCount,
            i);
        assert(0);
    }
    if (sprites.empty())
    {
        WARNING(
            "\"ImageAlphaGrid\" is null.",
            key.texName.c_str(),
            spriteCount,
            i);
    }
    return alphaGrids[i % spriteCount];
}

std::string AssetSprites::to_string() const
{
    std::stringstream ret;
    ret << "{ ";
    ret << "Sprite Count: ";
    ret << spriteCount << ", ";
    ret << "Key ";
    ret << key.to_string();
    ret << " }";
    return ret.str();
}

// AssetTexture implementations

std::string AssetTexture::to_string() const
{
    std::stringstream ret;
    ret << "{ ";
    ret << "Texture Handle: ";
    ret << texture.getNativeHandle() << ", ";
    ret << "Frame Size: ";
    ret << vec_str(frameSize) << ", ";
    ret << "Frame Count: ";
    ret << vec_str(frameCount);
    ret << " }";
    return ret.str();
}

// AssetManager implementations

AssetManager::AssetManager(const char *path)
{
    this->path = std::string(path);
}

AssetManager::~AssetManager()
{
    AssetSprites* sprite = nullptr;
    for (auto itr = sprites.begin(); itr != sprites.end(); itr++)
    {
        sprite = (*itr);
        delete[] sprite->alphaGrids;
        delete sprite;
    }
    for (auto itr = textures.begin(); itr != textures.end(); itr++)
    {
        delete (*itr);
    }
}

AssetSprites *AssetManager::get_sprite(int index)
{
    if (index == -1)
      return nullptr;
    if (0 <= index && index < sprites.size())
        return sprites[index];
    return nullptr;
}

int AssetManager::generate_null_texture()
{
    sf::Image img;
    img.resize({512, 512});
    
    for (int i = 0; i < 32; ++i)
    {
        int x = i % 8, y = i / 8;
        sf::Color c;
        c = ((i % 2) ? sf::Color::Black : sf::Color::Magenta);
        for (int px = 0; px < 512 / 32; ++px)
        {
            for (int py = 0; py < 512 / 32; ++py)
            {
                img.setPixel({static_cast<unsigned int>(px + x * 16), 
                              static_cast<unsigned int>(py + y * 16)}, c);
            }
        }
    }
    
    textures.push_back(new AssetTexture{});
    sf::Texture& t = textures.back()->texture;
    if (!t.loadFromImage(img)) {
        WARNING("Unable to load image");
        return 0;
    }

    DEBUG("todo");
    return -1;
}

int AssetManager::load_texture(
    const char *name,
    const char *subpath,
    const IVec &frameCount,
    const IVec &divisions,
    const char *type)
{
    int texId = -1;
    textures.push_back(new AssetTexture{});

    sf::Texture& t = textures.back()->texture;
    
    std::string fullDir = path;
    if (subpath && strlen(subpath) > 0) {
        fullDir += "/";
        fullDir += subpath;
    }

    char *filePath = new char[fullDir.size() + strlen(name) + strlen(type) + 3];
    sprintf(filePath, "%s/%s.%s", fullDir.c_str(), name, type);

    sf::Image img;
    if (img.loadFromFile(filePath))
    {
        textures.back()->alphaGrid.loadFromImage(img);
        if (t.loadFromImage(img))
        {
            texId = (int)textures.size() - 1u;
            AssetTexture& atex = *textures.back();
            atex.frameCount = frameCount;
            atex.frameSize = {
                (int)(t.getSize().x / frameCount.x),
                (int)(t.getSize().y / frameCount.y) };
            atex.divisions = divisions;
            texturesHash[str_uppercase(name)] = texId;
        }
        else
        {
            LOG_ERROR("Can't load texture from image that was originally at path \"%s\"", filePath);
        }
    }
    else
    {
        LOG_ERROR("Can't load image \"%s\"", filePath);
    }

    delete[] filePath;

    return texId;
}

int AssetManager::split_sprites(
    int index,
    const sf::Vector2i &framesStart,
    const sf::Vector2i &framesEnd,
    const sf::Vector2i &pos,
    const sf::Vector2i &buildSize)
{
    // Get original sprite
    AssetSprites *ogSprites = get_sprite(index);
    if (index == DEBUG_SELECT_BUILDING_SPRITE)
    {
        DEBUG("spriteCount: %d", (int)ogSprites->spriteCount);
    }
    if (!ogSprites)
    {
        WARNING("Sprite with key %d was not found", (int)index);
        return -1;
    }

    AssetKeySprites key = ogSprites->key;
    // Unique tile id for multitiled textures
    key.id = pos.x + pos.y * buildSize.x;

    // Create new sprite
    sprites.push_back(new AssetSprites{});
    int ret = (int)sprites.size() - 1;
    spritesHash[key] = ret;
    AssetSprites &sprite = *sprites.back();
    auto dif = framesEnd - framesStart + sf::Vector2i{1, 1};
    sprite.spriteCount = vec_prod<int>(dif);
    sprites.resize(sprite.spriteCount);
    sprite.key = key;
    /*
    if (dif.x != 1)
        WARNING("When using multitiled building, no divisions can be made in the horizontal axis for now");
    for (int y = 0; y < dif.y; ++y)
    {
        int index = pos.y * buildSize.x + pos.x;

        sprite.sprites[y] = ogSprites->sprites[index];
    }*/

    if (index == DEBUG_SELECT_BUILDING_SPRITE)
        DEBUG("\nframesStart: %s\nframesEnd: %s\npos: %s\nbuildSize: %s\nsprite: %d\nspriteCount: %d\nog spriteCount: %d",
            VEC_CSTR(framesStart),
            VEC_CSTR(framesEnd),
            VEC_CSTR(pos), 
            VEC_CSTR(buildSize),
            ret,
            (int)sprite.spriteCount,
            (int)ogSprites->spriteCount);
    for (int i = 0; i < sprite.spriteCount; ++i)
    {
        IVec newPos = {0, 0};
        newPos += pos;
        // framesStart, framesEnd, buildSize, pos
        int spriteIndex = (newPos.x + newPos.y * buildSize.x);
        spriteIndex += vec_prod(buildSize) * i;

        if (index == DEBUG_SELECT_BUILDING_SPRITE)
        {
            DEBUG("Sprite Id, Frame: %d -> Pos: %d", i, (int)spriteIndex);
        }
        if (ogSprites && spriteIndex < ogSprites->spriteCount)
        {
            sprite.sprites[i] = ogSprites->sprites[spriteIndex];
            sprite.alphaGrids[i] = ogSprites->alphaGrids[spriteIndex];
        }
        else
        assert(0);

        /*DEBUG("%d %s %s %s", i, VEC_CSTR(pos), VEC_CSTR(buildSize),
        VEC_CSTR(sprite.spriteCount));*/
    }
    return ret;
}

const AssetTexture &AssetManager::get_texture_from_name(const char *name)
{
    t_texhash_itr texItr = texturesHash.find(str_uppercase(name));
    if (texItr == texturesHash.end())
    {
        WARNING("Texture with name \"%s\" wasn't found, don't forget to add it.", name);
        return nullTexture;
    }
    int textureId = (*texItr).second;

    if (textureId >= textures.size())
    {
        WARNING("Texture with id \"%zu\" wasn't found.", textureId);
        return nullTexture;
    }

    return *textures.at(textureId);
}

int AssetManager::load_sprites(
    const char* name,
    const sf::Vector2i& start,
    const sf::Vector2i& end,
    const sf::Vector2i& size)
{
    if (!strcmp(name, ""))
    {
        WARNING("Can't load texture with no name.");
        return -1;
    }

    if (vec_or_equals<int>(start, non) || vec_or_equals<int>(end, non))
    {
        WARNING("Start or end can't be equal to -1 in the x or y axis.");
        return -1;
    }

    // Seach for texture id
    t_texhash_itr texItr = texturesHash.find(str_uppercase(name));
    if (texItr == texturesHash.end())
    {
        WARNING("Texture with name \"%s\" wasn't found, don't forget to add it.", str_uppercase(name).c_str());
        return -1;
    }
    int textureId = (*texItr).second;

    AssetTexture& texture = *textures.at(textureId);
    if (start.x >= (int)texture.frameSize.x ||
        start.y >= (int)texture.frameSize.y ||
        end.x >= (int)texture.frameSize.x ||
        end.y >= (int)texture.frameSize.y)
    {
        WARNING("Texture frame is out of bounds.");
        return -1;
    }
    
    int cx = end.x - start.x + 1;
    int cy = end.y - start.y + 1;

    if (cx <= 0 || cy <= 0)
    {
        WARNING("Animation end frame can't be before the start frame in the x or y axis.");
        return -1;
    }

    AssetKeySprites key = AssetKeySprites{
        std::string(name),
        start.x + start.y * texture.frameCount.x,
        end.x + start.y * texture.frameCount.x };

    // If sprite with the same characteristics was already made
    // return the existing sprite id
    if (spritesHash.count(key))
    {
        return (int)spritesHash.at(key);
    }

    IVec divs = static_cast<IVec>(texture.divisions);
    // Size of a section
    IVec sectionCount = texture.frameCount / divs;
    // Size in pixels
    IVec sectionSize = (IVec)texture.texture.getSize() / divs;

    //if (divs == IVec{ 1,1 }) sectionCount = { 1,1 };

    sprites.push_back(new AssetSprites{});
    int ret = (int)sprites.size() - 1u;
    spritesHash[key] = ret;
    AssetSprites& spriteOut = *sprites.back();
    // sprites will contain the animation frames for the tile
    // If the building has multiple tiles, sprites will also contain-
    // all of the tiles.
    spriteOut.spriteCount = cx * cy * vec_prod(size);
    
    spriteOut.sprites.clear();
    spriteOut.sprites.reserve(spriteOut.spriteCount);
    
    spriteOut.alphaGrids = new ImageAlphaGrid[spriteOut.spriteCount];
    spriteOut.key = key;
    spriteOut.sectionCount = sectionCount;
    spriteOut.frameCount = texture.frameCount;
    spriteOut.spriteSize = sectionSize;

    if (strcmp(name, DEBUG_SELECT_BUILDING_NAME) == STRCMP_EQUAL)
    {
        DEBUG(name);
        DEBUG("frameCount: %s\ndivs: %s",
            VEC_CSTR(texture.frameCount),
            VEC_CSTR(divs));
        DEBUG(
            "start: %s, end: %s, cx: %d,"
            "cy: %d, size: %s, divs:%s count:%s size:%s",
            VEC_CSTR(start),
            VEC_CSTR(end),
            cx, cy,
            VEC_CSTR(size),
            VEC_CSTR(divs),
            VEC_CSTR(sectionCount),
            VEC_CSTR(sectionSize));
        Logger::new_line();
        //sectionCount = texture.frameCount / divs;
    }

    int index = 0;
    auto sectionToSprites =
        [&spriteOut, &start, &texture, &index,
         &sectionCount, &sectionSize, &name](
            const IVec &section) {
                
            for (int j = 0; j < sectionCount.y; ++j)
            {
                for (int i = 0; i < sectionCount.x; ++i)
                {
                    if (strcmp(
                        name,
                        DEBUG_SELECT_BUILDING_NAME) ==
                        STRCMP_EQUAL)
                    {
                        DEBUG("\ni, j: %d %d\nsection: %s",
                            i,
                            j,
                            VEC_CSTR(section));
                    }
                    if (index >= spriteOut.spriteCount)
                        continue;

                    IVec size = texture.frameSize;
                    IVec pos;
                    pos = (start + section) * sectionCount;
                    pos += IVec{ i, j };
                    pos = pos * size;
                    auto rect = sf::IntRect{{pos.x, pos.y}, {size.x, size.y}};

                    // Construct the sprite at the end of the vector with the texture and rect
                    spriteOut.sprites.emplace_back(texture.texture, rect);

                    // If you need 's' for logging further down:
                    sf::Sprite& s = spriteOut.sprites.back();

                    s.setTexture(texture.texture);
                    s.setTextureRect(rect);
                    if (strcmp(name, DEBUG_SELECT_BUILDING_NAME) == 
                        STRCMP_EQUAL)
                    {
                        DEBUG("P: %s, S: %s, Frame: %d", 
                            VEC_CSTR(pos), 
                            VEC_CSTR(size), 
                            (int)index);
                    }

                    ImageAlphaGrid g;
                    g = texture.alphaGrid.subsection(
                        pos.x, 
                        pos.y, 
                        size.x, 
                        size.y);
                    spriteOut.alphaGrids[index] = g;

                    ++index;
                }
            }
        };

    // Iterate through sections
    for (int j = 0; j < cy; ++j)
    {
        for (int i = 0; i < cx; ++i)
        {
            sectionToSprites({i, j});
        }
    }
    
    if (strcmp(name, DEBUG_SELECT_BUILDING_NAME) == STRCMP_EQUAL)
    {
        for (int i = 0; i < get_sprite(ret)->spriteCount; ++i)
        {
            DEBUG("%d < %d", i, get_sprite(ret)->spriteCount);
        }
    }

    return (int)ret;
}