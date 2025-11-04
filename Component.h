#pragma once

class Component
{
protected:
    class Object* object = nullptr;
public:
    //デフォルトコンストラクタ消去
    Component() = delete;
    //新しいコンストでGameObjに登録させる
    Component(Object* obj)
    {
        object = obj;
    };

    virtual ~Component() {}
    virtual void Init() {}
    virtual void Update() {}
    virtual void Draw() {}
    virtual void Release() {}
};