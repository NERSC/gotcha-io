## Gotcha-IO
* IO analysis with Gotcha for dynamically linked apps
* Automatic tracing with light overhead at scale
* Initial goal is to trace HDF5, NetCDF, Root, POSIX, etc, any I/O related functions calls
## How to use
* Load compiler first for gotcha installation:
```
module load PrgEnv-intel  #for loading Intel compiler #unload other first. Example: module unload PrgEnv-compiler
module swap PrgEnv-intel PrgEnv-cray #for swapping to Cray compiler
module swap PrgEnv-intel PrgEnv-gnu #for swapping to GNU compiler
```

* Install Gotcha, export the path, thanks Elsa Gonsiorowski @llnl 
```
git clone https://github.com/llnl/gotcha
cd gotcha
mkdir build install
cd build
cmake ../ -DCMAKE_INSTALL_PREFIX=../install
make
make install
export GOTCHA=path_gotcha_install
cd ../..
```

* Install HDF5 tracer
```
git clone https://github.com/NERSC/gotcha-io.git
cd gotcha-io
module load cray-hdf5
make 
```


* Test HDF5 apps (Test with the compiler that gotcha uses) 
```
cd test
source test.sh #issue: if source does not work, try bash
```
## [Gotcha](https://gotcha.readthedocs.io/en/latest/#) is an API that provides function wrapping.

## [Gotcha-Tracer](https://github.com/llnl/GOTCHA-tracer) is a Python tool for creating tracer with Gotcha
