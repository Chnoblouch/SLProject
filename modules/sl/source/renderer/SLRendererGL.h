#ifndef SLPROJECT_SLRENDERERGL_H
#define SLPROJECT_SLRENDERERGL_H

#include <SLRenderer.h>
#include <SLNode.h>

//-----------------------------------------------------------------------------
class SLSceneView;
//-----------------------------------------------------------------------------
class SLRendererGL : public SLRenderer
{
public:
    SLRendererGL(SLSceneView* sv);
    void     initialize() final;
    bool     render() final;
    SLstring windowTitle() final;

private:
    void draw3DGLAll();
    void draw3DGLNodes(SLVNode& nodes, SLbool alphaBlended, SLbool depthSorted);
    void draw3DGLLines(SLVNode& nodes);
    void draw3DGLLinesOverlay(SLVNode& nodes);

private:
    SLSceneView* _sv;
};
//-----------------------------------------------------------------------------
#endif // SLPROJECT_SLRENDERERGL_H
