#ifdef ASDASD
// sprite_layout.hpp

enum class SpriteAxis : uint8_t
{
    ANIM = 0,
    FULLNESS = 1,
    OCCUPANT = 2,
    FIXED_AXIS_COUNT
};

// Points at either one of the fixed axes, or one of this building's
// upgrade paths (paths are per-building, so addressed by index, not enum).
struct AxisRef
{
    bool isPath = false;
    SpriteAxis fixed = SpriteAxis::ANIM;
    int pathIndex = 0;

    static AxisRef Fixed(SpriteAxis a) { return { false, a, 0 }; }
    static AxisRef Path(int i)        { return { true, SpriteAxis::ANIM, i }; }
};

struct SpriteAxisState
{
    std::array<int, (size_t)SpriteAxis::FIXED_AXIS_COUNT> fixed = {};
    std::vector<int> pathTiers; // pathTiers[pathId] = current tier on that path

    int get(const AxisRef& ref) const
    {
        if (ref.isPath)
            return (ref.pathIndex < (int)pathTiers.size()) ? pathTiers[ref.pathIndex] : 0;
        return fixed[(size_t)ref.fixed];
    }

    void set(const AxisRef& ref, int value)
    {
        if (ref.isPath)
        {
            if (ref.pathIndex >= (int)pathTiers.size())
                pathTiers.resize(ref.pathIndex + 1, 0);
            pathTiers[ref.pathIndex] = value;
        }
        else
            fixed[(size_t)ref.fixed] = value;
    }
};

struct SpriteAxisRule
{
    AxisRef when;
    int equals;
    AxisRef then;
    int becomes;
};

struct SpriteLayout
{
    std::array<int, (size_t)SpriteAxis::FIXED_AXIS_COUNT> fixedCounts = {1, 1, 1};
    std::vector<int> pathTierCounts;         // pathTierCounts[pathId] = tiers on that path

    std::vector<AxisRef> xAxes = { AxisRef::Fixed(SpriteAxis::ANIM) };
    std::vector<AxisRef> yAxes = { AxisRef::Fixed(SpriteAxis::FULLNESS) };

    std::vector<SpriteAxisRule> rules;

    int count_of(const AxisRef& ref) const
    {
        if (ref.isPath)
            return (ref.pathIndex < (int)pathTierCounts.size()) ? pathTierCounts[ref.pathIndex] : 1;
        return fixedCounts[(size_t)ref.fixed];
    }

    int fold(const std::vector<AxisRef>& axes, const SpriteAxisState& state) const
    {
        int idx = 0, mult = 1;
        for (const AxisRef& a : axes)
        {
            int c = std::max(1, count_of(a));
            int v = std::clamp(state.get(a), 0, c - 1);
            idx += v * mult;
            mult *= c;
        }
        return idx;
    }

    IVec grid_size() const
    {
        auto prod = [this](const std::vector<AxisRef>& axes) {
            int p = 1; for (auto& a : axes) p *= std::max(1, count_of(a)); return p;
        };
        return { prod(xAxes), prod(yAxes) };
    }

    int pack(SpriteAxisState state) const
    {
        for (const auto& r : rules)
            if (state.get(r.when) == r.equals)
                state.set(r.then, r.becomes);

        return fold(yAxes, state) * grid_size().x + fold(xAxes, state);
    }
};
#endif // ASDASD