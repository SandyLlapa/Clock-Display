#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "clock.h"

// Reads the time of day from the TIME_OF_DAY_PORT global variable. If
// the port's value is invalid (negative or larger than 16 times the
// number of seconds in a day) does nothing to tod and returns 1 to
// indicate an error. Otherwise, this function uses the port value to
// calculate the number of seconds from start of day (port value is
// 16*number of seconds from midnight). Rounds seconds up if there at
// least 8/16 have passed. Uses shifts and masks for this calculation
// to be efficient. Then uses division on the seconds since the
// begining of the day to calculate the time of day broken into hours,
// minutes, seconds, and sets the AM/PM designation with 1 for AM and
// 2 for PM. By the end, all fields of the `tod` struct are filled in
// and 0 is returned for success.
//
// CONSTRAINT: Uses only integer operations. No floating point
// operations are used as the target machine does not have a FPU.
//
// CONSTRAINT: Limit the complexity of code as much as possible. Do
// not use deeply nested conditional structures. Seek to make the code
// as short, and simple as possible. Code longer than 40 lines may be
// penalized for complexity.

int set_tod_from_ports(tod_t *tod){
  int port_val = TIME_OF_DAY_PORT; // if you were to divide this number by 16, it will give you the number of seconds since midnight

  if (port_val < 0 || port_val > 16 * 86400){ // error checking of out of bounds values
    return 1;
  }

  tod->day_secs = (port_val + 8) >> 4;

  tod->time_hours = (tod->day_secs / 3600) % 24;
  if (tod->time_hours == 0){ // special case: if hours is 0, then its changed to be 12
    tod->time_hours = 12;
  }
  else if (tod->time_hours > 12){ // otherwise subtract 12
    tod->time_hours -= 12;
  }

  tod->time_mins = (tod->day_secs % 3600) / 60;
  tod->time_secs = (tod->day_secs % 60);

  if (tod->day_secs >= 43200){  // if seconds is past noon (43200 sec), ampm will be 2
    tod->ampm = 2;
  }
  else{
    tod->ampm = 1;
  }
  return 0;
}

// Accepts a tod and alters the bits in the int pointed at by display
// to reflect how the LCD clock should appear. If any time_** fields
// of tod are negative or too large (e.g. bigger than 12 for hours,
// bigger than 59 for min/sec) or if the AM/PM is not 1 or 2, no
// change is made to display and 1 is returned to indicate an
// error. The display pattern is constructed via shifting bit patterns
// representing digits and using logical operations to combine them.
// May make use of an array of bit masks corresponding to the pattern
// for each digit of the clock to make the task easier.  Returns 0 to
// indicate success. This function DOES NOT modify any global
// variables
//
// CONSTRAINT: Limit the complexity of code as much as possible. Do
// not use deeply nested conditional structures. Seek to make the code
// as short, and simple as possible. Code longer than 85 lines may be
// penalized for complexity.

int set_display_from_tod(tod_t tod, int *display){
  //bit masks that correspond to digits 0-9
  int mask[10] = {0b1110111, 0b0100100, 0b1011101, 0b1101101, 0b0101110, 0b1101011, 0b1111011, 0b0100101, 0b1111111, 0b1101111}; 

  // error checking
  if (tod.time_secs < 0||tod.time_secs > 59 || tod.time_mins <0 ||tod.time_mins > 59 || tod.time_hours < 0 || tod.time_hours > 12 || tod.ampm > 2){
    return 1;
  }

  int min_ones = tod.time_mins % 10;  // ones place 
  int min_tens = tod.time_mins / 10; // tens place
  int hrs_ones = tod.time_hours % 10;
  int hrs_tens = tod.time_hours / 10;

  *display = mask[min_ones];// the ones place of minutes is to be added first into display

  if (hrs_tens == 0){ // avoids adding zero in the hours tens place when its displayed
    *display = (mask[min_tens] << 7) | (mask[hrs_ones] << 14) | *display;
  }
  else{
    // shift bits to the left and add in the correct bit pattern
    *display = (mask[min_tens] << 7) | (mask[hrs_ones] << 14) | (mask[hrs_tens] << 21) | *display;
  }

  if (tod.ampm == 1){
    *display = 1 << 28 | *display;
  }
  else{
    *display = 1 << 29 | *display;

  }

  return 0;
}

// Examines the TIME_OF_DAY_PORT global variable to determine hour,
// minute, and am/pm.  Sets the global variable CLOCK_DISPLAY_PORT bits
// to show the proper time.  If TIME_OF_DAY_PORT appears to be in error
// (to large/small) makes no change to CLOCK_DISPLAY_PORT and returns 1
// to indicate an error. Otherwise returns 0 to indicate success.
//
// Makes use of the previous two functions: set_tod_from_ports() and
// set_display_from_tod().
//
// CONSTRAINT: Does not allocate any heap memory as malloc() is NOT
// available on the target microcontroller.  Uses stack and global
// memory only.
int clock_update(){
  tod_t tod;
  int result = set_tod_from_ports(&tod); // call
  if(result ==1){ // if failed return 1
    return 1;
  }
  int ptr=0;
  set_display_from_tod(tod,&ptr); // call
  CLOCK_DISPLAY_PORT = ptr; // sets bits 
  return 0;
}