import bpy
import bmesh

def dude_select_vert():
    # indices = [133, 1450, 9807, 9928, 14813, 656, 12, 286, 257, 5853, 7669, 6675, 22524, 5643]
    indices = [ 12, 133, 257, 286, 656, 1450, 5643, 5853, 6675, 9807, 9928, 11111, 11234, 11350, 11390, 11736, 12510, 16636, 16851, 17655, 20663, 20778, 22516, 22524 ]

    obj = bpy.context.object
    me = obj.data
    bm = bmesh.from_edit_mesh(me)

    vertices= [e for e in bm.verts]
    oa = bpy.context.active_object

    for vert in vertices:
        if vert.index in indices:
            vert.select = True
        else:
            vert.select = False

    bmesh.update_edit_mesh(me, True)

dude_select_vert()

# print selected vertices
# import bmesh; [i.index for i in bmesh.from_edit_mesh(bpy.context.active_object.data).verts if i.select]