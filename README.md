# Hazel
Hazel Engine private development repository.

Latest Release: **2024.1**

[![Build](https://github.com/StudioCherno/Hazel/actions/workflows/build.yml/badge.svg?branch=2024.1)](https://github.com/StudioCherno/Hazel/actions/workflows/build.yml)

## Notes
As of 2024.1, Hazel has several branches which form specific releases and in-development versions of the engine. Below is an explanation of the commonly used ones, so you can decide what to use.

### `YYYY.VV` (eg. `2024.1`)
These branches contain QA tested release versions of Hazel. Y = year and V = version, for example **2024.1** is the first release for 2024.

We always recommend using the _latest_ release when starting a new project, or playing around with the engine, unless you specifically want access to in-development, not-yet-completed features that have not been tested and may not be stable.

These release branches will continue to receive bug fixes for a limited time going forward, but new features will be developed in the `dev` branch and be eventually released as part of a new release.

### `dev`
`dev` is our central development branch for future versions of Hazel - all other branches with work-in-progress (eg. feature/fix branches) are merged into this branch before they eventually make it to release. As such, this branch is **not** guaranteed to be stable, however should compile (see CI badge above for current build status).

If you are a user, this branch will provide you with the newest features and fixes, at the risk of being a little unstable. We usually quickly fix serious issues and crashes on this branch, however for serious project development it's advised you only stay on `dev` if you know what you're doing, and are prepared to take that risk.

Otherwise, a release branch (eg. `2024.1`) contains a version of Hazel which has been QA tested and certified stable, and may be the best option for you if you value stability. Our current release schedule is not set in stone, so releases can be few and far between.

So in summary:
- If you want a stable, production-ready version of Hazel - use a release branch such as `2024.1`
- If you want to try the latest features at the cost of stability - use the `dev` branch

 ## Building
 For instructions on how to build Hazel, check out the [Getting Started](https://docs.hazelengine.com/Welcome/GettingStarted) page in Hazel's documentation.