#ifndef SLPROJECT_SLRENDERERPATHTRACING_H
#define SLPROJECT_SLRENDERERPATHTRACING_H

#include <SLRenderer.h>
#include <SLPathtracer.h>

//-----------------------------------------------------------------------------
class SLSceneView;
//-----------------------------------------------------------------------------
class SLRendererPathtracing : public SLRenderer
{
public:
    SLRendererPathtracing(SLSceneView* sv);
    void     initialize() final;
    bool     render() final;
    SLstring windowTitle() final;

    SLPathtracer* pathtracer() { return &_pathtracer; }

private:
    SLSceneView* _sv;
    SLPathtracer _pathtracer;
};
//-----------------------------------------------------------------------------
#endif // SLPROJECT_SLRENDERERPATHTRACING_H
