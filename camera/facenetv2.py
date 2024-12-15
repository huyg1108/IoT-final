from facenet_pytorch import MTCNN, InceptionResnetV1
import torch
from torch.utils.data import DataLoader
from torchvision import datasets
import numpy as np
import pandas as pd
import os
from PIL import Image
from imgurpython import ImgurClient

CLIENT_ID = '8a5ecf01b351950'
CLIENT_SECRET = '74e1f7d2f28513def81de494f914d61ced4a6249'

database_dir = 'C:/Users/huygi/Downloads/VLCNTT/final/database/'
image_dir = 'C:/Users/huygi/Downloads/VLCNTT/final/captured_images/'

device = torch.device('cuda:0' if torch.cuda.is_available() else "cpu")
DISTANCE_THRESHOLD = 1.1

def collate_fn(x):
    return x[0]

def prepare_dataset(folder_path, mtcnn, resnet):
    # load dataset and prepare embeddings
    dataset = datasets.ImageFolder(folder_path)
    dataset.idx_to_class = {i: c for c, i in dataset.class_to_idx.items()}
    loader = DataLoader(dataset, collate_fn=lambda x: x[0])

    aligned, names = [], []
    for img, label in loader:
        img_aligned, _ = mtcnn(img, return_prob=True)
        if img_aligned is not None:
            aligned.append(img_aligned)
            names.append(dataset.idx_to_class[label])

    embeddings = resnet(torch.stack(aligned)).detach().cpu()
    return embeddings, names

def calculate_distance(emb1, emb2):
    return (emb1 - emb2).norm()

def who_is_it(image_path, database, model):
    # read all image in folder and predict
    identity = 'Stranger'
    link = ""
    for file in os.listdir(image_path):
        img = Image.open(image_path + file)
        img_cropped = mtcnn(img)
        if img_cropped is not None:
            img_embedding = model(img_cropped.unsqueeze(0)).detach().cpu()
            min_dist = 100
            identity = None
            for i in range(len(database)):
                dist = calculate_distance(database[i], img_embedding)
                if dist < min_dist and dist < DISTANCE_THRESHOLD:
                    min_dist = dist
                    identity = names[i]
                    link = file
                    
    return identity, link

if __name__ == "__main__":
    client = ImgurClient(CLIENT_ID, CLIENT_SECRET)

    mtcnn = MTCNN(
        image_size=160, margin=0, min_face_size=20,
        thresholds=[0.6, 0.7, 0.7], factor=0.709, post_process=True,
        device=device
    )

    resnet = InceptionResnetV1(pretrained='vggface2').eval().to(device)

    embeddings, names = prepare_dataset(database_dir, mtcnn, resnet)
                    
    identity, file_path = who_is_it(image_dir ,embeddings, resnet)
    if identity != "Stranger":
        response = client.upload_from_path(image_dir + file_path, anon=True)
        print(identity)
        print(response['link'])
    else:
        print("Stranger")
        print("NaN")