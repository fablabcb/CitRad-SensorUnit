#ifndef GFX_SPECTRUMCOLORIZER_HPP
#define GFX_SPECTRUMCOLORIZER_HPP

#include <array>
#include <vector>

namespace gfx
{
using Color = std::array<float, 3>;

class SpectrumColorizer
{
  public:
    void generateColorsIfRequried(size_t colorCount);

    std::vector<Color> getColorPalette() const { return colors; }

  private:
    std::vector<Color> colors; // in range [0.0f, 1.0f]
};

} // namespace gfx

#endif
