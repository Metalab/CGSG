#ifndef TEXTURE_H
#define TEXTURE_H

#include <SDL_image.h>
#include <string>
#include <GL/glew.h>

class Texture {
public:
  Texture(const std::string & filename):img(NULL),filename(filename) { }
  ~Texture();
  bool load();
  GLuint id;

protected:
  bool loadSDL();
  bool loadGL();
  GLint format() const;
  inline int w() { return img->w; }
  inline int h() { return img->h; }
  inline void * pixels() { return img->pixels; }
  inline int operator[](unsigned int idx) { return ((Uint8 *)img->pixels)[idx]; }

private:
  SDL_Surface * img;
  std::string filename; 
};

#endif // TEXTURE_H
