#include <gui/containers/MenuScrollImage.hpp>
#include <touchgfx\Bitmap.hpp>

MenuScrollImage::MenuScrollImage()
{

}

void MenuScrollImage::initialize()
{
    MenuScrollImageBase::initialize();
}

void MenuScrollImage::updateImage(touchgfx::Bitmap image)
{

    image1.setBitmap(image);
    image1.invalidate();
}