import sys

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

vertices = []
uvcoords = []
normals = []
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
      if vertices[i] == v[vertexi]:
        # print (vertices[i], v[vertexi])
        triangles[trianglei].append(i)
        found = 1
        break
    if not found:
      vertices.append(v[vertexi])
      normals.append(vn[normali])
      uvcoords.append(vt[uvcoordi])
      triangles[trianglei].append(len(vertices) - 1)

  trianglei+=1

print(len(vertices))
print(len(uvcoords))
print(len(normals))
print(vertices)
print(triangles)
