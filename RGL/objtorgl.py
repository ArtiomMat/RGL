import sys
from PIL import Image
import struct

args = sys.argv
argsn = len(sys.argv)

fp = args[1]
f = open(fp, "rb");
lines = f.readlines()

v = []
vt = []
vn = []
# Ordered v/vt/vn
f = []

for line in lines:
  line = line.decode("utf-8")
  args = line.split(" ");
  
  if args[0] == "v":
    x =[float(args[1]), float(args[2]), float(args[3])]
    v.append(x)
  elif args[0] == "vt":
    x = [float(args[1]), float(args[2])]
    vt.append(x)
  elif args[0] == "vn":
    x = [float(args[1]), float(args[2]), float(args[3])]
    vn.append(x)
  elif args[0] == "f":
    x = []
    for i in range(3):
      y = args[i+1].split("/")
      x.append([int(y[0]), int(y[1]), int(y[2])])
    f.append(x)

vertices = [] # Vertex list that contains XYZ triplet lists.
uvcoords = [] # UV coordinates
normals = [] # Same as vertex, but normals, ofc
triangles = [] # Contains 3 indices for the triangles

# Now to make it a normal fucking format omg
trianglei = 0;
for face in f:
  triangles.append([]) # Create a list, that will become a triad of indices
  for element in face:
    vertexi = element[0]-1
    uvcoordi = element[1]-1
    normali = element[2]-1

    found = 0
    # Find if there exist such indices
    for i in range(len(vertices)):
      if vertices[i] == v[vertexi] and normals[i] == vn[normali] and uvcoords[i] == vt[vertexi]:
        triangles[trianglei].append(i)
        found = 1
        break
    if not found:
      vertices.append(v[vertexi])
      normals.append(vn[normali])
      uvcoords.append(vt[uvcoordi])
      triangles[trianglei].append(len(vertices) - 1)

  trianglei+=1

# Open an image
# def get_image_data(image_path):
#     # Open the image using PIL
#     image = Image.open(image_path)

#     # Extract width, height, and pixels
#     width, height = image.size
#     pixels = list(image.getdata())

#     # Flatten the pixel array and convert to RGB format
#     rgb_pixels = [pixel[:3] for pixel in pixels]

#     return width, height, rgb_pixels

# texturew, textureh, texture = get_image_data(sys.argv[2])

def ftos(f):
  return "{:.5f}".format(f)

f = open(sys.argv[1].split(".")[0]+".rg3", "w");

f.write("RG3\n")
f.write(f"VN {len(vertices)}\n")
for i in range(len(vertices)):
  f.write("V "+ftos(vertices[i][0])+" ")
  f.write(ftos(vertices[i][1])+" ")
  f.write(ftos(vertices[i][2])+"\nN ")
  f.write(ftos(normals[i][0])+" ")
  f.write(ftos(normals[i][1])+" ")
  f.write(ftos(normals[i][2])+"\nT ")
  f.write(ftos(uvcoords[i][0])+" ")
  f.write(ftos(uvcoords[i][1])+"\n")

f.write(f"FN {len(triangles)}\n")
for i in range(len(triangles)):
  f.write("F "+str(triangles[i][0])+" ")
  f.write(str(triangles[i][1])+" ")
  f.write(str(triangles[i][2])+"\n")
