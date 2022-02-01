#ifndef SLPROJECT_SLRENDERER_H
#define SLPROJECT_SLRENDERER_H

#include <SL.h>

//-----------------------------------------------------------------------------
class SLRenderer
{
public:
    virtual ~SLRenderer()          = default;
    virtual void     initialize()  = 0;
    virtual bool     render()      = 0;
    virtual SLstring windowTitle() = 0;
};
//-----------------------------------------------------------------------------
#endif // SLPROJECT_SLRENDERER_H
