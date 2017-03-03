# OOMalloc

A library meant for testing application behavior in out-of-memory conditions with the use of [LD_PRELOAD trick](http://stackoverflow.com/questions/426230/what-is-the-ld-preload-trick).

## Requirements

* Linux
* glibc - the library contains some glibc-specific code. It will probably not work with alternative libc implementations.

## Compiling

```
cmake . && make
```

## Usage

```
# enable allocation logs
OOMALLOC_LOG=1 LD_PRELOAD=liboomalloc.so ./your-application

# make the 11th allocation return NULL
OOMALLOC_FAIL_AFTER=10 LD_PRELOAD=liboomalloc.so ./your-application

# make malloc/calloc/realloc fail when heap allocations exceed 10000 bytes
OOMALLOC_LIMIT_HEAP=10000 LD_PRELOAD=liboomalloc.so ./your-application
```

## Acknowledgements

Special thanks to [Mateusz Krawiec](https://github.com/sznaider) and [Mateusz Kwiatkowski](http://github.com/kfyatek) for pointing out issues with the library.
