#include <gui/setting_screen/SettingView.hpp>
#include <texts/TextKeysAndLanguages.hpp>

SettingView::SettingView()
{

}

void SettingView::setupScreen()
{
    SettingViewBase::setupScreen();
}

void SettingView::tearDownScreen()
{
    SettingViewBase::tearDownScreen();
}

void SettingView::changeLanguage()
{
	int idLanguage = (Texts::getLanguage()+1) % NUMBER_OF_LANGUAGES;

	Texts::setLanguage(idLanguage);
	//SettingView::invalidate();
	ConBtn_Set.invalidate();
	ChangeLanguageBtn.invalidate();
}
