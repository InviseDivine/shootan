#include <Scene.hpp>
#include "elements/Element.hpp"

void Scene::registerElement(Gui::Element* element) {
    element->m_scene = this;
    m_elements.push_back(element);
}

void Scene::render() {
    if (!this) return;
    for (auto& el : m_elements) {
        el->draw();
    }
}