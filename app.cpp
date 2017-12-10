//------------------------------------------------------------------------------
// File: app.cpp
// Desc: Demonstration of a simple glow technique.
//
// Created: 04 July 2003 13:17:33
//
// (c)2003 Neil Wakefield
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Included files:
//------------------------------------------------------------------------------
#include "app.h"
#include "resource.h"

#include <new>
#include <d3dfont.h>

//used for memory-leak checking in debug builds
#if defined(_DEBUG) || defined(DEBUG)
#include "crtdbg.h"
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif


//------------------------------------------------------------------------------
// Link Libraries:
//------------------------------------------------------------------------------
#pragma comment( lib, "winmm.lib" )
#pragma comment( lib, "d3d9.lib" )
#pragma comment( lib, "d3dx9.lib" )


//------------------------------------------------------------------------------
// Definitions:
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Name: App()
// Desc: Constructor for the application class
//------------------------------------------------------------------------------
App::App()
{
	//window settings
	m_strWindowTitle	= _T( "Glow demo - (c)2003 Neil Wakefield" );
	m_dwCreationHeight	= 600;
	m_dwCreationWidth	= 800;

	//enable z-buffering
	m_d3dEnumeration.AppUsesDepthBuffer	= true;
	m_d3dEnumeration.AppMinAlphaChannelBits = 8;

	//initialise member variables
	m_pFont			= NULL;
	m_pArcBall		= NULL;
	m_pMesh			= NULL;
	m_pvsNormal		= NULL;
	m_pvsGlow		= NULL;
	m_pvsGaussianX	= NULL;
	m_pvsGaussianY	= NULL;
	m_ppsModAlpha	= NULL;
	m_ppsGaussian	= NULL;
	m_pQuadVB		= NULL;
	m_pQuadDecl		= NULL;
	m_pBackground	= NULL;

	//set app states
	m_drawGlow			= true;
	m_drawGlowTexture	= false;
}

//------------------------------------------------------------------------------
// Name: ~App()
// Desc: Destructor for the application class
//------------------------------------------------------------------------------
App::~App()
{
}

//------------------------------------------------------------------------------
// Name: OneTimeSceneInit()
// Desc: Sets up application-specific data on first run
//------------------------------------------------------------------------------
HRESULT App::OneTimeSceneInit()
{
	HRESULT hr = S_OK;

	//create a font
	try{ m_pFont = new CD3DFont( _T( "Arial" ), 12, D3DFONT_BOLD ); }
	catch( std::bad_alloc& error )
	{
		OutputDebugString( error.what() );
		OutputDebugString( "\n" );
		return E_OUTOFMEMORY;
	}

	//create an arcball controller
	try{ m_pArcBall = new CD3DArcBall(); }
	catch( std::bad_alloc& error )
	{
		OutputDebugString( error.what() );
		OutputDebugString( "\n" );
		return E_OUTOFMEMORY;
	}

	return hr;
}

