#include "SpectrumVisualizer.hpp"
#include "../../sensor/statistics.hpp"

#include <cassert>
#include <iostream>

namespace gfx
{

SpectrumVisualizer::State::State(int windowW, int windowH)
    : windowH(windowH)
    , windowW(windowW)
{
    window = SDL_CreateWindow("render", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, windowW, windowH, 0);

    renderer = SDL_CreateRenderer(window, -1, 0);
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, windowW, windowH);

    try
    {
        ctx = setupCairo(texture, windowW, windowH);
    }
    catch(std::runtime_error const& e)
    {
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
    }
}

cairo_t* SpectrumVisualizer::State::setupCairo(SDL_Texture* texture, int w, int h)
{
    void* pixels = nullptr;
    int pitch;
    if(SDL_LockTexture(texture, nullptr, &pixels, &pitch))
        throw std::runtime_error("Failed to lock texture: " + std::string(SDL_GetError()));

    auto* cairoSurf = cairo_image_surface_create_for_data((unsigned char*)pixels, CAIRO_FORMAT_ARGB32, w, h, pitch);
    if(!cairoSurf)
        throw std::runtime_error("Failed to create cairo surface");

    auto ctx = cairo_create(cairoSurf);
    if(!ctx)
        throw std::runtime_error("Failed to create cairo context");

    cairo_set_line_width(ctx, 1);

    cairo_set_font_size(ctx, 20);

    return ctx;
}

SpectrumVisualizer::State::~State()
{
    if(window == nullptr)
        return;

    cairo_destroy(ctx);

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
}

SpectrumVisualizer::SpectrumVisualizer(int windowW, int windowH)
    : state(windowW, windowH)
{}

void setColor(cairo_t* ctx, Color const& color)
{
    cairo_set_source_rgba(
        ctx,
        static_cast<double>(color.r),
        static_cast<double>(color.g),
        static_cast<double>(color.b),
        static_cast<double>(color.a));
}

void setColor(cairo_t* ctx, std::array<float, 3> const& color)
{
    cairo_set_source_rgb(
        ctx,
        static_cast<double>(color[0]),
        static_cast<double>(color[1]),
        static_cast<double>(color[2]));
}

void SpectrumVisualizer::renderSpectrum(
    std::vector<float> const& data,
    std::vector<std::array<float, 3>> const& colorPalette,
    size_t column,
    float lowerValueRange,
    float upperValueRange)
{
    column = column % state.windowW;

    // divider/eraser
    cairo_set_source_rgb(state.ctx, 0, 0, 0);
    cairo_rectangle(state.ctx, column + 1, 0, 100, state.windowH);
    cairo_fill(state.ctx);

    size_t x = column;
    for(size_t row = 0; row < data.size(); row++)
    {
        size_t colorIndex =
            math::remap(data[row]).from(lowerValueRange, upperValueRange).to(0, colorPalette.size() - 1);
        size_t y = data.size() - row;
        setColor(state.ctx, colorPalette[colorIndex]);
        cairo_rectangle(state.ctx, x, y, 1, 1);
        cairo_fill(state.ctx);
    }
}

void SpectrumVisualizer::drawPoint(size_t column, size_t y, Color const& color, size_t size)
{
    column = column % state.windowW;

    setColor(state.ctx, color);
    cairo_rectangle(state.ctx, column > size / 2 ? column - size / 2 : column, y - size / 2, size, size);
    cairo_fill(state.ctx);
}

void SpectrumVisualizer::drawRectangle(size_t column, size_t y, size_t w, size_t h, Color const& color)
{
    column = column % state.windowW;

    setColor(state.ctx, color);
    cairo_rectangle(state.ctx, column, y, w, h);
    cairo_fill(state.ctx);
}

void SpectrumVisualizer::renderMarker(size_t column, Color const& color, size_t width, size_t height)
{
    column = column % state.windowW;

    if(height == 0)
        height = state.windowH;

    setColor(state.ctx, color);
    cairo_rectangle(state.ctx, column, 0, width, height);
    cairo_fill(state.ctx);
}

void SpectrumVisualizer::renderRegion(size_t startColumn, size_t y, size_t width, Color const& color)
{
    startColumn = startColumn % state.windowW;

    setColor(state.ctx, color);
    cairo_rectangle(state.ctx, startColumn, y, width, 10);
    cairo_fill(state.ctx);

    renderMarker(startColumn, color, 1);
    renderMarker(startColumn + width - 1, color, 1);
}

void SpectrumVisualizer::renderText(size_t startColumn, size_t y, std::string const& text, Color const& color)
{
    startColumn = startColumn % state.windowW;

    setColor(state.ctx, color);
    cairo_move_to(state.ctx, startColumn, y);
    cairo_show_text(state.ctx, text.c_str());
}

void SpectrumVisualizer::clear()
{
    // fill with black
    cairo_set_source_rgba(state.ctx, 0, 0, 0, 1);
    cairo_rectangle(state.ctx, 0, 0, state.windowW, state.windowH);
    cairo_fill(state.ctx);

    cairo_set_source_rgba(state.ctx, 0, 0, 0, 0.0);
    cairo_rectangle(state.ctx, 0, 0, state.windowW, state.windowH);
    cairo_fill(state.ctx);
}

void SpectrumVisualizer::updateOutput()
{
    // upload changes to video memory
    SDL_UnlockTexture(state.texture);
    SDL_UnlockTexture(state.texture);

    if(SDL_RenderCopy(state.renderer, state.texture, nullptr, nullptr))
        std::cout << "Failed to render texture: " << SDL_GetError() << std::endl;

    if(SDL_RenderCopy(state.renderer, state.texture, nullptr, nullptr))
        std::cout << "Failed to render texture: " << SDL_GetError() << std::endl;

    SDL_RenderPresent(state.renderer);
}

} // namespace gfx
