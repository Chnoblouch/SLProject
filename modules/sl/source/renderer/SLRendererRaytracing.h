#ifndef SLPROJECT_SLRENDERERRAYTRACING_H
#define SLPROJECT_SLRENDERERRAYTRACING_H

#include <SLRenderer.h>
#include <SLRaytracer.h>

//-----------------------------------------------------------------------------
class SLSceneView;
//-----------------------------------------------------------------------------
class SLRendererRaytracing : public SLRenderer
{
public:
    SLRendererRaytracing(SLSceneView* sv);
    void initialize() final;
    bool render() final;
    SLstring windowTitle() final;

    SLRaytracer* raytracer() { return &_raytracer; }

private:
    SLSceneView* _sv;
    SLRaytracer  _raytracer;
};
//-----------------------------------------------------------------------------
#endif // SLPROJECT_SLRENDERERRAYTRACING_H
