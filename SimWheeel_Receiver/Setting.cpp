#include "Lib_Simwheel.h"

using json = nlohmann::json;

// --- A struct to hold the driver settings ---
// This provides clear, descriptive names for the returned values.


// --- THIS FUNCTION NOW HANDLES EVERYTHING ---
// It checks if the settings file exists, creates it if not, then reads it
// and returns the settings inside the new Settings struct.
Settings getSettings() {
    const std::string filename = "settings.json";

    // --- Step 1: Check if the settings file exists ---
    std::ifstream check_file(filename);
    if (!check_file.is_open()) {
        // The file does not exist, so create it with default values.
        std::cout << "File '" << filename << "' not found. Creating a new one with default settings." << std::endl;
        std::ofstream create_file(filename);
        create_file << R"({
  // IF YOU MESSED UP THE SETTINGS FILE, DELETE IT AND RUN THE PROGRAM AGAIN

  // IMPORTANT: If you change the steering range, you must also change it
  // in both the GAME and APP settings to match the new range,
  // otherwise the steering will not work properly.

  // Default steering range (minimum: 90, maximum: 2520)
  "steering_range_default": 0,

  // Enable or disable logging of control data
  "is_log": true
})";
        create_file.close();
    }
    // The check_file object goes out of scope and closes here.


    // --- Step 2: Read and parse the settings.json file ---
    // At this point, the file is guaranteed to exist.
    try {
        // Open the file for reading.
        std::ifstream settings_file(filename);
        if (!settings_file.is_open()) {
            // This should ideally not happen since we just checked/created the file.
            std::cerr << "Error: Could not open settings.json for reading." << std::endl;
            // Return default values on error
            return { 0, true };
        }

        // Parse the file stream into a json object.
       // allows comments
        json settings_json = json::parse(settings_file, nullptr, true, true);


        // --- Step 3: Extract the values 
        // From that object, we get the values. If the keys don't exist, use the default 'true'.
        double steering = settings_json.value("steering_range_default", 0);
        bool log = settings_json.value("is_log", true);

        // Return the two values packaged in our new struct.
        return { steering, log };

    }
    catch (json::parse_error& e) {
        // Catch parsing errors (e.g., malformed JSON).
        std::cerr << "JSON Parse Error in " << filename << ": " << e.what() << '\n'
            << "exception id: " << e.id << '\n'
            << "byte position of error: " << e.byte << std::endl;
    }
    catch (json::exception& e) {
        // Catch other library exceptions.
        std::cerr << "JSON general exception: " << e.what() << std::endl;
    }

    // In case of any exception, return the default values.
    return { 900.0, true };
}

double userSteering() {
    double userRange = 900.0; // Default
    std::string input;

    std::cout << "Enter steering range (min 90, max 2520), or press Enter for default (900) :";
    std::getline(std::cin, input);

    if (!input.empty()) {
        try {
            double tempRange = std::stod(input);
            if (tempRange >= 90.0 && tempRange <= 2520.0) {
                userRange = tempRange;
            }
            else {
                std::cerr << "Range too low or high. Using default (900).\n";
            }
        }
        catch (...) {
            std::cerr << "Invalid input. Using default (900).\n";
        }
    }

    std::cout << "Using steering range: " << userRange << "\n";

    return userRange;
}