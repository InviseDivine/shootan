#pragma once
#include "Element.hpp"
#include <string>

namespace Gui {
    class Button : public Element {
    public:
        Button(const std::string& label = "") : Button({ 0 }, label) {}
        Button(Rectangle bounds = { 0 }, const std::string& label = "") : Element(bounds), m_label(label) {}
        void draw();

    protected:
        std::string m_label;
    };
};