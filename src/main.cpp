#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include <EEPROM.h>

#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF
#define GREY    0xD6BA

#define Buzzer 3
int melody1[] = {250};
int melody2[] = {440,392,349};
int melody3[] = {500};
int melody4[] = {100};

#define TFT_DC 9
#define TFT_CS 10
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

#define JOY_VERT 0
#define JOY_HORIZ 1
#define JOY_SEL 2

#define NORTH 2
#define EAST 1
#define SOUTH -2
#define WEST -1

struct SnakeBody{
  int x;
  int y;
  int XMax;
  int XMin;
  int YMax;
  int YMin;
};

SnakeBody snake[100];
SnakeBody food[1];
SnakeBody poison[1];

// Size is 1 more than the length of the snake
int size = 3;
unsigned int level = 1;
unsigned int score = 0;
unsigned int highScore = 0;

unsigned long foodTimer = 5;    //FOOD_DISAPPEAR_TIME 
unsigned long currentMillis = 0;
unsigned long foodSpawnTime = 0;

bool poison_initialized = false;
bool barrier_drawn = false;
bool menu_exit = false;

int barrier_x = 100;
int barrier_y = 100;

int direction = EAST;
int speed_delay = 400;
int flagAddress = 5;

void menu();
void home_screen();
void play();
void gen_food();
void poison_initialize();
void snake_movement(int direction);
void food_colision();
void buzzersoundfood();
void buzzersoundtimer();
void buzzersoundGameOver();
void poison_collision();
void gen_poison();
void clear();
bool check_collision();
void lose();
int joystick();
void update_snake();
void gen_barrier();
void food_respaw();

void setup(){
  tft.begin();
  Serial.begin(9600);
  randomSeed(analogRead(A2));
  tft.fillScreen(BLACK);
  home_screen();
  pinMode(JOY_SEL, INPUT_PULLUP);
  pinMode (Buzzer,OUTPUT);

  int firstTimeFlag = EEPROM.read(flagAddress);
  if (firstTimeFlag != 1) {
    // First Time Running
    // Set the EEPROM values at 1st time
    EEPROM.write(0, 0);  // Pre-Set Value
    // Set the flag to indicate that the program has run before
    EEPROM.write(flagAddress, 1);
  }
}

void loop(){ 
  if (digitalRead(JOY_SEL) == LOW){
    tft.fillScreen(GREY);
    tft.fillRect(0, 25, 240, 5, BLACK);
    tft.fillRect(0, 290, 300, 5, BLACK);
    play();
  }
  if (menu_exit == false){
    menu();
  }
}

void menu(){
  if(abs(analogRead(JOY_VERT) - 512) > 0){
    if(analogRead(JOY_VERT) < 510){
      int high_score = EEPROM.read(0);
      tft.fillScreen(BLACK);
      
      tft.setCursor(30, 30);
      tft.setTextColor(WHITE);
      tft.setTextSize(3);
      tft.println("High Score");

      tft.setTextColor(GREEN);
      tft.setTextSize(5);
      tft.setCursor(100, 130);
      tft.println(high_score);

      tft.setCursor(20, 240);
      tft.setTextColor(WHITE);
      tft.setTextSize(2);
      tft.println("Press OK to Start");
      tft.fillRect(18, 280, 205, 10, CYAN);
    }
  }
}

void home_screen(){
 tft.setCursor(80, 15);
 tft.setTextSize(2);
 tft.println("200341A");
 tft.setCursor(80, 40);
 tft.setTextSize(2);
 tft.println("200150L");

 tft.setCursor(65, 90);
 tft.setTextSize(4);
 tft.println("Snake");
 tft.setCursor(75, 140);
 tft.setTextSize(4);
 tft.println("Game");
 
 tft.fillRect(40, 70, 150, 10, CYAN);
 tft.fillRect(40, 70, 10, 110, CYAN);
 tft.fillRect(40, 180, 150, 10, CYAN);
 tft.fillRect(190, 70, 10, 120, CYAN);

 tft.setCursor(20, 220);
 tft.setTextSize(2);
 tft.println("Press OK to Start");
 
 tft.fillRect(18, 240, 205, 10, BLUE);
 
 tft.setCursor(20, 270);
 tft.setTextSize(2);
 tft.println("Press DOWN to see");
 tft.setCursor(20, 290);
 tft.setTextSize(2);
 tft.println("HIGH Score");

 tft.fillRect(18, 310, 205, 10, BLUE);
}

