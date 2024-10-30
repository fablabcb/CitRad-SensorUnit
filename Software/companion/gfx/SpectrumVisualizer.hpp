#ifndef SPECTRUMVISUALIZER_HPP
#define SPECTRUMVISUALIZER_HPP

#include <SDL2/SDL.h>
#include <cairo/cairo.h>

#include <array>
#include <string>
#include <vector>

namespace gfx
{

struct Color
{
    float r = 1.0;
    float g = 1.0;
    float b = 1.0;
    float a = 1.0;

    Color faded(float da) const
    {
        Color res = *this;
        res.a *= da;
        return res;
    }
};

/**
 * @brief The SpectrumVisualizer class provides rendering capabilities of color data using SDL and cairo.
 */
class SpectrumVisualizer
{
  public:
    struct State
    {
        SDL_Renderer* renderer = nullptr;
        SDL_Texture* texture = nullptr;
        SDL_Window* window = nullptr;
        cairo_t* ctx = nullptr;

        int windowH = 512;
        int windowW = 1024;

        State(int windowW, int windowH);
        ~State();

        State(State const&) = delete;
        State& operator=(State const&) = delete;

        static cairo_t* setupCairo(SDL_Texture* texture, int w, int h);
    };

    struct Options
    {
        double outerMargin = 6;
    };

    // state transform
  public:
    SpectrumVisualizer(int windowW, int windowH);

    void renderSpectrum(
        std::vector<float> const& data,
        std::vector<std::array<float, 3>> const& colorPalette,
        size_t column,
        float lowerValueRange,
        float upperValueRange);
    void drawPoint(size_t column, size_t y, Color const& color, size_t size = 1);
    void drawRectangle(size_t x, size_t y, size_t w, size_t h, Color const& color);
    void renderMarker(size_t column, Color const& color, size_t width, size_t height = 0);
    void renderRegion(size_t startColumn, size_t y, size_t width, Color const& color);
    void renderText(size_t startColumn, size_t y, std::string const& text, Color const& color);

    void clear();
    void updateOutput();

    // get info
  public:
    int getWindowW() const { return state.windowW; }
    int getWindowH() const { return state.windowH; }

    // options
  public:
    Options options;

  private:
    State state;
};

} // namespace gfx

#endif // SPECTRUMVISUALIZER_HPP
