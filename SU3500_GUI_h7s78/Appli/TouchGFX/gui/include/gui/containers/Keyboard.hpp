#ifndef KEYBOARD_HPP
#define KEYBOARD_HPP

#include <gui_generated/containers/KeyboardBase.hpp>

class Keyboard : public KeyboardBase
{
public:
    Keyboard();
    virtual ~Keyboard() {}

    virtual void initialize();

    /******  INITIALIZATION OF FUNCTIONS *******/
    virtual void writeButtonCharacter(char value);
    virtual void deleteLastCharacter();
    virtual void displayCapitalLetters();
    virtual void displayNumbers();
    virtual void displayCharactersPartTwo();
    char* getText();
protected:
    char buffer[60] = {}; // variable that will be displayed in the TextArea -> Text (contains the characters entered by the user)
    bool characterPartTwoMode; // true or false, depending of the position of the characterPartTwoToggleButton
    bool numberMode; // true or false, depending of the position of the modelNumberToggleButton
    bool capitalLetterMode; // true or false, depending of the position of the capitalLetterToggleButton
    int positionText; /* position of the character (entered by the user on the keyboard) in the variable buffer. 
    The value is between 0 and 60. 
    (location is initialized to -1, because the writing starts at position 0 of the buffer)*/
    /*******************************************/

};

#endif // KEYBOARD_HPP
