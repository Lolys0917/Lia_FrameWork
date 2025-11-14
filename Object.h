#pragma once

#include "Component.h"
#include "ComponentSpriteWorld.h"
#include "ComponentCamera.h"
#include "Grid.h"
#include "ComponentSpriteScreen.h"
#include "ComponentSpriteCylinder.h"
#include "ComponentModel.h"
#include <vector>

class Object
{
private:

public:
    std::vector<std::vector <class Component*>> m_lpComp;

    int UseScene = -1;
    bool Active = true;

    virtual void Init() {

    }
    virtual void Update() {
        for (auto& i : m_lpComp)
        {
            for (auto& ii : i)
            {
                ii->Update();
            }
        }

    }
    virtual void Draw() {
        for (auto& i : m_lpComp)
        {
            for (auto& ii : i)
            {
                ii->Draw();
            }
        }
    }
    virtual void Release() {
        for (auto& i : m_lpComp)
        {
            for (auto& ii : i)
            {
                ii->Release();
            }
        }
    }

    template<typename T = Component>
    T* AddComponent()
    {
        //static_assert(std::is_base_of<Component, T>::value, "T must be a Component");

        static_assert(std::is_base_of<Component, T>::value, "T must inherit Component");

        if (m_lpComp.size() < 6) m_lpComp.resize(6);

        int type = -1;

        if constexpr (std::is_same_v<T, class Camera>) type = 0;
        else if constexpr (std::is_same_v<T, class Grid>) type = 1;
        else if constexpr (std::is_same_v<T, class Model>) type = 2;
        else if constexpr (std::is_same_v<T, class SpriteWorld>) type = 3;
        else if constexpr (std::is_same_v<T, class SpriteScreen>) type = 4;
        else if constexpr (std::is_same_v<T, class SpriteCylinder>) type = 5;
        else type = -1;


        if (type == -1) return nullptr; // –¢’m‚ÌŒ^

        T* component = new T(this);  // this‚Í Template*
        m_lpComp[type].push_back(component);
        component->Init();
        return component;
    }
    template <typename T = Component>
    T* GetComponent(int index)
    {
        int type = -1;

        if constexpr (std::is_same_v<T, class Camera>) type = 0;
        else if constexpr (std::is_same_v<T, class Grid>) type = 1;
        else if constexpr (std::is_same_v<T, class Model>) type = 2;
        else if constexpr (std::is_same_v<T, class SpriteWorld>) type = 3;
        else if constexpr (std::is_same_v<T, class SpriteScreen>) type = 4;
        else if constexpr (std::is_same_v<T, class SpriteCylinder>) type = 5;
        else type = -1;


        if (type == -1) return nullptr; // –¢’m‚ÌŒ^


        if (index < 0 || index >= m_lpComp[type].size())
            return nullptr;
        return dynamic_cast<T*>(m_lpComp[type][index]);
    }
    template <typename T = Component>
    int GetSize()
    {
        int type = -1;

        if constexpr (std::is_same_v<T, class Camera>) type = 0;
        else if constexpr (std::is_same_v<T, class Grid>) type = 1;
        else if constexpr (std::is_same_v<T, class Model>) type = 2;
        else if constexpr (std::is_same_v<T, class SpriteWorld>) type = 3;
        else if constexpr (std::is_same_v<T, class SpriteScreen>) type = 4;
        else if constexpr (std::is_same_v<T, class SpriteCylinder>) type = 5;
        else type = -1;


        if (type == -1) return -1; // –¢’m‚ÌŒ^

        return static_cast<int>(m_lpComp[type].size());
    }
};