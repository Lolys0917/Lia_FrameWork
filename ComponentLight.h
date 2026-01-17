#pragma once

#include "Manager.h"

#include <DirectXMath.h>

enum LightType
{
	LightType_Directional,
	LightType_Point,
	LightType_Spot,
};

class Light : public Component
{
public:
	using Component::Component;
	void Init() override;
	void Update() override;
	void Draw() override;
	void Release() override;

	//ライトの種類
	void SetLightType(LightType type);
	LightType GetLightType();
	//ライトの方向
	void SetDirection(const DirectX::XMFLOAT3& dir);
	DirectX::XMFLOAT3 GetDirection();
	//ライトの色
	void SetColor(const DirectX::XMFLOAT4& color);
	DirectX::XMFLOAT4 GetColor();
	//ライトの強度
	void SetLightIntensity(float intensity);
	float GetLightIntensity();
	//ライトの位置
	void SetPosition(const DirectX::XMFLOAT3& pos);
	DirectX::XMFLOAT3 GetPosition();
	//ライトの範囲
	void SetRange(float range);
	float GetRange();
	//スポットライトの角度
	void SetSpotAngle(float angle);
	float GetSpotAngle();

	//影の有効無効
	void SetShadowEnable(bool enable);
	bool GetShadowEnable();
	//影の解像度
	void SetShadowResolution(int resolution);
	int GetShadowResolution();
	//影の強度
	void SetShadowIntensity(float intensity);
	float GetShadowIntensity();
	//影のぼかし
	void SetShadowBlur(float blur);
	float GetShadowBlur();

	//ライトデバッグ表示
	void SetDebugDisplay(bool display);
	bool GetDebugDisplay();
private:

};