#include <gui/containers/Keyboard.hpp>
#include <touchgfx/utils.hpp>

Keyboard::Keyboard():
characterPartTwoMode(false),numberMode(false),capitalLetterMode(false),positionText(-1)// initialization 
{
}

void Keyboard::initialize()
{
    KeyboardBase::initialize();
    buffer[0] = ' '; // initialization of the first character of the buffer string
}


/*********************************************************************************/
/**********  DISPLAY THE CHARACTER ON THE TEXT, WHEN THE USER CHOOSES  ***********/
/**********         IT BY CLICKING ON THE BUTTON OF HIS CHOICE.        ***********/
/*********************************************************************************/
/*  You can see several buttons on the Keyboard container. Each button is included 
    in different containers depending on its type. For example you can find the
    buttons numbers in the container named numberCustomContainer.
    letterCustomContainer -> Buttons for the letters
    buttonSpacebarCustomContainer -> Button for the spacebar
    capitalLetterCustomContainer ->  Buttons for the capital letters
    buttonSkipLineCustomContainer -> Button to skip a line
    specialCharPartOneCustomContainer -> First part of the special characters
    specialCharPartTwoCustomContainer -> Second part of the special characters

    Each button is associated with a particular character (e.g. oneFlexButton is associated 
    with the character '1'). To retrieve the associated character to each button we use 
    triggers : (they are all char types)
    numberCustomContainer -> is called valueNumber
    letterCustomContainer -> is called charValueLetter
    buttonSpacebarCustomContainer -> is called charValueSpace 
    capitalLetterCustomContainer ->  is called valueCapitalLetter
    buttonSkipLineCustomContainer -> is called charValueSkipLine
    specialCharPartOneCustomContainer -> is called valueSpecialCharPartOne
    specialCharPartTwoCustomContainer -> is called valueSpecialCharPartTwo
    After a click on a button, the value of a trigger is assigned to corresponding characters.
    This is done thanks to interactions in containers. There are as many interactions as buttons. 
    (e.g. oneFlexButton put value trigger to '1' in the writeCharOne() interaction 
    in the numberCustomContainer container)

    In the Keyboard container the writeButtonCharacter() action takes as entry the value returned 
    by the triggers. This is done by the following interactions: writeCharacter_SpecialCharPartTwo, 
    writeCharacter_SpecialCharPartOne, skipALine, writeCharacter_CapitalLetters, 
    addASpace, writeCharacter_Lettre and writeCharacter_Number. 

    Finally, the writeButtonCharacter() action is implemented to
    add the character to a variable buffer and then assign the buffer to the TextArea Text.
*/
void Keyboard::writeButtonCharacter(char value){
    // value-> corresponds to the character associated with the button that was clicked
    positionText++;
    if (positionText<=60){ // string length limit
        buffer[positionText] = value; //adds the character to the string char (buffer)
        Unicode::strncpy(keyboardTextAreaBuffer, buffer, KEYBOARDTEXTAREA_SIZE);// assign the string to the Text
        keyboardTextArea.resizeToCurrentText();
        keyboardTextArea.invalidate();
    } 
}


/*********************************************************************************/
/**********           DELETE THE LAST CHARACTER ON THE TEXT,           ***********/
/**********        WHEN THE USER HAS CLICKED ON DELETEFLEXBUTTON       ***********/
/*********************************************************************************/
/*  For the function deleteLastCharacter() we use:
    - The buttonDeleteCustomContainer Container
    - The valueDelete trigger on buttonDeleteCustomContainer 
    - The deleteCharacter interaction on buttonDeleteCustomContainer 
    (valueDelete is emited when the deleteFlexButton is cliked)
    - The deleteLastCharacter() action on the Keyboard
    - The deleteTheLastCharacter interaction on the Keyboard 
    (call deleteLastCharacter() when the valueDelete is emit)
*/
void Keyboard::deleteLastCharacter()
{
    // Delete the last character of the buffer: 
    if (positionText>0){
        buffer[positionText] = '\0';
        Unicode::strncpy(keyboardTextAreaBuffer, buffer, KEYBOARDTEXTAREA_SIZE);
        keyboardTextArea.invalidate(); 
        positionText--;  
    }else if (positionText==0){ //put a space if we delete all the characters of string 
        buffer[positionText] =' ';
        Unicode::strncpy(keyboardTextAreaBuffer, buffer, KEYBOARDTEXTAREA_SIZE);
        keyboardTextArea.invalidate(); 
        positionText=-1;
    }
}


