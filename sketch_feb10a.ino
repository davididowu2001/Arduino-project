#include <Wire.h>
#include <Adafruit_RGBLCDShield.h>
#include <utility/Adafruit_MCP23017.h>

Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();

typedef enum {
  SYNC,
  MAIN,
  UP,
  DOWN,
  SELECT_BEFORE,
  SELECT_AFTER,
} State;

struct channels {
  char channel_name;
  String description;
  byte maxim;
  byte minim;
  byte value;

  void update_channel_name(char channel_name) {
    this-> channel_name = channel_name;
  }
  void update_description(String description) {
    //function to update channel description

    this->description = description;
  }

  void update_max(byte maximum) {
    //function to update max

    this->maxim = maximum;
  }
  void update_min(byte minimum) {
    //function to update max

    this->minim = minimum;
  }
  void update_value(byte value) {
    //funcion to update value

    this->value = value;
  }
};

bool is_number(String input) {
  //checks if a string corresponds to a number

  //iterate and check if character is a number
  for (int index = 0; index < input.length(); index++) { //position in input
    if (('\n' != input[index]) and not isDigit(input[index])) { //character is not a digit
      return false;
    }
  }
  return true;
}

#ifdef __arm__
// should use uinstd.h to define sbrk but Due causes a conflict
extern "C" char* sbrk(int incr);
#else // __ARM__
extern char *__brkval;
#endif // __arm__
int freeMemory() {
char top;
#ifdef __arm__
return &top - reinterpret_cast<char*>(sbrk(0));
#elif defined(CORE_TEENSY) || (ARDUINO > 103 && ARDUINO != 151)
return &top - __brkval;
#else // __arm__
return __brkval ? &top - __brkval : &top - __malloc_heap_start;
#endif // __arm__
}

void create_channel(channels* chan, String description, char chan_name) {
  //index the channel array
  //that gives you the channel
  //define the channel with its attributes

  chan->update_description(description);
  chan->update_max(255);
  chan->update_min(0);
  chan->update_value(0);
  chan->update_channel_name(chan_name);
}

bool validate(String command) {
  //this function checks if the command is valid

  int width = command.length();

  if (width >= 3) { //command length is at least 3
    if (isUpperCase(command[1])) {//second character is letter

      String description = command.substring(2, width);

      if (command[0] == 'C' && description.length() <= 15) { //the first character is 'C' and length (description ) <= 15
        return true;
      }

      /* if command starts with X or V or N, it should contain a letter and the other bit
         should be a number in the range(0,255) */
      else if ((command[0] == 'X') or (command[0] == 'V') or (command[0] == 'N')) { //the first character is 'X'or V or 'N')

        if (is_number(description)) { //description is a number
          long value = description.toInt();
          if (value >= 0 and value <= 255) { //description in the range (0,255)
            return true;
          }
        }
      }
    }
  }
  return false;
}

int get_channel_above(channels channel_array[], int pointer) {
  if (pointer == -1)
    return -1;

  pointer--;
  for (; pointer >= 0; pointer--) {
    if (channel_exists(channel_array + pointer)) {
      return pointer;
    }
  }

  return -1;
}

bool has_channel_above(channels channel_array[], int pointer) {
  return get_channel_above(channel_array, pointer) != - 1;
}

int get_channel_below(channels channel_array[], int pointer) {
  if (pointer == -1)
    return -1;

  pointer++;
  for (; pointer < 26; pointer++) {
    if (channel_exists(channel_array + pointer)) {
      return pointer;
    }
  }

  return -1;
}

bool has_channel_below(channels channel_array[], int pointer) {
  return get_channel_below(channel_array, pointer) != - 1;
}

