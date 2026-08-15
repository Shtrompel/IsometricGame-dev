#pragma once

#include <TGUI/TGUI.hpp>
#include <TGUI/Any.hpp>
#include <TGUI/Backend/SFML-Graphics.hpp>

#include <SFML/Graphics.hpp>

constexpr const char* CHARS_ARRAY =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789"
    ":!? ";
constexpr size_t CHARS_ARRAY_LEN = 67;

struct CharSpriteSheet
{
    sf::Sprite* sprites[256] = { nullptr };
    sf::RenderTexture texFinal{};
    sf::Vector2i sizeFinal = { 0, 0 };

    int padding = 10;
    int characterSize = -1;

    static sf::Font emptyFont;

    CharSpriteSheet();
    ~CharSpriteSheet();

    void init(sf::Font& font, sf::Text text = sf::Text(emptyFont), int padding = 5);

    sf::Sprite& operator[](char c) const;
    sf::Sprite& getSprite(char c) const;
    sf::IntRect getRect(char c) const;
    sf::Vector2i getSize(char c) const;
    int getMaxHeight() const;
    int getCharacterSize();
};

class FastLabel : public tgui::Label
{
public:
    using Ptr = std::shared_ptr<FastLabel>;
    using ConstPtr = std::shared_ptr<const FastLabel>;

    static constexpr const char StaticWidgetType[] = "FastLabel";

    CharSpriteSheet* spriteSheet = nullptr;

    sf::RenderWindow* renderWindow = nullptr;
    int spacing = 2;
    std::string stdString = "";
    tgui::Vector2i textRect = { 0, 0 };

    FastLabel(const char* typeName = "Label", bool initRenderer = true);

    void set(tgui::Label::Ptr label);

    static FastLabel::Ptr copy(FastLabel::ConstPtr label);

    static Ptr create(const tgui::String& text = "");

    void setCharSpriteSheet(CharSpriteSheet* spriteSheet);
    void setTextSpacing(int spacing);
    void setText(const tgui::String& string);

    void drawText(const tgui::RenderStates& states) const;

    void draw(tgui::BackendRenderTarget& target, tgui::RenderStates states) const override;

protected:
    Widget::Ptr clone() const override
    {
        return std::make_shared<FastLabel>(*this);
    }
};