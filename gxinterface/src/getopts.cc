#include <getopt.h>  // For getopt_long (POSIX-compliant on Windows via MinGW)
#include <iostream>
#include <filesystem>

// Function prototypes
void add_custom_font(const std::string& font_path);
void display_help();

int main(int argc, char* argv[]) {
    // Option variables
    char interface = '\0';
    std::string font_path;
    bool help_requested = false;

    // Long options
    const option long_options[] = {
        {"interface", required_argument, nullptr, 'u'},
        {"font", required_argument, nullptr, 'f'},
        {"help", no_argument, nullptr, 'h'},
        {nullptr, 0, nullptr, 0}
    };

    // Argument parsing
    int opt;
    while ((opt = getopt_long(argc, argv, "u:f:h", long_options, nullptr)) != -1) {
        switch (opt) {
            case 'u':
                interface = (optarg && *optarg) ? optarg[0] : '\0';
                std::cout << _("Interface graphic mode: ") << interface << std::endl;
                break;

            case 'f':
                font_path = optarg ? optarg : "";
                if (!font_path.empty() && std::filesystem::exists(font_path)) {
                    add_custom_font(font_path);
                } else {
                    std::cerr << "Error: Font file not found: " << font_path << std::endl;
                }
                break;

            case 'h':
                help_requested = true;
                break;

            case '?':
                // Unknown option/argument error
                return 1;

            default:
                std::cerr << "Unexpected error during argument parsing" << std::endl;
                return 1;
        }
    }

    if (help_requested) {
        display_help();
        return 0;
    }

    // Your application logic here
    // ...
}

void display_help() {
    std::cout << "Usage: app [OPTIONS]\n"
              << "Options:\n"
              << "  -u, --interface CHAR   Set graphic interface mode\n"
              << "  -f, --font PATH        Load custom font file\n"
              << "  -h, --help             Show this help message\n";
}

// Your existing implementation
void add_custom_font(const std::string& font_path) {
    // Font loading logic using AddFontResourceEx
}