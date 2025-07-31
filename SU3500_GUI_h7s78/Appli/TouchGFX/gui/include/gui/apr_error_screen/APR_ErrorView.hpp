#ifndef APR_ERRORVIEW_HPP
#define APR_ERRORVIEW_HPP

#include <gui_generated/apr_error_screen/Apr_ErrorViewBase.hpp>
#include <gui/apr_error_screen/Apr_ErrorPresenter.hpp>

class Apr_ErrorView : public Apr_ErrorViewBase
{
public:
    Apr_ErrorView();
    virtual ~Apr_ErrorView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
protected:
};

#endif // APR_ERRORVIEW_HPP
