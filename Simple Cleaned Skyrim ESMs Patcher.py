import hashlib
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

PATCHES = {
    # Steam 1.6
    "06ae741c9e11f765e4f902d1f8813cd6d9860a90": ("052fa4563e969ac633ee18b0f64da8e2777bbd87", Path("Delta Patches") / "Steam 1.6" / "Update_Steam_1.6.vcdiff"),
    "4951925a1956853bb0dbda3fbf40a874b1eb3a5d": ("a7d35871f26b7467ac64c22cf6804db762964a78", Path("Delta Patches") / "Steam 1.6" / "HearthFires_Steam_1.6.vcdiff"),
    "58298dd4d7f98d0af48a4c0bfefcd6026e9f8040": ("4df0937bb41986b0dfbfa8782ae8b04e59a301cb", Path("Delta Patches") / "Steam 1.6" / "Dragonborn_Steam_1.6.vcdiff"),
    "06a27b47a3d395df1ee53700dfd16116a7639215": ("8989fe77bf1379aaffae50aae469c391d159abad", Path("Delta Patches") / "Steam 1.6" / "Dawnguard_Steam_1.6.vcdiff"),

    # Steam 1.7
    "3a57a962c39588a4dfdd3193224063a20e03b450": ("80464c3c412e762e97ef1e8afa3fc5e2d8816d91", Path("Delta Patches") / "Steam 1.7" / "Update_Steam_1.7.vcdiff"),
    "c5bef0e60a237a2f0a12beeedd57067411d6d017": ("ca7d511eba41602de6dd78ecb67cf9abae1f0f62", Path("Delta Patches") / "Steam 1.7" / "HearthFires_Steam_1.7.vcdiff"),
    "e5ba131cb443bd53c3898d751e538422678b6450": ("f8283b9c9fb3fcf5a7a0cda1b2653603262d80bf", Path("Delta Patches") / "Steam 1.7" / "Dragonborn_Steam_1.7.vcdiff"),
    "ddaa3f01e177dce2d293e3d08e3513da62ce4710": ("0a2d397db68a1a1653fee3436537f9e2329dcdff", Path("Delta Patches") / "Steam 1.7" / "Dawnguard_Steam_1.7.vcdiff"),

    # GOG
    "52daff7cad6164d19c8399e97b753a871e64b568": ("eb86bcda893e2e9d0bd789e498d4cd889dcc3951", Path("Delta Patches") / "GOG" / "Update_GOG.vcdiff"),
    "263ba1429dffd30e4b4e4bf063253c44d1300479": ("96f490744ffe0194773e36970019e35363e2adab", Path("Delta Patches") / "GOG" / "HearthFires_GOG.vcdiff"),
    "8e493e55cba561cc7ddb959dfb556a6706a8c3c7": ("a3bebacbdf230e27ec3122c6be6c553318baf3bb", Path("Delta Patches") / "GOG" / "Dragonborn_GOG.vcdiff"),
    "9b19505fbc967e8ba50960e81faf96700cf5f162": ("986dc65e1612c9cf95f4687c17ec23c685db3ece", Path("Delta Patches") / "GOG" / "Dawnguard_GOG.vcdiff"),

    # CC
    "7a1c720d687c58358f7e703d6996901eb35304cf": ("27e0f9b404a870e7b583498d140887e29153e300", Path("Delta Patches") / "Shared" / "ccQDRSSE001-SurvivalMode.vcdiff"),
    "1ea46821e843d9804e5b19962da4d00121efc4ac": ("53913cff5e0fc1bd0afa742074efe4a5527bf7ca", Path("Delta Patches") / "Shared" / "ccBGSSSE025-AdvDSGS.vcdiff"),
    "3bb605c3f110702790838c9d13a6490e5d5096a8": ("ad170ffb8d675c6556d33b53ad60de913b55cb53", Path("Delta Patches") / "Shared" / "ccBGSSSE001-Fish.vcdiff"),
}

def get_sha1(file_path: Path) -> str:
    hasher = hashlib.sha1()
    with open(file_path, "rb") as f:
        while chunk := f.read(65536):
            hasher.update(chunk)
    return hasher.hexdigest()

def main():
    app_dir = Path(sys.executable if getattr(sys, 'frozen', False) else __file__).parent
    xdelta_exe = app_dir / "xdelta3.exe"

    if not xdelta_exe.exists():
        print("Error: xdelta3.exe not found. Make sure to extract everything from the downloaded archive.")
        print()
        input("Press ENTER to exit...")
        return

    plugin_files = [f for f in app_dir.iterdir() if f.suffix.lower() in ('.esm', '.esl', '.esp')]

    for file_path in plugin_files:
        current_hash = get_sha1(file_path)

        if current_hash not in PATCHES:
            continue

        expected_hash, rel_patch_path = PATCHES[current_hash]
        patch_path = app_dir / rel_patch_path

        if not patch_path.exists():
            print(f"{file_path.name}: Patch missing at '{rel_patch_path}'.")
            continue

        print(f"Patching {file_path.name}...")

        with tempfile.TemporaryDirectory() as temp_dir:
            temp_out = Path(temp_dir) / file_path.name

            cmd = [str(xdelta_exe), "-d", "-s", str(file_path), str(patch_path), str(temp_out)]
            res = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, creationflags=subprocess.CREATE_NO_WINDOW)

            if res.returncode == 0 and temp_out.exists():
                if get_sha1(temp_out) == expected_hash:
                    shutil.move(temp_out, file_path)
                    print(f"-> {file_path.name}: Patched successfully!")
                else:
                    print(f"-> {file_path.name}: Output hash mismatch!")
            else:
                print(f"-> {file_path.name}: xdelta3 error ({res.stdout.strip()}).")

    print()
    input("Patching successful. Press ENTER to close...")

if __name__ == "__main__":
    main()