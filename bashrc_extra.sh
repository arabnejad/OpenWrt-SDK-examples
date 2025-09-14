
export PS1='\[\e[48;5;236;38;5;250m\][DOCKER-OPENWRT-SDK]\[\e[0m\]\[\e[36m\][\u]\[\e[0m\]\[\e[32m\][\w]\[\e[0m\]\n\$ '
export nproc=4

alias quilt='/usr/bin/quilt'

use_sdk() {
    local sdk="$1"
    if [ ! -d "$sdk" ]; then
        echo "No such SDK: $sdk" >&2;
        return 1;
    fi

    export SDK_DIR="$sdk"
    export STAGING_DIR="$sdk/staging_dir"
    export PATH="$SDK_DIR/staging_dir/toolchain-*_gcc-*_musl/bin:$PATH"

    alias cd_home='cd $SDK_DIR'

    echo "Now in SDK   = $SDK_DIR"
    echo "STAGING_DIR  = $STAGING_DIR"
    echo "PATH         = $PATH"
    cd $SDK_DIR

    # Rebuild feeds.conf
    local local_feeds_conf="/work/local-feeds/local_feeds.conf"
    # cat "$SDK_DIR/feeds.conf.default" "$local_feeds_conf" > "$SDK_DIR/feeds.conf"
    cat "$local_feeds_conf" > "$SDK_DIR/feeds.conf"

    ./scripts/feeds update -a && \
    ./scripts/feeds update -f local_feeds && \
    ./scripts/feeds install -a -p local_feeds && \
    echo "Local feed wired into $SDK_DIR/feeds.conf"

}