//------------------------------------------------------------------------------
// Name: InitDeviceObjects
// Desc: Sets up device-specific data on startup and device change
//------------------------------------------------------------------------------
HRESULT App::InitDeviceObjects()
{
	HRESULT hr = S_OK;
	
	//initialise the font
	hr = m_pFont->InitDeviceObjects( m_pd3dDevice );
	if( FAILED( hr ) ) return hr;

	//load the mesh file
	LPD3DXBUFFER pMaterials = NULL;

	#if defined(_DEBUG) || defined(DEBUG)
	hr = D3DXLoadMeshFromX( "model.x", D3DXMESH_MANAGED, m_pd3dDevice, NULL,
							&pMaterials, NULL, &m_numMaterials, &m_pMesh );
	#else
	hr = D3DXLoadMeshFromXResource( NULL, MAKEINTRESOURCE(IDR_X_MODEL), "X",
									D3DXMESH_MANAGED, m_pd3dDevice, NULL,
									&pMaterials, NULL, &m_numMaterials, &m_pMesh );
	#endif
	if( FAILED( hr ) ) return hr;

	//store materials
	try{ m_materials = new D3DXVECTOR3[ m_numMaterials ]; }
	catch( std::bad_alloc& error )
	{
		OutputDebugString( error.what() );
		OutputDebugString( "\n" );
		return E_OUTOFMEMORY;
	}

	D3DXMATERIAL* pMats = reinterpret_cast< D3DXMATERIAL* >( pMaterials->GetBufferPointer() );
	for( DWORD material = 0; material < m_numMaterials; ++material )
	{
		D3DMATERIAL9& mat = pMats[ material ].MatD3D;
		m_materials[ material ] = D3DXVECTOR3( mat.Diffuse.r, mat.Diffuse.g, mat.Diffuse.b );
	}

	//HACK: set material 2 to a different colour as black doesn't look very good
	m_materials[ 2 ] = D3DXVECTOR3( 0.5f, 0.5f, 0.5f );

	SAFE_RELEASE( pMaterials );

	//load shaders
	LPD3DXBUFFER pCode, pErrors;
	DWORD flags = 0;
	#if defined(_DEBUG) || defined(DEBUG)
	flags |= D3DXSHADER_DEBUG;
	#endif

	#if defined(_DEBUG) || defined(DEBUG)
	hr = D3DXAssembleShaderFromFile( "normal.vsh", NULL, NULL, flags, &pCode, &pErrors );
	#else
	hr = D3DXAssembleShaderFromResource( NULL, MAKEINTRESOURCE( IDR_VS_NORMAL ), NULL,
										 NULL, flags, &pCode, &pErrors );
	#endif
	if( FAILED( hr ) )
	{
		OutputDebugString( "Failed to assemble vertex shader (normal), errors:\n" );
		OutputDebugString( (char*)pErrors->GetBufferPointer() );
		OutputDebugString( "\n" );

		SAFE_RELEASE( pCode );
		SAFE_RELEASE( pErrors );

		return hr;
	}

	hr = m_pd3dDevice->CreateVertexShader( (DWORD*)pCode->GetBufferPointer(), &m_pvsNormal );
	if( FAILED( hr ) ) return hr;

	SAFE_RELEASE( pCode );
	SAFE_RELEASE( pErrors );

	#if defined(_DEBUG) || defined(DEBUG)
	hr = D3DXAssembleShaderFromFile( "glow.vsh", NULL, NULL, flags, &pCode, &pErrors );
	#else
	hr = D3DXAssembleShaderFromResource( NULL, MAKEINTRESOURCE( IDR_VS_GLOW ), NULL,
										 NULL, flags, &pCode, &pErrors );
	#endif
	if( FAILED( hr ) )
	{
		OutputDebugString( "Failed to assemble vertex shader (glow), errors:\n" );
		OutputDebugString( (char*)pErrors->GetBufferPointer() );
		OutputDebugString( "\n" );

		SAFE_RELEASE( pCode );
		SAFE_RELEASE( pErrors );

		return hr;
	}

	hr = m_pd3dDevice->CreateVertexShader( (DWORD*)pCode->GetBufferPointer(), &m_pvsGlow );
	if( FAILED( hr ) ) return hr;

	SAFE_RELEASE( pCode );
	SAFE_RELEASE( pErrors );

	#if defined(_DEBUG) || defined(DEBUG)
	hr = D3DXAssembleShaderFromFile( "gaussianx.vsh", NULL, NULL, flags, &pCode, &pErrors );
	#else
	hr = D3DXAssembleShaderFromResource( NULL, MAKEINTRESOURCE( IDR_VS_GAUSSIANX ), NULL,
										 NULL, flags, &pCode, &pErrors );
	#endif
	if( FAILED( hr ) )
	{
		OutputDebugString( "Failed to assemble vertex shader (gaussianx), errors:\n" );
		OutputDebugString( (char*)pErrors->GetBufferPointer() );
		OutputDebugString( "\n" );

		SAFE_RELEASE( pCode );
		SAFE_RELEASE( pErrors );

		return hr;
	}

	hr = m_pd3dDevice->CreateVertexShader( (DWORD*)pCode->GetBufferPointer(), &m_pvsGaussianX );
	if( FAILED( hr ) ) return hr;

	SAFE_RELEASE( pCode );
	SAFE_RELEASE( pErrors );

	#if defined(_DEBUG) || defined(DEBUG)
	hr = D3DXAssembleShaderFromFile( "gaussiany.vsh", NULL, NULL, flags, &pCode, &pErrors );
	#else
	hr = D3DXAssembleShaderFromResource( NULL, MAKEINTRESOURCE( IDR_VS_GAUSSIANY ), NULL,
										 NULL, flags, &pCode, &pErrors );
	#endif
	if( FAILED( hr ) )
	{
		OutputDebugString( "Failed to assemble vertex shader (gaussiany), errors:\n" );
		OutputDebugString( (char*)pErrors->GetBufferPointer() );
		OutputDebugString( "\n" );

		SAFE_RELEASE( pCode );
		SAFE_RELEASE( pErrors );

		return hr;
	}

	hr = m_pd3dDevice->CreateVertexShader( (DWORD*)pCode->GetBufferPointer(), &m_pvsGaussianY );
	if( FAILED( hr ) ) return hr;

	SAFE_RELEASE( pCode );
	SAFE_RELEASE( pErrors );

	#if defined(_DEBUG) || defined(DEBUG)
	hr = D3DXAssembleShaderFromFile( "modalpha.psh", NULL, NULL, flags, &pCode, &pErrors );
	#else
	hr = D3DXAssembleShaderFromResource( NULL, MAKEINTRESOURCE( IDR_PS_MODALPHA ), NULL,
										 NULL, flags, &pCode, &pErrors );
	#endif
	if( FAILED( hr ) )
	{
		OutputDebugString( "Failed to assemble pixel shader (modalpha), errors:\n" );
		OutputDebugString( (char*)pErrors->GetBufferPointer() );
		OutputDebugString( "\n" );

		SAFE_RELEASE( pCode );
		SAFE_RELEASE( pErrors );

		return hr;
	}

	hr = m_pd3dDevice->CreatePixelShader( (DWORD*)pCode->GetBufferPointer(), &m_ppsModAlpha );
	if( FAILED( hr ) ) return hr;

	SAFE_RELEASE( pCode );
	SAFE_RELEASE( pErrors );
	
	#if defined(_DEBUG) || defined(DEBUG)
	hr = D3DXAssembleShaderFromFile( "gaussian.psh", NULL, NULL, flags, &pCode, &pErrors );
	#else
	hr = D3DXAssembleShaderFromResource( NULL, MAKEINTRESOURCE( IDR_PS_GAUSSIAN ), NULL,
										 NULL, flags, &pCode, &pErrors );
	#endif
	if( FAILED( hr ) )
	{
		OutputDebugString( "Failed to assemble pixel shader (gaussian), errors:\n" );
		OutputDebugString( (char*)pErrors->GetBufferPointer() );
		OutputDebugString( "\n" );

		SAFE_RELEASE( pCode );
		SAFE_RELEASE( pErrors );

		return hr;
	}

	hr = m_pd3dDevice->CreatePixelShader( (DWORD*)pCode->GetBufferPointer(), &m_ppsGaussian );
	if( FAILED( hr ) ) return hr;

	SAFE_RELEASE( pCode );
	SAFE_RELEASE( pErrors );

	return hr;
}

