# inform about missing libraries potentially
try:
	from structs import Mesh
	from structs import CURRENT_VERSION
	from structs import TexXYFilterMode
	from structs import TexMipFilterMode
	from structs import TexAnisoRatio
	from structs import TexWrapMode
	#from structs import Texture
	from structs import RenderFlags
	from structs import CompareFunc
	from structs import CullingMode
	from structs import BlendFactor
	from structs import BlendEquation
	from structs import StencilOp
	from structs import PolygonMode
	from structs import Material
	from structs import Model
except ImportError as e:
	print('\033[91mNOTE: copy all of the files from rio/tools/ModelCreator from MY rio fork (ariankordi/rio) to this folder before running pack_body.py\033[0m')
	raise e

#from obj_reader import read_obj
from gltf_reader import read_gltf
from packer import pack

import sys, os

# NOTE: NOTE: IN ORDER TO USE THIS, YOU NEED...
# * to run FFLResource.py from my or aboood40091's ffl repo, run it on FFLBodyRes.dat
# FFLBodyRes.dat is found in NWF titles with Miis. I mention some in nwf-mii-cemu-toy's readme.
# https://github.com/ariankordi/nwf-mii-cemu-toy/blob/master/README.md
# Specify the "_shape" path in argv[1].
# Then put the resulting mii_static_body* files in fs/content/models.

fflbodyres_shape_path = sys.argv[1]

vertices, indices = read_gltf(os.path.join(fflbodyres_shape_path, "Nose_3.glb"))

mesh = Mesh()
mesh.vertices = vertices
mesh.indices = indices
mesh.materialIdx = 0

model = Model()
model.meshes = [mesh]
model.materials = []

dataLE = pack(model, '<')
with open("mii_static_body0_LE.rmdl", "wb") as outf:
    outf.write(dataLE)

dataBE = pack(model, '>')
with open("mii_static_body0_BE.rmdl", "wb") as outf:
    outf.write(dataBE)

vertices, indices = read_gltf(os.path.join(fflbodyres_shape_path, "Nose_4.glb"))

mesh = Mesh()
mesh.vertices = vertices
mesh.indices = indices
mesh.materialIdx = 0

model = Model()
model.meshes = [mesh]
model.materials = []

dataLE = pack(model, '<')
with open("mii_static_body1_LE.rmdl", "wb") as outf:
    outf.write(dataLE)

dataBE = pack(model, '>')
with open("mii_static_body1_BE.rmdl", "wb") as outf:
    outf.write(dataBE)
