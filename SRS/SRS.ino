#define PIR_PIN  8
#define SOUND_PIN A0
#define PUMP_PIN 13

#define PIR_ENABLE 1
#define SOUND_ENABLE 0
#define PUMP_ENABLE 1

#define PUMP_DURATION 500  // in milli
#define PUMP_DOWNTIME 1000 // in milli

#define SOUND_TIMEOUT 2000 // in milli
#define PIR_STANDBY 20000  // in milli
#define PIR_STARTUP 60000  // in milli

class Runnable {
    static Runnable *headRunnable;
    Runnable *nextRunnable;

  public:
    Runnable() {
      nextRunnable = headRunnable;
      headRunnable = this;
    }

    virtual void setup() = 0;
    virtual void loop() = 0;

    static void setupAll() {
      for (Runnable *r = headRunnable; r; r = r->nextRunnable)
        r->setup();
    }

    static void loopAll() {
      for (Runnable *r = headRunnable; r; r = r->nextRunnable)
        r->loop();
    }

};

Runnable *Runnable::headRunnable = NULL;

class PassiveIR: public Runnable {
    const byte inPin;
    const boolean enable;

    boolean initiated;
    unsigned long insideMs;
    unsigned long startMs;

    enum State {
        WAIT = 0,
        TRIGD = 1,
        STANDBY = 2
        } state;
    
  public:
    boolean statusOut;

    PassiveIR(byte inAttach , boolean enableIn) :
      inPin(inAttach),
      enable(enableIn)
    {
    }

    void setup() {
      pinMode(inPin, INPUT);

      state = WAIT;
      initiated = false;
      startMs = millis();
      statusOut = false ;
    }

    void loop() {
      if (enable) 
        {
          if (!initiated)
          {
            if   ((millis() - startMs) > PIR_STARTUP)
              initiated = true;
          }
          else
          {
            switch (state)
            {
              case WAIT:
                if (digitalRead(inPin))
                  state = TRIGD;
                break;
              case TRIGD:
                if (!digitalRead(inPin))
                {
                  state = STANDBY;
                  insideMs = millis();
                  statusOut = true;
                }
                break;
              case STANDBY:
                if ((millis() - insideMs) > PIR_STANDBY)
                  state = WAIT; 
              break;
            }
          }
        }
      else
        statusOut = true ;
    }
} ;

class MeowDetector: public Runnable {
    const byte inPin;
    const boolean enable;

  public:
    boolean statusOut;
    boolean activate;
    
    MeowDetector(byte inAttach , boolean enableIn) :
      inPin(inAttach),
      enable(enableIn)
      {
      }
      
    void setup() {
      statusOut = false;
      activate = false;
    }

    void loop() {
        if (enable)
        {
          if (activate)  
          {
            
          }
          else
            statusOut = false;
        }
      else
        statusOut = true;
        activate = false;
    }

    void detect()
    {
      activate = true;
    }
} ;

class Logic: public Runnable {
  MeowDetector &meow;
  PassiveIR &pir;
  
  enum State {
      WAIT = 0,
      TRIGD = 1,
      SHOOT = 2,
      RESET = 3
    } state;
    
  public: 
    boolean statusOut;

    Logic(MeowDetector &attachToMeowDetector, PassiveIR &attachToPassiveIR):
      meow(attachToMeowDetector),
      pir(attachToPassiveIR)
    {
    }

    void setup() 
    {
      statusOut = false;
      state = WAIT;          
    }

    void loop() {
      if (!statusOut)
      {
      switch (state)
      {
        case WAIT:
        
        if (pir.statusOut)
        {
          state = TRIGD;
          meow.activate = true;
        }
        break;
        case TRIGD:
        if (!meow.activate)
        {
          if (meow.statusOut)
          {
            state = SHOOT;
          }
          else
          {
            state = RESET;
          }
        }
        break;
        case SHOOT:
          statusOut = true;
          state = RESET;
        break;
        case RESET:
          if (!statusOut)
          {
            state = WAIT;
            meow.statusOut = false;
            pir.statusOut = false;
          }
        break;
      }
      }
    }
};

class WaterPump: public Runnable {
    const byte outPin;
    const boolean enable;
    Logic &logic;
    unsigned long startMs;

    enum State {
      WAIT = 0,
      WATER = 1,
      STOP = 2
    } state;
    
  public:
    boolean statusOut;

    WaterPump(byte outAttach , boolean enableIn ,Logic &attachToLogic) :
      outPin(outAttach),
      enable(enableIn),
      logic(attachToLogic)
    {
    }

    void setup() {
      pinMode(outPin, OUTPUT);

      statusOut = false;
      state = WAIT;
      digitalWrite(outPin, LOW);
    }

    void loop() {
         if (enable)
      {
        switch (state) {
          case WAIT:
            if (logic.statusOut)
            {
              digitalWrite(outPin, HIGH);
              startMs = millis();
              state = WATER;
            }
            else
              digitalWrite(outPin, LOW);
          break;   
          case WATER:
            if ((millis()-startMs) > PUMP_DURATION)
            {
              digitalWrite(outPin, LOW);
              state = STOP;
              startMs = millis();
            }
          break;
          case STOP:
            if ((millis()-startMs) > PUMP_DOWNTIME)
            {
              state = WAIT;
              logic.statusOut = false;
            }
          break;
        }
      }
    else
      digitalWrite(outPin, LOW);
    }
} ;

PassiveIR passiveIR(PIR_PIN,PIR_ENABLE);
MeowDetector meowDetector(SOUND_PIN,SOUND_ENABLE);
Logic logic(meowDetector,passiveIR);
WaterPump waterPump(PUMP_PIN,PUMP_ENABLE,logic);

void setup() {
  Runnable::setupAll();
}

void loop() {
  Runnable::loopAll();
}
