attribute vec4 vertex;
attribute vec2 texCoord;

uniform mat4 modelViewMatrix;
uniform mat4 projectionMatrix;

varying vec2 vTextureCoord;

void main(void) {
  gl_Position = projectionMatrix * modelViewMatrix * vertex;
  vTextureCoord = texCoord;
}
