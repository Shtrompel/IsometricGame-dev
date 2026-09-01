#ifndef _GAME_SPRITE_LAYOUT
#define _GAME_SPRITE_LAYOUT

#include <vector>
#include <unordered_map>

#include "utils/definitions.hpp"
#include "utils/globals.hpp"
#include "utils/math.hpp"
#include "nlohmann/json.hpp"

typedef int t_spriteaxis;

constexpr t_spriteaxis SPRITE_LAYOUT_ANIM = 0;
constexpr t_spriteaxis SPRITE_LAYOUT_FULLNESS = 1;
constexpr t_spriteaxis SPRITE_LAYOUT_OCCUPENT = 2;
constexpr t_spriteaxis SPRITE_LAYOUT_UPGRADE0 = 3;
constexpr t_spriteaxis SPRITE_LAYOUT_UPGRADE1 = 4;
constexpr t_spriteaxis SPRITE_LAYOUT_UPGRADE2 = 5;

struct SpriteSheetAxis
{
    t_spriteaxis axis = 0;
    int value = 0;

    SpriteSheetAxis()
    {
    }

    SpriteSheetAxis(t_spriteaxis axis, int value)
    {
        this->axis = axis;
        this->value = value;
    }
};

struct SpriteSheetSystem
{
    struct SpriteSheet2DAxis
    {
        int x = 0;
        int y = 0;

        SpriteSheet2DAxis() {}

        SpriteSheet2DAxis(int x, int y)
        {
            this->x = x;
            this->y = y;
        }
    };

    std::vector<t_spriteaxis> xAxes;
    std::vector<t_spriteaxis> yAxes;

    std::unordered_map<t_spriteaxis, int> axisSizes;
    std::unordered_map<t_spriteaxis, int> xStrides;
    std::unordered_map<t_spriteaxis, int> yStrides;

    int gridWidth, gridHeight;

    SpriteSheetSystem()
    {
    }

    SpriteSheetSystem(
        const std::vector<t_spriteaxis> &xAxes,
        const std::vector<t_spriteaxis> &yAxes,
        const std::unordered_map<t_spriteaxis, int> &axisSizes,
        int gridWidth, int gridHeight)
    {
        this->xAxes = xAxes;
        this->yAxes = yAxes;
        this->axisSizes = axisSizes;
        this->gridWidth = gridWidth;
        this->gridHeight = gridHeight;

        calculate_strides(this->xAxes, this->xStrides);
        calculate_strides(this->yAxes, this->yStrides);
    }

    void calculate_strides(
        const std::vector<t_spriteaxis> &basis,
        std::unordered_map<t_spriteaxis, int> &strides)
    {
        int currentStride = 1;
        for (t_spriteaxis axis : basis)
        {
            strides[axis] = currentStride;
            currentStride *= (axisSizes.count(axis) ? axisSizes[axis] : 1);
        }
    }

    SpriteSheetSystem(
        std::vector<t_spriteaxis> &&xAxes,
        std::vector<t_spriteaxis> &&yAxes)
    {
        this->xAxes = std::move(xAxes);
        this->yAxes = std::move(yAxes);
    }

    // A default-constructed / never-loaded layout has no grid size,
    // callers use this to fall back to the old single-frame behaviour.
    bool is_configured() const
    {
        return gridWidth > 0 && gridHeight > 0;
    }

    SpriteSheet2DAxis get_nd_to_2d(const std::vector<SpriteSheetAxis> &index) const
    {
        SpriteSheet2DAxis out;
        for (const SpriteSheetAxis &i : index)
        {
            if (xStrides.count(i.axis))
            {
                out.x += i.value * xStrides.at(i.axis);
            }
            else if (yStrides.count(i.axis))
            {
                out.y += i.value * yStrides.at(i.axis);
            }
        }
        out.x = std::clamp(out.x, 0, gridWidth - 1);
        out.y = std::clamp(out.y, 0, gridHeight - 1);
        return out;
    }

    int get_2d_to_1d(const SpriteSheet2DAxis &index) const
    {
        return index.x + index.y * gridWidth;
    }

    int get_nd_to_1d(const std::vector<SpriteSheetAxis> &index) const
    {
        return get_2d_to_1d(get_nd_to_2d(index));
    }
};

inline void to_json(nlohmann::json &j, const SpriteSheetSystem::SpriteSheet2DAxis &axis)
{
    j = nlohmann::json{
        {"x", axis.x},
        {"y", axis.y}};
}

inline void from_json(const nlohmann::json &j, SpriteSheetSystem::SpriteSheet2DAxis &axis)
{
    j.at("x").get_to(axis.x);
    j.at("y").get_to(axis.y);
}

// Serialization for the main struct
inline void to_json(nlohmann::json &j, const SpriteSheetSystem &system)
{
    j = nlohmann::json{
        {"xa", system.xAxes},
        {"ya", system.yAxes},
        {"as", system.axisSizes},
        {"xs", system.xStrides},
        {"sy", system.yStrides},
        {"gw", system.gridWidth},
        {"gh", system.gridHeight}};
}

inline void from_json(const nlohmann::json &j, SpriteSheetSystem &system)
{
    j.at("xa").get_to(system.xAxes);
    j.at("ya").get_to(system.yAxes);
    j.at("as").get_to(system.axisSizes);
    j.at("xs").get_to(system.xStrides);
    j.at("sy").get_to(system.yStrides);
    j.at("gw").get_to(system.gridWidth);
    j.at("gh").get_to(system.gridHeight);
}

#endif // _GAME_SPRITE_LAYOUT