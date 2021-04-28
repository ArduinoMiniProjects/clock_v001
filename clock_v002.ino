/*

  The circuit:
 * LCD RS pin to digital pin 10
 * LCD Enable pin to digital pin 9
 * LCD D4 pin to digital pin 5
 * LCD D5 pin to digital pin 4
 * LCD D6 pin to digital pin 3
 * LCD D7 pin to digital pin 2
 * LCD R/W pin to ground
 * LCD VSS pin to ground
 * LCD VCC pin to 5V
 * 10K resistor:
 * ends to +5V and ground
 * wiper to LCD VO pin (pin 3)
*/

#include <LiquidCrystal.h>

// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
const int rs = 10, en = 9, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

char char_arrow = 0b01111111; // symbol of an arrow
char char_accept = 0b11111111; // symbol of accept

int b_up = 12; // button up, Pin 12, PB4
int b_enter = 11; // button enter, Pin 11, PB3 
int b_down = 8; // button down, Pin 8, PB0

int buzzer_pin = 6; // output for buzzer (200R resistor is used)
int pwm_val = 0; // pwm value for buzzer

int menu = 1;

int hours = 23;
int minutes = 59;
int seconds = 45;

int alarm_hours = 0;
int alarm_minutes = 0; 

bool alarm = false;

void timer_ini(void); // initialize registers for timers
void increase_time(void);
void print_time(void);
void check_alarm(void);
void print_alarm_time(void);
void print_menu(void);

void setup() {
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  // Print a message to the LCD.
  lcd.setCursor(10,0);
  lcd.print("Time");
  lcd.setCursor(9, 1);
  lcd.print("Alarm");

  print_alarm_time();

  lcd.setCursor(15, 0);
  lcd.print(char_arrow);

  print_menu();

  cli(); // interrupts forbidden

  pinMode(b_up, INPUT); // Pin 12, PB4
  pinMode(b_enter, INPUT); // Pin 11, PB3
  pinMode(b_down, INPUT); // Pin 8, PB0

  pinMode(buzzer_pin, OUTPUT); // pin 6

  // pull up resistors for PB4, PB3, PB0 inputs
  PORTB |= (1 << PORTB4) | (1 << PORTB3) | (1 << PORTB0);

  // inputs as Pin Change interrupts
  PCMSK0 |= (1 << PCINT4) | (1 << PCINT3) | (1 << PCINT0);
  
  // Pin Change interrupts enabled PCINT4, PCINT3, PCINT0
  PCICR |= (1 << PCIE0);
  
  timer_ini();
  sei(); // interrupts are enabled
}

void loop() {
  // set the cursor to column 0, line 1
  // (note: line 1 is the second row, since counting begins with 0):
  //lcd.setCursor(0, 1);
  
  //lcd.print(seconds);
  // print the number of seconds since reset:
  //lcd.print(millis() / 1000);

  if(alarm)
  {
    lcd.setCursor(5,1);
    lcd.print("XXX");
    analogWrite(buzzer_pin, 255);
    delay(400);
    lcd.setCursor(5,1);
    lcd.print("   ");
    analogWrite(buzzer_pin, 0);
    delay(400);
    print_menu();
  }

  switch(menu)
  {
    case 3:
    lcd.setCursor(0,0);
    lcd.print(' ');
    delay(400);
    lcd.setCursor(0,0);
    lcd.print(hours/10);
    delay(400);
    break;

    case 4:
    lcd.setCursor(1,0);
    lcd.print(' ');
    delay(400);
    lcd.setCursor(1,0);
    lcd.print(hours%10);
    delay(400);
    break;

    case 5:
    lcd.setCursor(3,0);
    lcd.print(' ');
    delay(400);
    lcd.setCursor(3,0);
    lcd.print(minutes/10);
    delay(400);
    break;

    case 6:
    lcd.setCursor(4,0);
    lcd.print(' ');
    delay(400);
    lcd.setCursor(4,0);
    lcd.print(minutes%10);
    delay(400);
    break;

    case 7:
    lcd.setCursor(0,1);
    lcd.print(' ');
    delay(400);
    lcd.setCursor(0,1);
    lcd.print(alarm_hours/10);
    delay(400);
    break;

    case 8:
    lcd.setCursor(1,1);
    lcd.print(' ');
    delay(400);
    lcd.setCursor(1,1);
    lcd.print(alarm_hours%10);
    delay(400);
    break;

    case 9:
    lcd.setCursor(3,1);
    lcd.print(' ');
    delay(400);
    lcd.setCursor(3,1);
    lcd.print(alarm_minutes/10);
    delay(400);
    break;

    case 10:
    lcd.setCursor(4,1);
    lcd.print(' ');
    delay(400);
    lcd.setCursor(4,1);
    lcd.print(alarm_minutes%10);
    delay(400);
    break;

    default:
    break;
  }
}

