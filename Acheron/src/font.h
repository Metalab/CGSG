#ifndef __FONT_H
#define __FONT_H

#include <string.h>
#include <vector>
#include <iostream>
#include <iomanip>
#include <SDL.h>
#include <SDL_ttf.h>

using namespace std;

template <typename T>
struct Point { T x, y; Point(T x, T y):x(x),y(y){}};


class Font {

    TTF_Font *font;
    vector<Point<int> > chars[256];


    public:
    Font(char *filename, int pointSize) {
        font = TTF_OpenFont(filename, pointSize);
        if (!font)
            printf("font mag ned: %s", TTF_GetError());
    }

    ~Font() {
        TTF_CloseFont(font);
    }

    vector<Point<int> > GetChar(char c) {
        if (chars[(int)c].size() == 0)
            PopulateCharacter(c);

        return chars[(int)c];
    }

    private:
    void PopulateCharacter(char c) {
        //render the character
        char string[2];

        string[0] = c;
        string[1] = 0;

        SDL_Color white = {255,255,255,255};


        SDL_Surface *img = TTF_RenderText_Solid(font, string, white);

        //get the coordinates
        SDL_LockSurface(img);

        char *pixels = (char*)img->pixels;
        int extraPerRow = img->pitch - img->w;

        for (int i=0; i < img->h; i++) {
            for (int j=0; j < img->w; j++) {
                if (*(pixels++) != 0) {
                    chars[(int)c].push_back(Point<int>(j,i));
                }
            }
            pixels+=extraPerRow;
        }

        SDL_UnlockSurface(img);
        SDL_FreeSurface(img);
    }
};

#endif
