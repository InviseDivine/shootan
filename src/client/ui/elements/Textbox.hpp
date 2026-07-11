#pragma once
#include "Element.hpp"
#include <vector>
#include <string>

namespace Gui {
    class Textbox : public Element {
    public:
        Textbox(Rectangle bounds = {0}, size_t maxLength = 255) : Element(bounds), m_chars(maxLength) {}
        void draw();

        std::string getString() { return std::string(m_chars.begin(), m_chars.end()); }
        void setString(const std::string& str) { m_chars.reserve(str.size()); std::copy(str.begin(), str.end(), m_chars.begin()); }

    protected:
        std::vector<char> m_chars;
    };
};