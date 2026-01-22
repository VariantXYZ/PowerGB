target="${1:-default}"
script_dir=$(realpath $(dirname $0))

make $target --always-make --dry-run \
 | grep -wE 'c\+\+|clang|gcc|g\+\+' \
 | grep -w '\-c' \
 | jq -nR '[inputs|{directory:"'$script_dir'", command:., file: match(" [^ ]+$").string[1:]}]' \
 > compile_commands.json
