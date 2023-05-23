// Variables GLOBALES para todos los ficheros
// definición de pulsadores
int LOADING_STATE = 0;

bool PLAY = false;
bool PAUSE = true;
bool REC = false;
bool STOP = false;
bool FFWIND = false;
bool RWIND = false;
bool UP = false;
bool DOWN = false;
bool LEFT = false;
bool RIGHT = false;
bool ENTER = false;

// Gestion de menú
bool MENU = false;
bool menuOn = false;
int nMENU = 0;

// Variables locals
const int button1_GPIO = 36;
const int button2_GPIO = 13;
const int button3_GPIO = 19;
const int button4_GPIO = 23;
const int button5_GPIO = 18;
const int button6_GPIO = 5;

// Control de los botones
int button1 = 0;
int button2 = 0;
int button3 = 0;
int button4 = 0;
int button5 = 0;
int button6 = 0;

// Button state
bool button1_down = false;
bool button2_down = false;
bool button3_down = false;
bool button4_down = false;
bool button5_down = false;
bool button6_down = false;

bool button1_pressed = false;
bool button2_pressed = false;
bool button3_pressed = false;
bool button4_pressed = false;
bool button5_pressed = false;
bool button6_pressed = false;

void configureButtons()
{
    pinMode(button1_GPIO, INPUT);
    pinMode(button2_GPIO, INPUT);
    pinMode(button3_GPIO, INPUT);
    pinMode(button4_GPIO, INPUT);
    pinMode(button5_GPIO, INPUT);
    pinMode(button6_GPIO, INPUT);
}
void showMenu(int nMenu)
{}

void button1Action()
{
    // PLAY / PAUSE
    // DOWN
    Serial.print(" Button 1 pressed ");
    if (menuOn)
    {
        DOWN = true;
    }
    else
    {
        PLAY = true;
        PAUSE = false;
        STOP = false;
    }
}

void button2Action()
{
    // FFWIND
    // UP

    Serial.print(" Button 2 pressed ");
    if (menuOn)
    {
        UP = true;
    }
    else
    {
        FFWIND = true;
        PLAY = false;
        PAUSE = true;
    }
}

void button3Action()
{
    // RWIND
    // LEFT

    Serial.print(" Button 3 pressed ");
    if (menuOn)
    {
        LEFT = true;
    }
    else
    {
        RWIND = true;
        PLAY = false;
        PAUSE = true;
    }

}

void button4Action()
{
    //STOP
    //RIGHT

    Serial.print(" Button 4 pressed ");
    if (menuOn)
    {
        RIGHT = true;
    }
    else
    {
        STOP = true;
        PLAY = false;
        PAUSE = true;
    }
}

void button5Action()
{
    // REC
    // ENTER

    Serial.print(" Button 5 pressed ");
    if (menuOn)
    {
        ENTER = true;
    }
    else
    {
        REC = true;
        PLAY = false;
        PAUSE = true;
    }
}

void button6Action()
{
    // MENU

    Serial.print(" Button 6 pressed ");
    if (menuOn)
    {
        // Acción sobre el MENU
        MENU = true;
    }
    else
    {
        // Vamos al menu principal
        nMENU = 0;
    }
}

void buttonsControl()
{
    button1 = digitalRead(button1_GPIO);
    button2 = digitalRead(button2_GPIO);
    button3 = digitalRead(button3_GPIO);
    button4 = digitalRead(button4_GPIO);
    button5 = digitalRead(button5_GPIO);
    button6 = digitalRead(button6_GPIO);

    if (button1 == HIGH)
    {
        button1_down = true;
        button1_pressed = true;

        // Pulsacion prolongada
        delay(100);
    }
    else
    {
        if (button1_pressed)
        {
            // Pulsacion corta
            button1_pressed = false;
            button1Action();
        }
        //
        button1_down = false;
    }

    if (button2 == LOW)
    {
        button2Action();
    }

    if (button3 == LOW)
    {
        button3Action();
    }

    if (button4 == HIGH)
    {
        button4Action();
    }

    if (button5 == HIGH)
    {
        button5Action();
    }

    if (button6 == HIGH)
    {
        button6Action();
    }

}