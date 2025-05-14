# Graduation project

## Building and running
Though you can use just CMake to build this project, it is much easier with nix / nixpkgs.
You can install nix package manager onto WSL or MacOS, for example, with installer from Determinate Systems:
https://determinate.systems/nix-installer/

### How to build
```nix
git clone https://github.com/dtredu/grad-proj.git
cd grad-proj
nix build path:.
```
or simply 

```nix
nix build github:dtredu/grad-proj/main
```

### How to run
First - use nix develop to enter the developer environment, that has all necessary dependencies,
Second - run the buidt program
```
nix develop  github:dtredu/grad-proj/main
./result/bin/main path/to/image
```

