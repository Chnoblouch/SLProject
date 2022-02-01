#include <SLRendererPathtracing.h>

#include <SLSceneView.h>

//-----------------------------------------------------------------------------
SLRendererPathtracing::SLRendererPathtracing(SLSceneView* sv)
  : _sv(sv)
{
}
//-----------------------------------------------------------------------------
void SLRendererPathtracing::initialize()
{
    _pathtracer.maxDepth(5);
    _pathtracer.aaSamples(10);
}
//-----------------------------------------------------------------------------
/*!
SLSceneView::updateAndRT3D starts the raytracing or refreshes the current RT
image during rendering. The function returns true if an animation was done
prior to the rendering start.
*/
bool SLRendererPathtracing::render()
{
    SLbool updated = false;

    // if the pathtracer not yet got started
    if (_pathtracer.state() == rtReady)
    {
        if (_sv->s()->root3D())
        {
            // Update transforms and AABBs
            // @Todo: causes multithreading bug in RT
            // s->root3D()->needUpdate();

            // Do software skinning on all changed skeletons
            _sv->s()->root3D()->updateMeshAccelStructs();
        }

        // Start raytracing
        _pathtracer.render(_sv);
    }

    // Refresh the render image during PT
    _pathtracer.renderImage(_sv);

    // React on the stop flag (e.g. ESC)
    //    if (_stopPT)
    //    {
    //        _renderType = RT_gl;
    //        updated     = true;
    //    }

    return updated;
}
//-----------------------------------------------------------------------------
SLstring SLRendererPathtracing::windowTitle()
{
    char title[255];
    sprintf(title,
            "Path Tracing: %s (Threads: %d)",
            _sv->name().c_str(),
            _pathtracer.numThreads());
    return title;
}
//-----------------------------------------------------------------------------