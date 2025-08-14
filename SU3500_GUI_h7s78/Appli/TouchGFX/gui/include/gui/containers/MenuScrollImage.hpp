#ifndef MENUSCROLLIMAGE_HPP
#define MENUSCROLLIMAGE_HPP

#include <gui_generated/containers/MenuScrollImageBase.hpp>
#include <touchgfx/Bitmap.hpp>

class MenuScrollImage : public MenuScrollImageBase
{
public:
    MenuScrollImage();
    virtual ~MenuScrollImage() {}

    virtual void initialize();

    void updateImage(touchgfx::Bitmap image);

protected:
};

#endif // MENUSCROLLIMAGE_HPP
