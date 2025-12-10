import torch

ckpt_path = r"./weights/best_93.pt"
ckpt = torch.load(ckpt_path, map_location="cpu", weights_only=False)
print(ckpt['model'].yaml)