//------------------------------------------------------------------------------
// Name: RestoreDeviceObjects
// Desc: Sets up device-specific data on res change
//------------------------------------------------------------------------------
HRESULT App::RestoreDeviceObjects()
{
	HRESULT hr = S_OK;

	//restore the font
	hr = m_pFont->RestoreDeviceObjects();
	if( FAILED( hr ) ) return hr;

	//set up the arcball control window
	m_pArcBall->SetRadius( 0.0f );
	m_pArcBall->SetWindow( m_d3dsdBackBuffer.Width, m_d3dsdBackBuffer.Height );

	//create the background texture
	const unsigned int width = m_d3dsdBackBuffer.Width / 4;
	const unsigned int height = m_d3dsdBackBuffer.Height / 4;
	hr = m_pd3dDevice->CreateTexture( width, height, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED,
									  &m_pBackground, NULL );
	if( FAILED( hr ) ) return hr;
	D3DLOCKED_RECT lockedRect;
	hr = m_pBackground->LockRect( 0, &lockedRect, NULL, 0 );
	if( FAILED( hr ) ) return hr;

	//draw a starscape
	DWORD* pPixel = (DWORD*)lockedRect.pBits;
	for( DWORD i = 0; i < height; ++i )
	{
		for( int j = 0; j < ( lockedRect.Pitch / 4 ); ++j )
		{
			const int randVal = rand() % 500;
			if( randVal == 1 )
				*pPixel = 0xffffffff;	//slight glow on stars
			else
				*pPixel = 0x00151515;
			
			++pPixel;
		}
	}

	m_pBackground->UnlockRect( 0 );

	//create temp rendertargets
	const unsigned int widthSmall = m_d3dsdBackBuffer.Width / 4;
	const unsigned int heightSmall = m_d3dsdBackBuffer.Height / 4;

	hr = m_pd3dDevice->CreateTexture( widthSmall, heightSmall, 1, D3DUSAGE_RENDERTARGET,
									  m_d3dpp.BackBufferFormat, D3DPOOL_DEFAULT,
									  &m_pRT1, NULL );
	if( FAILED( hr ) ) return hr;

	hr = m_pd3dDevice->CreateTexture( widthSmall, heightSmall, 1, D3DUSAGE_RENDERTARGET,
									  m_d3dpp.BackBufferFormat, D3DPOOL_DEFAULT,
									  &m_pRT2, NULL );
	if( FAILED( hr ) ) return hr;

	//create screen-aligned quad for rendertargets
	const int VB_SIZE = 4 * sizeof( QuadVertex );
	if( FAILED( m_pd3dDevice->CreateVertexBuffer( VB_SIZE, D3DUSAGE_WRITEONLY, 0,
												  D3DPOOL_MANAGED, &m_pQuadVB, NULL ) ) )
		return E_FAIL;

	QuadVertex vertices[] =
	{
		{ -1.0f, 1.0f, 0.5f, 0.0f, 0.0f },
		{ 1.0f, 1.0f, 0.5f, 1.0f, 0.0f },
		{ 1.0f, -1.0f, 0.5f, 1.0f, 1.0f },
		{ -1.0f, -1.0f, 0.5f, 0.0f, 1.0f },
	};

	QuadVertex* pBuffer = NULL;
	if( FAILED( m_pQuadVB->Lock( 0, sizeof( vertices ), (void**)&pBuffer, 0 ) ) )
		return E_FAIL;
	memcpy( pBuffer, vertices, sizeof( vertices ) );
	m_pQuadVB->Unlock();

	//create vertex declaration for the quad
	D3DVERTEXELEMENT9 vsDecl[] =
	{
		{ 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
		{ 0, 12, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
		D3DDECL_END(),
	};
	if( FAILED( m_pd3dDevice->CreateVertexDeclaration( vsDecl, &m_pQuadDecl ) ) )
		return E_FAIL;

	//calculate camera orbiting distance
	struct MeshVertex { D3DXVECTOR3 p, n; };
	MeshVertex* pVertices = NULL;
	m_pMesh->LockVertexBuffer( 0, reinterpret_cast< void** >( &pVertices ) );
	
	float xMax = 0.0f, yMax = 0.0f, zMax = 0.0f;
	for( DWORD vertex = 0; vertex < m_pMesh->GetNumVertices(); ++vertex )
	{
		const D3DXVECTOR3& currentVertex = pVertices[ vertex ].p;
		xMax = max( xMax, currentVertex.x );
		yMax = max( yMax, currentVertex.y );
		zMax = max( zMax, currentVertex.z );
	}

	m_pMesh->UnlockVertexBuffer();

	//set the camera orbit distance
	const float objectRadius = sqrtf( xMax * xMax + yMax * yMax + zMax * zMax ) * 2.4f;
	const D3DXVECTOR3 vEyePt	= D3DXVECTOR3( 0.0f, 0.0f, -objectRadius );
	const D3DXVECTOR3 vLookAtPt	= D3DXVECTOR3( 0.0f, 0.0f, 0.0f );
	const D3DXVECTOR3 vUp		= D3DXVECTOR3( 0.0f, 1.0f, 0.0f );
    D3DXMatrixLookAtLH( &m_matView, &vEyePt, &vLookAtPt, &vUp );

	//set the projection matrix
	const float fAspect = m_d3dsdBackBuffer.Width / float(m_d3dsdBackBuffer.Height);
	D3DXMatrixPerspectiveFovLH( &m_matProjection, D3DX_PI/4, fAspect, 1.0f, 500.0f );

	//set the filtering mode
	m_pd3dDevice->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
	m_pd3dDevice->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
	m_pd3dDevice->SetSamplerState( 0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR );

	return hr;
}

//------------------------------------------------------------------------------
// Name: Render()
// Desc: Renders the current frame
//------------------------------------------------------------------------------
HRESULT App::Render()
{
	HRESULT hr = S_OK;

	//clear the z-buffer
	hr = m_pd3dDevice->Clear( 0, NULL, D3DCLEAR_ZBUFFER, 0, 1.0f, 0 );
	if( FAILED( hr ) ) return hr;

	//set the vertex shader constants
    D3DXMATRIX matTransform;
	D3DXMatrixMultiply( &matTransform, &m_matWorld, &m_matView );
	D3DXMatrixMultiplyTranspose( &matTransform, &matTransform, &m_matProjection );

	//render the scene
	if( SUCCEEDED( m_pd3dDevice->BeginScene() ) )
	{
		//draw the background plane
		m_pd3dDevice->SetRenderState( D3DRS_ZENABLE, FALSE );
		m_pd3dDevice->SetVertexDeclaration( m_pQuadDecl );
		m_pd3dDevice->SetVertexShader( m_pvsGaussianX );
		const D3DXVECTOR4 step( 0.001f, 0.001f, 1.0f, 1.0f );
		m_pd3dDevice->SetVertexShaderConstantF( 0, (float*)&step, 1 );
		m_pd3dDevice->SetTexture( 0, m_pBackground );
		m_pd3dDevice->SetStreamSource( 0, m_pQuadVB, 0, sizeof( QuadVertex ) );
		m_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLEFAN, 0, 2 );
		m_pd3dDevice->SetRenderState( D3DRS_ZENABLE, TRUE );
		m_pd3dDevice->SetVertexDeclaration( NULL );
		m_pd3dDevice->SetVertexShader( NULL );
		m_pd3dDevice->SetTexture( 0, NULL );

		m_pd3dDevice->SetVertexShader( m_pvsNormal );
		m_pd3dDevice->SetVertexShaderConstantF( 0, (float*)&matTransform, 4 );

		//render the model
		for( DWORD material = 0; material < m_numMaterials; ++material )
		{
			if( material == 0 || material == 1 )
				m_pd3dDevice->SetVertexShader( m_pvsGlow );

			m_pd3dDevice->SetVertexShaderConstantF( 4, (float*)m_materials[ material ], 1 );
			m_pMesh->DrawSubset( material );

			if( material == 0 || material == 1 )
				m_pd3dDevice->SetVertexShader( m_pvsNormal );
		}

		m_pd3dDevice->EndScene();
	}

	if( m_drawGlow )
	{
		//copy the back buffer to an offscreen surface
		LPDIRECT3DSURFACE9 pBackBuffer, pTextureSurface;
		m_pd3dDevice->GetRenderTarget( 0, &pBackBuffer );
		m_pRT1->GetSurfaceLevel( 0, &pTextureSurface );
		m_pd3dDevice->StretchRect( pBackBuffer, NULL, pTextureSurface, NULL, D3DTEXF_NONE );
		SAFE_RELEASE( pTextureSurface );

		//perform post-processing on texture...
		m_pd3dDevice->SetRenderState( D3DRS_ZENABLE, FALSE );
		m_pd3dDevice->SetVertexDeclaration( m_pQuadDecl );
		m_pd3dDevice->SetStreamSource( 0, m_pQuadVB, 0, sizeof( QuadVertex ) );
		m_pd3dDevice->SetVertexShader( m_pvsGaussianX );
		const float xstep = 10.0f / m_d3dsdBackBuffer.Width;
		const D3DXVECTOR4 stepVecX( xstep, xstep / 2.0f, 1.0f, 1.0f );
		m_pd3dDevice->SetVertexShaderConstantF( 0, (float*)&stepVecX, 1 );

		//modulate the texture by its alpha component
		//this gives an emissivity texture
		m_pRT2->GetSurfaceLevel( 0, &pTextureSurface );
		m_pd3dDevice->SetRenderTarget( 0, pTextureSurface );
		SAFE_RELEASE( pTextureSurface );
		
		if( SUCCEEDED( m_pd3dDevice->BeginScene() ) )
		{
			m_pd3dDevice->SetPixelShader( m_ppsModAlpha );
			m_pd3dDevice->SetTexture( 0, m_pRT1 );
			m_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLEFAN, 0, 2 );
			
			m_pd3dDevice->EndScene();
		}

		//blur the emissivity texture
		m_pRT1->GetSurfaceLevel( 0, &pTextureSurface );
		m_pd3dDevice->SetRenderTarget( 0, pTextureSurface );
		SAFE_RELEASE( pTextureSurface );
		m_pd3dDevice->SetPixelShader( m_ppsGaussian );
		
		if( SUCCEEDED( m_pd3dDevice->BeginScene() ) )
		{
			
			m_pd3dDevice->SetTexture( 0, m_pRT2 );
			m_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLEFAN, 0, 2 );
			
			m_pd3dDevice->EndScene();
		}

		m_pRT2->GetSurfaceLevel( 0, &pTextureSurface );
		m_pd3dDevice->SetRenderTarget( 0, pTextureSurface );
		SAFE_RELEASE( pTextureSurface );
		m_pd3dDevice->SetVertexShader( m_pvsGaussianY );
		const float ystep = 10.0f / m_d3dsdBackBuffer.Height;
		const D3DXVECTOR4 stepVecY( ystep, ystep / 2.0f, 1.0f, 1.0f );
		m_pd3dDevice->SetVertexShaderConstantF( 0, (float*)&stepVecY, 1 );
		
		if( SUCCEEDED( m_pd3dDevice->BeginScene() ) )
		{
			m_pd3dDevice->SetTexture( 0, m_pRT1 );
			m_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLEFAN, 0, 2 );
			
			m_pd3dDevice->EndScene();
		}

		//additively blend the blur texture with the framebuffer
		m_pd3dDevice->SetRenderTarget( 0, pBackBuffer );
		SAFE_RELEASE( pBackBuffer );
		if( ! m_drawGlowTexture )
		{
			m_pd3dDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
		}
		m_pd3dDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_ONE );

		m_pd3dDevice->SetPixelShader( NULL );
		if( SUCCEEDED( m_pd3dDevice->BeginScene() ) )
		{		
			m_pd3dDevice->SetTexture( 0, m_pRT2 );
			m_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLEFAN, 0, 2 );
			
			m_pd3dDevice->EndScene();
		}

		//unset states changed in post-processing
		m_pd3dDevice->SetRenderState( D3DRS_ZENABLE, TRUE );
		m_pd3dDevice->SetVertexDeclaration( NULL );
		m_pd3dDevice->SetStreamSource( 0, NULL, 0, 0 );
		m_pd3dDevice->SetTexture( 0, NULL );
		m_pd3dDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
		m_pd3dDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_ZERO );
	}

	//draw the UI
	if( SUCCEEDED( m_pd3dDevice->BeginScene() ) )
	{
		//render the frame statistics
		m_pFont->DrawText( 10, 10, 0xffffff00, m_strDeviceStats );
		m_pFont->DrawText( 10, 30, 0xffffff00, m_strFrameStats );
		if( m_drawGlow )
		{
			m_pFont->DrawText( 10, 50, 0xffffff00, "Glow is: ON   (1 to turn off)" );

			if( m_drawGlowTexture )
				m_pFont->DrawText( 10, 70, 0xffffff00, "Displaying glow texture only (4 to return to normal mode)" );
			else
				m_pFont->DrawText( 10, 70, 0xffffff00, "Press 3 to show glow texture" );
		}
		else
			m_pFont->DrawText( 10, 50, 0xffffff00, "Glow is: OFF (2 to turn on)" );

		m_pd3dDevice->EndScene();
	}

	return hr;
}

