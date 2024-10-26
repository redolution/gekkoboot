{ stdenvNoCC
, callPackage
, lib
, fetchFromGitHub
}:

stdenvNoCC.mkDerivation (finalAttrs: {
  pname = "devkitPPC-rules";
  version = "1.2.1";
  src = fetchFromGitHub {
    hash = "sha256-SnbsyvnEQcHkLTVZBPat0lk+Dr/7OT/MrR036gxGe98=";
    owner = "devkitPro";
    repo = "devkitppc-rules";
    rev = "v${finalAttrs.version}";
  };

  patches = [
    ./0001-Use-generate_compile_commands-from-PATH.patch
    ./0002-Allow-installing-to-another-prefix.patch
  ];

  noConfigurePhase = true;
  noBuildPhase = true;

  __structuredAttrs = true;

  installPhase = ''
    runHook preInstall
    mkdir -p "$out"
    make install PREFIX="$out"
    runHook postInstall
  '';

  meta = {
    description = "Makefile includes for devkitPPC";
    homepage = "https://github.com/devkitPro/devkitppc-rules";
    #license = ???;
    platforms = lib.platforms.unix;
  };
})
