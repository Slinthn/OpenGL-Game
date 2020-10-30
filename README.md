# OpenGL-Game
A game made with opengl (now with: a program which converts obj files into a better, easier to use format!)

I will hopefully be constantly revising the code so I doubt most things will stay the same.


**note**: don't take this next part for granted. I may have changed the code and forgot to change the info here.

The sobj.exe file you get with build.bat is limiting:
- the file must be called model.obj
- the output file will always be model.sobj
- at least one vertex, texture and normal value must exist
- vertices must only have x y z components, textures only s t, and normals x y z
- faces must all be triangular.

Other than this, the format is:
- header:
  - 4 bytes unsigned for triangleCount (1 triangle = 3 indices)
  - 4 bytes unsigned for vertex count (vertex count = texture count = normal count, when it comes to the format of this file!)
- *3\*triangleCount* 4 byte integer values for all the vertex indices
- *3\*vertexCount* 4 byte float values for vertex data
- *2\*vertexCount* 4 byte float values for texture data
- *3\*vertexCount* 4 byte float values for normal data

If this is confusing, sorry. See *loadSObj(char \*source)* for a more code-y example. sobj.c is the only file for the code for conversion.