void display_channels(channels channel_array[], int point_value,char** store_values) {
  int position_of_second_channel = get_channel_below(channel_array, point_value);

  bool has_above = has_channel_above(channel_array, point_value);
  bool has_below = has_channel_below(channel_array, position_of_second_channel);

  channels point = channel_array[point_value];
  if (channel_exists(&point)) {
    char info[17];

    // lcd.clear();
    byte uparrow[] = { B00100, B01110, B11111, B00100, B00100, B00100, B00100, B00000 };
    lcd.createChar(0, uparrow);
    char arrow = has_above ? 0 : ' ';
    lcd.setCursor(0, 0);
    sprintf(info, "%c%3d,%3d", point.channel_name, point.value, averageofvalues(point_value, store_values));
    Serial.println("----------------");
    Serial.print(has_above ? "^" : "");
    Serial.println(info);
    lcd.write(arrow);
    lcd.print(info);

    point = channel_array[position_of_second_channel];
    if (channel_exists(&point)) {
      byte downarrow[] = { B00000, B00100, B00100, B11111, B01110, B00100, B00000, B00000 };
      lcd.createChar(1, downarrow);
      arrow = has_below ? 1 : ' ';
      sprintf(info, "%c%3d,%3d", point.channel_name, point.value, averageofvalues(position_of_second_channel, store_values));
      Serial.print(has_below ? "v" : "");
      Serial.println(info);
      lcd.setCursor(0, 1);
      lcd.write(arrow);
      lcd.print(info);
    }
  }

  bool red = false;
  bool green = false;

  for (int i = 0; i < 26; i++) {
    channels channel = channel_array[i];
    if (channel_exists(&channel)) {
      if (channel.value > channel.maxim) {
        red = true;
      } else if (channel.value < channel.minim) {
        green = true;
      }
    }
  }

  if (red and green) {
    lcd.setBacklight(3);
  }
  else if (red) {
    lcd.setBacklight(1);
  }
  else if (green) {
    lcd.setBacklight(2);
  }
  else {
    lcd.setBacklight(7);
  }
}

int findchannel(channels array_channel[], char key) {
  //checks if a channel already exists
  //checks existing channels based on name
  // iterates through channels

  //if name exists , return index
  //else return -1

  if (!isUpperCase(key)) {
    return -1;
  }

  int index = key - 'A';
  if (channel_exists(array_channel + index))
    return index;

  return -1;
}

int get_channel_position(char name) {
  if (isUpperCase(name)) {
    return name - 'A';
  } else {
    return -1;
  }
}

bool channel_exists(channels *ch) {
  return ch->description.length() != 0;
}

int read_command(String &command, channels channel_array[], int pointer,char** store_values) {
  String desc = command.substring(2);
  int positionofchannel = get_channel_position(command[1]);
  channels *channel = &channel_array[positionofchannel];

  if (command[0] == 'C') { //first character is 'C')
    /* if the channel exists then update the desriptoion
       otherwise create a new channel in the array
    */

    Serial.print(F("DEBUG: exists: "));

    if (channel_exists(channel)) {
      Serial.println(F("true"));
      channel->update_description(desc);
    } else {
      Serial.println(F("false"));
      create_channel(channel, desc, command[1]);
      create_record(positionofchannel, store_values);
      
      // making first channel
      if (pointer == -1)
        pointer = positionofchannel;
    }
  }
  else {
    byte number = desc.toInt();
    
    if (channel_exists(channel)) {
      if (command[0] == 'X') {
        Serial.println(F("new max"));
        channel->update_max(number);
      }
      else if (command[0] == 'N') {
        Serial.println(F("new min"));
        channel->update_min(number);
      }
      else if (command[0] == 'V') {
        Serial.println(F("new val"));
        channel->update_value(number);
        update_store_values(number,positionofchannel,store_values);
      }
    }
  }

  return pointer;
}

int try_read_command(channels channel_array[], int pointer, bool display, char** store_values) {
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');

    if (validate(command)) {
      pointer = read_command(command, channel_array, pointer, store_values);
      if (display)
        display_channels(channel_array, pointer, store_values);
    } else {
      Serial.print(F("ERROR: "));
      Serial.println(command);
    }
  }

  return pointer;
}

void update_store_values(char value, char index,char** store_values){
  char pointer = store_values[index][0];
  store_values[index][1 + pointer] = value;
  store_values[index][0] = (pointer + 1)%64;
}

int averageofvalues(char index, char** store_values){
  char pointer = store_values[index][0];
  int sum = 0;

   Serial.print("Pointer for '");
   Serial.print((int) index);
   Serial.print("': ");
   Serial.print((int) pointer);
  
  for (char i = 0; i < pointer; i++){
    // Serial.print(", ");
    // Serial.print((int) store_values[65*index + i + 1]);
    sum += store_values[index][i + 1];
  }
  return sum/pointer;
}