void timer_ini(void)
{
  TCCR1A |= (1 << WGM12); // CTC mode
  TCCR1A &= ~( (1<<WGM13)|(1<<WGM11)|(1<<WGM10) );// set to zero
  TIMSK1 |= (1 << OCIE1A); // Output compare A match interrupt enabled
  OCR1AH = 0x3D; //  1 second when 16MHZ oscilator
  OCR1AL = 0x09; //  1 second when 16MHZ ocsilator
  TCCR1B |= (1 << CS12) | (1 << CS10); // prescaler 1024 for 1. timer 
  TCCR1B &= ~(1 << CS11); // set CS11 to zero
  //TCCR1B |= (1 << CS11); // prescaler 64 for 1. timer;
  //TCCR1B &= ~( (1 << CS12) | (1 << CS10) ); // set CS12 and CS10 to zero
}

ISR(TIMER1_COMPA_vect)
{
  if( !(menu >= 3 and menu <= 6) )
  {
    increase_time();
    print_time();
  }
  
  TCNT1H = 0x00; // clear timer1 after calculations
  TCNT1L = 0x00; // clear timer1 after calculations
}

void increase_time(void)
{
  seconds++;
  if(seconds == 60)
  {
    seconds = 0;
    minutes++;
    if(minutes == 60)
    {
      minutes = 0;
      hours++;
      if(hours == 24)
      {
        hours = 0;
      }
    }
    check_alarm();
  }
}

void print_time(void)
{
  // set the cursor to column 0, line 1
  // (note: line 1 is the second row, since counting begins with 0):
  //lcd.setCursor(0, 1);
  lcd.setCursor(0, 0);
  lcd.print(hours/10); // print first digit of hours
  lcd.print(hours%10); // print second digit of hours
  lcd.print(':');
  lcd.print(minutes/10);
  lcd.print(minutes%10);
  lcd.print(':');
  lcd.print(seconds/10);
  lcd.print(seconds%10);
}

void check_alarm(void)
{
  alarm = (alarm_hours == hours) and (alarm_minutes == minutes); // bool value if true, then start alarm
}

void print_alarm_time(void)
{
  lcd.setCursor(0, 1);
  lcd.print(alarm_hours/10); // print first digit of hours
  lcd.print(alarm_hours%10); // print second digit of hours
  lcd.print(':');
  lcd.print(alarm_minutes/10);
  lcd.print(alarm_minutes%10);
}


