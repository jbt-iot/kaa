{ stdenv
, fetchFromGitHub
, fetchpatch
, maven
}:
stdenv.mkDerivation {
  name = "kaa-c++-sdk";

  src = fetchFromGitHub {
    owner = "kaaproject";
    repo = "kaa";
    rev = "v0.10.0";
    sha256 = "04k73njqb72910jqb10lwm9djgl1k1mdnmba9imllk68v7h2bvq8";
  };

  patches = [
    # http://jira.kaaproject.org/browse/KAA-1390
    (fetchpatch {
      url = "https://github.com/kaaproject/kaa/commit/3771c78ddd4e8b4b74d5d86f6e28c52d4d38aebe.patch";
      sha256 = "0gaa8zs6wx3mlvahzh997702f1l542rjzz4sgvnxz3ilmlni8v6y";
    })

    # https://jbt-iot.atlassian.net/browse/OJI-1246
    # http://jira.kaaproject.org/browse/KAA-1087
    # ./0001-KAA-1087-C-SDK-Bad-padding-in-PKCS7.patch
    #./0002-Fix-http-empty-response.patch
    ./0003-Disable-checkConnectivity.patch
  ];

  nativeBuildInputs = [
    maven
  ];

  # This build requires network, so does not work with sandboxing.
  __noChroot = true;

  buildPhase = ''
    cd client/client-multi/client-cpp
    export M2_REPO=$TMPDIR/repository
    mvn clean install verify -DskipTests -Dmaven.repo.local=$M2_REPO
    cd ../../..
  '';

  installPhase = ''
    mkdir -p $out/lib/kaa-node/sdk/cpp/
    cp client/client-multi/client-cpp/target/client-cpp-0.10.0-cpp-sdk.tar.gz $out/lib/kaa-node/sdk/cpp/kaa-cpp-ep-sdk-0.9.0.tar.gz
  '';
}
