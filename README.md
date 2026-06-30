IoT-Based Two-Way Sign Language Understanding & Response hub

An Assistive Technology IoT project that bridges the communication gap between the deaf/hard-of-hearing community and individuals who do not understand sign language.

This system uses a distributed edge-computing architecture to capture hand signs via an ESP32-CAM, process the video feed using AI (MediaPipe/TensorFlow) on a local PC, and broadcast the translated conversational responses back to a physical 16x2 LCD, a mobile Web HUD, and the Blynk IoT Cloud platform in real-time.

Features

Real-time AI Hand Tracking: Utilizes Google MediaPipe for rapid landmark detection and classification.

Distributed Processing: Offloads heavy AI inference to a local machine while keeping the edge nodes (ESP32) lightweight and cost-effective.

Multi-Modal Output: Displays translations on a local I2C LCD, a web-based HUD interface, and the Blynk mobile app simultaneously.

Bypass Strict Firewalls: The central hub uses an AJAX polling web server architecture to ensure the web HUD works even on strict enterprise/university Wi-Fi networks that block WebSocket ports.

Future Expansion Ready: Includes boilerplate code for expanding to dynamic sign language translations using LSTM neural networks.

System Architecture

Vision Node (ESP32-CAM): Captures video and streams frames over Wi-Fi.

AI Inference (PC): Processes frames, classifies the hand sign, and sends HTTP POST requests.

Central Hub (ESP32 DevKit): Receives AI data and acts as a router.

Local Displays: Updates the physical 16x2 LCD and the hosted Web HUD.

Cloud Monitor: Syncs the system status to the Blynk IoT Dashboard.

Hardware Requirements

ESP32 Development Board (Main Hub)

AI-Thinker ESP32-CAM Module (Vision Node)

16x2 I2C LCD Display

5V Power Supply / USB cables

Setup Instructions

Create a Blynk account and set up a new template with two Virtual Pins (V0 for Status, V1 for Detected Sign).

Flash esp32_cam_stream.ino to your ESP32-CAM. Update the Wi-Fi credentials.

Flash esp32_gateway.ino to your ESP32 Main Hub. Update the Wi-Fi credentials and Blynk tokens.

Note the IP addresses printed in the Arduino Serial Monitor for both boards.

Install Python dependencies: pip install opencv-python mediapipe requests numpy

Update main_ai.py with your specific ESP32 IP addresses.

Run python main_ai.py to start the tracking pipeline!

Acknowledgements

Note: Generative AI tools were utilized during the development of this project as a pair-programming assistant to aid in code generation, network debugging, and structural formatting. All hardware assembly, system integration, troubleshooting, and architectural design decisions were executed independently by the project team.
