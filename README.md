# Xenon

Cross-platform graphics engine for testing new technologies.

## How to build?

Start off by cloning the repository to a local directory. You can follow the following commands to get started.

```sh
git clone https://github.com/Nexonous/Xenon
cd Xenon
git submodule update --init --recursive
./Scripts/Bootstrap.[bat/ sh]
```

*Note: Use the proper shell script for the platform. For Windows it's the `Bootstrap.bat` file and for Linux it's the `Bootstrap.sh` file.*
*Also the submodule update will also download glTF samples which is like 2 gigabytes.*

The scripts will create the output directory and the CMake project files. From there on it's just a CMake build to build the binaries and run!

## How to render something?

Once you've built the Studio project, run the `XenonStudio` application. Then drag and drop a glTF model file. Then go to View -> Layer View to view the model. Use the W, A, S, and D keys to move in the 3D world.
To rotate the camera, hold down the middle mouse button and then move the mouse. If the movement speeds are slower than necessary, you can edit them in the Configuration widget (enabled through View -> Configuration).

## How to use it with a project?

Take a look into the Studio project's `/Studio/CMakeLists.txt` file to get started. You can easily add this repo as a submodule if you want which will configure
a lot of things for you. Right now we don't have an option to install Xenon (because it's not our biggest concern yet). Just make sure that you set the include path to `/Engine/` and link the `XenonEngine`
static library file (which will be in the `/Build/Engine/Xenon/` folder).

## License

This project is licensed under Apache-2.0.
