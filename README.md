<h1 align="center">
  Modernized Latex
</h1>

<p align="center">
  <a href="https://en.cppreference.com/w/cpp/compiler_support">
    <img alt="C++ Standard" src="https://img.shields.io/badge/C%2B%2B-20-blue?logo=c%2B%2B"
  ></a>
  <a href="https://github.com/aveloux/latex/actions">
    <img alt="Build Status" src="https://img.shields.io/github/actions/workflow/status/aveloux/latex/build.yml?branch=main&label=build"
  ></a>
  <a href="https://github.com/aveloux/latex">
    <img alt="Platforms" src="https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey"
  ></a>
  <a href="https://github.com/aveloux/latex/stargazers">
    <img alt="GitHub Stars" src="https://img.shields.io/github/stars/aveloux/latex?color=239dad&label=stars"
  ></a>
  <a href="https://github.com/aveloux/latex/issues">
    <img alt="GitHub Issues" src="https://img.shields.io/github/issues/aveloux/latex?color=5865F2&label=issues"
  ></a>
  <a href="https://github.com/aveloux/latex/pulls">
    <img alt="PRs Welcome" src="https://img.shields.io/badge/PRs-welcome-A561FF"
  ></a>
  <a href="https://github.com/aveloux/latex/commits/main">
    <img alt="Last Commit" src="https://img.shields.io/github/last-commit/aveloux/latex?color=success"
  ></a>
  <a href="https://github.com/aveloux/latex/blob/main/LICENSE">
    <img alt="AGPL v3 License" src="https://img.shields.io/badge/license-AGPL%20v3-brightgreen"
  ></a>
</p>

A rewritten typesetting system known as LaTeX, celebrated for its professionalism and written in C++ for maximum compatibility and speed.

We all love and share the LaTeX engine for typesetting documents. Although standard LaTeX provides a high-level descriptive markup language, it is known for being slow and unforgiving when writing documents. This is largely because of its century-old macro-processing mechanism that inherently limits execution speed, alongside consuming excessive engine space. 

LaTeX+ isn't just a copy of LaTeX—though that was the original plan—but rather a rewritten version that keeps the syntax and language intact while providing a minimalist footprint and unparalleled execution speed compared to alternative modern systems like [Typst](https://github.com/typst/typst). It is engineered for maximum capacity and security against attacks, supporting robust document generation while offering the comfortable stability of traditional LaTeX—surpassing traditional word processors like [Microsoft Word](https://en.wikipedia.org/wiki/Microsoft_Word) in mathematical typesetting fidelity. Nothing fundamental changes; rather, the approach itself has evolved, so senior authors can keep their hopes and dreams alive.

## Installation
mLatex's CLI is available from various sources:

- You can get the source code and pre-built binaries for the latest release of mLaTeX from the releases page. Download the archive for your platform and place it in a directory that is in your PATH. To stay up to date with future releases, you can simply run `mlatex update`.
- You can install mLaTeX through different package managers. Note that the versions in the package managers might lag slightly behind the latest release.
  - Linux:
  - macOS:
  - Windows:
    
  > Note: No packages have been released at the moment.

## Usage
Once you correctly set up mLatex, you can run it via the command line:

```sh
# Get help information from the CLI
mlatex --help

# Check the installed version
mlatex --version

```

You can also compile your PDF directly just by providing an input and output file:

```sh
mlatex compile --input input.mtex --output output.pdf # --jit

```

> By enabling the `--jit` parameter, you have now set the engine to compile Just-In-Time.

## Community & Support

The main place where the community gathers is our Discord server. It's a great place to ask quicker questions, discuss contributing, or just chat. We'd be happy to see you there!

* **Join the Discord:** https://discord.gg/null

As the sole maintainer of this project, I want to ensure everyone has a good experience. If you have questions, need to discuss project direction, or had a bad experience in the community, please reach out to me directly:

* **Contact:** Andres Hernandez at andromedeyz@hotmail.com

## Contributing

We love to see contributions from the community. If you experience bugs, feel free to open an issue. If you would like to implement a new feature or bug fix, please follow the steps outlined in our [contribution guide](https://www.google.com/search?q=CONTRIBUTING.md).

To build mLatex yourself, first ensure that you have a C++20 compatible compiler and [CMake](https://cmake.org/) installed. Then, clone this repository and build the CLI with the following commands:

```sh
git clone [https://github.com/aveloux/latex](https://github.com/aveloux/latex)
cd latex
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .

```

## Acknowledgements

I'd like to acknowledge the Vatican for their profound inspiration. Although it might seem random, their organization, timeless art, and ancient architecture inspired me to make this tool, approaching its design as if it were a god-given mission.

```
