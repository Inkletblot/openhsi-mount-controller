# OpenHSI Camera Mount Controller

This simple arduino sketch pairs with a linear actuator camera mount that was
created for my honours project at Flinders University. The simple code allows
the user to set an approximate time over which they want the controller to
move. When activated the init function will count the number of rotations
between each end stop on the mount and roughly calculate how many rotations
will be required to traverse the distance in the requested time.

Given that the mount is a relatively simple combination of:

* An Arduino Uno 
* A TB6600 stepper motor driver
* And a NEMA 23 stepper motor (23H-D56001Y-32B)
* A trigger switch
* And two end stop limit switches

It is relatively easy to program, and doesn't necessarily need to be used with
this code. I'm simply providing it here as a reference for any future
student/user who may want it.

Check out the `.ino` file for more details on the configuration for the code.
