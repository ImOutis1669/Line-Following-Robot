#  Line Following Robot (Embedded Systems)

##  Overview
This project implements a **line-following robot** using embedded systems and sensor-based navigation.  

Developed as part of a **team-based engineering project**, the system processes real-time input from an IR sensor array to detect a path and adjusts motor behaviour accordingly. It also integrates obstacle detection and manual control, demonstrating a **multi-system embedded design**.

---

##  Features
- Line following using IR sensor array  
- Real-time decision logic for path tracking  
- Ultrasonic obstacle detection with adaptive response  
- PWM motor control for speed and direction  
- Bluetooth (HC-05) manual control mode  
- Simulation in Proteus and hardware implementation  

---

##  System Architecture

The system consists of multiple subsystems working together:

###  Sensing System
- IR sensor array for line detection  
- Ultrasonic sensor for obstacle detection  

###  Control Logic
- Processes sensor input to determine movement  
- Adjusts motor outputs based on line position  
- Implements decision-based navigation logic  

###  Actuation System
- PWM-controlled DC motors  
- Differential drive for steering and movement  

###  Communication System
- Bluetooth (HC-05) module for manual override  

---

##  Control Strategy

- Line position is detected using IR sensors  
- The system applies **rule-based logic** to determine direction  
- Motor speeds are adjusted to maintain alignment with the path  
- Obstacle detection overrides movement when necessary  

---

##  Simulation & Testing

- Developed and tested system logic in **Proteus simulation**  
- Verified sensor behaviour and control response  
- Iteratively refined system performance through testing  

---

##  Hardware Implementation

- Microcontroller (ATmega-based / Arduino)  
- IR sensor array  
- Ultrasonic sensor  
- DC motors with driver module  
- Bluetooth module (HC-05)  

---

##  Team Contribution

This project was developed as part of a **collaborative team effort**, involving shared responsibilities across system design, implementation, and testing.  

My contributions focused on:
- Embedded system implementation and control logic  
- Sensor integration and system behaviour testing  
- Debugging and validating system performance
  
---

##  Key Learnings
- Designing **real-time embedded systems**  
- Working effectively in a **team-based engineering environment**  
- Sensor integration and decision-based control  
- Debugging using **simulation + hardware testing**  
- Integrating multiple subsystems into one platform  

---

##  Future Improvements
- Implement PID control for smoother tracking  
- Add sensor filtering for noise reduction  
- Improve path detection for complex tracks  
- Optimise motor response for better stability  

---

##  Author
**Jeremy Antwi**  
Electrical & Electronic Engineering Student  
Interested in Embedded Systems, FPGA, and Hardware Design  
