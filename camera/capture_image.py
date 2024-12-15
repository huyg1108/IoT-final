import cv2
import time
import os
cap = cv2.VideoCapture(0, cv2.CAP_DSHOW)

path = 'C:/Users/huygi/Downloads/VLCNTT/final/'

if not cap.isOpened():
    print("Error: Could not open camera.")
    exit()

output_dir = path + "captured_images/"
if not os.path.exists(output_dir):
    os.makedirs(output_dir)
time.sleep(2)

for i in range(10):
    ret, frame = cap.read()
    if not ret:
        print(f"Error: Could not capture image {i + 1}.")
        continue    
    filename = os.path.join(output_dir, f"image_{i + 1}.jpg")
    cv2.imwrite(filename, frame)
    print(f"Captured image {i + 1}: {filename}")
    
cap.release()
cv2.destroyAllWindows()