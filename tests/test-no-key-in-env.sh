#!/usr/bin/env bash
# cc teamclaude must never write a proxy client key into env.sh (row d3d3cc19; katana leak 2026-11-17, row 438265ac):
# every host runs its own loopback forwarder on 127.0.0.1:3456 that holds the key in a 0600 file, so a key in env.sh
# sits in every process env and any tool's error text can print it. Runs in a throwaway HOME; no network.
set -uo pipefail
here=$(cd "$(dirname "$0")/.." && pwd)
pass=0; fail=0
ok()  { echo "  PASS  $1"; pass=$((pass + 1)); }
bad() { echo "  FAIL  $1"; fail=$((fail + 1)); }
KEY=tck_FAKEKEY_d3d3cc19

run_case() {  # $1 label, $2 mitm flag, $3 remote.env body ("" = no file), $4 "cli" or "cached"
  local label=$1 flag=$2 remote=$3 how=$4
  local h; h=$(mktemp -d "${TMPDIR:-/var/tmp}/cc-nokey.XXXXXX")
  mkdir -p "$h/.config/claude-code" "$h/.claude" "$h/bin"
  echo '{"model":"x"}' > "$h/.claude/settings.json"
  local envout='export HTTPS_PROXY=http://127.0.0.1:3456
export HTTP_PROXY=http://127.0.0.1:3456
export NODE_EXTRA_CA_CERTS='"'"'/home/u/.config/teamclaude-ca.pem'"'"'
unset ANTHROPIC_BASE_URL'
  [ -z "$flag" ] && envout='export ANTHROPIC_BASE_URL=http://localhost:3456
export ANTHROPIC_API_KEY=placeholder'
  if [ "$how" = cli ]; then
    printf '#!/bin/sh\ncat <<"X"\n%s\nX\n' "$envout" > "$h/bin/teamclaude"; chmod +x "$h/bin/teamclaude"
  else
    printf '%s\n' "$envout" > "$h/.config/claude-code/teamclaude.env"
  fi
  [ -n "$remote" ] && printf '%s\n' "$remote" > "$h/.config/claude-code/teamclaude-remote.env"
  local out; out=$(HOME=$h PATH="$h/bin:/usr/bin:/bin" CC_AUTO_SOURCE=1 bash "$here/cc" teamclaude $flag 2>&1)
  local env="$h/.config/claude-code/env.sh"
  if [ ! -s "$env" ]; then bad "$label: env.sh written"; rm -rf "$h"; return; fi
  if grep -q "$KEY" "$env"; then bad "$label: no client key in env.sh"; else ok "$label: no client key in env.sh"; fi
  if grep -qE '://[^/@ ]+@' "$env"; then bad "$label: no userinfo in any URL"; else ok "$label: no userinfo in any URL"; fi
  if grep -qE '127\.0\.0\.1:3456|localhost:3456' "$env"; then ok "$label: points at the loopback forwarder"
  else bad "$label: points at the loopback forwarder ($(grep -E 'PROXY|BASE_URL' "$env" | head -2 | tr '\n' ' '))"; fi
  if printf '%s' "$out" | grep -q "$KEY"; then bad "$label: key not printed"; else ok "$label: key not printed"; fi
  echo "$env" > "$h/.envpath"; LAST_HOME=$h
}

R1="TC_REMOTE=10.0.6.107:3443
TC_KEY=$KEY
TC_SCHEME=https
TC_CA=/home/u/.config/teamclaude-ca-bundle.pem"
run_case "mitm, cli, remote.env present" --mitm "$R1" cli
grep -q 'teamclaude-remote.env' "$(cat "$LAST_HOME/.envpath")" && ok "remote.env present: env.sh says it is ignored" || bad "remote.env present: env.sh says it is ignored"
grep -q '^export NODE_EXTRA_CA_CERTS=/home/u/.config/teamclaude-ca-bundle.pem' "$(cat "$LAST_HOME/.envpath")" \
  && ok "TC_CA still names the CA bundle (a path, not a secret)" || bad "TC_CA still names the CA bundle"
rm -rf "$LAST_HOME"
run_case "mitm, cached env, remote.env present" --mitm "$R1" cached; rm -rf "$LAST_HOME"
run_case "reverse, cli, remote.env present" "" "$R1" cli; rm -rf "$LAST_HOME"
run_case "mitm, cli, no remote.env" --mitm "" cli
grep -q 'teamclaude-remote.env' "$(cat "$LAST_HOME/.envpath")" && bad "no remote.env: no notice" || ok "no remote.env: no notice"
rm -rf "$LAST_HOME"
echo "$pass passed, $fail failed"
[ "$fail" = 0 ]