//------------------------------------------------------------------------------
// Name: FrameMove()
// Desc: Performs between-frame animation
//------------------------------------------------------------------------------
HRESULT App::FrameMove()
{
	HRESULT hr = S_OK;

	//update the camera rotation
	m_matWorld = *m_pArcBall->GetRotationMatrix();

	//process user input
	if( GetKeyState( '1' ) & 0x80 )
	{
		m_drawGlow = false;
	}
	else if( GetKeyState( '2' ) & 0x80 )
	{
		m_drawGlow = true;
	}
	if( GetKeyState( '3' ) & 0x80 )
	{
		m_drawGlowTexture = true;
	}
	else if( GetKeyState( '4' ) & 0x80 )
	{
		m_drawGlowTexture = false;
	}
	return hr;
}

//------------------------------------------------------------------------------
// Name: InvalidateDeviceObjects
// Desc: Tidies up device-specific data on res change
//------------------------------------------------------------------------------
HRESULT App::InvalidateDeviceObjects()
{
	HRESULT hr = S_OK;

	//invalidate the font
	hr = m_pFont->InvalidateDeviceObjects();
	if( FAILED( hr ) ) return hr;

	//unload the background
	SAFE_RELEASE( m_pBackground );

	//tidy up the render target
	SAFE_RELEASE( m_pRT1 );
	SAFE_RELEASE( m_pRT2 );
	SAFE_RELEASE( m_pQuadVB );
	SAFE_RELEASE( m_pQuadDecl );

	return hr;
}

