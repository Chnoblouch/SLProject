#include <SLRendererConetracing.h>
#include <SLSceneView.h>
#include <GlobalTimer.h>

//-----------------------------------------------------------------------------
SLRendererConetracing::SLRendererConetracing(SLSceneView* sv, SLstring shaderDir)
  : _sv(sv),
    _conetracer(shaderDir)
{
}
//-----------------------------------------------------------------------------
void SLRendererConetracing::initialize()
{
}
//-----------------------------------------------------------------------------
/*!
SLSceneView::draw3DCT draws all 3D content with voxel cone tracing.
*/
bool SLRendererConetracing::render()
{
    // SL_LOG("Rendering VXC ");
    SLfloat startMS = GlobalTimer::timeMS();

    SLbool rendered = _conetracer.render(_sv);

    _sv->_draw3DTimeMS = GlobalTimer::timeMS() - startMS;

    return true;
}
//-----------------------------------------------------------------------------
SLstring SLRendererConetracing::windowTitle()
{
    char title[255];
    sprintf(title,
            "Path Tracing: %s",
            _sv->name().c_str());
    return title;
}
//-----------------------------------------------------------------------------