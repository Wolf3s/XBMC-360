#ifndef H_CXBAPPLICATIONEX
#define H_CXBAPPLICATIONEX

#include "utils\Stdafx.h"
#include "xbox\XBInput.h"
#include <xui.h>

class CXBApplicationEX
{
public:
	CXBApplicationEX();
	~CXBApplicationEX();

	bool Create();

	virtual bool Initialize() = 0;
	virtual void Process() = 0;
	virtual void FrameMove() = 0;
	virtual void Render() = 0;
	virtual void Stop() = 0;
	virtual void Cleanup() {};

	int Run();
	void Destroy();

	bool IsStopping() { return m_bStop; };

protected:
	// Main Direct3D objects
	LPDIRECT3D9						m_pD3D;
	IDirect3DDevice9*				m_pd3dDevice;	
	D3DPRESENT_PARAMETERS			m_d3dpp;	

	// XUI objects
	HXUIDC							m_hXUIDC; // Xui device context
	XUIInitParams					m_XUIParams;

	// Members to init the XBINPUT devices.
	GAMEPAD	   m_Gamepads[4];
	GAMEPAD    m_DefaultGamepad;

	bool m_bStop;

private:
 	void ReadInput();
};

#endif //H_CXBAPPLICATIONEX