void play(){
  // Initializng the position of the food
  food[0].x = 10;
  food[0].y = 10;
  food[0].XMax = 16 * food[0].x + 16;
  food[0].XMin = 16 * food[0].x;
  food[0].YMax = 16 * food[0].y + 16;
  food[0].YMin = 16 * food[0].y;
  // Initializing the starting postion of the snake
  snake[0].x = 6;
  snake[0].y = 10;
  snake[1].x = 6;
  snake[1].y = 10;
  // Draw boarders of the display (tft library won't let us use the whole screen)
  for (int i = 0; i < size; i++){
    snake[i].XMax = 16 * snake[i].x + 16;
    snake[i].XMin = 16 * snake[i].x;
    snake[i].YMax = 16 * snake[i].y + 16;
    snake[i].YMin = 16 * snake[i].y;
  }
  direction = EAST;
  size = 3;
  bool game_state = true;
  while (game_state == true){
    // Prevents the snake from going in the exact opposite direction it is traveling in
    if (direction != -joystick()){
    // Changes direction to the direction of the joystick
      direction = joystick();
    }
    // Moves snake head in the direction of travel 
    snake_movement(direction);
    // Moves rest of snake body accordingly (func pear is inside that checks for food collision)
    update_snake();
    game_state = check_collision();
  }
  return;
}

// Moves the head of the snake in the specified direction
void snake_movement(int direction){
  if (direction == NORTH){
    snake[0].y ++;
    if (snake[0].y == 18){
      snake[0].y = 2;
    }
  }
  else if (direction == SOUTH){
    snake[0].y --;
    if (snake[0].y == 1){
      snake[0].y = 17;
    }
  }
  else if (direction == WEST){
    snake[0].x --;
    if (snake[0].x == 0){
      snake[0].x = 14;
    }
  }
  else if (direction == EAST){
    snake[0].x ++;
    if (snake[0].x == 15){
      snake[0].x = 0;
    }
  }
}

// Reads the joystick
int joystick(){
  if(abs(analogRead(JOY_VERT) - 512) > 0){
   if(analogRead(JOY_VERT) < 510){
    return NORTH;
    }
    else if(analogRead(JOY_VERT) > 520){
    return SOUTH;
   }
  }
  else if(abs(analogRead(JOY_HORIZ) - 512) > 0){
   if(analogRead(JOY_HORIZ) < 510){
    return EAST;
   }
   else if(analogRead(JOY_HORIZ) > 520){
      return WEST;
   }
  }
  else{
   return direction;
  }
}

// Moves the snake
void update_snake(){
  for (int i = 0; i < size; i++){
    tft.fillRect(snake[i].XMin, snake[i].YMin, 15, 15, GREY);
  }
  for (int i = size; i > 0; i--){
    snake[i].x = snake[i-1].x;
    snake[i].y = snake[i-1].y;
  }
  for (int i = 0; i < size; i++){
    snake[i].XMax = 16 * snake[i].x + 16;
    snake[i].XMin = 16 * snake[i].x;
    snake[i].YMax = 16 * snake[i].y + 16;
    snake[i].YMin = 16 * snake[i].y;
  }
  for (int i = 1; i < size; i++){
    tft.fillRect(snake[i].XMin, snake[i].YMin, 15, 15, CYAN);
  }
  tft.fillRect(snake[0].XMin, snake[0].YMin, 15, 15, BLUE);
  food_colision();
  delay(speed_delay);

  // LEVEL 2
  if (level >= 2){
    if (barrier_drawn == false){
      gen_barrier();
      tft.setCursor(barrier_x, barrier_y);
      tft.setTextColor(BLACK);
      tft.setTextSize(8);
      tft.print("6");
      barrier_drawn = true;
    }
  }

  // LEVEL 3
  if (level >= 3){
    currentMillis = millis();
    // If food has been present for more than 5 seconds, make it disappear and respawn
    if (currentMillis - foodSpawnTime >= foodTimer * 1000) {
      tft.fillRect(food[0].XMin, food[0].YMin, 16, 16, GREY);
      food_respaw();  // Respawn food
      foodSpawnTime = millis();  // Reset timer
    }
    // Display countdown for food disappearing
    int remainingTime = foodTimer - ((millis() - foodSpawnTime) / 1000);
    // Clear previous timer display & Display remaining time
    tft.fillRect(150, 300, 150, 25, GREY);
    tft.setCursor(80, 300);
    tft.setTextColor(BLACK);
    tft.setTextSize(2);
    tft.print("Time");
    
    tft.setCursor(150, 300);
    tft.setTextColor(BLACK);
    tft.setTextSize(2);
    tft.print(remainingTime);
    buzzersoundtimer();
  }

  // LEVEL 4
  if (level >= 4){
    if (poison_initialized == false){
      poison_initialize();
      poison_initialized = true;
    }
    poison_collision();
  }

  // LEVEL 5
  if (level >= 5){
    // Increase Speed by 20% (Default speed_delay = 400)
    speed_delay = (400-(400*0.2*(level-4)));
    //poison_level5();
  }
}

