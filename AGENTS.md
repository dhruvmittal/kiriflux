# Project Intelligence: KDE Kirigami + NixOS

## Agent Workflow (Token Optimization)
- **High-Reasoning Phase (Pro/Ultra/Claude Opus):** - Define C++ class hierarchies and D-Bus interfaces.
  - Architect Nix flake outputs and devShell dependencies.
  - Strategy for `googletest` or `QtTest` coverage.
- **Fast Execution Phase (Flash):**
  - Implement `.cpp` and `.h` boilerplate.
  - Write QML UI layouts using Kirigami components.
  - Update `CMakeLists.txt` for new modules.
  - Run `nix-shell --run "cmake --build build"` to verify syntax.

## Technical Context
- **Build System:** CMake + Nix Flakes (`nix develop`).
- **Frameworks:** KDE Frameworks 6 (KF6), Kirigami, Qt 6.
- **Testing:** Strict 80%+ code coverage requirement. Use `LCOV` or `gcov`.
- **Modularity:** - Logic must be separated into `src/core` (library) and `src/ui` (QML/View).
  - Use `add_library` for core logic to facilitate unit testing without the GUI.

## Execution Rules
1. **Plan First:** Always update `implementation_plan.md` before touching C++ files.
2. **Nix-Native:** Never use `sudo apt` or global paths. All dependencies must be in `flake.nix`.
3. **Small Context:** When implementing, focus on one CMake target at a time to minimize token churn.
