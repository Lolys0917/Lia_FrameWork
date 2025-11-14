#pragma once

#include "Component.h"
#include "Manager.h"
#include "Main.h"
#include <DirectXMath.h>
#include <wrl.h>

using namespace DirectX;

class SpriteCylinder : public Component
{
public:
	using Component::Component;

	void Init() override;
	void Update() override {}
	void Draw()override;
	void Release()override;

	void SetPos(float x, float y, float z);
	void SetSize(float radius, float height);
	void SetAngle(float rx, float ry, float rz);
	void SetColor(float r, float g, float b, float a);

    void SetView(const XMMATRIX& view);
    void SetProj(const XMMATRIX& proj);

	void SetSideTexture(const char* path);
	void SetTopTexture(const char* path);
	void SetBottomTexture(const char* path);

	void SetSegment(int seg);

private:
	struct Vertex {
		XMFLOAT3 pos;
		XMFLOAT2 uv;
	};
	struct MatrixBuffer {
		XMMATRIX mvp;           // 64 bytes
		XMFLOAT4 diffuseColor;  // 16 bytes
		int useTexture;         // 4 bytes
		XMFLOAT3 pad;           // 12 bytes -> çáåv 96 (=16*6)
	};
	struct ColorBuffer {
		XMFLOAT4 color;
	};

	XMFLOAT3 m_pos{ 0,0,0 };
	XMFLOAT3 m_angle{ 0,0,0 };
	XMFLOAT2 m_size{ 1,1 };
	XMFLOAT4 m_color{ 1,1,1,1 };

    XMMATRIX ViewSet;
    XMMATRIX ProjSet;

	ID3D11ShaderResourceView* m_srvSide = nullptr;
	ID3D11ShaderResourceView* m_srvTop = nullptr;
	ID3D11ShaderResourceView* m_srvBottom = nullptr;

	Microsoft::WRL::ComPtr<ID3D11Buffer> m_vbSide;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_vbTop;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_vbBottom;

	int m_seg = 32; // â~é¸ï™äÑêî

	Microsoft::WRL::ComPtr<ID3D11InputLayout> m_layout;
	Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vs;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> m_ps;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_matrixBuf;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_colorBuf;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> m_sampler;

	void BuildMesh();
};