void score_print(){
  if (score % 2 == 0) {
    level = ((score / 2) + 1);
  }
    
  tft.fillRect(0, 0, 240, 25, GREY);

  tft.setCursor(15, 5);
  tft.setTextColor(BLACK);
  tft.setTextSize(2);
  tft.println("Score");

  tft.setTextColor(BLACK);
  tft.setCursor(90, 5);
  tft.setTextSize(2);
  tft.println(score);

  tft.setCursor(130, 5);
  tft.setTextColor(BLACK);
  tft.setTextSize(2);
  tft.println("Level");

  tft.setTextColor(BLACK);
  tft.setCursor(210, 5);
  tft.setTextSize(2);
  tft.println(level);
}

bool check_collision(){
  for (int i = 2; i <= size; i++){
    if (snake[0].x == snake[i].x && snake[0].y == snake[i].y){
      lose();
      return false;
    }
  }
  if (level >= 2){
    for (int i = 0; i <= 40; i++){
      if (snake[0].x == int((barrier_x+i)/16)){
        for (int j = 0; j <= 60; j++){
          if (snake[0].y == int(((barrier_y+j)/16))){
            lose();
            return false;
          }
        }
      }
    }
  }
  return true;
}

void lose(){
  // Save the score of the game
  if (score > highScore){
    highScore = score;
    EEPROM.write(0, score); 
  }
  // Clear the game (End the game)
  clear();
  tft.fillScreen(GREY);
  // Print the game over messages to the screen
  tft.setTextColor(RED);
  tft.setCursor(60, 20);
  tft.setTextSize(4);
  tft.println("X X X");
  tft.setCursor(15, 75);
  tft.setTextSize(4);
  tft.println("GAME OVER");
  // Prints the score of the game to the screen
  tft.setTextColor(BLACK);
  tft.setCursor(55, 135);
  tft.setTextSize(3);
  tft.println("SCORE: ");
  tft.setCursor(160, 135);
  tft.println(score);
  tft.setTextSize(2);
  tft.setCursor(40, 175);
  tft.println("HIGH SCORE: ");
  tft.setCursor(185, 175);
  tft.println(highScore);
  tft.setTextColor(MAGENTA);
  tft.setTextSize(2);
  tft.setCursor(55, 230);
  tft.println("PRESS OK TO");
  tft.setCursor(30, 255);
  tft.setTextSize(3);
  tft.println("PLAY AGAIN");
  buzzersoundGameOver();
  // Resets the the game parameters
  score = 0;
  level = 1;
  barrier_drawn = false;
  poison_initialized = false;
  speed_delay = 400;
}

void clear(){
  for (int i = 0; i < size; i++){
    tft.fillRect(snake[i].XMin, snake[i].YMin, 16, 16, GREY);
  } 
  tft.fillRect(food[0].XMin, food[0].YMin, 16, 16, GREY);
  tft.fillRect(poison[0].XMin, poison[0].YMin, 16, 16, GREY);
  for (int i = 100; i > 3; i--){
    snake[i].x = -1;
    snake[i].y = -1;
  }
}

void buzzersoundfood(){
  for (int i =0 ; i < 1 ; i++){
    tone(Buzzer,melody1[i]);
    delay(50);
    noTone(Buzzer);
    delay(50);
  }
}

