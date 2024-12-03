#include <iostream>
#include <vector>
#include <string>
#include <limits>
#include <algorithm>

// Amplifier struct
struct Amplifier {
    std::string name;
    double gain1GHz_min, gain1GHz_typ, gain1GHz_max;
    double gain20GHz_min, gain20GHz_typ, gain20GHz_max;
    double p1dB; // Output power limit
    double cost;
};

// Switch struct
struct Switch {
    std::string name;
    double gainOn1GHz, gainOn20GHz;
    double gainOff1GHz, gainOff20GHz;
    double p1dB; // Input power handling capability
    double cost;
};

// Fixed parameters
const double FIXED_ATTENUATOR_GAIN = -1.0; // Gain for fixed attenuator (dB)
const double POWER_DIVIDER_GAIN_1GHz = -6.0; // Divider attenuation at 1 GHz
const double POWER_DIVIDER_GAIN_20GHz = -7.0; // Divider attenuation at 20 GHz
const double INPUT_POWER = 10.0; // Input power in dBm
const double REQUIRED_MAX_POWER_1GHz = 19.0; // Target max power at 1 GHz
const double REQUIRED_MAX_POWER_20GHz = 16.0; // Target max power at 20 GHz
const double REQUIRED_MIN_LEAKAGE_1GHz = -145.0; // Min leakage in dBm at 1 GHz
const double REQUIRED_MIN_LEAKAGE_20GHz = -137.0; // Min leakage in dBm at 20 GHz

// Amplifier candidates
std::vector<Amplifier> amplifiers = {
    {"Amp-A", 15.0, 17.0, 19.0, 14.0, 16.0, 18.0, 20.0, 20.0},
    {"Amp-B", 12.0, 14.0, 15.0, 8.0, 11.0, 12.0, 26.0, 60.0},
    {"Amp-C", 8.0, 10.0, 12.0, 8.0, 10.0, 12.0, 21.0, 21.0},
    {"Amp-D", 12.0, 14.0, 15.0, 13.0, 15.0, 16.0, 29.0, 77.0},
    {"Amp-E", 14.0, 15.0, 17.5, 13.0, 15.0, 17.5, 20.0, 17.5},
    {"Amp-F", 13.5, 14.5, 15.0, 14.0, 15.0, 16.0, 11.0, 33.0}
};

// Switch candidates
std::vector<Switch> switches = {
    {"SW-A", -0.7, -1.4, -65.0, -55.0, 28.0, 45.0},
    {"SW-B", -0.1, -2.0, -45.0, -20.0, 27.0, 19.0},
    {"SW-C", -1.3, -1.8, -60.0, -35.0, 35.0, 13.0},
    {"SW-D", -0.8, -1.6, -65.0, -45.0, 28.0, 35.0},
    {"SW-E", -1.5, -2.5, -60.0, -40.0, 27.5, 24.0},
    {"SW-F", -1.1, -1.5, -60.0, -34.0, 27.5, 22.0}
};

// Function to select the best amplifier dynamically
Amplifier selectAmplifier() {
    Amplifier bestAmp;
    double bestScore = std::numeric_limits<double>::max();

    for (const auto& amp : amplifiers) {
        double score = 0.5 * (1 / amp.gain1GHz_typ + 1 / amp.gain20GHz_typ) + 0.3 * (1 / amp.p1dB) + 0.2 * amp.cost;
        if (score < bestScore) {
            bestScore = score;
            bestAmp = amp;
        }
    }
    return bestAmp;
}

// Function to select the best switch dynamically
Switch selectSwitch(double inputPower, double ampGain1GHz, double ampGain20GHz) {
    Switch bestSwitch = {"", 0.0, 0.0, 0.0, 0.0, 0.0, std::numeric_limits<double>::max()};
    for (const auto& sw : switches) {
        if (inputPower + ampGain1GHz + sw.gainOn1GHz >= REQUIRED_MAX_POWER_1GHz &&
            inputPower + ampGain20GHz + sw.gainOn20GHz >= REQUIRED_MAX_POWER_20GHz &&
            sw.p1dB >= inputPower) {
            if (sw.cost < bestSwitch.cost) {
                bestSwitch = sw;
            }
        }
    }
    return bestSwitch;
}

