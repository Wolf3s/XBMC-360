#ifndef H_CVIDEOSETTINGS
#define H_CVIDEOSETTINGS

enum EINTERLACEMETHOD
{
	VS_INTERLACEMETHOD_NONE = 0,
	VS_INTERLACEMETHOD_AUTO = 1,
	VS_INTERLACEMETHOD_RENDER_BLEND = 2,

	VS_INTERLACEMETHOD_RENDER_WEAVE_INVERTED = 3,
	VS_INTERLACEMETHOD_RENDER_WEAVE = 4,
  
	VS_INTERLACEMETHOD_RENDER_BOB_INVERTED = 5,
	VS_INTERLACEMETHOD_RENDER_BOB = 6,

	VS_INTERLACEMETHOD_DEINTERLACE = 7
};

class CVideoSettings
{
public:
	CVideoSettings();
	~CVideoSettings() {};

	float m_AudioDelay;
	int m_AudioStream;
	bool m_PostProcess;
	EINTERLACEMETHOD m_InterlaceMethod;
};

#endif //H_CVIDEOSETTINGS