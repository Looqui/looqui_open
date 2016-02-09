/*
    Copyright (C) 2014 Politecnico di Torino
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

    This software is developed within the PARLOMA project, which aims
    at developing a communication system for deablinf people (www.parloma.com)
    The PARLOMA project is developed with the Turin node of AsTech laboraroies
    network of Italian CINI (Consorzio Interuniversitario Nazionale di Informatica)

    Contributors:
        Ludovico O. Russo (ludovico.russo@polito.it)
        Andrea Bulgarelli
 */


#include <Servo.h>
#include <EEPROM.h>

#define CONFIG_VERSION  "ls1"
#define CONFIG_START    32


#define thumb_flex      0
#define index_flex      1
#define middle_flex     2
#define ring_flex       3
#define pinky_flex      4
#define thumb_abd       5
#define index_abd       6
#define middle_abd      7
#define wrist_1         8

#define MOTOR_NUM       9


struct StoreStruct {
  char version[4];
  int reset[MOTOR_NUM];
};

struct StoreStruct storeReset = {
  CONFIG_VERSION,
  {0,180,0,0,0,30,90,90,20} //punti di rest
};

void loadConfig() {
  // To make sure there are settings, and they are YOURS!
  // If nothing is found it will use the default settings.
  if (EEPROM.read(CONFIG_START + 0) == CONFIG_VERSION[0] &&
      EEPROM.read(CONFIG_START + 1) == CONFIG_VERSION[1] &&
      EEPROM.read(CONFIG_START + 2) == CONFIG_VERSION[2])
    for (unsigned int t=0; t<sizeof(storeReset); t++)
      *((char*)&storeReset + t) = EEPROM.read(CONFIG_START + t);
}

void saveConfig() {
  for (unsigned int t=0; t<sizeof(storeReset); t++)
    EEPROM.write(CONFIG_START + t, *((char*)&storeReset + t));
}


const int   SET_ALL              = 241;
const int   SET_ONE              = 242;
const int   GET_ALL_POS          = 243;
const int   GET_FINGER_POS       = 244;
const int   RESET_ALL            = 245;
const int   TEST_ALL            = 246;
const int   SAVE_RESET_ALL        = 247;


class Motor{
  public:
    Motor();
    void init(int pin, int pos = 0, int p_min = 0, int p_max = 180, bool direct = true);
    void reset();
    void setPosition(int pos);
    int get_mpos();
    void test();
    void set_reset(int res);
    void check_detach();

  private:
    Servo motor;
    int mpos;
    int p_max;
    int p_min;
    int pos_reset;
    bool direct;
    int detach_count;
    int pin;
};

Motor::Motor(): mpos(-1), detach_count(0) {}

void Motor::check_detach() {
  this->detach_count++;
  if (this->detach_count > 1000) {
    this->motor.detach();
  }
}

void Motor::set_reset(int res) {
  pos_reset = res;
}


void Motor::init(int pin, int pos, int p_min, int p_max, bool direct)
{
  this->p_max = p_max;
  this->p_min = p_min;
  this->direct = direct;
  if (pos >= 0 && pos <= 180) {
    pos_reset = pos;
  } else {
    pos_reset = 0;
  }
  this->pin = pin;
  this->reset();
}



void Motor::reset() {
  this->setPosition(pos_reset);
  detach_count = 0;
}

void Motor::setPosition(int pos) {
  if (!this->motor.attached()) {
      this->motor.attach(this->pin);
    }
  this->detach_count = 0;
  if (pos >= 0 && pos <= 180) {
    if (!this->direct) pos = 180-pos;
    this->mpos = map(pos, 0, 180, p_min, p_max);
    this->motor.write(this->mpos);
  }
}

int Motor::get_mpos(){
  return map(mpos, p_min, p_max, 0, 180);
}