void create_record(char index, char** store_values){
  store_values[index] = (char*) malloc(sizeof(char)*65);
  store_values[index][1] = 0;
  store_values[index][0] = 1;
  Serial.print("Pointer for '");
  Serial.print((int) index);
  Serial.print("': ");
  Serial.println((int) store_values[index][0]);
}

void setup() {
  // put your setup code here, to run once:
  lcd.begin(16, 2);
  lcd.setBacklight(5);
  lcd.clear();
  Serial.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly:

  //every static variable is an attribute of the manager
  static State current_state = SYNC;
  static channels channel_array[26];
  static long time_since_last_q = millis();
  static long timer = time_since_last_q;
  static int pointer = -1;
  static int time_of_select_held;
  static char** store_values = (char**) malloc(sizeof(char*)*26);

  int button = lcd.readButtons();

  switch (current_state) {
    case SYNC:
    /* 
    the current state is set to SYNC 
    for every one second that passes, 'Q' is printed on the screen
    when X is inputted, stop printing Q
    and light is set to purple 
    after that the state is set to MAIN*/
      if (1000 <= millis() - time_since_last_q) { //if the program has been run for 1 second,
        time_since_last_q = millis();
        Serial.print('Q');//print Q on the serial
      }

      if (Serial.available() and Serial.readString()[0] == 'X') { //if the command is X,
        lcd.setBacklight(7); //change lcd light to purple
        Serial.println(F("UDCHARS,FREERAM")); //print these
        current_state = MAIN; // move to the MAIN state
      }

      break;

    case MAIN:
    /* 
    pointer is set to the current channel
    and if the up  button is pressed, then the channel above is displayed
    if the down button is pressed, then the channel below is displayed
    if the select button held for a while, then the state move to the select before state
    */
      pointer = try_read_command(channel_array, pointer, true, store_values);

      if (button & BUTTON_UP) {// if up button is pressed
        Serial.println(F("DEBUG: UP pressed"));
        current_state = UP; //change state
      } else if (button & BUTTON_DOWN) {//if down button is pressed
        Serial.println(F("DEBUG: DOWN pressed"));
        current_state = DOWN; //change state
      } else if (button & BUTTON_SELECT) {
        Serial.println(F("DEBUG: SELECT pressed"));
        time_of_select_held = millis();
        current_state =  SELECT_BEFORE;
      }

      break;

    case UP:
      // if up button has been released
      if( not(button & BUTTON_UP)){ 
        //move to the next channel above
        pointer = get_channel_above(channel_array, pointer);
        if (has_channel_above(channel_array, pointer)) //if there is a channel above,
          pointer = get_channel_above(channel_array, pointer); //pointer moves to the above channel
        display_channels(channel_array, pointer, store_values); //display the above channel
        Serial.println(F("DEBUG: UP released"));
        current_state = MAIN; //state goes back to the main state
      }
      break;

    case DOWN:
      if (not(button & BUTTON_DOWN)){// if down button has been released
        int below = get_channel_below(channel_array, pointer);
        if (has_channel_below(channel_array, below))//move to the next channel below
          pointer = below;
        display_channels(channel_array, pointer, store_values);//display lower channel
        Serial.println(F("DEBUG: DOWN released"));
        current_state = MAIN; //state goes back to the main state
      }
      break;

    case SELECT_BEFORE:
      // check that 1s has gone since time_of_select_held
      if (millis() - time_of_select_held >= 1000) {
        lcd.clear();
        lcd.setBacklight(5);
        lcd.setCursor(0, 0);
        lcd.print(F("F127290"));
        lcd.setCursor(0, 1);
        lcd.print(freeMemory());
        current_state = SELECT_AFTER;
      }

      // select released
      if (not(button & BUTTON_SELECT)) {
        Serial.println(F("DEBUG: SELECT released before 1 second"));
        current_state = MAIN;
      }

      break;

    case SELECT_AFTER:
      pointer = try_read_command(channel_array, pointer, false, store_values);

      // select released
      if (not(button & BUTTON_SELECT)) {
        Serial.println(F("DEBUG: SELECT released"));
        lcd.clear();
        lcd.setBacklight(7);
        display_channels(channel_array, pointer, store_values);
        current_state = MAIN;
      }
      break;
  }
}

// for the HCL
//make right and left as states
//and then make function to get functions to display channels that are beyond maximum


//EEPROM
//make two states, OFF and ON
//function to write into the EEPROM
//function to read from the EEPROM