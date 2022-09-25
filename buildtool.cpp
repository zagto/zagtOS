#include <iostream>
#include <string>
#include <vector>
#include <optional>
#include <fstream>
#include <filesystem>
#include <unordered_map>
#include <queue>
#include <cassert>
#include <cstdlib>


class Module {
private:
    enum class Type {
        SCRIPTS, MAKE, AUTOTOOLS
    };

    std::filesystem::path srcDir;
    std::filesystem::path buildDir;
    Type type;

    void detectType();
    void compile();
    void install();

public:
    std::string name;
    uint32_t id;
    std::filesystem::file_time_type lastModified;
    std::filesystem::file_time_type lastBuilt;
    std::vector<uint32_t> dependents;
    size_t numDependencies;
    bool dependenciesUpdated;

    Module(std::string moduleName, uint32_t _id);
    void loadDependencies();
    void build();
};

static int ParallelJobs;
static std::string TargetArchitecture;

static std::string ModulesString;
static std::vector<Module> Modules;
static std::unordered_map<std::string, uint32_t> ModulesByName;
static std::vector<uint32_t> BuildOrder;
static std::filesystem::path BuildRoot;

/* for environment variables */
static std::string BuildRootString;
static std::string ParallelJobsString;
static std::string PathString;
static std::string SysrootString;

const char SPACES[] = " \t";


template <typename TP>
std::time_t to_time_t(TP tp)
{
    using namespace std::chrono;
    auto sctp = time_point_cast<system_clock::duration>(tp - TP::clock::now()
              + system_clock::now());
    return system_clock::to_time_t(sctp);
}

std::filesystem::file_time_type lastModifiedDir(std::string path) {
    std::filesystem::file_time_type result = std::filesystem::file_time_type::min();

    for (const std::filesystem::directory_entry& entry:
            std::filesystem::recursive_directory_iterator(path)) {

        std::string filename = entry.path().string();
        if (filename.find(".creator.") == std::string::npos
                && filename.substr(filename.length() - 4) != ".bak"
                && filename.substr(filename.length() - 7) != ".config"
                && filename.substr(filename.length() - 8) != ".creator"
                && filename.substr(filename.length() - 6) != ".files"
                && filename.substr(filename.length() - 9) != ".includes"
                && filename.substr(filename.length() - 7) != ".cflags"
                && filename.substr(filename.length() - 9) != ".cxxflags"
                && filename.substr(0, 1) != ".") {

            result = max(result, entry.last_write_time());
        }
    }
    return result;
}

Module::Module(std::string moduleName, uint32_t _id) :
        name{moduleName},
        id{_id},
        numDependencies{0},
        dependenciesUpdated{false} {
    buildDir = BuildRoot;
    srcDir = BuildRoot;
    buildDir += "/build/" + TargetArchitecture + "/" + name;
    srcDir += "/src/" + name;

    detectType();

    lastModified = lastModifiedDir("src/" + name);

    /* if the module used sources from other directories, it specifies them is this file */
    std::string sourceFileName = "src/" + name + "/source-dirs";
    if (std::filesystem::exists(sourceFileName)) {
        std::ifstream sourceDirsFile(sourceFileName);
        if (!sourceDirsFile.is_open()) {
            std::cerr << "Could not open source-dirs file for module " << name
                      << " (" << sourceFileName << ")." << std::endl;
            exit(1);
        }

        while(sourceDirsFile.good()) {
            std::string sourceDir;
            sourceDirsFile >> sourceDir;

            if (sourceDir == "") {
                continue;
            }

            lastModified = max(lastModified, lastModifiedDir("src/" + sourceDir));
        }
    }

    try {
        lastBuilt = std::filesystem::last_write_time(
                    "build/" + TargetArchitecture + "/" + name + "/timestamp");
    }  catch (std::filesystem::filesystem_error) {
        lastBuilt = std::filesystem::file_time_type::min();
    }

    ModulesByName.insert({name, id});
}


void Module::loadDependencies() {
    std::string fileName = "src/" + name + "/dependencies";
    std::ifstream file(fileName);
    if (!file.is_open()) {
        std::cerr << "Could not open dependencies file for module " << name
                  << " (" << fileName << ")." << std::endl;
        exit(1);
    }

    while(file.good()) {
        std::string entry;
        file >> entry;

        if (entry == "") {
            return;
        }

        auto it = ModulesByName.find(entry);
        if (it == ModulesByName.end()) {
            std::cerr << "Module " << name << " has dependency '" << entry
                      << "' which is not an enabled module." << std::endl;
            exit(1);
        }

        numDependencies++;
        Modules[it->second].dependents.push_back(id);
    }
}


