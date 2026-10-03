# epaper

Empty reMarkable 2 shell: CMake, the Docker SDK build, and a Qt `main` that
starts `QGuiApplication` and does nothing else.

The previous application, including drawing and the local SDK installer, is
in `epaper_old/` until a later story rebuilds that behavior here.

```bash
./scripts/build.sh
./scripts/deploy-rm2.sh
```

See [TOOLCHAIN.md](TOOLCHAIN.md) for where the SDK `.sh` lives.
