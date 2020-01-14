{ stdenv, fetchFromGitHub, fetchpatch, maven
}:
stdenv.mkDerivation {
  name = "kaa-0.9.0";

  src = fetchFromGitHub {
    owner = "jbt-iot";
    repo = "kaa";
    rev = "6c5bddd2200951870949b381f4a335e83c74a45f";
    sha256 = "0ypa1vfyzsc07xpsi3l2ji18x25yjr8lf3l9rgjf7844vp6wxg80";
  };

  nativeBuildInputs = [
    maven
  ];

  # This build requires network, so does not work with sandboxing.
  __noChroot = true;

  buildPhase = ''
    export M2_REPO=$TMPDIR/repository
    mvn -P compile-gwt,cassandra-dao,postgresql-dao,kafka clean install verify -DskipTests -Dmaven.javadoc.skip=true -Dmaven.repo.local=$M2_REPO
  '';

  installPhase = ''
    mkdir $out/
    cp server/node/target/kaa-node.deb $out
  '';
}
