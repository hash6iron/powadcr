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
    // PLAY
    hmi.writeString("click btnPlay,1");
}

void button2Action()
{

    // PAUSE
    hmi.writeString("click btnPause,1");    
}

void button3Action()
{

    // STOP
    hmi.writeString("click btnStop,1");    
}

void button4Action()
{
    // RWIND
    hmi.writeString("click btnRWD,1");
}

void button5Action()
{
    // FFWIND
    hmi.writeString("click btnFFWD,1");
}

void button6Action()
{
    // REC
    hmi.writeString("click btnRec,1");
}

void buttonsControl()
{
    button1 = digitalRead(button1_GPIO);
    button2 = digitalRead(button2_GPIO);
    button3 = digitalRead(button3_GPIO);
    button4 = digitalRead(button4_GPIO);
    button5 = digitalRead(button5_GPIO);
    button6 = digitalRead(button6_GPIO);

    // PLAY
    if (button1 == 0 && !button1_down)
    {
        //Presiono
        button1_down = true;
    }
    else if (button1 == 1 && button1_down)
    {     
        // Suelto 
        button1Action();
        button1_down = false;
    }

    //RWD
    if (button2 == 0 && !button2_down)
    {
        //Presiono
        button2_down = true;
    }
    else if (button2 == 1 && button2_down)
    {     
        // Suelto 
        button2Action();
        button2_down = false;
    }

    // FFWD
    if (button3 == 0 && !button3_down)
    {
        //Presiono
        button3_down = true;
    }
    else if (button3 == 1 && button3_down)
    {     
        // Suelto 
        button3Action();
        button3_down = false;
    }    

    // PAUSE
    if (button4 == 0 && !button4_down)
    {
        //Presiono
        button4_down = true;
    }
    else if (button4 == 1 && button4_down)
    {     
        // Suelto 
        button4Action();
        button4_down = false;
    }    

    // STOP
    if (button5 == 0 && !button5_down)
    {
        //Presiono
        button5_down = true;
    }
    else if (button5 == 1 && button5_down)
    {     
        // Suelto 
        button5Action();
        button5_down = false;
    } 

    // REC
    if (button6 == 0 && !button6_down)
    {
        //Presiono
        button6_down = true;
    }
    else if (button6 == 1 && button6_down)
    {     
        // Suelto 
        button6Action();
        button6_down = false;
    } 


}
