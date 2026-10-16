#!/bin/sh
# CI only (docs/development/ci.md): the last step of every job with the bundle, run whatever
# happened before it. Stops the Wine servers, then removes the decrypted bundle, the clone, the key
# files, the Wine prefixes, what the set-up wrote into the trees and everything built (build/).
# Run from the repository root.
t=${RUNNER_TEMP:?}
for p in "$t"/forgiv-wine/* inverters/gen3-lv/*/build/gate/wine/* inverters/gen3-lv/*/build/wine/*; do
  [ -f "$p/system.reg" ] && WINEPREFIX=$p wineserver -k >/dev/null 2>&1
done
rm -rf "$t/forgiv-assets" "$t/forgiv-assets.age" "$t/forgiv-ci-assets" "$t/forgiv-deploy-key" \
       "$t/forgiv-known-hosts" "$t/forgiv-wine" "$t/modbuild.log" \
       inverters/gen3-lv/a316/build inverters/gen3-lv/d316/build \
       inverters/gen3-lv/a316/lib/stdperiph35 \
       inverters/gen3-lv/a316/toolchain.local.toml inverters/gen3-lv/d316/toolchain.local.toml
echo "cleanup: done"
