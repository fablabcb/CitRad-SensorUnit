#ifndef SPECTRUMCOLORIZER_HPP
#define SPECTRUMCOLORIZER_HPP

#include <array>
#include <vector>

namespace gfx
{

class SpectrumColorizer
{
  private:
    class ColorRingbuffer
    {
      public:
        using Color = std::array<float, 3>;

        bool isEmpty() const { return colors.empty(); }
        void advance();
        Color const& get(size_t index) const;

        void setSize(size_t size);
        size_t getSize() const { return colors.size(); }

        std::vector<Color> getColors() const { return colors; }

      private:
        std::vector<Color> colors; // in range [0.0f, 1.0f]
        size_t base = 0;
    };

  public:
    void generateColorsIfRequried(size_t colorCount);

    std::vector<std::array<float, 3>> getColorPalette() const { return colorBuffer.getColors(); }

  private:
    ColorRingbuffer colorBuffer;
};

} // namespace gfx

#endif
