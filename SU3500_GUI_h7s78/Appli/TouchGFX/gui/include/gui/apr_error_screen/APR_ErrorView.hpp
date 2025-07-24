#ifndef APR_ERRORVIEW_HPP
#define APR_ERRORVIEW_HPP

#include <gui_generated/apr_error_screen/APR_ErrorViewBase.hpp>
#include <gui/apr_error_screen/APR_ErrorPresenter.hpp>

class APR_ErrorView : public APR_ErrorViewBase
{
public:
    APR_ErrorView();
    virtual ~APR_ErrorView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
protected:
};

#endif // APR_ERRORVIEW_HPP
