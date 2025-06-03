### **Sigma.Build Change Log**


#### **Version 0.00.03**  -- _2025-06-02_
Expand target functionality with multiple target configuration and command-line target execution.

- Multi-target configuration: multiple target configurations
- Specify target from command line:
  - `--build <config>.json[:target]`
- Specify default target
- Introduced `op` type targets for command execution

**JSON** _Example_:  
``` json
  "targets": [
    {
      "name": "sigbuild",
      "type": "exe",
      "sources": [

      ],
      "build_dir": "{BLD_DIR}/",

      "out_dir": "{BIN_DIR}/"
    },
    {
      "name": "clean",
      "type": "op",
      "commands": {
        "linux": [
          "rm {BLD_DIR}/*"
        ],
        "windows": [
          "del {BLD_DIR}\\*"
        ]
      }
    }
  ],
  "default_target": "sigbuild"

```

-----  
#### **Version 0.00.02**  -- _2025-06-01_  

Offical start of change log. **Sigma.Build** is operable without any known bugs.
Core features:
- self-hosting: `sigma.build` was designed to be self-building.
- CLI flags/options:
  - `--build <config>.json`: configuration file
  - `--about`: about version information
  - `--log=(0-2)`: logging level - `NONE`, `NORMAL`, `VERBOSE`
  - `--help`: display usage information

**JSON configuration**:  
- variable annotation:  
``` json
"variables": {
   "CORE": "src/core/",
   "BLD_DIR": "build/"
}
```  
- single target although JSON annotation is array (multi-target next)  
``` json
"targets": [
   {
      "name": "sigbuild",
      "type": "executable",
      "sources": [
        "{CORE}cli_parser.c",
        "{CORE}var_table.c",
        "{CORE}loader.c",
        "{CORE}builder.c",
        "src/sigbuild.c",
        "src/main.c",
        "lib/cjson/cJSON.c"
      ],
      "output_format": "c_source",
      "build_dir": "{BLD_DIR}",
      "compiler": "gcc",
      "compiler_flags": [
        "-Wall",
        "-O2",
        "-c",
        "-Iinclude",
        "-Ilib/cjson"
      ],
      "linker_flags": [],
      "input_formats": [
        "c_source"
      ],
      "max_file_size_mb": 10
   }
]
```  
- other JSON fields:
``` json
"name": "bootstrap_build",
"log_file": "logs/sigma_build.log",
"incremental_build": false,
"parallel_jobs": 1,
"default_target": "sigbuild"
```  
