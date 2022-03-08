# SuperTuxKart-TAS-experimental

An experimental set of TAS creation tools for SuperTuxKart. This was made as part of a class project with a limited timeframe, and as such has some limitations.

## Usage:

This is designed *very specifically* for the [v1.3 official Windows release](https://supertuxkart.net/Download) of the game. This will not work for any other version.

(Keep the Injector.exe and Payload.dll in the same directory as parser.py.)

1. Download the zip from the [releases page](https://github.com/UncraftedName/SuperTuxKart-TAS-experimental/releases) and unzip it.
2. Open up the game.
3. Execute TAS scripts from command line:

    3.1) Get [python](https://www.python.org/downloads/) (at least version 3.8) and make sure it's in your PATH.

    3.2) Open up a e.g. powershell window in the same directory as parser.py by shift+right clicking in that folder.

    3.3) Run TAS scripts from the command line like so: `parser.py -p "scripts\sample_script.peng"`.

Check out the [TAS syntax doc](https://docs.google.com/document/d/1l9Jg-ELLlUAnMihQhPJFEH2yhZ2HhizQygtwTfeNIbs/edit?usp=sharing), see the README in the download for any clarifications on stuff that might not work.

You can unload the dll from the game by running unload.py.

## Building and Coding

This project uses visual studio 2022 and python v3.8. Open up the project and set the default startup project as 'Injector'. The injector will inject payload.dll into the game. If you would like to debug anything that happens in the payload then launch the game, run the injector (not necessarily from vs), and attach vs to supertuxkart.exe. This allows you to set breakpoints and stuff like that.

The parser communicates with the payload via IPC, here are the [docs](https://docs.google.com/document/d/1BfxaQ6Ansk1bdIQwlF5HifvIgDqkZBVnWTprWw1O5Vs/edit?usp=sharing) for the specifics of the message format.

If you run the injector, then you'll have to unload it from the game before rebuilding the project. This can be done with unload.py.

## Inspiration

This project was heavily inspired by the [TAS tools made for Portal 1](https://github.com/YaLTeR/SourcePauseTool). It also uses MinHook and a very similar TAS scripting syntax.
