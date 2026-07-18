#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <vector>
#include <cassert>
#include "sha1/sha1.h"

namespace fs = std::filesystem;

struct PluginData {
    std::string filename, originalHash, cleanedHash, patchFile;
};

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
        sprintf_s(out + (i * 2), 3, "%02X", res[i]);
    }
    free(res);
    return std::string(out);
}

int main(int argc, char* argv[]) {
    std::vector<PluginData> plugins = {
        { "Update.esm",                  "06AE741C9E11F765E4F902D1F8813CD6D9860A90", "3AA35200DBE4C8DE8CF669FDBE8E598D7B567AF2", "Update_Patch.vcdiff" },
        { "HearthFires.esm",             "4951925A1956853BB0DBDA3FBF40A874B1EB3A5D", "368EBE38160F16B8F6c4DEF58A73F17C37333E54", "HearthFires_Patch.vcdiff" },
        { "Dragonborn.esm",              "58298DD4D7F98D0AF48A4C0BFEFCD6026E9F8040", "0046BBFF6AB4CFB101E74B7122553E673c412284", "Dragonborn_Patch.vcdiff" },
        { "Dawnguard.esm",               "06A27B47A3D395DF1EE53700DFD16116A7639215", "E47E6D8D8DF6A2E9716AA2D884FE6E9FB07A50C3", "Dawnguard_Patch.vcdiff" },
        { "ccQDRSSE001-SurvivalMode.esl", "7A1C720D687C58358F7E703D6996901EB35304CF", "756FFBF3671893AA5FE4A829111F7B6908E215D6", "ccQDRSSE001-SurvivalMode_Patch.vcdiff" },
        { "ccBGSSSE025-AdvDSGS.esm",     "1EA46821E843D9804E5B19962DA4D00121EFC4AC", "84A7942ACB3814FA029724A99AD0C41D94D9D326", "ccBGSSSE025-AdvDSGS_Patch.vcdiff" },
        { "ccBGSSSE001-Fish.esm",        "3BB605C3F110702790838C9D13A6490E5D5096A8", "F76263B8D957732AF58373241F93B48DF13E5E4C", "ccBGSSSE001-Fish_Patch.vcdiff" }
    };

    fs::path backupDir = "Original ESMs backups";
    fs::create_directories(backupDir);

    std::cout << "Starting Skyrim ESM Patcher...\n\n";

    for (const auto& plugin : plugins) {
        std::cout << "Checking " << plugin.filename << "... ";

        if (!fs::exists(plugin.filename)) {
            std::cout << "Not found, skipped.\n";
            continue;
        }

        std::string currentHash = SHA1Out(plugin.filename);

        if (currentHash == plugin.originalHash) {
            if (!fs::exists(plugin.patchFile)) {
                std::cout << "FAILED (Patch missing).\n";
                continue;
            }

            std::cout << "Needs patch.\n -> Patching... ";
            fs::path backupPath = backupDir / plugin.filename;
            fs::rename(plugin.filename, backupPath);

            std::string cmd = "xdelta3.exe -d -vfs \"" + backupPath.string() + "\" \"" + plugin.patchFile + "\" \"" + plugin.filename + "\"";

            if (system(cmd.c_str()) == 0) {
                std::cout << "Success!\n";
                fs::remove(plugin.patchFile);
            }
            else {
                std::cout << "FAILED (xdelta3 error).\n";
                fs::rename(backupPath, plugin.filename);
            }
        }
        else if (currentHash == plugin.cleanedHash) {
            std::cout << "Already cleaned.\n";
            fs::remove(plugin.patchFile);
        }
        else {
            std::cout << "Unknown hash: " << currentHash << ". Aborted.\n";
        }
    }

    std::cout << "\n"
        << "Process finished.\n"
        << "Press ENTER to clean up and close.";

    std::cin.ignore();

    fs::remove("xdelta3.exe");

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