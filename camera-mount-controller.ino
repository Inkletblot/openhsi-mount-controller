//---------------------------------------------------------------------------//
// This code operates the hyperspectral camera mount actuator built for my
// honours project at Flinders Uni.
//
// The TB6600 ENA (enable) input is actually active LOW, so to turn off the
// stepper, pull this pin HIGH.
//
// The TB6600 DIR (direction) is, in our usecase, LOW to drive the mount away
// from the stepper motor, and HIGH to pull the mount towards the stepper motor.
//
// Good default pulse widths:
// Microsetp Pulse/Rev S1 S2 S3 Default
// 1         200       1  1  0  700
// 8         1600      0  1  0  60
//
// The Stepper being used is a 23H-D56001Y-32B from Maker Store. It is rated for
// 2.8A but I have it set to, and recommend, 2.5A on the TB6600 due to the peak
// being 2.7A. This equates to a switch configuration of S4=0, S5=1, S6=1.
//
// Author: Solomon Laing (lain0004, 2165308)
// Date: 2023-02-21
//---------------------------------------------------------------------------//

const int enable_pin = 13;
const int dir_pin = 12;
const int pulse_pin = 11;

const int short_stop_pin = 8; // closest to stepper
const int long_stop_pin = 9; // farthest from stepper

const int button_pin = 10;

boolean long_stop_flag = false;
boolean short_stop_flag = false;

enum State {
    START,     // 0
    RESET,     // 1
    RUN,       // 2
    STOPPED,   // 3
    STOP_WAIT  // 4
};

enum Direction {
    LONG,
    SHORT
};

State current_state;
Direction current_direction;

//---------------------------------------------------------------------------//
// This is a general idea of how long you want the transit from one end to the
// other to take. It will ususally be a couple of seconds over the number set
// here when transiting the full length of the actuator.
//
// Note, that setting to a small value that is too small, factoring for the 
// current microstep setting for the stepper and defined timed_pulse_width below
// may cause the stepper to malfunction.
//---------------------------------------------------------------------------//
const int TOTAL_SECONDS = 25;

const int PUL_PER_REV = 1600;

//--------------------------USER DEFINED DEFAULTS----------------------------//
// If you want to run run the timing init code, set init_flag to true. If this
// is not set, then timed_pulse_width will be used.
//
// Note, timed_pulse_width being too low for the current microstep setting will
// cause the stepper motor to malfunction.
//---------------------------------------------------------------------------//
boolean init_flag = true;

long timed_pulse_width = 100; // try and pick sane value

void pulse(int pulse_width) {
    digitalWrite(pulse_pin, HIGH);
    delayMicroseconds(pulse_width);
    digitalWrite(pulse_pin, LOW);
    delayMicroseconds(pulse_width);
}

void enable() {
    digitalWrite(enable_pin, LOW);
}

void disable() {
    digitalWrite(enable_pin, HIGH);
}

void go_short() {
    current_direction = SHORT;
    digitalWrite(dir_pin, HIGH);
}

void go_long() {
    current_direction = LONG;
    digitalWrite(dir_pin, LOW);
}

boolean short_stop() {
    return digitalRead(short_stop_pin) == LOW;
}

boolean long_stop() {
    return digitalRead(long_stop_pin) == LOW;
}

boolean button() {
    return digitalRead(button_pin) == LOW;
}

int init_pulse_timing() {
    Serial.print("Calculating pulse width required for a travel time of ");
    Serial.print(TOTAL_SECONDS);
    Serial.println(" seconds...");

    long pulses = 0;
    int pulse_width = timed_pulse_width;

    // run till oposite reset position
    enable();
    go_long();

    Serial.println("Resetting platform to long end of actuator...");

    while (digitalRead(long_stop_pin)) {
        if (!digitalRead(button_pin)) return;
        pulse(pulse_width);
    }

    go_short();

    Serial.println("Returning platform to home (short end) and counting pulses...");

    while (digitalRead(short_stop_pin)){
        if (!digitalRead(button_pin)) return;
        pulses++;
        pulse(pulse_width);
    }

    disable();

    int pul_per_sec = pulses / TOTAL_SECONDS;

    // how many microseconds will I need per pulse for my desired speed/time?
    // don't forget to divide by two because the pulse width is half of the
    // period.
    pulse_width = 1000000 / pul_per_sec / 2;

    Serial.print("A total of ");
    Serial.print(pulses);
    Serial.println(" pulses were counted.");

    Serial.print("That means that a total of ");
    Serial.print(pul_per_sec);
    Serial.println(" pulses per second will be required.");

    Serial.print("Which means a pulse width of ");
    Serial.print(pulse_width);
    Serial.print(" microseconds will be used.");
    
    return pulse_width;
}

void setup() {
    Serial.begin(9600); // start serial

    pinMode(short_stop_pin, INPUT_PULLUP);
    pinMode(long_stop_pin, INPUT_PULLUP);
    pinMode(button_pin, INPUT_PULLUP);

    pinMode(enable_pin, OUTPUT);
    pinMode(dir_pin, OUTPUT);
    pinMode(pulse_pin, OUTPUT);

    // Ensure stepper does not run
    disable();

    if (init_flag) {
        timed_pulse_width = init_pulse_timing();
    }

    current_state = START;

    Serial.println("Setup Complete!");
}

void loop() {
    //---------------------------------START---------------------------------//
    if (current_state == START){
        current_state = RESET;
    } 
    //---------------------------------RESET---------------------------------//
    else if (current_state == RESET) {
        enable();

        if (current_direction != SHORT) {
            // Don't need to run this multiple times
            go_short();
        }

        pulse(timed_pulse_width);

        if (short_stop() || button()) {
            current_state = STOP_WAIT;
        }
    }
    //-------------------------------STOP_WAIT-------------------------------//
    else if (current_state == STOP_WAIT) {
        disable();
        
        delay(500);

        current_state = STOPPED;
    }
    //----------------------------------RUN----------------------------------//
    else if (current_state == RUN) {
        if (current_direction != LONG) {
            // Don't need to run this multiple times
            go_long();
        }
        
        pulse(timed_pulse_width);

        if (long_stop()) {
            current_state = STOP_WAIT;
        }

        if (button()) {
            current_state = STOP_WAIT;
        }
    }
    //---------------------------------STOP----------------------------------//
    else if (current_state == STOPPED) {
        if (button()) {
            if (short_stop()) {
                current_state = RUN;
            } else {
                current_state = RESET;
            }

            enable();
            delay(250);
        }
    }
}
