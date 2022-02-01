#include <SLRendererRaytracing.h>
#include <SLSceneView.h>

//-----------------------------------------------------------------------------
SLRendererRaytracing::SLRendererRaytracing(SLSceneView* sv)
  : _sv(sv)
{
}
//-----------------------------------------------------------------------------
void SLRendererRaytracing::initialize()
{
    _raytracer.maxDepth(5);
}
//-----------------------------------------------------------------------------
/*!
SLSceneView::updateAndRT3D starts the raytracing or refreshes the current RT
image during rendering. The function returns true if an animation was done
prior to the rendering start.
*/
bool SLRendererRaytracing::render()
{
    SLbool updated = false;

    // if the raytracer not yet got started
    if (_raytracer.state() == rtReady)
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
        if (_raytracer.doDistributed())
            _raytracer.renderDistrib(_sv);
        else
            _raytracer.renderClassic(_sv);
    }

    // Refresh the render image during RT
    _raytracer.renderImage(true);

    // React on the stop flag (e.g. ESC)
    //    if (_stopRT)
    //    {
    //        _renderType = RT_gl;
    //        updated     = true;
    //    }

    return updated;
}
//-----------------------------------------------------------------------------
SLstring SLRendererRaytracing::windowTitle()
{
    char title[255];

    if (_raytracer.doContinuous())
    {
        sprintf(title,
                "Ray Tracing: %s (fps: %4.1f, Threads: %d)",
                _sv->s()->name().c_str(),
                _sv->s()->fps(),
                _raytracer.numThreads());
    }
    else
    {
        sprintf(title,
                "Ray Tracing: %s (Threads: %d)",
                _sv->s()->name().c_str(),
                _raytracer.numThreads());
    }

    return title;
}
//-----------------------------------------------------------------------------