#ifndef _GAME_GUI
#define _GAME_GUI

#include <TGUI/TGUI.hpp>
#include <TGUI/Backend/SFML-Graphics.hpp>

class DirtyLabel : public tgui::Label
{
public:
    using Ptr = std::shared_ptr<DirtyLabel>;
    using ConstPtr = std::shared_ptr<const DirtyLabel>;

    static constexpr const char StaticWidgetType[] = "DirtyLabel";

    sf::RenderTarget* target = nullptr;
    sf::Text* m_text = nullptr;


    DirtyLabel(const char* typeName = "Label", bool initRenderer = true) :
        Label{ typeName, initRenderer }
    {

    }

    ~DirtyLabel()
    {
        delete m_text;
    }

    void set(tgui::Label::Ptr label)
    {
        // Label::operator=(*label.get());
        // setPosition(label->getAbsolutePosition());
        setSize(label->getSize());
        setTextSize(label->getTextSize());

        tgui::Vector2f v;
        v = label->getAbsolutePosition() + label->getSize() / 2.f;
        if (m_text)
        m_text->setPosition(v);

    }

    static DirtyLabel::Ptr copy(DirtyLabel::ConstPtr label)
    {
        if (label)
            return std::static_pointer_cast<DirtyLabel>(label->clone());
        else
            return nullptr;
    }

    Widget::Ptr clone() const override
    {
        return std::make_shared<DirtyLabel>(*this);
    }

    static Ptr create(const tgui::String& text = "")
    {
        auto label = std::make_shared<DirtyLabel>();

        if (!text.empty())
            label->setText(text);

        return label;
    }

    void setText2(const std::string& string)
    {
        this->m_string = string;

        rearrangeText3();
        
    }

    void setFont(const tgui::Font& font)
    {
        using namespace tgui;

        if (font && m_text)
        {
            TGUI_ASSERT(std::dynamic_pointer_cast<tgui::BackendFontSFML>(font.getBackendFont()), "BackendTextSFML::setFont requires font of type BackendFontSFML");
            m_text->setFont(*std::static_pointer_cast<tgui::BackendFontSFML>(font.getBackendFont())->getInternalFont());
        }
        else
        {
            // If an empty font is replaced by an empty font then no work needs to be done
            if (!m_text)
                return;
            delete m_text;

            // sf::Text has no function to pass an empty font and we can't keep using a pointer to the old font (it might be destroyed)
            // So the text object has to be completely recreated with all properties copied except for the font.
            sf::Text* newText = new sf::Text{m_text->getFont()};

            newText->setString(m_text->getString());
            newText->setCharacterSize(m_text->getCharacterSize());
            newText->setStyle(m_text->getStyle());
            newText->setFillColor(m_text->getFillColor());
            newText->setOutlineColor(m_text->getOutlineColor());
            newText->setOutlineThickness(m_text->getOutlineThickness());
            m_text = std::move(newText);
        }
    }

    bool first = true;
    void rearrangeText3()
    {
        using namespace tgui;

        if (m_fontCached == nullptr)
            return;

        if (first)
        {
            m_text->setCharacterSize(getTextSize());
            setFont(m_fontCached);
            m_text->setStyle(m_textStyleCached);
            m_text->setString(sf::String{ m_string.toStdString()});
            m_text->setFillColor(sf::Color::Black);
            m_text->setString(m_string.toStdString());
        }
        else
        {
            m_text->setString(m_string.toStdString());
        }

        first = false;
    }

    void draw(tgui::BackendRenderTarget&, tgui::RenderStates states) const override
    {
        assert(target);
        if (!target)
            return;

        using namespace tgui;

        RenderStates movedStates = states;
        movedStates.transform.translate(getPosition());

        // Round the position to avoid blurry text.
        // The top position is floored instead of rounded because it often results in the text looking more centered. A small
        // number is added before flooring to prevent 0.99 to be "rounded" to 0.
        sf::RenderStates sfStates;
        const std::array<float, 16> matrix = movedStates.transform.getMatrix();
        sfStates.transform = sf::Transform{ matrix[0], matrix[4], std::round(matrix[12]),
                                           matrix[1], matrix[5], std::floor(matrix[13] + 0.1f),
                                           matrix[3], matrix[7], matrix[15] };
        
        target->draw(*m_text);

    }
};

#endif // _GAME_GUI