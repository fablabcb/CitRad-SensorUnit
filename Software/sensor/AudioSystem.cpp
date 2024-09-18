#include "AudioSystem.h"

#include "noise_floor.h"

#include <cstddef> // size_t

AudioSystem::AudioSystem()
    : patchCord1(linein, 0, I_gain, 0)
    , patchCord2(I_gain, 0, fft_IQ1024, 0)
    , patchCord3(linein, 0, Q_mixer, 0)
    , patchCord3b(linein, 1, Q_mixer, 1)
    , patchCord4(Q_mixer, 0, fft_IQ1024, 1)
    , patchCord5(linein, 0, peak1, 0)
    , patchCord6(linein, 0, headphone, 0)
    , patchCord7(linein, 1, headphone, 1)
{}

void AudioSystem::setup(AudioSystem::Config const& config, float maxPedestrianSpeed, float sendMaxSpeed)
{
    this->config = config;

    // Audio connections require memory to work. For more detailed information, see the MemoryAndCpuUsage example
    AudioMemory_F32(400);

    sgtl5000_1.enable();
    sgtl5000_1.inputSelect(config.audio_input);  // AUDIO_INPUT_LINEIN or AUDIO_INPUT_MIC
    sgtl5000_1.micGain(config.mic_gain);         // only relevant if AUDIO_INPUT_MIC is used
    sgtl5000_1.lineInLevel(config.linein_level); // only relevant if AUDIO_INPUT_LINEIN is used
    sgtl5000_1.volume(.5);

    updateIQ(config);

    fft_IQ1024.windowFunction(AudioWindowHanning1024);
    fft_IQ1024.setNAverage(1);
    fft_IQ1024.setOutputType(FFT_DBFS); // FFT_RMS or FFT_POWER or FFT_DBFS
    fft_IQ1024.setXAxis(3);

    speedConversion = 1.0 * (config.sample_rate / config.fftWidth) / 44.0; // conversion from Hz to m/s
    max_pedestrian_bin = maxPedestrianSpeed / speedConversion;             // convert max_pedestrian_speed to bin
    uint16_t rawBinCount = (uint16_t)min(config.fftWidth / 2, sendMaxSpeed / speedConversion);

    // IQ = symetric FFT
    if(config.iq_measurement)
    {
        iq_offset = config.fftWidth / 2;   // new middle point
        numberOfFftBins = rawBinCount * 2; // send both sides; not only one
        maxBinIndex = numberOfFftBins;
        minBinIndex = (config.fftWidth - maxBinIndex);
    }
    else
    {
        iq_offset = 0;
        minBinIndex = 0;
        numberOfFftBins = rawBinCount;
    }
}

void AudioSystem::processData(Results& results)
{
    float* pointer = fft_IQ1024.getData();

    results.maxBinIndex = this->maxBinIndex;
    results.minBinIndex = this->minBinIndex;
    results.max_pedestrian_bin = this->max_pedestrian_bin;
    results.numberOfFftBins = this->numberOfFftBins;

    results.process(pointer, iq_offset, config.noise_floor_distance_threshold, speedConversion);
}

void AudioSystem::Results::process(
    float* pointer, uint16_t iq_offset, float noiseFloorDistanceThreshold, float speedConversion)
{
    for(size_t kk = 0; kk < 1024; kk++)
        spectrum[kk] = *(pointer + kk);

    int smooth_n = 1000; // number of samples used for smoothing the spectrum
    for(size_t i = 0; i < 1024; i++)
        spectrum_smoothed[i] = ((smooth_n - 1) * spectrum_smoothed[i] + spectrum[i]) / smooth_n;

    // TODO FIX ME - this was active
    // global_noiseFloor = spectrum_smoothed; // WHAT? noise_floor is const!

    // detect highest frequency
    amplitudeMax = -9999.0;
    max_freq_Index = 0;
    max_freq_Index_reverse = 0;
    mean_amplitude = 0.0;
    mean_amplitude_reverse = 0.0;
    pedestrian_amplitude = 0.0;
    bins_with_signal = 0;
    bins_with_signal_reverse = 0;

    // detect pedestrian
    for(size_t i = 3 + iq_offset; i < max_pedestrian_bin + iq_offset; i++)
    {
        pedestrian_amplitude = pedestrian_amplitude + spectrum[i];
    }
    pedestrian_amplitude = pedestrian_amplitude / max_pedestrian_bin;

    for(size_t i = 0; i < 1024; i++)
    {
        noise_floor_distance[i] = spectrum[i] - global_noiseFloor[i];
    }

    for(size_t i = (max_pedestrian_bin + 1 + iq_offset); i < maxBinIndex; i++)
    {
        mean_amplitude = mean_amplitude + noise_floor_distance[i];
        if(noise_floor_distance[i] > noiseFloorDistanceThreshold)
            bins_with_signal++;

        mean_amplitude_reverse = mean_amplitude_reverse + noise_floor_distance[1024 - i];
        if(noise_floor_distance[1024 - i] > noiseFloorDistanceThreshold)
            bins_with_signal_reverse++;

        // with noise_floor_distance[i] > noise_floor_distance[1024-i] make shure that the signal is in the right
        // direction
        if(noise_floor_distance[i] > noise_floor_distance[1024 - i] && noise_floor_distance[i] > amplitudeMax)
        {
            amplitudeMax = noise_floor_distance[i]; // remember highest amplitude
            max_freq_Index = i;                     // remember frequency index
        }
        if(noise_floor_distance[1024 - i] > noise_floor_distance[i] && noise_floor_distance[1024 - i] > amplitudeMax)
        {
            amplitudeMaxReverse = noise_floor_distance[1024 - i]; // remember highest amplitude
            max_freq_Index_reverse = i;                           // remember frequency index
        }
    }
    detected_speed = (max_freq_Index - iq_offset) * speedConversion;
    detected_speed_reverse = (max_freq_Index_reverse - iq_offset) * speedConversion;

    mean_amplitude =
        mean_amplitude /
        (maxBinIndex - (max_pedestrian_bin + 1 + iq_offset)); // TODO: is this valid when working with dB values?
    mean_amplitude_reverse =
        mean_amplitude_reverse /
        (maxBinIndex - (max_pedestrian_bin + 1 + iq_offset)); // TODO: is this valid when working with dB values?
}

bool AudioSystem::hasData()
{
    return fft_IQ1024.available();
}

void AudioSystem::updateIQ(Config const& config)
{
    // after https://www.faculty.ece.vt.edu/swe/argus/iqbal.pdf
    float A = 1 / config.alpha;
    float C = -sin(config.psi) / (config.alpha * cos(config.psi));
    float D = 1 / cos(config.psi);

    I_gain.setGain(A);
    Q_mixer.gain(0, C);
    Q_mixer.gain(1, D);
}

AudioSystem::Config& AudioSystem::Config::operator=(const Config& other)
{
    mic_gain = other.mic_gain;
    alpha = other.alpha;
    psi = other.psi;

    return *this;
}
