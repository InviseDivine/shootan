#include <Scene.hpp>
#include "elements/Element.hpp"

void Scene::registerElement(Gui::Element* element) {
    element->m_scene = this;
    m_elements.push_back(element);
}

void Scene::render() {
    for (auto& el : m_elements) {
        el->draw();
    }
}