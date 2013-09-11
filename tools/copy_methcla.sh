if [ -z "$1" ]; then
    echo "Usage: `basename $0` METHCLA_SOURCE_DIR"
    exit 1
fi

methcla_src="$1/engine"
build_config="release"

rsync -av "$methcla_src/include/" "libs/methcla/include"
cp "$methcla_src/build/$build_config/iphone-universal/libmethcla.a" "libs/methcla/ios/libmethcla.a"
cp "$methcla_src/build/$build_config/macosx/x86_64/libmethcla-jack.a" "libs/methcla/macosx/libmethcla-jack.a"
