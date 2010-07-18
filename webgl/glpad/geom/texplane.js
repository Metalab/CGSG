attributes.vertex = [
    1.0,  1.0,  0.0,
   -1.0,  1.0,  0.0,
    1.0, -1.0,  0.0,
   -1.0, -1.0,  0.0 ];
attributes.vertex.itemsize = 3;

attributes.texCoord = [
    1.0,  1.0,
    0.0,  1.0,
    1.0,  0.0,
    0.0,  0.0, ];
attributes.texCoord.itemsize = 2;

vbo.mode = gl.TRIANGLE_STRIP;
vbo.numitems = 4;

uniforms.texture = "wood.png"
