let
  pkgs = import
    (fetchTarball "https://github.com/NixOS/nixpkgs/archive/nixos-21.11.tar.gz")
    { };
  elfio = with pkgs;
    stdenv.mkDerivation {
      pname = "elfio";
      version = "3.9";
      srcs = builtins.fetchGit {
        url = "https://github.com/serge1/ELFIO.git";
        ref = "refs/tags/Release_3.9";
      };
      nativeBuildInputs = [ cmake ];
    };
  termcolor = with pkgs;
    stdenv.mkDerivation {
      pname = "termcolor";
      version = "v2.0.0";
      srcs = builtins.fetchGit {
        url = "https://github.com/ikalnytskyi/termcolor.git";
        ref = "refs/tags/v2.0.0";
      };
      nativeBuildInputs = [ cmake ];
    };

in with pkgs;
stdenv.mkDerivation {
  pname = "libtree";
  version = "2.0.0";
  srcs = nix-gitignore.gitignoreSource [ ] ./.;
  nativeBuildInputs = [ cmake makeWrapper ];
  buildInputs = [ cxxopts elfio termcolor ];
  checkInputs = [ gtest ];
  doCheck = true;
  cmakeFlags = [ "-DLIBTREE_BUILD_TESTS=ON" ];

  postInstall = ''
    wrapProgram $out/bin/libtree --suffix PATH : "${
      lib.makeBinPath [ binutils chrpath ]
    }"
  '';

  meta = with lib; {
    homepage = "https://github.com/haampie/libtree";
    description =
      "ldd as a tree with an option to bundle dependencies into a single folder";
    maintainers = with maintainers; [ fzakaria ];
    license = licenses.mit;
    platforms = platforms.unix;
  };
}
