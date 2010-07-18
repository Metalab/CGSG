attribute vec4 vertex;
attribute vec4 color;
uniform mat4 uMVMatrix;
uniform mat4 uPMatrix;
varying vec4 outColor;

void main() {
  outColor = color;
  gl_Position = uPMatrix * uMVMatrix * vertex;
}