void Module::build() {
    std::string buildDirString = buildDir.string();

    std::filesystem::current_path(srcDir);
    setenv("SHBUILD_BUILD_DIR", buildDirString.c_str(), 1);

    std::filesystem::create_directories(buildDir);

    if (!dependenciesUpdated && lastBuilt > lastModified) {
        std::cout << "buildtool: skipping " << name
                  << "(no changes)" << std::endl;
    } else {
        if (dependenciesUpdated) {
            std::cout << "buildtool: completely rebuilding " << name
                      << " (changed dependencies)" << std::endl;

            try {
                std::filesystem::remove_all(buildDir);
                std::filesystem::create_directories(buildDir);
            }  catch (std::filesystem::filesystem_error) {
                std::cerr << "unable to remove build/" << name << std::endl;
                exit(1);
            }
        } else {
            std::cout << "buildtool: building " << name << " (changes)" << std::endl;
        }

        compile();

        std::filesystem::current_path(srcDir);

        install();

        std::ofstream timestampFile(buildDir.string() + "/timestamp",
                                    std::ios_base::openmode::_S_out | std::ios_base::openmode::_S_trunc);
        assert(timestampFile.is_open());
        timestampFile << "!";
        timestampFile.close();

        for (uint32_t dep: dependents) {
            Modules[dep].dependenciesUpdated = true;
        }
    }


    std::filesystem::current_path(BuildRoot);
    setenv("SHBUILD_BUILD_DIR", "", 1);
}


void Module::detectType() {
    std::filesystem::current_path(srcDir);
    if (std::filesystem::exists("build-script")) {
        type = Type::SCRIPTS;
    } else if (std::filesystem::exists("configure")) {
        type = Type::AUTOTOOLS;
    } else if (std::filesystem::exists("Makefile")) {
        type = Type::MAKE;
    } else {
        std::cerr << "Could not detect type of module " << name << "."
                  << "It has neiter build-script nor Makefile nor configure." << std::endl;
        exit(1);
    }
    std::filesystem::current_path(BuildRoot);
}


void Module::compile() {
    switch (type) {
    case Type::AUTOTOOLS:
        std::filesystem::current_path(buildDir);

        if (system((srcDir.string() + "/configure --host=" + TargetArchitecture + "-zagto-zagtos").c_str())) {
            std::cerr << "Could not run configure script of module " << name << "." << std::endl;
            exit(1);
        }
        /* fall-through */
    case Type::MAKE:
        if (system(("make -j" + ParallelJobsString + " > /dev/null").c_str())) {
            std::cerr << "Could not run Makefile of module " << name << "." << std::endl;
            exit(1);
        }
        break;
    case Type::SCRIPTS:
        if (system("./build-script > /dev/null")) {
            std::cerr << "Could not run build-script of module " << name << "." << std::endl;
            exit(1);
        }
    }
}


void Module::install() {
    std::cout << "buildtool: installing " << name << std::endl;

    switch (type) {
    case Type::AUTOTOOLS:
        std::filesystem::current_path(buildDir);
        if (system(("make install -j" + ParallelJobsString + " DESTDIR=" + BuildRootString).c_str())) {
            std::cerr << "Could not run Makefile Install target of module " << name << "." << std::endl;
            exit(1);
        }
        break;
    case Type::MAKE:
        if (system(("make install -j" + ParallelJobsString + " > /dev/null").c_str())) {
            std::cerr << "Could not run Makefile Install target of module " << name << "." << std::endl;
            exit(1);
        }
        break;
    case Type::SCRIPTS:
        if (system("./install-script > /dev/null")) {
            std::cerr << "Could not run install-script of module " << name << "." << std::endl;
            exit(1);
        }
    }
}


void prepareEnvironment() {
    BuildRoot = std::filesystem::current_path();
    BuildRootString = BuildRoot.string();
    setenv("SHBUILD_ROOT", BuildRootString.c_str(), 1);

    ParallelJobsString = std::to_string(ParallelJobs);
    setenv("PARALLEL_JOBS", ParallelJobsString.c_str(), 1);

    PathString = BuildRootString + "/out/" + TargetArchitecture + "/toolchain/kernel/bin:"
            + BuildRootString + "/out/" + TargetArchitecture + "/toolchain/system/bin:"
            + getenv("PATH");
    setenv("PATH", PathString.c_str(), 1);

    SysrootString = BuildRootString + "/out/" + TargetArchitecture + "/toolchain/sysroot";
    setenv("SYSROOT", SysrootString.c_str(), 1);

    setenv("KERNEL_CFLAGS_x86_64", "-mgeneral-regs-only -mcmodel=large -mno-red-zone -ffixed-r15 -D _ZAGTOS_KERNEL=1", 1);
    setenv("KERNEL_CFLAGS_aarch64", "-mgeneral-regs-only -mcmodel=large -D _ZAGTOS_KERNEL=1", 1);

    setenv("ARCH", TargetArchitecture.c_str(), 1);

    try {
        std::filesystem::remove_all("out/" + TargetArchitecture + "/esp");
    }  catch (std::filesystem::filesystem_error) {
        std::cerr << "unable to clear out/" << std::endl;
        exit(1);
    }
}


