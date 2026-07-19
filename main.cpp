#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <vector>
#include "sha1/sha1.h"

namespace fs = std::filesystem;

struct PluginData {
    std::string filename, originalHash, cleanedHash, patchFile;
};

enum class Platform { Unknown, Steam, GOG };

std::string SHA1Out(const fs::path& pathIn) {
    std::ifstream fil(pathIn, std::ios::binary);
    std::vector<char> shaBuf(65536);
    SHA1 shaRes;

    while (fil.read(shaBuf.data(), shaBuf.size()) || fil.gcount() > 0) {
        shaRes.addBytes(shaBuf.data(), fil.gcount());
    }

    auto res = shaRes.getDigest();
    char out[41] = {};
    for (int i = 0; i < 20; ++i) {
        sprintf_s(out + (i * 2), 3, "%02x", res[i]);
    }
    free(res);
    return std::string(out);
}

int main(int argc, char* argv[]) {
    std::vector<PluginData> steamPlugins = {
        { "Update.esm",         "06ae741c9e11f765e4f902d1f8813cd6d9860a90", "052fa4563e969ac633ee18b0f64da8e2777bbd87", "Update_Steam.vcdiff" },
        { "HearthFires.esm",    "4951925a1956853bb0dbda3fbf40a874b1eb3a5d", "a7d35871f26b7467ac64c22cf6804db762964a78", "HearthFires_Steam.vcdiff" },
        { "Dragonborn.esm",     "58298dd4d7f98d0af48a4c0bfefcd6026e9f8040", "4df0937bb41986b0dfbfa8782ae8b04e59a301cb", "Dragonborn_Steam.vcdiff" },
        { "Dawnguard.esm",      "06a27b47a3d395df1ee53700dfd16116a7639215", "8989fe77bf1379aaffae50aae469c391d159abad", "Dawnguard_Steam.vcdiff" }
    };

    std::vector<PluginData> gogPlugins = {
        { "Update.esm",         "52daff7cad6164d19c8399e97b753a871e64b568", "eb86bcda893e2e9d0bd789e498d4cd889dcc3951", "Update_GOG.vcdiff" },
        { "HearthFires.esm",    "263ba1429dffd30e4b4e4bf063253c44d1300479", "96f490744ffe0194773e36970019e35363e2adab", "HearthFires_GOG.vcdiff" },
        { "Dragonborn.esm",     "8e493e55cba561cc7ddb959dfb556a6706a8c3c7", "a3bebacbdf230e27ec3122c6be6c553318baf3bb", "Dragonborn_GOG.vcdiff" },
        { "Dawnguard.esm",      "9b19505fbc967e8ba50960e81faf96700cf5f162", "986dc65e1612c9cf95f4687c17ec23c685db3ece", "Dawnguard_GOG.vcdiff" }
    };

    std::vector<PluginData> sharedPlugins = {
        { "ccQDRSSE001-SurvivalMode.esl", "7a1c720d687c58358f7e703d6996901eb35304cf", "27e0f9b404a870e7b583498d140887e29153e300", "ccQDRSSE001-SurvivalMode.vcdiff" },
        { "ccBGSSSE025-AdvDSGS.esm",      "1ea46821e843d9804e5b19962da4d00121efc4ac", "53913cff5e0fc1bd0afa742074efe4a5527bf7ca", "ccBGSSSE025-AdvDSGS.vcdiff" },
        { "ccBGSSSE001-Fish.esm",         "3bb605c3f110702790838c9d13a6490e5d5096a8", "ad170ffb8d675c6556d33b53ad60de913b55cb53", "ccBGSSSE001-Fish.vcdiff" }
    };

    std::cout << "Starting Skyrim ESM Patcher...\nAutodetecting game version...\n\n";

    int steamVanillaCount = 0, steamCleanedCount = 0;
    int gogVanillaCount = 0, gogCleanedCount = 0;
    int missingMainFiles = 0;

    // Check ESMs
    for (size_t i = 0; i < steamPlugins.size(); ++i) {
        const auto& sP = steamPlugins[i];
        const auto& gP = gogPlugins[i];

        if (!fs::exists(sP.filename)) {
            missingMainFiles++;
            continue;
        }

        std::string currentHash = SHA1Out(sP.filename);

        if (currentHash == sP.originalHash) steamVanillaCount++;
        else if (currentHash == sP.cleanedHash) steamCleanedCount++;

        if (currentHash == gP.originalHash) gogVanillaCount++;
        else if (currentHash == gP.cleanedHash) gogCleanedCount++;
    }

    // Wrong folder
    if (missingMainFiles == steamPlugins.size()) {
        std::cout << "ERROR: ESMs are missing from this directory.\n";
        std::cin.get(); return 1;
    }

	// Platform detection
    Platform detectedPlatform = Platform::Unknown;
    std::vector<PluginData>* activePlugins = nullptr;

    bool isSteamPotential = (steamVanillaCount + steamCleanedCount) > 0;
    bool isGogPotential = (gogVanillaCount + gogCleanedCount) > 0;

    if (isSteamPotential && !isGogPotential) {
        detectedPlatform = Platform::Steam;
        activePlugins = &steamPlugins;
        if (steamCleanedCount == steamPlugins.size()) {
            std::cout << "Your Steam ESMs are already cleaned. Checking CC Content...\n";
        }
        else {
            std::cout << "-> Detected: Steam version.\n";
        }
    }
    else if (isGogPotential && !isSteamPotential) {
        detectedPlatform = Platform::GOG;
        activePlugins = &gogPlugins;
        if (gogCleanedCount == gogPlugins.size()) {
            std::cout << "Your GOG ESMs are already cleaned. Checking CC Content...\n";
        }
        else {
            std::cout << "-> Detected: GOG version.\n";
        }
    }
    else if (isSteamPotential && isGogPotential) {
        // Paranoia
        std::cout << "ERROR: Mixed files detected. Some files match Steam, others match GOG.\n";
        std::cin.get(); return 1;
    }
    else {
        std::cout << "ERROR: Game version could not be identified (Unknown file hashes).\n";
        std::cin.get(); return 1;
    }

    // Queue
    std::vector<PluginData> patchQueue;
    if (activePlugins) {
        patchQueue.insert(patchQueue.end(), activePlugins->begin(), activePlugins->end());
    }

    // CC is shared so patch regardless
    patchQueue.insert(patchQueue.end(), sharedPlugins.begin(), sharedPlugins.end());

    fs::path backupDir = "Original ESMs backups";
    bool overallSuccess = true;
    bool workWasDone = false;
    
    // Patching
    for (const auto& plugin : patchQueue) {
        std::cout << "Checking " << plugin.filename << "... ";

        if (!fs::exists(plugin.filename)) {
            std::cout << "MISSING (Skipped)\n";
            overallSuccess = false;
            continue;
        }

        std::string currentHash = SHA1Out(plugin.filename);

        if (currentHash == plugin.cleanedHash) {
            std::cout << "Already cleaned.\n";
        }
        else if (currentHash == plugin.originalHash) {
            if (!fs::exists(plugin.patchFile)) {
                std::cout << "FAILED (Patch delta file missing).\n";
                overallSuccess = false;
                continue;
            }

            std::cout << "Needs patch.\n -> Patching... \n";
            fs::create_directories(backupDir);

            fs::path backupPath = backupDir / plugin.filename;
            fs::rename(plugin.filename, backupPath);

            std::string cmd = "xdelta3.exe -d -vfs \"" + backupPath.string() + "\" \"" + plugin.patchFile + "\" \"" + plugin.filename + "\"";

            if (system(cmd.c_str()) == 0) {
                std::cout << "Success!\n";
                workWasDone = true;
            }
            else {
                std::cout << "FAILED (xdelta3 system failure).\n";
                fs::rename(backupPath, plugin.filename); // Rollback
                overallSuccess = false;
            }
        }
        else {
            std::cout << "UNKNOWN HASH (Skipped)\n";
            overallSuccess = false;
        }
    }

    if (!overallSuccess) {
        std::cout << "\nOperation finished with issues. Tool files will not be deleted.\n";
        std::cin.get();
        return 1;
    }

    if (!workWasDone) {
        std::cout << "\nEverything was already cleaned! No changes made.\n";
        std::cin.get();
        return 0;
    }

    // Clean up vcdiffs
    for (const auto& list : { steamPlugins, gogPlugins, sharedPlugins }) {
        for (const auto& plugin : list) {
            if (fs::exists(plugin.patchFile)) fs::remove(plugin.patchFile);
        }
    }

    std::cout << "\nPatching complete! Press ENTER to wipe patch binaries and exit.";
    std::cin.get();

    fs::remove("xdelta3.exe");

    // Cleanup
    fs::path exePath = fs::absolute(argv[0]);
    std::ofstream batFile("cleanup.bat");
    batFile << "@echo off\n"
        << "timeout /t 1 /nobreak > NUL\n"
        << "del \"" << exePath.string() << "\"\n"
        << "(goto) 2>nul & del \"%~f0\"\n";
    batFile.close();

    system("start /b cleanup.bat");
    return 0;
}