// Function to calculate attenuation dynamically
double calculateAttenuation(double inputPower, double targetPower, double amplifierGain) {
    double requiredAttenuation = inputPower + amplifierGain - targetPower;
    return std::max(0.0, requiredAttenuation); // Ensure attenuation is not negative
}

int main() {
    // Select the best amplifier dynamically
    Amplifier bestAmp = selectAmplifier();

    // Variables for worst-case gain scenarios
    double ampGain1GHz_min = bestAmp.gain1GHz_min;
    double ampGain20GHz_min = bestAmp.gain20GHz_min;
    double ampGain1GHz_max = bestAmp.gain1GHz_max;
    double ampGain20GHz_max = bestAmp.gain20GHz_max;

    // Select the best switch dynamically
    Switch selectedSwitch = selectSwitch(INPUT_POWER, ampGain1GHz_min, ampGain20GHz_min);

    if (selectedSwitch.name.empty()) {
        std::cerr << "No suitable switch found!" << std::endl;
        return 1;
    }

    // Calculate attenuation for max power case
    double attenuation1GHz_maxPower = calculateAttenuation(INPUT_POWER, bestAmp.p1dB, ampGain1GHz_min);
    double attenuation20GHz_maxPower = calculateAttenuation(INPUT_POWER, bestAmp.p1dB, ampGain20GHz_min);

    // Calculate total gain and output power for max power case
    double totalGain1GHz_maxPower = ampGain1GHz_min + selectedSwitch.gainOn1GHz + FIXED_ATTENUATOR_GAIN + POWER_DIVIDER_GAIN_1GHz - attenuation1GHz_maxPower;
    double totalGain20GHz_maxPower = ampGain20GHz_min + selectedSwitch.gainOn20GHz + FIXED_ATTENUATOR_GAIN + POWER_DIVIDER_GAIN_20GHz - attenuation20GHz_maxPower;

    double outputPower1GHz_maxPower = INPUT_POWER + totalGain1GHz_maxPower;
    double outputPower20GHz_maxPower = INPUT_POWER + totalGain20GHz_maxPower;

    // Calculate leakage for worst-case minimum gain
    double leakage1GHz = INPUT_POWER + ampGain1GHz_max + selectedSwitch.gainOff1GHz + FIXED_ATTENUATOR_GAIN + POWER_DIVIDER_GAIN_1GHz;
    double leakage20GHz = INPUT_POWER + ampGain20GHz_max + selectedSwitch.gainOff20GHz + FIXED_ATTENUATOR_GAIN + POWER_DIVIDER_GAIN_20GHz;

    // Display results
    std::cout << "Selected Amplifier: " << bestAmp.name << " (Cost: $" << bestAmp.cost << ")" << std::endl;
    std::cout << "Selected Switch: " << selectedSwitch.name << " (Cost: $" << selectedSwitch.cost << ")" << std::endl;
    std::cout << "Max Power Output at 1 GHz: " << outputPower1GHz_maxPower << " dBm" << std::endl;
    std::cout << "Max Power Output at 20 GHz: " << outputPower20GHz_maxPower << " dBm" << std::endl;
    std::cout << "Leakage at 1 GHz: " << leakage1GHz << " dBm" << std::endl;
    std::cout << "Leakage at 20 GHz: " << leakage20GHz << " dBm" << std::endl;

    // Validate against requirements
    if (outputPower1GHz_maxPower >= REQUIRED_MAX_POWER_1GHz &&
        outputPower20GHz_maxPower >= REQUIRED_MAX_POWER_20GHz &&
        leakage1GHz <= REQUIRED_MIN_LEAKAGE_1GHz &&
        leakage20GHz <= REQUIRED_MIN_LEAKAGE_20GHz) {
        std::cout << "System meets specifications!" << std::endl;
    } else {
        std::cerr << "System failed to meet specifications!" << std::endl;
    }

    return 0;
}