//------------------------------------------------------------------------------
// Name: DeleteDeviceObjects
// Desc: Tidies up device-specific data on device change
//------------------------------------------------------------------------------
HRESULT App::DeleteDeviceObjects()
{
	HRESULT hr = S_OK;

	//delete the font resources
	hr = m_pFont->DeleteDeviceObjects();
	if( FAILED( hr ) ) return hr;

	//unload the mesh
	SAFE_RELEASE( m_pMesh );
	SAFE_DELETE_ARRAY( m_materials );
	m_numMaterials = 0;

	//unload the shaders
	SAFE_RELEASE( m_pvsNormal );
	SAFE_RELEASE( m_pvsGlow );
	SAFE_RELEASE( m_pvsGaussianX );
	SAFE_RELEASE( m_pvsGaussianY );
	SAFE_RELEASE( m_ppsModAlpha );
	SAFE_RELEASE( m_ppsGaussian );
	
	return hr;
}

//------------------------------------------------------------------------------
// Name: FinalCleanup()
// Desc: Tidies up application-specific data on shutdown
//------------------------------------------------------------------------------
HRESULT App::FinalCleanup()
{
	HRESULT hr = S_OK;

	//tidy up the font
	SAFE_DELETE( m_pFont );

	//tidy up the arcball
	SAFE_DELETE( m_pArcBall );

	return hr;
}

