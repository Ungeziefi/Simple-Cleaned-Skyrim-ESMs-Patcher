Nexus Mods page [here](https://www.nexusmods.com/skyrimspecialedition/mods/185743).

**Building**

```sh
uv sync
uv run pyinstaller --onefile --icon=icon.ico "Simple Cleaned Skyrim ESMs Patcher.py"
```

---

**Usage**  
Have the "Delta Patches" folder next to the `.py` or the `.exe`. Both the folder and the program should be in the same folder as ESMs to patch.