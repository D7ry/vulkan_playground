import os
import subprocess
import sys

for root, dirs, files in os.walk(os.getcwd()):
    for file in files:
        if file.endswith(".vert") or file.endswith(".frag"):
            print("Compiling shader: " + file)
            subprocess.call(["glslc", os.path.join(root, file), "-o", os.path.join(root, file + ".spv")])