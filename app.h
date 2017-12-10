//------------------------------------------------------------------------------
// File: app.h
// Desc: Demonstration of a simple glow technique.
//
// Created: 04 July 2003 13:18:17
//
// (c)2003 Neil Wakefield
//------------------------------------------------------------------------------


#ifndef INCLUSIONGUARD_APP_H
#define INCLUSIONGUARD_APP_H


//------------------------------------------------------------------------------
// Included files:
//------------------------------------------------------------------------------
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>
#include <d3dx9.h>
#include <dxutil.h>
#include <d3dutil.h>
#include <d3denumeration.h>
#include <d3dsettings.h>
#include <d3dapp.h>
#include <d3dres.h>


//------------------------------------------------------------------------------
// Prototypes and declarations:
//------------------------------------------------------------------------------
class CD3DFont;

//------------------------------------------------------------------------------
// Name: class App
// Desc: The core of application, using the CD3DApp framework
//------------------------------------------------------------------------------
class App : public CD3DApplication
{
public:
	App();
	~App();

	HRESULT OneTimeSceneInit();
	HRESULT FinalCleanup();
	HRESULT InitDeviceObjects();
	HRESULT DeleteDeviceObjects();
	HRESULT RestoreDeviceObjects();
	HRESULT InvalidateDeviceObjects();
	HRESULT Render();
	HRESULT FrameMove();
	HRESULT ConfirmDevice( D3DCAPS9* pCaps, DWORD behavior,
						   D3DFORMAT adaptorFormat, D3DFORMAT backBufferFormat );
	LRESULT MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );

private:
	//app states
	bool m_drawGlow;
	bool m_drawGlowTexture;

	//font used for rendering the UI
	CD3DFont* m_pFont;

	//the user camera
	CD3DArcBall* m_pArcBall;
	D3DXMATRIX m_matWorld;
	D3DXMATRIX m_matView;
	D3DXMATRIX m_matProjection;

	//shaders
	LPDIRECT3DVERTEXSHADER9 m_pvsNormal;
	LPDIRECT3DVERTEXSHADER9 m_pvsGlow;
	LPDIRECT3DVERTEXSHADER9 m_pvsGaussianX;
	LPDIRECT3DVERTEXSHADER9 m_pvsGaussianY;
	LPDIRECT3DPIXELSHADER9 m_ppsModAlpha;
	LPDIRECT3DPIXELSHADER9 m_ppsGaussian;

	//rendering surface
	LPDIRECT3DTEXTURE9 m_pRT1, m_pRT2;
	LPDIRECT3DVERTEXBUFFER9 m_pQuadVB;	//screen-aligned quad
	struct QuadVertex { float x,y,z; float tu,tv; };
	LPDIRECT3DVERTEXDECLARATION9 m_pQuadDecl;

	//spaceship model
	LPD3DXMESH		m_pMesh;
	D3DXVECTOR3*	m_materials;
	DWORD			m_numMaterials;

	//background
	LPDIRECT3DTEXTURE9 m_pBackground;

};


#endif //INCLUSIONGUARD_APP_H
