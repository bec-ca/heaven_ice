#!/bin/bash -eux

MELLOW_VERSION="v0.0.7"

function log {
  echo "$@" 1>&2
}

function build_bootstrap() {
  echo "Downloading and compiling mellow..."
  bootstrap_dir=.bootstrap
  mellow_dir=$bootstrap_dir/mellow-with-deps

  rm -rf $bootstrap_dir
  mkdir $bootstrap_dir
  pushd $bootstrap_dir
  curl -s -L "https://github.com/bec-ca/mellow/releases/download/${MELLOW_VERSION}/mellow-with-deps-${MELLOW_VERSION}.tar.gz" -o mellow-with-deps.tar.gz
  tar -xf mellow-with-deps.tar.gz
  popd

  pushd $mellow_dir
  make -j $(nproc) -f Makefile.bootstrap
  popd

  mkdir -p build
  cp $mellow_dir/build/bootstrap/mellow/mellow $MELLOW
}

function check_mellow {
  if [ -z "$1" ]; then
    return 1
  fi
  if ! "$1" help &> /dev/null; then
    log "Tried mellow at $1 but returned an error"
    return 1
  fi
  echo "$1"
  return 0
}

if check_mellow "$MELLOW"; then
  exit 0
fi

log "No mellow binary found, will build bootstrap"

build_bootstrap 1>&2

if check_mellow "$MELLOW"; then
  exit 0
fi

echo "Bootstrap build returned successfully, but binary not found"
exit 1
