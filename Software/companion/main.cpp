#include "../sensor/SignalAnalyzer.hpp"

#include "../sensor/statistics.hpp"
#include "InputStream.hpp"
#include "gfx/SpectrumColorizer.hpp"
#include "gfx/SpectrumVisualizer.hpp"

#include <chrono>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

void convertToFile(std::string const& inputFileName, std::string const& outputFileName);

size_t const graphsAreaHeight = 400;
float speedConversion = 0;

SignalAnalyzer createSignalAnalyzer(size_t sampleRate);
void renderResults(
    SignalAnalyzer::Results const& results,
    gfx::SpectrumVisualizer& visualizer,
    size_t column,
    SignalAnalyzer::Config const& analyzerConfig,
    std::vector<size_t>& timestamps);

gfx::Color red{1, 0, 0, 1};
gfx::Color sea{0, 1, 1, 0.5};
gfx::Color blue{0, 0, 1, 1};
gfx::Color green{0, 1, 0, 1};
gfx::Color black{0, 0, 0, 1};
gfx::Color white{1, 1, 1, 1};
gfx::Color paleGrey{0.2, 0.2, 0.2, 1};
gfx::Color grey{0.5, 0.5, 0.5, 1};

void tests();

int main(int argc, char* argv[])
{
    tests();

    bool useOldFile = true;
    std::string fileName = useOldFile ? "../../data processing method/data/Nordring/rawdata_2024-3-29_12-8-50.BIN"
                                      : "unit_xy_2019-01-01_00-01-00.bin";

    if(false)
    {
        convertToFile(fileName, "result.bin");
        return 0;
    }

    auto sdlFlags = SDL_INIT_EVENTS | SDL_INIT_VIDEO;
    if(SDL_Init(sdlFlags) != 0)
    {
        SDL_Log("Unable to initialize SDL %s", SDL_GetError());
        return 1;
    }

    std::cout << "Opening " << fileName << std::endl;
    std::unique_ptr<InputStream> input = std::make_unique<FileInputStream>(fileName);
    size_t binCount = input->getBatchSize();
    size_t numberOfBatches = 0;
    speedConversion = 1.0f * input->getSignalSampleRate() / (44.0 * 1024.0);

    gfx::SpectrumVisualizer visualizer(1024, binCount + graphsAreaHeight);
    gfx::SpectrumColorizer colorizer;
    colorizer.generateColorsIfRequried(128);
    auto colorPalette = colorizer.getColorPalette();

    auto signalAnalyzer = createSignalAnalyzer(input->getSignalSampleRate());
    std::vector<float> data(input->getBatchSize());
    SignalAnalyzer::Results analyzerResults;
    std::vector<size_t> timestamps;

    if(not input->getNextBatch(data, analyzerResults.timestamp))
    {
        std::cerr << "Failed to even read the first batch" << std::endl;
        return 1;
    }

    // bei 8910 habe ich eine doppelte Erkennung
    size_t batchesToStartWith = 3000;
    size_t renderSkip = 2000;
    size_t columnToMark = 0;
    bool running = true;

    visualizer.clear();

    while(running)
    {
        SDL_Event event;
        size_t batchesToProcess = batchesToStartWith;
        batchesToStartWith = 0;

        while(SDL_PollEvent(&event))
        {
            if(event.type == SDL_KEYDOWN)
            {
                switch(event.key.keysym.scancode)
                {
                        // fall-through
                    case SDL_SCANCODE_ESCAPE:
                    case SDL_SCANCODE_Q:
                        running = false;
                        break;

                    case SDL_SCANCODE_N:
                        batchesToProcess = 1;
                        break;
                    case SDL_SCANCODE_B:
                        batchesToProcess = 20;
                        break;

                    case SDL_SCANCODE_P:
                    {
                        static auto last = std::numeric_limits<size_t>::max();
                        std::cout << analyzerResults.timestamp - last << " " << analyzerResults.timestamp << std::endl;
                        last = analyzerResults.timestamp;
                    }
                    break;

                    default:
                        break;
                }
            }
            else if(event.type == SDL_MOUSEMOTION || event.type == SDL_MOUSEBUTTONDOWN)
            {
                columnToMark = event.button.button != 0 ? event.motion.x : 0;
            }
        }

        for(size_t i = 0; i < batchesToProcess; i++)
        {
            if(not input->getNextBatch(data, analyzerResults.timestamp))
                break;
            numberOfBatches++;

            size_t xColumn = numberOfBatches - 1;

            analyzerResults.spectrum = data;

            signalAnalyzer.processData(analyzerResults);
            timestamps.push_back(analyzerResults.timestamp);

            if(numberOfBatches < renderSkip)
                continue;

            if(bool renderRawSpectrum = false; renderRawSpectrum)
                visualizer.renderSpectrum(analyzerResults.spectrum, colorPalette, xColumn, -256, 0);
            else
                visualizer.renderSpectrum(analyzerResults.noiseFloorDistance, colorPalette, xColumn, -256, 0);

            renderResults(analyzerResults, visualizer, xColumn, signalAnalyzer.getConfig(), timestamps);
            if(numberOfBatches % 100 == 0)
                visualizer.renderMarker(xColumn - 2, white, 3, 20);
        }

        if(columnToMark != 0)
            visualizer.renderMarker(columnToMark, white, 1);

        {
            auto w = 1.0 * visualizer.getWindowW() / colorPalette.size();
            for(size_t i = 0; i < colorPalette.size(); i++)
            {
                auto const& c = colorPalette[i];
                visualizer.drawRectangle(
                    i * w,
                    visualizer.getWindowH() - graphsAreaHeight,
                    w,
                    20,
                    gfx::Color{c[0], c[1], c[2], 1.0});
            }
        }

        visualizer.updateOutput();

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return 0;
}

void convertToFile(std::string const& inputFileName, std::string const& outputFileName)
{
    std::unique_ptr<InputStream> input = std::make_unique<FileInputStream>(inputFileName);
    std::ofstream resultFile(outputFileName, std::ios::out | std::ios::binary);
    if(!resultFile)
        throw std::runtime_error("Failed to open " + outputFileName);

    size_t binCount = input->getBatchSize();
    std::vector<float> data(binCount);
    size_t numberOfBatches = 0;
    size_t timestamp;

    while(input->getNextBatch(data, timestamp))
    {
        numberOfBatches++;
        resultFile.write((char*)data.data(), 4 * data.size());
    }
    std::cout << "Written " << numberOfBatches << " batches" << std::endl;
}

SignalAnalyzer createSignalAnalyzer(size_t sampleRate)
{
    SignalAnalyzer result;
    SignalAnalyzer::Config config;
    config.fftWidth = 1024;
    config.signalSampleRate = sampleRate;

    result.setup(config);
    return result;
}

void renderResults(
    SignalAnalyzer::Results const& results,
    gfx::SpectrumVisualizer& visualizer,
    size_t column,
    SignalAnalyzer::Config const& analyzerConfig,
    std::vector<size_t>& timestamps)
{
    size_t bY = visualizer.getWindowH() - graphsAreaHeight / 2;
    size_t bY0 = visualizer.getWindowH();

    int xOff = -analyzerConfig.hannWindowN / 2;
    auto remap = [](auto v) { return math::remap(std::max(v, -128.0f)).from(-128, 0).to(0, 300); };
    visualizer.drawPoint(column, bY0 - remap(results.data.meanAmplitudeForCars), blue);

    static float lastCarTriggerSignal = 0.0;
    float carTriggerSignalDiff = results.data.carTriggerSignal - lastCarTriggerSignal;
    lastCarTriggerSignal = results.data.carTriggerSignal;

    visualizer.drawPoint(column + xOff, bY0 - remap(results.data.carTriggerSignal), green);
    visualizer.drawPoint(column + xOff, bY - carTriggerSignalDiff * 100, carTriggerSignalDiff >= 0 ? white : blue);
    visualizer.drawPoint(column + xOff, bY, white.faded(0.5));

    visualizer.drawPoint(
        column,
        results.spectrum.size() / 2 - results.forward.maxAmplitudeBin,
        results.forward.considerAsSignal ? blue : sea,
        3);
    visualizer.drawPoint(
        column,
        results.spectrum.size() / 2 + results.reverse.maxAmplitudeBin,
        results.reverse.considerAsSignal ? blue : sea,
        3);

    for(int i = -100; i <= 100; i += 10)
        visualizer.drawPoint(column, 1.0 * results.spectrum.size() / 2 - i / speedConversion, grey.faded(0.8), 1);

    static size_t yRegionOffset = 0;

    auto renderDetection = [&](auto const& optDetection) {
        if(not optDetection.has_value())
            return;

        SignalAnalyzer::Results::ObjectDetection const& detection = optDetection.value();

        auto getAge = [&]() -> std::optional<size_t> {
            size_t searchEnd = timestamps.size() > 2000 ? timestamps.size() - 2000 : 0;
            for(size_t i = timestamps.size() - 1; i >= searchEnd; i--)
                if(detection.timestamp == timestamps[i])
                    return column - i;
            return {};
        };

        auto optAge = getAge();
        if(optAge.has_value() == false)
            return;

        std::cout << "D" << detection.isForward << " c" << column << " vc" << detection.validSpeedCount << " sc"
                  << detection.speeds.size() << " s" << detection.medianSpeed << " a" << optAge.value_or(99999) << " ts"
                  << (detection.timestamp / 1000) % 1000 << std::endl;

        size_t w = detection.speeds.size();
        size_t x = column + xOff - *optAge - detection.speedMarkerAge - (detection.isForward ? 0 : w);
        visualizer.renderRegion(x, yRegionOffset, w, detection.isForward ? grey : white);
        yRegionOffset += 10;
        if(yRegionOffset > 100)
            yRegionOffset = 0;

        float y = 1.0 * results.spectrum.size() / 2 +
                  (detection.isForward ? -1 : 1) * detection.medianSpeed / speedConversion;
        visualizer.renderText(x, y + 30, std::to_string(detection.medianSpeed).substr(0, 4), white);
        visualizer.drawRectangle(x, y, w, 1, white);
    };

    renderDetection(results.completedForwardDetection);
    renderDetection(results.completedReverseDetection);
}

void tests()
{
    int initialValue = -1;
    RingBuffer<int> buffer(3, initialValue); // *-1,-1,-1
    assert(buffer.get(0) == initialValue);
    assert(buffer.get(100) == initialValue);

    buffer.set_noIncrement(0); // *0,-1,-1
    assert(buffer.get(0) == 0);
    buffer.incrementIndex(); // 0,*-1,-1
    assert(buffer.get(0) == initialValue);

    buffer.add(10); // 0,10,*-1
    assert(buffer.get(1) == 10);
    assert(buffer.get(2) == 0);

    buffer.add(20); // *0,10,20
    buffer.add(20); // 20,*10,20
    assert(buffer.get(0) == 10);
    buffer.add(20); // 20,20,*20
    assert(buffer.get(1) == 20);

    assert(math::remap(3).from(0, 10).to(0, 20) == 6);
    assert(math::remap(3).from(0, 10).to(10, 0) == 7);
    assert(math::remap(3).from(0, 10).to(10, 20) == 13);
    assert(math::remap(13).from(10, 20).to(0, 10) == 3);
    assert(math::remap(13).from(10, 20).to(0, -10) == -3);
    assert(math::remap(13).from(10, 20).to(0, -10) == -3);
    assert(math::remap(3.0).from(2.0, 4.0).to(10, 20.0) == 15.0);
    assert(math::remap(3.0).from(0.0, 10.0).to(-10, 10) == -4.0);
}