void buzzersoundpoison(){
  for (int i =0 ; i < 1 ; i++){
    tone(Buzzer,melody3[i]);
    delay(50);
    noTone(Buzzer);
    delay(50);
  }
}

void buzzersoundtimer(){
  for (int i =0 ; i < 1 ; i++){
    tone(Buzzer,melody4[i]);
    delay(50);
    noTone(Buzzer);
    delay(50);
  }
}

void buzzersoundGameOver(){
  for (int i =0 ; i < 3 ; i++){
    tone(Buzzer,melody2[i]);
    delay(500);
    noTone(Buzzer);
    delay(500);
  }
}

// Checks for colision with food
void food_colision(){ 
  if (food[0].x == snake[0].x && food[0].y == snake[0].y){
    foodSpawnTime = millis();  // Reset timer
    size++;
    score++;
    score_print();
    buzzersoundfood();
    gen_food();
    food[0].XMax = 16 * food[0].x + 16;
    food[0].XMin = 16 * food[0].x;
    food[0].YMax = 16 * food[0].y + 16;
    food[0].YMin = 16 * food[0].y;
  }
  tft.fillRect(food[0].XMin, food[0].YMin, 15, 15, GREEN);
}

void food_respaw(){ 
  buzzersoundtimer();
  gen_food();
  food[0].XMax = 16 * food[0].x + 16;
  food[0].XMin = 16 * food[0].x;
  food[0].YMax = 16 * food[0].y + 16;
  food[0].YMin = 16 * food[0].y;
  tft.fillRect(food[0].XMin, food[0].YMin, 15, 15, GREEN);
}

// Generates valid random positions for the food to spawn
void gen_food(){
  // Ensure food is not too close to the barrier
  food[0].x = random(2, 14);
  food[0].y = random(2, 18); 
  for (int i = 0; i <= 40; i++){
    if (food[0].x == int((barrier_x+i)/16)){
      for (int j = 0; j <= 60; j++){
        if (food[0].y == int(((barrier_y+j)/16))){
          gen_food();
        }
      }
    }
  }
  for (int i = 0; i < size; i++){
    if (food[0].x == snake[i].x && food[0].y == snake[i].y){
      gen_food();
      break;
    }
  }  
}

void poison_initialize(){
  poison[0].x = 10;
  poison[0].y = 10;
  poison[0].XMax = 16 * poison[0].x + 16;
  poison[0].XMin = 16 * poison[0].x;
  poison[0].YMax = 16 * poison[0].y + 16;
  poison[0].YMin = 16 * poison[0].y;
}
// Checks for colision with poison
void poison_collision(){ 
  if (poison[0].x == snake[0].x && poison[0].y == snake[0].y){
    score--;
    score_print();
    buzzersoundpoison();
    gen_poison();
    poison[0].XMax = 16 * poison[0].x + 16;
    poison[0].XMin = 16 * poison[0].x;
    poison[0].YMax = 16 * poison[0].y + 16;
    poison[0].YMin = 16 * poison[0].y;
  }
  tft.fillRect(poison[0].XMin, poison[0].YMin, 15, 15, RED);
}

// Generates valid random positions for the poison to spawn
void gen_poison(){
  // Ensure poison is not too close to the barrier
  poison[0].x = random(2, 14);
  poison[0].y = random(2, 18); 
  for (int i = 0; i <= 40; i++){
    if (poison[0].x == int((barrier_x+i)/16)){
      for (int j = 0; j <= 60; j++){
        if (poison[0].y == int(((barrier_y+j)/16))){
          gen_poison();
        }
      }
    }
  }
  for (int i = 0; i < size; i++){
    if (poison[0].x == snake[i].x && poison[0].y == snake[i].y){
      gen_poison();
      break;
    }
  }  
}

// Generates valid random positions for the barrier
void gen_barrier(){
  barrier_x = random(20, 200);
  barrier_y = random(50, 240);
  for (int i = -50; i <= 100; i++){
    if (int((barrier_x+i)/16) == snake[0].x){
      for (int j = -50; j <= 100; j++){
        if (int(((barrier_y+j)/16)) == snake[0].y){
            gen_barrier();
        }
      }
    }
  }
}


