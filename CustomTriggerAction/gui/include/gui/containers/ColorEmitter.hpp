#ifndef COLOREMITTER_HPP
#define COLOREMITTER_HPP

#include <gui_generated/containers/ColorEmitterBase.hpp>

class ColorEmitter : public ColorEmitterBase
{
public:
    ColorEmitter();
    virtual ~ColorEmitter() {}

    virtual void initialize();
protected:
};

#endif // COLOREMITTER_HPP