void createBootImage() {
    if (system((std::string("./create-boot-image-")
                + TargetArchitecture
                + std::string(".sh")).c_str())) {
        std::cerr << "buildtool: error during create-boot-image-" << TargetArchitecture << ".sh"
                  << std::endl;
        exit(1);
    }
}


void loadModules() {
    size_t position = 0;
    while (true) {
        size_t nameStart = ModulesString.find_first_not_of(SPACES, position);
        if (nameStart == std::string::npos) {
            return;
        }

        size_t nameEnd = ModulesString.find_first_of(SPACES, nameStart);
        if (nameEnd == std::string::npos) {
            nameEnd = ModulesString.size();
        }
        std::string name = ModulesString.substr(nameStart, nameEnd - nameStart);

        try {
            Modules.emplace_back(name, Modules.size());
        }  catch (std::filesystem::filesystem_error) {
            std::cerr << "Could not find module '" << name << "'" << std::endl;
            exit(1);
        }

        position = nameEnd + 1;
    }
}


void solveBuildOrder() {
    std::queue<uint32_t> q;
    for (const Module &mod: Modules) {
        if (mod.numDependencies == 0) {
            q.push(mod.id);
        }
    }

    while (!q.empty()) {
        Module &mod = Modules[q.front()];
        q.pop();

        for (uint32_t id: mod.dependents) {
            Module &other = Modules[id];
            other.numDependencies--;
            if (other.numDependencies == 0) {
                q.push(id);
            }
        }

        BuildOrder.push_back(mod.id);
    }

    bool foundProblem = false;
    for (const Module &mod: Modules) {
        if (mod.numDependencies != 0) {
            if (foundProblem) {
                std::cerr << ", ";
            } else {
                std::cerr << "The following modules have cyclic dependencies: ";
                foundProblem = true;
            }
            std::cerr << mod.name;
        }
    }
    if (foundProblem) {
        std::cerr << "." << std::endl;
        exit(1);
    }
}


void buildModules() {
    for (uint32_t id: BuildOrder) {
        Modules[id].build();
    }
}


void loadConfig() {
    std::ifstream file("config.ini");
    if (!file.is_open()) {
        std::cerr << "Could not open config.ini file" << std::endl;
        exit(1);
    }

    bool modules{false}, architecture{false}, parallelJobs{false};
    size_t lineNumber = 0;


    while (file.good()) {
        std::string line;
        getline(file, line);
        lineNumber++;

        size_t commentStart = line.find_first_of("#;");
        line = line.substr(0, commentStart);

        size_t assignmentPos = line.find('=');
        size_t keyStart = line.find_first_not_of(SPACES, 0);
        size_t keyEnd = line.find_last_not_of(SPACES, assignmentPos - 1);

        if (assignmentPos == std::string::npos) {
            if (keyStart == std::string::npos) {
                /* empty line */
                continue;
            } else {
                std::cerr << "config.ini: " << lineNumber << ": missing '='" << std::endl;
                exit(1);
            }
        } else {
            if (assignmentPos == 0 || keyEnd == std::string::npos) {
                std::cerr << "config.ini: " << lineNumber << ": missing option name" << std::endl;
                exit(1);
            }
        }

        size_t valueStart = line.find_first_not_of(SPACES, assignmentPos + 1);
        size_t valueEnd = line.find_last_not_of(SPACES);
        std::string value;
        if (valueStart == std::string::npos) {
            value = "";
        } else {
            value = line.substr(valueStart, valueEnd + 1 - valueStart);
        }

        std::string key = line.substr(keyStart, keyEnd - keyStart + 1);
        if (key == "PARALLEL_JOBS" && !parallelJobs) {
            try {
                ParallelJobs = std::stoi(value);
            }  catch (...) {
                ParallelJobs = 0;
            }
            if (ParallelJobs <= 0) {
                std::cerr << "config.ini: PARALLEL_JOBS must be a positive integer." << std::endl;
                exit(1);
            }
            parallelJobs = true;
        } else if (key == "ARCH" && !architecture) {
            if (value != "x86_64" && value != "aarch64") {
                std::cerr << "config.ini: ARCH must be one of the following: x86_64, aarch64."
                          << std::endl;
                exit(1);
            }
            TargetArchitecture = value;
            architecture = true;
        } else if (key == "MODULES" && !modules) {
            ModulesString = value;
            modules = true;
        } else {
            std::cerr << "config.ini: " << lineNumber << ": option '" << key
                      << "' is either invalid or already defined." << std::endl;
            exit(1);
        }
    }

    if (!modules || !parallelJobs || !architecture) {
        std::cerr << "config.ini: must set the following options: MODULES, ARCH, PARALLEL_JOBS."
                  << std::endl;
        exit(1);
    }
}


int main(int argc, char **argv) {
    if (argc > 1) {
        std::cerr << "Usage: " << argv[0] << std::endl;
        return 1;
    }
    loadConfig();
    prepareEnvironment();
    loadModules();
    for (auto &mod: Modules) {
        mod.loadDependencies();
    }
    solveBuildOrder();
    buildModules();
    createBootImage();
}
