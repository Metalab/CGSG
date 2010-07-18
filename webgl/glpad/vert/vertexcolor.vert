attribute vec4 vertex;
attribute vec4 color;
uniform mat4 modelViewMatrix;
uniform mat4 projectionMatrix;
varying vec4 outColor;

void main() {
  outColor = color;
  gl_Position = projectionMatrix * modelViewMatrix * vertex;
}
