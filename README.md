## Wrapper library
* wrapper library that can be used to wrap any function from any library

## Dependencies
* [Gotcha](https://gotcha.readthedocs.io/en/latest/#). Gotcha is an API that provides function wrapping.
* JANSSON 2.12 or higher
* openssl-1.1.0a or higher 
* [rabbitmq-c](https://github.com/alanxz/rabbitmq-c) 0.9.0 or higher 
 

## How to use
* * To get started go module/. You should create a configuration file with functions that needs to be wrap and other necessary parameters. More details in module/

* Once a configuration file has been created run ``python parser yourmodulename``. This will create necessary files and Makefile in $libwrap home. 

* Install wrapper
```
module load openssl/1.1.0a
export GOTCHA=path_gotcha_install
export LD_LIBRARY_PATH=/usr/common/software/rabbitmq/0.9.0/lib64:$LD_LIBRARY_PATH 
export JANSSON=/path/to/c/jannson
make 
```
* This step would create yourmodulename.so and yourmodule.a. You should now link the library to your application when executing for example LD\_PRELOAD=path/to/yourmodule.so:path/to/libgotcha.so ./myapp. See test/ for more examples.