void Motor::test() {
  int thispose = this->mpos;
  for (int i = thispose; i <= 180; i++) {
    this->setPosition(i);
    delay(5);
  }
  for (int i = 180; i >= 0; --i) {
      this->setPosition(i);
      delay(5);
  }
  for (int i = 0; i <= thispose; ++i) {
      this->setPosition(i);
      delay(5);
  }
}

Motor motors[MOTOR_NUM];


void allocate_motors() {
  motors[thumb_flex].init(  9,   storeReset.reset[thumb_flex],  0,  90,  true);
  motors[index_flex].init(  8,   storeReset.reset[index_flex],  0,  160,  false);
  motors[middle_flex].init(  7,   storeReset.reset[middle_flex],  0,  150,  true);
  motors[ring_flex].init(  10,   storeReset.reset[ring_flex],  0,  165,  true);
  motors[pinky_flex].init(  6,   storeReset.reset[pinky_flex],  0,  155,  true);
  motors[thumb_abd].init(  3,   storeReset.reset[thumb_abd],  0,  90,  true);
  motors[index_abd].init(  5,   storeReset.reset[index_abd],  0,  180,  true);
  motors[middle_abd].init(  4,   storeReset.reset[middle_abd],  0,  180,  true);
  motors[wrist_1].init(  11,   storeReset.reset[wrist_1],  10,  90,  true);
}


void setPosition_all() {

 while(Serial.available() < MOTOR_NUM);
 int val[MOTOR_NUM];
 for (int i = 0; i < MOTOR_NUM; i++) {
    val[i] = Serial.read();
  }

  for (int i = 0; i < MOTOR_NUM; i++) {
    motors[i].setPosition(val[i]);
  }
}

void reset_all() {
  for (int i = 0; i < MOTOR_NUM; i++) {
    motors[i].reset();
  }
}


void setPosition_one() {
  while (Serial.available() < 2);
  int tosetPosition = Serial.read();
  if (tosetPosition < 0 ) return;
  if (tosetPosition < MOTOR_NUM) {
    int pos = Serial.read();
    motors[tosetPosition].setPosition(pos);
    Serial.write(3);  
    Serial.write(SET_ONE);  
    Serial.write(tosetPosition);
    Serial.write(pos);
  }
}

void get_all_mpos() {
  for (int i = 0; i < MOTOR_NUM; i++) {
    Serial.write((char) motors[i].get_mpos());
  }
}

void get_finger_mpos() {
  if (Serial.available() < 1) {
    return;
  }
  int toget = Serial.read();
  if (toget < MOTOR_NUM) {
    Serial.write((char) motors[toget].get_mpos());
  }
}

void test_motor(int mot) {
  if (mot < MOTOR_NUM) {
     for (int i = 0; i < 180; i++) {
        motors[mot].setPosition((i+120)%180);
        delay(10);
    }
  }
}

void test_all(){
  Serial.write(1);
  Serial.write(TEST_ALL);
  for (int i = 0; i < MOTOR_NUM; i++) {
    motors[i].test();
  }
}

void check_detach_all(){
  for (int i = 0; i < MOTOR_NUM; i++) {
    motors[i].check_detach();
  }
}


void save_reset_all(){
  for (int i = 0; i < MOTOR_NUM; i++) {
    storeReset.reset[i] = motors[i].get_mpos();
    motors[i].set_reset(storeReset.reset[i]);
  }
  saveConfig();
}


void setup()
{
  loadConfig();
  allocate_motors();
  Serial.begin(9600);
}

void loop()
{
  if (Serial.available()) {
    int Sbyte = Serial.read(); //byte inizio
    switch (Sbyte) {
      case SET_ALL:
        setPosition_all();
        break;
      case TEST_ALL:
          test_all();
        break;
      case SET_ONE:
        setPosition_one();
        break;
      case GET_FINGER_POS:
        get_finger_mpos();
        break;
      case GET_ALL_POS:
        get_all_mpos();
        break;
      case RESET_ALL:
        reset_all();
        break;
      case SAVE_RESET_ALL:
        save_reset_all();
        break;
    }
  } else {
  }
  check_detach_all();
  delay(1);
}
