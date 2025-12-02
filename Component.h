#pragma once

class Component
{
protected:
    class Object* object = nullptr;

    struct FLOAT3
    {
        float x;
        float y;
        float z;
    };
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
private:
    FLOAT3 position;
    FLOAT3 size;
    FLOAT3 angle;
public:
    void SetPosition(float x, float y, float z)
        { position = {x,y,z}; }
    void SetSize(float x, float y, float z)
        { size = {x,y,z}; }
    void SetAngle(float x, float y, float z)
        { angle = {x,y,z}; }

    FLOAT3 GetPosition() { return position; }
    FLOAT3 GetSize() { return size; }
    FLOAT3 GetAngle() { return angle; }

};