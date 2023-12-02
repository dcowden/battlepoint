#ifndef __INCSIMPLETIMER
#define _INGSIMPLETIMER
#include <FastLED.h>
#include <ArduinoLog.h>
#include <Clock.h>

typedef struct {
  long blinkIntervalMillis;
  CRGB fgColor;
  CRGB bgColor;
} PixelColor;

class LedPixel{
    public:
        LedPixel(Clock* clock, CRGB* leds, int index, CRGB fgColor, CRGB bgColor ){
            _index = index;
            _leds = leds;  
            _fgColor = fgColor;
            _bgColor = bgColor;
            _clock = clock;
            resetBlinker();
        };
        void setPixelColor ( PixelColor newColor){
            if ( newColor.blinkIntervalMillis > 0){
                setBlinkingColor(newColor.fgColor, newColor.bgColor, newColor.blinkIntervalMillis);
            }
            else{
                setSolidColor(newColor.fgColor);
            }
        }
        void setSolidColor ( CRGB newColor){
            Log.infoln("LedPixel %d, solid %l",_index, newColor);
            _flashIntervalMillis = 0;
            _fgColor = newColor;
            _bgColor = newColor;
            setColor(newColor);
        };
        void setBlinkingColor ( CRGB fgColor, CRGB bgColor, long flashIntervalMillis){
            Log.infoln("LedPixel %d, blinking %l",_index, fgColor);
            _fgColor = fgColor;
            _bgColor = bgColor;
            _flashIntervalMillis = flashIntervalMillis;
            resetBlinker();
        };        
        void update(){
            long currentTimeMillis = _clock->milliseconds();
            if ( _flashIntervalMillis > 0){
                long elapsed = (currentTimeMillis - _lastFlashMillis);
                if ( elapsed >= _flashIntervalMillis){
                    if ( _flashOn ){
                        setColor(_fgColor);
                    }
                    else {
                        setColor(_bgColor);
                    }
                    _flashOn = ! _flashOn;
                    _lastFlashMillis = currentTimeMillis;
                }                
            }
            FastLED.show();
        };

    protected:
        void setColor(CRGB newColor){
            _leds[_index]=newColor;            
        };
        void resetBlinker(){
            _flashOn = true;
            setColor(_bgColor);
            _lastFlashMillis = _clock->milliseconds();
        };
        int _index=0;
        Clock* _clock;
        CRGB _fgColor;
        CRGB _bgColor;
        CRGB* _leds;
        boolean _flashOn = true;
        long _flashIntervalMillis = 0;
        long _lastFlashMillis;
};

class LedGroup{
    public:
        LedGroup(LedPixel _pixels[], int numPixels){
            _pixels = _pixels;
            _numPixels = numPixels;
        };
        void setSolidColor ( CRGB newColor){
            for ( int i=0;i< _numPixels;i++){
                _pixels[i].setSolidColor(newColor);
            }
        };
        void setBlinkingColor ( CRGB fgColor, CRGB bgColor, long flashIntervalMillis){
            for ( int i=0;i< _numPixels;i++){
                _pixels[i].setBlinkingColor(fgColor, bgColor, flashIntervalMillis);
            }
        };
        void update(long currentTimeMills){
            for ( int i=0;i< _numPixels;i++){
                _pixels[i].update();
            } 
            FastLED.show();   
        };

    protected:  
        int _numPixels;
        LedPixel* _pixels;

};
#endif