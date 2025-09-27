import numpy as np
from PIL import Image
import matplotlib.pyplot as plt
from os.path import join
import subprocess
import os

#_____________________________________________________________________________________
# Load the image in grayscale
img = Image.open(join("images", "image_flag.png")).convert("L")
img_array = np.array(img)  # 2D array (height x width)

shape = np.shape(img_array)

# Save the raw data into a .bin file
img_array.tofile("image_input.bin")

# Also save the dimensions so the C program knows them
with open("image_meta.txt", "w") as meta:
    meta.write(f"{shape[0]} {shape[1]}")

# Run the C program with input/output files
subprocess.run(["compress.exe", "image_input.bin", "image_meta.txt", "image_compressed.bin"])

# Get file sizes
size_raw = os.path.getsize("image_input.bin")
size_compressed = os.path.getsize("image_compressed.bin")

print(f"Raw size       : {size_raw} bytes")
print(f"Compressed size: {size_compressed} bytes")
print(f"Compression ratio: {size_compressed/size_raw:.2f}")

# Decompression:
subprocess.run(["decompress.exe", "image_compressed.bin", "image_decompressed.bin"])

# Load the original image
orig = np.fromfile("image_input.bin", dtype=np.uint8).reshape(shape)

# Load the decompressed image
decomp = np.fromfile("image_decompressed.bin", dtype=np.uint8).reshape(shape)

# Side-by-side display
plt.figure(figsize=(10,5))

plt.subplot(1,2,1)
plt.title("Original")
plt.imshow(orig, cmap="gray")
plt.axis("off")

plt.subplot(1,2,2)
plt.title("Decompressed")
plt.imshow(decomp, cmap="gray")
plt.axis("off")

plt.show()