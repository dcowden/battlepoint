#include <Arduino.h>
#include <unity.h>

#define LED_COUNT 8




void light_leds (int range_start, int range_end, int max_val, int val ){

  int startIndex = 0;
  int endIndex = 0;
  int indexIncrement =0;
  
  if ( range_start < range_end ){
    startIndex = range_start;
    endIndex = range_end;
    indexIncrement = 1;
    
  }
  else{
    //reversed
    startIndex = range_end;
    endIndex = range_start;
    indexIncrement = -1;
  }
  int total_lights = endIndex - startIndex + 1;
  int num_lights_on  = map(val,0,max_val,0,total_lights);
  int currentIndex = startIndex;
  Serial.print(val);
  Serial.print("-->\t [ ");
  for(int i=0;i<total_lights;i++){
    if ( i >= num_lights_on ){
      Serial.print("X");
    }
    else{
      Serial.print("0");
    }
    currentIndex += indexIncrement;
  }
  Serial.println("]");
}

long map_value(long v,  long max_v,  long out_min, long out_max) {  
    int r = v * out_max / max_v;   
    Serial.print(v);
    Serial.print("-->");
    Serial.println(r);  
}

void map_value_old(int v, int max_v, int startIndex, int endIndex){
    int r = map(v,0,max_v,startIndex,endIndex);    
    Serial.print(v);
    Serial.print("-->");
    Serial.println(r);
}

void test_map_value(int max_v, int startIndex, int endIndex){
    Serial.print("******* max=");
    Serial.print(max_v);
    Serial.print(", start=");
    Serial.print(startIndex);
    Serial.print(", end=");
    Serial.print(endIndex);
    Serial.println("*******");
    map_value(0,max_v,startIndex,endIndex);
    map_value(1,max_v,startIndex,endIndex);    
    map_value(13,max_v,startIndex,endIndex);
    map_value(20,max_v,startIndex,endIndex);
    map_value(25,max_v,startIndex,endIndex);
    map_value(26,max_v,startIndex,endIndex);
    map_value(37,max_v,startIndex,endIndex);
    map_value(38,max_v,startIndex,endIndex);
    map_value(39,max_v,startIndex,endIndex);
    map_value((max_v/2) - 1,max_v,startIndex,endIndex);
    map_value(max_v/2,max_v,startIndex,endIndex);
    map_value((max_v/2) + 1,max_v,startIndex,endIndex);    
    map_value(max_v-1,max_v,startIndex,endIndex);
    map_value(max_v,max_v,startIndex,endIndex);
}

void test_map_values(void){
    int max_val = 8;
    map_value(0,100,0,max_val);    
    map_value(12,100,0,max_val);
    map_value(13,100,0,max_val);
    map_value(24,100,0,max_val);
    map_value(25,100,0,max_val);
    map_value(37,100,0,max_val);
    map_value(38,100,0,max_val);
    map_value(49,100,0,max_val);
    map_value(50,100,0,max_val);
    map_value(85,100,0,max_val);
    map_value(87,100,0,max_val);
    map_value(88,100,0,max_val);
    map_value(99,100,0,max_val);
    map_value(100,100,0,max_val);
}

void test_map_positions(void) {
   
    int MAX_VAL=8;
    Serial.print("Testing: ");
    Serial.println(MAX_VAL);
    Serial.println("         1234567");     
    for (int i=0;i<=MAX_VAL;i++){
        light_leds(0,7,MAX_VAL,i);
    }
    MAX_VAL=16;
    Serial.print("Testing: ");
    Serial.println(MAX_VAL);
    Serial.println("   \t 1234567");     

    for (int i=0;i<=MAX_VAL;i++){
        light_leds(0,7,MAX_VAL,i);
    } 
    MAX_VAL=10;
    Serial.print("Testing: ");
    Serial.println(MAX_VAL);
    Serial.println("   \t 1234567");     

    for (int i=0;i<=MAX_VAL;i++){
        light_leds(0,7,MAX_VAL,i);
    }    

    MAX_VAL=24;
    Serial.print("Testing: ");
    Serial.println(MAX_VAL);
    Serial.println("   \t 1234567");     

    for (int i=0;i<=MAX_VAL;i++){
        light_leds(0,7,MAX_VAL,i);
    }           
}

void setup() {
    
    delay(500);
    Serial.begin(115200);
    UNITY_BEGIN();
    RUN_TEST(test_map_values);
    UNITY_END();

}
void loop() {
    delay(500);
}