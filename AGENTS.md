# Board repository boundary

Own board identity, built-in resources, platform dependencies and hardware I/O.
Keep application protocols, session handling, screens, product names and pairing
decisions outside this repository. Options and callbacks are the composition seam.
Resource numbers live in the profile; gar-tools generates gar_board_config.h and
the PlatformIO board section. Do not copy those numbers into Product bindings.
Keep compilers, firmware binaries, caches and local ports outside Git.
