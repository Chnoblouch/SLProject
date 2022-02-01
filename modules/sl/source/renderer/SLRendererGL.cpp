#include <SLRendererGL.h>

#include <SLSceneView.h>
#include <Profiler.h>
#include <GlobalTimer.h>
#include <SLSkybox.h>
#include <SLGLOculusFB.h>

//-----------------------------------------------------------------------------
SLRendererGL::SLRendererGL(SLSceneView* sv)
  : _sv(sv)
{
}
//-----------------------------------------------------------------------------
void SLRendererGL::initialize()
{
}
//-----------------------------------------------------------------------------
//! Draws the 3D scene with OpenGL
/*! This is the main routine for updating and drawing the 3D scene for one frame.
The following steps are processed:
<ol>
<li>
<b>Render shadow maps</b>:
Renders all shadow maps for lights in SLLight::renderShadowMap
</li>
<li>
<b>Updates the camera</b>:
If the active camera has an animation it gets updated first in SLCamera::camUpdate
</li>
<li>
<b>Clear all buffers</b>:
The color and depth buffer are cleared in this step. If the projection is
the Oculus stereo projection also the framebuffer target is bound.
</li>
<li>
<b>Set viewport</b>:
Depending on the projection we set the camera projection and the view
for the center or left eye.
</li>
<li>
<b>Render background</b>:
 If no skybox is used the background is rendered. This can be the camera image
 if the camera is turned on.
</li>
<li>
<b>Set projection and view</b>:
 Sets the camera projection matrix
</li>
<li>
<b>Frustum culling</b>:
During the cull traversal all materials that are seen in the view frustum get
collected in _visibleMaterials. All nodes with their meshes get collected in
SLMaterial::_nodesVisible3D. These materials and nodes get drawn in draw3DGLAll.
</li>
<li>
<b>Draw skybox</b>:
The skybox is draw as first object with frozen depth buffer.
The skybox is always around the active camera.
</li>
<li>
<b>Draw all visible nodes</b>:
 By calling the SLSceneView::draw3DGL all visible nodes of all visible materials
 get drawn sorted by material and transparency. If a stereo projection is set,
 the scene gets drawn a second time for the right eye.
</li>
<li>
<b>Draw right eye for stereo projections</b>
</li>
</ol>
*/
bool SLRendererGL::render()
{
    PROFILE_FUNCTION();

    SLGLState* stateGL       = SLGLState::instance();
    SLScene*   s             = _sv->s();
    SLCamera*  camera        = _sv->camera();
    float      elapsedTimeMS = s->elapsedTimeMS();

    _sv->preDraw();

    ///////////////////////////
    // 1. Render shadow maps //
    ///////////////////////////

    SLfloat startMS = GlobalTimer::timeMS();

    // Render shadow map for each light which creates shadows
    for (SLLight* light : s->lights())
    {
        if (light->createsShadows())
            light->renderShadowMap(_sv, s->root3D());
    }

    _sv->_shadowMapTimeMS = GlobalTimer::timeMS() - startMS;

    /////////////////////////
    // 2. Do camera update //
    /////////////////////////

    startMS = GlobalTimer::timeMS();

    // Update camera animation separately (smooth transition on key movement)
    // todo: ghm1: this is currently only necessary for walking animation (which is somehow always enabled)
    // A problem is also, that it only updates the current camera. This is maybe not what we want for sensor rotated camera.
    SLbool camUpdated = camera->camUpdate(_sv, elapsedTimeMS);

    //////////////////////
    // 3. Clear buffers //
    //////////////////////

    // Render into framebuffer if Oculus stereo projection is used
    if (camera->projection() == P_stereoSideBySideD)
    {
        s->oculus()->beginFrame();
        _sv->oculusFB()->bindFramebuffer((SLint)(s->oculus()->resolutionScale() * (SLfloat)_sv->scrW()),
                                         (SLint)(s->oculus()->resolutionScale() * (SLfloat)_sv->scrH()));
    }

    // Clear color buffer
    stateGL->clearColor(SLVec4f(0.00001f, 0.00001f, 0.00001f, 1.0f));
    stateGL->clearColorDepthBuffer();

    /////////////////////
    // 4. Set viewport //
    /////////////////////

    if (camera->projection() > P_monoOrthographic)
        camera->setViewport(_sv, ET_left);
    else
        camera->setViewport(_sv, ET_center);

    //////////////////////////
    // 5. Render background //
    //////////////////////////

    // Render solid color, gradient or textured background from active camera
    if (!s->skybox())
        camera->background().render(_sv->viewportRect().width, _sv->viewportRect().height);

    // Change state (only when changed)
    stateGL->multiSample(_sv->doMultiSampling());
    stateGL->depthTest(_sv->doDepthTest());

    //////////////////////////////
    // 6. Set projection & View //
    //////////////////////////////

    // Set projection
    if (camera->projection() > P_monoOrthographic)
    {
        camera->setProjection(_sv, ET_left);
        camera->setView(_sv, ET_left);
    }
    else
    {
        camera->setProjection(_sv, ET_center);
        // todo: ghm1: set view is only called on the active camera. Then the camera animation is not updated
        // of a camera the is not the current camera!
        camera->setView(_sv, ET_center);
    }

    ////////////////////////
    // 7. Frustum culling //
    ////////////////////////

    // Delete all visible nodes from the last frame
    for (auto* material : _sv->visibleMaterials3D())
        material->nodesVisible3D().clear();

    _sv->visibleMaterials3D().clear();
    _sv->nodesOpaque3D().clear();
    _sv->nodesBlended3D().clear();
    _sv->nodesOverdrawn().clear();
    _sv->stats3D().numNodesOpaque  = 0;
    _sv->stats3D().numNodesBlended = 0;
    _sv->camera()->setFrustumPlanes();

    if (s->root3D())
        s->root3D()->cull3DRec(_sv);

    _sv->_cullTimeMS = GlobalTimer::timeMS() - startMS;

    ////////////////////
    // 8. Draw skybox //
    ////////////////////

    if (s->skybox())
        s->skybox()->drawAroundCamera(_sv);

    ////////////////////////////
    // 9. Draw all visible nodes
    ////////////////////////////

    startMS = GlobalTimer::timeMS();

    draw3DGLAll();

    ///////////////////////////////////////////////
    // 10. Draw right eye for stereo projections //
    ///////////////////////////////////////////////

    if (camera->projection() > P_monoOrthographic)
    {
        camera->setViewport(_sv, ET_right);

        // Only draw backgrounds for stereo projections in different viewports
        if (!s->skybox() && camera->projection() < P_stereoLineByLine)
            camera->background().render(_sv->viewportRect().width, _sv->viewportRect().height);

        camera->setProjection(_sv, ET_right);
        camera->setView(_sv, ET_right);
        stateGL->depthTest(true);
        if (s->skybox())
            s->skybox()->drawAroundCamera(_sv);
        draw3DGLAll();
    }

    // Enable all color channels again
    stateGL->colorMask(1, 1, 1, 1);

    _sv->_draw3DTimeMS = GlobalTimer::timeMS() - startMS;

    _sv->postDraw();

    GET_GL_ERROR; // Check if any OGL errors occurred
    return camUpdated;
}
//-----------------------------------------------------------------------------
/*!
 SLSceneView::draw3DGLAll renders by material sorted to avoid expensive material
 switches on the GPU. During the cull traversal all materials that are seen in
 the view frustum get collected in _visibleMaterials. All nodes with their
 meshes get collected in SLMaterial::_nodesVisible3D. <br>
The 3D rendering has then the following steps:
1) Draw nodes with meshes with opaque materials and all helper lines sorted by material<br>
2) Draw remaining opaque nodes (SLCameras, needs redesign)<br>
3) Draw nodes with meshes with blended materials sorted by material and sorted back to front<br>
4) Draw remaining blended nodes (SLText, needs redesign)<br>
5) Draw helpers in overlay mode (not depth buffered)<br>
6) Draw visualization lines of animation curves<br>
*/
void SLRendererGL::draw3DGLAll()
{
    PROFILE_FUNCTION();

    // a) Draw nodes with meshes with opaque materials and all helper lines sorted by material
    for (auto material : _sv->visibleMaterials3D())
    {
        if (!material->hasAlpha())
        {
            draw3DGLNodes(material->nodesVisible3D(), false, false);
            _sv->stats3D().numNodesOpaque += (SLuint)material->nodesVisible3D().size();
        }
        draw3DGLLines(material->nodesVisible3D());
    }

    // b) Draw remaining opaque nodes without meshes (SLCameras, needs redesign)
    _sv->stats3D().numNodesOpaque += (SLuint)_sv->nodesOpaque3D().size();
    draw3DGLNodes(_sv->nodesOpaque3D(), false, false);

    // c) Draw nodes with meshes with blended materials sorted by material and sorted back to front
    for (auto material : _sv->visibleMaterials3D())
    {
        if (material->hasAlpha())
        {
            draw3DGLNodes(material->nodesVisible3D(), true, _sv->doAlphaSorting());
            _sv->stats3D().numNodesBlended += (SLuint)material->nodesVisible3D().size();
        }
    }

    // d) Draw remaining blended nodes (SLText, needs redesign)
    _sv->stats3D().numNodesBlended += (SLuint)_sv->nodesBlended3D().size();
    draw3DGLNodes(_sv->nodesBlended3D(), true, _sv->doAlphaSorting());

    // e) Draw helpers in overlay mode (not depth buffered)
    for (auto material : _sv->visibleMaterials3D())
        draw3DGLLinesOverlay(material->nodesVisible3D());
    draw3DGLLinesOverlay(_sv->nodesOverdrawn());

    // f) Draw visualization lines of animation curves
    _sv->s()->animManager().drawVisuals(_sv);

    // Turn blending off again for correct anaglyph stereo modes
    SLGLState* stateGL = SLGLState::instance();
    stateGL->blend(false);
    stateGL->depthMask(true);
    stateGL->depthTest(true);
}
//-----------------------------------------------------------------------------
/*!
SLSceneView::draw3DGLNodes draws the nodes meshes from the passed node vector
directly with their world transform after the view transform.
*/
void SLRendererGL::draw3DGLNodes(SLVNode& nodes, SLbool alphaBlended, SLbool depthSorted)
{
    // PROFILE_FUNCTION();

    if (nodes.empty()) return;

    // For blended nodes we activate OpenGL blending and stop depth buffer updates
    SLGLState* stateGL = SLGLState::instance();
    stateGL->blend(alphaBlended);
    stateGL->depthMask(!alphaBlended);

    // Important and expensive step for blended nodes with alpha meshes
    // Depth sort with lambda function by their view distance
    if (depthSorted)
    {
        std::sort(nodes.begin(), nodes.end(), [](SLNode* a, SLNode* b)
                  {
                      if (!a) return false;
                      if (!b) return true;
                      return a->aabb()->sqrViewDist() > b->aabb()->sqrViewDist(); });
    }

    // draw the shapes directly with their wm transform
    for (auto* node : nodes)
    {
        // Set the view transform
        stateGL->modelViewMatrix.setMatrix(stateGL->viewMatrix);

        // Apply world transform
        stateGL->modelViewMatrix.multiply(node->updateAndGetWM().m());

        // Finally draw the nodes mesh
        node->drawMesh(_sv);
    }

    GET_GL_ERROR; // Check if any OGL errors occurred
}
//-----------------------------------------------------------------------------
/*!
SLSceneView::draw3DGLLines draws the AABB from the passed node vector directly
with their world coordinates after the view transform. The lines must be drawn
without blending.
Colors:
Red   : AABB of nodes with meshes
Pink  : AABB of nodes without meshes (only child nodes)
Yellow: AABB of selected node
*/
void SLRendererGL::draw3DGLLines(SLVNode& nodes)
{
    PROFILE_FUNCTION();

    if (nodes.empty()) return;

    SLGLState* stateGL = SLGLState::instance();
    stateGL->blend(false);
    stateGL->depthMask(true);

    // Set the view transform for drawing in world space
    stateGL->modelViewMatrix.setMatrix(stateGL->viewMatrix);

    // draw the opaque shapes directly w. their wm transform
    for (auto* node : nodes)
    {
        if (node != _sv->camera())
        {
            // Draw first AABB of the shapes but not the camera
            if ((_sv->drawBit(SL_DB_BBOX) || node->drawBit(SL_DB_BBOX)) &&
                !node->isSelected())
            {
                if (node->mesh())
                    node->aabb()->drawWS(SLCol4f::RED);
                else
                    node->aabb()->drawWS(SLCol4f::MAGENTA);
            }

            // Draw AABB for selected shapes
            if (node->isSelected())
            {
                node->aabb()->drawWS(SLCol4f::YELLOW);
            }
        }
    }

    GET_GL_ERROR; // Check if any OGL errors occurred
}
//-----------------------------------------------------------------------------
/*!
SLSceneView::draw3DGLLinesOverlay draws the nodes axis and skeleton joints
as overlayed
*/
void SLRendererGL::draw3DGLLinesOverlay(SLVNode& nodes)
{
    PROFILE_FUNCTION();

    SLGLState* stateGL = SLGLState::instance();

    // draw the opaque shapes directly w. their wm transform
    for (auto* node : nodes)
    {
        if (node != _sv->camera())
        {
            if (_sv->drawBit(SL_DB_AXIS) || node->drawBit(SL_DB_AXIS) ||
                _sv->drawBit(SL_DB_SKELETON) || node->drawBit(SL_DB_SKELETON) ||
                node->isSelected())
            {
                // Set the view transform
                SLGLState* stateGL = SLGLState::instance();
                stateGL->modelViewMatrix.setMatrix(stateGL->viewMatrix);
                stateGL->blend(false);     // Turn off blending for overlay
                stateGL->depthMask(true);  // Freeze depth buffer for blending
                stateGL->depthTest(false); // Turn of depth test for overlay

                // Draw axis
                if (_sv->drawBit(SL_DB_AXIS) ||
                    node->drawBit(SL_DB_AXIS) ||
                    node->isSelected())
                {
                    node->aabb()->drawAxisWS();
                }

                // Draw skeleton
                if (_sv->drawBit(SL_DB_SKELETON) ||
                    node->drawBit(SL_DB_SKELETON))
                {
                    // Draw axis of the skeleton joints and its parent bones
                    const SLAnimSkeleton* skeleton = node->skeleton();
                    if (skeleton)
                    {
                        for (auto joint : skeleton->joints())
                        {
                            // Get the node wm & apply the joints wm
                            SLMat4f wm = node->updateAndGetWM();
                            wm *= joint->updateAndGetWM();

                            // Get parent node wm & apply the parent joint wm
                            SLMat4f parentWM;
                            if (joint->parent())
                            {
                                parentWM = node->parent()->updateAndGetWM();
                                parentWM *= joint->parent()->updateAndGetWM();
                                joint->aabb()->updateBoneWS(parentWM, false, wm);
                            }
                            else
                                joint->aabb()->updateBoneWS(parentWM, true, wm);

                            joint->aabb()->drawBoneWS();
                        }
                    }
                }
            }
            else if (_sv->drawBit(SL_DB_BRECT) || node->drawBit(SL_DB_BRECT))
            {
                node->aabb()->calculateRectSS();

                SLMat4f prevProjMat = stateGL->projectionMatrix;
                stateGL->pushModelViewMatrix();
                SLfloat w2 = (SLfloat)_sv->scrWdiv2();
                SLfloat h2 = (SLfloat)_sv->scrHdiv2();
                stateGL->projectionMatrix.ortho(-w2, w2, -h2, h2, 1.0f, -1.0f);
                stateGL->viewport(0, 0, _sv->scrW(), _sv->scrH());
                stateGL->modelViewMatrix.identity();
                stateGL->modelViewMatrix.translate(-w2, h2, 1.0f);
                stateGL->depthMask(false); // Freeze depth buffer for blending
                stateGL->depthTest(false); // Disable depth testing

                node->aabb()->rectSS().drawGL(SLCol4f::GREEN);

                stateGL->depthMask(true); // Freeze depth buffer for blending
                stateGL->depthTest(true); // Disable depth testing
                stateGL->popModelViewMatrix();
                stateGL->projectionMatrix = prevProjMat;
            }
            else if (node->drawBit(SL_DB_OVERDRAW))
            {
                if (node->mesh() && node->mesh()->mat())
                {
                    SLMesh* mesh     = node->mesh();
                    bool    hasAlpha = mesh->mat()->hasAlpha();

                    // For blended nodes we activate OpenGL blending and stop depth buffer updates
                    SLGLState* stateGL = SLGLState::instance();
                    stateGL->blend(hasAlpha);
                    stateGL->depthMask(!hasAlpha);
                    stateGL->depthTest(false); // Turn of depth test for overlay

                    // Set the view transform
                    stateGL->modelViewMatrix.setMatrix(stateGL->viewMatrix);

                    // Apply world transform
                    stateGL->modelViewMatrix.multiply(node->updateAndGetWM().m());

                    // Finally draw the nodes mesh
                    node->drawMesh(_sv);
                    GET_GL_ERROR; // Check if any OGL errors occurred
                }
            }
        }
    }

    GET_GL_ERROR; // Check if any OGL errors occurred
}
//-----------------------------------------------------------------------------
SLstring SLRendererGL::windowTitle()
{
    char title[255];

    string format;
    if (_sv->s()->fps() > 5)
        format = "OpenGL Renderer: %s (fps: %4.0f, %u nodes of %u rendered)";
    else
        format = "OpenGL Renderer: %s (fps: %4.1f, %u nodes of %u rendered)";

    sprintf(title,
            format.c_str(),
            _sv->s()->name().c_str(),
            _sv->s()->fps(),
            _sv->stats3D().numNodesOpaque + _sv->stats3D().numNodesBlended,
            _sv->stats3D().numNodes);

    return title;
}
//-----------------------------------------------------------------------------