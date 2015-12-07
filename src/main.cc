#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>
#include <set>
#include <string>
#include <stdexcept>

std::set<std::string> ignore_list = {
    ".git",
};

void BuildIndexHelper(const std::string &path, std::vector<std::string> *index) {
    DIR *d = opendir(path.c_str());
    if (d == nullptr) {
        std::cerr << "Failed to read dir: " << path << " "
            << strerror(errno) << std::endl;
        return;
    }

    while (true) {
        struct dirent ent;
        struct dirent *result;
        int ret = readdir_r(d, &ent, &result);
        if (ret != 0) {
            std::cerr << "Failed to read dir: " << path << " "
                      << strerror(errno) << std::endl;
        }
        if (result == nullptr) {
            break;
        }

        const std::string file(result->d_name);
        if (file == "." || file == "..") {
            continue;
        }
        if (ignore_list.find(file) != ignore_list.end()) {
            continue;
        }

        const std::string full_path = path + "/" + file;

        struct stat sb;
        if (stat(full_path.c_str(), &sb) == -1) {
            std::cerr << "Failed to stat: " << full_path << " "
                      << strerror(errno) << std::endl;
            continue;
        }
        index->push_back(full_path);

        if (S_ISDIR(sb.st_mode)) {
            BuildIndexHelper(full_path, index);
        }
    }
    closedir(d);
}

std::vector<std::string> BuildIndex(const std::string &path) {
    std::vector<std::string> index;
    BuildIndexHelper(path, &index);
    return index;
}

void PrintIndex(const std::vector<std::string> &index) {
    for (const auto &str : index) {
        std::cout << str << std::endl;
    }
}

void PrintFilteredIndex(const std::vector<std::pair<int, std::string> > index) {
    for (const auto &pair : index) {
        std::cout << pair.second << std::endl;
    }
}

int MatchesFilter(const std::string &filter, const std::string &input) {
    size_t off = 0;
    int count = 0;
    for (auto chr : filter) {
        std::size_t loc = input.find(chr, off);
        if (loc == std::string::npos) {
            return -1;
        }
        count += loc - off;
        off = loc + 1;
    }

    return count;
}

std::vector<std::pair<int,std::string> > FilterIndex(const std::string &filter,
        const std::vector<std::string> &index) {
    std::vector<std::pair<int, std::string> > filtered;

    for (const auto &str : index) {
        int score = MatchesFilter(filter, str);
        if (score < 0) {
            continue;
        }
        filtered.emplace_back(score, str);
    }

    return filtered;
}

void SortIndex(std::vector<std::pair<int,std::string> > *index) {
    std::sort(index->begin(), index->end(),
        [](const std::pair<int,std::string> &a, const std::pair<int,std::string> &b) {
        return a.first < b.first;
    });
}

int main(int argc, char **argv) {
    if (argc < 3) {
        std::cout << "Usage: <directory> <filter>" << std::endl;
        exit(EXIT_FAILURE);
    }

    std::string dir(argv[1]);
    std::string filter(argv[2]);

    auto start = std::chrono::system_clock::now();
    std::vector<std::string> index = BuildIndex(dir);
    auto end = std::chrono::system_clock::now();

    std::chrono::duration<double> seconds = end - start;
    std::cout << "Time to build index: " << seconds.count() << std::endl;

    start = std::chrono::system_clock::now();
    std::vector<std::pair<int, std::string> > filtered =
        FilterIndex(filter, index);
    end = std::chrono::system_clock::now();

    seconds = end - start;
    std::cout << "Time to filter index: " << seconds.count() << std::endl;

    start = std::chrono::system_clock::now();
    SortIndex(&filtered);
    end = std::chrono::system_clock::now();

    seconds = end - start;
    std::cout << "Time to sort index: " << seconds.count() << std::endl;

    PrintFilteredIndex(filtered);

    return 0;
}
