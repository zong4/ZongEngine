#include "LayerStack.hpp"

zong::core::LayerStack::~LayerStack()
{
    for (Layer* layer : _layers)
    {
        layer->onDetach();
        delete layer;
    }
}

void zong::core::LayerStack::pushLayer(Layer* layer)
{
    _layers.emplace(_layers.begin() + _layerInsertIndex, layer);
    ++_layerInsertIndex;
}

void zong::core::LayerStack::pushOverlay(Layer* overlay)
{
    _layers.emplace_back(overlay);
}

void zong::core::LayerStack::popLayer(Layer* layer)
{
    auto const it = std::find(_layers.begin(), _layers.begin() + _layerInsertIndex, layer);

    if (it != _layers.begin() + _layerInsertIndex)
    {
        layer->onDetach();
        _layers.erase(it);
        --_layerInsertIndex;
    }
}

void zong::core::LayerStack::popOverlay(Layer* overlay)
{
    auto const it = std::find(_layers.begin() + _layerInsertIndex, _layers.end(), overlay);

    if (it != _layers.end())
    {
        overlay->onDetach();
        _layers.erase(it);
    }
}
