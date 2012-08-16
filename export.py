import bpy
bpy.ops.import_mesh.stl(filepath='export.stl')
print('import 1')



bpy.ops.import_mesh.stl(filepath='exportLow.stl')

print('import 2')

high = bpy.data.objects['Export']
low = bpy.data.objects['exportLow']

low.name = 'low'
high.name = 'high'

bpy.context.scene.objects.active = high
high.select = 1
bpy.ops.object.shade_smooth()
bpy.ops.object.editmode_toggle()
bpy.ops.wm.context_set_value(data_path="tool_settings.mesh_select_mode",value="(False, False, True)")
bpy.ops.mesh.select_all(action='SELECT')
bpy.ops.mesh.flip_normals() 
bpy.ops.object.editmode_toggle()

high.select = 0
low.select = 1
bpy.context.scene.objects.active = low

bpy.ops.object.modifier_add(type='SMOOTH')
smooth = low.modifiers[0]
smooth.factor=1
smooth.iterations=10

bpy.ops.object.modifier_add(type='DECIMATE')
decimate = low.modifiers[1]
decimate.ratio = .006
print(low.modifiers[1])

bpy.ops.object.modifier_apply(modifier="Smooth")
bpy.ops.object.modifier_apply(modifier="Decimate")
bpy.ops.object.shade_smooth()

bpy.ops.uv.smart_project(angle_limit=2)

#select al  the faces
bpy.ops.object.editmode_toggle()
bpy.ops.wm.context_set_value(data_path="tool_settings.mesh_select_mode",value="(False, False, True)")
bpy.ops.mesh.select_all(action='SELECT')
bpy.ops.mesh.flip_normals() #normals are the wrong direction.

#create the new image

bpy.ops.image.new(name='Normals', width=1024, height=1024)

#crazy hack to assign/ select the uv image...
bpy.data.screens['UV Editing'].areas[1].spaces[0].image = bpy.data.images[0] 

#now jump out of edit mode, this order is important !!! need to create image in edit mode
bpy.ops.object.editmode_toggle()

#select in the rigth order
low.select = 0
high.select = 1
low.select = 1

bpy.ops.object.bake_image()

low.select = 0
high.select = 1
bpy.ops.object.delete()

low.select=1


'''
bpy.ops.object.modifier_add(type='REMESH')

low.modifiers[0].octree_depth=3
low.modifiers[0].scale=.7
print(low.modifiers[0])

bpy.ops.object.modifier_apply(modifier="Remesh")


'''