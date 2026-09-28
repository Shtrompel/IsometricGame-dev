#include "render/FastLabel.hpp"

#include "utils/class/logger.hpp"

// Define the static font member
sf::Font CharSpriteSheet::emptyFont;

// CharSpriteSheet implementation

CharSpriteSheet::CharSpriteSheet()
{
    // Constructor body empty (initializer list handles members)
}

CharSpriteSheet::~CharSpriteSheet()
{
    for (size_t i = 0; i < 256u; ++i)
    {
        if (sprites[i]) {
            delete sprites[i];
            sprites[i] = nullptr;
        }
    }
}

void CharSpriteSheet::init(sf::Font& font, sf::Text text, int padding)
{
    this->padding = padding;
    text.setFont(font);

    sf::Vector2i texSize = { padding, 0 };
    float lineTop = -1;
    characterSize = text.getCharacterSize();

    for (size_t i = 0; i < CHARS_ARRAY_LEN; ++i)
    {
        text.setString(CHARS_ARRAY[i]);
        sf::FloatRect bounds = text.getLocalBounds();
        int letterWidth = (int)(bounds.size.x + bounds.position.x*0);
        int letterHeight = (int)(bounds.size.y + bounds.position.y);
        if (sizeFinal.y < letterHeight)
            sizeFinal.y = letterHeight;
        
        sizeFinal.x += letterWidth;
        sizeFinal.y = letterHeight;

        texSize.x += letterWidth;
        texSize.x += padding;
        texSize.y = letterHeight;

        if (lineTop == -1)
            lineTop = bounds.position.y;
        else if (lineTop > bounds.position.y)
            lineTop = bounds.position.y;
    }
    texSize.y += 2 * padding;
    
    DEBUG("Why [[nodiscard]] in .resize?");

    (void)texFinal.resize({
        (unsigned)texSize.x,
        (unsigned)texSize.y
    });

    texFinal.draw(text);
    texFinal.display();
    texFinal.clear(sf::Color::Transparent);

    texFinal.setSmooth(false);

    text.setString("");

    sf::Vector2f lastSize = { 0.0f, 0.0f };
    sf::Vector2f letterPos = { (float)padding, (float)padding };

    for (size_t i = 0; i < CHARS_ARRAY_LEN; ++i)
    {
        text.setString(CHARS_ARRAY[i]);

        sf::FloatRect bounds = text.getLocalBounds();
        
        sf::Vector2f size = { bounds.size.x, bounds.size.y };

        text.setPosition(letterPos);
        texFinal.draw(text);
        
        sprites[CHARS_ARRAY[i]] = new sf::Sprite(
            texFinal.getTexture(),
            sf::IntRect(
                { (int)(bounds.position.x + letterPos.x), (int)(letterPos.y + lineTop) },
                { (int)size.x, (int)(lineTop - bounds.position.y + sizeFinal.y) }
            )
        );

        letterPos.x += size.x;
        letterPos.x += padding;
    }
}

sf::Sprite& CharSpriteSheet::operator[](char c) const
{
    assert(sprites[c]);
    return *sprites[c];
}

sf::Sprite& CharSpriteSheet::getSprite(char c) const
{
    assert(sprites[c]);
    return *sprites[c];
}

sf::IntRect CharSpriteSheet::getRect(char c) const
{
    assert(sprites[c]);
    return sprites[c]->getTextureRect();
}

sf::Vector2i CharSpriteSheet::getSize(char c) const
{
    assert(sprites[c]);
    sf::IntRect rect = getRect(c);
    return { rect.size.x, rect.size.y };
}

int CharSpriteSheet::getMaxHeight() const
{
    return this->sizeFinal.y;
}

int CharSpriteSheet::getCharacterSize()
{
    return this->characterSize;
}

// FastLabel implementation

FastLabel::FastLabel(const char* typeName, bool initRenderer) :
    tgui::Label{ typeName, initRenderer }
{
    getScrollbar()->setPolicy(tgui::Scrollbar::Policy::Never);
}

void FastLabel::set(tgui::Label::Ptr label)
{
    setHorizontalAlignment(label->getHorizontalAlignment());
    setVerticalAlignment(label->getVerticalAlignment());

    setVisible(label->isVisible());
    setEnabled(label->isEnabled());
    setParent(label->getParent());
    setPosition(label->getAbsolutePosition());
    setSize(label->getSize());
}

