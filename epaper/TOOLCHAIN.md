# Epaper toolchain

The reMarkable SDK installer is a local `.sh` (~400 MB) and is not committed.

Put `remarkable-*-x86_64-toolchain.sh` in `docker/sdk-installer/`.
`./scripts/build.sh` also accepts an installer that is already in
`epaper_old/docker/sdk-installer/` and bind-mounts that directory into the
container. It does not copy the file.

```bash
./scripts/build.sh
./scripts/deploy-rm2.sh
```

The build writes `build/bin/epaper`. Deploy copies that binary to
`root@10.11.99.1:/home/root/epaper` and launches it. Set `RM_SSH_KEY` to the
device private key. Host keys go in `~/.ssh/reawa_rm_known_hosts`, or in the
path given by `RM_SSH_KNOWN_HOSTS`.
