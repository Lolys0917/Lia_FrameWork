#pragma once

#include "Manager.h"
#include "Main.h"

#include <d3dcompiler.h>
#include <wrl/client.h>
#include <vector>

using Microsoft::WRL::ComPtr;
using namespace DirectX;

class SpriteBox : public Component
{
public:
	using Component::Component;

	~SpriteBox() override;

	void Init()override;
	void Draw()override;
	void Release()override;

	void SetTextureTop(const char* assetPath);
	void SetTextureBottom(const char* assetPath);
	void SetTextureFront(const char* assetPath);
	void SetTextureRear(const char* assetPath);
	void SetTextureLeft(const char* assetPath);
	void SetTextureRight(const char* assetPath);

	void SetPos(float x, float y, float z);
	void SetSize(float x, float y, float z);
	void SetAngle(float x, float y, float z);
	void SetColor(float r, float g, float b, float a);

	void SetView(const XMMATRIX& view);
	void SetProj(const XMMATRIX& proj);

private:
	struct Vertex{
		XMFLOAT3 pos;
		XMFLOAT2 uv;
	};
	struct MatrixBuffer {
		XMMATRIX mvp;
		XMFLOAT4 diffuseColor;
		int useTexture;
		XMFLOAT3 pad;
	};
	struct ColorBuffer {
		XMFLOAT4 color;
	};

	XMMATRIX ViewSet;
	XMMATRIX ProjSet;

	XMFLOAT3 m_pos{ 0,0,0 };
	XMFLOAT3 m_angle{ 0,0,0 };
	XMFLOAT2 m_size{ 1,1 };
	XMFLOAT4 m_color{ 1,1,1,1 };

	ID3D11ShaderResourceView* m_srvTop = nullptr;
	ID3D11ShaderResourceView* m_srvBottom = nullptr;
	ID3D11ShaderResourceView* m_srvFront = nullptr;
	ID3D11ShaderResourceView* m_srvRear = nullptr;
	ID3D11ShaderResourceView* m_srvLeft = nullptr;
	ID3D11ShaderResourceView* m_srvRight = nullptr;

	ComPtr<ID3D11Buffer> m_vbTop;
	ComPtr<ID3D11Buffer> m_vbBottom;
	ComPtr<ID3D11Buffer> m_vbFront;
	ComPtr<ID3D11Buffer> m_vbRear;
	ComPtr<ID3D11Buffer> m_vbLeft;
	ComPtr<ID3D11Buffer> m_vbRight;

	ComPtr<ID3D11Buffer> m_matrixBuf;
	ComPtr<ID3D11Buffer> m_colorBuf;
	ComPtr<ID3D11InputLayout> m_layout;
	ComPtr<ID3D11VertexShader> m_vs;
	ComPtr<ID3D11PixelShader> m_ps;

	ComPtr<ID3D11SamplerState> m_samplerState = nullptr;
	ComPtr<ID3D11BlendState> m_blendState = nullptr;

	ComPtr<ID3D11DepthStencilState> m_depthState = nullptr;

	UINT m_topVertexCount;
	UINT m_bottomVertexCount;

	void BuildMesh();
};