FastLabel::Ptr FastLabel::copy(FastLabel::ConstPtr label)
{
    if (label)
        return std::static_pointer_cast<FastLabel>(label->clone());
    else
        return nullptr;
}

FastLabel::Ptr FastLabel::create(const tgui::String& text)
{
    auto label = std::make_shared<FastLabel>();

    if (!text.empty())
        label->setText(text);

    return label;
}

void FastLabel::setCharSpriteSheet(CharSpriteSheet* spriteSheet)
{
    this->spriteSheet = spriteSheet;
}

void FastLabel::setTextSpacing(int spacing)
{
    this->spacing = spacing;
}

void FastLabel::setTextScale(float scale)
{
    this->textScale = scale;
}

void FastLabel::setText(const tgui::String& string)
{
    stdString = string.toStdString();

    float x = 0, y;
    for (size_t i = 0; i < stdString.size(); ++i)
    {
        x += spriteSheet->getSize(stdString[i]).x * textScale + spacing;
    }
    y = (float)spriteSheet->getMaxHeight() * textScale;

    this->textRect = { (int)x, (int)y };
}

void FastLabel::drawText(const tgui::RenderStates& states) const
{
    using namespace tgui;

    const float textOffset = Text::getExtraHorizontalPadding(m_fontCached, m_textSize);

    RenderStates movedStates = states;
    sf::RenderStates sfStates;
    {
        auto matrix = movedStates.transform.getMatrix();
        sfStates.transform = sf::Transform{ matrix[0], matrix[4], std::round(matrix[12]),
                                           matrix[1], matrix[5], std::floor(matrix[13] + 0.1f),
                                           matrix[3], matrix[7], matrix[15] };
        sfStates.transform.translate({
            m_paddingCached.getLeft() + textOffset,
            m_paddingCached.getTop()
        });
    }

    tgui::Vector2f vecAlgmnt = { 0.f, 0.f };
    switch (m_verticalAlignment)
    {
        case tgui::VerticalAlignment::Bottom:
            vecAlgmnt.y = (float)getSize().y - spriteSheet->getCharacterSize() * textScale;
        break;

        case tgui::VerticalAlignment::Center:
            vecAlgmnt.y = (float)getSize().y - spriteSheet->getCharacterSize() * textScale;
            vecAlgmnt.y /= 2.f;
        break;

        case tgui::VerticalAlignment::Top:
            vecAlgmnt.y = 0;
        break;
    };

    switch (m_horizontalAlignment)
    {
    case tgui::HorizontalAlignment::Right:
        vecAlgmnt.x = (float)getSize().x - textRect.x;
        break;

    case tgui::HorizontalAlignment::Center:
        vecAlgmnt.x = (float)getSize().x - textRect.x;
        vecAlgmnt.x /= 2.f;
        break;

    case tgui::HorizontalAlignment::Left:
        vecAlgmnt.x = 0;
        break;
    }


    sfStates.transform.translate({vecAlgmnt.x, vecAlgmnt.y});

    sf::Vector2i textPos = { 0, 0 };
    for (size_t i = 0; i < stdString.size(); ++i)
    {
        sf::RenderStates s = sfStates;
        s.transform.translate({
            (float)textPos.x,
            (float)0
        });
        s.transform.scale({textScale, textScale});
        renderWindow->draw((*spriteSheet)[stdString[i]], s);

        textPos.x += (int)(spriteSheet->getSize(stdString[i]).x * textScale) + spacing;
    }
}

void FastLabel::draw(tgui::BackendRenderTarget& target, tgui::RenderStates states) const
{
    using namespace tgui;
    
    const RenderStates statesForScrollbar = states;

    Vector2f innerSize = { getSize().x - m_bordersCached.getLeft() - m_bordersCached.getRight(),
                          getSize().y - m_bordersCached.getTop() - m_bordersCached.getBottom() };
    
    // Draw the borders
    if (m_bordersCached != Borders{ 0 })
    {
        target.drawBorders(states, m_bordersCached, getSize(), Color::applyOpacity(m_borderColorCached, m_opacityCached));
        states.transform.translate({ m_bordersCached.getLeft(), m_bordersCached.getTop() });
    }

    // Draw the background
    if (m_spriteBackground.isSet())
        target.drawSprite(states, m_spriteBackground);
    else if (m_backgroundColorCached.isSet() && (m_backgroundColorCached != Color::Transparent))
        target.drawFilledRect(states, innerSize, Color::applyOpacity(m_backgroundColorCached, m_opacityCached));;

    // Draw the text
    this->drawText(states);
}