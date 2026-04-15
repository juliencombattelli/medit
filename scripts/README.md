## Build scripts

These shell scripts are provided as a convenience for Linux and MinGW builds.
They are **not required**. The CMake build works out of the box via FetchContent
without any additional tooling.

### Why use these scripts over FetchContent?

Short answer: if you are a developer working on Medit.

Long answer:
- **Persistent build artifacts.** Dependencies are downloaded, built and
  installed in a directory *next to* the Medit source tree, outside of the Medit
  build directory. This means wiping or recreating the Medit build directory
  does **not** require rebuilding the dependencies from scratch.
- **Common CI and Dev workflows.** The CI pipelines already use those scripts.
  So when using these scripts for local development the build workflow becomes
  the same for development and CI, facilitating the integration and
  investigation of build issues.
- **Faster CI builds.** The dependency install directory can be cached between
  CI runs, avoiding redundant network fetches and rebuilds.
- **Stable, known-good dependencies.** The scripts, like FetchContent, fetch
  specific versions of each dependency that are Known-To-Work™ with this
  codebase.

### When use FetchContent only

Short answer: if you are an end-user.

Long-answer:
- **Quick and easy builds.** For an end-user it may be more convenient to just
  use the idiomatic CMake commands to build the project.
- **Minimal tool required.** The scripts additionally require Bash compared to
  FetchContent, which may not be available on all platform.