/*********************************************************************************/
/**********               SWITCH THE LETTERS EN CAPITAL               ***********/
/*********************************************************************************/
/*  The displayCapitalLetters() action is used in the same way as deleteLastCharacter(). 
    It uses:
    - The buttonModeCapitalLetterCustomContainer Container
    - The valueCapitalLetterMode trigger on buttonModeCapitalLetterCustomContainer 
    - The putModeCapitalLetter interaction on buttonModeCapitalLetterCustomContainer 
    (valueCapitalLetterMode is emited when the capitalLetterToggleButton is cliked)
    - The displayCapitalLetters() action on the Keyboard
    - The displayButtons_CapitalLetters interaction on the Keyboard 
    (call displayCapitalLetters() when the valueCapitalLetterMode is emit)
*/
void Keyboard::displayCapitalLetters()
{ 
    capitalLetterMode= ! capitalLetterMode; //switch the mode
    if((capitalLetterMode == true)){    //make the containers visible or not, depending on what we want to display
        letterContainer.setVisible(false);
        capitalLetterContainer.setVisible(true);
    }else{
        letterContainer.setVisible(true);
        capitalLetterContainer.setVisible(false);
    }
        letterContainer.invalidate();
        capitalLetterContainer.invalidate();
}


/*********************************************************************************/
/**********        DISPLAY THE SCREEN IN THE NUMBERED MODE, I.E.       ***********/
/**********  THE NUMBERS AND THE FIRST PARS OF THE SPECIAL CHARACTERS  ***********/
/*********************************************************************************/
/*  The displayNumbers() action is used in the same way as deleteLastCharacter(). 
    It uses:
    - The buttonModeNumberCustomContainer Container
    - The valueModeNumber trigger on buttonModeNumberCustomContainer 
    - The putModeNumber interaction on buttonModeNumberCustomContainer 
    (valueModeNumber is emited when the modeNumberToggleButton is cliked)
    - The displayNumbers() action on the Keyboard
    - The displayButtons_Numbers interaction on the Keyboard 
    (call displayNumbers() when the valueModeNumber is emit)
*/
void Keyboard::displayNumbers()
{
    numberMode=! numberMode; //switch the mode
    if((numberMode== true)){    
        //make the containers visible or not, depending on what we want to display
        numberModeContainer.setVisible(true);
        buttonModeCharPartTwoContainer.setVisible(true);
        capitalLetterContainer.setVisible(false);
        letterContainer.setVisible(false);
        buttonModeCapitalLetterContainer.setVisible(false);
    }else{
        numberModeContainer.setVisible(false);
        specialCharPartTwoContainer.setVisible(false);
        buttonModeCharPartTwoContainer.setVisible(false);
        capitalLetterContainer.setVisible(false);
        if((capitalLetterMode ==true)){  //check if the buttonModeCapitalLetterCustomContainer is still selected  
            letterContainer.setVisible(false);
            capitalLetterContainer.setVisible(true);
        }else{
            letterContainer.setVisible(true);
            capitalLetterContainer.setVisible(false);
        }
        buttonModeCapitalLetterContainer.setVisible(true);
    }
    numberModeContainer.invalidate();
    buttonModeCharPartTwoContainer.invalidate();
    capitalLetterContainer.invalidate();
    letterContainer.invalidate();
    buttonModeCapitalLetterContainer.invalidate();
    specialCharPartTwoContainer.invalidate();
}


/*********************************************************************************/
/**********      DISPLAY THE SECOND PART OF THE SPECIAL CHARACTERS     ***********/
/*********************************************************************************/
/*  The displayCharactersPartTwo() action is used in the same way as deleteLastCharacter(). 
    It uses:
    - The buttonModeCharPartTwoCustomContainer Container
    - The valueModeCharPartTwo trigger on buttonModeCharPartTwoCustomContainer 
    - The putModeCharPartTwo interaction on buttonModeCharPartTwoCustomContainer 
    (valueModeCharPartTwo is emited when the characterPartTwoToggleButton is cliked)
    - The displayCharactersPartTwo() action on the Keyboard
    - The displayButtons_SpecialCharPartTwo interaction on the Keyboard 
    (call displayCharactersPartTwo() when the valueModeCharPartTwo is emit)
*/
void Keyboard::displayCharactersPartTwo()
{
    characterPartTwoMode=!characterPartTwoMode; //switch the mode
    if(characterPartTwoMode==true){    
        //make the containers visible or not, depending on what we want to display
        specialCharPartTwoContainer.setVisible(true);
        numberModeContainer.setVisible(false);
    }else{
        specialCharPartTwoContainer.setVisible(false);
        numberModeContainer.setVisible(true);
    }
    specialCharPartTwoContainer.invalidate();
    numberModeContainer.invalidate();
}

/*********************************************************************************/
/**********     RETURN THE TEXT ENTERED BY THE USER ON THE KEYBOARD    ***********/
/*********************************************************************************/
char* Keyboard::getText(){
    return buffer;
}