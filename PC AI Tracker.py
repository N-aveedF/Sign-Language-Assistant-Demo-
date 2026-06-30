import cv2
import urllib.request
import os
import time
import threading
import requests 
import mediapipe as mp
from mediapipe.tasks import python
from mediapipe.tasks.python import vision

# ==========================================
# 1. CONFIGURE YOUR ESP32 HUB IP
# ==========================================
# Run the ESP32 C++ code first, look at the LCD screen for the IP address,
# and paste it here. Keep the "http://" prefix!
ESP32_IP = "http://YOUR_ESP32_HUB_IP"

# Define the 3 signs and their conversational responses 
SIGN_RESPONSES = {
    "Open Palm": "Hello! How are you?",
    "Thumbs Up": "Thank you!",
    "Peace Sign": "Goodbye!"
}

# Networking State
last_sent_sign = None
last_sent_time = 0
SEND_COOLDOWN = 2.0 

def send_to_esp32(sign):
    try:
        requests.post(f"{ESP32_IP}/ai_input", data={"sign": sign}, timeout=1)
        print(f"--> Transmitted to ESP32: {sign}")
    except Exception as e:
        print(f"--> Network Error: Could not reach {ESP32_IP}")

# Setup AI Model
model_path = 'hand_landmarker.task'
if not os.path.exists(model_path):
    urllib.request.urlretrieve("https://storage.googleapis.com/mediapipe-models/hand_landmarker/hand_landmarker/float16/1/hand_landmarker.task", model_path)

base_options = python.BaseOptions(model_asset_path=model_path)
options = vision.HandLandmarkerOptions(
    base_options=base_options, running_mode=vision.RunningMode.IMAGE,
    num_hands=1, min_hand_detection_confidence=0.7)
detector = vision.HandLandmarker.create_from_options(options)

TIP_IDS = [4, 8, 12, 16, 20]
CONNECTIONS = [
    (0,1), (1,2), (2,3), (3,4), (0,5), (5,6), (6,7), (7,8), 
    (5,9), (9,10), (10,11), (11,12), (9,13), (13,14), (14,15), 
    (15,16), (13,17), (0,17), (17,18), (18,19), (19,20)
]

def classify_sign(landmarks):
    fingers = []
    if landmarks[TIP_IDS[0]].x < landmarks[TIP_IDS[0] - 1].x: fingers.append(1)
    else: fingers.append(0)

    for i in range(1, 5):
        if landmarks[TIP_IDS[i]].y < landmarks[TIP_IDS[i] - 2].y: fingers.append(1)
        else: fingers.append(0)

    if fingers == [1, 1, 1, 1, 1]: return "Open Palm"
    elif fingers == [0, 1, 1, 0, 0] or fingers == [1, 1, 1, 0, 0]: return "Peace Sign"
    elif fingers == [1, 0, 0, 0, 0] or (fingers == [0, 0, 0, 0, 0] and landmarks[4].y < landmarks[3].y): return "Thumbs Up"
    
    return "Scanning..."


# ==========================================
# 2. CONFIGURE YOUR CAMERA INPUT
# ==========================================
# Update this with the IP address of your ESP32-CAM
# Example: cap = cv2.VideoCapture("http://192.168.1.55:81/stream")
cap = cv2.VideoCapture("http://YOUR_ESP32_CAM_IP:81/stream")

while cap.isOpened():
    success, frame = cap.read()
    if not success: continue

    frame = cv2.flip(frame, 1)
    height, width, _ = frame.shape
    rgb_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
    mp_image = mp.Image(image_format=mp.ImageFormat.SRGB, data=rgb_frame)
    
    results = detector.detect(mp_image)
    sign_text = "Scanning..."

    if results.hand_landmarks:
        for hand_landmarks in results.hand_landmarks:
            for start_idx, end_idx in CONNECTIONS:
                sx, sy = int(hand_landmarks[start_idx].x * width), int(hand_landmarks[start_idx].y * height)
                ex, ey = int(hand_landmarks[end_idx].x * width), int(hand_landmarks[end_idx].y * height)
                cv2.line(frame, (sx, sy), (ex, ey), (0, 255, 0), 2)
            
            sign_text = classify_sign(hand_landmarks)

            if sign_text in SIGN_RESPONSES:
                current_time = time.time()
                if sign_text != last_sent_sign or (current_time - last_sent_time) > SEND_COOLDOWN:
                    last_sent_sign = sign_text
                    last_sent_time = current_time
                    
                    # Fetch the appropriate conversational response
                    response_phrase = SIGN_RESPONSES[sign_text]
                    threading.Thread(target=send_to_esp32, args=(response_phrase,)).start()

    cv2.putText(frame, f"Sign: {sign_text}", (20, 50), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2, cv2.LINE_AA)
    cv2.imshow('AI Sign Tracker', frame)

    if cv2.waitKey(1) & 0xFF == ord('q'): break

cap.release()
cv2.destroyAllWindows()
