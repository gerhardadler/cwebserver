# cwebserver

This is a simple webserver I've created to learn the basics of C.

## Dependencies

- GNU Make
- GCC

```
sudo apt update
sudo apt install build-essential
```

## Build

```
make
```

## Usage

```
./webserver
  -h            host, for example 0.0.0.0 (default=localhost)
  -p            port, for example 80 (default=8080)
  -d            directory, the directory to be served (default="./")
```
