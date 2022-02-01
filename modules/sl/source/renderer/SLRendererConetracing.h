#ifndef SLPROJECT_SLRENDERERCONETRACING_H
#define SLPROJECT_SLRENDERERCONETRACING_H

#include <SLRenderer.h>
#include <SLGLConetracer.h>

//-----------------------------------------------------------------------------
class SLSceneView;
//-----------------------------------------------------------------------------
class SLRendererConetracing : public SLRenderer
{
public:
    SLRendererConetracing(SLSceneView* sv, SLstring shaderDir);
    void     initialize() final;
    bool     render() final;
    SLstring windowTitle() final;

    SLGLConetracer* conetracer() { return &_conetracer; }

private:
    SLSceneView*   _sv;
    SLGLConetracer _conetracer;
};
//-----------------------------------------------------------------------------
#endif // SLPROJECT_SLRENDERERCONETRACING_H
