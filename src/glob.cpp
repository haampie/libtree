#include <glob.h> // glob(), globfree()
#include <string.h> // memset()

#include <filesystem>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace fs = std::filesystem;

std::vector<fs::path> glob_wrapper(std::string const &pattern) {
    glob_t glob_result;
    memset(&glob_result, 0, sizeof(glob_result));

    int return_value = glob(pattern.c_str(), 0, NULL, &glob_result);
    if (return_value != 0) {
        globfree(&glob_result);
        std::stringstream ss;
        ss << "glob() failed with return_value " << return_value << '\n';
        throw std::runtime_error(ss.str());
    }

    std::vector<fs::path> filenames;
    for(size_t i = 0; i < glob_result.gl_pathc; ++i) {
        filenames.emplace_back(glob_result.gl_pathv[i]);
    }

    globfree(&glob_result);
    return filenames;
}