//-----------------------------------------------------------------------------
// Name: ConfirmDevice()
// Desc: Checks the device for some minimum set of capabilities
//-----------------------------------------------------------------------------
HRESULT App::ConfirmDevice( D3DCAPS9* pCaps, DWORD behavior, D3DFORMAT, D3DFORMAT )
{
	HRESULT hr = S_OK;

	UNREFERENCED_PARAMETER( behavior );
	UNREFERENCED_PARAMETER( pCaps );

	if( ( behavior & D3DCREATE_HARDWARE_VERTEXPROCESSING ) ||
		( behavior & D3DCREATE_MIXED_VERTEXPROCESSING ) )
	{
		if( pCaps->VertexShaderVersion < D3DVS_VERSION( 2, 0 ) )
		{
			return E_FAIL;
		}
	}

	//uncomment this if we need to use the ref device (eg for debugging shaders)
	/*if( pCaps->DeviceType != D3DDEVTYPE_REF )
		return E_FAIL;//*/

	return hr;
}

//-----------------------------------------------------------------------------
// Name: MsgProc()
// Desc: Message proc function to handle key and menu input
//-----------------------------------------------------------------------------
LRESULT App::MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	//pass mouse messages to the ArcBall so it can build internal matrices
	m_pArcBall->HandleMouseMessages( hWnd, uMsg, wParam, lParam );

	//trap context menu
	if( WM_CONTEXTMENU == uMsg )
		return 0;

	//pass remaining messages to default handler
	return CD3DApplication::MsgProc( hWnd, uMsg, wParam, lParam );
}

//------------------------------------------------------------------------------
// Name: WinMain()
// Desc: Entry point for the program
//------------------------------------------------------------------------------
int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE, LPSTR, int )
{
	//enable memory-leak checking in debug builds
	#if defined(_DEBUG) || defined(DEBUG)
	int flag = _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG );
	flag |= _CRTDBG_LEAK_CHECK_DF;
	_CrtSetDbgFlag( flag );
	#endif

	App myApp;
	myApp.Create( hInstance );
	return myApp.Run();
}