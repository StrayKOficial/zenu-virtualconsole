#ifndef GPU_HPP
#define GPU_HPP

#include <SDL2/SDL.h>
#include <vector>
#include <cstdint>
#include <string>

class GPU {
public:
    GPU();
    ~GPU();

    bool init();
    void update();
    void render(uint8_t* vram);
    void cleanup();
    void set_title(const std::string& title) {
        if (window) SDL_SetWindowTitle(window, title.c_str());
    }

    // 3D Primitives
    void draw_line(int x1, int y1, int x2, int y2, uint32_t color);
    void draw_triangle(int x1, int y1, int x2, int y2, int x3, int y3, uint32_t color);

private:
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
    
    uint32_t screen[160 * 144];
    static constexpr int WIDTH = 160;
    static constexpr int HEIGHT = 144;
};

#endif
