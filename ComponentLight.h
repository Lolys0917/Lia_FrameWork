#pragma once

#include "Manager.h"

#include <DirectXMath.h>

enum LightType;

//全体の明るさ
static int OverallBrightness;

class Light : public Component
{
public:
	using Component::Component;
	void Init() override;
	void Update() override;
	void Draw() override;
	void Release() override;

	//位置・角度・は親クラス準拠

    // ---- 設定 ----
    void SetLightType(LightType type) { m_Type = type; }
    LightType GetLightType() const { return m_Type; }

    void SetColor(const DirectX::XMFLOAT4& color) { m_Color = color; }
    DirectX::XMFLOAT4 GetColor() const { return m_Color; }

    void SetLightIntensity(float intensity) { m_Intensity = intensity; }
    float GetLightIntensity() const { return m_Intensity; }

    void SetRange(float range) { m_Range = range; }
    float GetRange() const { return m_Range; }

    void SetShadowEnable(bool enable) { m_ShadowEnable = enable; }
    bool GetShadowEnable() const { return m_ShadowEnable; }

    void SetShadowResolution(int res) { m_ShadowResolution = res; }
    int GetShadowResolution() const { return m_ShadowResolution; }

    void SetShadowIntensity(float i) { m_ShadowIntensity = i; }
    float GetShadowIntensity() const { return m_ShadowIntensity; }

    void SetShadowBlur(float b) { m_ShadowBlur = b; }
    float GetShadowBlur() const { return m_ShadowBlur; }

    void SetDebugDisplay(bool d) { m_DebugDisplay = d; }
    bool GetDebugDisplay() const { return m_DebugDisplay; }

    struct LightData
    {
        DirectX::XMFLOAT4 position;     //pos + type
        DirectX::XMFLOAT4 direction;    //dir
        DirectX::XMFLOAT4 color;        //color r,g,b,a
        DirectX::XMFLOAT4 param;	    //range, shadow,
    };
    struct LightConstantBuffer
    {
        LightData lightData;
        int lightCount;
        DirectX::XMFLOAT3 padding;
	};

private:
	LightType m_Type = LightType::DirectionalLight;
	DirectX::XMFLOAT4 m_Color = { 1,1,1,1 };
	float m_Intensity = 1.0f;
	float m_Range = 10.0f;

	bool  m_ShadowEnable = false;
	int   m_ShadowResolution = 1024;
	float m_ShadowIntensity = 1.0f;
	float m_ShadowBlur = 0.0f;

	bool  m_DebugDisplay = false;
};