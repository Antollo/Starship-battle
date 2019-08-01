# Starship battle

[![https://ci.appveyor.com/api/projects/status/github/Antollo/Starship-battle?svg=true](https://ci.appveyor.com/api/projects/status/github/Antollo/Starship-battle?svg=true)](https://ci.appveyor.com/project/Antollo/Starship-battle)

Two dimensional game with good physics and command line user interface.

![gif](https://6klyrw.am.files.1drv.com/y4mfIzUr-EFE6NMzC9YdTP9oke6DIIGS9nhERCnWK9xCCbKtil0C5IjAQH_oGh0An9QxXDobxa-Trtf5GCHiZi8RbFAx5fIpzYT_-Kdhjyim_og9fD54hzAySLZ387dxc7h2Zk36UZXwGEMwbZLpc5b7_ZcJausjb2sBUL0uMf5aqXGEfaUxcesjMpaipCF3zF5eVLhX1jZwxKh8Bgeg-_YQw)

## I just want to play...

Download game launcher [here](https://github.com/Antollo/Starship-battle/releases/latest). It will install itself, create shortcut on desktop, perform auto updates and update the game.

### First time?

Click `Play game` in launcher to start the game. Type `create T-4A yourName` (where `T-4A` is spaceship name and `yourName` is pilot's name) and hit enter. Then type `create-bots` and attack them! 
Control the ship with `W`, `A` and `D`, aim with right mouse button and fire with left mouse button! Use `help` command to learn more (e.g. how to list all spaceships names).

_Not recommended_: Latest game executable is available [here](https://ci.appveyor.com/api/projects/antollo/starship-battle/artifacts/src\artifacts.zip?branch=master&job=Image%3A%20Visual%20Studio%202017).

## Development - getting started

### Prerequisites

What do you need:

- `git 2.13` or newer
- `cmake 3.10` or newer
- compiler supporting `c++17`

### Installing

How to get the source code and compile it. Clone the repo:

```
git clone --recurse-submodules https://github.com/Antollo/Starship-battle.git
```

```
cd Starship-battle
```

Compile:

```
cmake .
cmake --build . --config Release
```

Run:

```
cd src/Release
./starship_battle_game server normal port 1717
```
```
cd src/Release
./starship_battle_game ip 127.0.0.1 port 1717
```

### Launcher

Works with `npm 5.6.0` or newer.

```
cd launcher/Starship-battle
npm install
npm install -g electron-forge
electron-forge start
```

## CI

[AppVeyor](https://ci.appveyor.com/project/Antollo/starship-battle)

## Contributing

Just contact me.

## License

TODO

## Credits

- SFML          - [www.sfml-dev.org](https://www.sfml-dev.org/)
- Box2D         - [box2d.org](http://box2d.org/)
- nlohmann/json - [github.com/nlohmann/json](https://github.com/nlohmann/json)
- Electron      - [electronjs.org](https://electronjs.org/)
- Ubuntu Mono   - [fonts.google.com/specimen/Ubuntu+Mono](https://fonts.google.com/specimen/Ubuntu+Mono)
- freesound
  - [freesound.org/people/spaciecat/sounds/456779](https://freesound.org/people/spaciecat/sounds/456779)
  - [freesound.org/people/Diboz/sounds/213925](https://freesound.org/people/Diboz/sounds/213925)
  - [freesound.org/people/ryansnook/sounds/110111](https://freesound.org/people/ryansnook/sounds/110111)
  - [freesound.org/people/debsound/sounds/437602](https://freesound.org/people/debsound/sounds/437602)
- misztsu - help