attribute vec4 vertex;
uniform mat4 modelViewMatrix;
uniform mat4 projectionMatrix;
varying vec4 outColor;

void main() {
  outColor = vec4(1,1,0,1);
  gl_Position = projectionMatrix * modelViewMatrix * vertex;
}
