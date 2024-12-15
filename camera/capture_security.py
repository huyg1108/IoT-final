import cv2
import time
import os
from imgurpython import ImgurClient

CLIENT_ID = '8a5ecf01b351950'
CLIENT_SECRET = '74e1f7d2f28513def81de494f914d61ced4a6249'

client = ImgurClient(CLIENT_ID, CLIENT_SECRET)
cap = cv2.VideoCapture(0, cv2.CAP_DSHOW)

path = 'C:/Users/huygi/Downloads/VLCNTT/final/'

if not cap.isOpened():
    exit()

output_dir = path + "security_images/"
if not os.path.exists(output_dir):
    os.makedirs(output_dir)

time.sleep(2)

for i in range(2):
    ret, frame = cap.read()
    if not ret:
        continue    
    filename = os.path.join(output_dir, f"image.jpg")
    cv2.imwrite(filename, frame)

cap.release()
cv2.destroyAllWindows()

image_path = path + "security_images/image.jpg"

response = client.upload_from_path(image_path, anon=True)
print(response['link'])