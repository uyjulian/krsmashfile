# krsmashfile

This Kirikiri (2/Z) plugin provides support for smash archives.

## Building

After cloning submodules and placing `ncbind` and `tp_stub` in the parent directory, a simple `make` will generate `krsmashfile.dll`.

## How to use

After `Plugins.link("krsmashfile.dll");` is used, the additional functions will be exposed under the `Storages` class.

## License

This project is licensed under the MIT license. Please read the `LICENSE` file for more information.  
