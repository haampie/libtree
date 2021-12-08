let
  pkgs = import
    (fetchTarball "https://github.com/NixOS/nixpkgs/archive/nixos-21.11.tar.gz")
    { };
  elfio = with pkgs;
    stdenv.mkDerivation {
      pname = "elfio";
      version = "3.10";
      srcs = builtins.fetchGit {
        url = "https://github.com/serge1/ELFIO.git";
        rev = "fad30e8f3bb8edc2498c1cf15c02567ee2b7e44b";
      };
      nativeBuildInputs = [ cmake ];
    };
  termcolor = with pkgs;
    stdenv.mkDerivation {
      pname = "termcolor";
      version = "latest";
      srcs = builtins.fetchGit {
        url = "https://github.com/ikalnytskyi/termcolor.git";
        rev = "13f559a970025cd0d5bf9f9077dfa2d618588fbe";
      };
      nativeBuildInputs = [ cmake ];
    };

in with pkgs;
stdenv.mkDerivation {
  pname = "libtree";
  version = "2.0.0";
  srcs = nix-gitignore.gitignoreSource [ ] ./.;
  nativeBuildInputs = [ cmake makeWrapper];
  buildInputs = [ cxxopts elfio termcolor ];

  postInstall = ''
    wrapProgram $out/bin/libtree --suffix PATH : "${
      lib.makeBinPath [ binutils chrpath ]
    }"
  '';

  meta = with lib; {
    homepage = "https://github.com/haampie/libtree";
    description = "ldd as a tree with an option to bundle dependencies into a single folder";
    maintainers = with maintainers; [ fzakaria ];
    license = licenses.mit;
    platforms = platforms.unix;
  };
}
