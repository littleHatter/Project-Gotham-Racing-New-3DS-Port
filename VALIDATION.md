# Version 1.3 validation

The user confirmed the v1.2-based software ghosting test on a New Nintendo 3DS with no extra delay. Version 1.3 uses the same software rendering and blend implementation with the supplied touchscreen selection artwork. The final touch prompt was cross-compiled and its framebuffer composition was verified offline.

# Version 1.2 validation

The install archives use the exact Paris Time Trial Test 3 executable.
On New 3DS hardware, the guard intercepted invalid sound references during
Corvette Paris Time Trial, the race reached gameplay, and the game exited
cleanly. The log also confirmed the title cue during the opening splash.
The invalid-reference source remains unresolved.

# Version 1.0 validation

The release application is byte-for-byte identical to the hardware-confirmed Test 5 application. Packaging does not change gameplay, rendering, audio or save behavior.

Hardware feedback confirmed the game running across career/time trial and varied events, corrected sound effects and controls, bottom-screen artwork/prompts, soundtrack timing/fades/volume, title music at the PGR splash, and continuous music during loading. A complete end-to-end playthrough and every HOME/sleep scenario are not claimed here.

Prior implementation checks passed: runtime configuration, finite/infinite sound continuation, sample replacement and stop handling, relocated control labels, music looping/fades/one-shot results silence/volume/mute/pause, bottom rendering, and a modeled NDSP worker running without game callbacks. The final desktop gameplay regression ran 1,400 frames including pause/resume, and the native build passed.

Release packaging checks compare all 12 newly extracted game files against the working installation. The standalone extractor also passes repeated runs, conflict rejection, explicit repair, save/unrelated-file preservation, rejected unsupported/truncated input, and paths containing spaces/non-ASCII characters.

The source archive has no SIS/SISX packages, prepared game payloads, soundtrack WAVs, saves or compiled artifacts. The extractor installation contains application files, soundtrack and extractor, but no Symbian-derived game payloads. The complete installation contains the identical application/music plus the 12 verified game files, without saves.
