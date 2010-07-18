attribute vec4 vertex;
uniform mat4 uMVMatrix;
uniform mat4 uPMatrix;
varying vec4 outColor;

void main() {
  outColor = vec4(1,1,0,1);
  gl_Position = uPMatrix * uMVMatrix * vertex;
}
