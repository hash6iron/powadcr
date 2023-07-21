/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    Nombre: interface.h
    
    Creado por:
      Copyright (c) Antonio Tamairón. 2023  / https://github.com/hash6iron/powadcr
      @hash6iron / https://powagames.itch.io/
    
    Descripción:
    Conjunto de metodos para la gestión de la botonera. 
    
    NOTA: Posiblemente a extinguir.
   
    Version: 1.0

    Historico de versiones


    Derechos de autor y distribución
    --------------------------------
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
    
    To Contact the dev team you can write to hash6iron@gmail.com
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

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
        PLAY = false;
        PAUSE = true;
        STOP = false;
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
        PLAY = false;
        PAUSE = false;
        STOP = true;
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