ISR(PCINT0_vect)
{  
  delay(50);
  
  if( !(PINB & 0x10) and !alarm) // button up and alarm is off
  {
    switch(menu)
    {
      case 1:
      menu = 2;
      break;

      case 2:
      menu = 1;
      break;

      case 3:
      hours += 10;
      if(hours > 23)
      {
        hours = 23;
      }
      break;

      case 4:
      hours += 1;
      if(hours > 23)
      {
        hours = 23;
      }
      break;

      case 5:
      minutes += 10;
      if(minutes > 59)
      {
        minutes = 59;
      }
      break;

      case 6:
      minutes += 1;
      if(minutes > 59)
      {
        minutes = 59;
      }
      break;

      case 7:
      alarm_hours += 10;
      if(alarm_hours > 23)
      {
        alarm_hours = 23;
      }
      break;

      case 8:
      alarm_hours += 1;
      if(alarm_hours > 23)
      {
        alarm_hours = 23;
      }
      break;

      case 9:
      alarm_minutes += 10;
      if(alarm_minutes > 59)
      {
        alarm_minutes = 59;
      }
      break;

      case 10:
      alarm_minutes += 1;
      if(alarm_minutes > 59)
      {
        alarm_minutes = 59;
      }
      break;

      default:
      break;
    }
  }
  else if( !(PINB & 0x08) and !alarm) // button enter and alarm is off
  {
    switch(menu)
    {
      case 1: // time select
      menu = 3;
      break;

      case 2: // alarm select
      menu = 7;
      break;

      case 3: //  hours +-10
      menu = 4;
      break;

      case 4: //  hours +-1
      menu = 5;
      break;

      case 5: //  minutes +-10
      menu = 6;
      break;

      case 6: // minutes +-1
      menu = 1;
      break;

      case 7: // alarm_hours +-10
      menu = 8;
      break;

      case 8: // alarm_hours +-1
      menu = 9;
      break;

      case 9: // alarm_minutes +-10
      menu = 10;
      break;

      case 10: // alarm_minutes +-11
      menu = 2;
      break;

      default:
      break;
    }
  }
  else if( !(PINB & 0x01) and !alarm ) // button down and alarm is off
  {
    switch(menu)
    {
      case 1:
      menu = 2;
      break;

      case 2:
      menu = 1;
      break;

      case 3:
      hours -= 10;
      if(hours < 0)
      {
        hours = 0;
      }
      break;

      case 4:
      hours -= 1;
      if(hours < 0)
      {
        hours = 0;
      }
      break;

      case 5:
      minutes -= 10;
      if(minutes < 0)
      {
        minutes = 0;
      }
      break;

      case 6:
      minutes -= 1;
      if(minutes < 0)
      {
        minutes = 0;
      }
      break;

      case 7:
      alarm_hours -= 10;
      if(alarm_hours < 0)
      {
        alarm_hours = 0;
      }
      break;

      case 8:
      alarm_hours -= 1;
      if(alarm_hours < 0)
      {
        alarm_hours = 0;
      }
      break;

      case 9:
      alarm_minutes -= 10;
      if(alarm_minutes < 0)
      {
        alarm_minutes = 0;
      }
      break;

      case 10:
      alarm_minutes -= 1;
      if(alarm_minutes < 0)
      {
        alarm_minutes = 0;
      }
      break;

      default:
      break;
    }
  }
  
  alarm = false;
  
  print_time();
  print_alarm_time();
  print_menu();
  
  delay(50);
}


void print_menu(void)
{
  switch(menu)
  {
    case 1:
    lcd.setCursor(8, 0);
    lcd.print(' ');
    lcd.setCursor(15, 1);
    lcd.print(' ');
    lcd.setCursor(15, 0);
    lcd.print(char_arrow);
    break;

    case 2:
    lcd.setCursor(5, 1);
    lcd.print(' ');
    lcd.setCursor(15, 0);
    lcd.print(' ');
    lcd.setCursor(15, 1);
    lcd.print(char_arrow);
    break;

    case 3:
    lcd.setCursor(8, 0);
    lcd.print(char_arrow);
    lcd.setCursor(15, 0);
    lcd.print(char_accept);
    break;

    case 7:
    lcd.setCursor(5, 1);
    lcd.print(char_arrow);
    lcd.setCursor(15, 1);
    lcd.print(char_accept);
    break;

    default:
    break;
  }
}
